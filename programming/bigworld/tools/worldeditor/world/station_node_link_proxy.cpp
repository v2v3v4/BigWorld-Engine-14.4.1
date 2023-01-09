#include "pch.hpp"
#include "worldeditor/world/station_node_link_proxy.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "worldeditor/world/editor_chunk_cache.hpp"
#include "worldeditor/world/items/editor_chunk_station.hpp"
#include "worldeditor/world/items/editor_chunk_entity.hpp"
#include "worldeditor/undo_redo/station_link_operation.hpp"
#include "worldeditor/undo_redo/merge_graphs_operation.hpp"
#include "worldeditor/undo_redo/station_entity_link_operation.hpp"
#include "worldeditor/editor/station_node_link_locator.hpp"
#include "worldeditor/editor/item_properties.hpp"
#include "worldeditor/editor/chunk_item_placer.hpp"
#include "resmgr/xml_section.hpp"
#include "chunk/chunk_space.hpp"
#include "chunk/chunk_manager.hpp"
#include "chunk/geometry_mapping.hpp"

#include <limits>

BW_BEGIN_NAMESPACE

/**
 *  StationNodeLinkProxy constructor.
 *
 *  @param node     The node to link things to.
 */
/*explicit*/ StationNodeLinkProxy::StationNodeLinkProxy
(
    EditorChunkStationNode      &node
) :
    node_(&node)
{
}


/**
 *  StationNodeLinkProxy destructor.
 */
/*virtual*/ StationNodeLinkProxy::~StationNodeLinkProxy()
{
    node_ = NULL;
}


/**
 *  What type of linking is supported?
 *
 *  @returns    LT_ADD | LT_LINK
 */
/*virtual*/ LinkProxy::LinkType StationNodeLinkProxy::linkType() const
{
    return (LinkType)(LT_ADD | LT_LINK);
}


/**
 *  Create a copy of the EditorChunkStationNode that the StationNodeLinkProxy 
 *  is working on, link this copy to the original and return a MatrixProxyPtr 
 *  that can set the position/orientation etc of the copy.
 *
 *  @returns        A proxy to set the position/orientation of the linked
 *                  item.
 */
/*virtual*/ MatrixProxyPtr StationNodeLinkProxy::createCopyForLink()
{
	BW_GUARD;

    // Create a copy of node_:
    EditorChunkStationNode *newNode = new EditorChunkStationNode();
    XMLSectionPtr section = new XMLSection("copy");
    section->copy(node_->pOwnSect());
    newNode->load(section, node_->chunk());
    node_->chunk()->addStaticItem(newNode);
    newNode->edTransform(node_->edTransform(), false);

    // Create an undo operation for the addition:      
    UndoRedo::instance().add(new ChunkItemExistenceOperation(newNode, NULL));

    // Create a link from node_ to the new node:    
    UndoRedo::instance().add
    ( 
        new StationLinkOperation
        (
            newNode, 
            node_, 
            ChunkLink::DIR_NONE
        )
    );
    newNode->createLink(ChunkLink::DIR_END_START, node_);

    // Set the new node as the selection:
    BW::vector<ChunkItemPtr> items;
    items.push_back(newNode);
    WorldManager::instance().setSelection(items);

    // Return a ChunkItemMatrix for the new node so that its position can be
    // edited:
    ChunkItemMatrix *result = new ChunkItemMatrix(newNode);
    result->recordState();
    return result;
}


/**
 *  See if there are any linkable nodes epsilon within pos.
 *
 *  @param locator  The locator used to find objects.
 *	@return			TS_CAN_LINK if it can be linked, TS_CANT_LINK if it
 *					cannot be linked, TS_NO_TARGET if there is no valid
 *					object to link to under the locator.
 */
/*virtual*/ LinkProxy::TargetState 
StationNodeLinkProxy::canLinkAtPos(ToolLocatorPtr locator_) const
{
	BW_GUARD;

    StationNodeLinkLocator *locator =
        (StationNodeLinkLocator *)locator_.getObject();
    bool canLink =
        locator->chunkItem() != NULL 
        && 
        locator->chunkItem().getObject() != node_;
	if( canLink )
	{
		ChunkItemPtr item = locator->chunkItem();
		if (item->isEditorEntity() || item->isEditorChunkStationNode())
		{
			canLink = item->edIsEditable();
		}
	}
	return canLink ? TS_CAN_LINK : TS_NO_TARGET;
}


/**
 *  This private method relinks entities currently linked to 'oldGraph' to
 *  another station graph 'newGraph'. It searches for entities in all
 *  chunks that contain a node, or that are neighbours to a chunk that
 *  contains a node.
 *  TODO: figure out a better way to do it, since these method is
 *  inefficient and might fail if the entity is in chunk not yet loaded
 *
 *  @param oldGraph     Graph to search for entities linked to it
 *  @param newGraph     Graph to which entities will be relinked
 */
void StationNodeLinkProxy::relinkEntities( StationGraph* oldGraph, StationGraph* newGraph )
{
	BW_GUARD;

	GeometryMapping *dirMap = WorldManager::instance().geometryMapping();
	const float gridSize = dirMap->pSpace()->gridSize();

	typedef std::pair<ChunkStationNode*,BW::string> NodePair;
	BW::vector<NodePair> chunks;

	BW::vector<ChunkStationNode*> nodes = oldGraph->getAllNodes();
	for ( BW::vector<ChunkStationNode*>::iterator it = nodes.begin();
        it != nodes.end(); ++it )
    {
        ChunkStationNode* node = *it;
		Vector3 pos = node->position();
		pos = node->chunk()->transform().applyPoint( pos );
        Vector3 localPos( pos.x, 0.0f, pos.z );

		// Radius used to search for entities linked to a node in the old
		// graph. 1 means up to 9 chunks get searched per node, 2 means
		// up to 25 nodes get checked per node, etc. Note that chunks
		// are not searched redundantly, so if a graph contains many nodes,
		// but all nodes are in one chunk, only (RELINK_DEPTH*2 + 1)^2
		// chunks get searched.
		const int RELINK_DEPTH = 2;

		// relink neighbour chunks
		for( int z = -RELINK_DEPTH; z <= RELINK_DEPTH; z++ )
		{
			for( int x = -RELINK_DEPTH; x <= RELINK_DEPTH; x++ )
			{
				// find the actual neighbour chunk and relink it
				Vector3 neighbourPos = localPos;
				neighbourPos.x += gridSize*x;
				neighbourPos.z += gridSize*z;
				NodePair chunk = NodePair( node, dirMap->outsideChunkIdentifier( neighbourPos ) );

				// add it if not already in
				if ( !chunk.second.empty() &&
					std::find( chunks.begin(), chunks.end(), chunk ) == chunks.end() )
				{
					// make sure the neighbour is loaded
					ChunkManager::instance().loadChunkNow( chunk.second, dirMap );

					chunks.push_back( chunk );
				}
			}
		}
    }
	for( BW::vector<NodePair>::iterator i = chunks.begin();
		i != chunks.end(); ++i )
	{
		relinkEntitiesInChunk( (*i).first, newGraph, (*i).second );
	}
}


/**
 *  This private method relinks entities currently linked to 'node' to
 *  another station graph 'newGraph'. It searches for entities in the 
 *	chunk identified bu 'chunkID'.
 *  TODO: figure out a better way to do it, since these method is
 *  inefficient and might fail if the entity is in chunk not yet loaded
 *
 *  @param node         Node the entity is linked to
 *  @param graph        Graph to which entities will be relinked
 *  @param chunkID      ID of the chunk to search for entities
 */
void StationNodeLinkProxy::relinkEntitiesInChunk( ChunkStationNode* node,
												StationGraph* graph,
												BW::string& chunkID )
{
	BW_GUARD;

	GeometryMapping *dirMap = WorldManager::instance().geometryMapping();

	Chunk* chunk = ChunkManager::instance().findChunkByName( chunkID, dirMap );
	if ( !chunk ) return;

	BW::vector<ChunkItemPtr> items;

	EditorChunkCache::instance( *chunk ).allItems( items );
	for( BW::vector<ChunkItemPtr>::iterator i = items.begin();
		i != items.end(); ++i )
	{
		if ( (*i)->isEditorEntity() )
		{
			// the item is an entity, so check it and relink if necesary
			EditorChunkEntity* ent = static_cast<EditorChunkEntity*>((*i).getObject());
			BW::string entNode = ent->patrolListNode();
			if ( entNode == BW::string( node->id() ) &&
				ent->patrolListGraphId() != BW::string( graph->name() ) )
			{
				// Create an undo operation for this linking operation:
				UndoRedo::instance().add( new StationEntityLinkOperation( ent ) );

				// Do the actual linking:
				ent->patrolListRelink( graph->name(), entNode );
			}
		}
	}
}


/**
 *  Create a link from the EditorChunkStationNode to the closest linkable 
 *  object at pos.
 *
 *  @param locator  The locator used to find objects.
 */
/*virtual*/ void StationNodeLinkProxy::createLinkAtPos(ToolLocatorPtr locator_)
{
	BW_GUARD;

    StationNodeLinkLocator *locator =
        (StationNodeLinkLocator *)locator_.getObject();
    ChunkItemPtr chunkItem = locator->chunkItem();
    if (chunkItem == NULL)
        return;

    EditorChunkItem *item = (EditorChunkItem *)chunkItem.getObject();

	if( item->isEditorEntity() )
	{
		if (!item->edIsEditable())
			return;
	}
	else if( item->isEditorChunkStationNode() )
	{
		ChunkSpacePtr pSpace = item->chunk()->space();
		EditorChunkStationNode* other = (EditorChunkStationNode*)item;
		Vector3 itemPosition = other->chunk()->transform().applyPoint( other->position() );
		Vector3 nodePosition = node_->chunk()->transform().applyPoint( node_->position() );
		int cx = pSpace->pointToGrid( itemPosition.x );
		int cz = pSpace->pointToGrid( itemPosition.z );
		int nx = pSpace->pointToGrid( nodePosition.x );
		int nz = pSpace->pointToGrid( nodePosition.z );

		if( !other->edIsEditable() )
			return;
		if( !node_->edIsEditable() )
			return;
		// check if they belong to the same lock area for safe
		WorldManager::instance().connection().linkPoint( (int16)cx, (int16)cz, (int16)nx, (int16)nz );
	}

    if (item->isEditorEntity())
    {

        EditorChunkEntity *entity = (EditorChunkEntity *)item;

        // Create an undo operation for this linking operation:
        UndoRedo::instance().add
        ( 
            new StationEntityLinkOperation(entity)
        );
        UndoRedo::instance().barrier(
			LocaliseUTF8( L"WORLDEDITOR/WORLDEDITOR/PROPERTIES/STATION_NODE_LINK_PROXY/LINK_ENTITY_TO_GRAPH" ),
			false);

		entity->patrolListRelink( node_->graph()->name(), node_->id() );
    }
    else if (item->isEditorChunkStationNode())
    {
        // The linked node:
        EditorChunkStationNode *other = (EditorChunkStationNode *)item;

        // Are we merging two graphs?
        if (node_->graph() != other->graph())
        {
            UndoRedo::instance().add
            (
                new MergeGraphsOperation
                (
                    node_->graph()->name(),
                    other->graph()->name()
                )
            );
            StationGraph::mergeGraphs
            (
                node_->graph()->name(),
                other->graph()->name(),
                WorldManager::instance().geometryMapping()
            );
			relinkEntities( node_->graph(), other->graph() );
        }

        // The link direction:
        ChunkLink::Direction dir = ChunkLink::DIR_END_START;
        if (node_->getLinkDirection(other) == ChunkLink::DIR_END_START)
            dir = ChunkLink::DIR_BOTH;

        // Create an undo operation for this linking operation:
        UndoRedo::instance().add
        ( 
            new StationLinkOperation
            (
                node_, 
                other, 
                node_->getLinkDirection(other)
            )
        );        

        UndoRedo::instance().barrier(
			LocaliseUTF8( L"WORLDEDITOR/WORLDEDITOR/PROPERTIES/STATION_NODE_LINK_PROXY/LINK_NODES" ), false);

        ChunkLinkPtr link = other->createLink(dir, node_);
        link->makeDirty();
    }
}


/*virtual*/ ToolLocatorPtr
StationNodeLinkProxy::createLocator() const
{
	BW_GUARD;

    return 
        ToolLocatorPtr
        (
            new StationNodeLinkLocator(StationNodeLinkLocator::LOCATE_BOTH), 
            true
        );
}
BW_END_NAMESPACE

