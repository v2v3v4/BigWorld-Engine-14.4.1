#ifndef ROMP_COLLIDER_HPP
#define ROMP_COLLIDER_HPP

#include "cstdmf/smartpointer.hpp"

BW_BEGIN_NAMESPACE

class Vector3;
class RompCollider;
typedef SmartPointer<RompCollider> RompColliderPtr;

/**
 *	This abstract class is the interface between the particle source's
 *	'grounded' feature and whatever collision scene is implemented.
 *
 *	The ground() method returns the height of the ground beneath the
 *	particle or NO_GROUND_COLLISION if none was found.
 */
class RompCollider : public ReferenceCount
{
public:
	enum FilterType
	{
		EVERYTHING,
		TERRAIN_ONLY
	};

    virtual float ground( const Vector3 &pos, float dropDistance, bool oneSided = false ) = 0;
	virtual float ground( const Vector3 &pos ) = 0;
	virtual float collide( const Vector3 &start, const Vector3& end, class WorldTriangle& result ) = 0;

    static float NO_GROUND_COLLISION;

	static RompColliderPtr createDefault( FilterType filter = EVERYTHING );

	typedef RompColliderPtr (*CreationFunction)( FilterType );
	static void setDefaultCreationFuction( CreationFunction func );

private:
	static CreationFunction s_defaultCreate;
};

BW_END_NAMESPACE

#endif // ROMP_COLLIDER_HPP

