#ifndef ENTITY_POPULATION_HPP
#define ENTITY_POPULATION_HPP

#include "ack_cell_app_death_helper.hpp"
#include "cellapp_death_listener.hpp"

#include "network/basictypes.hpp"

#include "cstdmf/bw_map.hpp"


BW_BEGIN_NAMESPACE

class Entity;
class CellAppChannel;

namespace Mercury
{
class BundleSendingMap;
}


/**
 *	This class is used to store all of the entities in the application.
 */
class EntityPopulation :
	public BW::map< EntityID, Entity * >,
	public CellAppDeathListener
{
public:
	/**
	*	This interface should be implemented by classes that want to watch the
	*	entity population for when a particular entity is added.
	*/
	class Observer
	{
	public:
		virtual ~Observer() {};
		virtual void onEntityAdded( Entity & entity ) = 0;
	};

	void add( Entity & entity );
	void remove( Entity & entity );

	void addObserver( EntityID id, Observer * pObserver ) const;
	bool removeObserver( EntityID id, Observer * pObserver ) const;

	void notifyObservers( Entity & entity ) const;

	// TODO: Rename these calls/fields to be 'real location'.
	void rememberRealChannel( EntityID id, CellAppChannel & channel );

	void forgetRealChannel( EntityID id )
	{
		currChannels_.erase( id );
		prevChannels_.erase( id );
	}

	void rememberBaseChannel( Entity & entity, const Mercury::Address & addr ) const;

	void expireRealChannels() const;

	void notifyBasesOfCellAppDeath( const Mercury::Address & addr,
									Mercury::BundleSendingMap & bundles,
									AckCellAppDeathHelper * pAckHelper ) const;

	CellAppChannel * findRealChannel( EntityID id ) const;

private:
	typedef BW::map< EntityID, CellAppChannel* > RealChannels;
	virtual void handleCellAppDeath( const Mercury::Address & addr );
	void forgetRealChannel( RealChannels & channels,
		const Mercury::Address & addr );

	typedef BW::multimap< EntityID, Observer * > Observers;

	mutable RealChannels currChannels_;
	mutable RealChannels prevChannels_;

	struct RecentTeleportData
	{
		Mercury::Address baseAddr;
		SpaceID spaceID;
		EntityID entityID;
	};

	typedef BW::multimap< Mercury::Address, RecentTeleportData > BaseAddrs;
	mutable BaseAddrs currRecentTeleportData_;
	mutable BaseAddrs prevRecentTeleportData_;

	void notifyBaseRangeOfCellAppDeath( 
		BaseAddrs & container, 
		const Mercury::Address & addr,
		Mercury::BundleSendingMap & bundles,
		AckCellAppDeathHelper * pAckHelper ) const;

	// This is made mutable so that the add and remove observer methods can be
	// const. We really only want the map to be const.
	mutable Observers observers_;
};


#ifdef CODE_INLINE
#include "entity_population.ipp"
#endif

BW_END_NAMESPACE

#endif // ENTITY_POPULATION_HPP
