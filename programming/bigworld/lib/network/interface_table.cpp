#include "pch.hpp"

#include "interface_table.hpp"

#include "event_dispatcher.hpp"
#include "machined_utils.hpp"

#include "cstdmf/watcher.hpp"

BW_BEGIN_NAMESPACE

namespace Mercury
{

/**
 *	Constructor.
 */
InterfaceTable::InterfaceTable( EventDispatcher & dispatcher ) :
	name_( "" ),
	id_( 0 ),
	table_( 256 ),
	pBundleEventHandler_( NULL ),
	statsTimerHandle_()
{
	statsTimerHandle_ = dispatcher.addTimer( 1000000, this, 
		NULL, "InterfaceStats" );
}


/**
 *	Destructor.
 */
InterfaceTable::~InterfaceTable()
{
	statsTimerHandle_.cancel();
}


/**
 *  This method registers an interface element as the handler for the given
 *  message ID on this interface.
 */
void InterfaceTable::serve( const InterfaceElement & ie,
	InputMessageHandler * pHandler )
{
	InterfaceElement & element = table_[ ie.id() ];
	element	= ie;
	element.pHandler( pHandler );
}


Reason InterfaceTable::registerWithMachined( const Address & addr )
{
	return MachineDaemon::registerWithMachined( addr,
			name_, id_ );
}

Reason InterfaceTable::registerWithMachined( const Address & addr,
				const BW::string & name, int id )
{
	name_ = name;
	id_ = id;
	return this->registerWithMachined( addr );
}


Reason InterfaceTable::deregisterWithMachined( const Address & addr )
{
	return MachineDaemon::deregisterWithMachined( addr,
			name_, id_ );
}


/**
 *
 */
void InterfaceTable::handleTimeout( TimerHandle handle, void * arg )
{
	// Update statistics
	for (uint i = 0; i < table_.size(); ++i)
	{
		table_[i].tick();
	}
}


void InterfaceTable::onBundleStarted( Channel * pChannel )
{
	if (pBundleEventHandler_)
	{
		pBundleEventHandler_->onBundleStarted( pChannel );
	}
}

/**
 *
 */
void InterfaceTable::onBundleFinished( Channel * pChannel )
{
	if (pBundleEventHandler_)
	{
		pBundleEventHandler_->onBundleFinished( pChannel );
	}
}


#if ENABLE_WATCHERS
/**
 *	This method returns a Watcher that inspects the entries by ID.
 */
WatcherPtr InterfaceTable::pWatcherByID()
{
	InterfaceTable * pNull = NULL;

	SequenceWatcher< Table > * pWatcher =
		new SequenceWatcher< Table >( pNull->table_ );
	pWatcher->addChild( "*", InterfaceElementWithStats::pWatcher() );

	return pWatcher;
}


/**
 *	This method returns a Watcher that inspects the entries by name.
 */
WatcherPtr InterfaceTable::pWatcherByName()
{
	InterfaceTable * pNull = NULL;

	SequenceWatcher< Table > * pWatcher =
		new SequenceWatcher< Table >( pNull->table_ );
	pWatcher->setLabelSubPath( "name" );
	pWatcher->addChild( "*", InterfaceElementWithStats::pWatcher() );

	return pWatcher;
}
#endif // ENABLE_WATCHERS

} // namespace Mercury

BW_END_NAMESPACE

// interface_table.cpp
