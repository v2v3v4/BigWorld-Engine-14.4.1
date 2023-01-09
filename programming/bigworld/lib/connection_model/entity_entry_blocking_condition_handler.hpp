#ifndef ENTITY_ENTRY_BLOCKING_CONDITION_HANDLER_HPP
#define ENTITY_ENTRY_BLOCKING_CONDITION_HANDLER_HPP

#include "cstdmf/bw_namespace.hpp"

BW_BEGIN_NAMESPACE

class EntityEntryBlockingConditionImpl;

/**
 *	This class is an interface for callbacks when an
 *	EntityEntryBlockingConditionImpl is destroyed
 */
class EntityEntryBlockingConditionHandler
{
public: /// C++ housekeeping
	EntityEntryBlockingConditionHandler(
		EntityEntryBlockingConditionImpl * pImpl );

	virtual ~EntityEntryBlockingConditionHandler();

public: /// EntityEntryBlockingConditionHandler interface
	void abort();

private: /// Interface for EntityEntryBlockingConditionImpl
	friend class EntityEntryBlockingConditionImpl;
	void onBlockingConditionCleared();

private: /// Subclass virtual interface
	/**
	 *	This method is called to notify subclasses when the
	 *	EntityEntryBlockingConditionImpl we are the callback for is being
	 *	destroyed
	 */
	virtual void onConditionCleared() = 0;

	/**
	 *	This method is called by abort() to notify subclasses that we have been
	 *	aborted and onBlockingConditionCleared will never be called
	 */
	virtual void onAborted() {};

private: /// C++ housekeeping
	// Disallow copy-construct and assignment
	EntityEntryBlockingConditionHandler(
		const EntityEntryBlockingConditionHandler & other );
	EntityEntryBlockingConditionHandler & operator =(
		const EntityEntryBlockingConditionHandler & );

private:
	EntityEntryBlockingConditionImpl * pImpl_;
	bool hasTriggered_;
};

BW_END_NAMESPACE

#endif // ENTITY_ENTRY_BLOCKING_CONDITION_HANDLER_HPP
