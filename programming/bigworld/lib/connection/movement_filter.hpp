#ifndef MOVEMENT_FILTER_HPP
#define MOVEMENT_FILTER_HPP

#include "cstdmf/smartpointer.hpp"
#include "network/basictypes.hpp"
#include "bwentity/bwentity_api.hpp"

BW_BEGIN_NAMESPACE

class MovementFilterTarget;
class FilterEnvironment;


/**
 *	This is the common base class for all filters.
 */
class MovementFilter
{
public:
	/** Destructor. */
	virtual BWENTITY_API ~MovementFilter() {}

	/**
	 *	This method is used to reset the filter to its initial state.
	 *
 	 *	@param time 	Reset the filter with this game time.
 	 */
	virtual void reset( double time ) = 0;

	/**
	 *	This method is used to give an input sample to the filter.
	 *
	 *	@param time 		The game time associated with this sample.
	 *	@param spaceID		The space ID of the update.
	 *	@param vehicleID 	The entity's vehicle status in this sample.
	 * 	@param position		The entity position sample.
	 *	@param positionError	The size of the position error.
	 *	@param direction 	The entity direction sample.
	 */
	virtual void input( double time, SpaceID spaceID, EntityID vehicleID, 
		const Position3D & position, const Vector3 & positionError,
		const Direction3D & direction ) = 0;

	/**
	 *	This method is used to output a position to a MovementFilterTarget
	 *	for the given game time.
	 *
	 *	@param time 	The game time for which a position is output.
	 *	@param target 	The MovementFilterTarget to output to.
	 */
	virtual void output( double time, MovementFilterTarget & target ) = 0;

	/**
	 *	This method returns the last input given to this filter.
	 *
	 *	@param time 	The last sample time.
	 *	@param spaceID 	The last sample's space ID.
	 *	@param vehicleID 	The last sample's vehicle ID.
	 *	@param position		The last sample's position.
	 *	@param positionError 	The last sample's position error.
	 *	@param direction 	The last sample's direction.
 	 */
	virtual bool getLastInput( double & time, SpaceID & spaceID, 
		EntityID & vehicleID, Position3D & position, Vector3 & positionError,
		Direction3D & direction ) const = 0;

	void copyState( const MovementFilter & rOtherFilter );

	// Methods delegated to the FilterEnvironment referenced by environment_.
	bool filterDropPoint( SpaceID spaceID, const Position3D & fall,
		Position3D & land, Vector3 * groundNormal = NULL );

	bool resolveOnGroundPosition( Position3D & position, 
		bool & isOnGround );

	void transformIntoCommon( SpaceID spaceID, EntityID vehicleID,
		Position3D & position, Direction3D & direction );

	void transformFromCommon( SpaceID spaceID, EntityID vehicleID,
		Position3D & position, Direction3D & direction );

protected:
	BWENTITY_API MovementFilter( FilterEnvironment & environment );

	/** This method returns the associated filter environment. */
	FilterEnvironment & environment() const { return environment_; }

private:

	/**
	 *	This method attempts to copy state (but not settings) from the given
	 *	MovementFilter into our own state. It should return true if state was
	 *	copied, and false if not. If it returns false, a fallback state-copy
	 *	will be performed.
	 *
	 *	MovementFilter implementations should override this if they wish to
	 *	copy more state than simply the last position.
	 *
	 *	@param rOtherFilter		The MovementFilter to copy state from
	 *
	 *	@return true if this MovementFilter's state is now updated, or false
	 *			if the state was not changed.
	 */
	virtual bool tryCopyState( const MovementFilter & rOtherFilter )
		{ return false; }

	FilterEnvironment & environment_;
};

BW_END_NAMESPACE

#endif // MOVEMENT_FILTER_HPP
