#include "pch.hpp"

#include "station_graph.hpp"

#include "chunk.hpp"
#include "chunk_manager.hpp"
#include "chunk_space.hpp"
#include "chunk_stationnode.hpp"
#include "geometry_mapping.hpp"

#include "cstdmf/debug.hpp"
#include "cstdmf/guard.hpp"

#include "resmgr/auto_config.hpp"

#include <limits>


DECLARE_DEBUG_COMPONENT2( "Chunk", 0 )


BW_BEGIN_NAMESPACE

int ChunkStationGraph_token;

typedef BW::map<UniqueID, StationGraph*> StationGraphs;
StationGraphs StationGraph::graphs_;


namespace
{
    static SimpleMutex s_nodesMutex;
    static SimpleMutex s_graphsMutex;

	// This removes any prepending of "id." (which makes .graph files
	// valid XML).
	BW::string stripNodeSectionName(BW::string const &name)
	{
		if (name.substr(0, 3) == "id.")
			return name.substr(3, BW::string::npos);
		else
			return name;
	}

#ifdef EDITOR_ENABLED
	// This adds "id." to a graph identifier so that it can be used as an
	// XML tag.
	BW::string buildNodeSectionName(BW::string const &name)
	{
		return "id." + name;
	}
#endif // EDITOR_ENABLED
}


/**
 *	This method returns the station graph identified by the given
 *	ID.
 *
 *	@param	graphName		ID of the graph to retrieve.
 *
 *	@return	StationGraph*	If available, the station graph represented by ID.
 */
StationGraph* StationGraph::getGraph( const UniqueID& graphName )
{
	BW_GUARD;
	s_graphsMutex.grab();
	BW::map<UniqueID, StationGraph*>::iterator i = graphs_.find( graphName );

	if (i != graphs_.end())
	{
		s_graphsMutex.give();
		return i->second;
	}
	else
	{
		s_graphsMutex.give();
		StationGraph* graph = new StationGraph( graphName );
		s_graphsMutex.grab();
		graphs_[graphName] = graph;
		s_graphsMutex.give();
		return graph;
	}
}


/**
 *	This method returns a vector of all loaded Station Graphs.
 *
 *  @return BW::vector<StationGraph*> The list of all loaded station graphs.
 */
BW::vector<StationGraph*> StationGraph::getAllGraphs()
{
	BW_GUARD;
	SimpleMutexHolder holder( s_graphsMutex );

	BW::vector<StationGraph*> g;

	BW::map<UniqueID, StationGraph*>::iterator i = graphs_.begin();
	for (; i != graphs_.end(); ++i)
	{
		g.push_back( i->second );
	}

	return g;
}


/**
 *  This method returns the unique id of the graph.
 *
 *  @return const UniqueID& The id of the graph.
 */
const UniqueID& StationGraph::name() const
{
    return name_;
}


/**
 *	This method returns whether or not the station graph is ready
 *	for use.  It will be true once the first chunk item node is
 *	loaded.  This is because without a chunk item, the real world
 *	positions of the nodes in the graph are not known, and also
 *	because graphs need to be loaded via the chunk loading thread
 *	and not directly by script or main thread code.
 */
bool StationGraph::isReady() const
{
	return isReady_;
}


/**
 *	This method returns whether or not there is a traversal available
 *	from src to dst.
 *
 *	@param	src		ID of the source node.
 *	@param	dst		ID of the destination node.
 *
 *	@return	bool	Whether or not you may traverse from src to dst.
 */
bool StationGraph::canTraverseFrom( const UniqueID& src, const UniqueID& dst )
{
	BW_GUARD;
	StationGraph::Node* node = this->node(src);
	if (node)
	{
		return node->hasTraversableLinkTo(dst);
	}
	else
	{
		return false;
	}
}


/**
 *	This method returns a vector of node IDs representing nodes that are
 *	reachable from the given source node.
 *
 *	@param	src			source Node
 *	@param	retNodeIDs	return vector of node IDs
 *
 *	@return	uint32		number of reachable nodes
 */
uint32 StationGraph::traversableNodes( const UniqueID& src,
    BW::vector<UniqueID>& retNodeIDs )
{
	BW_GUARD;
	retNodeIDs.clear();

	StationGraph::Node* srcNode = this->node(src);
	if (!srcNode)
	{
		ERROR_MSG( "StationGraph::traversableNodes was called with an invalid "
					"source node %s\n", src.toString().c_str() );
		return 0;
	}

	retNodeIDs.assign( srcNode->links().begin(), srcNode->links().end() );
	return static_cast<uint32>(retNodeIDs.size());
}


/**
 *	This method returns the world position of the given node.  The
 *	resultant world position is returned in the passed in retWorldPos
 *	Vector3 reference.
 *
 *	@param	src				ID of the node.
 *	@param	retWorldPos		Returned Vector3 containing the world position.
 *
 *	@return	bool			Whether or not retWorldPos has been set.
 */
bool StationGraph::worldPosition( const UniqueID& src, Vector3& retWorldPos )
{
	BW_GUARD;
	//calculate the world position of the given node.
	StationGraph::Node* node = this->node(src);
	if (node)
	{
		retWorldPos = node->worldPosition_;
		return true;
	}
	else
	{
		return false;
	}
}


/**
 *	This method returns the nearest node to the given world position.
 *
 *  @param worldPos         The query position.
 *
 *  @return                 The id of the nearest node.
 *                          The id will be zero if there isn't a nearest
 *                          node.
 */
const UniqueID& StationGraph::nearestNode( const Vector3& worldPos )
{
	BW_GUARD;
	SimpleMutexHolder holder( s_nodesMutex );

    float nearest = std::numeric_limits<float>::max();
	Node* found   = NULL;

	BW::map<UniqueID, StationGraph::Node>::iterator it = nodes_.begin();
	while (it != nodes_.end())
	{
		Node  &node = it->second;
		float dist  = Vector3(node.worldPosition_ - worldPos).lengthSquared();
		if (dist < nearest)
		{
			nearest = dist;
			found = &node;
		}
		++it;
	}

	if (found)
		return found->id_;

	return UniqueID::zero();
}


#ifdef EDITOR_ENABLED

/**
 *	This method determines whether a node has been registered with a graph.
 *
 *  @param node         The query node.
 *
 *  @return             True if the node is already registered, false
 *                      otherwise.
 */
bool StationGraph::isRegistered( ChunkStationNode* node )
{
	SimpleMutexHolder holder( s_nodesMutex );

	return edNodes_.find( node->id() ) != edNodes_.end();
}

/**
 *	This method determines whether a node has been registered with a graph.
 *
 *  @param node         The id of query node.
 *
 *  @return             True if the node is already registered, false
 *                      otherwise.
 */
bool StationGraph::isRegistered( const UniqueID& id )
{
	return getNode( id ) != NULL;
}

#endif


/**
 *	This method registers a node with a graph.  It keeps the internal Nodes
 *  in sync with the ChunkStationNodes of the graph.
 *
 *  @param node         The node to register.
 *  @param pChunk        The chunk that the node is in.
 */
void StationGraph::registerNode( ChunkStationNode* node, Chunk* pChunk )
{
	BW_GUARD;
	if (!isReady_)
	{
		//Now we have a node in a chunk, we can place the graph
		//in the correct orientation in the world.
		this->constructFilename( pChunk->mapping()->path() );
		GeometryMapping* dirMapping = pChunk->mapping();
		this->load(dirMapping->mapper());
		isReady_ = true;
	}

#ifdef EDITOR_ENABLED
	// Only the editor cares about keeping lists of chunk station nodes.
	{
		SimpleMutexHolder holder( s_nodesMutex );
		edNodes_[node->id()] = node;
	}

	StationGraph::Node n;
	n.id_            = node->id();
	n.worldPosition_ = node->edTransform().applyToOrigin();
    addNode(n);
#endif
}


/**
 *	This method deregisters the given Editor node from the graph.
 *
 *  @param node         The node to deregister.
 */
void StationGraph::deregisterNode( ChunkStationNode* node )
{
	BW_GUARD;	
#ifdef EDITOR_ENABLED
	if (isRegistered( node ))
	{
	    SimpleMutexHolder holder( s_nodesMutex );
	    edNodes_.erase( edNodes_.find( node->id() ) );
	    nodes_.erase( nodes_.find( node->id() ) );
    }
#endif
}


//-----------------------------------------------------------------------------
//	Section : Editor methods.
//-----------------------------------------------------------------------------


#ifdef EDITOR_ENABLED

/**
 *	This method returns the Editor node given by the passed in ID.
 *
 *  @param id           The id of the node to get.
 *
 *  @return             The node with the given id, NULL if there is no such
 *                      node.
 */
ChunkStationNode* StationGraph::getNode( const UniqueID& id )
{
	BW_GUARD;
	SimpleMutexHolder holder( s_nodesMutex );

	BW::map<UniqueID, ChunkStationNode*>::iterator i = edNodes_.find( id );

	if (i != edNodes_.end())
		return i->second;
	else
		return NULL;
}


/**
 *	This method returns all of the Editor nodes associated with this graph.
 *
 *  @return             The list of nodes with this graph.
 */
BW::vector<ChunkStationNode*> StationGraph::getAllNodes()
{
	BW_GUARD;
	SimpleMutexHolder holder( s_nodesMutex );

	BW::vector<ChunkStationNode*> nodes;
	nodes.reserve(edNodes_.size());

	BW::map<UniqueID, ChunkStationNode*>::iterator i = edNodes_.begin();
	for (; i != edNodes_.end(); ++i)
	{
		nodes.push_back( i->second );
	}

	return nodes;
}


/**
 *	This method returns the Editor node given by the ID from the specified
 *	graph.
 *
 *  @param graphName        The id of the graph.
 *  @param id               The id of the node.
 *
 *  @return                 The ChunkStationNode in the given graph with the
 *                          given id.  NULL will be returned if there is no
 *                          such graph or node.
 */
/*static*/ ChunkStationNode* StationGraph::getNode( const UniqueID& graphName,
    const UniqueID& id )
{
    BW_GUARD;
	StationGraph *graph = getGraph( graphName );
    if (graph == NULL)
        return NULL;
    else
        return graph->getNode( id );
}


/**
 *  This method merges two graphs by unregistering all nodes in one graph and
 *  registering them with the other graph.
 *
 *  @param graph1ID     The id of the graph to put all the nodes into.
 *  @param graph2ID     The id of the graph to remove all the nodes from.
 */
/*static*/ void
StationGraph::mergeGraphs( const UniqueID & graph1Id, const UniqueID & graph2Id,
    GeometryMapping * pMapping )
{
    BW_GUARD;
	StationGraph *graph1 = getGraph(graph1Id);
    StationGraph *graph2 = getGraph(graph2Id);

    graph1->loadAllChunks( pMapping );
    graph2->loadAllChunks( pMapping );

    BW::vector<ChunkStationNode*> nodes = graph2->getAllNodes();
    for (size_t i = 0; i < nodes.size(); ++i)
    {
        ChunkStationNode *node = nodes[i];
        graph2->deregisterNode(node);
        graph1->registerNode(node, node->chunk());
        node->graph(graph1);
    }

    // We now flag that the nodes are dirty.  We cannot do this in the above
    // loop because we don't have a topologically valid graph
    for (size_t i = 0; i < nodes.size(); ++i)
        nodes[i]->makeDirty();
}


/**
 *  This method forces all chunks containing nodes of the graph into memory.
 *
 *  @param pMapping       The GeometryMapping used to load nodes with.
 */
void StationGraph::loadAllChunks( GeometryMapping * pMapping )
{
    BW_GUARD;
	// We have to split the finding of chunk names and the loading of them into
    // two loops, otherwise we could be iterating over an invalid tree:
    BW::vector<BW::string> chunkNames;
	const float gridSize = pMapping->pSpace()->gridSize();
    for
    (
        BW::map<UniqueID, StationGraph::Node>::iterator it = nodes_.begin();
        it != nodes_.end();
        ++it
    )
    {
        Node        &node       = it->second;
        Vector3     &pos        = node.worldPosition_;
        int         wgx         = (int)floor(pos.x/gridSize);
        int         wgz         = (int)floor(pos.z/gridSize);
        Vector3     localPos    = Vector3
                                    (
                                        wgx*gridSize + gridSize/2.0f,
                                        0.0f,
                                        wgz*gridSize + gridSize/2.0f
                                    );
        BW::string chunkName   = pMapping->outsideChunkIdentifier(localPos);
        if
        (
            !chunkName.empty()
            &&
            (
                chunkNames.empty()
                ||
                chunkNames[chunkNames.size() - 1] != chunkName
            )
        )
        {
            chunkNames.push_back(chunkName);
        }
    }
    for (size_t i = 0; i < chunkNames.size(); ++i)
    {
        ChunkManager::instance().loadChunkNow( chunkNames[i], pMapping );
    }
}


/**
 *	This method saves a station graph to space/graphID.graph
 *	Each node saves its information to the graph.  Nodes in
 *	the graph file that we don't know about are left untouched,
 *	as this indicates nodes that exist but have not been loaded
 *	in this session.
 */
bool StationGraph::save()
{
	BW_GUARD;
	//TODO : Double check the removal of this mutex.  I'm pretty
	//sure WorldEditor (the only one using the save() method) turns
	//off chunk loading while saving, meaning no threading issues
	//during the save.  If not the case, then reinstate the mutex
	//use, but note you have to release it before the this->node
	//method call lower down (so the example use below of SimpleMutexHolder
	//is not valid, will assert).
	//SimpleMutexHolder holder( s_nodesMutex );

	if (pSect_ && isReady_)
	{
        if ( nodes_.empty() )
		{
			BW::string fullname = BWResource::resolveFilename( filename_ );
			std::remove( fullname.c_str() );
		}
		else
		{
			// Delete any children of pSect_ whose names do not match an id of the
			// graph.  These deleted data section nodes correspond to deleted
			// station graph nodes.  Note that we go through nodes_, not edNodes_,
			// this is because nodes_ exist, even  if nodes aren't loaded.
			for (int i = 0; i < pSect_->countChildren(); ++i)
			{
				BW::string name = pSect_->childSectionName(i);
				name = stripNodeSectionName(name);
				if (UniqueID::isUniqueID(name))
				{
					UniqueID id(name);
					if (nodes_.find(id) == nodes_.end())
					{
						DataSectionPtr child = pSect_->openChild(i);
						pSect_->delChild(child);
						--i;
					}
				}
			}

			BW::map<UniqueID, ChunkStationNode*>::iterator it = edNodes_.begin();
			BW::map<UniqueID, ChunkStationNode*>::iterator end = edNodes_.end();
			while (it != end)
			{
				ChunkStationNode* edNode = it->second;
				it++;

				if (edNode->chunk())
				{
					DataSectionPtr pNodeSect = 
						pSect_->openSection( buildNodeSectionName(edNode->id()), true );
					pNodeSect->delChildren();

					//Station Graph Nodes are dumb.  We now need to set their
					//values before saving.
					StationGraph::Node* sgn = this->node(edNode->id());
					if (!sgn)
						continue;

					//set the node's current world position
					Matrix chunkMatrix = edNode->chunk()->transform();
					chunkMatrix.multiply(edNode->edTransform(),chunkMatrix);
					sgn->worldPosition_ = chunkMatrix.applyToOrigin();

					//set the node's user string
					sgn->userString_ = edNode->userString();

					//set the node's links (but only the ones we know about.
					ChunkStationNode::LinkInfoConstIter nit = edNode->beginLinks();
					ChunkStationNode::LinkInfoConstIter ned = edNode->endLinks();
					while (nit != ned)
					{
						const UniqueID& id = nit->first;
						bool traversable = nit->second;

						if (traversable)
						{
							if (!sgn->hasTraversableLinkTo(id))
							{
								sgn->addLink(id);
							}
						}
						else
						{
							if (sgn->hasTraversableLinkTo(id))
							{
								sgn->delLink(id);
							}
						}

						nit++;
					}

					//and save.
					sgn->save( pNodeSect );
				}
			}

			pSect_->save();
		}

		return true;
	}
	else
	{
		//graph had not been loaded yet, because no ChunkStationNode items
		//were discovered.
		return false;
	}
}


/**
 *	This method saves all station graphs that have been
 *	accessed during this session.  It is not expected that
 *	there will be so many graphs that saving all station
 *	graphs will take a relatively long time.  Thus we simply
 *	save all graphs every time.
 */
/*static*/void StationGraph::saveAll()
{
	BW_GUARD;
	TRACE_MSG( "StationGraph::saveAll\n" );
	BW::vector<StationGraph*> graphs = StationGraph::getAllGraphs();
	for (uint i=0; i< graphs.size(); i++)
	{
		graphs[i]->save();
	}
}


/**
 *  This method tests the topological validity of the graph.
 *
 *  @param failureMsg           The failure message if the graph is invalid.
 *
 *  @return                     True if the graph is valid, false if invalid.
 *                              If the graph is invalid then failureMsg will
 *                              be set to the reason why the graph is not
 *                              valid.
 */
/*virtual*/ bool StationGraph::isValid( BW::string &failureMsg ) const
{
    BW_GUARD;
	if (edNodes_.size() != nodes_.size())
    {
        failureMsg = LocaliseUTF8(L"CHUNK/INHABITANTS/STATION_GRAPH/NODE_SIZE_UNEQUAL");
        return false;
    }

    for
    (
        BW::map<UniqueID, ChunkStationNode*>::const_iterator it = edNodes_.begin();
        it != edNodes_.end();
        ++it
    )
    {
        ChunkStationNode *node = it->second;
        if (!node->isValid(failureMsg)) // also validates links
            return false;
    }

    return true;
}

/**
 *	This can be called to completely reset the links of a particular node.
 *
 *  @param id			The id of the node to update.
 *  @param links		The linked nodes of the node.
 *	@return				True if the node was updated.
 */
bool StationGraph::updateNodeIds(const UniqueID &id, const BW::vector<UniqueID> &links)
{
	BW_GUARD;
	StationGraph::Node *sgn = node(id);
	if (sgn != NULL)
	{
		sgn->links().clear();
		for (size_t i = 0; i < links.size(); ++i)
			sgn->addLink(links[i]);
		return true;
	}
	else
	{
		return false;
	}
}

#endif // EDITOR_ENABLED


/**
 *	Constructor.
 *
 *  @param name         The id of the graph.
 */
StationGraph::StationGraph( const UniqueID& name ) :
	filename_( "" ),
	name_( name ),
	pSect_(NULL),
	isReady_( false )
{
	BW_GUARD;
	SimpleMutexHolder holder( s_graphsMutex );

	graphs_[name_] = this;
}


/**
 *	This method adds a node to the graph.
 *
 *  @param node         The node to add.
 */
void StationGraph::addNode( const StationGraph::Node& node )
{
	BW_GUARD;
	SimpleMutexHolder holder( s_nodesMutex );

	//takes a copy
	nodes_[node.id_] = node;
}


/**
 *	This method returns a Station Graph Node given an ID.  It returns NULL if
 *  the node is not found.  The node pointer returned becomes invalid if the
 *  nodes vector is changed, for example if a chunk is loaded.
 *
 *  @param id           The id of the node.
 *
 *  @return             The node with the given id.  NULL if there is no such
 *                      node.
 */
StationGraph::Node* StationGraph::node( const UniqueID& id )
{
	BW_GUARD;
	SimpleMutexHolder holder( s_nodesMutex );

	BW::map<UniqueID, StationGraph::Node>::iterator i = nodes_.find( id );

	if (i != nodes_.end())
	{
		Node& node = i->second;
		return &node;
	}
	else
	{
		return NULL;
	}
}


/**
 *	Create a filename for this graph.
 *
 *  @param spacePath        The base path to use for the graph's filename.
 */
void StationGraph::constructFilename( const BW::string& spacePath )
{
	BW_GUARD;
	filename_ =  spacePath;
	filename_ += this->name();
	filename_ += ".graph";
    std::transform( filename_.begin(), filename_.end(), filename_.begin(),
        tolower );
}

/**
 *  Load a station graph.
 *
 *  @param  mapping The transform to use on the nodes.
 *
 *	@return	bool	Whether or not the load was successful.
 */
bool StationGraph::load(const Matrix& mapping)
{
	BW_GUARD;
	IF_NOT_MF_ASSERT_DEV( filename_ != "" )
	{
		return false;
	}

	pSect_ = BWResource::openSection( filename_, true );
	if (!pSect_)
	{
		ERROR_MSG( "Could not open file %s for station graph\n",
					filename_.c_str() );
		return false;
	}

	DataSection::iterator it = pSect_->begin();
	while (it != pSect_->end())
	{
		DataSectionPtr pNodeSect = *it++;
		StationGraph::Node n;
		n.load( pNodeSect, mapping );
		this->addNode(n);
	}

	return true;
}


//-----------------------------------------------------------------------------
//	Section : StationGraph::Node
//-----------------------------------------------------------------------------

/**
 *	Constructor.
 */
StationGraph::Node::Node():
	worldPosition_( 0,0,0 ),
	id_( UniqueID::zero() )
{
}


/**
 *	This method loads a station graph node given a datasection.
 *
 *  @param pSect        The data section to load the node from.
 *  @param mapping      The transform to apply to the node.
 */
bool StationGraph::Node::load( DataSectionPtr pSect, const Matrix& mapping )
{
	BW_GUARD;
	Vector3 wPos = pSect->readVector3( "worldPosition", this->worldPosition_ );
	this->worldPosition_ = mapping.applyPoint(wPos);
	BW::string name = stripNodeSectionName(pSect->sectionName());
	this->id_ = UniqueID( name );

    this->userString_ = pSect->readString( "userString", "");

	BW::vector<DataSectionPtr> pLinkSects;
	pSect->openSections( "link", pLinkSects );
	for (uint32 i=0; i<pLinkSects.size(); i++)
	{
		DataSectionPtr pLinkSect = pLinkSects[i];
		UniqueID id( pLinkSect->readString( "to" ) );
		this->addLink(id);
	}
	return true;
}


#ifdef EDITOR_ENABLED

/**
 *	This method saves a station graph node into the .graph file.  This is
 *	separate to the ChunkStationNode::edSave method, which saves the node
 *	into a .chunk file.  The requirements for each file are different.
 *
 *  @param pSect        The DataSection to save to.
 *  @param bool         True if the saving was ok.
 */
bool StationGraph::Node::save( DataSectionPtr pSect )
{
	BW_GUARD;
	pSect->writeVector3( "worldPosition", this->worldPosition_ );
    pSect->writeString( "userString", this->userString_ );
	for (uint32 i=0; i<links_.size(); i++)
	{
		DataSectionPtr pLink = pSect->newSection( "link" );
		pLink->writeString( "to", links_[i].toString() );
	}
	return true;
}

#endif


/**
 *	This method returns whether or not this node has a traversable link
 *	to the node represented by the given ID.  Station Graph nodes only
 *	store traversable links.
 *
 *  @param nodeId           The id of the node to test traversability against.
 *
 *  @return                 True if you can go from this node to the node with
 *                          id nodeId.
 */
bool StationGraph::Node::hasTraversableLinkTo( const UniqueID& nodeId ) const
{
	BW_GUARD;
	for (size_t i = 0; i<links_.size(); i++)
    {
		if (links_[i] == nodeId)
			return true;
    }
	return false;
}


/**
 *  Adds a link from this node to the node with id 'id'.
 *
 *  @param id               The id of the node that this node can traverse
 *                          to.
 */
void StationGraph::Node::addLink( const UniqueID& id )
{
	BW_GUARD;
	if (!this->hasTraversableLinkTo(id))
		links_.push_back(UniqueID(id));
}


/**
 *  Remove a link from this node to the node with the given id.
 *
 *  @param id               The id of the node to remove the link.
 */
void StationGraph::Node::delLink( const UniqueID& to )
{
	BW_GUARD;
	BW::vector<UniqueID>::iterator it = links_.begin();
	BW::vector<UniqueID>::iterator end = links_.end();
	while (it != end)
	{
		if (*it == to)
		{
			links_.erase(it);
			return;
		}

		it++;
	}
}


/**
 *  This function returns a list of links from the node.
 *
 *  @return                 The list of links that the node goes to.
 */
BW::vector<UniqueID>& StationGraph::Node::links()
{
    return links_;
}

BW_END_NAMESPACE

// station_graph.cpp
