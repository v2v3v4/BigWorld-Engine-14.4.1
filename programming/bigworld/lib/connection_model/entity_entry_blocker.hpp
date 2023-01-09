#ifndef ENTITY_ENTRY_BLOCKER_HPP
#define ENTITY_ENTRY_BLOCKER_HPP

#include "entity_entry_blocking_condition.hpp"
#include "entity_entry_blocking_condition_impl.hpp"

BW_BEGIN_NAMESPACE

class EntityEntryBlocker
{
public: /// C++ Housekeeping
	EntityEntryBlocker();

public: /// Entity entry blocker interface
	EntityEntryBlockingCondition blockEntry() const;

	EntityEntryBlockingConditionImplPtr getImpl();

private:
	mutable EntityEntryBlockingConditionImplPtr pImpl_;
};

BW_END_NAMESPACE

#endif // ENTITY_ENTRY_BLOCKER_HPP
