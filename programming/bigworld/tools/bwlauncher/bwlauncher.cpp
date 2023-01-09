
// bwlauncher.cpp : Defines the class behaviors for the application.
//

#include "pch.hpp"
#include "launch_db.hpp"
#include "bwlauncher_config.hpp"
#include "utils.hpp"




const char  SELF_DELETE_BAT[] =
"@echo off\n"
":Repeat\n"
"@del \""BWLAUNCHER_EXE_NAME"\" > NUL\n"
"@if exist \""BWLAUNCHER_EXE_NAME"\" goto Repeat\n"
"@del \""BWLAUNCHER_UNINST_BAT"\" > NUL\n";


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

bool installSelf( bool allUsers );
bool uninstall( bool allUsers );
bool checkInstallation( bool allUsers );
bool processProtocolCommand( const StringVector & args );
bool registerGame( const tstring& gameID, const tstring& exePath );
tstring findInstalledLocation();



/**
 *	Main program entry point.
 */
int CALLBACK wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nShowCmd )
{
	StringVector args;
	splitCommandArgs( GetCommandLine(), args );

	if (gotArg(  _T( "/installCurrent" ), args))
	{
		DEBUG_MSG( _T("Attempting install for current user.\n") );
		installSelf( false );
		if (gotArg( _T("/runAfterInstall"), args))
		{
			runProgram( getLauncherInstalledExePath( false ) );
		}
	}
	else if (gotArg(  _T( "/install" ), args) || gotArg( _T("/installAll" ), args ))
	{
		try
		{
			DEBUG_MSG( _T("Attempting to install for all users.\n") );
			installSelf( true );
			if (gotArg( _T("/runAfterInstall"), args))
			{
				runProgram( getLauncherInstalledExePath( true ) );
			}
		}
		catch(const UnprivilegedException&)
		{
			if (!gotArg(_T("/restartedWithPrivileges"), args))
			{
				restartWithPrivileges( _T("/installAll") );
			}
			else
			{
				DEBUG_MSG( _T("Failed to install for all users, even after raising privileges.\n") );
				displayMessage( _T("Error installing BigWorld Launcher, could not gain required privileges."), MB_ICONERROR );
			}
		}
	}
	// UNCOMMENT FOR UNINSTALL SUPPORT
	/*
	else if (gotArg(  _T( "/uninstallCurrent" ), args))
	{
		DEBUG_MSG( _T("Attempting uninstall for current user.\n") );
		uninstall( false );
	}
	else if (gotArg( _T("/uninstall"), args) || gotArg( _T("/uninstallAll" ), args ))
	{
		try
		{
			DEBUG_MSG( _T("Attempting to uninstall for all users.\n") );
			uninstall( true );			
		}
		catch(const UnprivilegedException&)
		{
			if (!gotArg(_T("/restartedWithPrivileges"), args))
			{
				restartWithPrivileges( _T("/uninstallAll") );
			}
			else
			{
				DEBUG_MSG( _T("Failed to uninstall for all users, even after raising privileges.\n") );
				displayMessage( _T("Error installing BigWorld Launcher, could not gain required privileges."), MB_ICONERROR );
			}
		}
	}
	*/
	else if (gotArg( _T("/registerGame"), args ))
	{
		if (!checkInstallation(true) && !checkInstallation(false))
		{
			displayMessage( 
				_T("Cannot register a game because the BigWorld launcher is not properly installed."), 
				MB_ICONERROR );
		}
		else
		{
			// Get the next two args, which should be the full path to the source db file.
			int idx = findArg( _T("/registerGame"), args );
			assert( idx >= 0 );

			if (idx+2 >= (int)args.size())
			{
				displayMessage( _T("Usage: /registerGame gameID executablePath."),
					MB_ICONEXCLAMATION );
				return FALSE;
			}

			tstring gameID  = args[idx+1];
			tstring exePath = args[idx+2];

			try
			{
				registerGame( gameID, exePath );
			}
			catch(const UnprivilegedException&)
			{
				if (!gotArg(_T("/restartedWithPrivileges"), args))
				{				
					restartWithPrivileges( _T("/registerGame \"") + gameID + _T("\" \"") + exePath + _T("\"") );
				}
				else
				{
					DEBUG_MSG( _T("Failed to register, even after raising privileges.\n") );
					displayMessage(
						_T("Error installing BigWorld Launcher, could not gain required privileges."),						
						MB_ICONERROR );
				}
			}
		}
	}
	else if (!checkInstallation(true) && !checkInstallation(false))
	{
		int ret = displayMessage(
			_T("Do you want to install the BigWorld launcher?"), 
			MB_ICONQUESTION|MB_YESNO );

		if (ret == IDYES)
		{
			restartWithPrivileges( _T("/installAll /runAfterInstall") );
		}
	}
	else
	{
		processProtocolCommand( args );
	}

	return 0;
}

/**
 *	This function installs BWLauncher into the system by copying the executable 
 *	to a common location and then installs the required registry items for the
 *	bigworld:// protocol to work.
 *
 *	The launcher can be setup either for all users or only the current user. If
 *	it is already installed (which includes if it is already installed for all 
 *	users and we are now requesting installation for current user) then it will
 *	do nothing.
 */
bool installSelf( bool allUsers )
{
	// Check to see if there is an existing or newer version installed.
	if (checkInstallation(true) || (!allUsers && checkInstallation(false)))
	{
		return true;
	}

	// Check if target location exists
	tstring targetDir = getLauncherInstalledPath( allUsers );

	int result = SHCreateDirectoryEx( NULL, targetDir.c_str(), NULL );
	if (result != ERROR_SUCCESS && result != ERROR_ALREADY_EXISTS)
	{
		DEBUG_MSG( _T("Failed to create target executable folder.\n") );
		return false;
	}

	tstring fullTargetPath = getLauncherInstalledExePath( allUsers );

	// Copy ourselves to the target location
	if (!CopyFile( getAppFullPath().c_str(), fullTargetPath.c_str(), FALSE ))
	{
		if (GetLastError() == ERROR_ACCESS_DENIED)
		{
			throw UnprivilegedException();
		}
		DEBUG_MSG( _T("Error copying executable to target location.\n") );
		return false;
	}

	// Install this into the registry.
	HKEY baseKey = allUsers ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
	tstring protocolCmd = getProtocolShellCmd( allUsers );

	if (!writeToRegistry( baseKey, _T("Software\\Classes\\bigworld"), _T(""), _T("") ) ||
		!writeToRegistry( baseKey, _T("Software\\Classes\\bigworld"), _T("Launcher Version"), BWLAUNCHER_VERSION ) ||
		!writeToRegistry( baseKey, _T("Software\\Classes\\bigworld"), _T("URL Protocol"), _T("") ) ||
		!writeToRegistry( baseKey, _T("Software\\Classes\\bigworld\\Shell\\Open\\Command"), _T(""), protocolCmd.c_str() ))
	{
		DEBUG_MSG( _T("Failed to write launcher configuration keys to registry.\n") );
		return false;
	}

	return true;
}

/**
 *	Determines whether there is a valid installation of this launcher. Will only check
 *	based on the allUsers flag exactly (i.e. currentUser doesn't imply allUsers in this check).
 */
bool checkInstallation( bool allUsers )
{
	HKEY baseKey = allUsers ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
	
	tstring protocolValue = getProtocolShellCmd( allUsers );
	
	if (verifyRegistry( baseKey, _T("Software\\Classes\\bigworld"), _T("") ) &&
		verifyRegistry( baseKey, _T("Software\\Classes\\bigworld"), _T("Launcher Version"), BWLAUNCHER_VERSION ) &&
	    verifyRegistry( baseKey, _T("Software\\Classes\\bigworld"), _T("URL Protocol") ) &&
	    verifyRegistry( baseKey, _T("Software\\Classes\\bigworld\\Shell\\Open\\Command"), _T(""), protocolValue.c_str() ) &&
		fileExists(getLauncherInstalledExePath( allUsers )))
	{
		return true;
	}

	return false;
}

/**
 *	Finds the installed location of the launcher. Tries current user first, if that fails it tries all users.
 */
tstring findInstalledLocation()
{
	if (checkInstallation(false))
	{
		return getLauncherInstalledPath(false);
	}
	else if(checkInstallation(true))
	{
		return getLauncherInstalledPath(true);
	}
	else
	{
		return _T("");
	}
}

/**
 *	Uninstalls the registry keys for self.
 */
bool uninstall( bool allUsers )
{
	HKEY baseKey = allUsers ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
	if (!RegDelnode(  baseKey, _T("Software\\Classes\\bigworld") ))
	{
		return false;
	}

	// Hack/trick to delete ourselves. This creates a temporary batch file which we then
	// spawn before we quite ourselves. It sits in an infinite loop trying to delete and
	// closes and deletes itsef (batch files can delete themselves).
	// Reference: http://www.catch22.net/tuts/selfdel
	tstring uninstBat = getLauncherInstalledPath(allUsers) + _T("\\")_T(BWLAUNCHER_UNINST_BAT);

	HANDLE hfile = CreateFile( uninstBat.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if (hfile)
	{
		DWORD numWritten=0;
		WriteFile( hfile, (LPCVOID*)SELF_DELETE_BAT, sizeof(SELF_DELETE_BAT)-1, &numWritten, NULL );
	}

	CloseHandle(hfile);


	SHELLEXECUTEINFO sei = { sizeof( sei ) };

	tstring targetDir = getLauncherInstalledPath( allUsers );

	sei.fMask = SEE_MASK_NOCLOSEPROCESS;
	sei.nShow = SW_HIDE;

	sei.lpFile = uninstBat.c_str();
	sei.lpVerb = allUsers ? _T( "runas" ) : _T( "open" );
	sei.lpDirectory = targetDir.c_str();

	sei.lpParameters = NULL;
	
	ShellExecuteEx( &sei );

	return true;
}

/**
 *	Copies across the given DB so that it gets loaded by the launcher DB.
 */
bool registerGame( const tstring& gameID, const tstring& exePath )
{
	tstring installedLoc = findInstalledLocation();
	if (installedLoc.empty())
	{
		DEBUG_MSG( _T("Cannot register game, launcher is not properly installed.\n") );
		return false;
	}

	// This will just blat over any existing install with the same game ID.
	// Which isn't neccessarilly bad.
	tstring dbPath = installedLoc + _T("\\") + gameID + _T(".db");

	FILE* fp = NULL;
	errno_t err = _wfopen_s( &fp, dbPath.c_str(), L"w" );
	if (!fp || err != 0)
	{
		throw UnprivilegedException();
	}

	fprintf( fp, "<root>\n" );
	fprintf( fp, "\t<exePath> %S </exePath>\n",exePath.c_str() );
	fprintf( fp, "</root>\n" );

	fclose( fp );
	return true;
}

/**
 *	Processes the given command line, parsing the command coming in from the
 *	bigworld:// protocol (e.g. launch a game).
 */
bool processProtocolCommand( const StringVector& args )
{
	LaunchDB db( getAppPath() );

	if (args.size() > 1)
	{
		tstring cmd;
		tstring remainder;

		// make sure the URL does not end in a / to make parsing easier.
		tstring url = args[ 1 ];
		if (url[ url.size()-1 ] == _T('/'))
		{
			url.resize( url.size()-1 );
		}

		splitUrl( url, cmd, remainder );
		StringVector urlParts = splitString( remainder, _T("/") );

		if (cmd == _T( "launch" ))
		{
			if (urlParts.size() < 3)
			{
				displayMessage(
					_T("Invalid arguments provided to launch."), 
					MB_ICONERROR );
			}

			if (!db.launch( urlParts[0], urlParts[1], urlParts[2] ))
			{
				launchGameLibrary( urlParts[0] );
			}
		}
		else if (cmd == _T( "browse" ))
		{
			if (urlParts.empty() || urlParts[0].empty())
			{
				displayMessage(
					_T("No game ID provided."),
					MB_ICONERROR );
			}
			else
			{
				launchGameLibrary( urlParts[0] );
			}
		}
		else
		{
			displayMessage(
				( _T( "Unknown command '" ) + cmd + _T( "'" ) ).c_str(),
				MB_ICONERROR );
		}
	}
	else
	{
		// Just show the web page if nothing was provided.
		launchURL( BW_GAMES_URL );
	}

	return false;
}
