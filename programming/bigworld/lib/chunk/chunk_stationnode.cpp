#include "pch.hpp"
#include "chunk_stationnode.hpp"
#include "chunk.hpp"
#include "station_graph.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/auto_config.hpp"

#include "cstdmf/guard.hpp"


DECLARE_DEBUG_COMPONENT2( "StationNode", 0 )


BW_BEGIN_NAMESPACE

int ChunkStationNode_token;

/**
 *  This is the ChunkStationNode constructor.
 */
ChunkStationNode::ChunkStationNode() :
    ChunkItem(),
    position_( Vector3::zero() ),
    graph_( NULL ),
    moveCnt_( 0 )
{
	BW_GUARD;
	static bool throwedDeprecatedWarning = false;
	if (!throwedDeprecatedWarning)
	{
		const char *deprecatedMsg =
				"The old patrol path system is deprecated. Please switch to "
				"the 'PatrolNode' User Data Object system instead.\n";
#ifdef EDITOR_ENABLED
		// Error for the tools
		ERROR_MSG( deprecatedMsg );
#else
		// Warning for client/server
		WARNING_MSG( "%s", deprecatedMsg );
#endif
		throwedDeprecatedWarning = true;
	}
}


/**
 *  This is the ChunkStationNode destructor.
 */
ChunkStationNode::~ChunkStationNode()
{
	BW_GUARD;	
#ifdef EDITOR_ENABLED
	if (graph_ && graph_->isRegistered( this ))
		graph_->deregisterNode( this );
#endif
}


/**
 *  This gets the id of the node.
 *
 *  @return         The id of the node.
 */
UniqueID ChunkStationNode::id() const	
{ 
    return id_; 
}


/**
 *  This gets the graph the node is in.
 *
 *  @return         The graph of the node.
 */
StationGraph* ChunkStationNode::graph() const
{ 
    return graph_; 
}


/**
 *  This function gets the position of the node.
 *
 *  @return         The position of the node.
 */
Vector3 ChunkStationNode::position() const
{ 
    return position_; 
}


/**
 *  This function gets the user string associated with the node.
 *
 *  @return         The user string of the node.
 */
BW::string const &ChunkStationNode::userString() const 
{ 
    return userString_; 
}


/**
 *  This function sets the id of the node.
 *
 *  @param newid    The new id of the node.
 */
void ChunkStationNode::id(UniqueID const &newid)
{
    id_ = newid;
}


/**
 *  This function sets the graph of the node.
 *
 *  @param g        The new graph of the node.
 */
void ChunkStationNode::graph(StationGraph *g)
{
    graph_ = g;
}


/**
 *  This function sets the position of the node.
 *
 *  @param pos      The new position of the node.
 */
void ChunkStationNode::position(Vector3 const &pos)
{
    position_ = pos;
}


/**
 *  This function sets the user string of the node.
 *
 *  @param str      The new user string of the node.
 */
void ChunkStationNode::userString(BW::string const &str)
{
    userString_ = str;
}


/**
 *  This function returns an iterator representing the first of the links.
 *
 *  @return         An iterator that can iterate over the links.
 */
ChunkStationNode::LinkInfoConstIter ChunkStationNode::beginLinks() const
{
    return preloadLinks_.begin();
}


/**
 *  This function returns an iterator representing the one past the end of the
 *  links.
 *
 *  @return         An iterator that can iterate over the links.
 */
ChunkStationNode::LinkInfoConstIter ChunkStationNode::endLinks() const
{
    return preloadLinks_.end();
}


/**
 *  This function finds the link with the given id.
 *
 *  @return         An iterator to the given link.  If there is no such link 
 *                  then endLinks is returned.
 */
ChunkStationNode::LinkInfoConstIter ChunkStationNode::findLink(UniqueID const &id) const
{
	return preloadLinks_.find(id);
}


/**
 *  This function returns the number of links that the node has.
 *
 *  @return         The number of links to/from the node.
 */
size_t ChunkStationNode::numberLinks() const
{
    return preloadLinks_.size();
}


/**
 *  This function returns true if you can traverse to the other node from
 *  this node.
 *
 *  @param other    The other node.
 *
 *  @return         True if you can traverse from this node to other.  Note 
 *                  that this can still return false if linked but the 
 *                  direction is not from this node to other.
 */
bool ChunkStationNode::canTraverse(UniqueID const &other) const
{
    BW_GUARD;
	if (!isLinkedTo(other))
        return false;
    ChunkStationNode *myself = const_cast<ChunkStationNode *>(this);
    return myself->preloadLinks_[other];
}


/**
 *  This function returns whether this node is linked to another.
 *
 *  @param other    The other node.
 *
 *  @return         True if the nodes are linked in any way.
 */
bool ChunkStationNode::isLinkedTo(UniqueID const &other) const
{
    BW_GUARD;
	ChunkStationNode *myself = const_cast<ChunkStationNode *>(this);
    LinkInfo::iterator it = myself->preloadLinks_.find(other);
    return it != preloadLinks_.end();
}


/**
 *  This is called when the node is moved out of one chunk (and possibly into
 *  another.
 *
 *  @param chunk    The new chunk for the node.  This can be NULL if the node
 *                  is being thrown out of memory.
 */
/*virtual*/ void ChunkStationNode::toss(Chunk *chunk)
{
    BW_GUARD;
	// Remove references to links, i.e. break cyclic shared pointer use:
    if (moveCnt_ == 0)
    {
        if (chunk == NULL)
        {
            links_.clear();
            if (graph() != NULL)
            {
                graph()->deregisterNode(this);
            }
        }
        else if (graph() != NULL)
        {
            graph()->registerNode(this, chunk);
        }
    }

    ChunkItem::toss(chunk);
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
ChunkLinkPtr ChunkStationNode::setLink(ChunkStationNode *other, bool canTraverse)
{
    BW_GUARD;
	if (other == NULL)
        return ChunkLinkPtr();

    preloadLinks_[other->id_] = canTraverse;

    // If there isn't a link, create one:
    bool owns = id_ < other->id_;
    ChunkLinkPtr link = findLink(other);

    if (link == NULL)
    {
        link = createLink();
        if (owns)
        {
            link->startItem(this);
            link->endItem(other);
        }
        else
        {
            link->startItem(other);
            link->endItem(this);
        }
        links_.push_back(link);
        other->links_.push_back(link);
        if (owns)
        {
            if (chunk() != NULL)
                chunk()->addStaticItem(link);           
        }
        else
        {
            if (other->chunk() != NULL)
                other->chunk()->addStaticItem(link);
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


#ifdef EDITOR_ENABLED

/**
 *  This function tests for topological consistency.
 *
 *  @param failureMsg       A failure message if not consistent.
 *
 *  @return                 True if topologically consistent, false otherwise.
 */
/*virtual*/ bool ChunkStationNode::isValid(BW::string &failureMsg) const
{
    BW_GUARD;
	if (id() == UniqueID())
    {
        failureMsg = LocaliseUTF8(L"CHUNK/INHABITANTS/CHUNK_STATIONNODE/INVALID_ID");
        return false;
    }

    if (graph() == NULL)
    {
        failureMsg = LocaliseUTF8(L"CHUNK/INHABITANTS/CHUNK_STATIONNODE/NOT_CONNECTED_TO_GRAPH");
        return false;
    }

    if (links_.size() != preloadLinks_.size())
    {
        failureMsg = LocaliseUTF8(L"CHUNK/INHABITANTS/CHUNK_STATIONNODE/LINK_SIZE_UNEQUAL");
        return false;
    }

    for (size_t i = 0; i < links_.size(); ++i)
    {
        ChunkLinkPtr link = links_[i];
        if (!link->isValid(failureMsg))
            return false;
        ChunkStationNode *myself = (ChunkStationNode *)link->startItem().getObject();
        ChunkStationNode *other  = (ChunkStationNode *)link->endItem  ().getObject();
        if (myself != this && other != this)
        {
            failureMsg = LocaliseUTF8(L"CHUNK/INHABITANTS/CHUNK_STATIONNODE/NOT_MATCH_END_ITEM");
            return false;
        }
        if (myself != this)
            std::swap(myself, other);

        LinkInfoConstIter it1 = myself->preloadLinks_.find(other->id());
        LinkInfoConstIter it2 = other->preloadLinks_.find(id());
        if (it1 == preloadLinks_.end() || it2 == other->preloadLinks_.end())
        {
            failureMsg = LocaliseUTF8(L"CHUNK/INHABITANTS/CHUNK_STATIONNODE/PRELOAD_IS_MISSING_LINK");
            return false;
        }

        bool owns = id() < other->id();
        if (owns)
        {
            if 
            (
                myself->preloadLinks_[other->id()] 
                && 
                (link->direction() & ChunkLink::DIR_START_END) == 0
            )
            {
                failureMsg = LocaliseUTF8(L"CHUNK/INHABITANTS/CHUNK_STATIONNODE/LINK_PRELOAD_WRONG_DIRECTION");
                return false;
            }
            if 
            (
                !myself->preloadLinks_[other->id()] 
                && 
                (link->direction() & ChunkLink::DIR_START_END) != 0
            )
            {
                failureMsg = LocaliseUTF8(L"CHUNK/INHABITANTS/CHUNK_STATIONNODE/LINK_PRELOAD_WRONG_DIRECTION");
                return false;
            }
        }
        else
        if (owns)
        {
            if 
            (
                myself->preloadLinks_[other->id()] 
                && 
                (link->direction() & ChunkLink::DIR_END_START) == 0
            )
            {
                failureMsg = LocaliseUTF8(L"CHUNK/INHABITANTS/CHUNK_STATIONNODE/LINK_PRELOAD_WRONG_DIRECTION");
                return false;
            }
            if 
            (
                !myself->preloadLinks_[other->id()] 
                && 
                (link->direction() & ChunkLink::DIR_END_START) != 0
            )
            {
                failureMsg = LocaliseUTF8(L"CHUNK/INHABITANTS/CHUNK_STATIONNODE/LINK_PRELOAD_WRONG_DIRECTION");
                return false;
            }
        }
    }

    for
    (
        LinkInfoConstIter it = beginLinks();
        it != endLinks();
        ++it
    )
    {
        UniqueID otherID     = it->first;
        bool     canTraverse = it->second;
        bool     ok          = false;
        for (size_t i = 0; i < links_.size() && !ok; ++i)
        {
            ChunkLinkPtr     link = links_[i];
            ChunkStationNode *myself = (ChunkStationNode *)link->startItem().getObject();
            ChunkStationNode *other  = (ChunkStationNode *)link->endItem  ().getObject();
            if (myself != this)
                std::swap(myself, other);
            if (other->id() == otherID)
            {
                ok = true;
            }
        }
        if (!ok)
        {
            failureMsg = LocaliseUTF8(L"CHUNK/INHABITANTS/CHUNK_STATIONNODE/LINK_IN_PRELOAD_NOT_IN_LINK");
            return false;
        }
    }

    if (graph()->getNode(id()) == NULL)
    {
        failureMsg = LocaliseUTF8(L"CHUNK/INHABITANTS/CHUNK_STATIONNODE/NODE_NOT_REGISTER_TO_GRAPH");
        return false;
    }

    if (chunk() == NULL)
    {
        failureMsg = LocaliseUTF8(L"CHUNK/INHABITANTS/CHUNK_STATIONNODE/NODE_NOT_IN_CHUNK");
        return false;
    }

    return true;
}


/**
 *  This function marks the node as dirty.
 */
/*virtual*/ void ChunkStationNode::makeDirty()
{
}

#endif


/**
 *  This function is used as part of the loading process.  It loads all data
 *  except links.
 *
 *  @param pSection         The section to load from.
 *  @param pChunk           The chunk that the node will be placed into.
 */
bool ChunkStationNode::loadName( DataSectionPtr pSection, Chunk* pChunk)
{
	BW_GUARD;
	BW::string idStr    = pSection->readString( "id" );
	BW::string graphStr = pSection->readString( "graph" );

	if (idStr.empty() || graphStr.empty())
		return false;

	// Deregister ourself to support reloading
	if (graph_)
		graph_->deregisterNode( this );

	id_ = UniqueID( idStr );

	graph_ = StationGraph::getGraph( UniqueID( graphStr ) );
	graph_->registerNode( this, pChunk );

	return true;
}


/**
 *  This function is used to create a link.  Derived classes can override this
 *  to provide a more useful link type.
 *
 *  @return         A new link.
 */
/*virtual*/ ChunkLinkPtr ChunkStationNode::createLink() const
{
    return new ChunkLink();
}


#ifdef EDITOR_ENABLED

/**
 *  This function completely unlinks the node from any other node.
 */
void ChunkStationNode::unlink()
{
    BW_GUARD;
	while (links_.size() != 0)
    {
        removeLink(links_.front());
    }
}

#endif


/**
 *  This function unlinks this node from other.
 *
 *  @param other        The node to unlink from.
 */
void ChunkStationNode::removeLink(ChunkStationNode *other)
{
    BW_GUARD;
	if (other == NULL)
        return;

    ChunkLinkPtr link = findLink(other);
    if (link != NULL)
        removeLink(link);
}


/**
 *  This function finds a link to other.
 *
 *  @param other        The node to find the link for.
 * 
 *  @return             The link between the nodes.  This will be NULL if there
 *                      is no link.
 */
ChunkLinkPtr ChunkStationNode::findLink(ChunkStationNode const *other) const
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
            link->startItem() == other
            ||
            link->endItem() == other
        )
        {
            return link;
        }
    }
    return NULL;
}


#ifdef EDITOR_ENABLED

/**
 *  This function removes links between this and other.
 *  
 *  @param other        The node to remove the link with.
 */ 
void ChunkStationNode::removeLink(UniqueID const &other)
{
    ChunkStationNode *otherNode = graph()->getNode(other);
    if (otherNode == NULL)
        return;
    removeLink(otherNode);
    otherNode->removeLink(this);
}

#endif 


/**
 *  This function gets the idx'th link.
 *
 *  @param idx          The index of the link to get.
 *
 *  @return             The index'th link.
 */
ChunkLinkPtr ChunkStationNode::getChunkLink(size_t idx) const
{
    BW_GUARD;
	if (idx >= links_.size())
        return NULL;
    else
        return links_[idx];
}


/**
 *  This is used to flag that the node is being moved.
 */
void ChunkStationNode::beginMove()
{
    ++moveCnt_;
}


/**
 *  This is used to flag that the node has been moved.
 */
void ChunkStationNode::endMove()
{
    --moveCnt_;
}


/**
 *  This is used to make sure that the graph has the latest information about
 *  the node.
 *
 *  @param chunk            The chunk that the node will be in.
 */
void ChunkStationNode::updateRegistration( Chunk *chunk )
{
    BW_GUARD;
	if (graph_ != NULL)
    {
        graph_->deregisterNode(this);
        graph_->registerNode(this, chunk);
    }
}


/**
 *  This removes a link, by setting its start and end to NULL, removing from 
 *  out list of links etc.
 *
 *  @param link         The link to remove.
 */
void ChunkStationNode::removeLink(ChunkLinkPtr link)
{
	BW_GUARD;

	delLink( link );

	ChunkStationNode *other = static_cast<ChunkStationNode *>(
		link->endItem().get() );
	ChunkStationNode *myself = static_cast<ChunkStationNode *>(
		link->startItem().get() );

	if ((other == NULL) && (myself == NULL))
	{
		return;
	}

	if (other == this)
	{
		other = static_cast<ChunkStationNode *>(
			link->startItem().getObject() );
	}
	other->delLink( link );

	const bool owns = (id() < other->id());
	{
		LinkInfo::iterator lit = preloadLinks_.find( other->id() );
		if (lit != preloadLinks_.end())
		{
			preloadLinks_.erase( lit );
		}
	}
	{
		LinkInfo::iterator lit = other->preloadLinks_.find( id() );
		if (lit != other->preloadLinks_.end())
		{
			other->preloadLinks_.erase(lit);
		}
	}
	if (owns)
	{
		chunk()->delStaticItem(link);
	}
	else
	{
		other->chunk()->delStaticItem(link);
	}

	link->startItem( NULL );
	link->endItem( NULL );
}


/**
 *  This removes a link from our internal data structure.
 *
 *  @param link     The link to remove.
 */
void ChunkStationNode::delLink(ChunkLinkPtr link)
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
 *  This loads a ChunkStationNode from a DataSection.
 *
 *  @param pSection     The DataSection to load from.
 *  @param pChunk       The chunk the node will go into.
 *
 *  @return             True for a successful load.
 */
bool ChunkStationNode::load( DataSectionPtr pSection, Chunk* pChunk )
{
	BW_GUARD;
	// Setup id_ and graph_
	if (!loadName( pSection, pChunk ))
		return false;

    chunk(pChunk);

	position_   = pSection->readVector3( "position" );
    userString_ = pSection->readString( "userString" );

	// Read in the links
	BW::vector<DataSectionPtr>	linkSects;
	pSection->openSections( "link", linkSects );
	for (uint i = 0; i < linkSects.size(); ++i)
	{
		DataSectionPtr ds = linkSects[i];

		UniqueID toId    = ds->readString( "to" );
		bool traversable = ds->readBool( "traversable" );

		// We should never have a link to ourself
		IF_NOT_MF_ASSERT_DEV( toId != id_ )
		{
			continue;
		}

		preloadLinks_[toId] = traversable;
	}


    updateRegistration( pChunk );

	return true;
}


/**
 *  Create a ChunkStationNode from the input section and add it to the chunk.
 *
 *  @param pChunk       The chunk to place in.
 *  @param pSection     The section to copy.
 *
 *  @return             True upon success.
 */
ChunkItemFactory::Result ChunkStationNode::create( Chunk* pChunk, DataSectionPtr pSection )
{
	BW_GUARD;
	ChunkStationNode* node = new ChunkStationNode();
	if (!node->load( pSection, pChunk ))
	{
		BW::string err = "Failed to load patrol node";
		if ( node->id() != UniqueID::zero() )
		{
			err += ' ' + node->id().toString();
		}
		bw_safe_delete(node);
		return ChunkItemFactory::Result( NULL, err );
	}
	else
	{
		pChunk->addStaticItem( node );
		return ChunkItemFactory::Result( node );
	}
}


/// Static factory initialiser
ChunkItemFactory ChunkStationNode::factory_( "station", 0, ChunkStationNode::create );

BW_END_NAMESPACE

// chunk_stationnode.cpp
