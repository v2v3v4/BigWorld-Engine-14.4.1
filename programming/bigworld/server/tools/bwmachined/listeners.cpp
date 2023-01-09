#include "listeners.hpp"

#include "server_platform.hpp"

#include "network/endpoint.hpp"

#include <syslog.h>


BW_BEGIN_NAMESPACE

Listeners::Listeners( const ServerPlatform & platform ) :
	serverPlatform_( platform )
{
}


/**
 *
 */
void Listeners::handleNotify( const Endpoint & endpoint,
	const ProcessMessage & pm, in_addr addr )
{
	char address[6];
	memcpy( address, &addr, sizeof( addr ) );
	memcpy( address + sizeof( addr ), &pm.port_, sizeof( pm.port_ ) );

	Members::iterator iter = members_.begin();

	while (iter != members_.end())
	{
		ListenerMessage &lm = iter->lm_;

		if (lm.category_ == pm.category_ &&
			(lm.uid_ == lm.ANY_UID || lm.uid_ == pm.uid_) &&
			(lm.name_ == pm.name_ || lm.name_.size() == 0))
		{
			int msglen = lm.preAddr_.size() + sizeof( address ) +
				lm.postAddr_.size();
			char *data = new char[ msglen ];
			int preSize = lm.preAddr_.size();
			int postSize = lm.postAddr_.size();

			memcpy( data, lm.preAddr_.c_str(), preSize );
			memcpy( data + preSize, address, sizeof( address ) );
			memcpy( data + preSize + sizeof( address ), lm.postAddr_.c_str(),
				postSize );

			// and send to the appropriate port locally
			endpoint.sendto( data, msglen, lm.port_, iter->addr_ );
			delete [] data;
		}

		++iter;
	}
}


/**
 *
 */
void Listeners::checkListeners()
{
	for (Members::iterator it = members_.begin(); it != members_.end(); it++)
	{
		if (!serverPlatform_.isProcessRunning( it->lm_.pid_ ))
		{
			syslog( LOG_INFO, "Dropping dead listener (for %s's) with pid %d",
				it->lm_.name_.c_str(), it->lm_.pid_ );
			members_.erase( it-- );
		}
	}
}

BW_END_NAMESPACE

// listeners.cpp
