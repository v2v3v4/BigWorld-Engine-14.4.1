#ifndef INTERFACE_TABLE_HPP
#define INTERFACE_TABLE_HPP

#include "interfaces.hpp"
#include "interface_element.hpp"

#include "cstdmf/bw_string.hpp"

BW_BEGIN_NAMESPACE

namespace Mercury
{
class Channel;
class EventDispatcher;

class InterfaceTable : public TimerHandler
{
public:
	InterfaceTable( EventDispatcher & dispatcher );
	~InterfaceTable();

	Reason registerWithMachined( const Address & addr );
	Reason registerWithMachined( const Address & addr, const BW::string & name,
		int id = 0 );

	Reason deregisterWithMachined( const Address & addr );

	void serve( const InterfaceElement & ie, InputMessageHandler * pHandler );

	void onBundleStarted( Channel * pChannel );
	void onBundleFinished( Channel * pChannel );

	void pBundleEventHandler( BundleEventHandler * pHandler )
	{
		pBundleEventHandler_ = pHandler;
	}

	/**
	 *	This method returns the name of the message with the given id.
	 */
	INLINE const char * msgName( MessageID msgID ) const
	{
		return table_[ msgID ].name();
	}

	InterfaceElementWithStats & operator[]( int id )				{ return table_[ id ]; }
	const InterfaceElementWithStats & operator[]( int id ) const	{ return table_[ id ]; }

#if ENABLE_WATCHERS
	static WatcherPtr pWatcherByID();
	static WatcherPtr pWatcherByName();
#endif

private:
	void handleTimeout( TimerHandle handle, void * arg );

	/// The name of the interface as registered with bwmachined or an empty
	/// string if not registered.
	BW::string		name_;

	/// The ID this interface is registered with machined as (e.g. cellapp01).
	int				id_;

	typedef BW::vector< InterfaceElementWithStats > Table;
	Table table_;

	BundleEventHandler * pBundleEventHandler_;

	TimerHandle statsTimerHandle_;
};

} // namespace Mercury

BW_END_NAMESPACE

#endif // INTERFACE_TABLE_HPP
