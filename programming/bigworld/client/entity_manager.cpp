#include "pch.hpp"

#include "entity_manager.hpp"

#include "connection_control.hpp"
#include "entity_type.hpp"

#include "app.hpp"
#include "app_config.hpp"
#include "filter.hpp"
#include "script_player.hpp"
#include "py_entity.hpp"

#include "common/space_data_types.hpp"

#include "camera/annal.hpp"

#include "space/space_manager.hpp"
#include "space/client_space.hpp"

#include "chunk/chunk_space.hpp"
#include "chunk_scene_adapter/client_chunk_space_adapter.hpp"
#include "chunk/geometry_mapping.hpp"

#include "connection/login_handler.hpp"
#include "connection/replay_header.hpp"
#include "connection/replay_metadata.hpp"
#include "connection/smart_server_connection.hpp"

#include "connection_model/bw_connection.hpp"
#include "connection_model/bw_null_connection.hpp"
#include "connection_model/bw_replay_connection.hpp"
#include "connection_model/bw_server_connection.hpp"

#include "cstdmf/bgtask_manager.hpp"
#include "cstdmf/binaryfile.hpp"
#include "cstdmf/debug.hpp"
#include "cstdmf/memory_stream.hpp"

#include "input/py_input.hpp"

#include "network/basictypes.hpp"
#include "network/space_data_mapping.hpp"

#include "pyscript/personality.hpp"

#include "romp/time_of_day.hpp"

#include "urlrequest/manager.hpp"
#include "urlrequest/response.hpp"


DECLARE_DEBUG_COMPONENT2( "Entity", 0 );


BW_BEGIN_NAMESPACE

const uint16 SPACE_DATA_WEATHER = 8192;

#if ENABLE_ENVIRONMENT_SYNC

// flag indicating environmental sync state of demo client
static bool environmentSync = true;

void setEnvironmentSync( bool sync )
{
	environmentSync = sync;
}

PY_AUTO_MODULE_FUNCTION( RETVOID, setEnvironmentSync, ARG( bool, END ), BigWorld )

bool getEnvironmentSync()
{
	return environmentSync;
}

PY_AUTO_MODULE_FUNCTION( RETDATA, getEnvironmentSync, END, BigWorld )

#endif // ENABLE_ENVIRONMENT_SYNC

class EntityManager;
// -----------------------------------------------------------------------------
// Section: EntityManager
// -----------------------------------------------------------------------------

/**
 *	 Default constructor.
 */
EntityManager::EntityManager() :
	entityIsDestroyedExceptionType_(),
	pendingEntities_(),
	pURLRequest_(),
	pPyReplayHandler_(),
	filterEnvironment_()
{
	ScriptPlayer::init();

	PyObject * bigworldModule = PyImport_AddModule( "BigWorld" );
	entityIsDestroyedExceptionType_ =
		PyObjectPtr(
			PyErr_NewException(
				const_cast<char *>("BigWorld.EntityIsDestroyedException"),
				NULL, NULL ),
			PyObjectPtr::STEAL_REFERENCE );

	Py_XINCREF(entityIsDestroyedExceptionType_.get());
	PyModule_AddObject( bigworldModule, "EntityIsDestroyedException",
		entityIsDestroyedExceptionType_.getObject() );
}


/**
 *	 Destructor.
 */
EntityManager::~EntityManager()
{}


/**
 *	 Instance accessor.
 */
EntityManager & EntityManager::instance()
{
	static EntityManager ec;

	return ec;
}


BW_END_NAMESPACE
#include "cstdmf/profile.hpp"
BW_BEGIN_NAMESPACE

namespace // (anonymous)
{
class TickEntityVisitor : public EntityVisitor
{
public:
	TickEntityVisitor( double timeNow, double timeLast ):
		timeNow_( timeNow ),
		timeLast_( timeLast )
	{}

	virtual ~TickEntityVisitor() {}

	virtual bool onVisitEntity( Entity * pEntity )
	{
		pEntity->tick( timeNow_, timeLast_ );
		return true;
	}
private:
	double timeNow_;
	double timeLast_;

};

} // end namespace (anonymous)


/**
 *	This function is called to call the tick functions of all the
 *	entities. Note: These functions do not do things like animation -
 *	that is left to Models that are actually in the scene graph.
 *
 *	@param	timeNow		current timestamp
 *	@param	timeLast	timestamp when this method was called last.
 */
void EntityManager::tick( double timeNow, double timeLast )
{
	BW_GUARD;

	// see if any of the entities waiting on prerequisites or vehicles
	// are now ready
	size_t i = 0;
	while (i < pendingEntities_.size())
	{
		EntityPtr pEntity = pendingEntities_[i];

		bool isStillLoading = false;

		IF_NOT_MF_ASSERT_DEV( !pEntity->isInWorld() )
		{
			// An entity got into the pending list twice due to a bug.
			pEntity->abortLoadingPrerequisites();
		}

		if (!pEntity->isInAoI())
		{
			MF_ASSERT( pEntity->isDestroyed() || pEntity->isPlayer() );

			// The player may lose its cell-side without being destroyed.
			if (pEntity->isPlayer())
			{
				pEntity->abortLoadingPrerequisites();
			}

			// Destroyed entities abort their loaders, so we just need to
			// drop them from our list when we see them.
			MF_ASSERT( !pEntity->loadingPrerequisites() );
		}
		else
		{
			isStillLoading = pEntity->loadingPrerequisites() &&
				!pEntity->checkPrerequisites();
		}

		if (isStillLoading)
		{
			++i;
			continue;
		}

		std::swap<>( pendingEntities_[i], pendingEntities_.back() );
		pendingEntities_.pop_back();
	}

	ConnectionControl & connectionControl = ConnectionControl::instance();

	BWReplayConnection * pReplayConn = connectionControl.pReplayConnection();

	ReplayController * pReplayController;

	if (pReplayConn != NULL &&
		(pReplayController = pReplayConn->pReplayController()) != NULL &&
		pReplayController->getCurrentTick() == 1 &&
		gWorldDrawEnabled &&
		pendingEntities_.empty())
	{
		// The world has loaded and entity prerequisites have loaded,
		// so proceed with the replay playback.
		pReplayConn->onInitialDataProcessed();
	}

	connectionControl.visitEntities( TickEntityVisitor( timeNow, timeLast ) );
}


/**
 *	This method adds an Entity to our pendingEntities collection, so that its
 *	prerequisite loader will be ticked every tick
 */
void EntityManager::addPendingEntity( EntityPtr pEntity )
{
	BW_GUARD;

	MF_ASSERT( std::find( pendingEntities_.begin(), pendingEntities_.end(),
		pEntity ) == pendingEntities_.end() );

	pendingEntities_.push_back( pEntity );
}


/**
 *	This method is called when disconnecting from the server, to clean up
 *	any still-pending entities.
 */
void EntityManager::clearPendingEntities()
{
	BW_GUARD;

	pendingEntities_.clear();
}


/**
 *	This method sets the function which determines when the time
 *	of day from game time.
 *
 *	@param	pSpace					pointer to space whose time-of-day
 *									should be set.
 *	@param	gameTime				current game time.
 *	@param	initialTimeOfDay		the initial time of day
 *	@param	gameSecondsPerSecond	number of game seconds ellapsed
 *									per real world seconds.
 */
void EntityManager::setTimeInt( ClientSpacePtr pSpace, GameTime gameTime,
	float initialTimeOfDay, float gameSecondsPerSecond )
{
	BW_GUARD;

	TimeOfDay & timeOfDay = *pSpace->enviro().timeOfDay();
	timeOfDay.updateNotifiersOn( false );

	TRACE_MSG( "EntityManager::setTimeInt: gameTime %d initial %f gsec/sec %f\n",
		gameTime, initialTimeOfDay, gameSecondsPerSecond );

	float tod;
	if ( gameSecondsPerSecond > 0.f )
	{
		BWServerConnection * pServerConnectionModel =
			ConnectionControl::instance().pServerConnection();
		MF_ASSERT( pServerConnectionModel );
		const float updateFrequency = 
			pServerConnectionModel->pServerConnection()->updateFrequency();

		timeOfDay.secondsPerGameHour((1.0f / gameSecondsPerSecond) * 3600.0f);
		DEBUG_MSG( "\tsec/ghour = %f\n", (1.0f / gameSecondsPerSecond) * 3600.0f );
		// This gives us the time of day as in game seconds, since the start of time.
		tod = initialTimeOfDay + ((float)gameTime /
			updateFrequency *
			(float)gameSecondsPerSecond);
	}
	else
	{
		timeOfDay.secondsPerGameHour(0.f);
		DEBUG_MSG( "\tsec/ghour = 0.0\n" );
		tod = initialTimeOfDay;
	}

	DEBUG_MSG( "\ttherefore tod in seconds = %f\n", tod );

	// Set the time of day in hours.
	timeOfDay.gameTime(tod / 3600.0f);
	timeOfDay.tick(0.0f);

	timeOfDay.updateNotifiersOn( true );
}


void EntityManager::onTimeOfDay( SpaceID spaceID, float initialTime,
								float gameSecondsPerSecond )
{	BW_GUARD;

	ClientSpacePtr pSpace = SpaceManager::instance().space( spaceID );

	MF_ASSERT( pSpace != NULL );

	// TODO: This shouldn't depend on having a ServerConnection, surely?
	BWServerConnection * pServerConn;
	if ((pServerConn = ConnectionControl::instance().pServerConnection())
#if ENABLE_ENVIRONMENT_SYNC
		&& environmentSync
#endif
		)
	{
		this->setTimeInt( pSpace,
			pServerConn->pServerConnection()->gameTimeOfLastMessage(),
			initialTime, gameSecondsPerSecond );
	}
}


void EntityManager::onUserSpaceData( SpaceID spaceID, uint16 key,
	bool isInsertion, const BW::string & data )
{
	BW_GUARD;

	ClientSpacePtr pSpace = SpaceManager::instance().space( spaceID );

	MF_ASSERT( pSpace != NULL );

	if (key == SPACE_DATA_WEATHER && isInsertion)
	{
		Personality::instance().callMethod( "onWeatherChange",
			ScriptArgs::create( spaceID, data ),
			ScriptErrorPrint( "EntityManager::spaceData weather notifier: " ),
			/* allowNullMethod */ true );
	}
}


/*~ callback Entity.onGeometryMapped
 *
 *	This callback method tells the player entity about changes to
 *	the geometry in its current space.  It is called when geometry
 *	is mapped into the player's current space.  The name of the
 *	spaces geometry is passed in as a parameter
 *
 *	@param spaceName	name describing the space's geometry
 */
/*~ function BWPersonality.onGeometryMapped
 *	@components{ client }
 *	This callback method tells the player entity about changes to
 *	the geometry in a space.  It is called when geometry is mapped
 *	into any of the currently existing spaces on the client. The
 *	space id and name of the space geometry is passed in as parameters.
 *
 *	@param spaceID		id of the space the geometry is being mapped in to
 *	@param spaceName	name describing the space's geometry
 */
void EntityManager::onGeometryMapping( SpaceID spaceID, Matrix mappingMatrix,
	const BW::string & mappingName )
{
	BW_GUARD;

	ClientSpacePtr pSpace = SpaceManager::instance().space( spaceID );

	MF_ASSERT( pSpace != NULL );

	const SpaceDataMapping & spaceDataMapping = pSpace->spaceData();

	// Find the SpaceEntryID for this mapping.
	// TODO: ClientSpace::addMapping shouldn't require a key. It should key off
	// the mapping data.
	SpaceDataMapping::DataEntryMap::const_iterator iMapping;
	for (iMapping = spaceDataMapping.begin();
		iMapping != spaceDataMapping.end(); ++iMapping)
	{
		if (iMapping->second.key() != SPACE_DATA_MAPPING_KEY_CLIENT_ONLY &&
			iMapping->second.key() != SPACE_DATA_MAPPING_KEY_CLIENT_SERVER)
		{
			continue;
		}

		const BW::string & data = iMapping->second.data();

		if (data.size() != mappingName.size() + sizeof( SpaceData_MappingData ))
		{
			continue;
		}

		SpaceData_MappingData & mappingData =
			*(SpaceData_MappingData *)data.data();
		Matrix & matrix = *(Matrix *)&mappingData.matrix[0][0];

		if (matrix != mappingMatrix)
		{
			continue;
		}

		BW::string path( (char *)(&mappingData + 1),
			data.size() - sizeof( SpaceData_MappingData ) );

		if (path != mappingName)
		{
			continue;
		}

		break;
	}

	MF_ASSERT( iMapping != spaceDataMapping.end() );
	SpaceEntryID seid = iMapping->first;

	pSpace->addMapping( *(ClientSpace::GeometryMappingID*)&seid, mappingMatrix,
		mappingName );

	// tell the player about this if it is relevant to it
	// this system probably wants to be expanded in future
	Entity * pPlayer = ScriptPlayer::entity();
	if (pPlayer && !pPlayer->isDestroyed() &&
		pPlayer->pSpace() == pSpace)
	{
		pPlayer->pPyEntity().callMethod( "onGeometryMapped",
			ScriptArgs::create( mappingName ), ScriptErrorPrint(),
			/* allowNullMethod */ true );
		ScriptPlayer::instance().updateWeatherParticleSystems(
			pSpace->enviro().playerAttachments() );
	}

	Personality::instance().callMethod( "onGeometryMapped",
		ScriptArgs::create( spaceID, mappingName ),
		ScriptErrorPrint( "EntityManager::spaceData geometry notifier: " ),
		/* allowNullMethod */ true );
}


void EntityManager::onGeometryMappingDeleted( SpaceID spaceID,
	Matrix mappingMatrix, const BW::string & mappingName )
{
	BW_GUARD;

	ClientSpacePtr pSpace = SpaceManager::instance().space( spaceID );

	MF_ASSERT( pSpace != NULL );

	// TODO: ClientSpace::delMapping should not be keyed by EntryID.
	// For now, search pSpace->getMappings() for a matching mapping. Any mapping
	// that matches would be suitable.
	// TODO: This is going behind the ClientSpace API's back as well.
	// We can't search SpaceData like we do in onGeometryMapping, because it's
	// already been deleted.

	ChunkSpacePtr pChunkSpace =
		ClientChunkSpaceAdapter::getChunkSpace( pSpace );

	MF_ASSERT( pChunkSpace != NULL );

	const ChunkSpace::GeometryMappings & mappings = pChunkSpace->getMappings();
	ChunkSpace::GeometryMappings::const_iterator iMapping;

	for (iMapping = mappings.begin(); iMapping != mappings.end(); ++iMapping)
	{
		// "/" because GeometryMapping's constructor does it.
		if (iMapping->second->path() == (mappingName + "/") &&
			iMapping->second->mapper() == mappingMatrix)
		{
			break;
		}
	}

	if (iMapping == mappings.end())
	{
		ERROR_MSG( "EntityManager::onGeometryMappingDeleted: "
				"Tried to delete a mapping which doesn't exist." );
		return;
	}

	pSpace->delMapping( *(ClientSpace::GeometryMappingID*)&(iMapping->first) );
}


/*
 *	Override from BWEntitiesListener.
 */
void EntityManager::onEntitiesReset()
{
	// TODO: Defer this until the space is destroyed so as to avoid hitching
	// the frame rate.
	PyGC_Collect();
}


/**
 *	Finalises the EntityManager.
 */
void EntityManager::fini()
{
	BW_GUARD;

	// Clear entities in BWConnections
	ConnectionControl::instance().fini();

	ScriptPlayer::fini();
}


/**
 *	This function returns the Python type object of the EntityIsDestroyedException.
 *
 *	@return	Returns a borrowed pointer to the EntityIsDestroyedException type object.
 */
PyObject * EntityManager::entityIsDestroyedExceptionType()
{
	return entityIsDestroyedExceptionType_.getObject();
}


// -----------------------------------------------------------------------------
// Section: Replay handling
// -----------------------------------------------------------------------------


/**
 *	This class handles data retrieved from a URL request.
 */
class ReplayURLHandler : public URL::Response
{
public:
	/**
	 *	Constructor.
	 */
	ReplayURLHandler( EntityManager & entityManager ) :
			URL::Response(),
			responseCode_( 0 ),
			entityManager_( entityManager )
	{
	}


	/**
	 *	Destructor.
	 */
	virtual ~ReplayURLHandler() {}

	// Overrides from URL::Response
	virtual bool onResponseCode( long responseCode );
	virtual bool onContentTypeHeader( const BW::string & contentType );
	virtual bool onData( const char * data, size_t len );
	virtual void onDone( int responseCode, const char * error );

private:
	long					responseCode_;
	EntityManager &			entityManager_;
};


/*
 *	Override from URL::Response.
 */
bool ReplayURLHandler::onResponseCode( long responseCode )
{
	TRACE_MSG( "ReplayURLHandler::onResponseCode: %ld\n",
		responseCode );

	responseCode_ = responseCode;

	bool success = (responseCode == URL::RESPONSE_ERR_OK);
	BW::string errorString; 
	
	if (responseCode == 0)
	{
		ERROR_MSG( "ReplayURLHandler::onResponseCode: Got response code of 0\n" );

		errorString = "Got unexpected response code of 0";
	}
	else if (responseCode != URL::RESPONSE_ERR_OK)
	{
		errorString = BW::string( "Failed to retrieve URL: " ) +
				URL::httpResponseCodeToString( responseCode );
	}

	if (!success)
	{
		ReplayURLHandlerPtr pThisHolder( this );

		entityManager_.replayURLRequestAborted();
		entityManager_.callReplayCallback( /* shouldClear */ true,
			ConnectionControl::STAGE_LOGIN,
			"REPLAY_FAILED", errorString );
	}
	
	// Return true even if we have error, no need for cURL to signal it now.
	return true;
}


/*
 *	Override from URL::Response.
 */
bool ReplayURLHandler::onContentTypeHeader( const BW::string & contentType )
{
	// Should accept either binary/octet-stream as replay files would look to a
	// web server. Should not accept e.g. text/html.
	static const char EXPECTED_CONTENT_TYPE[] = "application/octet-stream";

	if (contentType != EXPECTED_CONTENT_TYPE )
	{
		// We check later if the actual contents of the data indicate that this
		// is not valid replay file.
		NOTICE_MSG( "ReplayURLHandler::onContentTypeHeader: "
				"Received invalid content type: \"%s\" (expected \"%s\")\n",
			contentType.c_str(), EXPECTED_CONTENT_TYPE );

		ReplayURLHandlerPtr pThisHolder( this );

		entityManager_.replayURLRequestAborted();
		entityManager_.callReplayCallback( /* shouldClear */ true,
			ConnectionControl::STAGE_LOGIN, "CONNECTION_FAILED",
			BW::string( "URL has invalid content type: " ) +
				contentType );
	}
	else
	{
		TRACE_MSG( "ReplayURLHandler::onContentTypeHeader: %s\n",
			contentType.c_str() );
	}

	return true;
}


/*
 *	Override from URL::Response.
 */
bool ReplayURLHandler::onData( const char * data, size_t len )
{
	// If we had a bad response code, we should have been cancelled 
	// before now.
	MF_ASSERT( responseCode_ == URL::RESPONSE_ERR_OK );

	MemoryIStream stream( data, static_cast<int>(len) );
	ConnectionControl::instance().pReplayConnection()->addReplayData( stream );

	return true;
}


/*
 *	Override from URL::Response.
 */
void ReplayURLHandler::onDone( int responseCode, const char * error )
{
	bool success = (error == NULL) && (responseCode == URL::RESPONSE_ERR_OK);

	BW::string errorString;
	if (error != NULL)
	{
		errorString = error;
	}
	else if ((responseCode != URL::RESPONSE_ERR_OK) &&
			(responseCode != 0))
	{
		errorString = "Bad response from server";
	}

	// Replay connection can be cleared between request cancel and done
	BWReplayConnection * pReplayConn =
		ConnectionControl::instance().pReplayConnection();

	if (!success)
	{
		ConnectionControl::LogOnStage stage = (responseCode == 0) ?
			ConnectionControl::STAGE_LOGIN :
			ConnectionControl::STAGE_DISCONNECTED;

		ReplayURLHandlerPtr pThisHolder( this );
		
		if (pReplayConn)
		{
			entityManager_.replayURLRequestAborted();
		}

		entityManager_.callReplayCallback( /* shouldClear */ true,
			stage, "CONNECTION_FAILED",
			errorString );
		
		return;
	}

	if (pReplayConn && pReplayConn->pReplayController())
	{
		pReplayConn->pReplayController()->replayDataFinalise();
	}

	entityManager_.replayURLRequestFinished();
}


/**
 *	This method calls a callback on the replay callback object.
 *
 *	@param shouldClear	Whether the replay handler should be cleared while it 
 *						is being called. This is typically true for the last
 *						callback.
 *	@param stage 		The connection stage.
 *	@param status 		The status code.
 *	@param message 		The message.
 */
void EntityManager::callReplayCallback( bool shouldClear,
		ConnectionControl::LogOnStage stage,
		const char * status, const BW::string & message )
{
	if (ConnectionControl::instance().pReplayConnection() == NULL)
	{
		MF_ASSERT( pURLRequest_ == NULL );
		MF_ASSERT( pPyReplayHandler_ == NULL );
		return;
	}

	this->callReplayHandler( shouldClear, "onProgress", 
		Py_BuildValue( "(iss)", stage,
		status, message.c_str() ) );
}


/**
 *	This method is called when the URL request failed for any reason. At this 
 *	point, we're about to call EntityManager::callReplayCallback() to notify
 *	the callback if any is set.
 */
void EntityManager::replayURLRequestAborted()
{
	BWReplayConnection * pReplayConn = 
		ConnectionControl::instance().pReplayConnection();
	MF_ASSERT( pReplayConn != NULL );

	// Prevent a callback to EntityManager::onReplayTerminated, we end up
	// in the same state as if it had been called.
	pReplayConn->setHandler( NULL );
	ConnectionControl::instance().stopReplay();

	if (pURLRequest_)
	{
		pURLRequest_->cancel();
		pURLRequest_ = NULL;
	}

	this->replayURLRequestFinished();
}



/**
 *	This method is called when the URL request has completed.
 */
void EntityManager::replayURLRequestFinished()
{
	pReplayURLHandler_ = NULL;
	pURLRequest_ = NULL;
}


/**
 *	This method calls the given function on the Python replay handler object.
 *
 *	@param shouldClear	Whether the replay handler should be cleared while it 
 *						is being called. This is typically true for the last
 *						callback.
 *	@param func 		The function name.
 *	@param args			A Python tuple of the arguments to pass to the 
 *						function.
 */
void EntityManager::callReplayHandler( bool shouldClear, const char * func, 
		PyObject * args )
{
	if (pPyReplayHandler_ == NULL)
	{
		return;
	}

	PyObjectPtr pPyReplayHandler = pPyReplayHandler_;
	
	if (shouldClear)
	{
		pPyReplayHandler_ = NULL;
	}

	Script::call(
		PyObject_GetAttrString( pPyReplayHandler.get(), func ),
		args,
		"EntityManager::callReplayHandler: ",
		true );
}


/**
 *	This method calls the given function on the Python replay handler object,
 *	returning is value cast as a bool.
 *
 *
 *	@param shouldClear	Whether the replay handler should be cleared while it 
 *						is being called. This is typically true for the last
 *						callback.
 *	@param func 		The function name.
 *	@param args			A Python tuple of the arguments to pass to the
 *						function.
 */
bool EntityManager::callReplayHandlerBoolRet( bool shouldClear,
		const char * func, PyObject * args )
{
	if (pPyReplayHandler_ == NULL)
	{
		return false;
	}

	bool handled = false;
	
	PyObjectPtr pPyReplayHandler = pPyReplayHandler_;
	
	if (shouldClear)
	{
		pPyReplayHandler_ = NULL;
	}

	PyObject * ret = Script::ask(
		PyObject_GetAttrString( pPyReplayHandler.get(), func ),
		args,
		"EntityManager::callReplayHandlerBoolRet: " );
	Script::setAnswer( ret, handled,
		"EntityManager::callReplayHandlerBoolRet retval" );

	return handled;
}


/*
 *	Override from BWReplayEventHandler.
 */
void EntityManager::onReplayHeaderRead( const ReplayHeader & header )
{
	this->callReplayCallback( /* shouldClear */ false,
		ConnectionControl::STAGE_DATA, "LOGGED_ON_OFFLINE" );
}


/*
 *	Override from BWReplayEventHandler.
 */
bool EntityManager::onReplayMetaData( const ReplayMetaData & metaData )
{
	ScriptObject replayHandler( pPyReplayHandler_.get(),
		ScriptObject::FROM_BORROWED_REFERENCE );

	ScriptObject method = replayHandler.getAttribute( "onReplayMetaData",
		ScriptErrorClear() );

	if (!method.exists())
	{
		return true;
	}

	ScriptDict metaDataDict = ScriptDict::create(
		static_cast<int>( metaData.size() ) );

	ReplayMetaData::const_iterator iter = metaData.begin();
	while (iter != metaData.end())
	{
		metaDataDict.setItem( iter->first.c_str(),
			ScriptString::create( iter->second ),
			ScriptErrorPrint() );
		++iter;
	}

	ScriptObject returnValue = method.callFunction(
		ScriptArgs::create( metaDataDict ),
		ScriptErrorPrint() );

	return (returnValue.exists() &&
		(returnValue.isNone() || returnValue.isTrue( ScriptErrorPrint() )));
}


/*
 *	Override from BWReplayEventHandler.
 */
void EntityManager::onReplayReachedEnd()
{
	this->callReplayHandler( /* shouldClear */ false, "onFinish",
		PyTuple_New( 0 ) );
}


/*
 *	Override from BWReplayEventHandler.
 */
void EntityManager::onReplayError( const BW::string & error )
{
	this->callReplayHandler( /* shouldClear */ false, "onError",
		Py_BuildValue( "(s)", error.c_str() ) );
}


/*
 *	Override from BWReplayEventHandler.
 */
void EntityManager::onReplayEntityClientChanged( const BWEntity * pEntity )
{
	MF_ASSERT( pEntity != NULL );
	MF_ASSERT( !pEntity->isDestroyed() );

	PyEntityPtr pPyEntity =
		static_cast< const Entity * >( pEntity )->pPyEntity();

	// TODO: Get hasBecomePlayer from somewhere.
	bool hasBecomePlayer = true;

	this->callReplayHandler( /* shouldClear */ false, "onPlayerStateChange",
		Py_BuildValue( "(OB)",
			static_cast< PyObject * >( pPyEntity.get() ),
			uint8( hasBecomePlayer ) ) );
}


/*
 *	Override from BWReplayEventHandler.
 */
void EntityManager::onReplayEntityAoIChanged( const BWEntity * pWitness,
	const BWEntity * pEntity, bool isEnter )
{
	MF_ASSERT( pWitness != NULL );
	MF_ASSERT( !pWitness->isDestroyed() );
	MF_ASSERT( pEntity != NULL );
	MF_ASSERT( !pEntity->isDestroyed() );

	PyEntityPtr pPyWitness =
		static_cast< const Entity * >( pWitness )->pPyEntity();
	PyEntityPtr pPyEntity =
		static_cast< const Entity * >( pEntity )->pPyEntity();

	this->callReplayHandler( /* shouldClear */ false, "onPlayerAoIChange",
		Py_BuildValue( "(OOB)",
			static_cast< PyObject * >( pPyWitness.get() ),
			static_cast< PyObject * >( pPyEntity.get() ),
			uint8( isEnter ) ) );
}


/*
 *	Override from BWReplayEventHandler.
 */
void EntityManager::onReplayTicked( GameTime currentTick, GameTime totalTicks )
{
	this->callReplayHandler( /* shouldClear */ false, "onPostTick",
		Py_BuildValue( "(ii)", currentTick, totalTicks ) );
}


/*
 *	Override from BWReplayEventHandler.
 */
void EntityManager::onReplaySeek( double dTime )
{
	App::instance().skipGameTimeForward( dTime );
}


/*
 *	Override from BWReplayEventHandler.
 */
void EntityManager::onReplayTerminated( ReplayTerminatedReason reason )
{
	BW_GUARD;

	if (pPyReplayHandler_)
	{
		switch( reason )
		{
		case BWReplayEventHandler::REPLAY_STOPPED_PLAYBACK:
			this->callReplayCallback( /* shouldClear */ true,
				ConnectionControl::STAGE_DISCONNECTED, "NOT_SET" );
			break;
		case BWReplayEventHandler::REPLAY_ABORTED_VERSION_MISMATCH:
			this->callReplayCallback( /* shouldClear */ true,
				ConnectionControl::STAGE_LOGIN, "LOGIN_BAD_PROTOCOL_VERSION" );
			break;
		case BWReplayEventHandler::REPLAY_ABORTED_ENTITYDEF_MISMATCH:
			this->callReplayCallback( /* shouldClear */ true,
				ConnectionControl::STAGE_LOGIN, "LOGIN_REJECTED_BAD_DIGEST" );
			break;
		case BWReplayEventHandler::REPLAY_ABORTED_METADATA_REJECTED:
			// Although triggered from script, don't rely on it to call 
			// BigWorld.disconnect().
			this->callReplayCallback( /* shouldClear */ true,
				ConnectionControl::STAGE_DISCONNECTED, "CONNECTION_FAILED" );
			break;
		case BWReplayEventHandler::REPLAY_ABORTED_CORRUPTED_DATA:
			this->callReplayCallback( /* shouldClear */ true,
				ConnectionControl::STAGE_DISCONNECTED, "CONNECTION_FAILED" );
			break;

		default:
			ERROR_MSG( "EntityManager::onReplayTerminated: "
					"Got unknown reason: %d, disconnecting\n",
				int( reason ) );

			this->callReplayCallback( /* shouldClear */ true,
				ConnectionControl::STAGE_DISCONNECTED, "CONNECTION_FAILED" );
		}

		pPyReplayHandler_ = NULL;
	}

	if (pURLRequest_ != NULL)
	{
		URL::Manager::instance()->cancelRequest( pURLRequest_ );
		pURLRequest_ = NULL;
	}

	this->clearPendingEntities();
}


/**
 *	This method starts a replay playback from a file on disk.
 *
 *	@param filename				The name of the file resource to read from.
 *	@param publicKey 			The EC public key string to use for verifying
 *								signatures, in PEM format.
 *	@param pReplayHandler 		The Python callback handler.
 *	@param volatileInjectionPeriod
 *								The maximum period for injected volatile
 *								updates.
 *
 *	@return						The SpaceID the playback is playing in, or
 *								NULL_SPACE_ID with Python Error set if something
 *								failed.
 */
SpaceID EntityManager::startReplayFromFile( const BW::string & fileName,
	const BW::string & publicKey, PyObjectPtr pReplayHandler,
	int volatileInjectionPeriod )
{
	BW_GUARD;

	SpaceID result = ConnectionControl::instance().startReplayFromFile(
		fileName, this, publicKey, volatileInjectionPeriod );

	if (result == NULL_SPACE_ID)
	{
		PyErr_SetString( PyExc_RuntimeError, "Already replaying" );
		return NULL_SPACE_ID;
	}

	pPyReplayHandler_ = pReplayHandler;
	return result;
}


/**
 *	This method starts a replay playback from a URL.
 *
 *	@param url 					The URL to retrieve.
 *	@param cacheFileName	 	The buffer file to use for playback.
 *	@param shouldKeepCache		true if the buffer file should not be deleted
 *								after playback terminates.
 *	@param publicKey 			The EC public key string to use for verifying
 *								signatures, in PEM format.
 *	@param pReplayHandler 		The Python callback handler.
 *	@param volatileInjectionPeriod
 *								The maximum period for injected volatile
 *								updates.
 *
 *	@return						The SpaceID the playback is playing in, or
 *								NULL_SPACE_ID with Python Error set if something
 *								failed.
 */
SpaceID EntityManager::startReplayFromURL( const BW::string & url,
	const BW::string & cacheFileName, bool shouldKeepCache,
	const BW::string & publicKey, PyObjectPtr pReplayHandler,
	int volatileInjectionPeriod )
{
	BW_GUARD;

	pPyReplayHandler_ = pReplayHandler;

	SpaceID result = ConnectionControl::instance().startReplayFromStream(
		cacheFileName, shouldKeepCache, this, publicKey,
		volatileInjectionPeriod );

	if (result == NULL_SPACE_ID)
	{
		PyErr_SetString( PyExc_RuntimeError, "Already replaying" );
		return NULL_SPACE_ID;
	}

	MF_ASSERT( pURLRequest_ == NULL );
	pReplayURLHandler_ = new ReplayURLHandler( *this );

	if (!URL::Manager::instance()->fetchURL( url,
			/* pResponse */pReplayURLHandler_.get(),
			/* pHeaders */NULL, URL::METHOD_GET,
			/* connectTimeoutInSeconds */0.f, /* pPostData */NULL,
			&pURLRequest_ ))
	{
		// So we don't call the "replay stopped safely" callback.
		pPyReplayHandler_ = NULL;

		ConnectionControl::instance().stopReplay();

		PyErr_SetString( PyExc_ValueError, "Could not fetch URL" );

		return NULL_SPACE_ID;
	}

	return result;
}


// -----------------------------------------------------------------------------
// Section: BWStreamDataHandler overrides
// -----------------------------------------------------------------------------

void EntityManager::onStreamDataComplete( uint16 streamID,
	const BW::string & rDescription, BinaryIStream & rData )
{
	// TODO: ScriptObject this up
	BW_GUARD;
	int len = rData.remainingLength();

	if (len <= 0)
	{
		ERROR_MSG( "EntityManager::onStreamDataComplete: "
			"Received zero length data\n" );
		return;
	}

	const char *pData = (const char*)rData.retrieve( len );

	ScriptObject func = Personality::getMember(
		/*currentName*/ "onStreamComplete",
		/*deprecatedName*/ "onProxyDataDownloadComplete" );

	if (func)
	{
		func.callFunction( ScriptArgs::create( streamID, rDescription,
				ScriptString::create( pData, len ) ),
			ScriptErrorPrint( "EntityManager::onStreamDataComplete" ) );
	}
}


// -----------------------------------------------------------------------------
// Section: Python module functions
// -----------------------------------------------------------------------------

/*~ function BigWorld entity
 *  Returns the entity with the given id, or None if not found.
 *  This function will search only entities that are currently in the world.
 *  An entity may be not in the world if the server indicates to the client that
 *  the entity has entered the player's area of interest but the client doesn't 
 *  have enough information about that entity yet.
 *
 *  @param id An integer representing the id of the entity to return.
 *  @return The entity corresponding to the id given, or None if no such
 *  entity is found.
 */
/**
 *	Returns the entity with the given id, or None if not found.
 */
static PyObjectPtr entity( EntityID id )
{
	BW_GUARD;

	Entity * pEntity = ConnectionControl::instance().findEntity( id );

	if (pEntity == NULL)
	{
		return NULL;
	}

	MF_ASSERT( !pEntity->isDestroyed() );

	return pEntity->pPyEntity();
}
PY_AUTO_MODULE_FUNCTION( RETDATA, entity,
		ARG( EntityID, END ), BigWorld );

/*~ function BigWorld createPlayerEntity
 *  Creates a new entity on the client and places it in the world. The
 *  resulting entity will have no base or cell part, but will have the
 *  properties of a base entity, and will be BigWorld.player().
 *
 *  This will only work when using an offline connection (i.e. connected to a
 *  server named ""). Entities created this way are destroyed when the offline
 *  mode is terminated by "disconnection".
 *
 *  @param className	The string name of the entity to instantiate.
 *  @param position		A Vector3 containing the position at which the new
 *                      entity is to be spawned.
 *  @param direction	A Vector3 containing the initial orientation of the new
 *                      entity (roll, pitch, yaw).
 *  @param state		A dictionary describing the initial states of the
 *                      entity's properties (as described in the entity's .def
 *                      file). A property will take on it's default value if it
 *                      is not listed here.
 *  @return				The ID of the new entity, as an integer.
 *
 *  Example:
 *  @{
 *  # start an offline connection and create a player
 *  BigWorld.connect( "", object(), connectionCallbackFn )
 *  position = ( 50, 100, 150 )
 *  direction = ( 0, 0, 0 )
 *  properties = { 'modelType':2, playerName: 'Player' }
 *  BigWorld.createPlayerEntity( 'Avatar', position, direction, properties )
 *  @}
 */
/**
 *	This function creates a player entity for a BWNullConnection-driven space.
 */
static PyObject * createPlayerEntity( const BW::string & className,
	const Position3D & position, const Vector3 & directionRPY,
	const ScriptDict & properties )
{
	Direction3D direction( directionRPY );

	// Now find the index of this class
	EntityType * pType = EntityType::find( className );
	if (pType == NULL)
	{
		PyErr_Format( PyExc_ValueError, "Class %s is not an entity class",
			className );
		return NULL;
	}

	BWNullConnection * pNullConn =
		ConnectionControl::instance().pNullConnection();
	if (pNullConn == NULL)
	{
		PyErr_Format( PyExc_ValueError, "Not running in offline mode" );
		return NULL;
	}

	if (pNullConn->pPlayer() != NULL)
	{
		PyErr_Format( PyExc_ValueError, "Player already created" );
		return NULL;
	}

	// Create an offline-mode player entity
	MemoryOStream stream( 4096 ); 
	pType->prepareBasePlayerStream( stream, properties );
	EntityID newEntityID =
		pNullConn->createNewPlayerBaseEntity( pType->index(), stream );

	if (newEntityID == NULL_ENTITY_ID)
	{
		PyErr_Format( PyExc_ValueError, 
			"Failed to create new player base Entity of type %s",
			className );
		return NULL;
	}

	stream.reset();

	pType->prepareCellPlayerStream( stream, properties );

	if (!pNullConn->giveCellToPlayerBaseEntity( position, direction,
		stream ))
	{
		PyErr_Format( PyExc_ValueError, 
			"Failed to create new player cell Entity of type %s",
			className );
		return NULL;
	}

	// TODO: We can change back to RETDATA and remove .newRef() when 
	// RETDATA supports exceptions.
	return ScriptObject::createFrom( newEntityID ).newRef();
}
PY_AUTO_MODULE_FUNCTION( RETOWN, createPlayerEntity, ARG( BW::string,
		ARG( Position3D, ARG( Vector3, ARG( ScriptDict, END ) ) ) ), 
	BigWorld )


/*~ function BigWorld createEntity
 *  Creates a new entity on the client and places it in the world. The
 *	resulting entity will have no base or cell part.
 *
 *  @param className	The string name of the entity to instantiate.
 *	@param spaceID		The id of the space in which to place the entity.
 *	@param vehicleID	The id of the vehicle on which to place the entity
 *						(0 for no vehicle).
 *  @param position		A Vector3 containing the position at which the new
 *						entity is to be spawned.
 *  @param direction	A Vector3 containing the initial orientation of the new
 *						entity (roll, pitch, yaw).
 *  @param state		A dictionary describing the initial states of the
 *						entity's properties (as described in the entity's .def
 *						file). A property will take on it's default value if it
 *						is not listed here.
 *  @return				The ID of the new entity, as an integer.
 *
 *  Example:
 *  @{
 *  # create an arrow style Info entity at the same position as the player
 *  p = BigWorld.player()
 *  direction = ( 0, 0, p.yaw )
 *  properties = { 'modelType':2, 'text':'Created Info Entity'}
 *  BigWorld.createEntity( 'Info', p.spaceID, 0, p.position, direction, properties )
 *  @}
 */
/**
 *	This function creates a new client-only entity in the given space.
 *	If there is no current player entity, then the new entity will be the
 *	player entity in an offline space, but this is a deprecated behaviour from
 *	pre-connection_model bwclient. Use createPlayerEntity instead.
 */
static PyObject * createEntity( const BW::string & className, SpaceID spaceID,
	EntityID vehicleID, const Position3D & position,
	const Vector3 & directionRPY, const ScriptDict & properties )
{
	Direction3D direction( directionRPY );

	if (spaceID == NULL_SPACE_ID)
	{
		PyErr_SetString( PyExc_ValueError, "Invalid space ID" );
		return NULL;
	}
	
	// Now find the index of this class
	EntityType * pType = EntityType::find( className );
	if (pType == NULL)
	{
		PyErr_Format( PyExc_ValueError, "Class %s is not an entity class", className );
		return NULL;
	}

	ConnectionControl & connectionControl = ConnectionControl::instance();
	SpaceManager  & spaceManager = SpaceManager::instance();

	BWConnection * pConnection = spaceManager.isLocalSpace( spaceID ) ?
		connectionControl.pSpaceConnection( spaceID ) :
		connectionControl.pConnection();

	if (!pConnection)
	{
		PyErr_Format( PyExc_ValueError, 
				"No server connection or client space with ID %d",
			spaceID );
		return NULL;
	}

	BWNullConnection * pNullConnection = connectionControl.pNullConnection();

	if (pNullConnection && pNullConnection->pPlayer() == NULL)
	{
		// Trying to create an offline-mode Player through the createEntity
		// call. This is a fallback from pre-connection_model BWClient.
		MF_ASSERT( spaceID != NULL_SPACE_ID );
		if (vehicleID != NULL_ENTITY_ID)
		{
			// TODO: Is there a strong reason this doesn't work? Just a flaw
			// in the BWNullConnection API? There's currently no Python way
			// to add the vehicle to the space though.
			WARNING_MSG( "createEntity: Trying to create a player entity "
					"on a vehicle (ID %d), but that is unimplemented\n",
				vehicleID );
		}
		return createPlayerEntity( className, position,
			directionRPY, properties );
	}


	// Add a client-only entity to an existing space.
	MemoryOStream stream( 4096 );
	pType->prepareCreationStream( stream, properties );

	BWEntity * pNewEntity = pConnection->createLocalEntity( pType->index(),
		spaceID, vehicleID, position, direction, stream );

	if (pNewEntity == NULL)
	{
		PyErr_Format( PyExc_ValueError, 
			"Failed to create new Entity of type %s",
			className );
		return NULL;
	}

	// TODO: We can change back to RETDATA and remove .newRef() when 
	// RETDATA supports exceptions.
	return ScriptObject::createFrom( pNewEntity->entityID() ).newRef();
}
PY_AUTO_MODULE_FUNCTION( RETOWN, createEntity, ARG( BW::string,
	NZARG( SpaceID, ARG( EntityID, ARG( Position3D, ARG( Vector3,
	ARG( ScriptDict, END ) ) ) ) ) ), BigWorld )



/*~ function BigWorld destroyEntity
 *  Destroys an exiting client-side entity.
 *  @param id The id of the entity to destroy.
 *
 *  Example:
 *  @{
 *  id = BigWorld.target().id # get current target ID
 *  BigWorld.destroyEntity( id )
 *  @}
 */
/**
 *	Destroys an existing client-side entity
 */
static bool destroyEntity( EntityID id )
{
	BW_GUARD;

	if (!BWEntities::isLocalEntity( id ))
	{
		PyErr_Format( PyExc_ValueError, "Not a client-only entity id" );
		return false;
	}

	ConnectionControl & connectionControl = ConnectionControl::instance();

	BWEntityPtr pEntity = connectionControl.findEntity( id );
	if (!pEntity)
	{
		PyErr_Format( PyExc_ValueError, "Unknown entity id" );
		return false;
	}

	SpaceID spaceID = pEntity->spaceID();
	MF_ASSERT( SpaceManager::instance().isLocalSpace( spaceID ) );

	BWConnection * pConnection = connectionControl.pSpaceConnection( spaceID );
	MF_ASSERT( pConnection );

	MF_VERIFY( pConnection->destroyLocalEntity( id ) );

	return true;
}
PY_AUTO_MODULE_FUNCTION( RETOK, destroyEntity, ARG( EntityID, END ), BigWorld );


// -----------------------------------------------------------------------------
// Section: BWReplay Python interface
// -----------------------------------------------------------------------------

/*~ module BWReplay
 *	@components{ client }
 *
 *	The BWReplay module handles playback of recorded replay files.
 */


/*~ function BWReplay.startPlayback
 *	@components{ client }
 *
 *	@param url			The URL to load the recording from
 *	@param publicKey 	The public key to use for verifying signatures.
 *	@param handler		The replay handler
 *	@param localFilename	(Optional) The path to save a local file name.
 *						If not specified will be a temporary file.
 *	@param volatileInjectionPeriod
 *						(Optional) The volatile injection period. This setting
 *						overrides the configuration setting.
 *	@return				The SpaceID of the space containing the playback
 *
 *	If you specify a url with using the file:// protocol, localFilename will
 *	be ignored, as the file will be read directly from the one saved on disk.
 *
 *	A replay handler must have callbacks onPostTick, onProgress, handleKeyEvent,
 *	and handleMouseEvent an example implementation of this handler is:
 *
 *	@{
 *	class ReplayHandler( object ):
 *	  def __init__( self ):
 *	    self.updateHz = 10
 *	    self.currentPlaybackTime = 0
 *
 *	  def onLoadStart( self ):
 *	    # Set camera's space so BigWorld.spaceLoadStatus() can be used
 *
 *	  def onLoadFinished( self ):
 *	    # Time to spawn into level
 *
 *	  def onPostTick( self, currentTick, totalTicks ):
 *	    # Tick 0 will be after space mapping has occurred
 *	    # Tick 1 will not start until BigWorld.worldDrawEnable( True )
 *	    if currentTick == 0:
 *	      self.onLoadStart()
 *	    elif currentTick == 1:
 *	      self.onLoadFinish()
 *	    # Record the time display in a GUI
 *	    self.currentPlaybackTime = currentTick / self.updateHz
 *
 *	  def onProgress( self, stage, status, message ):
 *	    # This method takes the same arguments as BigWorld.connect()'s progressFn
 *	    # to enable the values to be passed directly to it.
 *	    if stage == 2:
 *	      # Header has been loaded get our updateHz
 *	      self.updateHz = BWReplay.getUpdateHz()
 *
 *	  def onFinish( self ):
 *	    # This method is called when the recording reaches the end of what
 *	    # it can process. This is usually the end of the file/stream
 *	    # but will also be called if the connection to the stream is lost.
 *
 *	  def handleKeyEvent( self, event ):
 *	    # This method is called whenever BWReplay.handleKeyEvent is called
 *	    return False
 *
 *	  def handleMouseEvent( self, event ):
 *	    # This method is called whenever BWReplay.handleMouseEvent is called
 *	    return False
 *	@}
 *
 *	When the header is loaded has been successfully loaded the onProgress
 *	method will be called with the stage as 2 (Data) and the status set to
 *	"REPLAY". If the header was invalid then onProgress will be called
 *	with the stage as 1 (Login) and status set to the current error code,
 *	or message set to the error message.
 *
 *	After the header has been processed the first tick is processed,
 *	which contains the space geometry and the initial entities to create. The
 *	Replay handler notified via the onPostTick method with currentTick set
 *	to 0.
 *
 *	The ticks following tick 0 will only be loaded if
 *	BigWorld.worldDrawEnabled has been set to True, this allows
 *	BigWorld.spaceLoadStatus to be used for loading screens.
 *
 *	When the recording has finished playing the onProgress method will be called
 *	with stage as 6 (Disconnect).
 *
 */
PyObject * startPlayback( const BW::string & url, const BW::string & publicKey,
		PyObjectPtr handler, const BW::string & localFilename,
		int volatileInjectionPeriod )
{
	BW::string filename = localFilename;

	static const BW::string FILE_SCHEME( "file://" );

	// For local files, we don't use libcURL - we can play directly from disk.
	if (url.substr( 0, FILE_SCHEME.length() ) == FILE_SCHEME)
	{
		// We check for the existence of the URL host, which is allowed but
		// the value is then ignored.
		// Note that libcURL would parse a URL host as the first element of a
		// relative path instead, in violation of RFC1738.

		size_t hostStart = FILE_SCHEME.length();
		size_t pathStart = 0;

		if (url[hostStart] != '/')
		{
			pathStart = url.find( '/', hostStart ) + 1;
		}
		else
		{
			pathStart = hostStart + 1;
		}

		filename = url.substr( pathStart );

		std::replace( filename.begin(), filename.end(), '/', '\\' );
		std::replace( filename.begin(), filename.end(), '|', ':' );

		if (!BWResource::fileAbsolutelyExists( filename ))
		{
			PyErr_Format( PyExc_ValueError,
				"File in local file URL does not exist: %s",
				filename.c_str() );

			return NULL;
		}

		SpaceID result = EntityManager::instance().startReplayFromFile(
			filename, publicKey, handler, volatileInjectionPeriod );

		return (result != NULL_SPACE_ID) ? 
			ScriptInt::create( result ).newRef() : 
			NULL;
	}

	bool isCacheTemporary = true;

	if (filename.empty())
	{
		filename = bw_wtoacp( getTempFilePathName() );
		isCacheTemporary = false;
	}

	SpaceID result = EntityManager::instance().startReplayFromURL( url,
		filename, /* shouldKeepCache */ !isCacheTemporary, publicKey, handler,
		volatileInjectionPeriod );

	return (result != NULL_SPACE_ID) ? 
		ScriptInt::create( result ).newRef() : 
		NULL;
}
PY_AUTO_MODULE_FUNCTION( RETOWN, startPlayback,
		ARG( BW::string,
		ARG( BW::string,
		NZARG( PyObjectPtr,
		OPTARG( BW::string, "",
		OPTARG( int, -1,
			END ) )))), BWReplay );


/*~ function BWReplay.stopPlayback
 *	@components{ client }
 *
 *	Stop playback of a currently playing demo
 */
void stopPlayback()
{
	ConnectionControl::instance().stopReplay();
}

PY_AUTO_MODULE_FUNCTION( RETVOID, stopPlayback, END, BWReplay );


/*~ function BWReplay.getCurrentTick
 *	@components{ client }
 *
 *	@return The current tick index
 */
int getCurrentTick( )
{
	BWReplayConnection * pReplayConn =
		ConnectionControl::instance().pReplayConnection();
	if (pReplayConn == NULL || pReplayConn->pReplayController() == NULL)
	{
		return -1;
	}

	return pReplayConn->pReplayController()->getCurrentTick();
}

PY_AUTO_MODULE_FUNCTION( RETDATA, getCurrentTick, END, BWReplay );


/*~ function BWReplay.setCurrentTick
 *	@components{ client }
 *
 *	Using this method will cause space data, and entities to be reloaded if
 *	tick is less then BWReplay.getCurrentTick()
 *
 *	@param newTick		The tick to move to
 *
 */
bool setCurrentTick( int newTick )
{
	BWReplayConnection * pReplayConn =
		ConnectionControl::instance().pReplayConnection();
	if (pReplayConn == NULL || pReplayConn->pReplayController() == NULL)
	{
		PyErr_SetString( PyExc_ValueError, "Not currently replaying" );
		return false;
	}

	return pReplayConn->pReplayController()->setCurrentTick( newTick );
}

PY_AUTO_MODULE_FUNCTION( RETOK, setCurrentTick, ARG( int, END ), BWReplay );


/*~ function BWReplay.getTotalTicks
 *	@components{ client }
 *
 *	@return The total number of ticks loaded
 */
int getTotalTicks( )
{
	BWReplayConnection * pReplayConn =
		ConnectionControl::instance().pReplayConnection();
	if (pReplayConn == NULL || pReplayConn->pReplayController() == NULL)
	{
		return -1;
	}

	return pReplayConn->pReplayController()->numTicksTotal();
}

PY_AUTO_MODULE_FUNCTION( RETDATA, getTotalTicks, END, BWReplay );

/*~ function BWReplay.getNumLoadedTicks
 *	@components{ client }
 *
 *	@return The total number of ticks loaded
 */
int getNumLoadedTicks( )
{
	BWReplayConnection * pReplayConn =
		ConnectionControl::instance().pReplayConnection();
	if (pReplayConn == NULL || pReplayConn->pReplayController() == NULL)
	{
		return -1;
	}

	return pReplayConn->pReplayController()->numTicksLoaded();
}

PY_AUTO_MODULE_FUNCTION( RETDATA, getNumLoadedTicks, END, BWReplay );


/*~ function BWReplay.isLoaded
 *	@components{ client }
 *
 *	@return True if a recording is loaded, false otherwise
 */
bool isLoaded()
{
	BWReplayConnection * pReplayConn =
		ConnectionControl::instance().pReplayConnection();
	if (pReplayConn == NULL || pReplayConn->pReplayController() == NULL)
	{
		return false;
	}

	return pReplayConn->pReplayController()->isLoaded();
}

PY_AUTO_MODULE_FUNCTION( RETDATA, isLoaded, END, BWReplay );


/*~ function BWReplay.isPlaying
 *	@components{ client }
 *
 *	@return True if a recording is playing, false otherwise
 */
bool isPlaying()
{
	BWReplayConnection * pReplayConn =
		ConnectionControl::instance().pReplayConnection();
	if (pReplayConn == NULL || pReplayConn->pReplayController() == NULL)
	{
		return false;
	}

	return pReplayConn->pReplayController()->isPlaying();
}

PY_AUTO_MODULE_FUNCTION( RETDATA, isPlaying, END, BWReplay );


/*~ function BWReplay.pausePlayback
 *	@components{ client }
 *
 *	This method will pause the playback.
 */
void pausePlayback( )
{
	BWReplayConnection * pReplayConn =
		ConnectionControl::instance().pReplayConnection();
	if (pReplayConn == NULL)
	{
		return;
	}

	pReplayConn->pauseReplay();
}

PY_AUTO_MODULE_FUNCTION( RETVOID, pausePlayback, END, BWReplay );


/*~ function BWReplay.resumePlayback
 *	@components{ client }
 *
 *	This method will resume the playback.
 */
void resumePlayback( )
{
	BWReplayConnection * pReplayConn =
		ConnectionControl::instance().pReplayConnection();
	if (pReplayConn == NULL)
	{
		return;
	}

	pReplayConn->resumeReplay();
}

PY_AUTO_MODULE_FUNCTION( RETVOID, resumePlayback, END, BWReplay );


/*~ function BWReplay.getSpeedScale
 *	@components{ client }
 *
 *	@return The current speed scale
 */
float getSpeedScale( )
{
	BWReplayConnection * pReplayConn =
		ConnectionControl::instance().pReplayConnection();
	if (pReplayConn == NULL)
	{
		return -1.f;
	}

	return pReplayConn->speedScale();
}

PY_AUTO_MODULE_FUNCTION( RETDATA, getSpeedScale, END, BWReplay );



/*~ function BWReplay.setSpeedScale
 *	@components{ client }
 *
 * @param scale		The speed scale to set the current speed scale to
 */
void setSpeedScale( float scale )
{
	BWReplayConnection * pReplayConn =
		ConnectionControl::instance().pReplayConnection();
	if (pReplayConn != NULL)
	{
		pReplayConn->speedScale( scale );
	}
}

PY_AUTO_MODULE_FUNCTION( RETVOID, setSpeedScale,
						ARG( float, END ), BWReplay );

/*~ function BWReplay.handleKeyEvent
 *	@components{ client }
 *
 *	This method calls the handleKeyEvent method on the handler.
 *
 *	@param keyEvent		The key event
 *
 *	@return True if handled, false otherwise
 */
bool handleKeyEvent( const KeyEvent & keyEvent )
{
	return EntityManager::instance().callReplayHandlerBoolRet(
		/* shouldClear */ false,
		"handleKeyEvent", 
		Py_BuildValue( "(N)", Script::getData( keyEvent ) ) );
}

PY_AUTO_MODULE_FUNCTION( RETDATA, handleKeyEvent,
						ARG( KeyEvent, END ), BWReplay );


/*~ function BWReplay.handleMouseEvent
 *	@components{ client }
 *
 *	This method calls the handleMouseEvent method on the handler.
 *
 *	@param keyEvent		The mouse event
 *
 *	@return True if handled, false otherwise
 */
bool handleMouseEvent( const MouseEvent & mouseEvent )
{
	return EntityManager::instance().callReplayHandlerBoolRet(
		/* shouldClear */ false,
		"handleMouseEvent", 
		Py_BuildValue( "(N)",
			Script::getData( mouseEvent ) ) );
}

PY_AUTO_MODULE_FUNCTION( RETDATA, handleMouseEvent,
						ARG( MouseEvent, END ), BWReplay );


/*~ function BWReplay.getUpdateHz
 *	@components{ client }
 *
 *	@return		The update hertz of the loaded recording.
 */
int getUpdateHz()
{
	BWReplayConnection * pReplayConn =
		ConnectionControl::instance().pReplayConnection();
	if (pReplayConn == NULL || pReplayConn->pReplayController() == NULL)
	{
		return -1;
	}

	return pReplayConn->pReplayController()->updateFrequency();
}

PY_AUTO_MODULE_FUNCTION( RETDATA, getUpdateHz, END, BWReplay );


/*~ function BWReplay.isSeekingToTick
 *	@components{ client }
 *
 *	@return		True if currently fast forwarding, false otherwise
 */
bool isSeekingToTick()
{
	BWReplayConnection * pReplayConn =
		ConnectionControl::instance().pReplayConnection();
	if (pReplayConn == NULL || pReplayConn->pReplayController() == NULL)
	{
		return false;
	}

	return pReplayConn->pReplayController()->isSeekingToTick();
}

PY_AUTO_MODULE_FUNCTION( RETDATA, isSeekingToTick, END, BWReplay );


/*~ function BWReplay.numTicksLoaded
 *	@components{ client }
 *
 *	@return		The number of ticks loaded
 */
int numTicksLoaded()
{
	BWReplayConnection * pReplayConn =
		ConnectionControl::instance().pReplayConnection();
	if (pReplayConn == NULL || pReplayConn->pReplayController() == NULL)
	{
		return false;
	}


	return pReplayConn->pReplayController()->numTicksLoaded();
}

PY_AUTO_MODULE_FUNCTION( RETDATA, numTicksLoaded, END, BWReplay );


/*~ function BWReplay.getReplayHandler
 *	@components{ client }
 *
 *	@return		The replay handler
 */
PyObjectPtr getReplayHandler()
{
	return EntityManager::instance().pPyReplayHandler();
}

PY_AUTO_MODULE_FUNCTION( RETDATA, getReplayHandler, END, BWReplay );


/*~ function BWReplay.spaceID
 *	@components{ client }
 *
 *	@return		The spaceID that the replay is running in, or NULL_SPACE_ID (0)
 *				if no replay is running.
 */
SpaceID spaceID()
{
	BWReplayConnection * pReplayConn;

	if ((pReplayConn = ConnectionControl::instance().pReplayConnection()))
	{
		return pReplayConn->spaceID();
	}

	return NULL_SPACE_ID;
}

PY_AUTO_MODULE_FUNCTION( RETDATA, spaceID, END, BWReplay );

BW_END_NAMESPACE



// entity_manager.cpp
