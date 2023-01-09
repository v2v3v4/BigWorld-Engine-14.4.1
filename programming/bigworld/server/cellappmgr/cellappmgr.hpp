#ifndef CELLAPPMGR_HPP
#define CELLAPPMGR_HPP

#include "cellappmgr_interface.hpp"
#include "cell_data.hpp"
#include "cellapps.hpp"

#include "cstdmf/bw_vector.hpp"
#include "cstdmf/bw_map.hpp"
#include "cstdmf/singleton.hpp"

#include "network/channel_owner.hpp"

#include "server/common.hpp"
#include "server/manager_app.hpp"
#include "server/services_map.hpp"


BW_BEGIN_NAMESPACE

class CellAppMgrViewerServer;

class Space;
typedef BW::map< int, Space* > Spaces;
typedef BW::set< SpaceID > SpaceIDs;

class CellApp;
class CellAppGroup;
class CellAppDeathHandler;
class CellAppMgrConfig;
class ShutDownHandler;
class TimeKeeper;

typedef Mercury::ChannelOwner BaseAppMgr;
typedef Mercury::ChannelOwner DBApp;
typedef Mercury::ChannelOwner DBAppMgr;


/**
 *	An object of this type is the main object for the Cell App Manager
 *	application. It is used to manage the Cell Apps that are running on the
 *	server. It is responsible for maintaining the cells and the spaces and
 *	making sure the the Cell Apps are balanced.
 */
class CellAppMgr : public ManagerApp,
	public TimerHandler,
	public Singleton< CellAppMgr >
{
public:
	MANAGER_APP_HEADER( CellAppMgr, cellAppMgr )

	typedef CellAppMgrConfig Config;

	// ---- Construction ----
	CellAppMgr( Mercury::EventDispatcher & mainDispatcher,
			Mercury::NetworkInterface & interface );
	virtual ~CellAppMgr();

	void addWatchers();

	void setShutDownHandler( ShutDownHandler * pHandler )
		{ pShutDownHandler_ = pHandler; }
	ShutDownHandler * pShutDownHandler() const	{ return pShutDownHandler_; }

	void controlledShutDown();

	// ---- Message Handlers ----
	void createEntity( const Mercury::Address & addr,
		const Mercury::UnpackedMessageHeader& header,
		BinaryIStream & data );
	void createEntityInNewSpace( const Mercury::Address & addr,
		const Mercury::UnpackedMessageHeader& header,
		BinaryIStream & data );
	void createEntityCommon( Space * pSpace,
		const Mercury::Address & addr,
		const Mercury::UnpackedMessageHeader& header,
		BinaryIStream & data );

	void prepareForRestoreFromDB( const Mercury::Address & addr,
		const Mercury::UnpackedMessageHeader& header,
		BinaryIStream & data );

	void startup( const Mercury::Address & addr,
		const Mercury::UnpackedMessageHeader& header,
		BinaryIStream & data );

	virtual void shutDown() // from ServerApp
		{ this->shutDown( false ); }

	void shutDown( bool shutDownOthers );

	void shutDown( const CellAppMgrInterface::shutDownArgs & args );
	void controlledShutDown(
			const CellAppMgrInterface::controlledShutDownArgs & args );

	void triggerControlledShutDown();

	void shouldOffload( const CellAppMgrInterface::shouldOffloadArgs& args );

	void addApp( const Mercury::Address & addr,
			const Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data );
	void delApp( const CellAppMgrInterface::delAppArgs & args );

	void recoverCellApp( BinaryIStream & data );

	void setBaseApp( const CellAppMgrInterface::setBaseAppArgs & args );

	void handleCellAppMgrBirth(
		const CellAppMgrInterface::handleCellAppMgrBirthArgs & args );

	void handleBaseAppMgrBirth(
		const CellAppMgrInterface::handleBaseAppMgrBirthArgs & args );

	void handleCellAppDeath( const Mercury::Address & srcAddr,
		const Mercury::UnpackedMessageHeader & header,
		const CellAppMgrInterface::handleCellAppDeathArgs & args );

	void ackCellAppDeath( const Mercury::Address & srcAddr,
		const Mercury::UnpackedMessageHeader & header,
		const CellAppMgrInterface::ackCellAppDeathArgs & args );

	void handleBaseAppDeath( BinaryIStream & data );

	void handleDBAppMgrBirth(
		const CellAppMgrInterface::handleDBAppMgrBirthArgs & args );

	void setDBAppAlpha( const CellAppMgrInterface::setDBAppAlphaArgs & args );

	void setSharedData( const Mercury::Address & srcAddr,
		const Mercury::UnpackedMessageHeader & header, BinaryIStream & data );
	void delSharedData( const Mercury::Address & srcAddr,
		const Mercury::UnpackedMessageHeader & header, BinaryIStream & data );

	void addServiceFragment( BinaryIStream & data );
	void delServiceFragment( BinaryIStream & data );

	void updateSpaceData( const Mercury::Address & srcAddr,
		const Mercury::UnpackedMessageHeader & header, BinaryIStream & data );

	void shutDownSpace( const CellAppMgrInterface::shutDownSpaceArgs & args );

	void ackBaseAppsShutDown(
			const CellAppMgrInterface::ackBaseAppsShutDownArgs & args );

	void checkStatus( const Mercury::Address & addr,
		const Mercury::UnpackedMessageHeader& header,
		BinaryIStream & data );

	void gameTimeReading( const Mercury::Address & srcAddr,
			const Mercury::UnpackedMessageHeader & header,
			const CellAppMgrInterface::gameTimeReadingArgs & args );

	void requestEntityPositions();

	void ready( int component );

	// Accessors
	static Mercury::UDPChannel & getChannel( const Mercury::Address & addr )
	{
		return CellAppMgr::instance().interface().findOrCreateChannel( addr );
	}

	BaseAppMgr & baseAppMgr()		{ return baseAppMgr_; }
	DBApp & dbAppAlpha()			{ return dbAppAlpha_; }
	DBAppMgr & dbAppMgr() 			{ return dbAppMgr_; }
	CellApps & cellApps()			{ return cellApps_; }

	bool hasStarted() const			{ return hasStarted_; }

	Space * findSpace( SpaceID id );

	CellApp * findApp( const Mercury::Address & addr ) const;
	CellApp * findAlternateApp( CellApp * pApp ) const;

	enum ReadyComponent
	{
		READY_CELL_APP     = 0x1,
		READY_BASE_APP_MGR = 0x2,
		READY_BASE_APP     = 0x4,
		READY_ALL          =	READY_CELL_APP | READY_BASE_APP_MGR |
						    	READY_BASE_APP
	};

	// Watcher helper methods
	int numSpaces() const					{ return spaces_.size(); }
	int numCellApps() const					{ return cellApps_.size(); }
	int numActiveCellApps() const;
	int numCells() const;

	int numEntities() const;
	float minCellAppLoad() const;
	float avgCellAppLoad() const;
	float maxCellAppLoad() const;

	int cellsPerSpaceMax() const;
	float cellsPerMultiCellSpaceAvg() const;
	int numPartitions() const;
	BW::string cellAppGroups() const;
	int numMultiCellSpaces() const;
	int numMultiMachineSpaces() const;
	float machinesPerMultiCellSpaceAvg() const;
	int machinesPerMultiCellSpaceMax() const;
	int numMachinePartitions() const;

	void clearCellAppDeathHandler( const Mercury::Address & addr );

	void writeSpacesToDB();
	void writeGameTimeToDB();

	static bool shouldMetaLoadBalance()	{ return shouldMetaLoadBalance_; }
	static void setShouldMetaLoadBalance( bool newValue )
	{
		shouldMetaLoadBalance_ = newValue;
	}

	int numSpacesWithLoadedGeometry() const;

private:
	// From ServerApp
	virtual bool init( int argc, char *argv[] );

	void startTimer();

	void handleCellAppDeath( Mercury::Address addr );
	void sendUpdaterStepAck( int step );
	void checkForDeadCellApps();

	bool isRecovering() const	{ return isRecovering_; }
	bool startRecovery();
	void endRecovery();

	// Meta load balancing
	void metaLoadBalance();
	void loadBalance();
	void updateRanges();

	bool checkLoadingSpaces();

	void overloadCheck();
	bool calculateOverload( bool areCellAppsOverloaded,
							uint64 tolerancePeriodInStamps,
							uint64 &overloadStartTime,
							const char *warningMsg );

	Spaces spaces_;
	SpaceIDs spacesShuttingDown_;

	// Helper methods

	enum TimeOutType
	{
		TIMEOUT_GAME_TICK,
		TIMEOUT_LOAD_BALANCE,
		TIMEOUT_META_LOAD_BALANCE,
		TIMEOUT_OVERLOAD_CHECK,
		TIMEOUT_END_OF_RECOVERY
	};

	SpaceID generateSpaceID( SpaceID spaceID = NULL_SPACE_ID );
	void addSpace( Space * pSpace );
	void handleTimeout( TimerHandle handle, void * arg );

	CellApps cellApps_;

	typedef BW::map< BW::string, BW::string > SharedData;
	SharedData sharedCellAppData_;
	SharedData sharedGlobalData_;

	TimeKeeper * pTimeKeeper_;

	friend class CellAppMgrViewerConnection;
	friend class CellAppMgrCreateEntityHandler;

	BW::vector< CellApp * > pendingApps_;

	BaseAppMgr				baseAppMgr_;
	DBAppMgr				dbAppMgr_;
	DBApp					dbAppAlpha_;

	Mercury::Address baseAppAddr_;

	CellAppMgrViewerServer * pCellAppMgrViewerServer_;

	int					waitingFor_;
	int32				lastCellAppID_;
	SpaceID				lastSpaceID_;

	bool				allowNewCellApps_;
	bool				isRecovering_;
	bool				hasStarted_;
	bool				isShuttingDown_;

	uint64				avgCellAppOverloadStartTime_;
	uint64				maxCellAppOverloadStartTime_;

	Mercury::Address	lastCellAppDeath_;

	ShutDownHandler *	pShutDownHandler_;

	typedef BW::map< Mercury::Address, CellAppDeathHandler * >
		CellAppDeathHandlers;
	CellAppDeathHandlers cellAppDeathHandlers_;

	TimerHandle loadBalanceTimer_;
	TimerHandle metaLoadBalanceTimer_;
	TimerHandle overloadCheckTimer_;
	TimerHandle gameTimer_;
	TimerHandle recoveryTimer_;

	ServicesMap 		servicesMap_;
	static bool shouldMetaLoadBalance_;
};


#ifdef CODE_INLINE
#include "cellappmgr.ipp"
#endif

BW_END_NAMESPACE

#endif // CELLAPPMGR_HPP
