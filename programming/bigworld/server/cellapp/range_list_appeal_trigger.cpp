#include "script/first_include.hpp"

#include "range_list_appeal_trigger.hpp"
#include "entity.hpp"

BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: RangeListAppealTrigger
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
RangeListAppealTrigger::RangeListAppealTrigger(
		EntityRangeListNode * pEntityNode, float range ) :
	RangeTrigger( pEntityNode, range,
			RangeListNode::FLAG_NO_TRIGGERS,
			RangeListNode::FLAG_NO_TRIGGERS,
			RangeListNode::FLAG_UPPER_AOI_TRIGGER,
			RangeListNode::FLAG_LOWER_AOI_TRIGGER )
{
}


BW::string RangeListAppealTrigger::debugString() const
{
	char buf[256];
	Entity * pEntity = this->pEntity();
	bw_snprintf( buf, sizeof(buf), "RangeListAppealTrigger of entity %u",
		pEntity ? pEntity->id() : 0 );

	return buf;
}

BW_END_NAMESPACE

// range_list_appeal_trigger.cpp

