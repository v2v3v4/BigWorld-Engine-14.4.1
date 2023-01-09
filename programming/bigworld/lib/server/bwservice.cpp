#include "cstdmf/debug_filter.hpp"
#include "cstdmf/watcher.hpp"

BW_BEGIN_NAMESPACE

void bwParseCommandLine( int argc, char **argv )
{
	MF_WATCH( "config/shouldWriteToConsole",
		DebugFilter::shouldWriteToConsole );

	for (int i=1; i < argc; i++)
	{
		if (strcmp( argv[i], "-machined" ) == 0)
		{
			DebugFilter::shouldWriteToConsole( false );
		}
	}
}

BW_END_NAMESPACE

// bwservice.cpp
