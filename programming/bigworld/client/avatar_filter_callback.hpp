#ifndef AVATAR_FILTER_CALLBACK_HPP
#define AVATAR_FILTER_CALLBACK_HPP

#include "cstdmf/list_node.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class is a virtual base class for callbacks from the AvatarFilter,
 *	 triggered when the filter outputs past a certain time.
 *
 *	The callback receives the difference between the target time and the
 *	actual time, and returns true if the Filter's output should be snapped
 *	to the position at the target time.
 *
 *	Note that after being added to the AvatarFilter, the AvatarFilter owns this
 *	object and will delete it after the callback is triggered or if the Filter
 *	is destroyed before the callback is triggered
 */
class AvatarFilterCallback
{
public:
	AvatarFilterCallback( double targetTime );
	virtual ~AvatarFilterCallback();

	double targetTime() const { return targetTime_; }

	void insertIntoList( ListNode * pListHead );

	bool triggerCallback( double outputTime );

	void removeFromList();

	static AvatarFilterCallback * getFromListNode( ListNode * pListNode )
	{
		return CAST_NODE( pListNode, AvatarFilterCallback, callbackListNode_ );
	}

private:
	/**
	 *	This method is called to indicate the target time has passed, and
	 *	passes in by how much the target time has passed. It should return true
	 *	to cause the AvatarFilter to output as if the current filter time was
	 *	the targetTime, i.e., rolling the clock back on the AvatarFilter for
	 *	this tick only.
	 */
	// For subclasses to override
	virtual bool onCallback( double missedBy ) = 0;

	double targetTime_;

	ListNode callbackListNode_;
};

BW_END_NAMESPACE

#endif // AVATAR_FILTER_CALLBACK_HPP
