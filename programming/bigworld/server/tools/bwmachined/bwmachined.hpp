#ifndef BWMACHINED_HPP
#define BWMACHINED_HPP

#include "cstdmf/singleton.hpp"

#include "network/endpoint.hpp"

#include "server/server_info.hpp"

#include "cluster.hpp"
#include "common_machine_guard.hpp"
#include "listeners.hpp"
#include "server_platform.hpp"
#include "usermap.hpp"

BW_BEGIN_NAMESPACE

class IncomingPacket;

class BWMachined : public Singleton< BWMachined >
{
public:
	BWMachined();
	~BWMachined();

	bool readConfigFile();
	int run();

	void setPidPath( BW::string pidPath );

	static const char *STATE_FILE;
	void save();
	void load();

	Endpoint & endpoint();
	Cluster & cluster();
	const char * timingMethod() const;

	void closeEndpoints();

	friend class Cluster;
	friend class IncomingPacket;

private:
	void initNetworkInterfaces();

	bool handleCreateMessage( Endpoint &ep, sockaddr_in &sin,
		MachineGuardMessage &mgm, MGMPacket &replies );

	BW::string pidPath_;

protected:
	bool findBroadcastInterface();
	void updateSystemInfo();
	void update();
	bool broadcastToListeners( ProcessMessage &pm, int type );
	void removeRegisteredProc( unsigned index );
	void sendSignal (const SignalMessage & sm);

	void handlePacket( Endpoint & ep, sockaddr_in & sin, MGMPacket & packet );
	bool handleMessage( Endpoint & ep, sockaddr_in & sin,
		MachineGuardMessage & mgm, MGMPacket & replies );

	void readPacket( Endpoint & ep, TimeQueue64::TimeStamp & tickTime );

	bool readConfigFile( FILE * fileName );
	const char * findOption( 
		const char * optionName, const char * oldOptionName );

	// Time between statistics updates (in ms)
	static const TimeQueue64::TimeStamp UPDATE_INTERVAL = 1000;



	class PacketTimeoutHandler : public TimerHandler
	{
	public:
		void handleTimeout( TimerHandle handle, void *pUser );
		void onRelease( TimerHandle handle, void *pUser );
	};

	PacketTimeoutHandler packetTimeoutHandler_;

	class UpdateHandler : public TimerHandler
	{
	public:
		UpdateHandler( BWMachined &machined ) : machined_( machined ) {}
		void handleTimeout( TimerHandle handle, void *pUser );
		void onRelease( TimerHandle handle, void *pUser ) {}

	protected:
		BWMachined &machined_;
	};

	// The IP of the interface through which broadcast message are sent
	u_int32_t broadcastAddr_;
	Endpoint ep_;

	// Endpoint listening to 255.255.255.255 for broadcasts
	Endpoint epBroadcast_;

	// Endpoint listening to the localhost interface
	Endpoint epLocal_;

	Cluster cluster_;

	typedef BW::map< BW::string, Tags > TagsMap;
	TagsMap tags_;
	BW::string timingMethod_;
	ServerPlatform * pServerPlatform_;

	SystemInfo systemInfo_;
	BW::vector< ProcessInfo > procs_;

	Listeners birthListeners_;
	Listeners deathListeners_;
	UserMap users_;

	// A global TimeQueue for managing all callbacks in this app
	TimeQueue64 callbacks_;

	// A ServerInfo object for querying performance information from the system
	ServerInfo* pServerInfo_;

	// The maximum amount of time that a packet should be delayed by when
	// exceeding minPacketSizeToDelay_.
	int maxPacketDelayMillisec_;

public:
	int maxPacketDelay() const;
	TimeQueue64::TimeStamp timeStamp();

	static TimeQueue64::TimeStamp tvToTimeStamp( struct timeval &tv );

	static void timeStampToTV( const TimeQueue64::TimeStamp & ms,
		struct timeval & tv );

	TimeQueue64 & callbacks();
};

#if defined( CODE_INLINE )
#include "bwmachined.ipp"
#endif

BW_END_NAMESPACE

#endif // BWMACHINED_HPP
