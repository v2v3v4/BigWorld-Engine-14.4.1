#include "pch.hpp"
#include "worldeditor/world/items/editor_chunk_station.hpp"
#include "worldeditor/world/items/editor_chunk_substance.ipp"
#include "worldeditor/world/items/editor_chunk_link.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "worldeditor/world/editor_chunk_cache.hpp"
#include "worldeditor/world/station_node_link_proxy.hpp"
#include "worldeditor/undo_redo/station_link_operation.hpp"
#include "worldeditor/editor/chunk_item_placer.hpp"
#include "worldeditor/editor/item_editor.hpp"
#include "worldeditor/editor/item_properties.hpp"
#include "worldeditor/editor/link_property.hpp"
#include "worldeditor/misc/options_helper.hpp"
#include "appmgr/options.hpp"
#include "gizmo/item_view.hpp"
#include "chunk/chunk_model.hpp"
#include "chunk/chunk_manager.hpp"
#include "chunk/chunk_space.hpp"
#include "chunk/geometry_mapping.hpp"
#include "chunk/umbra_chunk_item.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/xml_section.hpp"
#include "resmgr/string_provider.hpp"
#include "resmgr/resource_cache.hpp"
#include "model/super_model.hpp"
#include "moo/geometrics.hpp"
#include "cstdmf/debug.hpp"

DECLARE_DEBUG_COMPONENT2( "Editor", 0 )

BW_BEGIN_NAMESPACE

namespace
{
    /**
     *  This class saves/restores the deleted state of a node.
     */
    class ECSSaveDeleteState : public UndoRedo::Operation
    {
    public:
        explicit ECSSaveDeleteState(EditorChunkStationNodePtr station) :
            UndoRedo::Operation(size_t(typeid(ECSSaveDeleteState).name())),
            station_(station),
            deleted_(station->deleted())
        {
        }

        void undo()
        {
			BW_GUARD;

            UndoRedo::instance().add
            (
                new ECSSaveDeleteState(station_)
            );

            station_->deleted(deleted_);
        }

        bool iseq(UndoRedo::Operation const &other) const
        {
            return false;
        }

    protected:
        EditorChunkStationNodePtr   station_;
        bool                        deleted_;        
    };


    /**
     *  This class registers/unregisters a node with its graph in undo/redo
     *  operations.
     */
    class ECSSaveRegisterState : public UndoRedo::Operation
    {
    public:
        ECSSaveRegisterState
        (
            EditorChunkStationNodePtr   station, 
            bool                        registered
        ) :
            UndoRedo::Operation(size_t(typeid(ECSSaveDeleteState).name())),
            station_(station),
            register_(registered)
        {
        }

        void undo()
        {
			BW_GUARD;

            UndoRedo::instance().add
            (
                new ECSSaveRegisterState(station_, !register_)
            );

            if (station_->graph() != NULL)
            {
	            if (register_)
	            {
	                station_->graph()->registerNode
	                (
	                    station_.getObject(), 
	                    station_->chunk()
	                );
	            }
	            else
	            {
	                station_->graph()->deregisterNode(station_.getObject());
	            }
        	}
        }

        bool iseq(UndoRedo::Operation const &other) const
        {
            return false;
        }

    protected:
        EditorChunkStationNodePtr   station_;
        bool                        register_; 
    };


    size_t findNode
    (
        UniqueID                                const &id,
        BW::vector<EditorChunkStationNodePtr>  const &list
    )
    {
		BW_GUARD;

        for (size_t i = 0; i < list.size(); ++i)
        {
            if (id == list[i]->id())
                return i;
        }
        return static_cast<size_t>(-1);
    }

}


/*static*/ bool EditorChunkStationNode::s_enableDraw_ = true;


/**
 *  EditorChunkStationNode constructor.
 */
EditorChunkStationNode::EditorChunkStationNode() :
	EditorChunkSubstance<ChunkStationNode>(),
	transform_( Matrix::identity ),
    deleted_( false )
{
	BW_GUARD;

	this->wantFlags_ = WantFlags( this->wantFlags_ | WANTS_DRAW );

	model_ = Model::get( "resources/models/station.model" );
	ResourceCache::instance().addResource( model_ );
}


/**
 *  EditorChunkStationNode destructor.
 */
EditorChunkStationNode::~EditorChunkStationNode()
{
}


/**
 *  This loads a EditorChunkStationNode.
 *
 *  @param pSection     The section to load the node from.
 *  @param pChunk       The chunk to load the node from.
 *  @param errorString  Returns error message here if not it's NULL.
 *
 *  @return             True if the load was successful.
 */
bool EditorChunkStationNode::load( DataSectionPtr pSection, Chunk * pChunk, BW::string* errorString )
{
	BW_GUARD;

	//Note : loading a node will implicitly load the graph the node is
	//a part of.
	bool ok = this->EditorChunkSubstance<ChunkStationNode>::load( pSection, pChunk );
	if (ok)
	{
		transform_.setIdentity();
		transform_.translation( position() );

		BW::vector<DataSectionPtr>	linkSects;
		pSection->openSections( "link", linkSects );
		for (uint i = 0; i < linkSects.size(); ++i)
		{
			DataSectionPtr ds = linkSects[i];

			UniqueID toId    = ds->readString( "to" );
			bool traversable = ds->readBool( "traversable" );

			ChunkStationNode *other = graph()->getNode(toId);
			if (other != NULL)
			{
				setLink(other, traversable);
				LinkInfoConstIter it = other->findLink(id());
				if (it != other->endLinks())
					other->setLink(this, it->second);
			}
		}

        updateRegistration( pChunk ); // registration was slightly incorrect
	}
	else if ( errorString )
	{
		*errorString = "Failed to load patrol node";
		if ( id() != UniqueID::zero() )
		{
			*errorString += ' ' + id().toString();
		}
	}

	return ok;
}


/**
 *  This is called when the EditorChunkStationNode is moved out of a chunk or
 *  placed into a chunk.
 *
 *  @param pChunk           The new chunk that the node will be placed into.
 *                          NULL if the node is being removed from a chunk.
 */ 
void EditorChunkStationNode::toss( Chunk * pChunk )
{
	BW_GUARD;

	if (pChunk_ != NULL)
	{
		ChunkModelObstacle::instance( *pChunk_ ).delObstacles( this );

		if (pOwnSect_ && !pChunk_->loading() && EditorChunkCache::instance( *pChunk_ ).pChunkSection() )
		{
			EditorChunkCache::instance( *pChunk_ ).
				pChunkSection()->delChild( pOwnSect_ );
			pOwnSect_ = NULL;
		}
	}

	ChunkStationNode::toss( pChunk );

	if (pChunk_ != NULL)
	{
		if (!pOwnSect_ && !pChunk_->loading() && EditorChunkCache::instance( *pChunk_ ).pChunkSection() )
		{
			pOwnSect_ = EditorChunkCache::instance( *pChunk_ ).
				pChunkSection()->newSection( this->sectName() );
			this->edSave( pOwnSect_ );
		}

		this->addAsObstacle();
	}
}


/**
 *  This is called to save the EditorChunkStationNode to a DataSection.
 *
 *  @param pSection     The DataSection to save to.
 */
bool EditorChunkStationNode::edSave( DataSectionPtr pSection )
{
	BW_GUARD;

	MF_ASSERT( pSection );	
	MF_ASSERT( pChunk_ );

	if (!edCommonSave( pSection ))
		return false;

	pSection->writeString( "id", id().toString() );
	pSection->writeVector3( "position", position() );
	if (graph())
		pSection->writeString( "graph", graph()->name().toString() );
    pSection->writeString( "userString", userString() );

	// Clear the links
	while (pSection->deleteSection( "link" ))
		;	

	// Write out the links
    LinkInfoConstIter i = beginLinks();
	for (; i != endLinks(); ++i)
	{
		// Only write the link if the node we are connecting
		// to exists, and has a chunk.  If it has no chunk,
		// that means the node has been deleted.
		ChunkStationNode* node = graph()->getNode(i->first);
		if (node && node->chunk())
		{
			DataSectionPtr ds = pSection->newSection( "link" );
			ds->writeString( "to", i->first.toString() );
			ds->writeBool( "traversable", i->second );
		}
	}		

	return true;
}


/**
 *  This returns the transformation of the EditorChunkStationNode.
 */
const Matrix & EditorChunkStationNode::edTransform() 
{ 
    return transform_; 
}


/**
 *	Change our transform, temporarily or permanently
 */
bool EditorChunkStationNode::edTransform( const Matrix & m, bool transient )
{
	BW_GUARD;

	// it's permanent, so find out where we belong now
	Chunk * pOldChunk = pChunk_;
	Chunk * pNewChunk = this->edDropChunk( m.applyToOrigin() );
	if (pNewChunk == NULL) return false;

	// if this is only a temporary change, keep it in the same chunk
	if (transient)
	{
		transform_ = m;
		this->syncInit();
		return true;
	}

	// make sure the chunks aren't readonly
	if (!EditorChunkCache::instance( *pOldChunk ).edIsWriteable() 
		|| !EditorChunkCache::instance( *pNewChunk ).edIsWriteable())
		return false;

	if( pOldChunk != pNewChunk )
	{
		GeometryMapping* dirMap = WorldManager::instance().geometryMapping();
		ChunkSpacePtr pSpace = dirMap->pSpace();
		Vector3 currentPosition = dirMap->invMapper().applyPoint( pOldChunk->centre() );
		Vector3 nextPosition = dirMap->invMapper().applyPoint( pNewChunk->centre() );

		int cx = pSpace->pointToGrid( currentPosition.x );
		int cz = pSpace->pointToGrid( currentPosition.z );
		int nx = pSpace->pointToGrid( nextPosition.x );
		int nz = pSpace->pointToGrid( nextPosition.z );
		WorldManager::instance().connection().linkPoint( (int16)cx, (int16)cz, (int16)nx, (int16)nz );
	}

	// ok, accept the transform change then
	transform_.multiply( m, pOldChunk->transform() );
	transform_.postMultiply( pNewChunk->transformInverse() );
	position( transform_.applyToOrigin() );

	// note that both affected chunks have seen changes
	WorldManager::instance().changedChunk( pOldChunk );
	if (pNewChunk != pOldChunk )
	{
		WorldManager::instance().changedChunk( pNewChunk );
	}

    beginMove();
	edMove( pOldChunk, pNewChunk );
    endMove();
	this->syncInit();
	return true;
}


/**
 *	Get a description of this item
 */
Name EditorChunkStationNode::edDescription()
{
	BW_GUARD;

	STATIC_LOCALISE_NAME( s_desc, "WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_STATION/ED_DESCRIPTION" );

	return s_desc;
}


/**
 *	Add the properties of this station node to the given editor
 */
bool EditorChunkStationNode::edEdit( class GeneralEditor & editor )
{
	BW_GUARD;

	if (this->edFrozen())
		return false;

	if (!edCommonEdit( editor ))
	{
		return false;
	}

	STATIC_LOCALISE_NAME( s_position, "WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_STATION/POSITION" );
	STATIC_LOCALISE_NAME( s_id, "WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_STATION/ID" );
	STATIC_LOCALISE_NAME( s_graph, "WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_STATION/GRAPH" );
	STATIC_LOCALISE_NAME( s_userString, "WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_STATION/USER_STRING" );
	STATIC_LOCALISE_NAME( s_link, "WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_STATION/LINK" );

	MatrixProxyPtr pMP = new ChunkItemMatrix( this );
	editor.addProperty( new ChunkItemPositionProperty( s_position, pMP, this ) );

	editor.addProperty( new StaticTextProperty( s_id, new
		ConstantDataProxy<StringProxy>( id().toString() ) ) );

	if (graph() != NULL)
		editor.addProperty( new StaticTextProperty( s_graph, new
			ConstantDataProxy<StringProxy>( graph()->name().toString() ) ) );

    editor.addProperty
    ( 
        new TextProperty
        ( 
            s_userString, 
            new AccessorDataProxy<EditorChunkStationNode, StringProxy>
            (
                this,
                "user string",
                &EditorChunkStationNode::getUserString,
                &EditorChunkStationNode::setUserString
            )
        )
    );

    LinkProxyPtr linkProxy = new StationNodeLinkProxy(*this);
    editor.addProperty( new LinkProperty( s_link, linkProxy, pMP ) );

	return true;
}


/**
 * Return false if any of our links aren't loaded
 */
bool EditorChunkStationNode::edCanDelete()
{
	BW_GUARD;

	bool baseCanDelete = EditorChunkSubstance<ChunkStationNode>::edCanDelete();

	if (beginLinks() == endLinks())
		return baseCanDelete;

	// First check for any non loaded items
	LinkInfoConstIter i = beginLinks();
	for (; i != endLinks(); ++i)
		if (!graph()->getNode( i->first ))
			return false;

	return baseCanDelete;
}


/**
 * Update the links of the nodes we're linked to, so they won't point to us
 */
void EditorChunkStationNode::edPreDelete()
{
	BW_GUARD;

    EditorChunkStationNode *first = NULL;

	// Add links from everything that link to use, to the first thing that
    // this node links to.
	for (LinkInfoConstIter i = beginLinks(); i != endLinks(); ++i)
	{
		EditorChunkStationNode* node = static_cast<EditorChunkStationNode*>
				(graph()->getNode( i->first ));

		MF_ASSERT( node->chunk() );
		MF_ASSERT( node->pOwnSect() );


		if (!first)
		{
			// everything that connects to us will now connect to first
			first = node;
		}
		else
		{
			// if the node is not linked to the first node, transfer our link onto it
			if (!node->isLinkedTo( first->id() ))
			{
                // Add an undo op between us and the node
		        UndoRedo::instance().add
                (
                    new StationLinkOperation( node, first, ChunkLink::DIR_NONE ) 
                );
                ChunkLink::Direction dir = node->getLinkDirection( this );
                MF_ASSERT(dir != ChunkLink::DIR_NONE);
				EditorChunkLink *newLink = 
                    (EditorChunkLink *)node->createLink(dir, first).getObject();
                newLink->makeDirty();
			}
		}
	}

	// Unlink everything:
    for (LinkInfoConstIter i = beginLinks(); i != endLinks(); ++i)
    {
		EditorChunkStationNode* node = static_cast<EditorChunkStationNode*>
				(graph()->getNode( i->first ));

        // Add an undo op between us and the node
		UndoRedo::instance().add
        (
            new StationLinkOperation( this, node, getLinkDirection( node ) ) 
        );
    }
    BW::vector<UniqueID> neighbours;
    getNeighbourHoodList(neighbours);    
	unlink();
    makeNeighboursDirty(neighbours);

    // Unregister the node:
    graph()->deregisterNode(this);
    UndoRedo::instance().add(new ECSSaveRegisterState(this, true));

    UndoRedo::instance().add(new ECSSaveDeleteState(this));

    deleted_ = true;
	EditorChunkItem::edPreDelete();
}


/**
 *  This is called to draw the EditorChunkStationNode.
 */
void EditorChunkStationNode::draw( Moo::DrawContext& drawContext )
{
	BW_GUARD;

	// cull the nodes in the reflection....
	if (Moo::rc().reflectionScene() && Moo::rc().mirroredTransform())
			return;

	EditorChunkSubstance<ChunkStationNode>::draw( drawContext );
}


/**
 *  This is called to determine whether the node should be drawn.
 *
 *  @return             The same value as ChunkStationNode returns.  The 
 *                      immediate base class assumes that this class represents
 *                      scenery.
 */
/*virtual*/ bool EditorChunkStationNode::edShouldDraw()
{
	BW_GUARD;

    return 
    	ChunkStationNode::edShouldDraw() &&
		OptionsGameObjects::udosVisible() &&
        s_enableDraw_;
}

/**
 *  This is called to determine whether the node could be edited.
 *
 *  @return             true if could be edited, otherwise false
 */
/*virtual*/ bool EditorChunkStationNode::edIsEditable() const
{
	BW_GUARD;

	if( !ChunkItem::edIsEditable() )
		return false;
	BW::vector<ChunkStationNode*> nodes = graph()->getAllNodes();
	ChunkSpacePtr pSpace = WorldManager::instance().geometryMapping()->pSpace();
	for( BW::vector<ChunkStationNode*>::size_type i = 0; i < nodes.size() - 1; ++i )
	{
		Vector3 currentPosition = nodes[ i ]->chunk()->transform().applyPoint( nodes[ i ]->position() );
		Vector3 nextPosition = nodes[ i + 1 ]->chunk()->transform().applyPoint( nodes[ i + 1 ]->position() );
		int cx = pSpace->pointToGrid( currentPosition.x );
		int cz = pSpace->pointToGrid( currentPosition.z );
		int nx = pSpace->pointToGrid( nextPosition.x );
		int nz = pSpace->pointToGrid( nextPosition.z );
		// check if they belong to the same lock area for safe
		return WorldManager::instance().connection().isSameLock( (int16)cx, (int16)cz, (int16)nx, (int16)nz );
	}
	return true;
}

/**
 *  This is called to get the direction of the link to other.
 */
ChunkLink::Direction 
EditorChunkStationNode::getLinkDirection( EditorChunkStationNode const *other ) const
{
	BW_GUARD;

	if (!isLinkedTo( other->id() ) || !other->isLinkedTo( id() ))
	{
		// Check that if we have one without a link, we have both without a 
        // link
		MF_ASSERT( isLinkedTo( other->id() ) == other->isLinkedTo( id() ) );
		return ChunkLink::DIR_NONE;
	}

	bool ago = false, bgo = false;

	if (canTraverse(other->id()))
		ago = true;

	if (other->canTraverse(id()))
		bgo = true;

	if (ago && bgo)
        return ChunkLink::DIR_BOTH;

	if (ago)
        return ChunkLink::DIR_START_END;

	if (bgo)
		return ChunkLink::DIR_END_START;

	MF_ASSERT( !"Neither node has a traversable link" );
	return ChunkLink::DIR_NONE;
}


/**
 *  This gets the idx'th link.
 */
EditorChunkLinkPtr EditorChunkStationNode::getLink(size_t idx) const
{
	BW_GUARD;

    ChunkLinkPtr chunkLink = getChunkLink(idx);
    if (chunkLink == NULL)
        return EditorChunkLinkPtr();
    if (chunkLink->isEditorChunkLink())
        return (EditorChunkLink*)chunkLink.getObject();
    else
        return EditorChunkLinkPtr();
}


/**
 *  This creates a link to other of the given type.
 *
 *  @param linkType         The type of the link.
 *  @param other            The node to link to.
 *
 *  @return                 The link between this and other.
 */
ChunkLinkPtr 
EditorChunkStationNode::createLink
( 
    ChunkLink::Direction    linkType, 
    EditorChunkStationNode  *other 
)
{
	BW_GUARD;

	MF_ASSERT( other );
	MF_ASSERT( other != this );

	MF_ASSERT( pOwnSect() );
	MF_ASSERT( other->pOwnSect() );

    ChunkLinkPtr result = NULL;

    if (graph() != other->graph())
        return result;

	switch (linkType)
	{
	case ChunkLink::DIR_NONE:
		if (isLinkedTo( other->id() ))
			removeLink( other );
		if (other->isLinkedTo( id() ))
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

	// Save the changes
	edSave( pOwnSect() );
	WorldManager::instance().changedChunk( chunk() );

	other->edSave( other->pOwnSect() );
	WorldManager::instance().changedChunk( other->chunk() );
	other->edPostModify();

    return result;
}


/**
 *  Yes, this is an EditorChunkStationNode.
 */
bool EditorChunkStationNode::isEditorChunkStationNode() const 
{ 
    return true; 
}


/**
 *  Has the isEditorChunkStationNode been deleted?
 */
bool EditorChunkStationNode::deleted() const 
{ 
    return deleted_; 
}


/**
 *  This is called to split the link to other.
 *
 *  @param other            The node whose link we wish to split.
 */
void EditorChunkStationNode::split(EditorChunkStationNode &other)
{
	BW_GUARD;

    // If not linked then we cannot split:
    ChunkLinkPtr link = findLink(&other);
    if (link == NULL)
        return;

    // Get the midpoint position:
    ChunkPtr midChunk = NULL;
    EditorChunkLink *editLink = 
        static_cast<EditorChunkLink *>(link.getObject());
    Vector3 midPoint = editLink->midPoint(midChunk);

    // Add undo/redo for the current link:
   	UndoRedo::instance().add
    (
        new StationLinkOperation
        (
            this, 
            &other, 
            getLinkDirection(&other)
        ) 
    );

    // Which way does the linking go:
    bool usToOther = canTraverse(other.id());
    bool otherToUs = other.canTraverse(id());

    // Remove the link:
    removeLink(&other);
    other.removeLink(this);

    // Add a new node at the mid point:
    UniqueID newNodeId = UniqueID::generate();
    EditorChunkStationNode *newNode = new EditorChunkStationNode();
    if (pOwnSect() != NULL)
    {
        XMLSectionPtr section = new XMLSection("copy");
        section->copy(pOwnSect());
        section->deleteSections("links");
        newNode->load(section, midChunk);
    }
    newNode->unlink();
    newNode->setUserString(BW::string());
    newNode->id(newNodeId);
    newNode->graph(graph());
    newNode->chunk(midChunk);
    midChunk->addStaticItem(newNode);
    if (!graph()->isRegistered(newNode))
        graph()->registerNode(newNode, midChunk);
    Matrix xform;
    xform.setIdentity();
    xform.translation(midPoint);
    newNode->edTransform(xform, false);    

    // Add an undo/redo for the new node:
	UndoRedo::instance().add
    (
		new ChunkItemExistenceOperation(newNode, NULL) 
    );

    // Add an undo/redo for these links:
   	UndoRedo::instance().add
    (
        new StationLinkOperation
        (
            this, 
            newNode, 
            ChunkLink::DIR_NONE
        ) 
    );
   	UndoRedo::instance().add
    (
        new StationLinkOperation
        (
            newNode, 
            &other, 
            ChunkLink::DIR_NONE
        ) 
    );

    // Add links from us, to the new node, to the end:
    setLink(newNode, usToOther);
    newNode->setLink(this  , otherToUs);
    newNode->setLink(&other, usToOther);
    other.setLink(newNode, otherToUs);

    // Add the undo barrier:
    UndoRedo::instance().barrier(LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_STATION/SPLIT_LINK"), false);

    // Force an update of the links data sections and mark chunks as dirty:
    makeDirty();
    newNode->makeDirty();
    other.makeDirty();
}


/**
 *  This tests the topological validity of the node.
 *
 *  @param failureMsg       This is set to the failure message if not valid.
 * 
 *  @return                 True if valid, false otherwise.
 */
/*virtual*/ bool EditorChunkStationNode::isValid(BW::string &failureMsg) const
{
	BW_GUARD;

    if (!ChunkStationNode::isValid(failureMsg))
        return false;

    return true;
}


/**
 *  This makes the node dirty, by making sure its pSection is up to date and
 *  it's chunk is flagged as dirty.
 */
void EditorChunkStationNode::makeDirty()
{
	BW_GUARD;

    Chunk *c = chunk();
    if (c != NULL)
    {
        beginMove();
        c->delStaticItem(this); 
        c->addStaticItem(this); // this also forces a save
        WorldManager::instance().changedChunk(c);
		syncLinksWithGraph();
        endMove();
    }    
}


/** 
 *  Set to true if the EditorChunkStationNode has been deleted.
 */
void EditorChunkStationNode::deleted(bool del) 
{ 
    deleted_ = del; 
}


/**
 *  This helps to create a clone of cloned nodes.
 *
 *  @param cloneNodes           The nodes that were cloned.
 *  @param newNodes             The new nodes.
 */
/*static*/ void EditorChunkStationNode::linkClonedNodes
(
    BW::vector<EditorChunkStationNodePtr> const    &cloneNodes,
    BW::vector<EditorChunkStationNodePtr>          &newNodes
)
{
	BW_GUARD;

    if (cloneNodes.size() == 1)
    {
		// Add an undo op to remove the link we're about to create:
		UndoRedo::instance().add
        ( 
            new StationLinkOperation
            (
                cloneNodes[0], 
                newNodes[0], 
                ChunkLink::DIR_NONE
            )
        );

        // Create a link in the new nodes:
        cloneNodes[0]->setLink(newNodes  [0].getObject(), true );
        newNodes  [0]->setLink(cloneNodes[0].getObject(), false);
        cloneNodes[0]->makeDirty();
        newNodes  [0]->makeDirty();
    }
    else
    {
        // Create a new graph for the cloned nodes:
        UniqueID graphId = UniqueID::generate();
        StationGraph *newGraph = StationGraph::getGraph(graphId);

        // For each of the new nodes, remove them from the old graph and add
        // them to the new graph:
        for (size_t i = 0; i < newNodes.size(); ++i)
        {
            EditorChunkStationNodePtr newNode = newNodes[i];
            StationGraph *oldGraph = newNode->graph();
            oldGraph->deregisterNode(newNode.getObject());
            newGraph->registerNode(newNode.getObject(), newNode->chunk());
            newNode->graph(newGraph);
            newNode->makeDirty();
        }        

        // For each of the links on the cloned nodes, see if the linked to node
        // was also cloned.  If so then create a link in the new nodes that 
        // matches this.
        for (size_t i = 0; i < cloneNodes.size(); ++i)
        {
            EditorChunkStationNodePtr oldNode = cloneNodes[i];
            EditorChunkStationNodePtr newNode = newNodes  [i];
            for 
            (
                LinkInfoConstIter it = oldNode->beginLinks(); 
                it != oldNode->endLinks(); 
                ++it
            )
            {
                size_t idx = findNode(it->first, cloneNodes);
                if (idx != static_cast<size_t>(-1))
                {
		            // Add an undo op to remove the link we're about to create:
		            UndoRedo::instance().add
                    ( 
                        new StationLinkOperation
                        (
                            newNode, 
                            newNodes[idx], 
                            ChunkLink::DIR_NONE
                        )
                    );

                    // Create a link in the new nodes:
                    ChunkLinkPtr link =
                        newNode->setLink(newNodes[idx].getObject(), it->second);
                    EditorChunkLink *editLink =
                        (EditorChunkLink *)link.getObject();
                    editLink->makeDirty();
                }
            }
        }
    }
}



/**
 *  This function can be used to disable/enable drawing of nodes.
 */
/*static*/ void EditorChunkStationNode::enableDraw(bool enable)
{
    s_enableDraw_ = enable;
}


/**
 *  This is used as part of the loading of a EditorChunkStationNode.
 *
 *  @param pSection     The section to load from.
 *  @param pChunk       The chunk that the node belongs to.
 */
bool EditorChunkStationNode::loadName( DataSectionPtr pSection, Chunk* pChunk )
{
	BW_GUARD;

	BW::string idStr = pSection->readString( "id" );
	BW::string graphStr = pSection->readString( "graph" );

	// Deregister ourself to support reloading
	if (graph())
		graph()->deregisterNode( this );

	if (graphStr.empty())
		graph( StationGraph::getGraph( UniqueID::generate() ) );
	else
		graph( StationGraph::getGraph( UniqueID( graphStr ) ) );


	if (idStr.empty())
	{
		id( UniqueID::generate() );
	}
	else if (graph()->isRegistered( UniqueID(idStr) ))
	{
		// There's already a node with our name, ie, we're being cloned: generate a new name
		UniqueID oldId = idStr;

		id( UniqueID::generate() );

		// Clear the links from the data section
		MF_ASSERT( beginLinks() == endLinks() );
		while (pSection->deleteSection( "link" ))
			;
	}
	else
	{
		id( UniqueID( idStr ) );
	}


	graph()->registerNode( this, pChunk );

	return true;
}


/**
 *  This helps create links of the correct type.
 *
 *  @return         A new EditorChunkLink.
 */
ChunkLinkPtr EditorChunkStationNode::createLink() const
{
	BW_GUARD;

    return new EditorChunkLink();
}


/**
 *  This sets the user string.
 *
 *  @param str      The new user string.
 *
 *  @return         True if this is possible.
 */
bool EditorChunkStationNode::setUserString(BW::string const &str)
{
	BW_GUARD;

    if (graph() == NULL)
        return false;

    ChunkStationNode *node = graph()->getNode(id());
    if (node == NULL)
        return false;

    node->userString( str );

    return true;
}


/**
 *  This gets the user string.
 */
BW::string EditorChunkStationNode::getUserString() const
{
	BW_GUARD;

    if (graph() == NULL)
        return BW::string();

    ChunkStationNode const *node = graph()->getNode(id());
    if (node == NULL)
        return BW::string();

    return node->userString();
}


/**
 *  This gets all the connected nodes.
 *
 *  @param neighbours       Nodes connected to this one.
 */
void EditorChunkStationNode::getNeighbourHoodList
(
    BW::vector<UniqueID>   &neighbours
) const
{
	BW_GUARD;

    neighbours.push_back(id());
    for
    (
        LinkInfoConstIter it = beginLinks();
        it != endLinks();
        ++it
    )
    {
        UniqueID otherID = it->first;
        neighbours.push_back(otherID);
    }
}


/**
 *  This makes the neighbours dirty.
 */
void EditorChunkStationNode::makeNeighboursDirty
(
    BW::vector<UniqueID> const &neighbours
) const
{
	BW_GUARD;

    for (size_t i = 0; i < neighbours.size(); ++i)
    {
        UniqueID const &otherID = neighbours[i];
        ChunkStationNode *otherCSN = graph()->getNode(otherID);
        if (otherCSN != NULL && otherCSN->isEditorChunkStationNode())
        {
            EditorChunkStationNode *editCSN = 
                (EditorChunkStationNode *)otherCSN;
            editCSN->makeDirty();
        }
    }
}


/**
 *	This synchronises the this node with the internal node that the graph keeps.
 */
void EditorChunkStationNode::syncLinksWithGraph()
{
	BW_GUARD;

	if (graph() != NULL)
	{
		BW::vector<UniqueID> links;
		for (LinkInfoConstIter it = beginLinks(); it != endLinks(); ++it)
		{
			if (it->second) // the graph's nodes only know things they link TO.
				links.push_back(it->first);
		}
		graph()->updateNodeIds(id(), links);
    }
}


/**
 *  This gets the section name.
 *
 *  @return         "station".
 */
const char *EditorChunkStationNode::sectName() const 
{ 
    return "station"; 
}


/**
 *  This gets if the category for the draw flag is turned on.
 *
 *  @return true if "render/gameObjects" is visible.
 */
bool EditorChunkStationNode::isDrawFlagVisible() const 
{ 
    return OptionsGameObjects::entitiesVisible(); 
}


/**
 *  This gets the drawing flag.
 *
 *  @return         "render/gameObjects/drawEntities".
 */
const char *EditorChunkStationNode::drawFlag() const 
{ 
    return "render/gameObjects/drawEntities"; 
}


/**
 *  This gets the representative model.
 *
 *  @return         The model from "resources/models/station.model".
 */
ModelPtr EditorChunkStationNode::reprModel() const
{
	return model_;
}


/// Write the factory statics stuff
#undef IMPLEMENT_CHUNK_ITEM_ARGS
#define IMPLEMENT_CHUNK_ITEM_ARGS (pSection, pChunk, &errorString)
IMPLEMENT_CHUNK_ITEM( EditorChunkStationNode, station, 1 )




// -----------------------------------------------------------------------------
// Section: StationGraphProperty
// -----------------------------------------------------------------------------


StationGraphProperty::StationGraphProperty( const Name& name, StringProxyPtr graph )
	: TextProperty( name, graph )
{
	GENPROPERTY_MAKE_VIEWS()
}


GENPROPERTY_VIEW_FACTORY( StationGraphProperty )


// -----------------------------------------------------------------------------
// Section: CurrentStationGraphProperties
// -----------------------------------------------------------------------------


BW::vector<StationGraphProperty*> PropertyCollator<StationGraphProperty>::properties_;
CurrentStationGraphProperties::ViewEnroller CurrentStationGraphProperties::s_viewEnroller;


ChunkLink::Direction increamentDirection(ChunkLink::Direction dir)
{
    switch (dir)
    {
    case ChunkLink::DIR_NONE:
        return ChunkLink::DIR_START_END;

    case ChunkLink::DIR_START_END:
        return ChunkLink::DIR_END_START;

    case ChunkLink::DIR_END_START:
        return ChunkLink::DIR_BOTH;

    case ChunkLink::DIR_BOTH:
        return ChunkLink::DIR_START_END;
    }

    return ChunkLink::DIR_START_END;
}


void EditorChunkStationNode::syncInit()
{
	BW_GUARD;

	#if UMBRA_ENABLE	

	bw_safe_delete(pUmbraDrawItem_);

	if (!this->reprModel())
	{	
		return;
	}
	BoundingBox bb = BoundingBox::s_insideOut_;
	// Grab the visibility bounding box
	bb = this->reprModel()->visibilityBox();	

	// Set up object transforms
	Matrix m = pChunk_->transform();
	m.preMultiply( transform_ );

	// Create the umbra chunk item
	UmbraChunkItem* pUmbraChunkItem = new UmbraChunkItem();
	pUmbraChunkItem->init( this, bb, m, pChunk_->getUmbraCell());
	pUmbraDrawItem_ = pUmbraChunkItem;

	this->updateUmbraLenders();
	#endif
}

// -----------------------------------------------------------------------------
// Section: adjustStationLink
// -----------------------------------------------------------------------------

/*
	Adjust the link between what's currently selected (passed in the 1st arg,
	and what properties are loaded), and the station passed in as the 2ed arg.

	Stations have their link type cycled

	CurrentStationGraphProperties are set to the graph of the 2ed arg
*/
static PyObject* py_adjustStationLink( PyObject* args )
{
	BW_GUARD;

	// parse arguments
	PyObject* pPyRev1 = NULL;
	PyObject* pPyRev2 = NULL;
	if (!PyArg_ParseTuple( args, "OO", &pPyRev1, &pPyRev2 ) ||
		!ChunkItemRevealer::Check( pPyRev1 ) ||
		!ChunkItemRevealer::Check( pPyRev2 ))
	{
		PyErr_SetString( PyExc_TypeError, "adjustStationLink() "
			"expects two Revealer arguments" );
		return NULL;
	}

	ChunkItemRevealer::ChunkItems items1;
	ChunkItemRevealer::ChunkItems items2;

	(static_cast<ChunkItemRevealer*>( pPyRev1 ))->reveal( items1 );
	(static_cast<ChunkItemRevealer*>( pPyRev2 ))->reveal( items2 );

	if (items1.size() == 0)
	{
		// nothing to do
		Py_RETURN_NONE;
	}

	STATIC_LOCALISE_NAME( s_stationDesc, "WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_STATION/ED_DESCRIPTION" );
	bool stationsFound = false;
	bool nonStationsFound = false;
	for (BW::vector<ChunkItemPtr>::iterator it = items1.begin();
		it != items1.end();
		it++)
	{
		if ((*it)->edDescription() == s_stationDesc)
			stationsFound = true;
		else
			nonStationsFound = true;
	}

	if (stationsFound && nonStationsFound)
	{
		return Py_BuildValue( "s", "Selected items must be of the same type" );
	}
	else if (!stationsFound)
	{
		// do nothing.. let someone else handle it
		Py_RETURN_NONE;
	}


	if (items2.size() != 1)
		return Py_BuildValue( "s", "to link stations, 2nd station may only be a single item" );

	if (items2.front()->edDescription() != "patrol node")
		return Py_BuildValue( "s", "to link stations, 2nd item must be a station" );

	EditorChunkStationNode* node2 = static_cast<EditorChunkStationNode*>(&*items2.front());

	// first check for any errors
	ChunkItemRevealer::ChunkItems::iterator i;
	for (i = items1.begin(); i != items1.end(); ++i)
	{
		if ((*i)->edDescription() == s_stationDesc)
		{
			EditorChunkStationNode* node1 = static_cast<EditorChunkStationNode*>(&*(*i));
			// Fail if they're in different graphs
			if (node1->graph() != node2->graph())
				return Py_BuildValue( "s", "Can't link stations in different graphs" );

			// Can't link to ourself, either
			if (node1 == node2)
				return Py_BuildValue( "s", "Can't link a station to itself" );
		}
	}

	// cycle link type on any stations in items1
	for (i = items1.begin(); i != items1.end(); ++i)
	{
		if ((*i)->edDescription() == s_stationDesc)
		{
			EditorChunkStationNode* node1 = 
                static_cast<EditorChunkStationNode*>(&*(*i));

			// Get the current link type between them
            ChunkLink::Direction dir = node1->getLinkDirection(node2);

			// Add an undo op
			UndoRedo::instance().add
            ( 
                new StationLinkOperation
                ( 
                    node1, 
                    node2, 
                    node1->getLinkDirection( node2 ) 
                ) 
            );

			// Change to the next link type
			dir = increamentDirection(dir);

			// Apply the new link type
			node1->createLink( dir, node2 );
		}
	}

	// ok, now go through all StationGraphProperties
	BW::vector<StationGraphProperty*> properties = CurrentStationGraphProperties::properties();
	BW::vector<StationGraphProperty*>::iterator pi = properties.begin();
	for (; pi != properties.end(); ++pi)
	{
		(*pi)->pString()->set( node2->graph()->name(), false );
	}

	if (!properties.empty())
		WorldManager::instance().addCommentaryMsg(
			LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_STATION/CONNECT_TO",
			node2->graph()->name().toString() ), 0 );

	Py_RETURN_NONE;
}
PY_MODULE_FUNCTION( adjustStationLink, WorldEditor )

BW_END_NAMESPACE

