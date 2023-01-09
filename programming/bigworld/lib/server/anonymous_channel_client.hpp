#ifndef ANONYMOUS_CHANNEL_CLIENT_HPP
#define ANONYMOUS_CHANNEL_CLIENT_HPP

#include "cstdmf/bw_namespace.hpp"

#include "network/interfaces.hpp"

#include "cstdmf/bw_string.hpp"



BW_BEGIN_NAMESPACE

class Watcher;

namespace Mercury
{
class ChannelOwner;
class InterfaceMinder;
class NetworkInterface;
}

/**
 *	This class is used to handle channels to processes that may not have
 *	explicit knowledge of us.
 */
class AnonymousChannelClient : public Mercury::InputMessageHandler
{
public:
	AnonymousChannelClient();
	~AnonymousChannelClient();

	bool init( Mercury::NetworkInterface & interface,
		Mercury::InterfaceMinder & interfaceMinder,
		const Mercury::InterfaceElement & birthMessage,
		const char * componentName,
		int numRetries );

	Mercury::ChannelOwner * pChannelOwner() const	{ return pChannelOwner_; }

	void addWatchers( const char * name, Watcher & watcher );

private:
	virtual void handleMessage( const Mercury::Address & srcAddr,
			Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data );

	Mercury::ChannelOwner * pChannelOwner_;
	BW::string				interfaceName_;
};

#define BW_INIT_ANONYMOUS_CHANNEL_CLIENT( INSTANCE, INTERFACE,				\
		CLIENT_INTERFACE, SERVER_INTERFACE, NUM_RETRIES )					\
	INSTANCE.init( INTERFACE,												\
		CLIENT_INTERFACE::gMinder,											\
		CLIENT_INTERFACE::SERVER_INTERFACE##Birth,							\
		#SERVER_INTERFACE,													\
		NUM_RETRIES )														\


#define BW_ANONYMOUS_CHANNEL_CLIENT_MSG( SERVER_INTERFACE )					\
	MERCURY_FIXED_MESSAGE( SERVER_INTERFACE##Birth,							\
		sizeof( Mercury::Address ),											\
		NULL /* Handler set by BW_INIT_ANONYMOUS_CHANNEL_CLIENT */ )

BW_END_NAMESPACE

#endif // ANONYMOUS_CHANNEL_CLIENT_HPP
