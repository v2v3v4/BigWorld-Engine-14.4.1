//
//  LocalServerDiscovery.h
//  FantasyDemo
//
//  Created by Evan Zachariadis on 20/04/12.
//  Copyright BigWorld Pty, Ltd. All rights reserved.
//

#import <Foundation/Foundation.h>

#include "connection/server_finder.hpp"
#include "ServerListViewController.h"

/**
 *	This class implements the ServerFinder abstract class to support server 
 *	discovery.
 */
class DiscoverMenuServerFinder : public BW::ServerFinder
{
public:
	DiscoverMenuServerFinder( ServerListViewController * serverListLayer );
	virtual ~DiscoverMenuServerFinder();
	
	void onProbeSuccess( const BW::ServerInfo & serverInfo, 
						const BW::string & hostname, const BW::string & username );
	
	void onProbeFinished();
	
	// Overrides from ServerFinder.
	virtual void onServerFound( const BW::ServerInfo & serverInfo );	
	virtual void onFinished();
	
private:
	void checkIfFinished();
	
	ServerListViewController *	serverListLayer_;
	int							numOutstandingProbes_;
};


/**
 *	This class implements the ServerProbeHandler abstract class to handle 
 *	probes to servers to retrieve information about its hostname, and running 
 *	user's name.
 */
class DiscoverMenuServerProbeHandler : public BW::ServerProbeHandler
{
public:
	DiscoverMenuServerProbeHandler( DiscoverMenuServerFinder & finder,
								   const BW::ServerInfo & serverInfo );
	
	virtual ~DiscoverMenuServerProbeHandler();
	
	
	// Overrides from ServerProbeHandler.
	
	virtual void onKeyValue( const BW::string & key,
							const BW::string & value );
	
	virtual void onSuccess();
	
	virtual void onFailure();
	
	virtual void onFinished();
	
	
private:
	DiscoverMenuServerFinder & finder_;
	BW::ServerInfo serverInfo_;
	
	BW::string serverHostname_;
	BW::string serverUsername_;
};


