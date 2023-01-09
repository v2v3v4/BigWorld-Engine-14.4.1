#include "pch.hpp"

#include "bw_server_message_handler.hpp"

#include "bw_entities.hpp"
#include "bw_space_data_listener.hpp"
#include "bw_space_data_storage.hpp"
#include "bw_stream_data_handler.hpp"

#include "common/space_data_types.hpp"

#include "cstdmf/memory_stream.hpp"


BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 *
 *	@param entities 		The entity collection.
 */
BWServerMessageHandler::BWServerMessageHandler( BWEntities & entities,
		BWSpaceDataStorage & spaceDataStorage ) :
	entities_( entities ),
	spaceDataStorage_( spaceDataStorage ),
	spaceDataListeners_(),
	streamDataHandlers_(),
	pStreamDataFallbackHandler_( NULL )
{
}


/**
 *	Destructor.
 */
BWServerMessageHandler::~BWServerMessageHandler()
{
}


/**
 *	This method adds a space data listener.
 *
 *	@param pListener 	The space data listener to add.
 */
void BWServerMessageHandler::addSpaceDataListener(
		BWSpaceDataListener * pListener )
{
	spaceDataListeners_.push_back( pListener );
}


/**
 *	This method removes a space data listener. 
 *
 *	@param pListener 	The space data listener to remove.
 *
 *	@return 			True if the data listener was previously registered,
 *						false otherwise.
 */
bool BWServerMessageHandler::removeSpaceDataListener(
		BWSpaceDataListener * pListener )
{
	SpaceDataListeners::iterator iter = std::find( spaceDataListeners_.begin(),
					spaceDataListeners_.end(), pListener );

	if (iter == spaceDataListeners_.end())
	{
		WARNING_MSG( "BWServerMessageHandler::removeSpaceDataListener: "
				"Failed to remove listener\n" );
		return false;
	}

	spaceDataListeners_.erase( iter );

	return true;
}


/**
 *	This method registers a stream data handler.
 *
 *	@param pListener 	The stream data handler to register
 *	@param id		 	The stream ID to register for
 */
void BWServerMessageHandler::registerStreamDataHandler(
		BWStreamDataHandler * pHandler, uint16 streamID )
{
	if (streamDataHandlers_[ streamID ] != NULL)
	{
		WARNING_MSG( "BWServerMessageHandler::addStreamDataHandler: "
				"trying to register handler for stream ID %u but one is "
				"already registered. Replacing old handler.\n",
			streamID );
	}

	streamDataHandlers_[ streamID ] = pHandler;
}


/**
 *	This method deregisters a stream data handler. 
 *
 *	@param pListener 	The stream data handler to deregister
 *	@param id		 	The stream ID to deregister from
 */
void BWServerMessageHandler::deregisterStreamDataHandler(
		BWStreamDataHandler * pHandler, uint16 streamID )
{
	StreamDataHandlers::iterator iHandler =
		streamDataHandlers_.find( streamID );

	if (iHandler == streamDataHandlers_.end())
	{
		WARNING_MSG( "BWServerMessageHandler::removeStreamDataHandler: "
				"trying to unregister handler for stream ID %u but one is "
				"not registered.\n",
			streamID );
		return;
	}

	if (iHandler->second != pHandler)
	{
		WARNING_MSG( "BWServerMessageHandler::removeStreamDataHandler: "
				"trying to unregister handler for stream ID %u but a different "
				"one is registered. Ignoring request.\n",
			streamID );
		return;
	}

	streamDataHandlers_.erase( iHandler );
}


/**
 *	This method registers a fallback stream data handler for streams which do
 *	not have a registered stream data handler.
 *
 *	@param pHandler 	The stream data handler to register, or NULL to remove
 *						the existing fallback handler.
 */
void BWServerMessageHandler::setStreamDataFallbackHandler(
	BWStreamDataHandler * pHandler )
{
	pStreamDataFallbackHandler_ = pHandler;
}


/**
 * This method clears the space for the given space ID.
 */
void BWServerMessageHandler::clearSpace( SpaceID spaceID )
{
	spaceDataStorage_.deleteSpace( spaceID );
}


/*
 *	Override from ServerMessageHandler.
 */
void BWServerMessageHandler::onBasePlayerCreate( EntityID id,
		EntityTypeID entityTypeID, BinaryIStream & data )
{
	entities_.handleBasePlayerCreate( id, entityTypeID, data );
}


/*
 *	Override from ServerMessageHandler.
 */
void BWServerMessageHandler::onCellPlayerCreate( EntityID id, SpaceID spaceID,
		EntityID vehicleID, const Position3D & pos, 
		float yaw, float pitch, float roll, 
		BinaryIStream & data )
{
	if (!spaceDataStorage_.addNewSpace( spaceID ))
	{
		// Space already exists, but that's OK. 
		DEBUG_MSG( "BWServerMessageHandler::onCellPlayerCreate: "
				"Space %d already existed\n",
			spaceID );
	}

	entities_.handleCellPlayerCreate( id, spaceID, vehicleID, pos, yaw, pitch,
		roll, data );
}


/*
 *	Override from ServerMessageHandler.
 */
void BWServerMessageHandler::onEntityControl( EntityID id, bool isControlling )
{
	entities_.handleEntityControl( id, isControlling );
}


/*
 *	Override from ServerMessageHandler.
 */
void BWServerMessageHandler::onEntityCreate( EntityID id, 
		EntityTypeID entityTypeID,
		SpaceID spaceID, EntityID vehicleID, const Position3D & position,
		float yaw, float pitch, float roll, BinaryIStream & data )
{
	entities_.handleEntityCreate( id, entityTypeID, spaceID, vehicleID,
		position, yaw, pitch, roll, data );
}


/*
 *	Override from ServerMessageHandler.
 */
const CacheStamps * BWServerMessageHandler::onEntityCacheTest( EntityID id )
{
	// No caching done
	return NULL;
}


/*
 *	Override from ServerMessageHandler.
 */
void BWServerMessageHandler::onEntityLeave( EntityID id, 
		const CacheStamps & stamps )
{
	entities_.handleEntityLeave( id );
}


/*
 *	Override from ServerMessageHandler.
 */
void BWServerMessageHandler::onEntityMethod( EntityID id, int methodID,
		BinaryIStream & data )
{
	entities_.handleEntityMethod( id, methodID, data );
}


/*
 *	Override from ServerMessageHandler.
 */
void BWServerMessageHandler::onEntityProperty( EntityID id, int propertyID,
		BinaryIStream & data )
{
	entities_.handleEntityProperty( id, propertyID, data );
}


/*
 *	Override from ServerMessageHandler.
 */
void BWServerMessageHandler::onNestedEntityProperty( EntityID id,
		BinaryIStream & data, bool isSlice )
{
	entities_.handleNestedEntityProperty( id, data, isSlice );
}


/*
 *	Override from ServerMessageHandler.
 */
int BWServerMessageHandler::getEntityMethodStreamSize( EntityID id,
	int methodID, bool isFailAllowed ) const
{
	return entities_.getEntityMethodStreamSize( id, methodID, isFailAllowed );
}


/*
 *	Override from ServerMessageHandler.
 */
int BWServerMessageHandler::getEntityPropertyStreamSize( EntityID id,
	int propertyID, bool isFailAllowed ) const
{
	return entities_.getEntityPropertyStreamSize( id, propertyID, isFailAllowed );
}


/**
 *	Override from ServerMessageHandler.
 */
void BWServerMessageHandler::onEntityMoveWithError( EntityID id, 
		SpaceID spaceID, EntityID vehicleID,
		const Position3D & pos, const Vector3 & posError,
		float yaw, float pitch, float roll,
		bool isVolatile )
{
	bool isSpaceChange = false;

	if (entities_.pPlayer() && entities_.pPlayer()->entityID() == id)
	{
		// Check player movement messages for space change.
		// TODO: Merge this into ServerConnection::forcedPosition
		BWEntity * pPlayer = entities_.pPlayer();
		MF_ASSERT( pPlayer->spaceID() != NULL_SPACE_ID );
		if (pPlayer->spaceID() != spaceID)
		{
			MF_ASSERT( !isVolatile );
			spaceDataStorage_.addNewSpace( spaceID );
			isSpaceChange = true;
		}
	}

	entities_.handleEntityMoveWithError( id, spaceID, vehicleID, pos,
		posError, yaw, pitch, roll, isVolatile );

	if (isSpaceChange)
	{
		// Make sure it was applied as an immediate teleport
		MF_ASSERT( entities_.pPlayer()->spaceID() == spaceID );
	}
}


/*
 *	Override from ServerMessageHandler.
 */
void BWServerMessageHandler::onEntityProperties( EntityID id, 
		BinaryIStream & data )
{
	entities_.handleEntityProperties( id, data );
}


/*
 *	Override from ServerMessageHandler.
 */
void BWServerMessageHandler::onEntitiesReset( bool keepPlayerOnBase )
{
	if (entities_.pPlayer() != NULL &&
		entities_.pPlayer()->spaceID() != NULL_SPACE_ID)
	{
		spaceDataStorage_.deleteSpace( entities_.pPlayer()->spaceID() );
	}

	entities_.handleEntitiesReset( keepPlayerOnBase );
}


/*
 *	Override from ServerMessageHandler.
 */
void BWServerMessageHandler::onRestoreClient( EntityID id, SpaceID spaceID,
	EntityID vehicleID, const Position3D & pos, const Direction3D & dir,
	BinaryIStream & data )
{
	MF_ASSERT( entities_.pPlayer() != NULL );
	MF_ASSERT( entities_.pPlayer()->isPlayer() );
	MF_ASSERT( entities_.pPlayer()->entityID() == id );

	entities_.handleRestoreClient( id, spaceID, vehicleID, pos, dir, data );
}


/*
 *	Override from ServerMessageHandler.
 */
void BWServerMessageHandler::spaceData( SpaceID spaceID, SpaceEntryID entryID, 
		uint16 key,	const BW::string & data )
{

	const uint16 DELETION_KEY = uint16(-1);

	SpaceDataMapping * pSpaceDataMapping =
		spaceDataStorage_.getSpaceMapping( spaceID );

	if (pSpaceDataMapping == NULL)
	{
		// SpaceData arrived after space cleared?
		MF_ASSERT( key == DELETION_KEY );
		return;
	}

	bool wasAdded = false;

	SpaceDataMapping::DataValue valueForCallback;

	if (pSpaceDataMapping != NULL)
	{
		if (key != uint16(-1))
		{
			wasAdded = pSpaceDataMapping->addDataEntry( entryID, key, data );
			if (wasAdded)
			{
				valueForCallback =
					pSpaceDataMapping->dataRetrieveSpecific( entryID );
			}
		}
		else
		{
			pSpaceDataMapping->delDataEntry( entryID, valueForCallback );
		}
	}

	const uint16 & modifiedKey = valueForCallback.key();
	const BW::string & callbackData = valueForCallback.data();

	if (modifiedKey == uint16(-1))
	{
		// Nothing changed, don't bother with the callbacks.
		return;
	}

	SpaceDataListeners listenersCopy( spaceDataListeners_ );
	SpaceDataListeners::iterator iListener = listenersCopy.begin();

	if ((modifiedKey == SPACE_DATA_MAPPING_KEY_CLIENT_SERVER) ||
		(modifiedKey == SPACE_DATA_MAPPING_KEY_CLIENT_ONLY))
	{
		if (callbackData.size() >= sizeof(SpaceData_MappingData))
		{
			SpaceData_MappingData & mappingData =
				*(SpaceData_MappingData *)callbackData.data();

			BW::string path( (char *)(&mappingData+1),
				callbackData.length() - sizeof( SpaceData_MappingData ) );

			Matrix mappingMatrix = *(Matrix *)&mappingData.matrix[0][0];


			while (iListener != listenersCopy.end())
			{
				BWSpaceDataListener & listener = **iListener;

				if (wasAdded)
				{
					listener.onGeometryMapping( spaceID, mappingMatrix, path );
				}
				else
				{
					listener.onGeometryMappingDeleted( spaceID, mappingMatrix,
						path );
				}
				++iListener;
			}
		}
		else
		{
			WARNING_MSG( "BWServerMessageHandler::spaceData: "
					"found no geometry mapping on key %hu "
					"for space %d (corrupted data?)\n",
				modifiedKey, spaceID );
		}
	}
	else if (modifiedKey == SPACE_DATA_TOD_KEY && wasAdded)
	{
		// No action on delete, SPACE_DATA_TOD_KEY is single-valued.
		if (callbackData.size() >= sizeof(SpaceData_ToDData))
		{
			SpaceData_ToDData & tod = *(SpaceData_ToDData *)callbackData.data();

			while (iListener != listenersCopy.end())
			{
				BWSpaceDataListener & listener = **iListener;

				listener.onTimeOfDay( spaceID, tod.initialTimeOfDay,
									   tod.gameSecondsPerSecond );
				++iListener;
			}
		}
		else
		{
			WARNING_MSG( "BWServerMessageHandler::spaceData: "
					"received no time of the day on key %hu "
					"for space %d (corrupted data?)\n",
				modifiedKey, spaceID );
		}
	}
	else if ((modifiedKey >= SPACE_DATA_FIRST_USER_KEY) &&
			 (modifiedKey <= SPACE_DATA_FINAL_USER_KEY))
	{
		while (iListener != listenersCopy.end())
		{
			BWSpaceDataListener & listener = **iListener;

			listener.onUserSpaceData( spaceID, valueForCallback.key(),
				/* isAddedData */ wasAdded, callbackData );
			++iListener;
		}
	}
	else
	{
		WARNING_MSG( "BWServerMessageHandler::spaceData: "
				"unhandled key %hu for space %d\n",
			modifiedKey, spaceID );
	}
}


/*
 *	Override from ServerMessageHandler.
 */
void BWServerMessageHandler::spaceGone( SpaceID spaceID )
{
	spaceDataStorage_.deleteSpace( spaceID );
}


/*
 *	Override from ServerMessageHandler.
 */
void BWServerMessageHandler::onStreamComplete( uint16 id,
	const BW::string & description, BinaryIStream & data )
{
	StreamDataHandlers::iterator iHandler = streamDataHandlers_.find( id );

	if (iHandler == streamDataHandlers_.end())
	{
		if (pStreamDataFallbackHandler_ != NULL)
		{
			pStreamDataFallbackHandler_->onStreamDataComplete( id, description,
				data );
			return;
		}

		WARNING_MSG( "BWServerMessageHandler::onStreamComplete: "
				"unhandled stream data id %hu: %s (%d bytes)\n",
			id, description.c_str(), data.remainingLength() );
		// Suppress _another_ message.
		data.finish();
		return;
	}

	iHandler->second->onStreamDataComplete( id, description, data );
	// Note that it's quite likely that iHandler is invalid here.
}

BW_END_NAMESPACE

// bw_server_message_handler.cpp
