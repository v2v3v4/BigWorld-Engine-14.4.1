#ifndef AVATAR_DROP_FILTER_HPP
#define AVATAR_DROP_FILTER_HPP

#include "avatar_filter.hpp"


BW_BEGIN_NAMESPACE

class Entity;
class PyAvatarDropFilter;

/**
 *	This is a specialised AvatarFilter that places its owner on the ground.
 *	It is intended for use with  navigating entities which have a tendency
 *	to hover above the ground as they follow the navigation mesh. \n\n
 *	
 *	@note	Currently the dropping is performed on input rather than output
 *			because it is less frequent and so cheaper. However this does
 *			mean that entities will clip through hills during periods of sparse
 *			input.
 */
class AvatarDropFilter : public AvatarFilter
{
public:
	AvatarDropFilter( PyAvatarDropFilter * pOwner );
	virtual ~AvatarDropFilter();

	static void drawDebugStuff();

	const Vector3 & groundNormal() const;

	bool alignToGround() const { return alignToGround_; }
	void alignToGround( bool newValue ) { alignToGround_ = newValue; }

protected:
	// Override from AvatarFilter
	virtual void onEntityPositionUpdated( Entity & entity );

	// Override from MovementFilter
	virtual bool tryCopyState( const MovementFilter & rOtherFilter );

private:
	bool alignToGround_;
	Vector3 groundNormal_;
};

BW_END_NAMESPACE

#endif // AVATAR_DROP_FILTER_HPP
