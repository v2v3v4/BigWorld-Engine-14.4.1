#include "pch.hpp"
#include "worldeditor/world/entity_link_proxy.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "worldeditor/world/editor_chunk_cache.hpp"
#include "worldeditor/editor/station_node_link_locator.hpp"
#include "worldeditor/undo_redo/station_entity_link_operation.hpp"
#include "chunk/chunk_space.hpp"
#include "resmgr/string_provider.hpp"

BW_BEGIN_NAMESPACE

/**
 *  EntityLinkProxy constructor.
 *
 *  @param entity           The entity being linked.
 */
/*explicit*/ EntityLinkProxy::EntityLinkProxy
(
    EditorChunkEntity       *entity
)
:
entity_(entity)
{
}


/**
 *  EntityLinkProxy destructor.
 */
/*virtual*/ EntityLinkProxy::~EntityLinkProxy()
{
}


/**
 *  With entities we only support linking, not the creation of new links.
 *
 *  @returns                LT_LINK
 */
/*virtual*/ LinkProxy::LinkType EntityLinkProxy::linkType() const
{
    return LT_LINK;
}


/**
 *  Usually this is called to create a new node to link the current one 
 *  against.  Since we only support LT_LINK this function does nothing.
 */
/*virtual*/ MatrixProxyPtr EntityLinkProxy::createCopyForLink()
{
    return NULL; // not supported
}


/**
 *  This function is used to determine whether the given locator's position
 *  can link to something.
 *
 *  @param locator      The locator to test.
 *	@return				TS_CAN_LINK if it can be linked, TS_CANT_LINK if it
 *						cannot be linked, TS_NO_TARGET if there is no valid
 *						object to link to under the locator.
 */
/*virtual*/ LinkProxy::TargetState EntityLinkProxy::canLinkAtPos(ToolLocatorPtr locator_) const
{
	BW_GUARD;

    StationNodeLinkLocator *locator =
        (StationNodeLinkLocator *)locator_.getObject();
	if ( locator->chunkItem() != NULL && entity_->edIsEditable() )
		return TS_NO_TARGET;

	if (!locator->chunkItem()->edIsEditable())
		return TS_CANT_LINK;

	return TS_CAN_LINK;
}


/**
 *  This links the entity to the item at the locator's position.  If the 
 *  entity is in the selection then all selected items are linked.
 *
 *  @param locator          The locator that has an item to link to.
 */
/*virtual*/ void EntityLinkProxy::createLinkAtPos(ToolLocatorPtr locator_)
{
	BW_GUARD;

    StationNodeLinkLocator *locator =
        (StationNodeLinkLocator *)locator_.getObject();
    ChunkItemPtr chunkItem = locator->chunkItem();
    if (chunkItem == NULL)
        return;

    // The linked node:
    EditorChunkItem *item = (EditorChunkItem *)chunkItem.getObject();
    if (!item->isEditorChunkStationNode())
        return;
    EditorChunkStationNode *node = (EditorChunkStationNode *)item;

    // The list of entities to link (populated below):
    BW::vector<ChunkItemPtr> entities;

    // See if the entity is in the selection, if it is then link all of the 
    // selection.
    const BW::vector<ChunkItemPtr> &selection =
        WorldManager::instance().selectedItems();
    for (size_t i = 0; i < selection.size(); ++i)
    {
        if (selection[i].getObject() == entity_.getObject())
        {
            entities = selection;
            break;
        }
    }

    // If the item is not in the selection then we only link it:
    if (entities.size() == 0)
        entities.push_back(entity_);

    size_t numLinked = 0;
    for (size_t i = 0; i < entities.size(); ++i)
    {
        // Only deal with writeable entities:
        if (!entities[i]->isEditorEntity())
            continue;
        EditorChunkEntity *entity = 
                (EditorChunkEntity *)entities[i].getObject();
        if( !entity->edIsEditable() )
	        continue;

        // Create an undo operation for this linking operation:
        UndoRedo::instance().add
        ( 
            new StationEntityLinkOperation(entity)
        );        

        // Do the actual linking:
        BW::string graphName = node->graph()->name();
		if ( entity->patrolListRelink( graphName, node->id() ) )
		{
            ++numLinked;
		}
    }
    if (numLinked == 1)
        UndoRedo::instance().barrier(
			LocaliseUTF8( L"WORLDEDITOR/WORLDEDITOR/PROPERTIES/CHUNK_LINK_PROXY/LINK_ENTITY_TO_GRAPH"), false);
    else
        UndoRedo::instance().barrier(
			LocaliseUTF8( L"WORLDEDITOR/WORLDEDITOR/PROPERTIES/CHUNK_LINK_PROXY/LINK_ENTITIES_TO_GRAPH"), false);
}


/**
 *  This funciton is used to create a tool locator appropriate to this linker.
 *  In this case we create a StationNodeLinkLocator and set it to only
 *  locator nodes (not entities).
 *
 *  @returns                The tool locator to use.
 */
/*virtual*/ ToolLocatorPtr EntityLinkProxy::createLocator() const
{
	BW_GUARD;

    return 
        ToolLocatorPtr
        (
            new StationNodeLinkLocator(StationNodeLinkLocator::LOCATE_NODES), 
            true
        );
}
BW_END_NAMESPACE

