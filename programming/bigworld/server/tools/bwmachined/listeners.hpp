#ifndef LISTENERS_HPP
#define LISTENERS_HPP

#include "network/machine_guard.hpp"


BW_BEGIN_NAMESPACE

class Endpoint;
class ServerPlatform;

class Listeners
{
private:
	class Member
	{
	public:
		Member( const ListenerMessage & lm, u_int32_t addr ) :
			lm_( lm ),
			addr_( addr ) {}
		ListenerMessage lm_;
		u_int32_t addr_;
	};

public:
	Listeners( const ServerPlatform & platform );

	void add( const ListenerMessage & lm, u_int32_t addr )
	{
		members_.push_back( Member( lm, addr ) );
	}

	void handleNotify( const Endpoint & endpoint, const ProcessMessage & pm,
		in_addr addr );
	void checkListeners();

private:
	typedef BW::vector< Member > Members;
	Members members_;

	const ServerPlatform & serverPlatform_;
};

BW_END_NAMESPACE

#endif
