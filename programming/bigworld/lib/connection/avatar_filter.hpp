#ifndef AVATAR_FILTER_HPP
#define AVATAR_FILTER_HPP

#include "avatar_filter_helper.hpp"
#include "movement_filter.hpp"

#include "bwentity/bwentity_api.hpp"
#include "cstdmf/bw_safe_allocatable.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class implements the default avatar filter. It uses AvatarFilterHelper
 *	to implement its functionality.
 */
class AvatarFilter : public MovementFilter, public SafeAllocatable
{
public:
	BWENTITY_API AvatarFilter( FilterEnvironment & filterEnvironment );
	virtual BWENTITY_API ~AvatarFilter();

	// Overrides from MovementFilter
	virtual void reset( double time );

	virtual void input( double time, SpaceID spaceID, EntityID vehicleID, 
		const Position3D & pos, const Vector3 & posError,
		const Direction3D & dir );

	virtual void output( double time, MovementFilterTarget & target );

	virtual bool getLastInput( double & time, SpaceID & spaceID, 
		EntityID & vehicleID, Position3D & pos, Vector3 & posError,
		Direction3D & dir ) const;

private:
	// Overrides from MovementFilter
	virtual bool tryCopyState( const MovementFilter & rOtherFilter );

	AvatarFilterHelper helper_;
};

BW_END_NAMESPACE

#endif // AVATAR_FILTER_HPP
