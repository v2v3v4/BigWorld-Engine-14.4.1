//
//  LocalServerDiscovery.mm
//  iosclient
//
//  Created by Evan Zachariadis on 20/04/12.
//  Copyright BigWorld Pty, Ltd. All rights reserved.
//

#import "LocalServerDiscovery.h"

#import "ApplicationDelegate.h"

#include "connection_model/bw_connection.hpp"
#include "connection/server_finder.hpp"

/**
 *	Constructor.
 */
DiscoverMenuServerFinder::DiscoverMenuServerFinder( ServerListViewController * serverListLayer ):
		BW::ServerFinder(),
		serverListLayer_( serverListLayer ),
		numOutstandingProbes_( 0 )
{
	[serverListLayer_ retain];
}


/**
 *	Destructor.
 */
DiscoverMenuServerFinder::~DiscoverMenuServerFinder()
{
	[serverListLayer_ release];
}


/**
 *	This method implements the abstract method in the ServerFinder class.
 */
void DiscoverMenuServerFinder::onServerFound( const BW::ServerInfo & serverInfo )
{
	DiscoverMenuServerProbeHandler * pHandler = 
	new DiscoverMenuServerProbeHandler( *this, serverInfo );
	
	this->sendProbe( serverInfo.address(), pHandler );
	
	++numOutstandingProbes_;
}


/**
 *	This method overrides the virtual method in the ServerFinder class.
 */
void DiscoverMenuServerFinder::onFinished()
{
	// If we sent out probes, don't clean up ourselves yet, wait for any probes
	// to come back.
	this->checkIfFinished();	
}


/**
 *	This method is called when a probe is successful in getting probe results.
 *
 *	@param serverInfo	The server information.
 *	@param hostname		The hostname of the machine running LoginApp.
 *	@param username		The name of the user running the server.
 */
void DiscoverMenuServerFinder::onProbeSuccess( const BW::ServerInfo & serverInfo,
											  const BW::string & hostname,
											  const BW::string & username )
{
	NSString * addressString = 
	[NSString stringWithUTF8String: serverInfo.address().c_str()];
	
	NSString * hostnameString = 
	[NSString stringWithUTF8String: hostname.c_str()];
	
	NSString * usernameString =
	[NSString stringWithUTF8String: username.c_str()];
	
	[serverListLayer_ onFoundServer: addressString
						   withName: hostnameString
							andUser: usernameString];
}


/**
 *	This method is called by a child probe when it has finished its operation.
 */
void DiscoverMenuServerFinder::onProbeFinished()
{
	--numOutstandingProbes_;
	this->checkIfFinished();
}


/**
 *	This method checks whether this server finder has completed its operation,
 *	and if so, cleans up the object.
 */
void DiscoverMenuServerFinder::checkIfFinished()
{
	if (numOutstandingProbes_ <= 0)
	{
		[serverListLayer_ onFindServersDone];
		
		// The superclass handles the deletion of this object.
		this->ServerFinder::onFinished();
	}		
}


/**
 *	Constructor.
 *
 *	@param finder		The server finder object, which will receive the probe 
 *						results from this probe.
 *	@param serverInfo	The identifying information for the server to probe.
 */
DiscoverMenuServerProbeHandler::DiscoverMenuServerProbeHandler( 															   
			DiscoverMenuServerFinder & finder,
			const BW::ServerInfo & serverInfo ) :
		ServerProbeHandler(),
		finder_( finder ),
		serverInfo_( serverInfo ),
		serverHostname_(),
		serverUsername_()
{}


/**
 *	Destructor.
 */
DiscoverMenuServerProbeHandler::~DiscoverMenuServerProbeHandler()
{}


/**
 *	This method implements the abstract method in ServerProbeHandler.
 */
void DiscoverMenuServerProbeHandler::onKeyValue( const BW::string & key,
												const BW::string & value )
{
	if (key == "ownerName")
	{
		serverUsername_ = value;
	}
	
	else if (key == "hostName")
	{
		serverHostname_ = value;
	}
}


/**
 *	This method implements the abstract method in ServerProbeHandler.
 */
void DiscoverMenuServerProbeHandler::onSuccess()
{
	finder_.onProbeSuccess( serverInfo_, serverHostname_, serverUsername_ );
}


/**
 *	This method overrides the virtual method in ServerProbeHandler.
 */
void DiscoverMenuServerProbeHandler::onFailure()
{
	NSLog( @"DiscoverMenuServerProbeHandler::onFailure: %s",
		  serverInfo_.address().c_str() );
}


/**
 *	This method overrides the virtual method in ServerProbeHandler.
 */
void DiscoverMenuServerProbeHandler::onFinished()
{
	finder_.onProbeFinished();
	
	// Clean up ourselves.
	this->ServerProbeHandler::onFinished();
}
