#include "pch.hpp"
#include "editor_chunk_item_linker_manager.hpp"

#include "worldeditor/world/items/editor_chunk_user_data_object_link.hpp"
#include "worldeditor/world/editor_chunk_item_linker.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "worldeditor/undo_redo/linker_operations.hpp"
#include "worldeditor/editor/chunk_item_placer.hpp"

#include "entitydef/base_user_data_object_description.hpp"
#include "entitydef/data_types.hpp"
#include "entitydef/data_types/sequence_data_type.hpp"

#include "common/array_properties_helper.hpp"
#include "common/properties_helper.hpp"

#include "chunk/chunk_manager.hpp"
#include "chunk/chunk_space.hpp"
#include "chunk/geometry_mapping.hpp"

#include "resmgr/xml_section.hpp"

#include "cstdmf/dogwatch.hpp"

#include "worldeditor/world/items/editor_chunk_user_data_object.hpp"
#include "worldeditor/world/items/editor_chunk_entity.hpp"


BW_BEGIN_NAMESPACE


namespace
{
	// Max time spent in the tick method, to avoid using too much time.
	const int MAX_LINK_PROCESSING_MILLIS = 2;

	DogWatch s_updateOSL( "osl" );
	DogWatch s_updateOSBL( "osbl" );
	DogWatch s_updateCL( "cl" );
	DogWatch s_updateCBL( "cbl" );
	DogWatch s_updateLink( "upd_lnk" );
	DogWatch s_updateChunkLink( "upd_cnk_lnk" );

    /// Predicate functor used to search for an object.
	class SameUniqueId
	{
	public:
		SameUniqueId( UniqueID linkableId ) : linkableId_( linkableId ) {}

		/**
		 *	Predicate function used to find a object with a certain UniqueID.
		 *
		 *	@return	True if the object has the same UniqueID as that stored
		 *			by this class instance.
		 */
		bool operator()( const EditorChunkItemLinkable* pLinkable ) const
		{
			return (pLinkable->guid() == linkableId_);
		}

	private:
		UniqueID linkableId_;
	};

	
	/// Predicate functor used to search for a link.
	class SameLink
	{
	public:
		SameLink( EditorChunkItemLinkableManager::ULPair linkPair ) : linkPair_( linkPair ) {}

		/**
		 *	Predicate function used to find a link.
		 *
		 *	@return	True if the link is the same as the link stored by this class
		 *			instance.
		 */
		bool operator()( const EditorChunkItemLinkableManager::ULPair& linkPair ) const
		{
			return (linkPair.second == linkPair_.second);
		}

	private:
		EditorChunkItemLinkableManager::ULPair linkPair_;
	};
}


//------------------------------------------------------------------------------
//	EDITOR CHUNK LINKER MANAGER SECTION
//------------------------------------------------------------------------------
/**
 *	Default constructor.
 */
EditorChunkItemLinkableManager::EditorChunkItemLinkableManager()
{
}


/**
 *	Called every frame to process links from newly loaded and unloaded chunks.
 */
void EditorChunkItemLinkableManager::tick()
{
	// Four lists are processed by this method:
	//
	// 1)	Outstanding links: These are outgoing links from newly loaded Linkers.
	// 	If the target Linker is loaded, the link is removed from the list and a
	// 	standard link is created.  If it is not loaded, a chunk link is created
	// 	and the link is moved to the chunk links list.
	// 
	// 2)	Outstanding back links: These are incoming links to a newly loaded
	// 	Linker.  If the source object is loaded, the link is simply removed from
	// 	the list since a standard link will have been created by the source
	// 	Linker.  If it is not loaded, a chunk back link is created and the link
	// 	is moved to the chunk back links list.
	// 
	// 3)	Chunk links: These are the chunk links from loaded source Linker objects
	// 	to unloaded target Linker objects.  Target Linkers are checked to see if
	// 	they have been loaded.  If a target is loaded, the chunk link is
	// 	destroyed, a standard link is created, and the link is removed from the
	// 	chunk links list.
	// 
	// 4)	Chunk back links: These are chunk back links from unloaded source Linker
	// 	objects to loaded target Linker objects.  Source Linkers are checked to
	// 	see if they have been loaded.  If a source is loaded, the chunk back
	// 	link is destroyed, a standard link is created (if needed), and the link
	// 	is removed from the chunk back links list.
 
	// Lock mutexes

	BW_GUARD;

	SimpleMutexHolder chunkLinksMutex( chunkLinksMutex_ );
	SimpleMutexHolder chunkBackLinksMutex( chunkBackLinksMutex_ );

	uint64 startTime = timestamp();
	uint64 maxTime =
		(stampsPerSecond() / 1000) *
		Options::getOptionInt(
			"render/links/maxProcessingMillis", MAX_LINK_PROCESSING_MILLIS );

	// Start outstanding links watcher
	s_updateOSL.start();
	
	// Limiting the scope of the outstanding links list mutex
	{
		SimpleMutexHolder outstandingLinksMutex( outstandingLinksMutex_ );
	
		// Iterate through the outstanding links
		ULMultimapIt olIt = outstandingLinks_.begin();
		while (olIt != outstandingLinks_.end() && (timestamp() - startTime) < maxTime)
		{
			// olIt->first (source) should be registered
			EditorChunkItemLinkable* pLinkFromLinker =
				getRegistered(olIt->first);
			MF_ASSERT(
				pLinkFromLinker != NULL &&
				"EditorChunkItemLinkableManager::tick : Linker "
				"found in outstandingLinks list not registered!" );
	
			// Check if olIt->second (target) is registered
			EditorChunkItemLinkable* pLinkToLinker =
				getRegistered( olIt->second.toUID_ );
	
			if (pLinkToLinker)
			{
				// Create a link
				s_updateLink.start();
				updateLink( pLinkFromLinker, pLinkToLinker );
				s_updateLink.stop();

				// Remove the link from the outstandingLinks_ list
				olIt = outstandingLinks_.erase( olIt );
			}
			else
			{
				// Chunk ids refer to the outside chunk id of a Linker.  If we
				// get to here in the code and the chunk ids are the same, one
				// or both of the linkers most be in separate shells in the same
				// outside chunk.  In this situation leave the link in the
				// outstanding links list since it will be used when the second
				// shell loads eventually, and continue to the next outstanding
				// link.
				if (olIt->second.fromCID_ == olIt->second.toCID_)
				{
					++olIt;
					continue;
				}
	
				// Create the link info and make sure it has not been added to
				// the chunk links list already
				Link link(
					olIt->second.fromUID_, olIt->second.fromCID_,
					olIt->second.toUID_, olIt->second.toCID_ );
				ULPair linkPair( olIt->second.fromUID_, link );

				SameLink linkEquality( linkPair );
				ULMultimapIt chunkLinkIt = find_if(
					chunkLinks_.begin(), chunkLinks_.end(), linkEquality );

				// This can happen if more than one property points at the same
				// object
				if (chunkLinkIt != chunkLinks_.end())
				{
					continue;
				}
	
				// Create chunk link that starts at pLinkFromLinker and fades to
				// the centre of pLinkToLinker's outside chunk
				s_updateChunkLink.start();
				pLinkFromLinker->createChunkLink(olIt->second.toCID_);
				s_updateChunkLink.stop();
	
				// Add to the chunk links set
				chunkLinks_.insert(linkPair);
	
				// We also remove it from outstanding since the chunk links list
				// will be searched for newly added objects below
				olIt = outstandingLinks_.erase(olIt);
			}
		}
	}

	// Stop outstanding links watcher
	s_updateOSL.stop();

	// Start outstanding back links watcher
	s_updateOSBL.start();
	
	// Limiting the scope of the outstanding back links list mutex
	{
		SimpleMutexHolder outstandingBackLinksMutex(
			outstandingBackLinksMutex_ );
	
		// Iterate through the outstanding back links
		ULMultimapIt olIt = outstandingBackLinks_.begin();
		while (olIt != outstandingBackLinks_.end() &&
				(timestamp() - startTime) < maxTime)
		{
			// The target linker should be registered
			EditorChunkItemLinkable* pLinkToLinker =
				getRegistered(olIt->second.toUID_);
			MF_ASSERT
				(
					pLinkToLinker != NULL &&
					"EditorChunkItemLinkableManager::tick : Linker "
					"found in outstandingLinks list not registered!"
				);

			// Check if source is registered
			EditorChunkItemLinkable* pLinkFromLinker =
				getRegistered( olIt->second.fromUID_ );

			// If it is loaded, there is no need to create a chunk back link
			if (pLinkFromLinker)
			{
				olIt = outstandingBackLinks_.erase(olIt);
			}
			else
			{
				// Similar to the argument above about linkers being in
				// different shells in the same outside chunk, This can occur
				// for the back links also.
				if (olIt->second.fromCID_ == olIt->second.toCID_)
				{
					++olIt;
					continue;
				}

				// Create the link info and make sure it has not been added to
				// the chunk back links list already
				Link link(
					olIt->second.fromUID_, olIt->second.fromCID_,
					olIt->second.toUID_, olIt->second.toCID_ );
				ULPair linkPair(olIt->second.fromUID_, link);

				SameLink linkEquality(linkPair);
				ULMultimapIt chunkLinkIt = find_if(
					chunkBackLinks_.begin(),
					chunkBackLinks_.end(),
					linkEquality );
	
				// This can happen if more than one property points at the same
				// object
				if (chunkLinkIt != chunkBackLinks_.end())
				{
					continue;
				}
	
				// Create chunk back link towards the to link
				s_updateChunkLink.start();
				pLinkToLinker->createChunkLink( olIt->second.fromCID_ );
				s_updateChunkLink.stop();
	
				// Add to the chunk links set
				chunkBackLinks_.insert( linkPair );
	
				// We also remove it from outstanding since the chunk links list
				// will be searched for newly added objects below
				olIt = outstandingBackLinks_.erase( olIt );
			}
		}
	}

	// Stop outstanding back links watcher
	s_updateOSBL.stop();

	// Start chunk links watcher
	s_updateCL.start();

	ULMultimapIt ulIt = chunkLinks_.begin();
	while (ulIt != chunkLinks_.end() && (timestamp() - startTime) < maxTime)
	{
		// Check if target is registered
		EditorChunkItemLinkable* pLinkerTo = getRegistered(ulIt->second.toUID_);
		if (!pLinkerTo)
		{
			ulIt++;
		}
		else
		{
			// Source should be registered
			EditorChunkItemLinkable* pLinkerFrom =
				getRegistered( ulIt->second.fromUID_);
			MF_ASSERT(
				(pLinkerFrom != 0) &&
				"EditorChunkItemLinkableManager::tick : Chunk linked "
				"object not registered!" );
			
			// Destroy the chunk link going to the targets outside chunk
			s_updateChunkLink.start();
			pLinkerFrom->removeChunkLink( ulIt->second.toCID_ );
			s_updateChunkLink.stop();

			// Remove ulIt from chunk links
			ulIt = chunkLinks_.erase(ulIt);

			// Update to create a standard link
			s_updateLink.start();
			updateLink(pLinkerFrom, pLinkerTo);
			s_updateLink.stop();
		}
	}

	// Stop chunk links watcher
	s_updateCL.stop();

	// Start chunk back links watcher
	s_updateCBL.start();

	for (ulIt = chunkBackLinks_.begin();
		 ulIt != chunkBackLinks_.end() && (timestamp() - startTime) < maxTime;)
	{
		// Check if source is registered
		EditorChunkItemLinkable* pLinkerFrom;
		if ( (pLinkerFrom = getRegistered(ulIt->second.fromUID_)) == 0 )
		{
			ulIt++;
			continue;
		}

		// Target should be registered
		EditorChunkItemLinkable* pLinkerTo = getRegistered(ulIt->second.toUID_);
		MF_ASSERT
			(
				(pLinkerTo != 0) &&
				"EditorChunkItemLinkableManager::tick : Chunk back linked "
				"object not registered!"
			);
		
		// Destroy the chunk link going to targets chunk
		s_updateChunkLink.start();
		pLinkerTo->removeChunkLink( ulIt->second.fromCID_ );
		s_updateChunkLink.stop();

		// Remove ulIt from chunk links
		ulIt = chunkBackLinks_.erase(ulIt);

		// Update to create a standard link
		s_updateLink.start();
		updateLink(pLinkerTo, pLinkerFrom);
		s_updateLink.stop();
	}

	// Stop chunk back links watcher
	s_updateCBL.stop();
}


/**
 *	Used by objects to notify the manager that they are about to be added to a
 *	chunk.  The chunk id for this object must be updated in the properties of
 *	every object that references it.
 *
 *	@param	pLinkable	The object being added.
 */
void EditorChunkItemLinkableManager::tossAdd( EditorChunkItemLinkable * pLinkable )
{
	BW_GUARD;

	MF_ASSERT
		(
			pLinkable &&
			"EditorChunkItemLinkableManager::tossAdd : NULL passed in!"
		);

	// The toss remove method is not called when moving a shell.  As a result
	// the tossAdd method my be called twice in succession without tossRemove
	// being called.  Only interested in checking the chunks current location
	// if this occurs (i.e. it is already loaded)
	if (isRegistered( pLinkable->guid() ))
	{
		checkLocation( pLinkable );
	}
	else
	{
		// Register the object
		registerLinker( pLinkable );

		// The link must be registered before checking its location
		checkLocation( pLinkable );

		// Add all of this object's links to the outstanding links list
		addAllLinksToOutstandingList( pLinkable );

		// Add all back links to the outstanding back links list
		addAllBackLinksToOutstandingBackLinksList( pLinkable );
	}
}


/**
 *	Used by objects to notify the manager that they are about to be removed from
 *	a chunk.
 *
 *	@param	pLinkable	The object being removed.
 */
void EditorChunkItemLinkableManager::tossRemove( EditorChunkItemLinkable* pLinkable )
{
	BW_GUARD;

	// The object should be registered prior to calling this method
	if (!isRegistered( pLinkable->guid() ))
	{
		ERROR_MSG(
			"EditorChunkItemLinkableManager::tossRemove : The Linked object "
			"should be registered prior to calling this method\n" );
		return;
	}

	// Clear the links
    pLinkable->unlink();

	// Clear the chunk links
	pLinkable->removeChunkLinks();

	// Clear from manager lists
	removeFromLists( pLinkable );

	// For each loaded back link, add an outstanding link to this object
	addAllBacklinksToOutstandingList( pLinkable );

	// Add links to outstanding back links list
	addAllLinksToOutstandingBackLinksList( pLinkable );

	// Remove from registered list
	unregisterLinker( pLinkable );
}


/**
 *	Informs the manager that a linker guid is about to change.  The guid is
 *	removed from the location cache since it is no longer needed.
 *
 *	@param	from	The previous unique id.
 *	@param	to		The new unique id.
 */
void EditorChunkItemLinkableManager::idPreChange(
	const UniqueID& from, const UniqueID& to )
{
	BW_GUARD;

	if (from == to)
	{
		return;
	}

	EditorChunkItemLinkable* pLinkable = getRegistered( from );
	if (pLinkable)
	{
		removeFromLocationCache( pLinkable );
	}

	EditorChunkItemLinkable* pToLinkable = getRegistered( to );
	if (pToLinkable)
	{
		WorldManager::instance().addError(
			pToLinkable->chunkItem()->chunk(),
			pToLinkable->chunkItem(),
			"Linked object is using a GUID that is already registered, "
				"%s. Delete it and recreate it.\n",
			to.toString().c_str() );
	}
}


/**
 *	Used by objects to notify the manager that they are about to be deleted.
 *	This occurs by the user deleting the object explicitly in the editor.  All
 *	objects refering or being refered to by this object need to have their links
 *	updated.
 *
 *	@param	pLinkable	The object to be deleted.
 */
void EditorChunkItemLinkableManager::deleted( EditorChunkItemLinkable* pLinkable )
{
	BW_GUARD;

	// The object should not be NULL and should be registered prior to calling
	// this method
	if (!pLinkable)
	{
		MF_ASSERT
			(
				0 &&
				"EditorChunkItemLinkableManager::deleted : pLinkable is NULL!"
			);
		return;
	}
	if (pLinkable && !isRegistered( pLinkable->guid() ))
	{
		registerLinker( pLinkable );
		if (!isRegistered( pLinkable->guid() ))
		{
			MF_ASSERT
				(
					0 &&
					"EditorChunkItemLinkableManager::deleted : "
					"pLinkable not registered prior to the method call!"
				);
	
			ERROR_MSG
				(
					"EditorChunkItemLinkableManager::deleted : Linker object %s "
					"in chunk %s not registered prior to delete.\n",
					pLinkable->guid().toString().c_str(),
					pLinkable->getOutsideChunkId().c_str()
				);
			return;
		}
	}

	// Need to run through pLinkable's back links removing all references to pLinkable
	// from each.
	EditorChunkItemLinkable::Links backLinksCopy;
	backLinksCopy.insert(pLinkable->getBackLinksBegin(), pLinkable->getBackLinksEnd());

	EditorChunkItemLinkable::LinksConstIter it;
	for (it = backLinksCopy.begin(); it != backLinksCopy.end(); ++it)
	{
		// Get the back link, force load if necessary
		EditorChunkItemLinkable* backLink = forceLoad( it->UID_, it->CID_ );
		if (backLink)
		{
			// Remove all references from backLink that point to pLinkable
			removeAllRefs( backLink, pLinkable );
		}
	}

	// Also need to examine the objects that we are pointing to.  We
	// have to remove all back links that point to pLinkable
	removeAllBackLinksToLinker( pLinkable );

	// Finally we need to delete all of our links so that they will not be
	// placed in the outstanding links list when the tossRemove method is called
	removeAllRefs( pLinkable, 0 );
	pLinkable->removeAllBackLinks();

	// Remove from location cache
	removeFromLocationCache( pLinkable );

	// Save the changes
	if (pLinkable->chunkItem()->pOwnSect())
	{
		pLinkable->chunkItem()->edSave( pLinkable->chunkItem()->pOwnSect() );
	}
	if (pLinkable->chunkItem()->chunk())
	{
        WorldManager::instance().changedChunk(
			pLinkable->chunkItem()->chunk(),
			InvalidateFlags::FLAG_NONE );
    }
}


/**
 *	Adds a link between two linker objects in the specified property.
 *
 *	@param	start	The source object.
 *	@param	end		The target object.
 *	@param	propIdx	The id of the property the link will be added to.
 */
void EditorChunkItemLinkableManager::addLink(
	EditorChunkItemLinkable* start, EditorChunkItemLinkable* end,
	const PropertyIndex& propIdx, bool updateOld /*= true*/ )
{
	BW_GUARD;

	// addLinkInternal does the actual linking
	EditorChunkItemLinkable* old = 0;
	bool result;
	if (updateOld)
	{
		result = addLinkInternal( start, end, propIdx, &old );
	}
	else
	{
		result = addLinkInternal( start, end, propIdx, 0, false );
	}

	if (!result)
	{
		return;
	}

	// The undo/redo operations are kept outside the main add link
	// method since the undo/redo operations call this method internally.
	// Recursively construcing undo/redo operations can be problematic
	// at best
	if (old)
	{
		// We have changed to point at a new object
		UndoRedo::instance().add(
			new LinkerUndoChangeLinkOperation(start, old, end, propIdx) );
	}
	else
	{
		// We have created a new link
		UndoRedo::instance().add(
			new LinkerUndoAddLinkOperation(start, end, propIdx) );
	}
}


/**
 *	Workhourse method that performs all the add link operations between two
 *	objects, adding the link in the specified property.
 *
 *	@param	start	The source object.
 *	@param	end		The target object.
 *	@param	propIdx	The id of the property the link will be added to.
 *	@param	old		The object currently being linked to.
 *	@param	updatePrevious
 *					Update the previously linked object
 *
 *	@return			Boolean success or failure.
 */
bool EditorChunkItemLinkableManager::addLinkInternal(
	EditorChunkItemLinkable* start, EditorChunkItemLinkable* end,
	const PropertyIndex& propIdx, EditorChunkItemLinkable** old,
	bool updatePrevious /*= true*/ )
{
	BW_GUARD;

	// Start and end should be registered prior to calling this method
	MF_ASSERT
		(
			start && end &&
			isRegistered( start->guid() ) && isRegistered( end->guid() ) &&
			"EditorChunkItemLinkableManager::addLinkInternal : "
			"start and/or end not registered prior the method call!"
		);

	// Get a datasection from the property's python object
	DataSectionPtr section = start->propHelper()->propGet( propIdx );

	// Before adding a new link at this property, check if there is an existing
	// link there.  If so we need to inform the linked object that we are
	// deleting this link and it should update its back links appropriately
	BW::string linkableId = section->readString( "guid" );
	BW::string chunkId = section->readString( "chunkId" );
	
	EditorChunkItemLinkable* previous = 0;
	if (updatePrevious)
		previous = forceLoad( linkableId, chunkId );

	// If the property is pointing at an existing linker, check if that linker
	// is editable
	if (previous && !previous->chunkItem()->edIsEditable())
		return false;

	// Do the actual linking:
	section->writeString( "guid", end->guid().toString() );
	section->writeString( "chunkId", end->getOutsideChunkId() );
	start->propHelper()->propSet( propIdx, section );

	// Update for the newly added link
	updateLink( start, end );

	// Save the changes
	start->chunkItem()->edSave( start->chunkItem()->pOwnSect() );
	if (start->chunkItem()->chunk())
		WorldManager::instance().changedChunk(
			start->chunkItem()->chunk(),
			InvalidateFlags::FLAG_NONE );

	end->chunkItem()->edSave( end->chunkItem()->pOwnSect() );
	if (end->chunkItem()->chunk())
		WorldManager::instance().changedChunk(
			end->chunkItem()->chunk(),
			InvalidateFlags::FLAG_NONE );

	// Update the previously linked object, if there was one
	if (previous)
	{
		updateLink( start, previous );

		// Save the changes
		previous->chunkItem()->edSave( previous->chunkItem()->pOwnSect() );
		if (previous->chunkItem()->chunk())
	        WorldManager::instance().changedChunk(
				previous->chunkItem()->chunk(),
				InvalidateFlags::FLAG_NONE );
	}

	if (old)
		*old = previous;

	return true;
}


/**
 *	Used to delete a specific link property from a linker.
 *
 *	@param	pLinkable		The linker whos property is to be deleted.
 *	@param	pOldLinkable	The linker that the property points to.
 *	@param	propIdx		The id of the property to be deleted.
 */
void EditorChunkItemLinkableManager::deleteLink(
	EditorChunkItemLinkable* pLinkable, EditorChunkItemLinkable* pOldLinkable,
	PropertyIndex propIdx )
{
	BW_GUARD;

	// Get the data for this property
	DataSectionPtr data = pLinkable->propHelper()->propGet( propIdx.valueAt( 0 ) );	
	
	// Delete the link
	deleteLinkInternal( pLinkable, pOldLinkable, propIdx );

	// The undo/redo operation is kept outside the main delete link method since
	// the undo/redo operation calls deleteLinkInternal internally.
	// Recursively construcing undo/redo operations can be problematic at best
	if (pOldLinkable)
	{
		UndoRedo::instance().add(
			new LinkerUndoDeleteLinkOperation(pLinkable, pOldLinkable, data, propIdx) );
	}
}


/**
 *	Used to delete a specific link property from a linker.
 *
 *	@param	pLinkable		The linker whos property is to be deleted.
 *	@param	pOldLinkable	The linker that the property points to, possibly NULL.
 *	@param	propIdx		The id of the property to be deleted.
 */
void EditorChunkItemLinkableManager::deleteLinkInternal(
	EditorChunkItemLinkable* pLinkable, EditorChunkItemLinkable* pOldLinkable,
	PropertyIndex propIdx )
{
	BW_GUARD;

	// Ws should not be passed a NULL pointer for the linker method
	MF_ASSERT
		(
			pLinkable && isRegistered(pLinkable->guid()) &&
			"EditorChunkItemLinkableManager::deleteLink : "
			"pLinkable should be registered prior the method call!"
		);

	// Is this property a object link
	if (pLinkable->propHelper()->isUserDataObjectLink( propIdx.valueAt( 0 ) ))
	{
		// Clear the property to its default value
		pLinkable->propHelper()->propSetToDefault( propIdx.valueAt( 0 ) );
	}
	else if (
		pLinkable->propHelper()->isUserDataObjectLinkArray( propIdx.valueAt( 0 ) ))
	{
		PyObjectPtr pValue(
			pLinkable->propHelper()->propGetPy( propIdx.valueAt(0) ),
			PyObjectPtr::STEAL_REFERENCE );

		DataDescription* pDD =
			pLinkable->propHelper()->pType()->property( propIdx.valueAt( 0 ) );

		SequenceDataType* dataType =
			static_cast<SequenceDataType*>( pDD->dataType() );

		ArrayPropertiesHelper propArray;
		propArray.init(
			pLinkable->chunkItem(),
			&(dataType->getElemType()),
			pValue.getObject() );
		
		// Delete the item
		propArray.delItem( propIdx.valueAt( 1 ) );
	}

	// Save the changes
	pLinkable->chunkItem()->edSave( pLinkable->chunkItem()->pOwnSect() );
	if (pLinkable->chunkItem()->chunk())
        WorldManager::instance().changedChunk( 
			pLinkable->chunkItem()->chunk(),
			InvalidateFlags::FLAG_NONE );

	// Is this property an array link item
	if (pOldLinkable)
	{
		updateLink( pLinkable, pOldLinkable );

		// Save the changes
		pOldLinkable->chunkItem()->edSave( pOldLinkable->chunkItem()->pOwnSect() );
		if (pOldLinkable->chunkItem()->chunk())
			WorldManager::instance().changedChunk(
				pOldLinkable->chunkItem()->chunk(),
				InvalidateFlags::FLAG_NONE );
	}
}


/**
 *	Used to delete all links between two objects.
 *
 *	@param	pLinkable1	A object.
 *	@param	pLinkable2	Another object.
 */
void EditorChunkItemLinkableManager::deleteAllLinks(
	EditorChunkItemLinkable* pLinkable1, EditorChunkItemLinkable* pLinkable2 )
{
	BW_GUARD;

	// Both objects should be registered prior to calling this
	// method
	MF_ASSERT
		(
			isRegistered( pLinkable1->guid() ) && isRegistered( pLinkable2->guid() ) &&
			"EditorChunkItemLinkableManager::deleteLink : "
			"Start and/or end nodes are not registered!"
		);

	// Remove all references from pLinkable1 to pLinkable2
	removeAllRefs( pLinkable1, pLinkable2 );

	// Remove all references from pLinkable2 to pLinkable1
	removeAllRefs( pLinkable2, pLinkable1 );

	// Update link
	updateLink( pLinkable1, pLinkable2 );

	// Save the changes
	pLinkable1->chunkItem()->edSave( pLinkable1->chunkItem()->pOwnSect() );
	if (pLinkable1->chunkItem()->chunk())
        WorldManager::instance().changedChunk( 
		pLinkable1->chunkItem()->chunk(),
		InvalidateFlags::FLAG_NONE );

	pLinkable2->chunkItem()->edSave( pLinkable2->chunkItem()->pOwnSect() );
	if (pLinkable2->chunkItem()->chunk())
        WorldManager::instance().changedChunk( 
		pLinkable2->chunkItem()->chunk(),
		InvalidateFlags::FLAG_NONE );
}


/**
 *	Used to force load a object by force loading the chunk the object resides
 *	in.
 *
 *	@param	linkableId	The UniqueID of the object being loaded.
 *	@param	chunkId	The ChunkID of the object being loaded.
 *	@return		The object to be force loaded.
 */
EditorChunkItemLinkable* EditorChunkItemLinkableManager::forceLoad(
	const UniqueID& linkableId, const BW::string& chunkId )
{
	BW_GUARD;

	if (linkableId == UniqueID() || chunkId == "") return 0;

	EditorChunkItemLinkable* pLinkable = getRegistered( linkableId );

	// If it's not we need to force load the chunk and then get the linker
	if (pLinkable == 0)
	{
		CWaitCursor waitCursor;

		// Shells may take a couple of seconds to load after force loading the
		// outside chunk.  That is why we wait until the shell has been loaded
		// before continuing
		ChunkManager::instance().switchToSyncMode( true );

		GeometryMapping* dirMap = WorldManager::instance().geometryMapping();
		ChunkManager::instance().loadChunkNow( chunkId, dirMap );

		// loop while the linker is not loaded and still things to load.
		while (
			!(pLinkable = getRegistered( linkableId )) &&
			ChunkManager::instance().checkLoadingChunks() )
		{
			ChunkManager::instance().tick( 0 );
			Sleep( 0 );
		}

		ChunkManager::instance().switchToSyncMode( false );

		// The linker should be loaded now
		pLinkable = getRegistered( linkableId );
		if ( !pLinkable )
		{
			ERROR_MSG
				(
					"EditorChunkItemLinkableManager::forceLoad : "
					"Failed to load linked object %s from chunk %s.\n",
					linkableId.toString().c_str(), chunkId.c_str()
				);
		}
	}

	return pLinkable;
}


/**
 *	Clears all managed lists.  This should be called when reloading all chunks
 *	or when changing space.
 */
void EditorChunkItemLinkableManager::reset()
{
	BW_GUARD;

	// Lock mutexes
	SimpleMutexHolder regLinksMutex( registeredLinkersMutex_ );
	SimpleMutexHolder cacheMutex( locationCacheMutex_ );
	SimpleMutexHolder outstandingLinksMutex( outstandingLinksMutex_ );
	SimpleMutexHolder outstandingBackLinksMutex( outstandingBackLinksMutex_ );
	SimpleMutexHolder chunkLinksMutex( chunkLinksMutex_ );
	SimpleMutexHolder chunkBackLinksMutex( chunkBackLinksMutex_ );

	registeredLinkers_.clear();
	locationCache_.clear();
	outstandingLinks_.clear();
	outstandingBackLinks_.clear();
	chunkLinks_.clear();
	chunkBackLinks_.clear();
}


/**
 *	Used to clear the passed object from all manager owned lists.
 *
 *	@param	pLinkable	The object to be removed.
 */
void EditorChunkItemLinkableManager::removeFromLists(
	EditorChunkItemLinkable* pLinkable )
{
	BW_GUARD;

	// The object should be registered prior to calling this method
	MF_ASSERT
		(
			isRegistered( pLinkable->guid() ) &&
			"EditorChunkItemLinkableManager::removeFromLists : "
			"pLinkable should be registered prior to calling this method!"
		);

	ULMultimapIt it;

	// Lock outstanding list mutex and limit scope
	{
		SimpleMutexHolder outstandingLinksMutex( outstandingLinksMutex_ );
		
		// Remove all outstanding links from pLinkable in outstandingLinks_
		for (it = outstandingLinks_.begin(); it != outstandingLinks_.end(); )
		{
			if (it->first == pLinkable->guid())
			{
				it = outstandingLinks_.erase( it );
			}
			else
			{
				it++;
			}
		}
	}

	// Lock outstanding list mutex and limit scope
	{
		SimpleMutexHolder outstandingBackLinksMutex(
			outstandingBackLinksMutex_ );
		
		// Remove all outstanding back links to pLinkable in outstandingBackLinks_
		for (
			it = outstandingBackLinks_.begin();
			it != outstandingBackLinks_.end();
			)
		{
			if (it->second.toUID_ == pLinkable->guid())
			{
				it = outstandingBackLinks_.erase( it );
			}
			else
			{
				it++;
			}
		}
	}

	// Lock chunk link mutex and limit scope
	{
		SimpleMutexHolder chunkLinksMutex( chunkLinksMutex_ );

		// Remove all chunk links from pLinkable in chunkLinks_
		for (it = chunkLinks_.begin(); it != chunkLinks_.end(); )
		{
			if (it->first == pLinkable->guid())
			{
				it = chunkLinks_.erase( it );
			}
			else
			{
				it++;
			}
		}
	}

	// Lock chunk back link mutex and limit scope
	{
		SimpleMutexHolder chunkBackLinksMutex( chunkBackLinksMutex_ );

		// Remove all chunk links to pLinkable in chunkBackLinks_
		for (it = chunkBackLinks_.begin(); it != chunkBackLinks_.end(); )
		{
			if (it->second.toUID_ == pLinkable->guid())
			{
				it = chunkBackLinks_.erase( it );
			}
			else
			{
				it++;
			}
		}
	}
}


/**
 *	Used to relink objects that were cloned.  Also removes links that are not
 *	contained in the map.
 *
 *	@param	linkerGuidMapping	The mapping between the original GUIDs and
 *								the new cloned GUIDs.
 */
void EditorChunkItemLinkableManager::updateMappedLinkers(
	GuidToGuidMap& linkerGuidMapping )
{
	BW_GUARD;

	if (linkerGuidMapping.size() == 0)
	{
		ERROR_MSG
			(
				"EditorChunkItemLinkableManager::linkClonedLinkers : "
				"Add least one linker object should be getting cloned!\n"
			);
		return;
	}

	// Search for missing linker objects and remove any not found.  This can
	// occur when using a prefab that is too large for the current space.
	GuidToGuidMap::iterator it = linkerGuidMapping.begin();
	while (it != linkerGuidMapping.end())
	{
		// It should be registered, but if it's not print an error msg that
		// something screwed up, remove the entry from the map, and continue
		EditorChunkItemLinkable* pClonedLinkable = getRegistered( it->second );
		if (!pClonedLinkable)
		{
			ERROR_MSG(
				"EditorChunkItemLinkableManager::linkClonedLinkers : "
				"The cloned linker should be registered!" );

			it = linkerGuidMapping.erase( it );

			continue;
		}

		++it;
	}

	// Iterate through the cloned linker objects, updating the properties of
	// each
	for (
		it = linkerGuidMapping.begin();
		it != linkerGuidMapping.end();
		++it )
	{
		// It should be registered
		EditorChunkItemLinkable* pClonedLinkable = getRegistered( it->second );
		MF_ASSERT
			(
				pClonedLinkable &&
				"EditorChunkItemLinkableManager::linkClonedLinkers : \
				 The cloned linker should be registered!"
			);
		
		// Clear from all managed lists
		removeFromLists( pClonedLinkable );

		// Iterate through pClonedLinkable's properties, updating appropriately
		updateCloneProperties( pClonedLinkable, linkerGuidMapping );

		// Iterate through pClonedLinkable's back links, updating appropriately
		updateCloneBackLinks( pClonedLinkable, linkerGuidMapping );
	}
}


/**
 *	Used to regenerate the guid in a datasection.
 *
 *	@param	item				The datasection to update
 *	@param	linkerGuidMapping	The mapping between the original GUIDs and
 *								the new cloned GUIDs.
 */
/*static*/ void EditorChunkItemLinkableManager::updateLinkerGuid(
	DataSectionPtr item, GuidToGuidMap& linkerGuidMapping )
{
	BW_GUARD;

	if (item == NULL)
	{
		ERROR_MSG( "EditorChunkItemLinkableManager::updateLinkerGuid: "
			"bad parameter, 'item' DataSection should not be NULL.\n" );
		return;
	}
	
	if (item->sectionName() != "entity" &&
		item->sectionName() != "UserDataObject" )
	{
		return;
	}

	// Need to read in the prefab GUID and then set it to a new GUID
	BW::string origGuid;
	origGuid = item->readString( "guid" );
	UniqueID newGuid = UniqueID::generate();
	linkerGuidMapping[origGuid] = newGuid;
	item->writeString( "guid", newGuid.toString() );
}


//------------------------------------------------------------------------------
//	PRIVATE HELPER METHODS SECTION
//------------------------------------------------------------------------------

/**
 *	Used to register a object with the manager.
 *
 *	@param	pLinkable	The object to be registered.
 */
void EditorChunkItemLinkableManager::registerLinker(
	EditorChunkItemLinkable* pLinkable )
{
	BW_GUARD;

	// The object should not be registered prior to calling this method
	MF_ASSERT
		(
			pLinkable &&
			"EditorChunkItemLinkableManager::registerLinker : "
			"pLinkable is NULL"
		);

	if (isRegistered( pLinkable->guid() ))
	{
		WorldManager::instance().addError(
			pLinkable->chunkItem()->chunk(),
			pLinkable->chunkItem(),
			"Linked object is using an GUID that is already registered, "
			"%s. Delete it and recreate it.\n",
			pLinkable->guid().toString().c_str() );
		return;
	}

	// Lock mutex
	SimpleMutexHolder regLinksMutex( registeredLinkersMutex_ );
	
	// Add to registered list
	registeredLinkers_.insert( pLinkable );
}


/**
 *	Used to unregister a object with the manager.
 *
 *	@param	pLinkable	The object to be unregistered.
 */
void EditorChunkItemLinkableManager::unregisterLinker(
	EditorChunkItemLinkable* pLinkable)
{
	BW_GUARD;

	// The object should be registered prior to calling this method
	MF_ASSERT
		(
			pLinkable &&
			"EditorChunkItemLinkableManager::unregisterLinker : "
			"pLinkable is NULL"
		);

	if (!isRegistered( pLinkable->guid() ))
	{
		WorldManager::instance().addError(
			pLinkable->chunkItem()->chunk(),
			pLinkable->chunkItem(),
			"Linked object is using an GUID that is already registered, %s. Delete it and recreate it.\n",
			pLinkable->guid().toString().c_str() );
		return;
	}

	// Lock mutex
	SimpleMutexHolder regLinksMutex( registeredLinkersMutex_ );
	
	// Remove from registered list
	registeredLinkers_.erase( pLinkable );
}


/**
 *	Used to check if a object with the passed UniqueID is registered.
 *
 *	@param	linkableId	The UniqueID of the target object.
 *	@return		Found or not found.
 */
bool EditorChunkItemLinkableManager::isRegistered( const UniqueID& linkableId )
{
	BW_GUARD;

	if (getRegistered( linkableId ))
	{
		return true;
	}
	else
	{
		return false;
	}
}


/**
 *	Used to get the object from the registered list with the passed UniqueID.
 *
 *	@param	linkableId	The UniqueID of the target object.
 *	@return		A pointer to the object, NULL if not found.
 */
EditorChunkItemLinkable* EditorChunkItemLinkableManager::getRegistered(
	const UniqueID& linkableId )
{
	BW_GUARD;

	// Lock mutex
	SimpleMutexHolder regLinksMutex( registeredLinkersMutex_ );
	
	LinkerSetIt it;
	it = find_if(
			registeredLinkers_.begin(),
			registeredLinkers_.end(),
			SameUniqueId( linkableId )) ;

	return ( it != registeredLinkers_.end() ) ? *it : 0;
}


/**
 *	Checks the validity of a link, and generates appropriate error messages if
 *	the link is not valid.
 *
 *	@param start		Start point of the link.
 *	@param end			End point of the link.
 *	@param endChunkId	End chunk ID according to the link info.
 *	@return				true if the link is valid, false otherwise.
 */
bool EditorChunkItemLinkableManager::isLinkValid(
	const EditorChunkItemLinkable* start, const EditorChunkItemLinkable* end,
	const BW::string& endChunkId ) const
{
	BW_GUARD;

	if (start == NULL || end == NULL)
		return false;

	BW::string actualEndChunkId = end->getOutsideChunkId();
	if (actualEndChunkId != endChunkId)
	{
		WorldManager::instance().addError(
			start->chunkItem()->chunk(),
			start->chunkItem(),
			"Linked object ID %s cannot be linked to %s, "
			"destination chunk doesn't match. Delete the link and relink.\n",
			start->guid().toString().c_str(),
			end->guid().toString().c_str() );
		return false;
	}

	return true;
}


/**
 *	Used to force load a object by force loading the chunk the object resides
 *	in.
 *
 *	@param	pPyLinkProp	The PyObject property.
 *	@return				The object to be force loaded.
 */
EditorChunkItemLinkable* EditorChunkItemLinkableManager::forceLoad(
	PyObjectPtr pPyLinkProp )
{
	BW_GUARD;

	// Need to obtain the unique ids and chunk ids of the linker
	BW::string linkableId = PyString_AsString(
			PyTuple_GetItem( pPyLinkProp.getObject(), 0 ) );
	BW::string chunkId = PyString_AsString(
			PyTuple_GetItem( pPyLinkProp.getObject(), 1 ) );

	return forceLoad( linkableId, chunkId );
}


/**
 *	Used to check if a property points to a particular object.
 *
 *	@param	pLinkable		The target object.
 *	@param	pPyLinkProp	The PyObject property.
 *	@return				Bool indicating whether the pPy property refer to pLinkable
 */
bool EditorChunkItemLinkableManager::isLinkerReferencedInProp(
	const EditorChunkItemLinkable* pLinkable, PyObjectPtr pPyLinkProp ) const
{
	BW_GUARD;

	// Need to obtain the unique ids and chunk ids of the linker
	BW::string uniqueId = PyString_AsString(
			PyTuple_GetItem( pPyLinkProp.getObject(), 0 ) );
	if (uniqueId == pLinkable->guid().toString())
	{
		return true;
	}
	else
	{
		return false;
	}
}


/**
 *	Updates the appearance of a link between two objects.  This method
 *	can create, alter or delete a link between two objects depending on whether
 *	the objects make reference to each other in their properties.
 *
 *	@param	pLinkable1	A first object.
 *	@param	pLinkable2	A second object.
 */
void EditorChunkItemLinkableManager::updateLink(
	EditorChunkItemLinkable* pLinkable1, EditorChunkItemLinkable* pLinkable2 )
{
	BW_GUARD;

	// Both objects should be registered
	MF_ASSERT
		(
			pLinkable1 && isRegistered( pLinkable1->guid() ) &&
			pLinkable2 && isRegistered( pLinkable2->guid() ) &&
			"EditorChunkItemLinkableManager::updateLink : "
			"Both objects should be registered prior to calling this method!"
		);
	
	// Need to determine if a link exists between pLinkable1 and pLinkable2.
	// If a link exists if could be DIR_START_END, DIR_END_START,
	// DIR_BOTH, or DIR_NONE.
	bool linkFound[] = { false, false };
	EditorChunkItemLinkable* pLinkables[] = { pLinkable1, pLinkable2 };

	// Need to update links for each object
	for (uint i = 0; i < 2; i++)
	{
		// Iterate through pLinkables[i]'s properties
		int propCounter = pLinkables[i]->propHelper()->propCount();
		for (int j = 0; j < propCounter; j++)
		{
			DataDescription* pDD =
				pLinkables[i]->propHelper()->pType()->property( j );
			
			// If non-editable, continue
			if (!pDD->editable())
				continue;
			
			// Is this property an object link
			if (pLinkables[i]->propHelper()->isUserDataObjectLink( j ))
			{
				PyObjectPtr pValue(
						pLinkables[i]->propHelper()->propGetPy( j ),
						PyObjectPtr::STEAL_REFERENCE );

				// Need to obtain the unique ids and chunk ids of the linked
				// pLinkable
				BW::string uniqueId = PyString_AsString(
						PyTuple_GetItem( pValue.getObject(), 0 ) );
				if (uniqueId == pLinkables[ (i+1)%2 ]->guid().toString())
				{
					BW::string chunkId = PyString_AsString( PyTuple_GetItem(
							pValue.getObject(), 1 ) );
					
					if (isLinkValid( pLinkables[i%2], pLinkables[(i+1)%2], chunkId ))
					{
						// Found a valid link, don't need to search for anymore
						// link for this linker object
						linkFound[i] = true;
						break;
					}
				}
			}
			
			// Is this property an array of object links
			if (pLinkables[i]->propHelper()->isUserDataObjectLinkArray( j ))
			{
				PyObjectPtr pValue(
						pLinkables[i]->propHelper()->propGetPy( j ),
						PyObjectPtr::STEAL_REFERENCE );

				SequenceDataType* dataType =
					static_cast<SequenceDataType*>( pDD->dataType() );
				
				ArrayPropertiesHelper propArray;
				propArray.init(
					pLinkables[i]->chunkItem(), &(dataType->getElemType()),
					pValue.getObject() );

				// Iterate through the array of links
				for (int k = 0; k < propArray.propCount(); k++)
				{
					PyObjectPtr link(
						PySequence_GetItem( pValue.getObject(), k ),
						PyObjectPtr::STEAL_REFERENCE );

					// Need to obtain the unique ids and chunk ids of the linked
					// pLinkable
					BW::string uniqueId = PyString_AsString(
							PyTuple_GetItem( link.getObject(), 0 ) );

					if (uniqueId == pLinkables[ (i + 1) % 2 ]->guid().toString())
					{
						BW::string chunkId = PyString_AsString(
								PyTuple_GetItem( link.getObject(), 1 ) );
						if (isLinkValid(
								pLinkables[i % 2], pLinkables[ (i + 1) % 2 ], chunkId ))
						{
							// Found the link, don't need to search for anymore
							// link for this linker object
							linkFound[i] = true;
							break;
						}
					}
				}

				// If a link was found break again here to stop processing
				// properties
				if (linkFound[i])
					break;
			}
		}
	}

	// Based on the relationship between the linkers, create the appropriate
	// link type
	ChunkLinkPtr link;
	if (linkFound[0])
	{
		if (linkFound[1])		// pLinkable1 <-> pLinkable2
		{
			link = pLinkable1->createLink(ChunkLink::DIR_BOTH, pLinkable2);

			// Add a back link to both
			pLinkable1->addBackLink(pLinkable2);
			pLinkable2->addBackLink(pLinkable1);
		}
		else					// pLinkable1  -> pLinkable2
		{
			link = pLinkable1->createLink(ChunkLink::DIR_START_END, pLinkable2);

			// Add a back link to pLinkable2, remove from pLinkable1
			pLinkable2->addBackLink(pLinkable1);
			pLinkable1->removeBackLink(pLinkable2);
		}
	}
	else
	{
		if (linkFound[1])		// pLinkable1 <-  pLinkable2
		{
			link = pLinkable1->createLink(ChunkLink::DIR_END_START, pLinkable2);

			// Add a back link to uodo1, remove from pLinkable2
			pLinkable1->addBackLink(pLinkable2);
			pLinkable2->removeBackLink(pLinkable1);
		}
		else					// pLinkable1  -  pLinkable2
		{
			link = pLinkable1->createLink( ChunkLink::DIR_NONE, pLinkable2 );

			// remove back link from both
			pLinkable1->removeBackLink(pLinkable2);
			pLinkable2->removeBackLink(pLinkable1);
		}
	}
}


/**
 *	Used to handle the transition of objects across chunk boundaries.
 *
 *	@param	pLinkable	The object being updated.
 */
void EditorChunkItemLinkableManager::checkLocation(
	EditorChunkItemLinkable* pLinkable )
{
	BW_GUARD;

	// Get the current location
	BW::string location = pLinkable->getOutsideChunkId();
	BW::string cachedLocation;

	// Limit the scope of the mutex since this method may be called again as a
	// result of the update methods at the end of this method
	{
		// Lock mutex
		SimpleMutexHolder cacheMutex( locationCacheMutex_ );
		
		// Get the cached location
		cachedLocation = locationCache_[ pLinkable->guid() ];

		// If one has not been set, set to the current and return
		if (cachedLocation == "")
		{
			locationCache_[ pLinkable->guid() ] = location;			
			return;
		}
	}
	
	// Check the cached loaction against the current location.
	// If they are the same return
	if (cachedLocation == location)
	{
		return;
	}
	else // They are different.  We have crosed a chunk boundary
	{
		{
			// Lock mutex
			SimpleMutexHolder cacheMutex( locationCacheMutex_ );
			
			// Update the cache
			locationCache_[ pLinkable->guid() ] = location;
		}

		// Iterate through pLinkable's properties, updating their back link to
		// pLinkable
		updateLocationInProperties( pLinkable );

		// Iterate through pLinkable's back links, updating the chunkId stored
		// in each property
		updateLocationInBackLinks( pLinkable );

		// Update the managed lists
		updateLocationInManagedLists( pLinkable );
	}
}


/**
 *	Removes a linker object from the location cache list.
 *
 *	@param	pLinkable	The object being updated.
 */
void EditorChunkItemLinkableManager::removeFromLocationCache(
	EditorChunkItemLinkable* pLinkable)
{
	BW_GUARD;

	// pLinkable should be in registeredLinkers_
	MF_ASSERT(	isRegistered( pLinkable->guid() ) &&
				"EditorChunkItemLinkableManager::removeFromLocationCache : "
				"pLinkable is not registered!" );
	
	// Lock mutex
	SimpleMutexHolder cacheMutex( locationCacheMutex_ );
	
	locationCache_.erase(pLinkable->guid());
}


/**
 *	Updates the objects being refered to by the properties of pLinkable after
 *	crossing a chunk boundary.
 *
 *	@param	pLinkable	The object being updated.
 */
void EditorChunkItemLinkableManager::updateLocationInProperties(
	EditorChunkItemLinkable* pLinkable)
{
	BW_GUARD;

	// Iterate through the properties of pLinkable
	bool found = false;
	int propCounter = pLinkable->propHelper()->propCount();
	for (int i=0; i < propCounter; i++)
	{
		DataDescription* pDD = pLinkable->propHelper()->pType()->property( i );
		if (!pDD->editable())
			continue;

		// Is this property a object link
		if (pLinkable->propHelper()->isUserDataObjectLink( i ))
		{
			PyObjectPtr ob
				(
					pLinkable->propHelper()->propGetPy( i ),
					PyObjectPtr::STEAL_REFERENCE
				);

			// Remove the back link in pLinkableBackLink to pLinkable
			EditorChunkItemLinkable* pLinkableBackLink = forceLoad(ob);

			if (pLinkableBackLink)
			{
				pLinkableBackLink->removeBackLink(pLinkable);
				pLinkableBackLink->addBackLink(pLinkable);

				pLinkableBackLink->chunkItem()->edSave( pLinkableBackLink->chunkItem()->pOwnSect() );
				if ( pLinkableBackLink->chunkItem()->chunk() != NULL )
					WorldManager::instance().changedChunk( 
						pLinkableBackLink->chunkItem()->chunk(),
						InvalidateFlags::FLAG_NONE );
			}
		}
		// Is this property an array of object links
		else if (pLinkable->propHelper()->isUserDataObjectLinkArray( i ))
		{
			PyObjectPtr ob
				(
					pLinkable->propHelper()->propGetPy( i ),
					PyObjectPtr::STEAL_REFERENCE
				);

			SequenceDataType* dataType =
				static_cast<SequenceDataType*>( pDD->dataType() );
			ArrayPropertiesHelper propArray;
			propArray.init(pLinkable->chunkItem(), &(dataType->getElemType()), ob.getObject());

			// Iterate through the array of links
			for(int j = 0; j < propArray.propCount(); j++)
			{
				PyObjectPtr ob(
					propArray.propGetPy( j ), PyObjectPtr::STEAL_REFERENCE );

				// Remove the back link in pLinkableBackLink to pLinkable
				EditorChunkItemLinkable* pLinkableBackLink = forceLoad(ob);

				if (pLinkableBackLink)
				{
					pLinkableBackLink->removeBackLink(pLinkable);
					pLinkableBackLink->addBackLink(pLinkable);

					pLinkableBackLink->chunkItem()->edSave( pLinkableBackLink->chunkItem()->pOwnSect() );
					if ( pLinkableBackLink->chunkItem()->chunk() != NULL )
						WorldManager::instance().changedChunk(
							pLinkableBackLink->chunkItem()->chunk(),
							InvalidateFlags::FLAG_NONE );
				}
			}
		}
	}
}


/**
 *	Updates the back links for the passed object after crossing a chunk
 *	boundary.
 *
 *	@param	pLinkable	The object being updated.
 */
void EditorChunkItemLinkableManager::updateLocationInBackLinks(
	EditorChunkItemLinkable* pLinkable)
{
	BW_GUARD;

	// Need to run through pLinkable's back links, updating pLinkable's chunkId in all
	// properties that point the pLinkable.
	EditorChunkItemLinkable::LinksConstIter it = pLinkable->getBackLinksBegin();
	for ( ; it != pLinkable->getBackLinksEnd(); it++ )
	{
		// Force load the back link
		EditorChunkItemLinkable* backLink = forceLoad(it->UID_, it->CID_);

		// Update the chunkId of pLinkable in the properties of backLink
		updateChunkIdsInProps(backLink, pLinkable);
	}
}


/**
 *	Updates the managed lists the passed object after it crosses a chunk
 *	boundary.
 *
 *	@param	pLinkable	The object being updated.
 */
void EditorChunkItemLinkableManager::updateLocationInManagedLists(
	EditorChunkItemLinkable* pLinkable)
{
	BW_GUARD;

	ULMultimapIt it;
	UniqueID pLinkableID =  pLinkable->guid();
	BW::string outsideChunkId = pLinkable->getOutsideChunkId();

	// The object should be registered prior to calling this
	// method
	MF_ASSERT
		(
			isRegistered( pLinkableID ) &&
			"EditorChunkItemLinkableManager::updateLocationInManagedLists : "
			"pLinkable should be registered prior to calling this method!"
		);

	// Lock outstanding list mutex and limit scope
	{
		SimpleMutexHolder outstandingLinksMutex( outstandingLinksMutex_ );
		
		// Update all outstanding links from pLinkable in outstandingLinks_
		for (	it = outstandingLinks_.begin();
				it != outstandingLinks_.end();
				++it )
		{
			if (it->first == pLinkableID)
				it->second.fromCID_ = outsideChunkId;
		}
	}

	// Lock outstanding back list mutex and limit scope
	{
		SimpleMutexHolder outstandingBackLinksMutex( outstandingBackLinksMutex_ );
		
		// Update all outstanding back links to pLinkable in outstandingBackLinks_
		for (	it = outstandingBackLinks_.begin();
				it != outstandingBackLinks_.end();
				++it )
		{
			if (it->second.toUID_ == pLinkableID)
				it->second.toCID_ = outsideChunkId;
		}
	}

	// Lock chunk link mutex and limit scope
	{
		SimpleMutexHolder chunkLinksMutex( chunkLinksMutex_ );

		// Update all chunk links from pLinkable in chunkLinks_
		for (	it = chunkLinks_.begin();
				it != chunkLinks_.end();
				++it )
		{
			if (it->first == pLinkableID)
				it->second.fromCID_ = outsideChunkId;
		}
	}

	// Lock chunk back link mutex and limit scope
	{
		SimpleMutexHolder chunkBackLinksMutex( chunkBackLinksMutex_ );

		// Remove all chunk links from pLinkable in chunkLinks_
		for (	it = chunkBackLinks_.begin();
				it != chunkBackLinks_.end();
				++it )
		{
			if (it->second.toUID_ == pLinkableID)
				it->second.toCID_ = outsideChunkId;
		}
	}
}


/**
 *	Updates properties of a cloned linker object to point at the cloned copies
 *	of each link.  If a link reference has not been cloned the link is deleted.
 *
 *	@param	pCloneLinkable				The clone object being updated.
 *	@param	linkerCloneGuidMapping	The mapping of cloned linker objects.
 */
void EditorChunkItemLinkableManager::updateCloneProperties(
	EditorChunkItemLinkable* pCloneLinkable,
	GuidToGuidMap& linkerCloneGuidMapping)
{
	BW_GUARD;

	// Iterate through the properties of pCloneLinkable
	int propCounter = pCloneLinkable->propHelper()->propCount();
	for (int i=0; i < propCounter; i++)
	{
		DataDescription* pDD = pCloneLinkable->propHelper()->pType()->property( i );
		if (!pDD->editable())
			continue;

		// Is this property a object link
		if (pCloneLinkable->propHelper()->isUserDataObjectLink( i ))
		{
			PyObjectPtr ob
				(
					pCloneLinkable->propHelper()->propGetPy( i ),
					PyObjectPtr::STEAL_REFERENCE
				);

			// Get the GUID of the linked object
			BW::string strPropGuid =
				PyString_AsString(PyTuple_GetItem(ob.getObject(), 0));
			
			// Check if the object is not contained in the map
			UniqueID propGuid = linkerCloneGuidMapping[strPropGuid];
			if (propGuid == UniqueID())
			{
				// Delete the property
				pCloneLinkable->propHelper()->propSetToDefault(i);
				linkerCloneGuidMapping.erase(strPropGuid);
			}
			else
			{
				// Get the new cloned object fromt the registered list
				EditorChunkItemLinkable* pLinkableCloneLink = getRegistered( propGuid );
				MF_ASSERT
					(
						pLinkableCloneLink &&
						"EditorChunkItemLinkableManager::updateCloneProperties : \
						 Clone link should be registered!"
					);

				addLink(pCloneLinkable, pLinkableCloneLink, i, false);
			}
		}
		// Is this property an array of object links
		else if (pCloneLinkable->propHelper()->isUserDataObjectLinkArray( i ))
		{
			PyObjectPtr ob
				(
					pCloneLinkable->propHelper()->propGetPy( i ),
					PyObjectPtr::STEAL_REFERENCE
				);

			SequenceDataType* dataType =
				static_cast<SequenceDataType*>( pDD->dataType() );
			ArrayPropertiesHelper propArray;
			propArray.init(pCloneLinkable->chunkItem(), &(dataType->getElemType()), ob.getObject());

			// Iterate through the array of links
			for(int j = 0; j < propArray.propCount(); j++)
			{
				PyObjectPtr ob(
					propArray.propGetPy( j ), PyObjectPtr::STEAL_REFERENCE );

				// Get the GUID of the linked object
				BW::string strPropGuid =
					PyString_AsString(PyTuple_GetItem(ob.getObject(), 0));
				
				// Check if the object is not contained in the map
				UniqueID propGuid = linkerCloneGuidMapping[strPropGuid];
				if (propGuid == UniqueID())
				{
					// Delete the link
					propArray.delItem(j--);
					linkerCloneGuidMapping.erase(strPropGuid);
				}
				else
				{
					// Get the new cloned object fromt the registered list
					EditorChunkItemLinkable* pLinkableCloneLink = getRegistered( propGuid );
					MF_ASSERT
						(
							pLinkableCloneLink &&
							"EditorChunkItemLinkableManager::updateCloneProperties : \
							 Clone link should be registered!"
						);

					// Add the link again
					PropertyIndex propIndex(i);
					propIndex.append(j);
					addLink(pCloneLinkable, pLinkableCloneLink, propIndex, false);
				}
			}
		}
	}
}


/**
 *	Updates back links of a cloned linker object.  If a back link reference has
 *	not been cloned, delete it.
 *
 *	@param	pCloneLinkable				The clone object being updated.
 *	@param	linkerCloneGuidMapping	The mapping of cloned linker objects.
 */
void EditorChunkItemLinkableManager::updateCloneBackLinks(
	EditorChunkItemLinkable* pCloneLinkable,
	GuidToGuidMap& linkerCloneGuidMapping)
{
	BW_GUARD;

	// Need to run through pCloneLinkable's back links deleteing as necessary
	EditorChunkItemLinkable::LinksConstIter it;
	GuidToGuidMap::iterator it2;
	for(it = pCloneLinkable->getBackLinksBegin();
		it != pCloneLinkable->getBackLinksEnd();
		)
	{
		// Check if the back link is contained in the range of the map,
		// if it is, move on to the next back link
		bool newlyAdded = false;
		for(it2 = linkerCloneGuidMapping.begin();
			it2 != linkerCloneGuidMapping.end();
			++it2)
		{
			if (it->UID_ == it2->second)
			{
				newlyAdded = true;
				break;
			}
		}
		if (newlyAdded)
		{
			it++;
			continue;
		}

		// If the back link has not been freshly added, check if the back link
		// is contained in the domain of the map.  If it was not in the map, delete
		// the newly added GUID
		UniqueID propGuid = linkerCloneGuidMapping[it->UID_.toString()];
		if (propGuid == UniqueID())
		{
			linkerCloneGuidMapping.erase(it->UID_.toString());
		}

		// Check the special case were a link has been created from the original
		// linker object to this cloned copy by the postClone hook.
		// As long as this is not the case add the new link and delete the previous
		// link.
		else if (propGuid == pCloneLinkable->guid())
		{
			it++;
			continue;
		}
		else
		{
			// Add the new back link and delete the previous back link
			EditorChunkItemLinkable* newBackLink = getRegistered( propGuid );
			MF_ASSERT
				(
					newBackLink &&
					"EditorChunkItemLinkableManager::updateCloneBackLinks : \
					 Clone link should be registered!"
				);
			pCloneLinkable->addBackLink(newBackLink);
		}

		// Delete the old back link
		pCloneLinkable->removeBackLink(it->UID_, it->CID_);

		// Reset the iterator
		it = pCloneLinkable->getBackLinksBegin();
	}
}


/**
 *	Updates the chunkId for chunkChangedLinker in updatingLinker's properties.
 *
 *	@param	updatingLinker		The object whos properties are being
 *							updated.
 *	@param	chunkChangedLinker	The object that has changed location to a
 *							new chunk.
 */
void EditorChunkItemLinkableManager::updateChunkIdsInProps(
	EditorChunkItemLinkable* updatingLinker,
	EditorChunkItemLinkable* chunkChangedLinker)
{
	BW_GUARD;

	if (!updatingLinker)
	{
		ERROR_MSG( "EditorChunkItemLinkable::updateChunkIdsInProps : Cannot update, updateLinker is NULL.\n" );
		return;
	}

	// Iterate through the properties of updatingLinker
	int propCounter = updatingLinker->propHelper()->propCount();
	for (int i=0; i < propCounter; i++)
	{
		DataDescription* pDD = updatingLinker->propHelper()->pType()->property( i );
		if (!pDD->editable())
			continue;

		// Is this property a object link
		if (updatingLinker->propHelper()->isUserDataObjectLink( i ))
		{
			PyObjectPtr ob
				(
					updatingLinker->propHelper()->propGetPy( i ),
					PyObjectPtr::STEAL_REFERENCE
				);

			// If the property points to chunkChangedLinker
			if ( isLinkerReferencedInProp(chunkChangedLinker, ob))
			{
				// Reset the properties
				updatingLinker->propHelper()->propSetToDefault(i);

				// Add the link again
				addLinkInternal(updatingLinker, chunkChangedLinker, i);
			}
		}
		// Is this property an array of object links
		else if (updatingLinker->propHelper()->isUserDataObjectLinkArray( i ))
		{
			PyObjectPtr ob
				(
					updatingLinker->propHelper()->propGetPy( i ),
					PyObjectPtr::STEAL_REFERENCE
				);
			
			SequenceDataType* dataType =
				static_cast<SequenceDataType*>( pDD->dataType() );
			ArrayPropertiesHelper propArray;
			propArray.init(updatingLinker->chunkItem(), &(dataType->getElemType()), ob.getObject());

			// Iterate through the array of links
			for(int j = 0; j < propArray.propCount(); j++)
			{
				PyObjectPtr ob(
					propArray.propGetPy( j ), PyObjectPtr::STEAL_REFERENCE );

				// If the property points to chunkChangedLinker
				if ( isLinkerReferencedInProp(chunkChangedLinker, ob))
				{
					// Reset the properties
					propArray.propSetToDefault(j);

					// Add the link again
					PropertyIndex propIndex(i);
					propIndex.append(j);
					addLinkInternal(updatingLinker, chunkChangedLinker, propIndex);
				}
			}
		}
	}

	// Save changes
	updatingLinker->chunkItem()->edSave( updatingLinker->chunkItem()->pOwnSect() );
	if ( updatingLinker->chunkItem()->chunk() != NULL )
		WorldManager::instance().changedChunk(
			updatingLinker->chunkItem()->chunk(),
			InvalidateFlags::FLAG_NONE );
}
	
	
/**
 *	Inform the manager that a link exists between two linkable objects but that
 *	the linkTo object may or may not be loaded.
 *
 *	The linkTo object may not be loaded for a number of reasons:
 *
 *	1)	linkTo is in the same chunk as linkFrom, but is defined later in the
 *		file.  linkTo should be created by the time the manager's update method
 *		gets called.  A link will be created at that time.
 *	2)	linkTo is in another chunk that is not loaded.  In such a situation no
 *		attempt is made to force load the chunk.
 *
 *	@param	linkFromUID	The UniqueID of the source linkable object.
 *	@param	linkFromCID	The ChunkID of the source linkable object.
 *	@param	linkToUID	The UniqueID of the target linkable object.
 *	@param	linkToCID	The ChunkID of the target linkable object.
 */
void EditorChunkItemLinkableManager::addLink(
	const UniqueID& linkFromUID, const BW::string& linkFromCID,
	const UniqueID& linkToUID, const BW::string& linkToCID)
{
	BW_GUARD;

	// Return if the passed chunkId's are invalid
	if (linkFromCID == "" || linkToCID == "")
		return;
	
	// Return if the passed GUIDs are the same
	if (linkFromUID == linkToUID)
		return;
	
	// linkFrom should be registered
	MF_ASSERT(	isRegistered( linkFromUID ) &&
				"EditorChunkItemLinkableManager::addingLink : "
				"linkFrom is not registered!" );

	// Add the link to the list of outstanding links if the link is not in the
	// chunk links list or outstanding links list already
	Link link(linkFromUID, linkFromCID, linkToUID, linkToCID);
	ULPair linkPair(linkFromUID, link);
	SameLink linkEquality(linkPair);

	// Lock mutexes
	SimpleMutexHolder outstandingLinksMutex( outstandingLinksMutex_ );
	SimpleMutexHolder chunkLinksMutex( chunkLinksMutex_ );
	
	if (find_if
			(
				outstandingLinks_.begin(),
				outstandingLinks_.end(),
				linkEquality
			) == outstandingLinks_.end() &&
		find_if
			(
				chunkLinks_.begin(),
				chunkLinks_.end(),
				linkEquality
			) == chunkLinks_.end()
		)
		outstandingLinks_.insert(linkPair);
}


/**
 *	Adds a link to the outstanding back links.
 *
 *	@param	linkFromUID	The UniqueID of the source linkable object.
 *	@param	linkFromCID	The ChunkID of the source linkable object.
 *	@param	linkToUID	The UniqueID of the target linkable object.
 *	@param	linkToCID	The ChunkID of the target linkable object.
 */
void EditorChunkItemLinkableManager::addLinkToOutstandingBackLinks(
	const UniqueID& linkFromUID, const BW::string& linkFromCID,
	const UniqueID& linkToUID, const BW::string& linkToCID)
{
	BW_GUARD;

	// Return is the passed chunkId's are invalid
	if (linkFromCID == "" || linkToCID == "")
		return;
	
	// Return if the passed GUIDs are the same
	if (linkFromUID == linkToUID)
		return;
	
	// linkFrom should be in registeredLinkers_
	MF_ASSERT(	isRegistered( linkFromUID ) &&
				"EditorChunkItemLinkableManager::addLinkToOutstandingBackLinks : "
				"linkFrom is not registered!" );

	// Only add if the target is loaded
	if (isRegistered(linkToUID))
	{
		// Add the link to the list of outstanding back links if the link is not in
		// the chunk back links list or outstanding back links list already
		Link link(linkFromUID, linkFromCID, linkToUID, linkToCID);
		ULPair linkPair(linkFromUID, link);
		SameLink linkEquality(linkPair);

		// Lock mutexes
		SimpleMutexHolder outstandingBackLinksMutex( outstandingBackLinksMutex_ );
		SimpleMutexHolder chunkBackLinksMutex( chunkBackLinksMutex_ );
				
		if (find_if
				(
					outstandingBackLinks_.begin(),
					outstandingBackLinks_.end(),
					linkEquality
				) == outstandingBackLinks_.end() &&
			find_if
				(
					chunkBackLinks_.begin(),
					chunkBackLinks_.end(),
					linkEquality
				) == chunkBackLinks_.end()
			)
			outstandingBackLinks_.insert(linkPair);
	}
}


/**
 *	Adds a link between two linkable objects.
 *
 *	@param	pLinkable		The source linker.
 *	@param	pPyLinkProp	The pyObject property with the target information.
 */
void EditorChunkItemLinkableManager::addLink(
	const EditorChunkItemLinkable* pLinkable, PyObjectPtr pPyLinkProp)
{
	BW_GUARD;

	// Need to obtain the unique ids and chunk ids of the linker
	BW::string uniqueId = PyString_AsString( PyTuple_GetItem( pPyLinkProp.getObject(), 0 ) );
	BW::string chunkId = PyString_AsString( PyTuple_GetItem( pPyLinkProp.getObject(), 1 ) );
	if (uniqueId == "" || chunkId == "")
		return;

	// Register the link with the EditorChunkItemLinkableManager
	addLink(
		pLinkable->guid(), pLinkable->getOutsideChunkId(),
		UniqueID(uniqueId), chunkId );
}


/**
 *	Adds a link to the outstanding back links list.
 *
 *	@param	pLinkable		The source linker.
 *	@param	pPyLinkProp	The pyObject property with the target information.
 */
void EditorChunkItemLinkableManager::addLinkToOutstandingBackLinks(
	const EditorChunkItemLinkable* pLinkable, PyObjectPtr pPyLinkProp)
{
	BW_GUARD;

	// Need to obtain the unique ids and chunk ids of the linker
	BW::string uniqueId = PyString_AsString( PyTuple_GetItem( pPyLinkProp.getObject(), 0 ) );
	BW::string chunkId = PyString_AsString( PyTuple_GetItem( pPyLinkProp.getObject(), 1 ) );
	if (uniqueId == "" || chunkId == "")
		return;

	// Add to outstanding back links list
	addLinkToOutstandingBackLinks(
		pLinkable->guid(), pLinkable->getOutsideChunkId(),
		UniqueID(uniqueId), chunkId );
}


/**
 *	Loads in all the links for the passed linker.
 *
 *	@param	pLinkable	The linker whos links are to be added.
 */
void EditorChunkItemLinkableManager::addAllLinksToOutstandingList(
	EditorChunkItemLinkable* pLinkable)
{
	BW_GUARD;

	// Iterate through the properties of this linker
	int propCounter = pLinkable->propHelper()->propCount();
	for (int i=0; i < propCounter; i++)
	{
		DataDescription* pDD = pLinkable->propHelper()->pType()->property( i );
		if (!pDD->editable())
			continue;

		// If this property is a linkable property
		if (pLinkable->propHelper()->isUserDataObjectLink( i ))
		{
			PyObjectPtr ob
				(
					pLinkable->propHelper()->propGetPy( i ),
					PyObjectPtr::STEAL_REFERENCE
				);

			// Add the link
			addLink(pLinkable, ob);
		}
		// If this property is an array of linkable properties
		else if (pLinkable->propHelper()->isUserDataObjectLinkArray( i ))
		{
			PyObjectPtr ob
				(
					pLinkable->propHelper()->propGetPy( i ),
					PyObjectPtr::STEAL_REFERENCE
				);
			
			SequenceDataType* dataType =
				static_cast<SequenceDataType*>( pDD->dataType() );
			ArrayPropertiesHelper propArray;
			propArray.init(pLinkable->chunkItem(), &(dataType->getElemType()), ob.getObject());

			// Iterate through the array of links
			for(int j = 0; j < propArray.propCount(); j++)
			{
				PyObjectPtr link(
					propArray.propGetPy( j ), PyObjectPtr::STEAL_REFERENCE );

				// Add the link
				addLink(pLinkable, link);
			}
		}
	}
}


/**
 *	Adds all forward links to the outstanding back links list.
 *
 *	@param	pLinkable	The linker whos links are to be added.
 */
void EditorChunkItemLinkableManager::addAllLinksToOutstandingBackLinksList(
	EditorChunkItemLinkable* pLinkable)
{
	BW_GUARD;

	// Iterate through the properties of this linker
	int propCounter = pLinkable->propHelper()->propCount();
	for (int i=0; i < propCounter; i++)
	{
		DataDescription* pDD = pLinkable->propHelper()->pType()->property( i );
		if (!pDD->editable())
			continue;

		// If this property is a linkable property
		if (pLinkable->propHelper()->isUserDataObjectLink( i ))
		{
			PyObjectPtr ob
				(
					pLinkable->propHelper()->propGetPy( i ),
					PyObjectPtr::STEAL_REFERENCE
				);

			// Add the link to the outstanding back links list
			addLinkToOutstandingBackLinks(pLinkable, ob);
		}
		// If this property is an array of linkable properties
		else if (pLinkable->propHelper()->isUserDataObjectLinkArray( i ))
		{
			PyObjectPtr ob
				(
					pLinkable->propHelper()->propGetPy( i ),
					PyObjectPtr::STEAL_REFERENCE
				);

			SequenceDataType* dataType =
				static_cast<SequenceDataType*>( pDD->dataType() );
			ArrayPropertiesHelper propArray;
			propArray.init(pLinkable->chunkItem(), &(dataType->getElemType()), ob.getObject());

			// Iterate through the array of links
			for(int j = 0; j < propArray.propCount(); j++)
			{
				PyObjectPtr link(
					propArray.propGetPy( j ), PyObjectPtr::STEAL_REFERENCE );

				// Add the link to the outstanding back links list
				addLinkToOutstandingBackLinks(pLinkable, link);
			}
		}
	}
}


/**
 *	Adds all the back links of the passed object to the back links outstanding
 *	list.  This will likely result in the creation of many chunk back links for
 *	this object when the update method is called.
 *
 *	@param	pLinkable	The linker whos back links are to be added.
 */
void EditorChunkItemLinkableManager::addAllBackLinksToOutstandingBackLinksList(
	EditorChunkItemLinkable* pLinkable)
{
	BW_GUARD;

	// pLinkable should be in registeredLinkers_ since it should have registered
	// itself before calling this method
	MF_ASSERT(	isRegistered( pLinkable->guid() ) &&
				"EditorChunkItemLinkableManager::addAllBackLinksToOutstandingBackLinksList : "
				"pLinkable is not registered!" );

	// Iterate through pLinkable's back links adding each to the outstanding back
	// links list
	EditorChunkItemLinkable::LinksConstIter it;
	for (
			it = pLinkable->getBackLinksBegin();
			it != pLinkable->getBackLinksEnd();
			it++
		)
	{
		// Add the link to the list of outstanding links if there is not a link
		// between them in the list already
		Link link(it->UID_, it->CID_, pLinkable->guid(), pLinkable->getOutsideChunkId());
		ULPair linkPair(it->UID_, link);
		SameLink linkEquality(linkPair);

		// Lock mutex
		SimpleMutexHolder outstandingBackLinksMutex( outstandingBackLinksMutex_ );
		
		if (find_if
				(
					outstandingBackLinks_.begin(),
					outstandingBackLinks_.end(),
					linkEquality
				) == outstandingBackLinks_.end() )
			outstandingBackLinks_.insert(linkPair);
	}
}


/**
 *	Adds all the back links of the passed object to the outstanding list.
 *	This will likely result in the creation of many chunk links for the back
 *	linked objects of this object when the update method is called.
 *
 *	@param	pLinkable	The linker whos back linked objects will be added.
 */
void EditorChunkItemLinkableManager::addAllBacklinksToOutstandingList(
	EditorChunkItemLinkable* pLinkable)
{
	BW_GUARD;

	// The object should be registered prior to calling this
	// method
	MF_ASSERT
		(
			isRegistered( pLinkable->guid() ) &&
			"EditorChunkItemLinkableManager::addBacklinksToOutstandingList : "
			"pLinkable should be registered prior to calling this method!"
		);

	// Need to run through pLinkable's back links adding an outstanding link for
	// each loaded object
	EditorChunkItemLinkable::LinksConstIter it = pLinkable->getBackLinksBegin();
	for ( ; it != pLinkable->getBackLinksEnd(); it++ )
	{
		// Check if the back link is loaded
		if (isRegistered(it->UID_))
		{
			// Add a link
			addLink(
				it->UID_, it->CID_,
				pLinkable->guid(), pLinkable->getOutsideChunkId());
		}
	}
}


/**
 *	Removes all links in "from" that point to "pointingTo".
 *
 *	@param	from	The object whos properties are getting links
 *					removed from.
 *	@param	of		The object being refered to by the properties.
 */
void EditorChunkItemLinkableManager::removeAllRefs(
	EditorChunkItemLinkable* from, EditorChunkItemLinkable* pointingTo)
{
	BW_GUARD;

	MF_ASSERT(from &&
		"EditorChunkItemLinkableManager::removeAllRefs : "
		"Received NULL pointer for from!" );

	// Iterate through the properties of from
	int propCounter = from->propHelper()->propCount();
	for (int i=0; i < propCounter; i++)
	{
		DataDescription* pDD = from->propHelper()->pType()->property( i );
		if (!pDD->editable())
			continue;

		// Is this property a object link
		if (from->propHelper()->isUserDataObjectLink( i ))
		{
			PyObjectPtr ob
				(
					from->propHelper()->propGetPy( i ),
					PyObjectPtr::STEAL_REFERENCE
				);

			EditorChunkItemLinkable* pLinkable = forceLoad(ob);
			if ((pointingTo == 0) ||
				(pLinkable && (pLinkable->guid() == pointingTo->guid())))
			{
				deleteLink(from, pLinkable, i);
			}
		}
		// Is this property an array of object links
		else if (from->propHelper()->isUserDataObjectLinkArray( i ))
		{
			PyObjectPtr ob
				(
					from->propHelper()->propGetPy( i ),
					PyObjectPtr::STEAL_REFERENCE
				);

			SequenceDataType* dataType =
				static_cast<SequenceDataType*>( pDD->dataType() );
			ArrayPropertiesHelper propArray;
			propArray.init(from->chunkItem(), &(dataType->getElemType()), ob.getObject());

			// Iterate through the array of links
			for(int j = 0; j < propArray.propCount(); j++)
			{
				PyObjectPtr ob(
					propArray.propGetPy( j ), PyObjectPtr::STEAL_REFERENCE );

				EditorChunkItemLinkable* pLinkable = forceLoad(ob);
				if ((pointingTo == 0) ||
					(pLinkable && (pLinkable->guid() == pointingTo->guid())))
				{
					// Reset the properties
					PropertyIndex propInx(i);
					propInx.append(j--);
					deleteLink(from, pLinkable, propInx);
				}
			}
		}
	}

	// Save changes
	if (from->chunkItem()->pOwnSect())
	{
		from->chunkItem()->edSave( from->chunkItem()->pOwnSect() );
	}
	if ( from->chunkItem()->chunk() != NULL )
		WorldManager::instance().changedChunk(
			from->chunkItem()->chunk(),
			InvalidateFlags::FLAG_NONE );
}


/**
 *	Removes all back links to a particlular object.
 *
 *	@param	to	The object being refered to.
 */
void EditorChunkItemLinkableManager::removeAllBackLinksToLinker(
	EditorChunkItemLinkable* pLinkable)
{
	BW_GUARD;

	// Iterate through the properties of from
	bool found = false;
	int propCounter = pLinkable->propHelper()->propCount();
	for (int i=0; i < propCounter; i++)
	{
		DataDescription* pDD = pLinkable->propHelper()->pType()->property( i );
		if (!pDD->editable())
			continue;

		// Is this property a object link
		if (pLinkable->propHelper()->isUserDataObjectLink( i ))
		{
			PyObjectPtr ob
				(
					pLinkable->propHelper()->propGetPy( i ),
					PyObjectPtr::STEAL_REFERENCE
				);

			// Remove the back link in linker to pLinkable
			EditorChunkItemLinkable* linker = forceLoad(ob);

			if (linker)
				linker->removeBackLink(pLinkable);
		}
		// Is this property an array of object links
		else if (pLinkable->propHelper()->isUserDataObjectLinkArray( i ))
		{
			PyObjectPtr ob
				(
					pLinkable->propHelper()->propGetPy( i ),
					PyObjectPtr::STEAL_REFERENCE
				);

			SequenceDataType* dataType =
				static_cast<SequenceDataType*>( pDD->dataType() );
			ArrayPropertiesHelper propArray;
			propArray.init(pLinkable->chunkItem(), &(dataType->getElemType()), ob.getObject());

			// Iterate through the array of links
			for(int j = 0; j < propArray.propCount(); j++)
			{
				PyObjectPtr ob(
					propArray.propGetPy( j ), PyObjectPtr::STEAL_REFERENCE );

				// Remove the back link in linker to pLinkable
				EditorChunkItemLinkable* linker = forceLoad(ob);

				if (linker)
					linker->removeBackLink(pLinkable);
			}
		}
	}
}


//------------------------------------------------------------------------------
//	LINK STRUCTURE SECTION
//------------------------------------------------------------------------------

/** Constructor */
EditorChunkItemLinkableManager::Link::Link(
	UniqueID fromUID, BW::string fromCID, UniqueID toUID, BW::string toCID) :
		fromUID_(fromUID), fromCID_(fromCID), toUID_(toUID), toCID_(toCID)
{}


/**
 *	Tests for equality.
 *
 *	@param	rhs	The link on the right hand side of the equality operator.
 *	@return		True if the two links are equal, false otherwise.
 */
bool EditorChunkItemLinkableManager::Link::operator==(const Link& rhs) const
{
 	return (
		this->fromUID_ == rhs.fromUID_ && this->fromCID_ == rhs.fromCID_ &&
		this->toUID_ == rhs.toUID_ && this->toCID_ == rhs.toCID_);
}


//------------------------------------------------------------------------------
// PYTHON-EXPOSED FUNCTIONS
//------------------------------------------------------------------------------

/*~ function WorldEditor.udoCreateLink
 *	@components{ worldeditor }
 *
 *	This function exposes creation of links to python.
 *
 *	@param guid1		GUID of the object at the start of the link.
 *	@param chunkId1		Chunk id of the object at the start of the link.
 *	@param guid2		GUID of the object at the end of the link.
 *	@param chunkId2		Chunk id of the object at the end of the link.
 *	@param propName		Name of the property that contains the link.
 */
static void udoCreateLink(
				const BW::string& guid1, const BW::string& chunkId1,
				const BW::string& guid2, const BW::string& chunkId2,
				const BW::string& propName )
{
	BW_GUARD;

	if ( guid1 == guid2 )
		return;

	EditorChunkItemLinkable* startIL = WorldManager::instance().linkerManager().forceLoad( guid1, chunkId1 );
	EditorChunkItemLinkable* endIL = WorldManager::instance().linkerManager().forceLoad( guid2, chunkId2 );

	if (!startIL)
	{
		ERROR_MSG(
			"Call to udoCreateLink failed: Couldn't find linked object %s in chunk %s.\n",
			guid1.c_str(), chunkId1.c_str() );
		return;
	}

	if (!endIL)
	{
		ERROR_MSG(
			"Call to udoCreateLink failed: Couldn't find linked object %s in chunk %s.\n",
			guid2.c_str(), chunkId2.c_str() );
		return;
	}

	if (!startIL->chunkItem()->isEditorUserDataObject())
	{
		ERROR_MSG(
			"Call to udoCreateLink failed: start item %s in chunk %s is not a User Data Object.\n",
			guid1.c_str(), chunkId1.c_str() );
		return;
	}

	if (!endIL->chunkItem()->isEditorUserDataObject())
	{
		ERROR_MSG(
			"Call to udoCreateLink failed: end item %s in chunk %s is not a User Data Object.\n",
			guid2.c_str(), chunkId2.c_str() );
		return;
	}

	EditorChunkUserDataObject* start = static_cast<EditorChunkUserDataObject*>(startIL->chunkItem());
    EditorChunkUserDataObject* end = static_cast<EditorChunkUserDataObject*>(endIL->chunkItem());
	
	if (!start->canLinkTo( propName, end ))
		return;

	PropertyIndex propIdx = start->propHelper()->propGetIdx( propName );
	if (start->propHelper()->isUserDataObjectLinkArray( propIdx.valueAt( 0 ) ) )
	{
		// it's an array, so must add a new item to it
		PyObjectPtr ob( start->propHelper()->propGetPy( propIdx ), PyObjectPtr::STEAL_REFERENCE );
		SequenceDataType* dataType =
			static_cast<SequenceDataType*>( start->propHelper()->pType()->property( propIdx.valueAt( 0 ) )->dataType() );
		ArrayPropertiesHelper propArray;
		propArray.init( start, &(dataType->getElemType()), ob.getObject() );
		propArray.addItem();
		// also, add a second level index for the item in the array
		propIdx.append( propArray.propCount() - 1 );
	}
	WorldManager::instance().linkerManager().addLink( start->chunkItemLinker(), end->chunkItemLinker(), propIdx );
}
PY_AUTO_MODULE_FUNCTION( RETVOID, udoCreateLink,
							ARG( BW::string /*guid1*/,
							ARG( BW::string /*chunkId1*/,
							ARG( BW::string /*guid2*/,
							ARG( BW::string /*chunkId2*/,
							ARG( BW::string /*propName*/,
							END ))))),
						WorldEditor );


/*~ function WorldEditor.udoCreateAtPosition
 *	@components{ worldeditor }
 *
 *	This static function returns a copy of the object identified
 *	by the guid and chunk id passed in. The new UDO will have empty properties.
 *
 *	@param guid				GUID of the UDO at the start of the link.
 *	@param chunkId			Chunk id of the UDO at the start of the link.
 *	@param pos				Desired position for the new node.
 *	@param snapToTerrain	Desired position for the new node.
 */
static PyObject* udoCreateAtPosition(
				const BW::string& guid, const BW::string& chunkId,
				const Vector3& pos, bool snapToTerrain )
{
	BW_GUARD;

	// get chunk position
	GeometryMapping* dirMap = WorldManager::instance().geometryMapping();
	BW::string newChunkId = dirMap->outsideChunkIdentifier( pos );
	int16 gridX, gridY;
	dirMap->gridFromChunkName( newChunkId, gridX, gridY );
	ChunkManager::instance().loadChunkNow( newChunkId, dirMap );
	ChunkSpacePtr pSpace = ChunkManager::instance().cameraSpace();
	ChunkPtr newChunk = pSpace->findChunk( newChunkId, "" );

	// get the UserDataObject to copy
	EditorChunkItemLinkable* pLinkable = WorldManager::instance().linkerManager().forceLoad( guid, chunkId );
	if ( !pLinkable )
	{
		ERROR_MSG(
			"Call to udoCreateAtPosition failed: Couldn't find linked object %s in chunk %s.\n",
			guid.c_str(), chunkId.c_str() );
		return NULL;
	}

	if (!pLinkable->chunkItem()->isEditorUserDataObject())
	{
		ERROR_MSG(
			"Call to udoCreateAtPosition failed: %s in chunk %s is not a User Data Object.\n",
			guid.c_str(), chunkId.c_str() );
		return NULL;
	}

	// copy it and clear its properties and guid (a new guid will be generated on load)
    EditorChunkUserDataObjectPtr newNode = new EditorChunkUserDataObject();
    DataSectionPtr newSection = new XMLSection("copy");
    newSection->copy( pLinkable->chunkItem()->pOwnSect() );
	newSection->delChild( "guid" );
	newSection->delChild( "properties" );

    newNode->load( newSection, newChunk );
	newChunk->addStaticItem( newNode );
	Vector3 localPos( newChunk->transformInverse().applyPoint( pos ) );
	if ( snapToTerrain )
	{
		ChunkTerrain* terrain = ChunkTerrainCache::instance( *newChunk ).pTerrain();
		if ( terrain!= NULL )
			localPos[1] = terrain->block()->heightAt( localPos[0], localPos[2] );
	}
	Matrix t;
	t.setTranslate( localPos );
	newNode->edTransform( t, false );

	UndoRedo::instance().add(
		new ChunkItemExistenceOperation( newNode, NULL ) );

	// infoDict creates a new dict
	return newNode->infoDict();
}
PY_AUTO_MODULE_FUNCTION( RETOWN, udoCreateAtPosition,
							ARG( BW::string /*guid*/,
							ARG( BW::string /*chunkId*/,
							ARG( Vector3 /*position*/,
							OPTARG( bool, true, /*snap to terrain*/
							END )))),
						WorldEditor );


/*~ function WorldEditor.udoDeleteLinks
 *	@components{ worldeditor }
 *
 *	This static function deletes all links between two UDOs.
 *
 *	@param guid1		GUID of the UDO at the start of the link.
 *	@param chunkId1		Chunk id of the UDO at the start of the link.
 *	@param guid2		GUID of the UDO at the end of the link.
 *	@param chunkId2		Chunk id of the UDO at the end of the link.
 */
static void udoDeleteLinks(
				const BW::string& guid1, const BW::string& chunkId1,
				const BW::string& guid2, const BW::string& chunkId2 )
{
	BW_GUARD;

	EditorChunkItemLinkable* udo1 = WorldManager::instance().linkerManager().forceLoad( guid1, chunkId1 );
	EditorChunkItemLinkable* udo2 = WorldManager::instance().linkerManager().forceLoad( guid2, chunkId2 );

	if (!udo1)
	{
		ERROR_MSG(
			"Call to udoDeleteLinks failed: Couldn't find linked object %s in chunk %s.\n",
			guid1.c_str(), chunkId1.c_str() );
		return;
	}

	if (!udo2)
	{
		ERROR_MSG(
			"Call to udoDeleteLinks failed: Couldn't find linked object %s in chunk %s.\n",
			guid2.c_str(), chunkId2.c_str() );
		return;
	}

	
	WorldManager::instance().linkerManager().deleteAllLinks( udo1, udo2 );
}
PY_AUTO_MODULE_FUNCTION( RETVOID, udoDeleteLinks,
							ARG( BW::string /*guid1*/,
							ARG( BW::string /*chunkId1*/,
							ARG( BW::string /*guid2*/,
							ARG( BW::string /*chunkId2*/,
							END )))),
						WorldEditor );


/*~ function WorldEditor.udoGet
 *	@components{ worldeditor }
 *
 *	Returns the object that matches the guid.
 *
 *	@param guid			GUID of the desired UDO.
 *	@param chunkId		Chunk id of the desired UDO
 */
static PyObject* udoGet( const BW::string& guid, const BW::string& chunkId )
{
	BW_GUARD;

	EditorChunkItemLinkable* pLinkable = WorldManager::instance().linkerManager().forceLoad( guid, chunkId );

	if ( !pLinkable )
	{
		ERROR_MSG(
			"Call to udoGet failed: Couldn't find linked object %s in chunk %s.\n",
			guid.c_str(), chunkId.c_str() );
		return NULL;
	}

	if (!pLinkable->chunkItem()->isEditorUserDataObject())
	{
		ERROR_MSG(
			"Call to udoGet failed: %s in chunk %s is not a User Data Object.\n",
			guid.c_str(), chunkId.c_str() );
		return NULL;
	}

	// infoDict returns a new dict
	return static_cast<EditorChunkUserDataObject*>(pLinkable->chunkItem())->infoDict();
}
PY_AUTO_MODULE_FUNCTION( RETOWN, udoGet,
							ARG( BW::string /*guid*/,
							ARG( BW::string /*chunkId*/,
							END )),
						WorldEditor );

BW_END_NAMESPACE

