#ifndef SPACE_ROMP_COLLIDER_HPP
#define SPACE_ROMP_COLLIDER_HPP

#include "forward_declarations.hpp"

#include <romp/romp_collider.hpp>

namespace BW
{

/**
 *	This class implements the standard ground specifier for the
 *	grounded source action of a particle system.
 */
class SpaceRompCollider : public RompCollider
{
public:
	/**
	 *	Constructor
	 */
	SpaceRompCollider() { }

    virtual float ground( const Vector3 &pos, float dropDistance, 
		bool onesided );
	virtual float ground( const Vector3 & pos );
	virtual float collide( const Vector3 &start, const Vector3& end, 
		WorldTriangle& result );

	static RompColliderPtr create( FilterType filter );
};


/**
 *	This class implements the standard ground specifier for the
 *	grounded source action of a particle system.
 *	It uses terrain as ground
 */
class SpaceRompTerrainCollider : public SpaceRompCollider
{
public:
	/**
	 *	Constructor
	 */
	SpaceRompTerrainCollider() { }

	virtual float ground( const Vector3 & pos );
};


} // namespace BW

#endif // SPACE_ROMP_COLLIDER_HPP