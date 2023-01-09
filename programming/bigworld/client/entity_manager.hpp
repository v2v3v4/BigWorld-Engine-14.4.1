#ifndef ENTITY_MANAGER_HPP
#define ENTITY_MANAGER_HPP

#include "bwclient_filter_environment.hpp"
#include "connection_control.hpp"
#include "entity.hpp"

#include "connection/filter_environment.hpp"

#include "connection_model/bw_entities_listener.hpp"
#include "connection_model/bw_replay_event_handler.hpp"
#include "connection_model/bw_space_data_listener.hpp"
#include "connection_model/bw_stream_data_handler.hpp"

#include "cstdmf/bw_map.hpp"
#include "cstdmf/bw_set.hpp"
#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_vector.hpp"

#include "network/space_data_mappings.hpp"

#include "urlrequest/request.hpp"

#include "network/basictypes.hpp"


BW_BEGIN_NAMESPACE

class ClientSpace;
class ReplayHeader;
class ReplayMetaData;
class ReplayURLHandler;
typedef SmartPointer< ReplayURLHandler > ReplayURLHandlerPtr;

typedef BW::vector< EntityPtr > EntitiesVector;
typedef SmartPointer< ClientSpace > ClientSpacePtr;



/**
 *	This class stores all the entities that exist on the client,
 *	and controls which ones should be displayed in the world.
 */
class EntityManager :
	public BWReplayEventHandler,
	public BWSpaceDataListener,
	public BWStreamDataHandler,
	public BWEntitiesListener
{
public:
	EntityManager();
	virtual ~EntityManager();

	static EntityManager & instance();

	void fini();

	// Overrides from BWReplayEventHandler
	void onReplayHeaderRead( const ReplayHeader & header ) /* override */;
	bool onReplayMetaData( const ReplayMetaData & metaData ) /* override */;
	void onReplayReachedEnd() /* override */;
	void onReplayError( const BW::string & error ) /* override */;
	void onReplayEntityClientChanged( const BWEntity * pEntity ) /* override */;
	void onReplayEntityAoIChanged( const BWEntity * pWitness,
		const BWEntity * pEntity, bool isEnter ) /* override */;
	void onReplayTicked( GameTime currentTick,
		GameTime totalTicks ) /* override */;
	void onReplaySeek( double dTime ) /* override */;
	void onReplayTerminated( ReplayTerminatedReason reason ) /* override */;

	// Overrides from BWSpaceDataListener
	void onGeometryMapping( SpaceID spaceID, Matrix mappingMatrix,
		const BW::string & mappingName );
	void onGeometryMappingDeleted( SpaceID spaceID, Matrix mappingMatrix,
		const BW::string & mappingName );
	void onTimeOfDay( SpaceID spaceID, float initialTime,
		float gameSecondsPerSecond );
	void onUserSpaceData( SpaceID spaceID, uint16 key, bool isInsertion,
		const BW::string & data );

	// Overrides from BWStreamDataHandler
	void onStreamDataComplete( uint16 streamID,
		const BW::string & rDescription, BinaryIStream & rData );
	
	// Overrides from BWEntitiesListener	
	virtual void onEntityEnter( BWEntity * pEntity ) /* override */ {}
	virtual void onEntityLeave( BWEntity * pEntity ) /* override */ {}
	virtual void onEntitiesReset();

	// EntityManager interface
	void tick( double timeNow, double timeLast );

	void addPendingEntity( EntityPtr pEntity );
	void clearPendingEntities();

	PyObject * entityIsDestroyedExceptionType();

	FilterEnvironment & filterEnvironment() { return filterEnvironment_; }

	// Replay related methods
	SpaceID startReplayFromFile( const BW::string & fileName,
		const BW::string & publicKey,
		PyObjectPtr pReplayHandler,
		int volatileInjectionPeriod );

	SpaceID startReplayFromURL( const BW::string & url,
		const BW::string & cacheFileName,
		bool shouldKeepCache,
		const BW::string & publicKey,
		PyObjectPtr pReplayHandler,
		int volatileInjectionPeriod );

	PyObjectPtr pPyReplayHandler() const { return pPyReplayHandler_; }

	void replayURLRequestAborted();
	void replayURLRequestFinished();

	void callReplayCallback( bool shouldClear,
		ConnectionControl::LogOnStage stage, 
		const char * status, const BW::string & message = BW::string( "" ));

	void callReplayHandler( bool shouldClear, const char * func, PyObject * args );
	bool callReplayHandlerBoolRet( bool shouldClear, const char * func, 
		PyObject * args );

private:
	void setTimeInt( ClientSpacePtr pSpace, GameTime gameTime,
		float initialTimeOfDay, float gameSecondsPerSecond );

	PyObjectPtr entityIsDestroyedExceptionType_;

	EntitiesVector pendingEntities_;

	URL::Request * pURLRequest_;
	ReplayURLHandlerPtr pReplayURLHandler_;
	PyObjectPtr pPyReplayHandler_;

	BWClientFilterEnvironment filterEnvironment_;
};

BW_END_NAMESPACE

#endif // ENTITY_MANAGER_HPP
