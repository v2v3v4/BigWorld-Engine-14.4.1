// TODO: helper class / function to fork and exec a script or program that is
//       waited upon.

#include "transfer_db.hpp"

#include "cstdmf/allocator.hpp"
#include "cstdmf/debug.hpp"
#include "cstdmf/debug_filter.hpp"

#include "resmgr/bwresource.hpp"

#include "network/basictypes.hpp"

#include "server/bwconfig.hpp"
#include "server/bwservice.hpp"

DECLARE_DEBUG_COMPONENT2( "SecondaryDB", 0 )

BW_USE_NAMESPACE

int
main( int argc, char **argv )
{
	BW_SYSTEMSTAGE_MAIN();
	bool isVerbose = true;

	// Args parsing
	//  consolidate        <sqlite_file>  <receiving IP>:<receiving port>
	//  snapshotprimary                   <dest IP> <dest path> <limit kbps>
	//  snapshotsecondary  <sqlite_file>  <dest IP> <dest path> <limit kbps>
	//

	DebugFilter::shouldWriteToConsole( isVerbose );

	BWResource bwresource;
	BWResource::init( argc, const_cast< const char ** >( argv ) );
	BWConfig::init( argc, argv );

	TransferDB transferDB;
	if (!transferDB.init( isVerbose ))
	{
		ERROR_MSG( "Initialisation failed\n" );
		return EXIT_FAILURE;
	}

	START_MSG( "TransferDB" );

	for (int i = 1; i < argc; ++i)
	{
		if (((strcmp( argv[i], "--res" ) == 0) ||
			 (strcmp( argv[i], "-r" ) == 0)) && i < (argc - 1))
		{
			++i;
		}
		else if (strcmp( argv[i], "consolidate" ) == 0)
		{
			// We expect 2 more args for consolidate db
			if ((argc - (i+1)) < 2)
			{
				ERROR_MSG( "Incorrect number of arguments provided for "
					"'consolidate' command.\n" );
				return EXIT_FAILURE;
			}

			const char *secondaryDB = argv[ ++i ];
			const char *sendToAddr  = argv[ ++i ];

			if (!transferDB.consolidate( secondaryDB, sendToAddr ))
			{
				return EXIT_FAILURE;
			}

			break;
		}
		else if (strcmp( argv[i], "snapshotprimary" ) == 0)
		{
			// Arg list:
			//  snapshotprimary		<dest IP> <dest path> <limit kbps>

			// We expect 3 more args for primary DB snapshotting
			if ((argc - (i+1)) < 3)
			{
				ERROR_MSG( "Incorrect number of arguments provided for "
					"'snapshotprimary' command.\n" );
				return EXIT_FAILURE;
			}

			const char *destIP    = argv[ ++i ];
			const char *destPath  = argv[ ++i ];
			const char *limitKbps = argv[ ++i ];

			if (!transferDB.snapshotPrimary( destIP, destPath, limitKbps ))
			{
				return EXIT_FAILURE;
			}
			break;
		}
		else if (strcmp( argv[i], "snapshotsecondary" ) == 0)
		{
			// Arg list:
			// snapshotsecondary <sqlitefile> <destIP> <destpath> <limit kbps>

			// We expect 4 more args for secondary DB snapshotting
			if ((argc - (i+1)) < 4)
			{
				ERROR_MSG( "Incorrect number of arguments provided for "
					"'snapshotsecondary' command.\n" );
				return EXIT_FAILURE;
			}

			const char *secondaryDB = argv[ ++i ];
			const char *destIP      = argv[ ++i ];
			const char *destPath    = argv[ ++i ];
			const char *limitKbps   = argv[ ++i ];

			if (!transferDB.snapshotSecondary( secondaryDB, destIP,
					destPath, limitKbps ))
			{
				return EXIT_FAILURE;
			}
			break;
		}
	}

	return EXIT_SUCCESS;
}

// main.cpp
