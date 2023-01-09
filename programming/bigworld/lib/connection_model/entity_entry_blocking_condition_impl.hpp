#ifndef ENTITY_ENTRY_BLOCKING_CONDITION_IMPL_HPP
#define ENTITY_ENTRY_BLOCKING_CONDITION_IMPL_HPP

#include "cstdmf/smartpointer.hpp"

BW_BEGIN_NAMESPACE

class EntityEntryBlocker;
class EntityEntryBlockingConditionHandler;

class EntityEntryBlockingConditionImpl : public SafeReferenceCount
{
private: /// Handler interface
	friend class EntityEntryBlockingConditionHandler;
	void setHandler( EntityEntryBlockingConditionHandler * pHandler );

private: /// C++ Housekeeping
	// Only EntityEntryBlocker may create an instance of this class
	friend class EntityEntryBlocker;
	EntityEntryBlockingConditionImpl() : pHandler_( NULL ) {};
	// Disallow delete except from our own decRef
	// and hence disallow subclassing
	~EntityEntryBlockingConditionImpl();

	// Disallow copy-construct and assignment
	EntityEntryBlockingConditionImpl(
		const EntityEntryBlockingConditionImpl & other );
	EntityEntryBlockingConditionImpl & operator =(
		const EntityEntryBlockingConditionImpl & );

private:
	EntityEntryBlockingConditionHandler * pHandler_;
};

typedef SmartPointer< EntityEntryBlockingConditionImpl >
	EntityEntryBlockingConditionImplPtr;

BW_END_NAMESPACE

#endif // ENTITY_ENTRY_BLOCKING_CONDITION_IMPL_HPP
