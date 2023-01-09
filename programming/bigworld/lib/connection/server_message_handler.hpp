#ifndef SERVER_MESSAGE_HANDLER_HPP
#define SERVER_MESSAGE_HANDLER_HPP

#include "network/basictypes.hpp"

#include "cstdmf/bw_string.hpp"

BW_BEGIN_NAMESPACE

class BinaryIStream;
class LoginHandler;

/**
 *	This abstract class defines the interface that is called by
 *	ServerConnection::processInput.
 *
 *	@ingroup network
 */
class ServerMessageHandler
{
public:
	/// This method is called to create a new player as far as required to
	/// talk to the base entity. Only data shared between the base and the
	/// client is provided in this method - the cell data will be provided by
	/// onCellPlayerCreate later if the player is put on the cell also.
	virtual void onBasePlayerCreate( EntityID id, EntityTypeID type,
		BinaryIStream & data ) = 0;

	/// This method is called to create a new player as far as required to
	/// talk to the cell entity. Only data shared between the cell and the
	/// client is provided in this method - the base data will have been
	/// previously provided by onBasePlayerCreate.
	virtual void onCellPlayerCreate( EntityID id,
		SpaceID spaceID, EntityID vehicleID, const Position3D & pos,
		float yaw, float pitch, float roll, BinaryIStream & data ) = 0;

	/// This method is called to indicate that the given entity is controlled
	/// by this client, at least as far as pose updates go. i.e. You may
	/// move the entity around with calls to addMove. Currently, control is
	/// implicit for the player entity and may not be withdrawn.
	virtual void onEntityControl( EntityID id, bool control ) { }

	/// This method is called to check whether the client has a cache of an
	/// entity.
	///	@param id The id of the entity to test.
	///
	/// @return A pointer to the cache stamps stored from the call to
	///		onEntityLeave, or NULL if this is no cache.
	virtual const CacheStamps * onEntityCacheTest( EntityID id )
	{
		return NULL;
	}

	/// This method is called when an entity leaves the client's AoI.
	/// The CacheStamps are a record of the data received about the entity to
	/// this point, to be returned in onEntityCacheTest if the Entity's data
	/// is cached for later reuse.
	virtual void onEntityLeave( EntityID id, const CacheStamps & stamps ) = 0;

	/// This method is called in response to provide the bulk of the information
	/// about this entity. If the client has seen this entity before then data
	/// it already has is not resent (as determined by the CacheStamps returned
	/// from onEntityCacheTest)
	virtual void onEntityCreate( EntityID id, EntityTypeID type,
		SpaceID spaceID, EntityID vehicleID, const Position3D & pos,
		float yaw, float pitch, float roll, BinaryIStream & data ) = 0;

	/// This method is called by the server when it wants to provide multiple
	/// properties at once to the client for an entity in its AoI, such as when
	/// a detail level boundary is crossed.
	virtual void onEntityProperties( EntityID id, BinaryIStream & data ) = 0;

	/// This method is called when the server wants to call an entity method.
	virtual void onEntityMethod( EntityID id, int methodID,
		BinaryIStream & data ) = 0;

	/// This method is called when the server wants to update an entity
	/// property.
	virtual void onEntityProperty( EntityID id, int propertyID,
		BinaryIStream & data ) = 0;

	/// This method is called when the server wants to update a nested entity
	/// property.
	virtual void onNestedEntityProperty( EntityID id,
			BinaryIStream & data, bool isSlice ) = 0;

	/// This method returns the stream size of the method call message to an
	/// entity. For fixed-length messages, this is the number of bytes, for
	/// variable-length messages, a negative number is used where the negative
	/// of the return value is interpreted as the preferred number of bytes to
	/// describe the variable length.
	virtual int getEntityMethodStreamSize( EntityID id,
			int methodID, bool isFailAllowed = false ) const = 0;

	/// This method returns the stream size of the property update message to
	/// an entity. For fixed-length messages, this is the number of bytes, for
	/// variable-length messages, a negative number is used where the negative
	/// of the return value is interpreted as the preferred number of bytes to
	/// describe the variable length.
	virtual int getEntityPropertyStreamSize( EntityID id,
			int propertyID, bool isFailAllowed = false ) const = 0;

	/// This method is called when the position of an entity changes.
	/// This will only be received for a controlled entity if the server
	/// overrides the position directly (physics correction, teleport, etc.)
	virtual void onEntityMoveWithError( EntityID id,
					SpaceID spaceID, EntityID vehicleID,
					const Position3D & pos, const Vector3 & posError,
					float yaw, float pitch, float roll,
					bool isVolatile ) {}

	/// This method is called when data associated with a space is received.
	virtual void spaceData( SpaceID spaceID, SpaceEntryID entryID,
		uint16 key, const BW::string & data ) = 0;

	/// This method is called when the given space is no longer visible
	/// to the client.
	virtual void spaceGone( SpaceID spaceID ) = 0;

	/// This method is called to deliver peer-to-peer data.
	virtual void onVoiceData( const Mercury::Address & srcAddr,
		BinaryIStream & data ) {}

	/// This method is called to deliver downloads when they are complete.  This
	/// was called onProxyData() in BW1.8.x and earlier, and didn't have the
	/// description parameter.
	virtual void onStreamComplete( uint16 id, const BW::string &desc,
		BinaryIStream & data ) {}

	/// This method is called when the server tells us to reset all our
	/// entities. The player entity may optionally be saved (but still
	/// should not be considered to be in the world (i.e. no cell part yet))
	virtual void onEntitiesReset( bool keepPlayerOnBase ) {}

	/// This method is called to indicate that the client entity has been
	/// restored from a (recent) backup due to a failure on the server.
	virtual void onRestoreClient( EntityID id,
		SpaceID spaceID, EntityID vehicleID,
		const Position3D & pos, const Direction3D & dir,
		BinaryIStream & data ) {}

	/// This method is called when the client connection is being migrated 
	/// to another BaseApp. This results in a BaseApp login request being 
	/// sent to the new BaseApp. The handler for the request is passed into 
	/// this method.
	virtual void onSwitchBaseApp( LoginHandler & loginHandler ) {}
};

BW_END_NAMESPACE

#endif // SERVER_MESSAGE_HANDLER_HPP

