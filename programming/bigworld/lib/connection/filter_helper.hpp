#ifndef FILTER_HELPER_HPP
#define FILTER_HELPER_HPP

#include "network/basictypes.hpp"

BW_BEGIN_NAMESPACE

class MovementFilterTarget;
class FilterEnvironment;


/**
 *	This class is used to implement a filter.
 */
class FilterHelper
{
public:
	virtual ~FilterHelper() {}

	virtual void reset( double time ) = 0;

	virtual void input(	double time,
			SpaceID spaceID, EntityID vehicleID,
			const Position3D & position, const Vector3 & positionError,
			const Direction3D & dir ) = 0;

	virtual double output( double time, SpaceID & rSpaceID,
			EntityID & rVehicleID, Position3D & rPosition,
			Vector3 & rVelocity, Direction3D & rDirection ) = 0;

	virtual bool getLastInput( double & time, SpaceID & spaceID,
			EntityID & vehicleID, Position3D & position,
			Vector3 & positionError, Direction3D & direction ) const
	{
		return false;
	}


protected:
	FilterHelper( FilterEnvironment & filterEnvironment );

	FilterEnvironment & environment() { return *pEnvironment_; }

	bool filterDropPoint( SpaceID spaceID,
			const Position3D & fall,
			Position3D & land,
			Vector3 * pGroundNormal = NULL );

	bool resolveOnGroundPosition( Position3D & position, 
			bool & isOnGround );

	void transformIntoCommon( SpaceID spaceID, EntityID vehicleID,
			Position3D & position, Direction3D & direction );

	void transformFromCommon( SpaceID spaceID, EntityID vehicleID,
			Position3D & position, Direction3D & direction );

	FilterEnvironment * pEnvironment_;
};

BW_END_NAMESPACE

#endif // FILTER_HELPER_HPP

