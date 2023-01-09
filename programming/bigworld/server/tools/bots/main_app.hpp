#ifndef MAIN_APP_HPP
#define MAIN_APP_HPP

#include "Python.h"

#include "bots_config.hpp"

#include "chunk_loading/geometry_mapper.hpp"
#include "chunk_loading/preloaded_chunk_space.hpp"

#include "connection/condemned_interfaces.hpp"
#include "connection/login_challenge_factory.hpp"

#include "cstdmf/bgtask_manager.hpp"
#include "cstdmf/bw_map.hpp"
#include "cstdmf/md5.hpp"
#include "cstdmf/singleton.hpp"

#include "network/event_dispatcher.hpp"
#include "network/network_interface.hpp"

#include "pyscript/script.hpp"
#include "pyscript/script_math.hpp"

#include "script/script_object.hpp"
#include "server/server_app.hpp"
#include "server/script_timers.hpp"

#include "terrain/manager.hpp"

#include <memory>


BW_BEGIN_NAMESPACE

class MainApp;
class ClientApp;
class StreamEncoder;
class MovementController;
class MovementFactory;
class PythonServer;
class PreloadedChunkSpace;
typedef ScriptObjectPtr< ClientApp > ClientAppPtr;


/**
 *	This ScriptTimeQueue subclass is specialised for MainApp's time
 */
class MainAppTimeQueue : public ScriptTimeQueue
{
public:
	MainAppTimeQueue( int updateHertz, MainApp & mainApp );
	virtual GameTime time() const;

private:
	MainApp & mainApp_;
};


class MainApp : public ServerApp,
		public GeometryMapper,
		public TimerHandler,
		public Singleton< MainApp >
{
private:
	enum TimeOutType
	{
		TIMEOUT_GAME_TICK,
		TIMEOUT_CHUNK_TICK
	};
public:
	typedef BotsConfig Config;

	SERVER_APP_HEADER( Bots, bots )

	MainApp( Mercury::EventDispatcher & mainDispatcher, 
		Mercury::NetworkInterface & networkInterface );
	virtual		~MainApp();

	// ServerApp overrides
	virtual bool init( int argc, char * argv[] );
	virtual void onTickProcessingComplete();

	// GeometryMapper overrides
	virtual void onSpaceGeometryLoaded( SpaceID spaceID,
			const BW::string & name );

	// TimerHandler overrides
	virtual void handleTimeout( TimerHandle timerHandle, void * args );

	void handleGameTickTimeSlice();
	void handleChunkTickTimeSlice();

	PreloadedChunkSpace * addSpaceGeometryMapping( ChunkSpaceID spaceID,
		BW::Matrix & matrix, const BW::string & name );

	PyObject * addSpaceGeometryMapping( ChunkSpaceID spaceID,
		MatrixProviderPtr pMapper, const BW::string & path );

	void addBot();
	void addBots( int num );
	void addBotsWithName( PyObjectPtr logInfoData );
	void delBots( int num );

	void delTaggedEntities( BW::string tag );

	void updateMovement( BW::string tag );

	MovementController * createDefaultMovementController( float & speed,
		Vector3 & position );
	MovementController * createMovementController( float & speed,
		Vector3 & position, const BW::string & controllerType,
		const BW::string & controllerData );

	double localTime() const { return localTime_; }

	static void addFactory( const BW::string & name,
									MovementFactory & factory );


	// ---- Accessors ----
	StreamEncoder * pLogOnParamsEncoder()
		{ return pLogOnParamsEncoder_.get(); }

	double sendTimeReportThreshold() const
		{ return sendTimeReportThreshold_; }

	CondemnedInterfaces & condemnedInterfaces()
		{ return condemnedInterfaces_; }

	void sendTimeReportThreshold( double threshold );

	// ---- Script related Methods ----
	ClientAppPtr findApp( EntityID id ) const;
	void appsKeys( PyObject * pList ) const;
	void appsValues( PyObject * pList ) const;
	void appsItems( PyObject * pList ) const;

	// ---- Get personality module ----
	ScriptObject getPersonalityModule() const;

	LoginChallengeFactories & loginChallengeFactories()
	{
		return loginChallengeFactories_;
	}

private:
	void parseCommandLine( int argc, char * argv[] );
	bool initScript();
	void initWatchers();
	bool initLogOnParamsEncoder( const BW::string & path );

	void addBotWithName( const BW::string & botName,
		const BW::string & botPassword );

	std::auto_ptr< StreamEncoder > 	pLogOnParamsEncoder_;

	typedef BW::list< ClientAppPtr > Bots;
	Bots bots_;
	Bots botsForLogOnRetry_;

	double localTime_;
	TimerHandle gameTimer_;
	TimerHandle chunkTimer_;
	double sendTimeReportThreshold_;

	MainAppTimeQueue timeQueue_;

	PythonServer *	pPythonServer_;

	Bots::iterator clientTickIndex_;

	BW::string appID_;
	int	nextBotID_;

	uint16 nextSpaceEntryIDSalt_;

	BgTaskManager			bgTaskManager_;
	FileIOTaskManager		fileIOTaskManager_;

	Terrain::Manager		terrainManager_;

	typedef BW::map< SpaceID, PreloadedChunkSpace * > Spaces;
	Spaces					spaces_;

	LoginChallengeFactories loginChallengeFactories_;
	CondemnedInterfaces		condemnedInterfaces_;
};

BW_END_NAMESPACE

#endif // MAIN_APP_HPP
