#include "pch.hpp"
#include "worldeditor/world/user_data_object_link_proxy.hpp"
#include "worldeditor/world/editor_chunk_item_linker.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "worldeditor/world/items/editor_chunk_entity.hpp"
#include "worldeditor/world/items/editor_chunk_user_data_object.hpp"
#include "worldeditor/world/editor_chunk_item_linker_manager.hpp"
#include "worldeditor/world/editor_chunk_cache.hpp"
#include "worldeditor/editor/user_data_object_link_locator.hpp"
#include "worldeditor/editor/chunk_item_placer.hpp"
#include "worldeditor/editor/item_properties.hpp"
#include "common/editor_views.hpp"
#include "chunk/user_data_object_link_data_type.hpp"
#include "resmgr/xml_section.hpp"

BW_BEGIN_NAMESPACE

/**
 *  UserDataObjectLinkProxy constructor.
 *
 *  @param linkName	The id linked.
 *  @param linker	The linker object being linked.
 */
/*explicit*/ UserDataObjectLinkProxy::UserDataObjectLinkProxy(
		const BW::string&		linkName,
		EditorChunkItemLinkable*	linker ) :
	linkName_(linkName),
	linker_(linker)
{
	BW_GUARD;

	PropertyIndex index =
		linker_->propHelper()->propGetIdx( linkName_ );
	PyObjectPtr ob( linker_->propHelper()->propGetPy( index ), PyObjectPtr::STEAL_REFERENCE );

	bw_utf8tow( UserDataObjectLinkDataType::asString( ob.getObject() ), linkValue_ );
}


/**
 *  UserDataObjectLinkProxy destructor.
 */
/*virtual*/ UserDataObjectLinkProxy::~UserDataObjectLinkProxy()
{
}


/**
 *  With entities we only support linking, not the creation of new links.
 *
 *  @returns                LT_LINK
 */
/*virtual*/ LinkProxy::LinkType UserDataObjectLinkProxy::linkType() const
{
	BW_GUARD;

	if ( linker_->chunkItem()->isEditorUserDataObject() )
	{
		EditorChunkUserDataObject* udo =
			static_cast<EditorChunkUserDataObject*>( linker_->chunkItem() );

		if ( udo->showAddGizmo( linkName_ ) )
			return (LinkType)( LT_ADD | LT_LINK );
		else
			return LT_LINK;
	}

	// Linker must be an entity.
	return LT_LINK;
}


/**
 *  Create a copy of the EditorChunkUserDataObject that the UserDataObjectLinkProxy 
 *  is working on, link this copy to the original and return a MatrixProxyPtr 
 *  that can set the position/orientation etc of the copy.
 *
 *  @return		A proxy to set the position/orientation of the linked
 *              item.
 */
/*virtual*/ MatrixProxyPtr UserDataObjectLinkProxy::createCopyForLink()
{
	BW_GUARD;

	if ( linker_->chunkItem()->isEditorEntity() )
	{
		// Not supported in entities at the moment.
		return NULL;
	}

    // Create a copy of node_:
    EditorChunkUserDataObject *newNode = new EditorChunkUserDataObject();
    DataSectionPtr newSection = new XMLSection("copy");
    newSection->copy( linker_->chunkItem()->pOwnSect() );
	newSection->delChild( "guid" );
	newSection->delChild( "backLinks" );

	// delete the link property, or the whole array if it's an array of links
	DataSectionPtr propsSection = newSection->openSection( "properties" );
	BW::string baseLinkName = linkName_;
	BW::string::size_type pos = baseLinkName.find_first_of( "[" );
	if ( pos != BW::string::npos )
		baseLinkName = baseLinkName.substr( 0, pos );
	if ( propsSection != NULL )
		propsSection->delChild( baseLinkName );

	// and load it.
    newNode->load( newSection, linker_->chunkItem()->chunk() );
    linker_->chunkItem()->chunk()->addStaticItem( newNode );
    newNode->edTransform( linker_->chunkItem()->edTransform(), false );

	// set the link in the current node to point to the newNode
	PropertyIndex propIdx =
		linker_->propHelper()->propGetIdx(linkName_);

	WorldManager::instance().linkerManager().addLink(
		linker_, newNode->chunkItemLinker(), propIdx );
	
	PyObjectPtr ob( linker_->propHelper()->propGetPy( propIdx ), PyObjectPtr::STEAL_REFERENCE );
	bw_utf8tow( UserDataObjectLinkDataType::asString( ob.getObject() ), linkValue_ );

    // Set the new node as the selection:
	int curSel = PropTable::table()->propertyList()->GetCurSel();
    BW::vector<ChunkItemPtr> items;
    items.push_back( newNode );
    WorldManager::instance().setSelection( items );
	PropTable::table()->propertyList()->selectItem( curSel, false );

	newNode->propHelper()->resetSelUpdate( true );

    // Return a ChunkItemMatrix for the new node so that its position can be
    // edited:
    ChunkItemMatrix *result = new ChunkItemMatrix( newNode );
    result->recordState();
    return result;
}


/**
 *  This function is used to determine whether the given locator's position
 *  can link to something.
 *
 *  @param pLocator     The locator to test.
 *	@return				TS_CAN_LINK if it can be linked, TS_CANT_LINK if it
 *						cannot be linked, TS_NO_TARGET if there is no valid
 *						object to link to under the locator.
 */
/*virtual*/ LinkProxy::TargetState UserDataObjectLinkProxy::canLinkAtPos(ToolLocatorPtr pLocator) const
{
	BW_GUARD;

	UserDataObjectLinkLocator *locator =
		static_cast<UserDataObjectLinkLocator*>( pLocator.getObject() );

    if (locator->chunkItem() == NULL || !linker_->chunkItem()->edIsEditable())
		return TS_NO_TARGET;

	if ( locator->chunkItem().getObject() == linker_->chunkItem() )
		return TS_NO_TARGET;	// avoid linking to ourselves

	if (!locator->chunkItem()->edIsEditable())
		return TS_CANT_LINK;

	EditorChunkItem *item = static_cast<EditorChunkItem*>( locator->chunkItem().getObject() );
	if ( item == NULL || !item->isEditorUserDataObject() )
		return TS_NO_TARGET;

	EditorChunkUserDataObject *ecudo = static_cast<EditorChunkUserDataObject*>( item );
	if ( linker_->chunkItem()->isEditorEntity() )
	{
		EditorChunkEntity* entity =
			static_cast<EditorChunkEntity*>( linker_->chunkItem() );

		// Get info for the target
		PyObjectPtr ecudoInfo( ecudo->infoDict(), PyObjectPtr::STEAL_REFERENCE );

		if ( !entity->canLinkTo( linkName_, ecudoInfo.getObject() ) )
			return TS_CANT_LINK;
	}
	else if ( linker_->chunkItem()->isEditorUserDataObject() )
	{
		EditorChunkUserDataObject* udo =
			static_cast<EditorChunkUserDataObject*>( linker_->chunkItem() );

		if ( !udo->canLinkTo( linkName_, ecudo ) )
			return TS_CANT_LINK;
	}

	return TS_CAN_LINK;
}


/**
 *  This links the udo to the udo at the locator's position.
 *
 *  @param pLocator          The locator that has an item to link to.
 */
/*virtual*/ void UserDataObjectLinkProxy::createLinkAtPos(ToolLocatorPtr pLocator)
{
	BW_GUARD;

    UserDataObjectLinkLocator* locator =
        (UserDataObjectLinkLocator*)pLocator.getObject();
    ChunkItemPtr chunkItem = locator->chunkItem();
    if (chunkItem == NULL)
        return;

    // The linked udo
    EditorChunkItem* item = static_cast<EditorChunkItem*>( chunkItem.getObject() );
    if (!item->isEditorUserDataObject())
        return;
    EditorChunkUserDataObject* ecudo = static_cast<EditorChunkUserDataObject*>( item );

    if (!ecudo->edIsEditable())
		return;

	PropertyIndex propIdx =
		linker_->propHelper()->propGetIdx(linkName_);

	if ( propIdx.empty() )
		//TODO: have a warning or error especially if its not a selection
		return;

	DataSectionPtr prevLinkInfo = linker_->propHelper()->propGet( propIdx );
	BW::string prevGuid = prevLinkInfo->readString( "guid" );
	BW::string prevChunkId = prevLinkInfo->readString( "chunkId" );
	bool validLink = !prevGuid.empty() && !prevChunkId.empty();

	if (validLink)
	{
		EditorChunkItemLinkable* prevLinkedItem =
			WorldManager::instance().linkerManager().forceLoad( prevGuid, prevChunkId );
		if (prevLinkedItem && prevLinkedItem->chunkItem() &&
			!prevLinkedItem->chunkItem()->edIsEditable())
		{
			// Previous linked end is frozen or its chunk is not locked for editing.
			ERROR_MSG( "The previously linked object '%s' is not editable.\n", prevGuid.c_str() );
			return;
		}
	}

	// Add a link from the linker to ecudo in property propIdx
	WorldManager::instance().linkerManager().addLink(
		linker_, ecudo->chunkItemLinker(), propIdx );
		
	// Update the property representation
	PyObjectPtr ob( linker_->propHelper()->propGetPy( propIdx ), PyObjectPtr::STEAL_REFERENCE );
	bw_utf8tow( UserDataObjectLinkDataType::asString( ob.getObject() ), linkValue_ );

	UndoRedo::instance().barrier(
		LocaliseUTF8( "WORLDEDITOR/WORLDEDITOR/PROPERTIES/STATION_NODE_LINK_PROXY/LINK_NODES" ), false);
}


/**
 *  This funciton is used to create a tool locator appropriate to this linker.
 *  In this case we create a UserDataObjectLinkLocator and set it to only
 *  locator nodes (not entities).
 *
 *  @returns                The tool locator to use.
 */
/*virtual*/ ToolLocatorPtr UserDataObjectLinkProxy::createLocator() const
{
	BW_GUARD;

	BW::string nlinkValue;
	bw_wtoutf8( linkValue_, nlinkValue );
    return 
        ToolLocatorPtr
        (
            new  UserDataObjectLinkLocator(nlinkValue, UserDataObjectLinkLocator::LOCATE_USER_DATA_OBJECTS), 
            true
        );
}
BW_END_NAMESPACE

