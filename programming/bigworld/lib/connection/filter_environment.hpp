#ifndef FILTER_ENVIRONMENT_HPP
#define FILTER_ENVIRONMENT_HPP

#include "network/basictypes.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This interface class represents the environment with which a filter 
 *	interacts, handling terrain queries and vehicle transforms, and 
 *	coordinate system checks.
 */
class FilterEnvironment
{
public:

	virtual ~FilterEnvironment() {}

	virtual bool filterDropPoint( SpaceID spaceID,
			const Position3D & fall, Position3D & land,
			Vector3 * pGroundNormal = NULL ) = 0;

	virtual bool resolveOnGroundPosition( Position3D & position, 
		bool & isOnGround ) = 0;

	virtual void transformIntoCommon( SpaceID spaceID, EntityID vehicleID,
		Position3D & position, Direction3D & direction ) = 0;

	virtual void transformFromCommon( SpaceID spaceID, EntityID vehicleID,
		Position3D & position, Direction3D & direction ) = 0;
};

BW_END_NAMESPACE

#endif // FILTER_ENVIRONMENT_HPP

