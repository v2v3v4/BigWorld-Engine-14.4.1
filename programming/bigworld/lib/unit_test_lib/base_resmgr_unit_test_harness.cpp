#include "pch.hpp"

#include "base_resmgr_unit_test_harness.hpp"

#include "unit_test_lib/unit_test.hpp"

#include "resmgr/bwresource.hpp"

#include "cstdmf/allocator.hpp"
#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_util.hpp"
#include "cstdmf/log_msg.hpp"
#include "cstdmf/string_builder.hpp"

#ifdef __linux__
#include <libgen.h>
#endif


BW_BEGIN_NAMESPACE

//static 
int BaseResMgrUnitTestHarness::s_cmdArgC = 0;

//static
char ** BaseResMgrUnitTestHarness::s_cmdArgV = NULL;

// ----------------------------------------------------------------------------
BaseResMgrUnitTestHarness::BaseResMgrUnitTestHarness( const char* testName ):
		isOK_( false )
{
	MF_ASSERT( s_cmdArgC > 0 );
	MF_ASSERT( s_cmdArgV != NULL );

	BW::Allocator::setCrashOnLeak( true );

	new BWResource();

	// If res arguments are not provided then create our own res paths.
	bool resPathSpecified = false;
	for (int i = 1; i < s_cmdArgC; ++i)
	{
		if (strncmp( s_cmdArgV[i], "--res", 5 ) == 0)
		{
			resPathSpecified = true;
			break;
		}
	}
	if (!resPathSpecified)
	{
		const size_t MAX_NUM_PATHS_USED = 6;
		char myResPath[ (BW_MAX_PATH + 1) * MAX_NUM_PATHS_USED ];

		BW::StringBuilder sb_myResPath( myResPath, 
				ARRAY_SIZE( myResPath ) );

		// Res path must be last argument if not given with --res
		bool hasRootPathSpecified = false;
		for (int i = 1; i < s_cmdArgC; ++i)
		{
			if ((strncmp( s_cmdArgV[i], "--root", 6 ) == 0) &&
				((i + 1) < s_cmdArgC))
			{
				hasRootPathSpecified = true;
				sb_myResPath.append( s_cmdArgV[i + 1] );
				sb_myResPath.append( "/programming/bigworld/lib/" +
					BW::string(testName) + "/unit_test/res" );
				sb_myResPath.append( BW_RES_PATH_SEPARATOR );
				sb_myResPath.append( s_cmdArgV[i + 1] );
				sb_myResPath.append( "/game/res/bigworld" );
				break;
			}
		}

		if (!hasRootPathSpecified) 
		{
			sb_myResPath.append( BWUtil::executableDirectory().c_str() );
#ifndef _WIN32
			sb_myResPath.append( "/../" );
#endif
			sb_myResPath.append( "../../../../programming/bigworld/lib/" + 
				BW::string(testName) + "/unit_test/res/" );
		}

		sb_myResPath.append( BW_RES_PATH_SEPARATOR );
		sb_myResPath.append( BWUtil::executableDirectory().c_str() );
#ifndef _WIN32
		sb_myResPath.append( "/../" );
#endif
		sb_myResPath.append( "../../../../game/res/bigworld");

		const char *myargv[] =
		{
			"unit_test",
			"--res",
			myResPath
		};
		int myargc = ARRAY_SIZE( myargv );
		myargv[0] = s_cmdArgV[0]; // patch in the real application


		if (!BWResource::init( myargc, myargv ))
		{
			BWUnitTest::unitTestError( "could not initialise BWResource\n" );
			return;
		}
	}
	// If arguments are provided then use the supplied res paths.
	else
	{
		if (!BWResource::init( s_cmdArgC, (const char**)s_cmdArgV ))
		{
			BWUnitTest::unitTestError( "could not initialise BWResource\n" );
			return;
		}
	}

	isOK_ = true;
}

// ----------------------------------------------------------------------------
BaseResMgrUnitTestHarness::~BaseResMgrUnitTestHarness() 
{
	delete BWResource::pInstance();
	BWResource::fini();

	LogMsg::fini();
}

BW_END_NAMESPACE
