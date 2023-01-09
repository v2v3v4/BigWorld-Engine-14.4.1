#ifndef BW_DEBUG_HPP
#define BW_DEBUG_HPP

#include "cstdmf/bw_namespace.hpp"
#include "cstdmf/config.hpp"
#include "cstdmf/debug_message_priority.hpp"
#include "cstdmf/log_msg.hpp"

#include <assert.h>


/**
 *	This macro is a helper used by the *_MSG macros.
 */
namespace
{
	// This is the default s_componentPriority for files that does not
	// have DECLARE_DEBUG_COMPONENT2(). Useful for hpp and ipp files that
	// uses debug macros. s_componentPriority declared by
	//	DECLARE_DEBUG_COMPONENT2() will have precedence over this one.
	const BW_NAMESPACE DebugMessagePriority s_componentPriority = 
		BW_NAMESPACE MESSAGE_PRIORITY_TRACE;
	const char s_categoryName[] = "";
}


BW_BEGIN_NAMESPACE


/*
 *	This method handles wrapping a LogMsg object to enable the _MSG macros
 *	to work as though they are objects.
 */
CSTDMF_DLL BW_NAMESPACE LogMsg & addMessageMetadataForScope( BW_NAMESPACE LogMsg & msg );


/*
 * 	This function is an unfortunate work around for the fact that we can't pass
 * 	non-const references from the message macros.
 */
CSTDMF_DLL BW_NAMESPACE LogMsg & constWrapper_addMessageMetadataForScope( const BW_NAMESPACE LogMsg & msg );



/// This macro prints a debug message with CRITICAL priority.
/// CRITICAL_MSG is always enabled no matter what the build target is.
#define CRITICAL_MSG  constWrapper_addMessageMetadataForScope( BW_NAMESPACE CriticalMsg() )


#define CRITICAL_MSG_WITH_CATEGORY( CATEGORY )							\
	BW_NAMESPACE CriticalMsg( #CATEGORY ).write

/// This macro prints a debug message with ASSET priority.
#define ASSET_MSG  constWrapper_addMessageMetadataForScope( BW_NAMESPACE AssetMsg( ::s_categoryName ) )



#if ENABLE_MSG_LOGGING

//
// The following macros are used to display debug information. See comment at
// the top of this file.

#define TRACE_MSG	constWrapper_addMessageMetadataForScope( BW_NAMESPACE TraceMsg( ::s_categoryName ) )
#define DEBUG_MSG	constWrapper_addMessageMetadataForScope( BW_NAMESPACE DebugMsg( ::s_categoryName ) )
#define INFO_MSG	constWrapper_addMessageMetadataForScope( BW_NAMESPACE InfoMsg( ::s_categoryName ) )
#define NOTICE_MSG	constWrapper_addMessageMetadataForScope( BW_NAMESPACE NoticeMsg( ::s_categoryName ) )
#define WARNING_MSG	constWrapper_addMessageMetadataForScope( BW_NAMESPACE WarningMsg( ::s_categoryName ) )
#define ERROR_MSG	constWrapper_addMessageMetadataForScope( BW_NAMESPACE ErrorMsg( ::s_categoryName ) )
#define HACK_MSG	constWrapper_addMessageMetadataForScope( BW_NAMESPACE HackMsg( ::s_categoryName ) )



// TODO: MACRO REFACTOR
// only used in 3 places
// * lib/entitydef/unit_test/test_streamsizes.hpp (DEBUG_BACKTRACE)
// * lib/network/delayed_channels.cpp (ERROR_BACKTRACE)
// * server/tools/consolidate_dbs/db_consolidator_errors.hpp (HACK_BACKTRACE)
#define BACKTRACE_WITH_PRIORITY( PRIORITY )								\
	BW_NAMESPACE LogMsg( PRIORITY, ::s_categoryName ).writeBackTrace


#define TRACE_BACKTRACE		BACKTRACE_WITH_PRIORITY( BW_NAMESPACE MESSAGE_PRIORITY_TRACE )
#define DEBUG_BACKTRACE		BACKTRACE_WITH_PRIORITY( BW_NAMESPACE MESSAGE_PRIORITY_DEBUG )
#define INFO_BACKTRACE		BACKTRACE_WITH_PRIORITY( BW_NAMESPACE MESSAGE_PRIORITY_INFO )
#define NOTICE_BACKTRACE	BACKTRACE_WITH_PRIORITY( BW_NAMESPACE MESSAGE_PRIORITY_NOTICE )
#define WARNING_BACKTRACE	BACKTRACE_WITH_PRIORITY( BW_NAMESPACE MESSAGE_PRIORITY_WARNING )
#define ERROR_BACKTRACE		BACKTRACE_WITH_PRIORITY( BW_NAMESPACE MESSAGE_PRIORITY_ERROR )
#define CRITICAL_BACKTRACE	BACKTRACE_WITH_PRIORITY( BW_NAMESPACE MESSAGE_PRIORITY_CRITICAL )
#define HACK_BACKTRACE		BACKTRACE_WITH_PRIORITY( BW_NAMESPACE MESSAGE_PRIORITY_HACK )




#define DEBUG_MSG_WITH_PRIORITY_AND_CATEGORY( PRIORITY, CATEGORY )		\
	BW_NAMESPACE LogMsg( PRIORITY, #CATEGORY ).write

/// This macro prints a development time only message CRITICAL priority.
#define DEV_CRITICAL_MSG	CRITICAL_MSG




#else	// ENABLE_MSG_LOGGING



#define NULL_MSG(...)														\
	do																		\
	{																		\
		if (false)															\
			printf(__VA_ARGS__);											\
	}																		\
	while (false)

#define DEBUG_MSG_WITH_PRIORITY_AND_CATEGORY( PRIORITY, CATEGORY )	\
	debugMsgNULL

#define TRACE_MSG(...)			NULL_MSG(__VA_ARGS__)
#define DEBUG_MSG(...)			NULL_MSG(__VA_ARGS__)
#define INFO_MSG(...)			NULL_MSG(__VA_ARGS__)
#define NOTICE_MSG(...)			NULL_MSG(__VA_ARGS__)
#define WARNING_MSG(...)		NULL_MSG(__VA_ARGS__)
#define ERROR_MSG(...)			NULL_MSG(__VA_ARGS__)
#define HACK_MSG(...)			NULL_MSG(__VA_ARGS__)
#define SCRIPT_MSG(...)			NULL_MSG(__VA_ARGS__)
#define DEV_CRITICAL_MSG(...)	NULL_MSG(__VA_ARGS__)

#define TRACE_BACKTRACE()
#define DEBUG_BACKTRACE()
#define INFO_BACKTRACE()
#define NOTICE_BACKTRACE()
#define WARNING_BACKTRACE()
#define ERROR_BACKTRACE()
#define CRITICAL_BACKTRACE()
#define HACK_BACKTRACE()

#endif	// ENABLE_MSG_LOGGING




#define TRACE_MSG_WITH_CATEGORY( CATEGORY ) \
	DEBUG_MSG_WITH_PRIORITY_AND_CATEGORY( MESSAGE_PRIORITY_TRACE, CATEGORY )

#define DEBUG_MSG_WITH_CATEGORY( CATEGORY ) \
	DEBUG_MSG_WITH_PRIORITY_AND_CATEGORY( MESSAGE_PRIORITY_DEBUG, CATEGORY )

#define INFO_MSG_WITH_CATEGORY( CATEGORY ) \
	DEBUG_MSG_WITH_PRIORITY_AND_CATEGORY( MESSAGE_PRIORITY_INFO, CATEGORY )

#define NOTICE_MSG_WITH_CATEGORY( CATEGORY ) \
	DEBUG_MSG_WITH_PRIORITY_AND_CATEGORY( MESSAGE_PRIORITY_NOTICE, CATEGORY )

#define WARNING_MSG_WITH_CATEGORY( CATEGORY ) \
	DEBUG_MSG_WITH_PRIORITY_AND_CATEGORY( MESSAGE_PRIORITY_WARNING, CATEGORY )

#define ERROR_MSG_WITH_CATEGORY( CATEGORY ) \
	DEBUG_MSG_WITH_PRIORITY_AND_CATEGORY( MESSAGE_PRIORITY_ERROR, CATEGORY )

#define HACK_MSG_WITH_CATEGORY( CATEGORY ) \
	DEBUG_MSG_WITH_PRIORITY_AND_CATEGORY( MESSAGE_PRIORITY_HACK, CATEGORY )

#define ASSET_MSG_WITH_CATEGORY( CATEGORY ) \
	DEBUG_MSG_WITH_PRIORITY_AND_CATEGORY( MESSAGE_PRIORITY_ASSET, CATEGORY )



/**
 *	This macro used to display trace information. Can be used later to add in
 *	our own callstack if necessary.
 */
#define ENTER( className, methodName )									\
	TRACE_MSG( className "::" methodName "\n" )


/**
 *	This function eats the arguments of *_MSG macros when in Release mode
 */
inline void debugMsgNULL( const char * /*format*/, ... )
{
}



CSTDMF_DLL const char * bw_debugMunge( const char * path, const char * module = NULL );


/**
 *  This function determines if a float is a valid and finite
 */
union IntFloat
{
	float f;
	unsigned int ui32;
};

inline bool isFloatValid( float f )
{
	IntFloat intFloat;
	intFloat.f = f;
	return ( intFloat.ui32 & 0x7f800000 ) != 0x7f800000;
}


#if defined( __GNUC__ ) || defined( __clang__ )
# define BW_UNUSED_ATTRIBUTE __attribute__((__unused__))
#else
# define BW_UNUSED_ATTRIBUTE
#endif


#if !defined( _RELEASE ) && !defined( EMSCRIPTEN ) && 						\
(!defined( BW_BLOB_CONFIG ) || defined( BW_ASSET_PIPELINE ))
# if ENABLE_WATCHERS

extern int bwWatchDebugMessagePriority( DebugMessagePriority & value,
								const char * path );

#define DEFINE_WATCH_DEBUG_MESSAGE_PRIORITY_FUNC							\
int bwWatchDebugMessagePriority( DebugMessagePriority & value,				\
	const char * path )														\
{                                                                           \
	WatcherPtr pWatcher = new DataWatcher<DebugMessagePriority>(			\
		value, Watcher::WT_READ_WRITE );									\
	Watcher::rootWatcher().addChild( path, pWatcher );						\
	return 0;																\
}

/**
 *	This macro needs to be placed in a cpp file before any of the *_MSG macros
 *	can be used.
 *
 *	@param module	The name (or path) of the watcher module that the component
 *					priority should be displayed in.
 *	@param priority	The initial component priority of the messages in the file.
 */
#define DECLARE_DEBUG_COMPONENT2( module, priority )						\
	static BW_NAMESPACE DebugMessagePriority s_componentPriority 			\
			BW_UNUSED_ATTRIBUTE =											\
		(BW_NAMESPACE DebugMessagePriority)priority ;						\
	static int IGNORE_THIS_COMPONENT_WATCHER_INIT BW_UNUSED_ATTRIBUTE =		\
		 bwWatchDebugMessagePriority( ::s_componentPriority,				\
				 BW_NAMESPACE bw_debugMunge( __FILE__, module ) ); 			\
	static const char s_categoryName[] BW_UNUSED_ATTRIBUTE = module;

# else // !ENABLE_WATCHERS

#define DECLARE_DEBUG_COMPONENT2( module, priority )						\
	static BW_NAMESPACE DebugMessagePriority s_componentPriority =			\
			(BW_NAMESPACE DebugMessagePriority)priority BW_UNUSED_ATTRIBUTE;\
	static const char s_categoryName[] = "" BW_UNUSED_ATTRIBUTE;

# endif // ENABLE_WATCHERS

#else // !_RELEASE

#define DEFINE_WATCH_DEBUG_MESSAGE_PRIORITY_FUNC
#define DECLARE_DEBUG_COMPONENT2( module, priority )

#endif // !_RELEASE

/**
 *	This macro needs to be placed in a cpp file before any of the *_MSG macros
 *	can be used.
 *
 *	@param priority	The initial component priority of the messages in the file.
 */
#define DECLARE_DEBUG_COMPONENT( priority )									\
	DECLARE_DEBUG_COMPONENT2( "", priority )




#ifdef __ASSERT_FUNCTION
#	define MF_FUNCNAME __ASSERT_FUNCTION
#else
#		define MF_FUNCNAME ""
#endif

#ifdef MF_USE_ASSERTS
#	define MF_REAL_ASSERT assert(0);
#else
#	define MF_REAL_ASSERT
#endif

#define MF_ASSERT_IMPLEMENTATION( exp )									\
	if (!(exp))															\
	{																	\
		BW_NAMESPACE CriticalMsg().write(								\
			"ASSERTION FAILED: %s\n" __FILE__ "(%d)%s%s\n",				\
				#exp,													\
				(int)__LINE__,											\
				*MF_FUNCNAME ? " in " : "",								\
				MF_FUNCNAME );											\
																		\
		MF_REAL_ASSERT													\
	}

// The MF_ASSERT macro should used in place of the assert and ASSERT macros.
#if !defined( _RELEASE )
/**
 *	This macro should be used instead of assert.
 *
 *	@see MF_ASSERT_DEBUG
 */
#	define MF_ASSERT( exp ) MF_ASSERT_IMPLEMENTATION( exp )

//-----------------------------------------------------------------------------

// Assert the performs no memory allocation. For use in memory functions to
// avoid infinite recursion.
#ifdef WIN32
#define MF_DEBUGPRINT_NOALLOC( msg )										\
	{																		\
		const char* p = (msg);												\
		OutputDebugStringA( p );											\
		fputs( p, stderr );											\
	}
#else
#define MF_DEBUGPRINT_NOALLOC( msg ) fputs( (msg), stdout )
#endif

#	define MF_ASSERT_NOALLOC( exp )											\
		if (!(exp))															\
		{																	\
			char buffer[256];												\
			bw_snprintf( buffer, 256, "ASSERTION FAILED: %s\n%s : %d\n",	\
											#exp, __FILE__ , __LINE__ );	\
			MF_DEBUGPRINT_NOALLOC( buffer );								\
			ENTER_DEBUGGER();												\
		}

#else	// _RELEASE

#	define MF_ASSERT( exp )
#	define MF_DEBUGPRINT_NOALLOC( msg )
#	define MF_ASSERT_NOALLOC( exp )
#endif // !_RELEASE


// The MF_ASSERT_DEBUG is like MF_ASSERT except it is only evaluated
// in debug builds.
#ifdef _DEBUG
#	define MF_ASSERT_DEBUG		MF_ASSERT
#else
/**
 *	This macro should be used instead of assert. It is enabled only
 *	in debug builds, unlike MF_ASSERT which is enabled in both
 *	debug and hybrid builds.
 *
 *	@see MF_ASSERT
 */
#	define MF_ASSERT_DEBUG( exp )
#endif


#define BW_BUILD_SUPPORTS_DEV_ASSERT (defined( MF_SERVER ) || defined ( EDITOR_ENABLED ) || !defined( _RELEASE ))

#if BW_BUILD_SUPPORTS_DEV_ASSERT
/**
 *	An assertion which is only lethal when not in a production environment.
 *	These are disabled for client release builds.
 *
 *	@see MF_ASSERT
 */
#	define MF_ASSERT_DEV( exp )										\
			if (!(exp))												\
			{														\
				BW_NAMESPACE CriticalMsg().writeAndAssert(			\
					"MF_ASSERT_DEV FAILED: " #exp "\n"				\
					__FILE__ "(%d)%s%s\n",							\
					(int)__LINE__,									\
					*MF_FUNCNAME ? " in " : "", MF_FUNCNAME );		\
			}

#else // BW_BUILD_SUPPORTS_DEV_ASSERT

/**
 *	Empty versions of above function - not available on client release builds.
 */
#	define MF_ASSERT_DEV( exp )

#endif // BW_BUILD_SUPPORTS_DEV_ASSERT

#undef BW_BUILD_SUPPORTS_DEV_ASSERT

/**
 *	An assertion which is only lethal when not in a production environment.
 *	In a production environment, the block of code following the macro will
 *	be executed if the assertion fails.
 *
 *	@see MF_ASSERT_DEV
 */
#define IF_NOT_MF_ASSERT_DEV( exp )									\
		if ((!( exp )) && (											\
			BW_NAMESPACE CriticalMsg().writeAndAssert(				\
				"MF_ASSERT_DEV FAILED: " #exp "\n"					\
					__FILE__ "(%d)%s%s\n", (int)__LINE__,			\
					*MF_FUNCNAME ? " in " : "", MF_FUNCNAME ),		\
			true))		// leave trailing block after message


/**
 *	This macro is used to assert a pre-condition.
 *
 *	@see MF_ASSERT
 *	@see POST
 */
#define PRE( exp )	MF_ASSERT( exp )

/**
 *	This macro is used to assert a post-condition.
 *
 *	@see MF_ASSERT
 *	@see PRE
 */
#define POST( exp )	MF_ASSERT( exp )

/**
 *	This macro is used to verify an expression. In non-release it
 *	asserts on failure, and in release the expression is still
 *	evaluated.
 *
 *	@see MF_ASSERT
 */
#ifdef _RELEASE
#	define MF_VERIFY( exp ) (exp)
#	define MF_VERIFY_DEV( exp ) (exp)
#else
#	define MF_VERIFY MF_ASSERT
#	define MF_VERIFY_DEV MF_ASSERT_DEV
#endif

// this is a placeholder until a better solution can be implemented
#define MF_EXIT(msg) {														\
			BW_NAMESPACE CriticalMsg().write(								\
				"FATAL ERROR: " #msg "\n"							\
					__FILE__ "(%d)%s%s\n", (int)__LINE__,					\
					*MF_FUNCNAME ? " in " : "",								\
					MF_FUNCNAME );											\
																			\
			MF_REAL_ASSERT													\
}

#if defined( BW_CLIENT )
#define MF_ASSERT_CLIENT_ONLY( exp ) MF_ASSERT( exp )
#else
#define MF_ASSERT_CLIENT_ONLY( exp )
#endif


/**
 *	Transform asserts are for checking that:
 *	1. a model must have had it's animations updated before drawing
 *	2. a model is animated only once per frame
 */
#if ENABLE_TRANSFORM_VALIDATION
#define TRANSFORM_ASSERT( exp ) MF_ASSERT_IMPLEMENTATION( exp )
#else
#define TRANSFORM_ASSERT( exp )
#endif

/**
 *	This class is used to query if the current thread is the main thread.
 */
class MainThreadTracker
{
public:
	MainThreadTracker();

	CSTDMF_DLL static bool isCurrentThreadMain();
};

BW_END_NAMESPACE

#endif // BW_DEBUG_HPP
