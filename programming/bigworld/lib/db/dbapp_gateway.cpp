#include "dbapp_gateway.hpp"

BW_BEGIN_NAMESPACE


const DBAppGateway DBAppGateway::NONE = DBAppGateway();


/**
 *	Default constructor.
 */
DBAppGateway::DBAppGateway() :
		id_( 0 ),
		address_( Mercury::Address::NONE )
{}



/**
 *	Constructor.
 */
DBAppGateway::DBAppGateway( DBAppID id, const Mercury::Address & address ) :
		id_( id ),
		address_( address )
{
	MF_ASSERT( address != Mercury::Address::NONE );
}


/**
 *	This static method returns a watcher suitable for this class.
 */
WatcherPtr DBAppGateway::pWatcher()
{
	DirectoryWatcher * pWatcher = new DirectoryWatcher();

	pWatcher->addChild( "id", makeWatcher( &DBAppGateway::id ) );
	pWatcher->addChild( "address", makeWatcher( &DBAppGateway::address ) );

	return pWatcher;
}


BW_END_NAMESPACE


// dbapp_gateway.cpp
