/*
 *	Class: CellApp - represents the Cell application.
 *
 *	This class is used as a singleton. This object represents the entire Cell
 *	application. It's main functionality is to handle the interface to this
 *	application and redirect the calls.
 */

#ifndef CELLAPP_HPP
#define CELLAPP_HPP

#include <Python.h>

#include "cellapp_death_listener.hpp"
#include "cellapp_interface.hpp"
#include "cellappmgr_gateway.hpp"
#include "cells.hpp"
#include "emergency_throttle.hpp"
#include "profile.hpp"

#include "chunk_loading/geometry_mapper.hpp"
#include "chunk_loading/preloaded_chunk_space.hpp"

#include "cstdmf/memory_stream.hpp"
#include "cstdmf/singleton.hpp"
#include "cstdmf/stringmap.hpp"

#include "entitydef/entity_description_map.hpp"

#include "pyscript/pickler.hpp"

#include "server/common.hpp"
#include "server/entity_app.hpp"
#include "server/id_client.hpp"
#include "server/py_services_map.hpp"

#include "script/script_object.hpp"


BW_BEGIN_NAMESPACE

class Cell;
class CellAppChannels;
class Entity;
class SharedData;
class Space;
class Spaces;
class TimeKeeper;
class Witness;
struct CellAppInitData;
class CellViewerServer;

typedef Mercury::ChannelOwner DBApp;

class BufferedEntityMessages;
class BufferedGhostMessages;
class BufferedInputMessages;
class CellAppConfig;

namespace Terrain
{
	class Manager;
}

/**
 *	This singleton class represents the entire application.
 */
class CellApp : public EntityApp, public TimerHandler, GeometryMapper,
	public Singleton< CellApp >
{
public:
	typedef CellAppConfig Config;

private:
	enum TimeOutType
	{
		TIMEOUT_GAME_TICK,
		TIMEOUT_TRIM_HISTORIES,
		TIMEOUT_LOADING_TICK
	};

public:
	ENTITY_APP_HEADER( CellApp, cellApp )

	/// @name Construction/Initialisation
	//@{
	CellApp( Mercury::EventDispatcher & mainDispatcher,
			Mercury::NetworkInterface & interface );
	virtual ~CellApp();

	bool finishInit( const CellAppInitData & initData );

	void preloadSpaces();

	void onGetFirstCell( bool isFromDB );

	//@}

	/// @name Message handlers
	//@{
	void addCell( const Mercury::Address & srcAddr,
			const Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data );
	void startup( const CellAppInterface::startupArgs & args );
	void setGameTime( const CellAppInterface::setGameTimeArgs & args );

	void handleCellAppMgrBirth(
		const CellAppInterface::handleCellAppMgrBirthArgs & args );
	void addCellAppMgrRebirthData( BinaryOStream & stream );

	void handleCellAppDeath(
		const CellAppInterface::handleCellAppDeathArgs & args );

	void handleBaseAppDeath( BinaryIStream & data );

	virtual void shutDown();
	void shutDown( const CellAppInterface::shutDownArgs & args );
	void controlledShutDown(
			const CellAppInterface::controlledShutDownArgs & args );

	void triggerControlledShutDown();

	void setSharedData( BinaryIStream & data );
	void delSharedData( BinaryIStream & data );

	void addServiceFragment( BinaryIStream & data );
	void delServiceFragment( BinaryIStream & data );

	void setBaseApp( const CellAppInterface::setBaseAppArgs & args );
	void setDBAppAlpha( const CellAppInterface::setDBAppAlphaArgs & args );

	void onloadTeleportedEntity( const Mercury::Address & srcAddr,
		const Mercury::UnpackedMessageHeader & header, BinaryIStream & data );

	void cellAppMgrInfo( const CellAppInterface::cellAppMgrInfoArgs & args );

	void handleOnBaseOffloadedFailure( EntityID entityID,
			const Mercury::Address &oldBaseAddr,
			const CellAppInterface::onBaseOffloadedArgs & args );

	void onRemoteTickComplete( const Mercury::Address & srcAddr,
			const Mercury::UnpackedMessageHeader & header,
			const CellAppInterface::onRemoteTickCompleteArgs & args );

	//@}

	/// @name Utility methods
	//@{
	Entity * findEntity( EntityID id ) const;

	void entityKeys( PyObject * pList ) const;
	void entityValues( PyObject * pList ) const;
	void entityItems( PyObject * pList ) const;

	BW::string pickle( ScriptObject args );
	ScriptObject unpickle( const BW::string & str );
	PyObject * newClassInstance( PyObject * pyClass,
			PyObject * pDictionary );

	bool reloadScript( bool isFullReload );
	//@}

	/// @name Accessors
	//@{
	Cell *	findCell( SpaceID id ) const;
	Space *	findSpace( SpaceID id ) const;

	static Mercury::UDPChannel & getChannel( const Mercury::Address & addr )
	{
		return CellApp::instance().interface_.findOrCreateChannel( addr );
	}

	CellAppMgrGateway & cellAppMgr()			{ return cellAppMgr_; }
	DBApp & dbAppAlpha()						{ return dbAppAlpha_; }

	float getLoad() const						{ return persistentLoad_; }
	float getEntityLoad() const					{ return totalEntityLoad_; }
	float getPerEntityLoadShare() const			{ return perEntityLoadShare_; }

	void adjustLoadForOffload( float entityLoad );

	uint64 lastGameTickTime() const				{ return lastGameTickTime_; }

	Cells & cells()								{ return cells_; }
	const Cells & cells() const					{ return cells_; }

	const Spaces & spaces() const				{ return *pSpaces_; }

	bool hasStarted() const					{ return gameTimer_.isSet(); }
	bool isShuttingDown() const					{ return shutDownTime_ != 0; }

	int numRealEntities() const;

	float maxCellAppLoad() const				{ return maxCellAppLoad_; }
	float emergencyThrottle() const			{ return throttle_.value(); }

	const Mercury::Address & baseAppAddr() const	{ return baseAppAddr_; }

	ServicesMap & servicesMap() const	{ return pPyServicesMap_->map(); }

	IDClient & idClient()				{ return idClient_; }
	//@}

	/// @name Update methods
	//@{
	bool nextTickPending() const;	// are we running out of time?
	//@}

	/// @name Misc
	//@{
	void destroyCell( Cell * pCell );

	void detectDeadCellApps(
		const BW::vector< Mercury::Address > & addrs );
	//@}

	BufferedEntityMessages & bufferedEntityMessages()
		{ return *pBufferedEntityMessages_; }
	BufferedGhostMessages & bufferedGhostMessages()
		{ return *pBufferedGhostMessages_; }
	BufferedInputMessages & bufferedInputMessages()
		{ return *pBufferedInputMessages_; }

	void callWatcher( const Mercury::Address& srcAddr,
		const Mercury::UnpackedMessageHeader& header,
		BinaryIStream & data );

	bool shouldOffload() const { return shouldOffload_; }
	void shouldOffload( bool b ) { shouldOffload_ = b; }

	int id() const				{ return id_; }

	virtual void onSignalled( int sigNum );

	bool registerWitness( Witness * pObject, int level = 0 );
	bool deregisterWitness( Witness * pObject );

	SpaceEntryID nextSpaceEntryID();

	virtual void onSpaceGeometryLoaded(
			SpaceID spaceID, const BW::string & name );

	bool hasCalledWitnesses() const { return hasCalledWitnesses_; }

private:
	
	// Override from ServerApp
	ManagerAppGateway * pManagerAppGateway() /* override */
	{
		return &cellAppMgr_;
	}

	virtual bool init( int argc, char *argv[] );

	virtual void onStartOfTick();

	virtual void onEndOfTick();

	virtual void onTickProcessingComplete();

	// Methods
	bool initExtensions();
	bool initScript();

	void addWatchers();

	void checkSendWindowOverflows();

	void checkPython();

	int secondsToTicks( float seconds, int lowerBound );

	void startGameTime();

	void sendShutdownAck( ShutDownStage stage );

	bool inShutDownPause() const
	{
		return (shutDownTime_ != 0) && (time_ == shutDownTime_);
	}

	size_t numSpaces() const;

	/// @name Overrides
	//@{
	void handleTimeout( TimerHandle handle, void * arg );
	//@}
	void handleGameTickTimeSlice();
	void handleTrimHistoriesTimeSlice();

	void tickShutdown();

	uint64 calcTickPeriod();
	double calcTransientLoadTime();
	double calcSpareTime();
	double calcThrottledLoadTime();

	void checkTickWarnings( double persistentLoadTime, double tickTime,
		   double spareTime );

	void addToLoad( float timeSpent, float & result ) const;
	void updateLoad();
	void tickProfilers( uint64 lastTickInStamps );

	void updateBoundary();
	void tickBackup();
	void checkOffloads();
	void syncTime();

	float numSecondsBehind() const;

	void callWitnesses( bool checkReceivedTime );

	// Data
	Cells			cells_;
	Spaces *		pSpaces_;

	typedef StringHashMap< PreloadedChunkSpace * > PreloadedChunkSpaces;
	PreloadedChunkSpaces		preloadedSpaces_;

	// Must be before dbAppAlpha_ as dbAppAlpha_ destructor can cancel pending
	// requests and call back to idClient_.
	IDClient				idClient_;

	CellAppMgrGateway		cellAppMgr_;
	DBApp					dbAppAlpha_;

	GameTime		shutDownTime_;
	TimeKeeper * 	pTimeKeeper_;

	Pickler * pPickler_;

	float					timeoutPeriod_;

	// Used for throttling back
	EmergencyThrottle throttle_;

	uint64					lastGameTickTime_;
	timeval					oldTimeval_;

	SharedData *			pCellAppData_;
	SharedData *			pGlobalData_;

	Mercury::Address		baseAppAddr_;

	int						backupIndex_;

	TimerHandle				gameTimer_;
	TimerHandle				loadingTimer_;
	TimerHandle				trimHistoryTimer_;
	uint64					reservedTickTime_;

	CellViewerServer *		pViewerServer_;
	CellAppID				id_;

	float					persistentLoad_;
	float					transientLoad_;
	float					totalLoad_;
	float					totalEntityLoad_;
	float					totalAddedLoad_;
	float					perEntityLoadShare_;

	float					maxCellAppLoad_;

	bool					shouldOffload_;
	bool 					hasAckedCellAppMgrShutDown_;
	bool					hasCalledWitnesses_;
	bool					isReadyToStart_;


	BufferedEntityMessages * pBufferedEntityMessages_;
 	BufferedGhostMessages *  pBufferedGhostMessages_;
	BufferedInputMessages * pBufferedInputMessages_;

	CellAppChannels *		pCellAppChannels_;

	FileIOTaskManager		fileIOTaskManager_;

	Updatables				witnesses_;

	PyServicesMap			* pPyServicesMap_;

	typedef std::auto_ptr< Terrain::Manager > TerrainManagerPtr;
	TerrainManagerPtr pTerrainManager_;

	friend class CellAppResourceReloader;

	uint16 nextSpaceEntryIDSalt_;
};



#ifdef CODE_INLINE
#include "cellapp.ipp"
#endif

BW_END_NAMESPACE

#endif // CELLAPP_HPP
