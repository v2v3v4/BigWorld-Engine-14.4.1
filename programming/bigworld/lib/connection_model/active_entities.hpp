#ifndef ACTIVE_ENTITIES_HPP
#define ACTIVE_ENTITIES_HPP

#include "bw_entity.hpp"
#include "bw_entities_listener.hpp"

#include "cstdmf/bw_list.hpp"
#include "cstdmf/bw_map.hpp"

BW_BEGIN_NAMESPACE

class BWEntities;

class ActiveEntities
{
	typedef BW::map< EntityID, BWEntityPtr > Collection;

public:
	/// C++ Housekeeping
	ActiveEntities( BWEntities & entities );
	~ActiveEntities();

	/// Entity List Management
	BWEntity * findEntity( EntityID entityID ) const;

	/**
	 *	This method indicates that there are no entities pending in our list
	 */
	bool empty() const { return collection_.empty(); }

	/**
	 *	This method returns the number of entities pending in our list
	 */
	size_t size() const { return collection_.size(); }

	void addEntityToWorld( BWEntity * pEntity );

	bool removeEntityFromWorld( EntityID entityID,
		PassengersVector * pFormerPassengers );

	void clearEntities( bool keepLocalEntities = false );
	BWEntityPtr clearEntitiesWithException( EntityID exceptEntityID );

	void processEntityFilters( double gameTime,
		BW::list< BWEntity * > & inconsistentEntities );

	void sendControlledEntitiesUpdates( double gameTime );

	typedef Collection::const_iterator const_iterator;

	const_iterator begin() const	{ return collection_.begin(); }
	const_iterator end() const	{ return collection_.end(); }

	/// BWEntitiesListener registration
	void addListener( BWEntitiesListener * pListener );
	bool removeListener( BWEntitiesListener * pListener );
	void notifyListenersOfEntitiesReset() const;

private:
	void doRemoveEntityFromWorld( Collection::iterator iter,
		PassengersVector * pFormerPassengers = NULL );

	BWEntities & entities_;

	/// BWEntitiesListener support
	typedef void (BWEntitiesListener::*ListenerFunc)( BWEntity * );
	void callListeners( ListenerFunc func, BWEntity * pEntity ) const;

	typedef BW::vector< BWEntitiesListener * > Listeners;
	Listeners listeners_;

	/// Entity List Management
	Collection collection_;
};

BW_END_NAMESPACE

#endif // ACTIVE_ENTITIES_HPP
