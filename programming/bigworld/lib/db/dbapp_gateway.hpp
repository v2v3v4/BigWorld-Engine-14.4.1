#ifndef DBAPP_GATEWAY_HPP
#define DBAPP_GATEWAY_HPP

#include "cstdmf/watcher.hpp"

#include "network/basictypes.hpp"
#include "network/channel_owner.hpp"

BW_BEGIN_NAMESPACE


/**
 *	This class is a gateway for a remote DBApp process.
 */
class DBAppGateway
{
public:
	static const DBAppGateway NONE;

	DBAppGateway();

	DBAppGateway( DBAppID id, const Mercury::Address & address );

	/** Return the ID for this DBApp. */
	DBAppID id() const { return id_; }

	/** Return the address for this DBApp. */
	const Mercury::Address & address() const { return address_; }

	/** Return true if this gateway is valid. */
	bool isValid() const 
	{ 
		return (id_ != 0) &&
			(address_ != Mercury::Address::NONE);
	}

	bool operator==( const DBAppGateway & other ) const
	{
		return (id_ == other.id_) && (address_ ==  other.address_);
	}

	bool operator!=( const DBAppGateway & other ) const
	{
		return !(*this == other);
	}

	static WatcherPtr pWatcher();

private:
	DBAppID 				id_;
	Mercury::Address 		address_;
};


BW_END_NAMESPACE


#endif // DBAPP_GATEWAY_HPP
