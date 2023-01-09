#if defined(DEFINE_INTERFACE_HERE) || defined(DEFINE_SERVER_HERE)
	#undef CLIENT_INTERFACE_HPP
#endif


#ifndef CLIENT_INTERFACE_HPP
#define CLIENT_INTERFACE_HPP

#include "network/interface_macros.hpp"
#include "network/msgtypes.hpp"
#include "network/network_interface.hpp"

BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: Helper macros
// -----------------------------------------------------------------------------

#undef MF_EMPTY_CLIENT_MSG
#define MF_EMPTY_CLIENT_MSG( NAME )											\
	MERCURY_HANDLED_EMPTY_MESSAGE( NAME,									\
			ClientMessageHandler< void >,									\
			&ServerConnection::NAME )										\

#undef MF_BEGIN_CLIENT_MSG
#define MF_BEGIN_CLIENT_MSG( NAME )											\
	BEGIN_HANDLED_STRUCT_MESSAGE( NAME,										\
			ClientMessageHandler< ClientInterface::NAME##Args >,			\
			&ServerConnection::NAME )										\

#undef MF_BEGIN_CLIENT_MSG_EX
#define MF_BEGIN_CLIENT_MSG_EX( NAME )										\
	BEGIN_HANDLED_STRUCT_MESSAGE( NAME,										\
			ClientMessageHandlerEx< ClientInterface::NAME##Args >,			\
			&ServerConnection::NAME )										\

#undef MF_VARLEN_CLIENT_MSG
#define MF_VARLEN_CLIENT_MSG( NAME )										\
	MERCURY_HANDLED_VARIABLE_MESSAGE( NAME, 2,								\
			ClientVarLenMessageHandler,	&ServerConnection::NAME )

#undef MF_VARLEN_CLIENT_MSG_EX
#define MF_VARLEN_CLIENT_MSG_EX( NAME )										\
	MERCURY_HANDLED_VARIABLE_MESSAGE( NAME, 2,								\
			ClientVarLenMessageHandlerEx,	&ServerConnection::NAME )

#undef MF_CALLBACK_MSG
#define MF_CALLBACK_MSG( NAME )												\
	MERCURY_HANDLED_CALLBACK_MESSAGE( NAME,									\
			ClientCallbackMessageHandler,									\
			std::make_pair( &ServerConnection::NAME,						\
				&ServerConnection::NAME ## GetStreamSize ) )


// -----------------------------------------------------------------------------
// Section: Client interface
// -----------------------------------------------------------------------------

#pragma pack(push, 1)
BEGIN_MERCURY_INTERFACE( ClientInterface )

	MF_BEGIN_CLIENT_MSG( authenticate )
		uint32				key;
	END_STRUCT_MESSAGE()
	MERCURY_ISTREAM( authenticate, x.key )
	MERCURY_OSTREAM( authenticate, x.key )

	MF_BEGIN_CLIENT_MSG( bandwidthNotification )
		uint32				bps;
	END_STRUCT_MESSAGE()
	MERCURY_ISTREAM( bandwidthNotification, x.bps )
	MERCURY_OSTREAM( bandwidthNotification, x.bps )

	MF_BEGIN_CLIENT_MSG( updateFrequencyNotification )
		uint8				hertz;
	END_STRUCT_MESSAGE()
	MERCURY_ISTREAM( updateFrequencyNotification, x.hertz )
	MERCURY_OSTREAM( updateFrequencyNotification, x.hertz )

	MF_BEGIN_CLIENT_MSG( setGameTime )
		GameTime			gameTime;
	END_STRUCT_MESSAGE()
	MERCURY_ISTREAM( setGameTime, x.gameTime )
	MERCURY_OSTREAM( setGameTime, x.gameTime )

	MF_BEGIN_CLIENT_MSG( resetEntities )
		bool			keepPlayerOnBase;
	END_STRUCT_MESSAGE()
	MERCURY_ISTREAM( resetEntities, x.keepPlayerOnBase )
	MERCURY_OSTREAM( resetEntities, x.keepPlayerOnBase )

	MF_VARLEN_CLIENT_MSG( createBasePlayer )
	MF_VARLEN_CLIENT_MSG( createCellPlayer )

	MF_VARLEN_CLIENT_MSG( spaceData )
	//	EntityID		spaceID
	//	SpaceEntryID	entryID
	//	uint16			key;
	//	char[]			value;		// rest of message

	MF_VARLEN_CLIENT_MSG( createEntity )
	MF_VARLEN_CLIENT_MSG( createEntityDetailed )

	MF_BEGIN_CLIENT_MSG( enterAoI )
		EntityID			id;
		IDAlias				idAlias;
	END_STRUCT_MESSAGE()
	MERCURY_ISTREAM( enterAoI, x.id >> x.idAlias )
	MERCURY_OSTREAM( enterAoI, x.id << x.idAlias )

	MF_BEGIN_CLIENT_MSG( enterAoIOnVehicle )
		EntityID			id;
		EntityID			vehicleID;
		IDAlias				idAlias;
	END_STRUCT_MESSAGE()
	MERCURY_ISTREAM( enterAoIOnVehicle,
		x.id >> x.vehicleID >> x.idAlias )
	MERCURY_OSTREAM( enterAoIOnVehicle,
		x.id << x.vehicleID << x.idAlias )

	MF_VARLEN_CLIENT_MSG( leaveAoI )
	//	EntityID		id;
	//	EventNumber[]	lastEventNumbers;	// rest


	// The interface that is shared with the base app.
#define MF_EMPTY_COMMON_RELIABLE_MSG MF_EMPTY_CLIENT_MSG
#define MF_BEGIN_COMMON_UNRELIABLE_MSG MF_BEGIN_CLIENT_MSG
#define MF_BEGIN_COMMON_PASSENGER_MSG MF_BEGIN_CLIENT_MSG
#define MF_BEGIN_COMMON_RELIABLE_MSG MF_BEGIN_CLIENT_MSG
#include "common_client_interface.hpp"
#undef MF_EMPTY_COMMON_RELIABLE_MSG
#undef MF_BEGIN_COMMON_UNRELIABLE_MSG
#undef MF_BEGIN_COMMON_PASSENGER_MSG
#undef MF_BEGIN_COMMON_RELIABLE_MSG


	MF_BEGIN_CLIENT_MSG( controlEntity )
		EntityID		id;
		bool			on;
	END_STRUCT_MESSAGE()
	MERCURY_ISTREAM( controlEntity, x.id >> x.on )
	MERCURY_OSTREAM( controlEntity, x.id << x.on )


	MF_VARLEN_CLIENT_MSG_EX( voiceData )

	MF_VARLEN_CLIENT_MSG( restoreClient )

	MF_BEGIN_CLIENT_MSG_EX( switchBaseApp )
		Mercury::Address	baseAddr;
		bool				shouldResetEntities;
	END_STRUCT_MESSAGE()
	MERCURY_ISTREAM( switchBaseApp, x.baseAddr >> x.shouldResetEntities )
	MERCURY_OSTREAM( switchBaseApp, x.baseAddr << x.shouldResetEntities )

	MF_VARLEN_CLIENT_MSG( resourceHeader )

	MF_VARLEN_CLIENT_MSG( resourceFragment )
	// ResourceFragmentArgs
#ifndef CLIENT_INTERFACE_HPP_ONCE
	struct ResourceFragmentArgs {
		uint16			rid;
		uint8			seq;
		uint8			flags; };	// 1 = first (method in seq), 2 = final
	//	uint8			data[];		// rest
#endif

	MF_BEGIN_CLIENT_MSG( loggedOff )
		uint8	reason;
	END_STRUCT_MESSAGE()
	MERCURY_ISTREAM( loggedOff, x.reason )
	MERCURY_OSTREAM( loggedOff, x.reason )

	// Messages that rely on the select*Entity messages from
	// common_client_interface.hpp, as they are streamed out to all
	// relevant clients.

	// This message is used to send an accurate position of an entity down to
	// the client. It is sent when the volatile information of an entity becomes
	// less volatile, or the entity teleports.
	MF_BEGIN_CLIENT_MSG( detailedPosition )
		Position3D		position;
		Direction3D		direction;
	END_STRUCT_MESSAGE()
	MERCURY_ISTREAM( detailedPosition, x.position >> x.direction )
	MERCURY_OSTREAM( detailedPosition, x.position << x.direction )

	MF_VARLEN_CLIENT_MSG( nestedEntityProperty )
	MF_VARLEN_CLIENT_MSG( sliceEntityProperty )
	MF_VARLEN_CLIENT_MSG( updateEntity )

	MF_CALLBACK_MSG( entityMethod )
	MERCURY_METHOD_RANGE_MSG( entityMethod, 2 ) /* 1/2 of the remaining range */

	MF_CALLBACK_MSG( entityProperty )
	MERCURY_PROPERTY_RANGE_MSG( entityProperty, 1 ) /* All of the remaining range */

END_MERCURY_INTERFACE()
#pragma pack( pop )


#ifndef CLIENT_INTERFACE_HPP_ONCE

inline BinaryIStream & operator>>( BinaryIStream & in,
				ClientInterface::ResourceFragmentArgs & out )
{
	in >> out.rid;
	in >> out.seq;
	in >> out.flags;
	return in;
}


inline BinaryOStream & operator<<( BinaryOStream & out,
		const ClientInterface::ResourceFragmentArgs & in )
{
	out << in.rid;
	out << in.seq;
	out << in.flags;
	return out;
}

#endif // CLIENT_INTERFACE_HPP_ONCE


#define CLIENT_INTERFACE_HPP_ONCE

BW_END_NAMESPACE

#endif // CLIENT_INTERFACE_HPP
