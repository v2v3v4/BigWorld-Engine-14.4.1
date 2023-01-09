#include "pch.hpp"

#include "cstdmf/debug.hpp"
#include "cstdmf/memory_stream.hpp"

#include "network/bsd_snprintf.h"
#include "network/endpoint.hpp"
#include "network/logger_message_forwarder.hpp"
#include "network/event_dispatcher.hpp"

BW_BEGIN_NAMESPACE

namespace // (anonymous)
{

void parseForArgs( ForwardingStringHandler & forwardingStartHandler,
		MemoryOStream & os, ... )
{
	va_list args;
	va_start( args, os );
	forwardingStartHandler.parseArgs( args, os );
	va_end( args );
}


} // end namespace (anonymous)

TEST( bsd_snprintf )
{
	static const char GOOD_FORMAT[] = "%s %d %u %ld %lu %lld %llu %hd %hu %hhd "
			"%hhu %c %f %f\n";
	static const char BAD_FORMAT[] = "%s %v\n";

	// Test the good format
	ForwardingStringHandler forwardingStringHandler( GOOD_FORMAT );
	CHECK( forwardingStringHandler.isOk() );

	// These should all work
	const char * string = "\"A string\"";
	int d = -42;
	uint u = 42;
	long ld = -84L;
	unsigned long lu = 84UL;
	long long lld = -84LL;
	unsigned long long llu = 84LLU;
	short hd = -42;
	unsigned short hu = 42;
	char hhd = -0x42;
	unsigned char hhu = 0x42;
	char c = 'A';
	float f = 42.f;
	double df = 42.;

	MemoryOStream os;
	parseForArgs( forwardingStringHandler, os,
		string, d, u, ld, lu, lld, llu, hd, hu, hhd, hhu, c, f, df );

	// Test the bad format
	ForwardingStringHandler badForwardingStringHandler( BAD_FORMAT );
	CHECK( !badForwardingStringHandler.isOk() );
}


BW_END_NAMESPACE

// test_snprintf.cpp

