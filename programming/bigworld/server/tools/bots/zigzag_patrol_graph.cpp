#include "script/first_include.hpp"		// See http://docs.python.org/api/includes.html

#include "zigzag_patrol_graph.hpp"
#include "cstdmf/bw_string.hpp"
using namespace std;

#include "cstdmf/debug.hpp"

DECLARE_DEBUG_COMPONENT2( "Bots", 100 );


BW_BEGIN_NAMESPACE

namespace ZigzagPatrol
{
const int ZigzagGraphTraverser::DEFAULT_CORRIDOR_WIDTH = 50;

static float frand( float f ) { return f * (float)rand()/RAND_MAX; }

ZigzagGraphTraverser::ZigzagGraphTraverser( const Graph &graph,
											float &speed,
											Vector3 &startPosition,
											float corridorWidth ):
	GraphTraverser( graph, speed, startPosition, true, true ),
	corridorWidth_( corridorWidth )
{
	// The initial source position is the start position
	sourcePos_ = startPosition;

	// The corridor width is the 'tunnel' between the source and destination
	// nodes that the bot will stay within
// 		corridorWidth_ = (destinationPos_ - sourcePos_).length() / 8.0;

	// Set up first target
	setZigzagPos(startPosition);
}

// Based on the current position and the destination position, will choose a
// new location to move towards
void ZigzagGraphTraverser::setZigzagPos( const Vector3 &currPos )
{
	// Path between nodes and currentPos
	Vector3 a( sourcePos_ ), b( destinationPos_ ), p( currPos );

	// Calculate closest point to position on line between a and b
	float x,z;
	Vector3 poi;
	if (!almostEqual( b.x, a.x ))
	{
		float m = ( b.z - a.z )/( b.x - a.x );
		x = ( p.x + m*p.z + m*m*a.x - m*a.z ) / (m*m + 1);
		z = m * (x - a.x) + a.z;
		poi = Vector3( x, currPos.y, z );
	}

	// This bit prevents divide by zero in the above block
	else
		poi = Vector3( a.x, currPos.y, p.z );


	// If the distance from the poi to the center of the destination node is
	// less than the corridor width, then set the zigzagPos to be the
	// destination _position_
	if ((destinationPos_ - poi).length() < 2 * corridorWidth_)
	{
		zigzagPos_ = destinationPos_;
		return;
	}

	// Unit vector on edge
	Vector3 along = b - a; along.normalise();

	// Unit vector from curr pos towards edge
	Vector3 across = poi - p; across.normalise();

	// Somewhere on the other side of the edge and forward
	float alongAmt = 2 * frand( corridorWidth_ );
	float acrossAmt = frand( corridorWidth_ );
	zigzagPos_ = (poi + along*alongAmt + across*acrossAmt);
}

bool ZigzagGraphTraverser::nextStep( float & speed,
									 float dTime,
									 Vector3 &pos,
									 Direction3D &dir )
{
	// Distance travelled per tick
	float distance = speed * dTime;

	// The destination node
	const Node *pNode = graph_.node( destinationNode_ );

	// Vectors to dest and zzp respectively
	Vector3 destVec = destinationPos_ - pos;
	Vector3 zzVec = zigzagPos_ - pos;

	// If we are already at a node
	if (stayTime_ > 0)
	{
		stayTime_ -= dTime;

		// If our stay is over pick a new dest and zzp
		if (stayTime_ <= 0)
		{
			sourcePos_ = pNode->position();
			destinationNode_ = pNode->nextNode();
			pNode = graph_.node( destinationNode_ );
			destinationPos_ = pNode->generateDestination();
			setZigzagPos(pos);
		}

		// If we're still hanging around but we've arrived at the zzp, pick
		// a new one
		else if (zzVec.length() < distance)
		{
			zigzagPos_ = pNode->generateDestination();
		}
	}

	// If we have approached a node
	else if (destVec.length() < distance)
	{
		stayTime_ = pNode->generateStayTime();
		zigzagPos_ = pNode->generateDestination();
	}

	// If we are at a zzp
	else if (zzVec.length() < distance)
	{
		setZigzagPos(pos);
	}

	// Move!
	zzVec = zigzagPos_ - pos;
	pos += zzVec * (distance / zzVec.length());
	dir.yaw = zzVec.yaw();

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
class ZigzagPatrolFactory : public MovementFactory
{
public:
	ZigzagPatrolFactory() : MovementFactory( "ZigzagPatrol" )
	{
	}
	virtual ~ZigzagPatrolFactory() {}

	/**
	 *	This method returns a patrol movement controller.
	 */
	MovementController *create( const BW::string & data, float & speed,
								Vector3 & position )
	{
		BW::string graphName;
		float corridorWidth = -1;

		uint argpos = data.find_first_of( ',' );
		if (argpos < data.size())
		{
			graphName = data.substr( 0, argpos );
			BW::string arg2 = data.substr( argpos+1 );
			corridorWidth = atof( arg2.c_str() );
		}
		else
			graphName = data;

		Graph * pGraph = graphs_.getGraph( graphName, 1.f, 1.f );

		if (!pGraph)
		{
			PyErr_Format( PyExc_IOError, "Unable to load %s",
						  graphName.c_str() );
			return NULL;
		}
		else if (corridorWidth > 0)
			return new ZigzagGraphTraverser( *pGraph, speed, position,
											 corridorWidth );
		else
			return new ZigzagGraphTraverser( *pGraph, speed, position );
	}

private:
	Graphs graphs_;
};

ZigzagPatrolFactory s_patrolFactory;

} // anon namespace
} // ZigzagPatrol

BW_END_NAMESPACE

// zigzag_patrol_graph.cpp

