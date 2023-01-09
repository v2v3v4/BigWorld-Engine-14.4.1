#ifndef PENDING_ENTITIES_HPP
#define PENDING_ENTITIES_HPP

#include "bw_entity.hpp"

#include "entity_entry_blocking_condition_impl.hpp"
#include "pending_entities_blocking_condition_handler.hpp"

#include "cstdmf/bw_map.hpp"

BW_BEGIN_NAMESPACE

class BWEntities;

class PendingEntities
{
public: /// C++ Housekeeping
	PendingEntities( BWEntities & entities );
	~PendingEntities();

public: /// Entity list management
	BWEntity * findEntity( EntityID entityID ) const;

	/**
	 *	This method indicates that there are no entities pending in our list
	 */
	bool empty() const { return collection_.empty(); }

	/**
	 *	This method returns the number of entities pending in our list
	 */
	size_t size() const { return collection_.size(); }

	void addEntity( BWEntity * pEntity,
		EntityEntryBlockingConditionImplPtr pBlockingConditionImpl );

	bool eraseEntity( EntityID entityID );

	void erasePassengers( EntityID vehicleID,
		PassengersVector * pFormerPassengers );

	void clear( bool keepLocalEntities = false );

	void processEntityFilters( double gameTime,
		BW::list< BWEntity * > & inconsistentEntities );

private: /// Private list management interface
	friend class PendingEntitiesBlockingConditionHandler;
	void releaseEntity( EntityID entityID );

private:
	BWEntities & entities_;

	typedef BW::map< EntityID,
		std::pair< BWEntityPtr, PendingEntitiesBlockingConditionHandlerPtr > >
		Collection;

	Collection collection_;
};

BW_END_NAMESPACE

#endif // PENDING_ENTITIES_HPP
