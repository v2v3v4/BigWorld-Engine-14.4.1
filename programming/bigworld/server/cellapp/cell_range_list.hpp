#ifndef CELL_RANGE_LIST_HPP
#define CELL_RANGE_LIST_HPP

#include "range_list_node.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class implements a range list.
 */
class RangeList
{
public:
	RangeList();

	void add( RangeListNode * pNode );

	// For debugging
	bool isSorted() const;
	void debugDump();

	const RangeListNode * pFirstNode() const	{ return &first_; }
	const RangeListNode * pLastNode() const		{ return &last_; }

	RangeListNode * pFirstNode() { return &first_; }
	RangeListNode * pLastNode()	{ return &last_; }

private:
	RangeListTerminator first_;
	RangeListTerminator last_;
};


#ifdef CODE_INLINE
#include "cell_range_list.ipp"
#endif

BW_END_NAMESPACE

#endif // CELL_RANGE_LIST_HPP
