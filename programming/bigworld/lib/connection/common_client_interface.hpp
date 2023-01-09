/**
 *	This file contains the interface from the cell to the client. It's called
 *	common_client_interface because it is common between client_interface.hpp
 *	and proxy_int_interface.hpp. These two interface files include this file to
 *	add these messages to those interfaces. They define the
 *	MF_EMPTY_COMMON_RELIABLE_MSG, MF_BEGIN_COMMON_RELIABLE_MSG,
 *	MF_BEGIN_COMMON_PASSENGER_MSG, and MF_BEGIN_COMMON_UNRELIABLE_MSG macros to
 *	their interface specific macro.
 *
 *	This file is also used by proxy.hpp and server_connection.hpp for the
 *	declaration of the handler methods of these messages and is included by
 *	proxy.cpp to implement the pass-through handler methods of these messages.
 */

#include "cstdmf/bw_namespace.hpp"

// -----------------------------------------------------------------------------
// Section: Macros
// -----------------------------------------------------------------------------

// This macro is used to decide whether or not to keep the arguments. For the
// non-interface files that include this file, they tend to not want the
// arguments and so define this macro as nothing.
#ifndef MF_COMMON_ARGS
#define MF_COMMON_ARGS( ARGS ) ARGS
#define MF_COMMON_ISTREAM( NAME, XSTREAM ) MERCURY_ISTREAM( NAME, XSTREAM )
#define MF_COMMON_OSTREAM( NAME, XSTREAM ) MERCURY_OSTREAM( NAME, XSTREAM )
#endif

// This macro is used to decide whether what should appear at the end of each
// message. For the non-interface files that include this file, they tend to not
// want anything and so define this macro as nothing.
#ifndef MF_END_COMMON_MSG
#define MF_END_COMMON_MSG END_STRUCT_MESSAGE
#endif

// -----------------------------------------------------------------------------
// Section: General cell to client messages
// -----------------------------------------------------------------------------

MF_BEGIN_COMMON_PASSENGER_MSG( tickSync )
MF_COMMON_ARGS(
	uint8				tickByte; )
MF_END_COMMON_MSG()
MF_COMMON_ISTREAM( tickSync, x.tickByte )
MF_COMMON_OSTREAM( tickSync, x.tickByte )

#if !VOLATILE_POSITIONS_ARE_ABSOLUTE
// This message indicates the base position that is used for subsequent relative
// positions. It is the sequenceNumber that was used to send this position from
// the client to the server.
MF_BEGIN_COMMON_UNRELIABLE_MSG( relativePositionReference )
MF_COMMON_ARGS(
	uint8				sequenceNumber; )
MF_END_COMMON_MSG()
MF_COMMON_ISTREAM( relativePositionReference, x.sequenceNumber )
MF_COMMON_OSTREAM( relativePositionReference, x.sequenceNumber )

// This message indicates the base position that is used for subsequent relative
// positions.
MF_BEGIN_COMMON_UNRELIABLE_MSG( relativePosition )
MF_COMMON_ARGS(
	Position3D			position; )
MF_END_COMMON_MSG()
MF_COMMON_ISTREAM( relativePosition, x.position )
MF_COMMON_OSTREAM( relativePosition, x.position )
#endif /* !VOLATILE_POSITIONS_ARE_ABSOLUTE */


// This message indicates that the next position update is in the context of
// the given vehicle id, which may not be the one currently associated with
// that entity.
MF_BEGIN_COMMON_RELIABLE_MSG( setVehicle )
MF_COMMON_ARGS(
	EntityID			passengerID;
	EntityID			vehicleID; )
MF_END_COMMON_MSG()
MF_COMMON_ISTREAM( setVehicle, x.passengerID >> x.vehicleID )
MF_COMMON_OSTREAM( setVehicle, x.passengerID << x.vehicleID )

MF_BEGIN_COMMON_RELIABLE_MSG( selectAliasedEntity )
MF_COMMON_ARGS(
		IDAlias idAlias; )
MF_END_COMMON_MSG()
MF_COMMON_ISTREAM( selectAliasedEntity, x.idAlias )
MF_COMMON_OSTREAM( selectAliasedEntity, x.idAlias )

MF_BEGIN_COMMON_RELIABLE_MSG( selectEntity )
MF_COMMON_ARGS(
		EntityID id; )
MF_END_COMMON_MSG()
MF_COMMON_ISTREAM( selectEntity, x.id )
MF_COMMON_OSTREAM( selectEntity, x.id )

MF_EMPTY_COMMON_RELIABLE_MSG( selectPlayerEntity )

// This is used to send a physics correction or other server-set position
// to the client for an entity that it is controlling
MF_BEGIN_COMMON_RELIABLE_MSG( forcedPosition )
MF_COMMON_ARGS(
	EntityID		id;
	SpaceID			spaceID;
	EntityID		vehicleID;
	Position3D		position;
	Direction3D		direction; )
MF_END_COMMON_MSG()
MF_COMMON_ISTREAM( forcedPosition,
	x.id >> x.spaceID >> x.vehicleID >> x.position >> x.direction )
MF_COMMON_OSTREAM( forcedPosition,
	x.id << x.spaceID << x.vehicleID << x.position << x.direction )

// -----------------------------------------------------------------------------
// Section: avatarUpdate messages
// -----------------------------------------------------------------------------

// These three messages are sent to carry a full-detail volatile position, for
// cases where the position is too far from the reference position for PackedXYZ
// or we desire more precision than PackedXYZ, at the cost of more bandwidth.
// They are all detailed enough to be reference positions, but only
// avatarUpdatePlayerDetailed is actually used for that as only the player
// itself ever has its position used as the reference position.
MF_BEGIN_COMMON_UNRELIABLE_MSG( avatarUpdateNoAliasDetailed )
MF_COMMON_ARGS(
	EntityID		id;
	Position3D		position;
	Direction3D		dir; )
MF_END_COMMON_MSG()
MF_COMMON_ISTREAM( avatarUpdateNoAliasDetailed,
	x.id >> x.position >> x.dir )
MF_COMMON_OSTREAM( avatarUpdateNoAliasDetailed,
	x.id << x.position << x.dir )

MF_BEGIN_COMMON_UNRELIABLE_MSG( avatarUpdateAliasDetailed )
MF_COMMON_ARGS(
	IDAlias			idAlias;
	Position3D		position;
	Direction3D		dir; )
MF_END_COMMON_MSG()
MF_COMMON_ISTREAM( avatarUpdateAliasDetailed,
	x.idAlias >> x.position >> x.dir )
MF_COMMON_OSTREAM( avatarUpdateAliasDetailed,
	x.idAlias << x.position << x.dir )

MF_BEGIN_COMMON_UNRELIABLE_MSG( avatarUpdatePlayerDetailed )
MF_COMMON_ARGS(
	Position3D		position;
	Direction3D		dir; )
MF_END_COMMON_MSG()
MF_COMMON_ISTREAM( avatarUpdatePlayerDetailed,
	x.position >> x.dir )
MF_COMMON_OSTREAM( avatarUpdatePlayerDetailed,
	x.position << x.dir )

// The AVUPID_STREAM_* defines don't need the <</>> in front of them like the
// AVUPPOS and AVUPDIR macros because there's already a streaming operator after
// the stream in the MERCURY_[IO]STREAM() macros.  We need to include the
// streaming operator as part of the define because of the defines that leave
// out the argument entirely (i.e. AVUPPOS_NoPos and AVUPDIR_NoDir).
//
// Thankfully the ID is always part of the message, otherwise we'd have to edit
// the MERCURY_ISTREAM()/MERCURY_OSTREAM() macros to not automatically include
// the first <</>> operator after the BinaryIOStream.  Small mercies eh? :)

#define AVUPID_NoAlias			EntityID		id;
#define AVUPID_STREAM_NoAlias					x.id

#define AVUPID_Alias			IDAlias			idAlias;
#define AVUPID_STREAM_Alias						x.idAlias

// --

#define AVUPPOS_FullPos			PackedXYZ		position;
#define AVUPPOS_ISTREAM_FullPos					>> x.position
#define AVUPPOS_OSTREAM_FullPos					<< x.position

#define AVUPPOS_OnGround		PackedXZ		position;
#define AVUPPOS_ISTREAM_OnGround				>> x.position
#define AVUPPOS_OSTREAM_OnGround				<< x.position

#define AVUPPOS_NoPos
#define AVUPPOS_ISTREAM_NoPos
#define AVUPPOS_OSTREAM_NoPos

// --

#define AVUPDIR_YawPitchRoll	YawPitchRoll	dir;
#define AVUPDIR_ISTREAM_YawPitchRoll			>> x.dir
#define AVUPDIR_OSTREAM_YawPitchRoll			<< x.dir

#define AVUPDIR_YawPitch		YawPitch		dir;
#define AVUPDIR_ISTREAM_YawPitch				>> x.dir
#define AVUPDIR_OSTREAM_YawPitch				<< x.dir

#define AVUPDIR_Yaw				Yaw				dir;
#define AVUPDIR_ISTREAM_Yaw						>> x.dir
#define AVUPDIR_OSTREAM_Yaw						<< x.dir

#define AVUPDIR_NoDir
#define AVUPDIR_ISTREAM_NoDir
#define AVUPDIR_OSTREAM_NoDir

#ifdef AVUPMSG
#undef AVUPMSG
#endif

#define AVUPMSG( ID, POS, DIR )											\
	MF_BEGIN_COMMON_UNRELIABLE_MSG( avatarUpdate##ID##POS##DIR )		\
	MF_COMMON_ARGS(														\
		AVUPID_##ID														\
		AVUPPOS_##POS													\
		AVUPDIR_##DIR )													\
	MF_END_COMMON_MSG()													\
	MF_COMMON_ISTREAM( avatarUpdate##ID##POS##DIR,						\
		AVUPID_STREAM_##ID AVUPPOS_ISTREAM_##POS AVUPDIR_ISTREAM_##DIR )\
	MF_COMMON_OSTREAM( avatarUpdate##ID##POS##DIR,						\
		AVUPID_STREAM_##ID AVUPPOS_OSTREAM_##POS AVUPDIR_OSTREAM_##DIR )\


	AVUPMSG( NoAlias, FullPos, YawPitchRoll )
	AVUPMSG( NoAlias, FullPos, YawPitch )
	AVUPMSG( NoAlias, FullPos, Yaw )
	AVUPMSG( NoAlias, FullPos, NoDir )
	AVUPMSG( NoAlias, OnGround, YawPitchRoll )
	AVUPMSG( NoAlias, OnGround, YawPitch )
	AVUPMSG( NoAlias, OnGround, Yaw )
	AVUPMSG( NoAlias, OnGround, NoDir )
	AVUPMSG( NoAlias, NoPos, YawPitchRoll )
	AVUPMSG( NoAlias, NoPos, YawPitch )
	AVUPMSG( NoAlias, NoPos, Yaw )
	AVUPMSG( NoAlias, NoPos, NoDir )
	AVUPMSG( Alias, FullPos, YawPitchRoll )
	AVUPMSG( Alias, FullPos, YawPitch )
	AVUPMSG( Alias, FullPos, Yaw )
	AVUPMSG( Alias, FullPos, NoDir )
	AVUPMSG( Alias, OnGround, YawPitchRoll )
	AVUPMSG( Alias, OnGround, YawPitch )
	AVUPMSG( Alias, OnGround, Yaw )
	AVUPMSG( Alias, OnGround, NoDir )
	AVUPMSG( Alias, NoPos, YawPitchRoll )
	AVUPMSG( Alias, NoPos, YawPitch )
	AVUPMSG( Alias, NoPos, Yaw )
	AVUPMSG( Alias, NoPos, NoDir )


#undef MF_EMPTY_COMMON_RELIABLE_MSG
#undef MF_BEGIN_COMMON_UNRELIABLE_MSG
#undef MF_BEGIN_COMMON_PASSENGER_MSG
#undef MF_BEGIN_COMMON_RELIABLE_MSG
#undef MF_COMMON_ARGS
#undef MF_END_COMMON_MSG
#undef MF_COMMON_ISTREAM
#undef MF_COMMON_OSTREAM

// common_client_interface.hpp
