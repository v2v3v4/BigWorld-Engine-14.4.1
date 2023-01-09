#include "pch.hpp"

#include "cstdmf/debug.hpp"
#include "cstdmf/debug_filter.hpp"
#include "cstdmf/memory_stream.hpp"

BW_BEGIN_NAMESPACE

#if ENABLE_MSG_LOGGING

#define ENABLE_CRITICAL_MESSAGES 0

const bool shouldWriteToConsole = false;

TEST( OldMessageMacroFormat )
{
	DebugFilter::shouldWriteToConsole( shouldWriteToConsole );

#if 0

	HACK_MSG[ "DBMgr" ].
		meta( "key1", 1 ).
		meta( "key2", "value" ).
		write( "This was a test\n" );


	DebugFilter::instance().hasDevelopmentAssertions( true );
	printf( "==========\n\n\n\n=============\n" );
	MF_ASSERT_DEV( false );

	// TODO: remove this after testing
	DebugFilter::instance().hasDevelopmentAssertions( false );
	printf( "==========\n\n\n\n=============\n" );
	MF_ASSERT_DEV( false );
	DebugFilter::instance().hasDevelopmentAssertions( true );
	printf( "==========\n\n\n\n=============\n" );
	MF_ASSERT_DEV( false );
#endif


	// Old messages work

#define MSG_MACRO_WRAPPER( SEVERITY )				\
	SEVERITY( "Simple Format String\n" );			\
	SEVERITY( "More format: %s\n", "string" );		\
	SEVERITY( "%d %f %hd\n", 12, 32.f, 5 );			\
	SEVERITY( "blah %f %s\n", 32.f, "a string" );	\

	MSG_MACRO_WRAPPER( TRACE_MSG );
	MSG_MACRO_WRAPPER( DEBUG_MSG );
	MSG_MACRO_WRAPPER( INFO_MSG );
	MSG_MACRO_WRAPPER( NOTICE_MSG );
	MSG_MACRO_WRAPPER( WARNING_MSG );
	MSG_MACRO_WRAPPER( ERROR_MSG );
#if ENABLE_CRITICAL_MESSAGES
	MSG_MACRO_WRAPPER( CRITICAL_MSG );
#endif // ENABLE_CRITICAL_MESSAGES
	MSG_MACRO_WRAPPER( HACK_MSG );
	MSG_MACRO_WRAPPER( ASSET_MSG );

#undef MSG_MACRO_WRAPPER

	DebugFilter::shouldWriteToConsole( false );
}


TEST( NewMessageMacrosWithCategory )
{
	DebugFilter::shouldWriteToConsole( shouldWriteToConsole );

	// New support with categories

#define MSG_MACRO_WRAPPER( SEVERITY )			\
	SEVERITY[ "UnitTestDebugWithCategory" ]( "Simple Format String\n" );	\
	SEVERITY[ "UnitTestDebugWithCategory" ]( "More format: %s\n", "string" ); \
	SEVERITY[ "UnitTestDebugWithCategory" ]( "%d %f %hd\n", 12, 32.f, 5 );

	MSG_MACRO_WRAPPER( TRACE_MSG );
	MSG_MACRO_WRAPPER( DEBUG_MSG );
	MSG_MACRO_WRAPPER( INFO_MSG );
	MSG_MACRO_WRAPPER( NOTICE_MSG );
	MSG_MACRO_WRAPPER( WARNING_MSG );
	MSG_MACRO_WRAPPER( ERROR_MSG );
#if ENABLE_CRITICAL_MESSAGES
	MSG_MACRO_WRAPPER( CRITICAL_MSG );
#endif // ENABLE_CRITICAL_MESSAGES
	MSG_MACRO_WRAPPER( HACK_MSG );
	MSG_MACRO_WRAPPER( ASSET_MSG );


#undef MSG_MACRO_WRAPPER
	DebugFilter::shouldWriteToConsole( false );
}


TEST( NewMessageMacrosWithCategoryUsingMetaData )
{

	DebugFilter::shouldWriteToConsole( shouldWriteToConsole );

#define MSG_MACRO_WRAPPER( SEVERITY )			\
	SEVERITY[ "UnitTestDebugWithMetaData" ].	\
		meta( "int", intValue ).				\
		meta( "long", longValue ).				\
		meta( "float", floatValue ).			\
		meta( "double", doubleValue ).			\
		meta( "str", strValue ).				\
		meta( "unsigned", unsignedValue ).		\
		write( "Simple format string\n" )

	int intValue = -21;
	long long int longValue = 123678213;
	double doubleValue = -12.372;
	float floatValue = -48.24f;
	const char * strValue = "my first string";
	unsigned int unsignedValue = 23;


	MSG_MACRO_WRAPPER( TRACE_MSG );
	MSG_MACRO_WRAPPER( DEBUG_MSG );
	MSG_MACRO_WRAPPER( INFO_MSG );
	MSG_MACRO_WRAPPER( NOTICE_MSG );
	MSG_MACRO_WRAPPER( WARNING_MSG );
	MSG_MACRO_WRAPPER( ERROR_MSG );
#if ENABLE_CRITICAL_MESSAGES
//	MSG_MACRO_WRAPPER( CRITICAL_MSG );
#endif // ENABLE_CRITICAL_MESSAGES
	MSG_MACRO_WRAPPER( HACK_MSG );
	MSG_MACRO_WRAPPER( ASSET_MSG );

#undef MSG_MACRO_WRAPPER

	DebugFilter::shouldWriteToConsole( false );

}


TEST( NewMessageMacrosWithSource )
{
	DebugFilter::shouldWriteToConsole( shouldWriteToConsole );

#define MSG_MACRO_WRAPPER( SEVERITY )			\
	SEVERITY[ "UnitTestDebugWithSource" ].		\
		source( MESSAGE_SOURCE_CPP ).			\
		write( "Simple format string\n" );		\
	SEVERITY[ "UnitTestDebugWithSource" ].		\
		source( MESSAGE_SOURCE_SCRIPT ).		\
		write( "Simple format string\n" );		\

	MSG_MACRO_WRAPPER( TRACE_MSG );
	MSG_MACRO_WRAPPER( DEBUG_MSG );
	MSG_MACRO_WRAPPER( INFO_MSG );
	MSG_MACRO_WRAPPER( NOTICE_MSG );
	MSG_MACRO_WRAPPER( WARNING_MSG );
	MSG_MACRO_WRAPPER( ERROR_MSG );
#if ENABLE_CRITICAL_MESSAGES
	MSG_MACRO_WRAPPER( CRITICAL_MSG );
#endif // ENABLE_CRITICAL_MESSAGES
	MSG_MACRO_WRAPPER( HACK_MSG );
	MSG_MACRO_WRAPPER( ASSET_MSG );

#undef MSG_MACRO_WRAPPER

	DebugFilter::shouldWriteToConsole( false );

}


TEST( CritWithCategory )
{

	DebugFilter::shouldWriteToConsole( shouldWriteToConsole );

#if ENABLE_CRITICAL_MESSAGES
	CRITICAL_MSG_WITH_CATEGORY( "MyCategoryEh" )( "This is my message\n" );
#endif // ENABLE_CRITICAL_MESSAGES

	DebugFilter::shouldWriteToConsole( false );

}


TEST( ImplicitBacktraceMacros )
{

	DebugFilter::shouldWriteToConsole( shouldWriteToConsole );

// TODO: these should
// 1) be removed
// 2) be made to work with the re-write of the messageBackTrace

	TRACE_BACKTRACE();
	DEBUG_BACKTRACE();
	INFO_BACKTRACE();
	NOTICE_BACKTRACE();
	WARNING_BACKTRACE();
	ERROR_BACKTRACE();
#if ENABLE_CRITICAL_MESSAGES
	CRITICAL_BACKTRACE();
#endif // ENABLE_CRITICAL_MESSAGES
	HACK_BACKTRACE();
// TODO: this isn't implemented, should it be?
//	ASSET_BACKTRACE();

#undef MSG_MACRO_WRAPPER

	DebugFilter::shouldWriteToConsole( false );


}

#endif // ENABLE_MSG_LOGGING

BW_END_NAMESPACE

// test_debug.cpp
