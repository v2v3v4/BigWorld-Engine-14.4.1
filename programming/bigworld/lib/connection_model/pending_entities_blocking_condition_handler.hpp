#ifndef PENDING_ENTITIES_BLOCKING_CONDITION_HANDLER_HPP
#define PENDING_ENTITIES_BLOCKING_CONDITION_HANDLER_HPP

#include "entity_entry_blocking_condition_handler.hpp"

#include "cstdmf/smartpointer.hpp"
#include "network/basictypes.hpp"

BW_BEGIN_NAMESPACE

class PendingEntities;

class PendingEntitiesBlockingConditionHandler;
typedef SmartPointer< PendingEntitiesBlockingConditionHandler > 
	PendingEntitiesBlockingConditionHandlerPtr;

class PendingEntitiesBlockingConditionHandler : public SafeReferenceCount,
	public EntityEntryBlockingConditionHandler
{
public: /// C++ Housekeeping
	PendingEntitiesBlockingConditionHandler(
			EntityEntryBlockingConditionImpl * pImpl,
			PendingEntities & pendingEntities,
			EntityID entityID ) :
		EntityEntryBlockingConditionHandler( pImpl ),
		pendingEntities_( pendingEntities ),
		entityID_( entityID ),
		pAbortedSelf_( NULL ){};

private: /// EntityEntryBlockingConditionHandler interface
	void onConditionCleared();

private: /// C++ Housekeeping
	// Disallow delete except from our own decRef
	// and hence disallow subclassing
	~PendingEntitiesBlockingConditionHandler() {}

	// Disallow copy-construct and assignment
	PendingEntitiesBlockingConditionHandler(
		const PendingEntitiesBlockingConditionHandler & other );
	PendingEntitiesBlockingConditionHandler & operator =(
		const PendingEntitiesBlockingConditionHandler & );

private:
	PendingEntities & pendingEntities_;
	EntityID entityID_;
	PendingEntitiesBlockingConditionHandlerPtr pAbortedSelf_;
};

BW_END_NAMESPACE

#endif // PENDING_ENTITIES_BLOCKING_CONDITION_HANDLER_HPP
