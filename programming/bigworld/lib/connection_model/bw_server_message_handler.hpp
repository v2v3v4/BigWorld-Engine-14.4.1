#ifndef BW_SERVER_MESSAGE_HANDLER_HPP
#define BW_SERVER_MESSAGE_HANDLER_HPP


#include "bw_entity.hpp"

#include "connection/server_message_handler.hpp"

#include "network/space_data_mappings.hpp"


BW_BEGIN_NAMESPACE

class BWSpaceDataListener;
class BWSpaceDataStorage;
class BWStreamDataHandler;
class BWEntities;
class EntityBase;


/**
 *	This class is used to handle messages from the server.
 */
class BWServerMessageHandler : public ServerMessageHandler
{
public:
	BWServerMessageHandler( BWEntities & entities,
		BWSpaceDataStorage & spaceDataStorage );
	virtual ~BWServerMessageHandler();

	void addSpaceDataListener( BWSpaceDataListener * pListener );
	bool removeSpaceDataListener( BWSpaceDataListener * pListener );

	void registerStreamDataHandler( BWStreamDataHandler * pHandler,
		uint16 streamID );
	void deregisterStreamDataHandler( BWStreamDataHandler * pHandler,
		uint16 streamID );
	void setStreamDataFallbackHandler( BWStreamDataHandler * pHandler );

	void clearSpace( SpaceID spaceID );

	// Overrides from ServerMessageHandler
	virtual void onBasePlayerCreate( EntityID id, EntityTypeID type, 
			BinaryIStream & data );

	virtual void onCellPlayerCreate( EntityID id, SpaceID spaceID, 
		EntityID vehicleID, const Position3D & position, 
		float yaw, float pitch, float roll, 
		BinaryIStream & data );

	virtual void onEntityControl( EntityID id, bool isControlling );

	virtual const CacheStamps * onEntityCacheTest( EntityID id );

	virtual void onEntityLeave( EntityID id, const CacheStamps & stamps );

	virtual void onEntityCreate( EntityID id, EntityTypeID type, 
		SpaceID spaceID, EntityID vehicleID, const Position3D & position,
		float yaw, float pitch, float roll, BinaryIStream & data );

	virtual void onEntityProperties( EntityID id, BinaryIStream & data );

	virtual void onEntityMethod( EntityID id, int methodID,
		BinaryIStream & data );

	virtual void onEntityProperty( EntityID id, int propertyID,
		BinaryIStream & data );

	virtual void onNestedEntityProperty( EntityID id, BinaryIStream & data,
		   bool isSlice );

	virtual int getEntityMethodStreamSize(
		EntityID id,
		int methodID,
		bool isFailAllowed = false ) const;

	virtual int getEntityPropertyStreamSize(
		EntityID id,
		int propertyID,
		bool isFailAllowed = false ) const;

	virtual void onEntityMoveWithError( EntityID id, SpaceID spaceID, 
		EntityID vehicleID,
		const Position3D & pos, const Vector3 & posError,
		float yaw, float pitch, float roll,
		bool isVolatile );

	virtual void onEntitiesReset( bool keepPlayerOnBase );

	virtual void onRestoreClient( EntityID id, SpaceID spaceID,
		EntityID vehicleID, const Position3D & pos, const Direction3D & dir,
		BinaryIStream & data );

	virtual void spaceData( SpaceID spaceID, SpaceEntryID entryID, uint16 key,
		const BW::string & data );

	virtual void spaceGone( SpaceID spaceID );

	virtual void onStreamComplete( uint16 id, const BW::string & description,
		BinaryIStream & data );

private:
	BWEntities & 						entities_;
	BWSpaceDataStorage &				spaceDataStorage_;

	typedef BW::vector< BWSpaceDataListener * > SpaceDataListeners;
	SpaceDataListeners spaceDataListeners_;

	typedef BW::map< uint16, BWStreamDataHandler * > StreamDataHandlers;
	StreamDataHandlers streamDataHandlers_;
	BWStreamDataHandler * pStreamDataFallbackHandler_;
};

BW_END_NAMESPACE

#endif // BW_SERVER_MESSAGE_HANDLER_HPP

