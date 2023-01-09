#ifndef SERVER_FINDER_HPP
#define SERVER_FINDER_HPP

#include "network/endpoint.hpp"
#include "network/interfaces.hpp"
#include "network/machine_guard_response_checker.hpp"

BW_BEGIN_NAMESPACE

class ProcessMessage;

namespace Mercury
{
	class Address;
	class NetworkInterface;
}


/**
 *	This class represents the details of a newly found server.
 */
class ServerInfo
{
public:
	ServerInfo( const Mercury::Address & addr, uint16 uid ) :
		addr_( addr ),
		uid_( uid )
	{
	}

	const Mercury::Address & address() const	{ return addr_; }
	uint16 uid() const		{ return uid_; }

private:
	Mercury::Address addr_;
	uint16 uid_;
};


class ServerProbeHandler : public Mercury::ShutdownSafeReplyMessageHandler
{
public:
	virtual void handleMessage( const Mercury::Address & srcAddr,
		Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data,
		void * arg );

	virtual void handleException( const Mercury::NubException & /*exception*/,
		void * /*arg*/ );

protected:
	virtual void onKeyValue( const BW::string & key,
		const BW::string & value ) = 0;
	virtual void onSuccess() = 0;
	virtual void onFailure() {};

	virtual void onFinished();
};


/**
 *	This class finds servers and their details, in its own time.
 */
class ServerFinder : public Mercury::InputNotificationHandler,
	public TimerHandler
{
public:
	ServerFinder();
	virtual ~ServerFinder();

	void findServers( Mercury::NetworkInterface * pNetworkInterface,
		   float timeout = 2.f );

	virtual int handleInputNotification( int fd );
	virtual void handleTimeout( TimerHandle handle, void * pUser );
protected:
	virtual void onRelease( TimerHandle handle, void * pUser );

	virtual void onServerFound( const ServerInfo & serverInfo ) = 0;

	virtual void onFinished();

	void sendProbe( const Mercury::Address & address,
		ServerProbeHandler * pHandler );

private:
	ServerFinder( const ServerFinder & );
	ServerFinder & operator=( const ServerFinder & );

	void sendFindMGM();
	bool recv();
	void onMessage( const ProcessMessage & msg,
				   const Mercury::Address & srcAddr );

	Mercury::NetworkInterface * pNetworkInterface_;

	Endpoint endpoint_;
	TimerHandle timerHandle_;
	MachineGuardResponseChecker responseChecker_;
};

BW_END_NAMESPACE

#endif // SERVER_FINDER_HPP
