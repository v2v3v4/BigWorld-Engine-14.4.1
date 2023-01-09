#include "pch.hpp"
#include "worldeditor/world/editor_chunk_item_linker.hpp"
#include "worldeditor/world/editor_chunk_item_linker_manager.hpp"
#include "worldeditor/world/items/editor_chunk_user_data_object.hpp"
#include "worldeditor/world/items/editor_chunk_user_data_object_link.hpp"
#include "worldeditor/world/items/editor_chunk_point_link.hpp"
#include "worldeditor/world/items/editor_chunk_entity.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "chunk/chunk_space.hpp"
#include "chunk/geometry_mapping.hpp"
#include "common/array_properties_helper.hpp"
#include "common/properties_helper.hpp"
#include "entitydef/data_types.hpp"
#include "entitydef/data_types/sequence_data_type.hpp"

BW_BEGIN_NAMESPACE

static const float CHUNK_LINK_SIZE = 8.0f;


namespace
{
	bool isLinkOwner( EditorChunkItemLinkable * linker, EditorChunkItemLinkable * endLinker )
	{
		BW_GUARD;

	    if (linker->chunkItem()->isEditorEntity())
		{
			// Entity->UDO:  True, the entity is the owner.
			return true;
		}
		else if (endLinker->chunkItem()->isEditorEntity())
		{
			// UDO->Entity:  False, the end entity is the owner.
			return false;
		}

		// UDO->UDO:  Simply choose one UDO consistently over the other.
		return linker->guid() < endLinker->guid();
	}
} // anonymous namespace


//------------------------------------------------------------------------------
// LINK CLASS DEFINITIONS
//------------------------------------------------------------------------------
/**
 *	Constructor
 *
 *	@param UID	The unique id of the item being linked to.
 *	@param CID	The chunk id of the item being linked to.
 */
EditorChunkItemLinkable::Link::Link(UniqueID UID, BW::string CID) :
	UID_(UID), CID_(CID)
{
}


/**
 *	Method used to order links based on their unique id's
 *
 *	@param rhs	The link on the right hand side of the less than operator.
 *	@return		True if this object preceeds rhs, false otherwise.
 */
bool EditorChunkItemLinkable::Link::operator<( const Link& rhs ) const
{
	return UID_ < rhs.UID_;
}



//------------------------------------------------------------------------------
//	EDITOR CHUNK ITEM LINKER DEFINITIONS
//------------------------------------------------------------------------------
/**
 *	Constructor
 *
 *	@param UID	The unique id of the item being linked to.
 *	@param CID	The chunk id of the item being linked to.
 */
EditorChunkItemLinkable::EditorChunkItemLinkable(
	ChunkItem* chunkItem, UniqueID guid, PropertiesHelper* propHelper) :
		chunkItem_(chunkItem), guid_(guid), propHelper_(propHelper)
{
}


/**
 *	Destructor
 */
EditorChunkItemLinkable::~EditorChunkItemLinkable()
{
}


/**
 *	Called when chunkItem_ has been added to a chunk.
 */
void EditorChunkItemLinkable::tossAdd()
{
	BW_GUARD;

	WorldManager::instance().linkerManager().tossAdd(this);
}


/**
 *	Called when chunkItem_ has been removed from a chunk.
 */
void EditorChunkItemLinkable::tossRemove()
{
	BW_GUARD;

	WorldManager::instance().linkerManager().tossRemove(this);
}


/**
 *	Called when chunkItem_ has been deleted.
 */
void EditorChunkItemLinkable::deleted()
{
	BW_GUARD;

	WorldManager::instance().linkerManager().deleted(this);
}


/**
 *	Called when cloned to clear the cloned links.
 */
void EditorChunkItemLinkable::clearAllLinks()
{
	BW_GUARD;

	// Clear from managers lists
	WorldManager::instance().linkerManager().removeFromLists(this);

	// Iterate through the properties of this linker
	int propCounter = propHelper()->propCount();
	for (int i=0; i < propCounter; i++)
	{
		DataDescription* pDD = propHelper()->pType()->property( i );
		if (!pDD->editable())
			continue;

		// If this property is a linkable property
		if (propHelper()->isUserDataObjectLink( i ))
		{
			propHelper()->propSetToDefault(i);
		}
		// If this property is an array of linkable properties
		else if (propHelper()->isUserDataObjectLinkArray( i ))
		{
			PyObjectPtr ob
				(
					propHelper()->propGetPy( i ),
					PyObjectPtr::STEAL_REFERENCE
				);

			SequenceDataType* dataType =
				static_cast<SequenceDataType*>( pDD->dataType() );
			ArrayPropertiesHelper propArray;
			propArray.init(chunkItem(), &(dataType->getElemType()), ob.getObject());

			// Iterate through the array of links
			while (propArray.propCount())
			{
				// Delete the link
				propArray.delItem(0);
			}
		}
	}

	// Save changes
	chunkItem()->edSave( chunkItem()->pOwnSect() );
	if ( chunkItem()->chunk() != NULL )
		WorldManager::instance().changedChunk( 
			chunkItem()->chunk(),
			InvalidateFlags::FLAG_NONE );
}


/**
 *	Returns the chunk id of the outside chunk this item is contained in.
 *
 *	@return	The outside chunk id of this item.
 */
BW::string EditorChunkItemLinkable::getOutsideChunkId() const
{
	BW_GUARD;

	Chunk* chunk = chunkItem()->chunk(); 

	// If chunk is NULL return the empty string
	if (!chunk)
	{
		return "";
	}
	
	// If this linker's chunk is an outside chunk
	if (chunk->isOutsideChunk())
	{
		// Return its id
		return chunk->identifier();
	}

	// Otherwise this linker's chunk is an inside chunk.  Find the location
	// of this chunk in absolute coordinates
	Vector3 absPosition = chunk->transform().applyToOrigin();

	// Get the outside chunk at this position
	// get chunk position
	GeometryMapping* dirMap = WorldManager::instance().geometryMapping();
	if (dirMap)
	{
		return dirMap->outsideChunkIdentifier( absPosition );
	}
	else
	{
		return "";
	}
}


/**
 *	Returns the unique id of this item.
 *
 *	@return	The unique id of this item.
 */
UniqueID EditorChunkItemLinkable::guid() const
{
	return guid_;
};


/**
 *	Sets the unique id of this item.
 *
 *	@param	guid	The new unique id of this item.
 */
void EditorChunkItemLinkable::guid(UniqueID guid)
{
	BW_GUARD;

	// Inform the manager that my guid is being changed
	WorldManager::instance().linkerManager().idPreChange(guid_, guid);
	guid_ = guid;
};


/**
 *	Method indicating whether this item's chunk is writable.
 *
 *	@return	True if writable, false otherwise.
 */
bool EditorChunkItemLinkable::linkedChunksWriteable()
{
	BW_GUARD;

	GeometryMapping* dirMap = WorldManager::instance().geometryMapping();
	int16 gridX, gridZ;

	// process forward links
	int propCounter = propHelper()->propCount();
	for (int i=0; i < propCounter; i++)
	{
		DataDescription* pDD = propHelper()->pType()->property( i );
		if (!pDD->editable())
			continue;

		// Is this property a user data object link
		if (propHelper()->isUserDataObjectLink( i ))
		{
			PyObjectPtr ob( propHelper()->propGetPy( i ), PyObjectPtr::STEAL_REFERENCE );

			BW::string id = PyString_AsString( PyTuple_GetItem( ob.getObject(), 0 ) );
			BW::string chunkId = PyString_AsString( PyTuple_GetItem( ob.getObject(), 1 ) );
			if ( id == "" || chunkId == "" )
				continue;

			// Force load the linker since we need to examine its parent chunk
			EditorChunkItemLinkable* linkableUDO =
				WorldManager::instance().linkerManager().forceLoad( id, chunkId );
			
			// If the chunk is not an outside chunk, see if it's writeable
			if (linkableUDO && linkableUDO->chunkItem())
			{
				if (!linkableUDO->chunkItem()->edIsEditable())
				{
					return false;
				}
			}
			else
			{
				ERROR_MSG(
						"EditorChunkItemLinkable::linkedChunksWriteable : "
						"Need access to parent chunk!\n" );
				if (!dirMap->gridFromChunkName( chunkId, gridX, gridZ ) ||
					!EditorChunk::outsideChunkWriteable( gridX, gridZ, false ) )
				{
					return false;
				}
			}
		}
		// Is this property an array of user data object links
		else if (propHelper()->isUserDataObjectLinkArray( i ))
		{
			PyObjectPtr ob(	propHelper()->propGetPy( i ), PyObjectPtr::STEAL_REFERENCE );

			SequenceDataType* dataType =
				static_cast<SequenceDataType*>( pDD->dataType() );
			ArrayPropertiesHelper propArray;
			propArray.init(this->chunkItem(),&(dataType->getElemType()), ob.getObject());

			// Iterate through the array of links
			for(int j = 0; j < propArray.propCount(); j++)
			{
				PyObjectPtr aob( propArray.propGetPy( j ), PyObjectPtr::STEAL_REFERENCE );
				BW::string id = PyString_AsString( PyTuple_GetItem( aob.getObject(), 0 ) );
				BW::string chunkId = PyString_AsString( PyTuple_GetItem( aob.getObject(), 1 ) );
				if ( id == "" || chunkId == "" )
					continue;

				// Force load the linker since we need to examine its parent chunk
				EditorChunkItemLinkable* linkableUDO =
					WorldManager::instance().linkerManager().forceLoad( id, chunkId );
				
				// If the chunk is not an outside chunk, see if it's writeable
				if (linkableUDO && linkableUDO->chunkItem())
				{
					if (!linkableUDO->chunkItem()->edIsEditable())
					{
						return false;
					}
				}
				else
				{
					ERROR_MSG(
							"EditorChunkItemLinkable::linkedChunksWriteable : "
							"Need access to parent chunk!\n" );

					if (!dirMap->gridFromChunkName( chunkId, gridX, gridZ ) ||
						!EditorChunk::outsideChunkWriteable( gridX, gridZ, false ) )
					{
						return false;
					}
				}
			}
		}
	}

	// process back links
	for ( LinksConstIter it = getBackLinksBegin();
		it != getBackLinksEnd(); ++it )
	{
		// Force load the linker since we need to examine its parent chunk
		EditorChunkItemLinkable* linkableUDO =
			WorldManager::instance().linkerManager().forceLoad( (*it).UID_, (*it).CID_ );
		
		// If the chunk is not an outside chunk, see if it's writeable
		if (linkableUDO && linkableUDO->chunkItem())
		{
			if (!linkableUDO->chunkItem()->edIsEditable())
			{
				return false;
			}
		}
		else
		{
			ERROR_MSG(
					"EditorChunkItemLinkable::linkedChunksWriteable : "
					"Need access to parent chunk!\n" );

			if (!dirMap->gridFromChunkName( (*it).CID_, gridX, gridZ ) ||
				!EditorChunk::outsideChunkWriteable( gridX, gridZ, false ) )
			{
				return false;
			}
		}
	}

	return true;
}


/**
 *  This method creates a chunk link to the chunk corresponding to 'chunkId'.
 *
 *	@param chunkId	Chunk to use as the end point of the chunk link.
 */
void EditorChunkItemLinkable::createChunkLink( const BW::string& chunkId )
{
	BW_GUARD;

	if (chunkItem()->chunk() == NULL ||
		chunkPointLinks_.find( chunkId ) != chunkPointLinks_.end() )
		return;

	GeometryMapping* dirMap = WorldManager::instance().geometryMapping();
	int16 gridX, gridZ;
	const float gridSize = dirMap->pSpace()->gridSize();
	if ( !dirMap->gridFromChunkName( chunkId, gridX, gridZ ) )
		return;
	Vector3 chunkPos = chunkItem()->edTransform().applyToOrigin();
	chunkPos = chunkItem()->chunk()->transform().applyPoint( chunkPos );
	Vector3 nextChunkDir = Vector3(
		( gridX + 0.5f ) * gridSize - chunkPos[0],
		0,
		( gridZ + 0.5f ) * gridSize - chunkPos[2] );
	nextChunkDir.normalise();
	nextChunkDir = nextChunkDir * CHUNK_LINK_SIZE + chunkPos;

	EditorChunkPointLinkPtr link = new EditorChunkPointLink();
	link->startItem( chunkItem() );
	link->endPoint( nextChunkDir, chunkId );

	chunkPointLinks_.insert( std::pair< BW::string, ChunkLinkPtr >( chunkId, link ) );
	chunkItem()->chunk()->addStaticItem(link);
}


/**
 *  This method removes the chunk link going to 'chunkId' owned by this object.
 *
 *	@param chunkId	The end point of the chunk link to be removed.
 */
void EditorChunkItemLinkable::removeChunkLink(const BW::string& chunkId)
{
	BW_GUARD;

	if (chunkPointLinks_.find(chunkId) == chunkPointLinks_.end())
		return;

	ChunkLinkPtr chunkLink = chunkPointLinks_[chunkId];

	if (chunkLink.getObject())
	{
		chunkItem()->chunk()->delStaticItem( chunkLink.getObject() );
		chunkPointLinks_.erase(chunkId);
	}
}


/**
 *  This method removes all chunk links owned by this object.
 */
void EditorChunkItemLinkable::removeChunkLinks()
{
	BW_GUARD;

	for( BW::map< BW::string, ChunkLinkPtr >::iterator it = chunkPointLinks_.begin();
		it != chunkPointLinks_.end(); ++it )
	{
		chunkItem()->chunk()->delStaticItem( (*it).second );
	}
	chunkPointLinks_.clear();
}


/**
 *  This method recreates all the chunk links.
 */
void EditorChunkItemLinkable::updateChunkLinks()
{
	BW_GUARD;

	BW::map< BW::string, ChunkLinkPtr > oldChunkLinks = chunkPointLinks_;
	removeChunkLinks();
	for( BW::map< BW::string, ChunkLinkPtr >::iterator it = oldChunkLinks.begin();
		it != oldChunkLinks.end(); ++it )
	{
		createChunkLink( (*it).first );
	}
}


/**
 *  Loads the back links for this object from the passed data section.
 *
 *  @param pSection	The data section to load the backlinks from.
 */
void EditorChunkItemLinkable::loadBackLinks(DataSectionPtr pSection)
{
	BW_GUARD;

	backLinks_.clear();
	DataSectionPtr backLinksSection = pSection->openSection( "backLinks", true );
	BW::vector<DataSectionPtr>	linkSects;
	backLinksSection->openSections( "link", linkSects );
	for (uint i = 0; i < linkSects.size(); ++i)
	{
		DataSectionPtr ds = linkSects[i];

		UniqueID	uniqueId( ds->readString( "guid" ) );
		BW::string	chunkId = ds->readString( "chunkId" );

		backLinks_.insert(Link(uniqueId, chunkId));
	}
}


/**
 *  Saves the back links for this object to the passed data section.
 *
 *  @param pSection	The data section to save the backlinks to.
 */
void EditorChunkItemLinkable::saveBackLinks(DataSectionPtr pSection)
{
	BW_GUARD;

	DataSectionPtr backLinksSection = pSection->openSection( "backLinks", true );
	backLinksSection->delChildren();

	// Write out the links
	LinksConstIter it = backLinks_.begin();
	for (; it != backLinks_.end(); it++)
	{
		DataSectionPtr ds = backLinksSection->newSection( "link" );
		ds->writeString( "guid", it->UID_.toString() );
		ds->writeString( "chunkId", it->CID_ );
	}		
}


/**
 *  Add the passed linker to this objects list of back links.
 *
 *  @param pEcil	The linker being added to the back links list.
 */
void EditorChunkItemLinkable::addBackLink(EditorChunkItemLinkable* pEcil)
{
	BW_GUARD;

	if (this == pEcil)
	{
		ERROR_MSG( "EditorChunkItemLinkable::addBackLink : Should not be creating a back link to itself (%s).\n",
					this->guid().toString().c_str() );
		return;
	}

	addBackLink(pEcil->guid(), pEcil->getOutsideChunkId());
}


/**
 *  Add the passed linker to this objects list of back links.
 *
 *  @param guid		GUID of the link to remove
 *  @param chunkId	Chunk Id of the link to remove
 */
void EditorChunkItemLinkable::addBackLink(const UniqueID& guid, const BW::string& chunkId)
{
	BW_GUARD;

	if (this->guid() == guid)
	{
		ERROR_MSG( "EditorChunkItemLinkable::addBackLink : Should not be creating a back link to itself (%s).\n",
					guid.toString().c_str() );
		return;
	}
	
	Link link(guid, chunkId);
	backLinks_.insert(link);
}


/**
 *  Remove the passed linker from this objects list of back links.
 *
 *  @param pEcil	The linker being removed from the back links list.
 */
void EditorChunkItemLinkable::removeBackLink(EditorChunkItemLinkable* pEcil)
{
	BW_GUARD;

	removeBackLink(pEcil->guid(), pEcil->getOutsideChunkId());
}


/**
 *  Remove the linker "guid" from this objects list of back links.
 *
 *  @param guid		GUID of the link to remove
 *  @param chunkId	Chunk Id of the link to remove
 */
void EditorChunkItemLinkable::removeBackLink(const UniqueID& guid, const BW::string& chunkId)
{
	BW_GUARD;

	Link link( guid, chunkId );
	LinksIter it = backLinks_.find(link);

	if( it != backLinks_.end() )
	{
		backLinks_.erase( it );
		
		chunkItem()->edSave( chunkItem()->pOwnSect() );
		if ( chunkItem()->chunk() != NULL )
			WorldManager::instance().changedChunk( 
				chunkItem()->chunk(),
				InvalidateFlags::FLAG_NONE );
	}	
}


/**
 *  Removes all back links from the back links list.
 */
void EditorChunkItemLinkable::removeAllBackLinks()
{
	BW_GUARD;

	backLinks_.clear();
}


/**
 *  Returns the begin iterator for the back links list.
 *
 *  @return The begin iterator for the back links list.
 */
EditorChunkItemLinkable::LinksConstIter EditorChunkItemLinkable::getBackLinksBegin()
{
	return backLinks_.begin();
}


/**
 *  Returns the end iterator for the back links list.
 *
 *  @return The end iterator for the back links list.
 */
EditorChunkItemLinkable::LinksConstIter EditorChunkItemLinkable::getBackLinksEnd()
{
	return backLinks_.end();
}


/**
 *  Return the number of elements in the back links list.
 *
 *  @return	The number of elements in the back links list.
 */
size_t EditorChunkItemLinkable::getBackLinksCount() const
{
	return backLinks_.size();
}


/**
 *  This method returns true if the linker has links to the linker identified
 *	by "guid"/"chunkId".
 *
 *	@param guid		GUID of the linker to search in this linker's properties.
 *	@param chunkId	Chunk if the linker to search in this linker's properties.
 *  @return			true if this linker has links to "guid"
 */
bool EditorChunkItemLinkable::hasLinksTo(const UniqueID& guid, const BW::string& chunkId)
{
	BW_GUARD;

	// look for more links to "guid"
	int propCounter = propHelper()->propCount();
	for (int i=0; i < propCounter; i++)
	{
		DataDescription* pDD = propHelper()->pType()->property( i );
		if (!pDD->editable())
			continue;

		// Is this property a user data object link
		if (propHelper()->isUserDataObjectLink( i ))
		{
			PyObjectPtr ob( propHelper()->propGetPy( i ), PyObjectPtr::STEAL_REFERENCE );

			BW::string propId = PyString_AsString( PyTuple_GetItem( ob.getObject(), 0 ) );
			BW::string propChunkId = PyString_AsString( PyTuple_GetItem( ob.getObject(), 1 ) );
			if ( propId == guid.toString() && propChunkId == chunkId )
				return true;
		}
		// Is this property an array of user data object links
		else if (propHelper()->isUserDataObjectLinkArray( i ))
		{
			PyObjectPtr ob(	propHelper()->propGetPy( i ), PyObjectPtr::STEAL_REFERENCE );

			SequenceDataType* dataType =
				static_cast<SequenceDataType*>( pDD->dataType() );
			ArrayPropertiesHelper propArray;
			propArray.init(this->chunkItem(),&(dataType->getElemType()), ob.getObject());

			// Iterate through the array of links
			for(int j = 0; j < propArray.propCount(); j++)
			{
				PyObjectPtr aob( propArray.propGetPy( j ), PyObjectPtr::STEAL_REFERENCE );
				BW::string propId = PyString_AsString( PyTuple_GetItem( aob.getObject(), 0 ) );
				BW::string propChunkId = PyString_AsString( PyTuple_GetItem( aob.getObject(), 1 ) );
				if ( propId == guid.toString() && propChunkId == chunkId )
					return true;
			}
		}
	}
	
	return false;
}


/**
 *  This creates a link from this object to 'other' of the given 'linkType'.
 *
 *  @param linkType         The type of the link.
 *  @param other            The node to link to.
 *
 *  @return                 The link between this and other.
 */
ChunkLinkPtr EditorChunkItemLinkable::createLink(
	ChunkLink::Direction linkType, EditorChunkItemLinkable* other)
{
	BW_GUARD;

	MF_ASSERT( other );

	if (other == this)
	{
		ERROR_MSG( "EditorChunkItemLinkable::createLink : Cannot create link to ourselves (%s).\n",
					this->guid().toString().c_str() );
		return NULL;
	}

	MF_ASSERT( chunkItem()->pOwnSect() );
	MF_ASSERT( other->chunkItem()->pOwnSect() );

    ChunkLinkPtr result = NULL;

	switch (linkType)
	{
	case ChunkLink::DIR_NONE:
		if (isLinkedTo( other->guid() ))
			removeLink( other );
		if (other->isLinkedTo( guid() ))
			other->removeLink( this );
		result = NULL;
		break;
	case ChunkLink::DIR_START_END:
		result = setLink( other, true );
		other->setLink( this, false );
		break;
	case ChunkLink::DIR_END_START:
		result = setLink( other, false );
		other->setLink( this, true );
		break;
	case ChunkLink::DIR_BOTH:
		result = setLink( other, true );
		other->setLink( this, true );
		break;
	}

	// Check for links that are too short
	if (linkType != ChunkLink::DIR_NONE &&
		chunkItem() && chunkItem()->chunk() &&
		other->chunkItem() && other->chunkItem()->chunk() &&
		chunkItem()->isEditorUserDataObject() && other->chunkItem()->isEditorUserDataObject())
	{
		Vector3 startPos = chunkItem()->chunk()->transform().applyPoint(
								chunkItem()->edTransform().applyToOrigin() );
		Vector3 endPos = other->chunkItem()->chunk()->transform().applyPoint(
							other->chunkItem()->edTransform().applyToOrigin() );
		
		static float s_minLinkDistance = Options::getOptionFloat( "render/links/minLinkLength", 0.2f );

		float minLengthSquared = s_minLinkDistance * s_minLinkDistance;
		if ((endPos - startPos).lengthSquared() < minLengthSquared)
		{
			WARNING_MSG( "A link from '%s' to '%s' is too short, which can cause "
							"problems if used for entity navigation.\n",
						this->guid().toString().c_str(), other->guid().toString().c_str() );
		}
	}

    return result;
}


/**
 *  This function completely unlinks the node from all other nodes.
 */
void EditorChunkItemLinkable::unlink()
{
	BW_GUARD;

    while (links_.size() != 0)
    {
        removeLink(links_.front());
    }
}


/**
 *  This function returns whether this node is linked to another.
 *
 *  @param other    The other node.
 *
 *  @return         True if the nodes are linked in any way.
 */
bool EditorChunkItemLinkable::isLinkedTo(UniqueID const &other) const
{
	BW_GUARD;

    EditorChunkItemLinkable *myself = const_cast<EditorChunkItemLinkable *>(this);
    LinkInfo::iterator it = myself->preloadLinks_.find(other);
    return it != preloadLinks_.end();
}


/**
 *  This function unlinks this node from other.
 *
 *  @param other        The node to unlink from.
 */
void EditorChunkItemLinkable::removeLink(EditorChunkItemLinkable* other)
{
	BW_GUARD;

    if (other == NULL)
        return;

    ChunkLinkPtr link = findLink(other);
    if (link != NULL)
        removeLink(link);
}


/**
 *  This functions sets whether we can traverse to the other node.  A link is
 *  created if necessary.  Note that calls to this function should be 
 *  symmetric - you should also call other->setLink(this, blah).
 *
 *  @param other        The other node to link to.
 *  @param canTraverse  Can we go from this node to other?
 *
 *  @return             The link from this to other.
 */
ChunkLinkPtr EditorChunkItemLinkable::setLink(EditorChunkItemLinkable *other, bool canTraverse)
{
	BW_GUARD;

    if (other == NULL)
        return ChunkLinkPtr();
    
    preloadLinks_[other->guid_] = canTraverse;

    // If there isn't a link, create one:
    bool owns = isLinkOwner( this, other );
    ChunkLinkPtr link = findLink(other);

    if (link == NULL)
    {
        link = createLink();
        if (owns)
        {
			link->startItem(chunkItem());
            link->endItem(other->chunkItem());
        }
        else
        {
            link->startItem(other->chunkItem());
            link->endItem(chunkItem());
        }
        links_.push_back(link);
        other->links_.push_back(link);
        if (owns)
        {
            if (chunkItem()->chunk() != NULL)
				chunkItem()->chunk()->addStaticItem(link);           
        }
        else
        {
            if (other->chunkItem()->chunk() != NULL)
                other->chunkItem()->chunk()->addStaticItem(link);
        }
    }
   
    ChunkLink::Direction dir = link->direction();
    if (canTraverse)
    {
        if (owns)
        {
            link->direction
            (
                (ChunkLink::Direction)(dir | ChunkLink::DIR_START_END)
            );
        }
        else
        {
            link->direction
            (
                (ChunkLink::Direction)(dir | ChunkLink::DIR_END_START)
            );
        }
    }
    else
    {
        if (owns)
        {
            link->direction
            (
                (ChunkLink::Direction)
                (ChunkLink::DIR_BOTH & (dir & ~ChunkLink::DIR_START_END))
            );
        }
        else
        {
            link->direction
            (
                (ChunkLink::Direction)
                (ChunkLink::DIR_BOTH & (dir & ~ChunkLink::DIR_END_START))
            );
        }
    }

    return link;
}


/**
 *  This removes a link from our internal data structure.
 *
 *  @param link     The link to remove.
 */
void EditorChunkItemLinkable::delLink(ChunkLinkPtr link)
{
	BW_GUARD;

    for 
    (
        BW::vector<ChunkLinkPtr>::iterator it = links_.begin(); 
        it != links_.end();
        ++it
    )
    {
        if (*it == link)
        {
            links_.erase(it);
            break;
        }
    }
}


/**
 *  This function finds a link to other.
 *
 *  @param other        The node to find the link for.
 * 
 *  @return             The link between the nodes.  This will be NULL if there
 *                      is no link.
 */
ChunkLinkPtr EditorChunkItemLinkable::findLink(EditorChunkItemLinkable const *other) const
{
	BW_GUARD;

    for
    (
        BW::vector<ChunkLinkPtr>::const_iterator it = links_.begin();
        it != links_.end();
        ++it
    )
    {
        ChunkLinkPtr link = *it;
        if 
        (
            link->startItem() == other->chunkItem()
            ||
            link->endItem() == other->chunkItem()
        )
        {
            return link;
        }
    }
    return NULL;
}


/**
 *  This removes a link, by setting its start and end to NULL, removing from 
 *  out list of links etc.
 *
 *  @param link         The link to remove.
 */
void EditorChunkItemLinkable::removeLink(ChunkLinkPtr link)
{
	BW_GUARD;

    delLink(link);

	ChunkItem* otherCI =
		static_cast<ChunkItem*>(link->endItem().getObject());
    ChunkItem* myselfCI =
		static_cast<ChunkItem*>(link->startItem().getObject());

	EditorChunkItemLinkable* otherLinker = 0;
	EditorChunkItemLinkable* myselfLinker = 0;

	if (otherCI->isEditorEntity())
		otherLinker = static_cast<EditorChunkEntity*>(otherCI)->chunkItemLinker();
	else if (otherCI->isEditorUserDataObject())
		otherLinker = static_cast<EditorChunkUserDataObject*>(otherCI)->chunkItemLinker();
	
	if (myselfCI->isEditorEntity())
		myselfLinker = static_cast<EditorChunkEntity*>(myselfCI)->chunkItemLinker();
	else if (myselfCI->isEditorUserDataObject())
		myselfLinker = static_cast<EditorChunkUserDataObject*>(myselfCI)->chunkItemLinker();

	if (otherLinker == NULL && myselfLinker == NULL)
        return;
	if (otherLinker->chunkItem() == chunkItem())
	{
		if (myselfCI->isEditorEntity())
			otherLinker = static_cast<EditorChunkEntity*>(myselfCI)->chunkItemLinker();
		else if (myselfCI->isEditorUserDataObject())
			otherLinker = static_cast<EditorChunkUserDataObject*>(myselfCI)->chunkItemLinker();
	}
    otherLinker->delLink(link);
    bool owns = isLinkOwner( this, otherLinker );
    
	LinkInfo::iterator lit = preloadLinks_.find(otherLinker->guid());
    if (lit != preloadLinks_.end())
        preloadLinks_.erase(lit);

    lit = otherLinker->preloadLinks_.find(guid());
	if (lit != otherLinker->preloadLinks_.end())
        otherLinker->preloadLinks_.erase(lit);

	if (owns)
		chunkItem()->chunk()->delStaticItem(link);
    else
		otherLinker->chunkItem()->chunk()->delStaticItem(link);

	link->startItem(NULL);
    link->endItem(NULL);
}


/**
 *  This function is used to create a link.  Derived classes can override this
 *  to provide a more useful link type.
 *
 *  @return         A new link.
 */
/*virtual*/ ChunkLinkPtr EditorChunkItemLinkable::createLink() const
{
	BW_GUARD;

	return new EditorChunkUserDataObjectLink();
}
BW_END_NAMESPACE

