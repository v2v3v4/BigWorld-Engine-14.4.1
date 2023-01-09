#ifndef INTERFACE_MACROS_HPP
	#define INTERFACE_MACROS_HPP

	#include "bundle.hpp"
	#include "exposed_message_range.hpp"
	#include "interface_minder.hpp"
	#include "network_interface.hpp"

	// Helper macros

	#define MERCURY_FIXED_MESSAGE( NAME, PARAM, HANDLER )					\
		MERCURY_MESSAGE( NAME, FIXED_LENGTH_MESSAGE, PARAM, HANDLER )		\

	#define MERCURY_VARIABLE_MESSAGE( NAME, PARAM, HANDLER )				\
		MERCURY_MESSAGE( NAME, VARIABLE_LENGTH_MESSAGE, PARAM, HANDLER )	\

	#define MERCURY_CALLBACK_MESSAGE( NAME, HANDLER )						\
		MERCURY_MESSAGE( NAME, CALLBACK_LENGTH_MESSAGE, 0, HANDLER )		\

	#define MERCURY_EMPTY_MESSAGE( NAME, HANDLER )							\
		MERCURY_MESSAGE( NAME, FIXED_LENGTH_MESSAGE, 0, HANDLER )			\

	#define BEGIN_HANDLED_STRUCT_MESSAGE( NAME, HANDLERTYPE, HANDLERARG )	\
		MERCURY_HANDLED_STRUCT_MESSAGE( NAME, HANDLERTYPE, HANDLERARG )		\
		{																	\
			MERCURY_STRUCT_GOODIES( NAME )									\

	#define END_HANDLED_STRUCT_MESSAGE()									\
		};																	\

	#define BEGIN_STRUCT_MESSAGE( NAME, HANDLER )							\
		MERCURY_STRUCT_MESSAGE( NAME, HANDLER )								\
		{																	\
			MERCURY_STRUCT_GOODIES( NAME )									\

	#define END_STRUCT_MESSAGE()											\
		};																	\

	#define BEGIN_HANDLED_PREFIXED_MESSAGE(									\
			NAME, PREFIXTYPE, HANDLERTYPE, HANDLERARG )						\
		MERCURY_HANDLED_PREFIXED_MESSAGE(									\
			NAME, PREFIXTYPE, HANDLERTYPE, HANDLERARG )						\
		{																	\
			MERCURY_PREFIXED_GOODIES( NAME, PREFIXTYPE )					\

	#define END_HANDLED_PREFIXED_MESSAGE()									\
		};																	\

	#define BEGIN_PREFIXED_MESSAGE( NAME, PREFIXTYPE, HANDLER )				\
		MERCURY_PREFIXED_MESSAGE( NAME, PREFIXTYPE, HANDLER )				\
		{																	\
			MERCURY_PREFIXED_GOODIES( NAME, PREFIXTYPE )					\

	#define END_PREFIXED_MESSAGE()											\
		};																	\


	// Handler stuff

	#define MERCURY_HANDLED_EMPTY_MESSAGE(									\
				NAME, HANDLERTYPE, HANDLERARG )								\
		HANDLER_STATEMENT( NAME, HANDLERTYPE, HANDLERARG )					\
		MERCURY_EMPTY_MESSAGE( NAME, HANDLER_ARGUMENT( NAME ) )				\

	#define MERCURY_HANDLED_PREFIXED_EMPTY_MESSAGE(							\
				NAME, PREFIXTYPE, HANDLERTYPE, HANDLERARG )					\
		HANDLER_STATEMENT( NAME, HANDLERTYPE, HANDLERARG )					\
		MERCURY_PREFIXED_EMPTY_MESSAGE(										\
			NAME, PREFIXTYPE, HANDLER_ARGUMENT( NAME ) )					\

	#define MERCURY_PREFIXED_EMPTY_MESSAGE(									\
				NAME, PREFIXTYPE, HANDLER )									\
		MERCURY_MESSAGE( NAME, FIXED_LENGTH_MESSAGE,						\
			sizeof(PREFIXTYPE), HANDLER )									\

	#define MERCURY_HANDLED_STRUCT_MESSAGE(									\
				NAME, HANDLERTYPE, HANDLERARG )								\
		HANDLER_STATEMENT( NAME, HANDLERTYPE, HANDLERARG )					\
		MERCURY_STRUCT_MESSAGE( NAME, HANDLER_ARGUMENT( NAME ) )			\

	#define MERCURY_HANDLED_PREFIXED_MESSAGE(								\
			NAME, PREFIXTYPE, HANDLERTYPE, HANDLERARG )						\
		HANDLER_STATEMENT( NAME, HANDLERTYPE, HANDLERARG )					\
		MERCURY_PREFIXED_MESSAGE( NAME, PREFIXTYPE,							\
				HANDLER_ARGUMENT( NAME ) )									\

	#define MERCURY_HANDLED_VARIABLE_MESSAGE(								\
		NAME, PARAM, HANDLERTYPE, HANDLERARG )								\
		HANDLER_STATEMENT( NAME, HANDLERTYPE, HANDLERARG )					\
		MERCURY_VARIABLE_MESSAGE( NAME, PARAM, HANDLER_ARGUMENT( NAME ) )	\

	#define MERCURY_HANDLED_CALLBACK_MESSAGE(								\
			NAME, HANDLERTYPE, HANDLERARG )									\
		HANDLER_STATEMENT( NAME, HANDLERTYPE, HANDLERARG )					\
		MERCURY_CALLBACK_MESSAGE( NAME, HANDLER_ARGUMENT( NAME ) )			\


	// Struct goodies

	#define MERCURY_STRUCT_GOODIES( NAME )									\
		static NAME##Args & start( Mercury::Bundle & b,						\
			Mercury::ReliableType reliable = Mercury::RELIABLE_DRIVER )		\
		{																	\
			return *(NAME##Args*)b.startStructMessage( NAME, reliable );	\
		}																	\
		static NAME##Args & startRequest( Mercury::Bundle & b,				\
			Mercury::ReplyMessageHandler * handler,							\
			void * arg = NULL,												\
			int timeout = Mercury::DEFAULT_REQUEST_TIMEOUT,					\
			Mercury::ReliableType reliable = Mercury::RELIABLE_DRIVER )		\
		{																	\
			return *(NAME##Args*)b.startStructRequest( NAME,				\
				handler, arg, timeout, reliable );							\
		}																	\
		static const Mercury::InterfaceElement & interfaceElement()			\
		{																	\
			return NAME;													\
		}																	\

	#define MERCURY_PREFIXED_GOODIES( NAME, PREFIXTYPE )					\
		static NAME##Args & start( Mercury::Bundle & b,						\
			const PREFIXTYPE & prefix,										\
			Mercury::ReliableType reliable = Mercury::RELIABLE_DRIVER )		\
		{																	\
			b.startMessage( NAME, reliable );								\
			b << prefix;													\
			return *(NAME##Args*)b.reserve( sizeof(NAME##Args) );			\
		}																	\
		static NAME##Args & startRequest( Mercury::Bundle & b,				\
			const PREFIXTYPE & prefix,										\
			Mercury::ReplyMessageHandler * handler,							\
			void * arg = NULL,												\
			int timeout = Mercury::DEFAULT_REQUEST_TIMEOUT,					\
			Mercury::ReliableType reliable = Mercury::RELIABLE_DRIVER )		\
		{																	\
			b.startRequest( NAME, handler, arg, timeout, reliable );		\
			b << prefix;													\
			return *(NAME##Args*)b.reserve( sizeof(NAME##Args) );			\
		}																	\


#endif // INTERFACE_MACROS_HPP


// Cleanup
#ifdef BEGIN_MERCURY_INTERFACE
	#undef BEGIN_MERCURY_INTERFACE
	#undef MERCURY_MESSAGE
	#undef END_MERCURY_INTERFACE

	#undef MERCURY_STRUCT_MESSAGE
	#undef MERCURY_PREFIXED_MESSAGE

	#undef MERCURY_METHOD_RANGE_MSG
	#undef MERCURY_PROPERTY_RANGE_MSG

	#undef MERCURY_OSTREAM
	#undef MERCURY_ISTREAM

	#undef HANDLER_STATEMENT
	#undef HANDLER_ARGUMENT
	#undef NULL_IF_NOT_SERVER
#endif


#ifdef DEFINE_SERVER_HERE
	#undef DEFINE_SERVER_HERE

	#ifndef DEFINE_INTERFACE_HERE
		#define DEFINE_INTERFACE_HERE
	#endif

	#define HANDLER_STATEMENT(NAME,TYPE,ARG)								\
		TYPE gHandler_##NAME(ARG);
	#define HANDLER_ARGUMENT(NAME) &gHandler_##NAME
	#define NULL_IF_NOT_SERVER(ARG) ARG
#else
	#define HANDLER_STATEMENT(NAME,TYPE,ARG)
	#define HANDLER_ARGUMENT(NAME) NULL
	#define NULL_IF_NOT_SERVER(ARG) NULL
#endif



// Core macros
#ifdef DEFINE_INTERFACE_HERE
	#undef DEFINE_INTERFACE_HERE

	#define BEGIN_MERCURY_INTERFACE( INAME ) 								\
		namespace INAME { 													\
			Mercury::InterfaceMinder gMinder( #INAME );						\
																			\
			void registerWithInterface(										\
					Mercury::NetworkInterface & networkInterface )			\
			{																\
				gMinder.registerWithInterface( networkInterface );			\
			}																\
																			\
			Mercury::Reason registerWithMachined(							\
					Mercury::NetworkInterface & networkInterface, int id )	\
			{																\
				return gMinder.registerWithMachined(						\
						networkInterface.address(), id );					\
			}																\
																			\
			Mercury::Reason registerWithMachinedAs( const char * name,		\
					Mercury::NetworkInterface & networkInterface, int id )	\
			{																\
				return gMinder.registerWithMachinedAs( name,				\
						networkInterface.address(), id );					\
			}																\

	#define END_MERCURY_INTERFACE()											\
		}

	#define MERCURY_MESSAGE( NAME, STYLE, PARAM, HANDLER )					\
			const Mercury::InterfaceElement & NAME =						\
				gMinder.add( #NAME, Mercury::STYLE, PARAM,					\
						NULL_IF_NOT_SERVER( HANDLER ) );

	#define MERCURY_STRUCT_MESSAGE( NAME, HANDLER )							\
		MERCURY_MESSAGE( NAME,												\
			FIXED_LENGTH_MESSAGE,											\
			sizeof(struct NAME##Args),										\
			HANDLER )														\
		Mercury::Bundle & operator<<( Mercury::Bundle & b,					\
			const struct NAME##Args &s )									\
		{																	\
			b.startMessage( NAME );											\
			(*(BinaryOStream*)( &b )) << s;									\
			return b;														\
		}																	\
		struct __Garbage__##NAME##Args



	#define MERCURY_PREFIXED_MESSAGE( NAME, PREFIXTYPE, HANDLER )			\
		MERCURY_MESSAGE(NAME,												\
			FIXED_LENGTH_MESSAGE,											\
			sizeof(PREFIXTYPE)+sizeof(struct NAME##Args),					\
			HANDLER )														\
		Mercury::Bundle & operator<<(										\
			Mercury::Bundle & b,											\
			const std::pair< PREFIXTYPE, struct NAME##Args > & p )			\
		{																	\
			b.startMessage( NAME );											\
			b << p.first;													\
			b << p.second;													\
			return b;														\
		}																	\
		struct __Garbage__##NAME##Args

	#define MERCURY_METHOD_RANGE_MSG( NAME, RANGE_PORTION )					\
		namespace Range														\
		{																	\
			ExposedMethodMessageRange NAME ## Range( NAME.id(),				\
				gMinder.addRange( NAME, RANGE_PORTION ) );					\
		}																	\

	#define MERCURY_PROPERTY_RANGE_MSG( NAME, RANGE_PORTION )				\
		namespace Range														\
		{																	\
			ExposedPropertyMessageRange NAME ## Range( NAME.id(),			\
				gMinder.addRange( NAME, RANGE_PORTION ) );					\
		}																	\

/**
 *  These are the streaming operators that must follow any Mercury interface
 *  definition that uses a struct to describe the message format.  Since BW now
 *  supports both big and little endian platforms, we can no longer simply
 *  assign structures directly onto network streams without regard for their
 *  internal structure.  Yes I know that this bloats the formerly quite elegant
 *  interface definitions, but I don't think there's an easier way around it.
 *
 *  Note that these macros assume the object being streamed is called 'x', and so
 *  an example of usage might be:
 *
 *	MF_BEGIN_CLIENT_MSG( controlEntity )
 *  	EntityID		id;
 *		bool			on;
 *  END_STRUCT_MESSAGE()
 *  MERCURY_ISTREAM( controlEntity, x.id >> x.on )
 *  MERCURY_OSTREAM( controlEntity, x.id << x.on )
 */
#define MERCURY_ISTREAM( NAME, XSTREAM )	 								\
BinaryIStream& operator>>( BinaryIStream &is, NAME##Args &x )				\
{																			\
	return is >> XSTREAM;													\
}

#define MERCURY_OSTREAM( NAME, XSTREAM ) 									\
BinaryOStream& operator<<( BinaryOStream &os, const NAME##Args &x )			\
{																			\
	return os << XSTREAM;													\
}

#else // ! DEFINE_INTERFACE_HERE

	#define BEGIN_MERCURY_INTERFACE( INAME )								\
		/** @internal														\
		 * @{																\
		 */																	\
		namespace INAME { 													\
			extern Mercury::InterfaceMinder gMinder;						\
			void registerWithInterface(										\
					Mercury::NetworkInterface & networkInterface );			\
			Mercury::Reason registerWithMachined(							\
					Mercury::NetworkInterface & networkInterface, int id );	\
			Mercury::Reason registerWithMachinedAs(	const char * name,		\
					Mercury::NetworkInterface & networkInterface, int id );	\

	#define MERCURY_MESSAGE( NAME, STYLE, PARAM, HANDLER )					\
			/** @internal */												\
			extern const Mercury::InterfaceElement & NAME;

	#define END_MERCURY_INTERFACE()											\
		/** @} */															\
		}


	#define MERCURY_STRUCT_MESSAGE( NAME, HANDLER )							\
		struct NAME##Args;													\
		MERCURY_MESSAGE( NAME, 0, 0, 0 )									\
		/** @internal */													\
		Mercury::Bundle & operator<<(										\
			Mercury::Bundle & b,											\
			const struct NAME##Args &s );									\
		/** @internal */													\
		struct NAME##Args


	#define MERCURY_PREFIXED_MESSAGE( NAME, PREFIXTYPE, HANDLER )			\
		struct NAME##Args;													\
		MERCURY_MESSAGE( NAME, 0, 0, 0 )									\
		/** @internal */													\
		Mercury::Bundle & operator<<(										\
			Mercury::Bundle & b,											\
			const std::pair< PREFIXTYPE, struct NAME##Args > & p );			\
		/** @internal */													\
		struct NAME##Args

	#define MERCURY_METHOD_RANGE_MSG( NAME, RANGE_PORTION )					\
		namespace Range														\
		{																	\
			extern ExposedMethodMessageRange NAME ## Range;					\
		}																	\

	#define MERCURY_PROPERTY_RANGE_MSG( NAME, RANGE_PORTION )				\
		namespace Range														\
		{																	\
			extern ExposedPropertyMessageRange NAME ## Range;				\
		}																	\

#define MERCURY_ISTREAM( NAME, XSTREAM ) 									\
BinaryIStream& operator>>( BinaryIStream &in, NAME##Args &x );				\

#define MERCURY_OSTREAM( NAME, XSTREAM ) 									\
BinaryOStream& operator<<( BinaryOStream &out, const NAME##Args &x );		\

#endif // DEFINE_INTERFACE_HERE

