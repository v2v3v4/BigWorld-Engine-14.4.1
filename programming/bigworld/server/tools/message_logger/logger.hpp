#ifndef LOGGER_HPP
#define LOGGER_HPP

#include "unary_integer_file.hpp"
#include "types.hpp"

#include "cstdmf/timer_handler.hpp"
#include "network/event_dispatcher.hpp"
#include "network/logger_message_forwarder.hpp"
#include "network/machine_guard.hpp"
#include "network/watcher_nub.hpp"


BW_BEGIN_NAMESPACE

class LogStorage;

// These classes are defined in the .cpp, but are given friend access here.
class FindHandler;
class TagsHandler;

/**
 *	This is the main class of the message_logger process. It is responsible for
 *	receiving log messages from other components.
 */

class Logger : public WatcherRequestHandler, 
	public TimerHandler
{
	friend class FindHandler;
	friend class TagsHandler;

public:
	Logger();
	virtual ~Logger();

	bool init( int argc, char * argv[] );
	bool handleNextMessage();

	void shouldRoll( bool status ) { shouldRoll_ = status; }

	bool shouldLogPriority(
		MessageLogger::NetworkMessagePriority messagePriority );

	void shouldValidateHostnames( bool status )
		{ shouldValidateHostnames_ = status; }

	Mercury::EventDispatcher * getDispatcher (){ return &dispatcher_; };

	const BW::string & getLoggerID(){ return loggerID_; }

protected:
	virtual void processExtensionMessage( int messageID,
			char * data, int dataLen, WatcherEndpoint & watcherEndpoint );

	virtual void processDisconnect( WatcherEndpoint & watcherEndpoint );

public:
	class Component : public LoggerComponentMessage
	{
	public:

		Component( WatcherEndpoint & watcherEndpoint ) :
			watcherEndpoint_( watcherEndpoint )
		{
		}

		static WatcherPtr pWatcher();
		const char *name() const { return componentName_.c_str(); }

		// TODO: Fix this
		bool commandAttached() const	{ return true; }
		void commandAttached( bool value );

		WatcherEndpoint & watcherEndpoint()
		{
			return watcherEndpoint_;
		}

		const WatcherEndpoint & watcherEndpoint() const
		{
			return watcherEndpoint_;
		}

	private:
		Component( const Component & other );
		Component & operator=( const Component & other );

		WatcherEndpoint watcherEndpoint_;
	};

private:

	bool parseCommandLine( int argc, char * argv[] );

	void initClusterGroups();
	void initComponents();

	void handleBirth( const Mercury::Address & addr );
	void handleDeath( const Mercury::Address & addr );

	void handleLogMessage( MemoryIStream &is, const Mercury::Address & addr,
		WatcherEndpoint & watcherEndpoint );
	void handleRegisterRequest( char * data, int dataLen,
		const Mercury::Address & addr, WatcherEndpoint & watcherEndpoint );

	bool shouldConnect( const Component & component ) const;

	bool sendAdd( const Mercury::Address & addr, const int sendType );
	void sendDel( WatcherEndpoint & watcherEndpoint );

	void delComponent( const Mercury::Address & addr, bool send = true );
	void delComponent( Component * pComponent );

	bool commandReattachAll() const { return true; }
	void commandReattachAll( bool value );

	bool resetFileDescriptors();

	Endpoint & socket()		{ return watcherNub_.udpSocket(); }

	// Watcher
	int size() const	{ return components_.size(); }

	enum TimeOutType
	{
		TIMEOUT_PROFILER_UPDATE,
	};
	void startProfilerUpdateTimer();
	// Override from TimerHandler
	virtual void handleTimeout( TimerHandle handle, void * arg );
	TimerHandle profilerTimer_;

	BW::string interfaceName_;
	WatcherNub watcherNub_;
	Mercury::EventDispatcher dispatcher_;

	// ID of the processes whose messages should be logged. The default is the
	// empty string, a special value that causes logging all processes,
	// regardless of loggerID.
	LoggerID loggerID_;

	uint logUser_;
	bool logAllUsers_;
	BW::vector< BW::string > logNames_;
	BW::vector< BW::string > doNotLogNames_;
	bool quietMode_;
	bool daemonMode_;
	bool shouldRoll_;
	bool shouldValidateHostnames_;
	bool shouldWriteToStdout_;

	bool createEndpointAndQueryMsg(
		MachineGuardMessage &mgm, MessageLogger::IPAddress ipAddress,
		MachineGuardMessage::ReplyHandler &handler );

	bool shouldLogFromGroup( const Mercury::Address &addr );

	class MachineGroups
	{
	public:
		// Groups that have been defined for the current machine
		MessageLogger::StringList groups_;

		// Last time we asked the machine for it's updated list
		// of groups.
		uint64 lastPollTime_;
	};

	typedef BW::map< uint32, MachineGroups * > MachineGroupsMap;
	MachineGroupsMap machineGroups_;

	// The list of group names this MessageLogger process should service.
	MessageLogger::StringList groupNames_;

	BW::string configFile_;

	BW::string outputFilename_;
	BW::string errorFilename_;

	BW::string addLoggerData_;
	BW::string addLoggerDataTCP_;
	BW::string addLoggerDataTCPWithMetaDataV2_9_;
	BW::string addLoggerDataTCPWithMetaDataV14_4_;
	BW::string delLoggerData_;

	BW::string storageType_;
	
	UnaryIntegerFile pid_;
	BW::string pidPath_;

	typedef BW::map< Mercury::Address, Component* > Components;
	Components components_;

	bool shouldLogMessagePriority_[ NUM_MESSAGE_PRIORITY ];

	LogStorage *pLogStorage_;

	BW::string workingDirectory_;
	BW::string rootLogDirectory_;
	bool isMongoDBDriverInitialised_;
};

BW_END_NAMESPACE

#endif // LOGGER_HPP
