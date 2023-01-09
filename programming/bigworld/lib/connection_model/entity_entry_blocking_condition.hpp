#ifndef ENTITY_ENTRY_BLOCKING_CONDITION_HPP
#define ENTITY_ENTRY_BLOCKING_CONDITION_HPP

#include "entity_entry_blocking_condition_impl.hpp"

BW_BEGIN_NAMESPACE

class EntityEntryBlockingCondition
{
public:
	EntityEntryBlockingCondition( EntityEntryBlockingConditionImplPtr pImpl ) :
		pImpl_( pImpl ) {}

	~EntityEntryBlockingCondition()
	{
		pImpl_ = NULL;
	}

	void release()
	{
		pImpl_ = NULL;
	}

private:
	EntityEntryBlockingConditionImplPtr pImpl_;
};

BW_END_NAMESPACE

#endif // ENTITY_ENTRY_BLOCKING_CONDITION_HPP
