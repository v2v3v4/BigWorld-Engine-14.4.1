#ifndef AVATAR_FILTER_HPP
#define AVATAR_FILTER_HPP

#include "filter.hpp"

#include "cstdmf/list_node.hpp"
#include "connection/avatar_filter_helper.hpp"
#include "network/basictypes.hpp"


BW_BEGIN_NAMESPACE

class AvatarFilterCallback;
class Entity;
class PyAvatarFilter;

/**
 *	This is the standard filter to avatar like entities. It provides smooth
 *	movement and support for correctly timed python callbacks.
 *	The AvatarFilter works by storing a history of inputs rather than just
 *	one and then offsetting time so that requested outputs fall inside the
 *	period represented in the history.
 */
class AvatarFilter : public Filter
{
public:
	static const uint NUM_STORED_INPUTS = AvatarFilterHelper::NUM_STORED_INPUTS;

	AvatarFilter( PyAvatarFilter * pOwner );
	~AvatarFilter();

	float latency() const { return helper_.latency(); }
	void latency( float v ) { helper_.latency( v ); }

	void addCallback( AvatarFilterCallback * pCallback );

	// Useful for visualising StoredInputs
	const AvatarFilterHelper::StoredInput & getStoredInput( uint index ) const
	{
		return helper_.getStoredInput( index );
	}

	// Overrides from MovementFilter
	void reset( double time );

	virtual void input(	double time,
						SpaceID spaceID,
						EntityID vehicleID,
						const Position3D & pos,
						const Vector3 & posError,
						const Direction3D & dir );

	virtual void output( double time, MovementFilterTarget & target );

	bool getLastInput(	double & time,
						SpaceID & spaceID,
						EntityID & vehicleID,
						Position3D & pos,
						Vector3 & posError,
						Direction3D & dir ) const;

protected:
	// Overrides from MovementFilter
	virtual bool tryCopyState( const MovementFilter & rOtherFilter );

private:
	/**
	 *	This method gives subclasses a chance to override the position set by
	 *	AvatarFilter::output()
	 */
	virtual void onEntityPositionUpdated( Entity & entity ) {}

	AvatarFilterHelper helper_;

	ListNode callbackListHead_;
};

BW_END_NAMESPACE

#endif // AVATAR_FILTER_HPP
