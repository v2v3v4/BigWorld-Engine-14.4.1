#include "server_platform.hpp"


BW_BEGIN_NAMESPACE


/**
 *	Constructor.
 */
ServerPlatform::ServerPlatform() :
	isInitialised_( false )
{
	this->initVersionsProcesses();
}


/**
 *	This method returns whether the object has been successfully initialised.
 *
 * 	@returns true if the object is initialised properly, false otherwise.
 */
bool ServerPlatform::isInitialised() const
{
	return isInitialised_;
}


/**
 *	This method initialises the process name sets for each BigWorld version
 *	where it changed.
 */
void ServerPlatform::initVersionsProcesses()
{
	// Note: Using 0.0 for the initial version because this can be parsed by
	// pycommon.process.Version to mean the initial version.
	ProcessBinaryVersion initial( 0, 0 );

	ProcessSet & initialSet = versionsProcesses_[ initial ];
	initialSet.insert( "baseappmgr" );
	initialSet.insert( "cellappmgr" );
	initialSet.insert( "dbmgr" );
	initialSet.insert( "baseapp" );
	initialSet.insert( "cellapp" );
	initialSet.insert( "loginapp" );

	// 2.1: Added ServiceApp.
	ProcessBinaryVersion bw_2_1( 2, 1 ); // Added ServiceApp
	ProcessSet & bw_2_1_set = versionsProcesses_[ bw_2_1 ] = initialSet;
	bw_2_1_set.insert( "serviceapp" );

	// 2.10: Scalable DB: DBMgr -> DBApp and DBAppMgr
	ProcessBinaryVersion bw_2_10( 2, 10 ); 
	ProcessSet & bw_2_10_set = versionsProcesses_[ bw_2_10 ] = bw_2_1_set;
	bw_2_10_set.erase( "dbmgr" );
	bw_2_10_set.insert( "dbappmgr" );
	bw_2_10_set.insert( "dbapp" );
}


/**
 *	This method attempts to match the binaries under BW_ROOT to the first
 *	BigWorld version that had same set of processes, that is, the earliest
 *	version the binaries _could_ be judging only by what binaries are present.
 *
 *  @param uid      The user ID we are searching under (used for log output).
 *	@param bwRoot 	The BW_ROOT path to check under.
 *	@param version 	This is filled with the found version. Possibly "0.0" for
 *					the "initial" version.
 *	@return 		true if a version's processes were found, false otherwise.
 */
bool ServerPlatform::determineVersion( MachineGuardMessage::UserId uid,
        const BW::string & bwRoot,
		BW::string & version )
{
	// Find the latest version that matches, and return that.
	for (VersionsProcesses::const_reverse_iterator iter = 
				versionsProcesses_.rbegin();
			iter != versionsProcesses_.rend();
			++iter)
	{
		if (this->checkBinariesExist( uid, bwRoot, iter->second ))
		{
			version = iter->first.str();
			return true;
		}
	}

	return false;
}


BW_END_NAMESPACE

// server_platform.cpp
