#ifndef ENTITY_EXTENSION_HPP
#define ENTITY_EXTENSION_HPP

#include "cstdmf/bw_namespace.hpp"

BW_BEGIN_NAMESPACE

class BWEntity;
class EntityEntryBlocker;
class MovementFilter;


/**
 *	This class is the base class of all entity extensions.
 *
 *	Entity extensions handle events for an entity and are used to implement
 *	game logic. A particular entity instance can have multiple extensions.
 */
class EntityExtension
{
public:
	EntityExtension( const BWEntity * pEntity );
	virtual ~EntityExtension();

	void triggerOnBecomePlayer()	{ this->onBecomePlayer(); }
	void triggerOnEnterAoI( const EntityEntryBlocker & rBlocker )
									{ this->onEnterAoI( rBlocker ); }
	void triggerOnEnterWorld()		{ this->onEnterWorld(); }
	void triggerOnChangeSpace()		{ this->onChangeSpace(); }
	void triggerOnLeaveWorld()		{ this->onLeaveWorld(); }
	void triggerOnLeaveAoI()		{ this->onLeaveAoI(); }
	void triggerOnBecomeNonPlayer()	{ this->onBecomeNonPlayer(); }

	void triggerOnChangeControl( bool isControlling, bool isInitialising )
		{ this->onChangeControl( isControlling, isInitialising ); }
	void triggerOnChangeReceivingVolatileUpdates()
		{ this->onChangeReceivingVolatileUpdates(); }

	void onEntityDestroyed();

	void setEntityFilter( MovementFilter * pFilter );

	/**
	 *	This method returns the entity for this extension.
	 *
	 *	This can be overridden by subclasses to support covariance in the
	 *	return type.
	 */
	virtual const BWEntity * pEntity() const { return pEntity_; }

	void setDelegate( EntityExtension * pDelegate );

	virtual const EntityExtension * pDelegate() const	{ return pDelegate_; }
	virtual EntityExtension * pDelegate()				{ return pDelegate_; }

protected:
	// Methods to override.
	// By default they forward to the delegate if it exists
	virtual void onBecomePlayer();
	virtual void onEnterAoI( const EntityEntryBlocker & rBlocker );
	virtual void onEnterWorld();
	virtual void onChangeSpace();
	virtual void onLeaveWorld();
	virtual void onLeaveAoI();
	virtual void onBecomeNonPlayer();
	virtual void onChangeControl( bool isControlling, bool isInitialising );
	virtual void onChangeReceivingVolatileUpdates();

private:
	// Derived classes can override this if they want to be reference counted.
	virtual void destroySelf();

	const BWEntity * pEntity_;

	EntityExtension * pDelegate_;
};

BW_END_NAMESPACE

#endif // ENTITY_EXTENSION_HPP
