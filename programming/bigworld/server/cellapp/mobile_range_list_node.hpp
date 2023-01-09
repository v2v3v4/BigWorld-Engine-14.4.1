#ifndef MOBILE_RANGE_LIST_NODE_HPP
#define MOBILE_RANGE_LIST_NODE_HPP

#include "range_list_node.hpp"

#include "cstdmf/bw_vector.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class is used as a RangeListNode with arbitrary position and flags
 *	provided by the creator.
 */
class MobileRangeListNode : public RangeListNode
{
public:
	MobileRangeListNode( float x, float z, RangeListFlags wantsFlags,
		RangeListFlags makesFlags,
		RangeListOrder order = RANGE_LIST_ORDER_ENTITY );

	/// RangeListNode overrides
	float x() const;
	float z() const;

	BW::string debugString() const;
	
	/// Local methods
	void setPosition( float newX, float newZ );
	void remove();

	void addTrigger( RangeTrigger * pTrigger );
	void modTrigger( RangeTrigger * pTrigger );
	void delTrigger( RangeTrigger * pTrigger );

private:
	float x_;
	float z_;

	typedef BW::vector< RangeTrigger * > Triggers;
	Triggers triggers_;

};

BW_END_NAMESPACE

#endif // MOBILE_RANGE_LIST_NODE_HPP
