#ifndef MOVEMENT_FILTER_TARGET_HPP
#define MOVEMENT_FILTER_TARGET_HPP

#include "cstdmf/debug.hpp"
#include "network/basictypes.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This interface is used by MovementFilter.
 */
class MovementFilterTarget
{
public:
	/**
	 *	This method updates the MovementFilterTarget's "current" position,
	 *	relative to the given space and vehicle.
	 *
	 *	@param	spaceID		The SpaceID of the space the target currently
	 *						inhabits.
	 *	@param	vehicleID	The VehicleID of a vehicle the target's
	 *						co-ordinates are relative to, or NULL_ENTITY_ID
	 *						if the target is not relative to a vehicle.
	 *	@param	position	The Position3D of the target relative to the space
	 *						and vehicle if any.
	 *	@param	direction	The Direction3D of the target's facing relative to
	 *						the space and vehicle if any.
	 */
	virtual void setSpaceVehiclePositionAndDirection( const SpaceID & spaceID,
		const EntityID & vehicleID, const Position3D & position,
		const Direction3D & direction ) = 0;

protected:
	MovementFilterTarget() {}
	~MovementFilterTarget() {}
};

BW_END_NAMESPACE

#endif // MOVEMENT_FILTER_TARGET_HPP

