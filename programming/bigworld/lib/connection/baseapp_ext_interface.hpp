#if defined( DEFINE_INTERFACE_HERE ) || defined( DEFINE_SERVER_HERE )
	#undef BASEAPP_EXT_INTERFACE_HPP
#endif

#ifndef BASEAPP_EXT_INTERFACE_HPP
#define BASEAPP_EXT_INTERFACE_HPP

#undef INTERFACE_NAME
#define INTERFACE_NAME BaseAppExtInterface
#include "common_baseapp_interface.hpp"

// -----------------------------------------------------------------------------
// Section: Includes
// -----------------------------------------------------------------------------

#include "network/msgtypes.hpp"
#include "network/exposed_message_range.hpp"

BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: BaseApp External Interface
// -----------------------------------------------------------------------------

#pragma pack( push, 1 )
BEGIN_MERCURY_INTERFACE( BaseAppExtInterface )

	// let the proxy know who we really are
	BW_BEGIN_STRUCT_MSG_EX( BaseApp, baseAppLogin )
		SessionKey	key;
		uint8		numAttempts;
	END_STRUCT_MESSAGE()
	MERCURY_ISTREAM( baseAppLogin, x.key >> x.numAttempts )
	MERCURY_OSTREAM( baseAppLogin, x.key << x.numAttempts )

	// let the proxy know who we really are
	BW_BEGIN_STRUCT_MSG_EX( BaseApp, authenticate )
		SessionKey		key;
	END_STRUCT_MESSAGE()
	MERCURY_ISTREAM( authenticate, x.key )
	MERCURY_OSTREAM( authenticate, x.key )

#if VOLATILE_POSITIONS_ARE_ABSOLUTE
	// send an update for our position.
	MF_BEGIN_BLOCKABLE_PROXY_MSG( avatarUpdateImplicit )
		Position3D		pos;
		YawPitchRoll	dir;
	END_STRUCT_MESSAGE();
	MERCURY_ISTREAM( avatarUpdateImplicit, x.pos >> x.dir )
	MERCURY_OSTREAM( avatarUpdateImplicit, x.pos << x.dir )

	// Bit values for the 'flags' member: Must match cellapp_interface.hpp
#define AVATAR_UPDATE_EXPLICT_FLAG_ONGROUND 0x1
	MF_BEGIN_BLOCKABLE_PROXY_MSG( avatarUpdateExplicit )
		EntityID		vehicleID;
		Position3D		pos;
		YawPitchRoll	dir;
		uint8			flags;
	END_STRUCT_MESSAGE();
	MERCURY_ISTREAM( avatarUpdateExplicit,
		x.vehicleID >> x.pos >> x.dir >> x.flags );
	MERCURY_OSTREAM( avatarUpdateExplicit,
		x.vehicleID << x.pos << x.dir << x.flags );

#else /* VOLATILE_POSITIONS_ARE_ABSOLUTE */
	// send an update for our position. refNum is used to refer to this position
	// later as the base for relative positions.
	MF_BEGIN_BLOCKABLE_PROXY_MSG( avatarUpdateImplicit )
		Position3D		pos;
		YawPitchRoll	dir;
		uint8			refNum;
	END_STRUCT_MESSAGE();
	MERCURY_ISTREAM( avatarUpdateImplicit, x.pos >> x.dir >> x.refNum )
	MERCURY_OSTREAM( avatarUpdateImplicit, x.pos << x.dir << x.refNum )

	// Bit values for the 'flags' member: Must match cellapp_interface.hpp
#define AVATAR_UPDATE_EXPLICT_FLAG_ONGROUND 0x1
	MF_BEGIN_BLOCKABLE_PROXY_MSG( avatarUpdateExplicit )
		EntityID		vehicleID;
		Position3D		pos;
		YawPitchRoll	dir;
		uint8			flags;
		uint8			refNum;
	END_STRUCT_MESSAGE();
	MERCURY_ISTREAM( avatarUpdateExplicit,
		x.vehicleID >> x.pos >> x.dir >> x.flags >> x.refNum );
	MERCURY_OSTREAM( avatarUpdateExplicit,
		x.vehicleID << x.pos << x.dir << x.flags << x.refNum );

#endif /* VOLATILE_POSITIONS_ARE_ABSOLUTE */

	MF_BEGIN_BLOCKABLE_PROXY_MSG( avatarUpdateWardImplicit )
		EntityID		ward;
		Position3D		pos;
		YawPitchRoll	dir;
	END_STRUCT_MESSAGE();
	MERCURY_ISTREAM( avatarUpdateWardImplicit, x.ward >> x.pos >> x.dir )
	MERCURY_OSTREAM( avatarUpdateWardImplicit, x.ward << x.pos << x.dir )

	// Bit values for the 'flags' member are those from avatarUpdateExplicit
	MF_BEGIN_BLOCKABLE_PROXY_MSG( avatarUpdateWardExplicit )
		EntityID		ward;
		SpaceID			spaceID;
		EntityID		vehicleID;
		Position3D		pos;
		YawPitchRoll	dir;
		uint8			flags;
	END_STRUCT_MESSAGE();
	MERCURY_ISTREAM( avatarUpdateWardExplicit,
		x.ward >> x.spaceID >> x.vehicleID >> x.pos >> x.dir >> x.flags );
	MERCURY_OSTREAM( avatarUpdateWardExplicit,
		x.ward << x.spaceID << x.vehicleID << x.pos << x.dir << x.flags );

	MF_EMPTY_BLOCKABLE_PROXY_MSG( ackPhysicsCorrection )

	MF_BEGIN_BLOCKABLE_PROXY_MSG( ackWardPhysicsCorrection )
		EntityID ward;
	END_STRUCT_MESSAGE()
	MERCURY_ISTREAM( ackWardPhysicsCorrection, x.ward )
	MERCURY_OSTREAM( ackWardPhysicsCorrection, x.ward )

	// requestEntityUpdate:
	MF_VARLEN_BLOCKABLE_PROXY_MSG( requestEntityUpdate )
	//	EntityID		id;
	//	EventNumber[]	lastEventNumbers;

	MF_EMPTY_BLOCKABLE_PROXY_MSG( enableEntities )

	MF_BEGIN_UNBLOCKABLE_PROXY_MSG( restoreClientAck )
		int					id;
	END_STRUCT_MESSAGE();
	MERCURY_ISTREAM( restoreClientAck, x.id )
	MERCURY_OSTREAM( restoreClientAck, x.id )

	MF_BEGIN_UNBLOCKABLE_PROXY_MSG( disconnectClient )
		uint8 reason;
	END_STRUCT_MESSAGE()
	MERCURY_ISTREAM( disconnectClient, x.reason )
	MERCURY_OSTREAM( disconnectClient, x.reason )

	MF_METHOD_RANGE_MSG( cellEntityMethod, 2 )
	MF_METHOD_RANGE_MSG( baseEntityMethod, 1 )

END_MERCURY_INTERFACE()

#pragma pack( pop )

BW_END_NAMESPACE

#endif // BASEAPP_EXT_INTERFACE_HPP
