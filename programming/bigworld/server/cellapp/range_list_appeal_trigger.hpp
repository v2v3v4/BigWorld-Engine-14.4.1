#ifndef RANGE_LIST_APPEAL_TRIGGER_HPP
#define RANGE_LIST_APPEAL_TRIGGER_HPP

#include "range_trigger.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class is used to support "large" entities. That is entities that have
 *	a non-zero AppealRadius. These entities can be seen further than the Area
 *	of Interest of a player.
 */
class RangeListAppealTrigger : public RangeTrigger
{
public:
	RangeListAppealTrigger( EntityRangeListNode * pEntityNode, float range );

	virtual void triggerEnter( Entity & entity ) {}
	virtual void triggerLeave( Entity & entity ) {}
	virtual Entity * pEntity() const { return
		EntityRangeListNode::getEntity( this->pCentralNode() ); }


	virtual BW::string debugString() const;
};

BW_END_NAMESPACE

#endif // RANGE_LIST_APPEAL_TRIGGER_HPP
