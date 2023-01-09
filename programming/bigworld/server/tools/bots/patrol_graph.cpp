#include "script/first_include.hpp"		// See http://docs.python.org/api/includes.html

#include "patrol_graph.hpp"

#include "cstdmf/debug.hpp"

#include "resmgr/bwresource.hpp"
#include "resmgr/datasection.hpp"
#include "cstdmf/debug.hpp"

DECLARE_DEBUG_COMPONENT2( "Bots", 0 );


BW_BEGIN_NAMESPACE

namespace Patrol
{

// -----------------------------------------------------------------------------
// Section: Node
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
Node::Node()
{
}


/**
 *	This method initialises this node.
 */
bool Node::init1( int index, DataSection * pSection,
		const Properties & defaultProps )
{
	props_ = defaultProps;

	if (!pSection) return false;

	id_ = index;

	position_ = pSection->readVector3( "pos" );
	name_ = pSection->readString( "name" );

	props_.init( pSection );

	if (name_.empty())
	{
		char buf[10];
#ifdef _WIN32
		_snprintf( buf, 10, "%d", index );
#else
		snprintf( buf, 10, "%d", index );
#endif
		name_ = buf;
	}

	return true;
}

bool Node::init2( const NodeMap & map, DataSection * pSection )
{
	int numEdges = 0;
	DataSectionPtr pEdges = pSection->openSection( "edges" );

	if (pEdges)
	{
		numEdges = pEdges->countChildren();
	}
	else
	{
		WARNING_MSG( "Graph::init2: No edges section\n" );
	}

	toNodes_.resize( numEdges );

	for (int i = 0; i < numEdges; ++i)
	{
		BW::string dstName = pEdges->openChild( i )->asString();
		NodeMap::const_iterator iter = map.find( dstName );

		if (iter == map.end())
		{
			ERROR_MSG( "Graph::init2: Could not find %s for %s\n",
					dstName.c_str(), name_.c_str() );
			return false;
		}

		toNodes_[i] = iter->second;
	}

	return true;
}


/**
 *	This method returns the square of the distance to the input position.
 */
float Node::flatDistSqrTo( const Vector3 & pos ) const
{
	float x = position_.x - pos.x;
	float z = position_.z - pos.z;
	return x*x + z*z;
}


/**
 *	This method returns the distance to the input position.
 */
float Node::flatDistTo( const Vector3 & pos ) const
{
	return sqrtf( this->flatDistSqrTo( pos ) );
}


/**
 *	This method returns the next node to travel to from this one.
 */
int Node::nextNode() const
{
	if (!toNodes_.empty())
	{
		return toNodes_[ rand() % toNodes_.size() ];
	}
	else
	{
		return id_;
	}
}


/**
 *	Randomise the input position to be on the path from this node to one
 *	of our adjacencies, and return which adjacency we selected.
 */
int Node::randomisePosition( Vector3 & pos, const Graph & graph ) const
{
	if (toNodes_.empty())
	{
		pos = position_;
	   	return id_;
	}

	int nextNode = toNodes_[ rand() % toNodes_.size() ];
	pos = position_;
	pos += (graph.node( nextNode )->position_ - position_) *
		(float(rand())/float(RAND_MAX));
	return nextNode;
}


/**
 *	This method returns a random destination position around this node.
 */
Vector3 Node::generateDestination() const
{
	float radius = props_.radius();

	return Vector3( randInRange( position_.x - radius, position_.x + radius ),
			position_.y,
			randInRange( position_.z - radius, position_.z + radius ) );
}


/**
 *	This method returns a random wait time associated with the node.
 */
float Node::generateStayTime() const
{
	return randInRange( props_.minStay(), props_.maxStay() );
}


/**
 *	This method returns a random speed associated with the node.
 */
float Node::generateSpeed( float ospeed ) const
{
	if (props_.minSpeed() <= 0.f || props_.maxSpeed() <= 0.f) return ospeed;

	return randInRange( props_.minSpeed(), props_.maxSpeed() );
}


/**
 *	This method initialises the properties from a data section.
 */
void Node::Properties::init( DataSection * pSection )
{
	if (!pSection) return;

	minStay_ = pSection->readFloat( "minStay", minStay_ );
	maxStay_ = pSection->readFloat( "maxStay", maxStay_ );
	radius_ = pSection->readFloat( "radius", radius_ );
	minSpeed_ = pSection->readFloat( "minSpeed", minSpeed_ );
	maxSpeed_ = pSection->readFloat( "maxSpeed", maxSpeed_ );
}


// -----------------------------------------------------------------------------
// Section: Graph
// -----------------------------------------------------------------------------

/**
 * Constructor.
 */
Graph::Graph()
{
}


/**
 *	This method initialises this patrol graph.
 */
bool Graph::init( DataSection * pSection, float scalePos, float scaleSpeed )
{
	DataSectionPtr pNodes = pSection->findChild( "nodes" );

	if (!pNodes)
	{
		ERROR_MSG( "Graph::init: No nodes section\n" );
		return false;
	}

	Node::Properties nodeDefaults;
	nodeDefaults.init( pSection->findChild( "nodeDefaults" ).get() );

	int numNodes = pNodes->countChildren();
	nodes_.resize( numNodes );

	Node::NodeMap nodeMap;

	for (int i = 0; i < numNodes; ++i)
	{
		DataSectionPtr pNode = pNodes->openChild( i );

		if (!nodes_[i].init1( i, pNode.get(), nodeDefaults ))
		{
			ERROR_MSG( "Graph::init: Failed to initialise node %d\n", i );
			return false;
		}
		nodes_[i].position_ *= scalePos;
		nodes_[i].props_.radius_ *= scalePos;
		nodes_[i].props_.minSpeed_ *= scaleSpeed;
		nodes_[i].props_.maxSpeed_ *= scaleSpeed;

		if (!nodeMap.insert( std::make_pair( nodes_[i].name(), i ) ).second)
		{
			ERROR_MSG( "Graph::init: Node %s is a duplicate\n",
					nodes_[i].name().c_str() );
			return false;
		}
	}

	for (int i = 0; i < numNodes; ++i)
	{
		DataSectionPtr pNode = pNodes->openChild( i );

		if (!nodes_[i].init2( nodeMap, pNode.get() ))
		{
			ERROR_MSG( "Graph::init: Failed to initialise edges for node %d\n",
					i );
			return false;
		}
	}

	return true;
}


/**
 *	This method returns the node that is closest to the input point.
 */
int Graph::findClosestNode( const Vector3 & position ) const
{
	if (nodes_.empty())
		return -1;

	float bestDist = nodes_[0].flatDistSqrTo( position );
	int bestIndex = 0;

	int size = nodes_.size();

	for (int i = 1; i < size; ++i)
	{
		float dist = nodes_[i].flatDistSqrTo( position );

		if (dist < bestDist)
		{
			bestDist = dist;
			bestIndex = i;
		}
	}

	return bestIndex;
}


/**
 *	This method return the node with the input index.
 */
const Node * Graph::node( int index ) const
{
	return &nodes_[ index ];
}


// -----------------------------------------------------------------------------
// Section: Graphs
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
Graphs::Graphs()
{
}


/**
 *	This method returns the graph with the input name.
 */
Graph * Graphs::getGraph( const BW::string & graphName,
	float scalePos, float scaleSpeed )
{
	char dnbuf[1024];
	if (graphName.size() > sizeof(dnbuf)-64) return NULL;

	bw_snprintf( dnbuf, sizeof(dnbuf), "%s,%f,%f",
		graphName.c_str(), scalePos, scaleSpeed );
	BW::string decoratedName = dnbuf;

	Map::iterator iter = graphs_.find( decoratedName );

	if (iter != graphs_.end())
	{
		return &iter->second;
	}

	BWResource::instance().purge( graphName );
	DataSectionPtr pSection = BWResource::openSection( graphName );

	if (!pSection)
	{
		return NULL;
	}

	Graph & graph = graphs_[ decoratedName ];
	if (graph.init( pSection.get(), scalePos, scaleSpeed ))
	{
		return &graph;
	}
	else
	{
		graphs_.erase( decoratedName );
		return NULL;
	}
}


// -----------------------------------------------------------------------------
// Section: GraphTraverser
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
GraphTraverser::GraphTraverser( const Graph & graph, float & speed,
		Vector3 & startPosition, bool randomise, bool snap ) :
	destinationNode_( -1 ),
	stayTime_( -1.f ),
	graph_( graph )
{
	// head for either the closest node or any old node
	if (randomise)
	{
		destinationNode_ = rand() % graph_.size();
	}
	else
	{
		destinationNode_ = graph_.findClosestNode( startPosition );
	}

	const Node * pNode = graph_.node( destinationNode_ );
	// start off either where we were or on the path to a neighbouring node
	if (snap)
	{		// startPosition is modified below:
		destinationNode_ = pNode->randomisePosition( startPosition, graph_ );
		pNode = graph_.node( destinationNode_ );
	}

	destinationPos_ = pNode->generateDestination();
	speed = pNode->generateSpeed( speed );
}


/**
 *	This method finds the next point to move to.
 */
bool GraphTraverser::nextStep( float & speed, float dTime,
		Vector3 & pos, Direction3D & dir )
{
	const Node * pNode = graph_.node( destinationNode_ );

	if (stayTime_ >= 0.f)
	{
		stayTime_ -= dTime;

		if (stayTime_ <= 0.f)
		{
			destinationNode_ = pNode->nextNode();
			pNode = graph_.node( destinationNode_ );
			destinationPos_ = pNode->generateDestination();
			stayTime_ = -1.f;
			speed = pNode->generateSpeed( speed );
		}
	}

	float distance = speed * dTime;

	// destinationNode_ = pNode->moveToward( distance, pos, dir );
	Vector3 dirVec = destinationPos_ - pos;
	float length = dirVec.length();

	// TODO: When is near enough
	if (length < distance)
	{
		// Note: Negative stay time means we are in transit.
		if (stayTime_ < 0.f)
		{
			stayTime_ = pNode->generateStayTime();
		}

		destinationPos_ = pNode->generateDestination();
	}
	else
	{
		pos += (distance/length) * dirVec;
		dir.yaw = dirVec.yaw();
	}

	return true;
}


// -----------------------------------------------------------------------------
// Section: PatrolFactory
// -----------------------------------------------------------------------------

namespace
{
/**
 *	This class is used to create patrol movement controllers.
 */
class PatrolFactory : public MovementFactory
{
public:
	PatrolFactory() : MovementFactory( "Patrol" )
	{
	}
	virtual ~PatrolFactory() {}

	/**
	 *	This method returns a patrol movement controller.
	 */
	MovementController *
		create( const BW::string & data, float & speed, Vector3 & position )
	{
		BW::string graphName;
		bool randomise = false, snap = false;
		float scalePos = 1.f, scaleSpeed = 1.f;

		uint argbeg = 0;
		int argnum = 0;
		do
		{
			uint argend = data.find_first_of( ',', argbeg );
			if (argend >= data.size())
				argend = data.size();
			BW::string argstr = data.substr( argbeg, argend-argbeg );

			if (argnum == 0)
			{
				graphName = argstr;
			}
			else
			{
				if (argstr == "random")
					randomise = true;
				else if (argstr == "snap")
					snap = true;
				else if (argstr.substr( 0, 9 ) == "scalePos=")
					scalePos = atof( argstr.substr( 9 ).c_str() );
				else if (argstr.substr( 0, 11 ) == "scaleSpeed=")
					scaleSpeed = atof( argstr.substr( 11 ).c_str() );
				else
					PyErr_Format( PyExc_ValueError, "PatrolFactory::create: "
						"Unknown argument '%s'\n", argstr.c_str() );
			}
			argbeg = argend+1;
			argnum++;
		} while (argbeg < data.size());

		Graph * pGraph = graphs_.getGraph( graphName, scalePos, scaleSpeed );

		if (pGraph)
		{
			return new GraphTraverser( *pGraph, speed, position, randomise, snap );
		}

		PyErr_Format( PyExc_IOError, "Unable to load graph '%s'", graphName.c_str() );
		return NULL;
	}

private:
	Graphs graphs_;
};

PatrolFactory s_patrolFactory;

} // anon namespace

} // namespace Patrol

BW_END_NAMESPACE

// patrol_graph.cpp
