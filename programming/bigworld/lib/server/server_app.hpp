#ifndef SERVER_APP_HPP
#define SERVER_APP_HPP

#include "common.hpp"
#include "updatables.hpp"

#include "cstdmf/bw_string.hpp"
#include "cstdmf/watcher.hpp"

#include "network/network_interface.hpp"
#include "network/unpacked_message_header.hpp"

#include <memory>
#include <vector>


BW_BEGIN_NAMESPACE

class ManagerAppGateway;
class SignalHandler;
class Watcher;

namespace Mercury
{
class EventDispatcher;
class NetworkInterface;
class TCPServer;
}

#define SERVER_APP_HEADER_CUSTOM_NAME( APP_NAME, CONFIG_PATH ) 				\
	static const char * appName()				{ return #APP_NAME; }		\
	static const char * configPath()			{ return #CONFIG_PATH; }	\
	virtual const char * getConfigPath() const	{ return #CONFIG_PATH; }	\

#define SERVER_APP_HEADER( APP_NAME, CONFIG_PATH ) 							\
	SERVER_APP_HEADER_CUSTOM_NAME( APP_NAME, CONFIG_PATH )					\
	virtual const char * getAppName() const		{ return #APP_NAME; }		\

/**
 *	This class is the base class of the main application class for BigWorld
 *	server-side processes, and supplies many common data and functions such as
 *	the event dispatcher, network interface as well as signal handling.
 *
 *	Subclasses should use the SERVER_APP_HEADER macro to supply the process
 *	name and configuration path root.
 *
 *	e.g. 
 *	class MyServerApp : public ServerApp
 *	{
 *	public:
 *		SERVER_APP_HEADER( MyServerApp, myServerApp )
 *	}
 */
class ServerApp
{
public:
	ServerApp( Mercury::EventDispatcher & mainDispatcher,
			Mercury::NetworkInterface & interface );
	virtual ~ServerApp();

	Mercury::EventDispatcher & mainDispatcher()		{ return mainDispatcher_; }
	Mercury::NetworkInterface & interface()			{ return interface_; }

	template< class APP_TYPE >
	static APP_TYPE & getApp( const Mercury::UnpackedMessageHeader & header )
	{
		return *static_cast< APP_TYPE * >(
				reinterpret_cast< ServerApp * >(
					header.pInterface->pExtensionData() ) );
	}

	/**
	 *	This method returns a (possibly new) channel for the given address
	 *	attached to this app's interface.
	 *
	 *	@param addr 	The channel address.
	 *
	 *	@return 		A channel to the given address.
	 */
	template< class APP_TYPE >
	static Mercury::UDPChannel & getChannel( const Mercury::Address & addr )
	{
		return APP_TYPE::instance().interface().findOrCreateChannel( addr );
	}

	// The internal IP as discovered via query to BWMachineD
	static u_int32_t discoveredInternalIP;

	// Implemented by SERVER_APP_HEADER macro.
	virtual const char * getAppName() const = 0;
	virtual const char * getConfigPath() const = 0;

	virtual void requestRetirement();

	virtual void shutDown();

	bool runApp( int argc, char * argv[] );

	void advanceTime();

	void setStartTime( GameTime time )
	{
		this->onSetStartTime( time_, time );
		time_ = time;
	}

	GameTime time() const				{ return time_; }
	double gameTimeInSeconds() const;

	const char * uptime() const;
	time_t uptimeInSeconds() const;

	void setBuildDate( const char * timeStr, const char * dateStr )
	{
		buildDate_ = timeStr;
		buildDate_ += " ";
		buildDate_ += dateStr;
	}

	const char * buildDate() const	{ return buildDate_.c_str(); }
	const char * startDate() const;

	bool registerForUpdate( Updatable * pObject, int level = 0 );
	bool deregisterForUpdate( Updatable * pObject );

	static const char * shutDownStageToString( ShutDownStage value );

	virtual void onSignalled( int sigNum );

protected:
	virtual ManagerAppGateway * pManagerAppGateway() { return NULL; }

	virtual void onSetStartTime( GameTime oldTime, GameTime newTime ) {}

	virtual bool init( int argc, char * argv[] );
	virtual void fini() {}

	virtual bool run();
	virtual void onRunComplete() {}

	void enableSignalHandler( int sigNum, bool enable=true );

	virtual SignalHandler * createSignalHandler();

	void raiseFileDescriptorLimit( long limit );
	int fileDescriptorLimit() const ;
	int numFileDescriptorsOpen() const;

	void addWatchers( Watcher & watcher );

	typedef BW::vector< int > Ports;
	static bool bindToPrescribedPort( Mercury::NetworkInterface & networkInterface, 
		Mercury::TCPServer & server, const BW::string & externalInterfaceAddress,
		const Ports & externalPorts );
	static bool bindToRandomPort( Mercury::NetworkInterface & networkInterface, 
		Mercury::TCPServer & server, const BW::string & externalInterfaceAddress );

	virtual void onTickPeriod( double tickPeriod );

	/*
	 * This method gives subclasses a chance to act at the end of a tick
	 * immediately before the current server time is incremented.
	 */
	virtual void onEndOfTick() {};
	
	/*
	 * This method gives subclasses a chance to act at the beginning of a tick
	 * immediately after the current server time is incremented.
	 */
	virtual void onStartOfTick() {};
	
	/*
	 * This method gives subclassses a chance to act at the end of ServerApp's
	 * tick processing, immediately before control returns to the caller of
	 * ServerApp::advanceTime();
	 */
	virtual void onTickProcessingComplete() {};

	GameTime time_;

	Mercury::EventDispatcher & mainDispatcher_;
	Mercury::NetworkInterface & interface_;

	time_t startTime_;

	BW::string buildDate_;

	Updatables updatables_;

private:
	void callUpdatables();

	uint64 lastAdvanceTime_;
	std::auto_ptr< SignalHandler > pSignalHandler_;
};

BW_END_NAMESPACE

#endif // SERVER_APP_HPP
