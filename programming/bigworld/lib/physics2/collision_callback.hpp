#ifndef COLLISION_CALLBACK_HPP
#define COLLISION_CALLBACK_HPP

#include "worldtri.hpp"

BW_BEGIN_NAMESPACE

class CollisionObstacle;

const int COLLIDE_STOP = 0;
const int COLLIDE_BEFORE = 1;
const int COLLIDE_AFTER = 2;
const int COLLIDE_NOT_IMPLEMENTED = 4;
const int COLLIDE_ALL = COLLIDE_BEFORE | COLLIDE_AFTER;

/**
 *	This class contains the function that is called when a collision with an
 *	obstacle occurs
 */
class CollisionCallback
{
public:
	/**
	 *	Destructor
	 */
	virtual ~CollisionCallback() 
	{

	};

	int visit( const CollisionObstacle & obstacle,
		const WorldTriangle & triangle, float dist )
	{

#ifdef MF_SERVER
		int result = this->doVisit( obstacle, triangle, dist );
		if (result == COLLIDE_NOT_IMPLEMENTED)
		{
			return this->doVisit_deprecated( obstacle, triangle, dist );
		}
		return result;
#else
		return (*this)( obstacle, triangle, dist );
#endif
	}

	static CollisionCallback s_default;

#ifndef MF_SERVER
	virtual bool useHighestAvailableLod() { return false; }
#endif

protected:

	int doVisit_deprecated( const CollisionObstacle & obstacle,
		const WorldTriangle & triangle, float dist )
	{
		// Deprecated collide
		if (triangle.flags() & TRIANGLE_WATER)
		{
			return COLLIDE_ALL;
		}
		else
		{
			return (*this)( obstacle, triangle, dist );
		}
	}

	/**
	 *	User-defined method to check a collision.
	 *
	 *	@param	obstacle	the obstacle contains the triangle that collides the ray
	 *	@param	triangle	the world triangle that collides the ray
	 *	@param	dist	the distance between the collision point and the source of the ray
	 *	@return - COLLIDE_BEFORE for interested in collisions before this,
	 *			- COLLIDE_AFTER for interested in collisions after it, or
	 *			- COLLIDE_ALL for interested in collisions before and after
	 *			- COLLIDE_STOP for done
	 *
	 *	@note The dist returned from the collide function is whatever
	 *	was passed into the last callback call, or -1 if there were none.
	 *
	 *	@note The WorldTriangle is in the co-ordinate system of the
	 *	CollisionObstacle, and only exists for the duration of the call.
	 */
	virtual int doVisit( const CollisionObstacle & obstacle,
		const WorldTriangle & triangle, float dist )
	{
		return COLLIDE_NOT_IMPLEMENTED;
	}

	/**
	 *  Deprecated User-defined method to check a collision
	 */
	virtual int operator()( const CollisionObstacle & obstacle,
		const WorldTriangle & triangle, float dist )
	{
		return COLLIDE_STOP;
	}
};


/**
 *	This class finds the first collision with an obstacle
 */
class ClosestObstacle : public CollisionCallback
{
public:
	/**
	 *	the overridden operator()
	 */
	virtual int operator()( const CollisionObstacle & /*obstacle*/,
		const WorldTriangle & /*triangle*/, float /*dist*/ )
	{
		return COLLIDE_BEFORE;
	}

	static ClosestObstacle s_default;
};


/**
 *  This class finds the first collision with an obstacle, optionally only
 *  finding collisions in the same direction as the search ray. 
 */
class ClosestObstacleEx : public CollisionCallback
{
public:
	/**
	 *	Constructor
	 *	@param	oneSided	if it is true, we'll not collide with back faces
	 *	@param	rayDir	the direction of ray
	 */
    ClosestObstacleEx(bool oneSided, const Vector3 & rayDir)
    :
    oneSided_(oneSided),
    rayDir_(rayDir)
    {
        rayDir_.normalise();
    }

	/**
	 *	The overridden operator()
	 */
	virtual int operator()( const CollisionObstacle & /*obstacle*/,
		const WorldTriangle &triangle, float /*dist*/ )
	{
        // Two-sided collisions case:
        if (!oneSided_)
        {
            return COLLIDE_BEFORE; 
        }
        // One sided collisions case:
        else
        {
            // If the normal of the triangle and the ray's direction are the
            // same then this this is not a collision.
            Vector3 normal = triangle.normal();
            normal.normalise();
            if (normal.dotProduct(rayDir_) > 0)
                return COLLIDE_ALL; 
            else
		        return COLLIDE_BEFORE;
        }
	}

private:
    bool        oneSided_;
    Vector3     rayDir_;
};


/**
 *	This class contains the state of a chunk space collision, or at least
 *	references to it, that is passed to a ChunkSpaceObstacle's
 *	collide method. See CSCV::visit for for examples of their use.
 */
class CollisionState
{
public:
	/**
	 *	Constructor
	 *	@param	cc	user-supplied collision callback object
	 */
	CollisionState( CollisionCallback & cc ) :
		cc_( cc ),
		onlyLess_( false ),
		onlyMore_( false ),
		sTravel_( 0.f ),
		eTravel_( 0.f ),
		dist_( -1.0f )
	{ }

public:
	CollisionCallback	& cc_;	// (RW) user-supplied collision callback object
	bool onlyLess_;				// (RW) only interested in nearer than dist_
	bool onlyMore_;				// (RW) only interested in further than dist_
	float sTravel_;				// (RO) dist along line at source
	float eTravel_;				// (RO) dist along line at extent
	float dist_;				// (RW) last value passed to cc_
};

BW_END_NAMESPACE

#endif // COLLISION_CALLBACK_HPP
