#include "pch.hpp"

#include "entity_extension.hpp"

#include "bw_entity.hpp"

#include "connection/movement_filter.hpp"

BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 */
EntityExtension::EntityExtension( const BWEntity * pEntity ) :
	pEntity_( pEntity ),
	pDelegate_( NULL )
{
}


/**
 *	Destructor.
 */
EntityExtension::~EntityExtension()
{
	if (pDelegate_)
	{
		pDelegate_->destroySelf();
	}
}


/**
 *
 */
void EntityExtension::onEntityDestroyed()
{
	pEntity_ = NULL;
	this->destroySelf();
}


/**
 *
 */
void EntityExtension::setDelegate( EntityExtension * pDelegate )
{
	if (pDelegate_)
	{
		pDelegate_->destroySelf();
	}

	pDelegate_ = pDelegate;
}



/**
 *
 */
void EntityExtension::setEntityFilter( MovementFilter * pFilter )
{
	const_cast< BWEntity * >( pEntity_ )->pFilter( pFilter );
}


// -----------------------------------------------------------------------------
// Section: Methods to override.
// -----------------------------------------------------------------------------

/**
 *	This method is called when a player entity is initially created.
 */
void EntityExtension::onBecomePlayer()
{
	if (pDelegate_)
	{
		pDelegate_->triggerOnBecomePlayer();
	}
}


/**
 *	This method is called when the entity first has a consistent cell position.
 */
void EntityExtension::onEnterAoI( const EntityEntryBlocker & rBlocker )
{
	if (pDelegate_)
	{
		pDelegate_->triggerOnEnterAoI( rBlocker );
	}
}


/**
 *	This method is called once all blocking conditions from the call to
 *	onEnterAoI have been satisfied.
 */
void EntityExtension::onEnterWorld()
{
	if (pDelegate_)
	{
		pDelegate_->triggerOnEnterWorld();
	}
}


/**
 *	This method is called if an entity in world gets a position update into
 *	a different space.
 */
void EntityExtension::onChangeSpace()
{
	if (pDelegate_)
	{
		pDelegate_->triggerOnChangeSpace();
	}
}


/**
 *	This method is called when the client no longer knows about the cell entity.
 *	It is only called if onEnterWorld has been called.
 */
void EntityExtension::onLeaveWorld()
{
	if (pDelegate_)
	{
		pDelegate_->triggerOnLeaveWorld();
	}
}


/**
 *	This method is called when the client no longer knows about the cell entity.
 */
void EntityExtension::onLeaveAoI()
{
	if (pDelegate_)
	{
		pDelegate_->triggerOnLeaveAoI();
	}
}


/**
 *	This method is called when the associated entity stops being the player.
 */
void EntityExtension::onBecomeNonPlayer()
{
	if (pDelegate_)
	{
		pDelegate_->triggerOnBecomeNonPlayer();
	}
}


/**
 *	This method is called when the associated entity changes control status
 */
void EntityExtension::onChangeControl( bool isControlling,
	bool isInitialising )
{
	if (pDelegate_)
	{
		pDelegate_->triggerOnChangeControl( isControlling, isInitialising );
	}
}


/**
 *	This method is called when this entity receives a volatile update when
 *	it has been receiving non-volatile updates or vice-versa.
 */
void EntityExtension::onChangeReceivingVolatileUpdates()
{
	if (pDelegate_)
	{
		pDelegate_->triggerOnChangeReceivingVolatileUpdates();
	}
}


/**
 *	This method is called when the entity is done with this extension.
 *	Derived classes can override this if they want to be reference counted.
 */
void EntityExtension::destroySelf()
{
	delete this;
}

BW_END_NAMESPACE

// entity_extension.cpp
