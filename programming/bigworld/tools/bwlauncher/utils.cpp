#include "pch.hpp"

#include "utils.hpp"
#include "bwlauncher_config.hpp"

/**
 *	Displays a message box with standard message box flags and returns the result.
 *	Uses a dummy parent window to hide the task bar button (given the launcher is a
 *	windowless app, it puts in a default taskbar icon which looks ugly).
 */
int displayMessage( const tstring& msg, int mbFlags )
{	
	HINSTANCE hInstance = GetModuleHandle(NULL);
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style              = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc        = DefWindowProc;
	wcex.cbClsExtra         = 0;
	wcex.cbWndExtra         = 0;
	wcex.hInstance          = hInstance;
	wcex.hIcon              = LoadIcon( hInstance, _T("BWLAUNCHER_ICON") );
	wcex.hCursor            = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground      = 0;
	wcex.lpszMenuName       = 0;
	wcex.lpszClassName      = _T("BWLauncher");
	wcex.hIconSm            = LoadIcon( hInstance, _T("BWLAUNCHER_ICON") );
	RegisterClassEx(&wcex);
	HWND wnd = ::CreateWindow( 
		wcex.lpszClassName, _T("BWLauncher"), WS_ICONIC | WS_DISABLED,
	  -1000, -1000, 1, 1, NULL, NULL, hInstance, NULL);

	int result = ::MessageBox( wnd, msg.c_str(), _T("BigWorld Launcher"), mbFlags );

	DestroyWindow( wnd );
	return result;
}

/**
 *	Launches the default browser into the given url.
 */
void launchURL( const tstring& url )
{
	::ShellExecute( NULL, _T( "open" ), url.c_str(), NULL, NULL, SW_SHOW );
}

/**
 *	Launches a browser to the game library.
 */
void launchGameLibrary( const tstring& gameID )
{
	StringMap tags;
	tags[ _T("%(GameID)") ] = gameID;
	tstring browseURL = replaceSubStrings( BW_GAMES_LIBRARY_URL, tags );
	launchURL( browseURL );
}

/**
 * Splits the given string in the style of Windows command line parsing (i.e. the
 * same style which is passed into a C main function under win32).
 *
 * - The string is split by delimeters (default: whitespace/newlines), which are 
 *   ignored when between matching binding characters (default: ' or "). 
 * - \" escapes the quote.
 * - \\" escapes the slash only when inside a quoted block.
 */
size_t splitCommandArgs (const tstring& str, StringVector& out, 
						 const tstring& delim, 
						 const tstring& bind,
						 wchar_t escape)
{
	const size_t prevSize = out.size();

	tstring buf;
	wchar_t binding = 0;

	for(size_t i = 0; i < str.size(); ++i)
	{
		// Process \" (escaped binding char)
		if(str[i] == escape && i < str.size()-1)
		{
			size_t pos = bind.find_first_of( str[i+1] );
			if (pos != BW::wstring::npos)
			{
				buf.push_back( bind[pos] );
				++i;
				continue;
			}
		}

		// Two modes: bound, or unbound.
		// bound = we are currently inside a quote, so process until we hit a non-quote
		if(binding)
		{
			// Process \\" (escaped slash-with-a-quote)
			if(str[i] == escape && i < str.size()-2 && str[i+1] == escape)
			{
				size_t pos = bind.find_first_of( str[i+2] );
				if (pos != BW::wstring::npos)
				{
					buf.push_back( (TCHAR)escape );
					++i;
					continue;
				}
			}

			if(str[i] == binding)
			{
				// Hit matching binding char, add buffer to output (always - empty strings included)
				out.push_back( buf );
				buf.clear();

				// Set unbound
				binding = 0;
			}
			else
			{
				// Not special, add char to buffer
				buf.push_back(str[i]);
			}
		}
		else
		{
			if(delim.find_first_of(str[i]) != BW::wstring::npos)
			{
				// Hit a delimeter
				if(!buf.empty())
				{
					// Add buffer to output (if nonempty - ignore empty strings when unbound)
					out.push_back( buf );
					buf.clear();
				}
			}
			else if(bind.find_first_of(str[i]) != BW::wstring::npos)
			{
				// Hit binding character
				if(!buf.empty())
				{
					// Add buffer to output (if nonempty - ignore empty strings when unbound)
					out.push_back( buf );
					buf.clear();
				}

				// Set bounding character
				binding = str[i];
			}
			else
			{
				// Not special, add char to buffer
				buf.push_back( str[i] );
			}
		}
	}

	if(!buf.empty())
	{
		out.push_back( buf );
	}

	return out.size() - prevSize;
}


/**
 *	Helper function to determine the index of the given argument in the vecotr of strings.
 *	Returns -1 if not found.
 */
int findArg( const tstring& arg, const StringVector& args )
{
	for( size_t i = 0; i < args.size(); i++)
	{
		if (_tcsicmp( args[i].c_str(), arg.c_str() ) == 0)
		{
			return (int)i;
		}
	}
	return -1;
}

/**
 *	Helper function to determine if the given string is in the given vector of strings.
 *	It does case insensitive comparisons.
 */
bool gotArg( const tstring& arg, const StringVector& args )
{
	return findArg( arg, args ) >= 0;
}

/**
 *	Returns only the trailing filename given a full path.
 */
tstring getFilename( const tstring& src )
{
	size_t pos = src.rfind( _T("\\") );
	if (pos == tstring::npos)
	{
		return src;
	}

	return src.substr( pos+1, tstring::npos );
}

/**
 *	Returns only the base path of the given file name
 */
tstring getBasePath( const tstring& src )
{
	size_t lastSlash = src.rfind( _T("\\") );
	if (lastSlash == tstring::npos)
		return src;

	return src.substr( 0, lastSlash );
}

/**
 *	Helper function that gets the full path to the current process's executable.
 *	This version includes the executable file name.
 */
tstring getAppFullPath()
{
	BW::vector<TCHAR> path( MAX_PATH );

	while (GetModuleFileName( NULL, &path[0], (DWORD)path.size() ) == path.size())
	{
		path.resize( path.size() * 2 );
	}

	_tcslwr_s( &path[0], path.size() );

	return &path[0];
}

/**
 *	Helper function that gets the full path to the current process's executable.
 *	This version does not include the executable file name.
 */
tstring getAppPath()
{
	return getBasePath ( getAppFullPath() );
}

/**
 *	This will parse the given raw URL string into two parts: the command
 *	and the remainder. The format of the URL is:
 *
 *	bigworld://command/remainder
 *
 */
bool splitUrl( const tstring & url, tstring & cmd, tstring & remainder )
{
	size_t start = 0;

	if (_tcsnicmp( url.c_str(), _T( "bigworld:" ), 9 ) == 0)
	{
		start += 9;
	}

	while (url[ start ] == '/')
	{
		++start;
	}

	size_t end = url.find( '/', start );

	if (end != url.npos)
	{
		cmd = url.substr( start, end - start );
		remainder = url.substr( end + 1 );
	}
	else
	{
		cmd = url.substr( start );
	}

	return true;
}

/*
 *	Helper function that wraps SHGetFolderPath. Returns "" if the path can't be found.
 */
tstring getWindowsPath( int cisdl )
{
	TCHAR wappDataPath[MAX_PATH];
	HRESULT result= SHGetFolderPath( NULL, cisdl, NULL, SHGFP_TYPE_CURRENT, wappDataPath );
	if( SUCCEEDED( result ) )
	{	
		return wappDataPath;
	}
	return _T("");
}

/*
 *	Gets the directory where the launcher should be installed to.
 */
tstring getLauncherInstalledPath( bool allUsers )
{
	int flags = allUsers ? CSIDL_COMMON_APPDATA : CSIDL_LOCAL_APPDATA;
	tstring targetDir = getWindowsPath( flags|CSIDL_FLAG_CREATE );
	targetDir += _T("\\BigWorld");
	return targetDir;
}

/*
 *	Gets the full path and filename of the launcher executable's correct installed location.
 */
tstring getLauncherInstalledExePath( bool allUsers )
{
	int flags = allUsers ? CSIDL_COMMON_APPDATA : CSIDL_LOCAL_APPDATA;
	tstring fullPath = getWindowsPath( flags|CSIDL_FLAG_CREATE );
	fullPath += _T("\\BigWorld\\")_T(BWLAUNCHER_EXE_NAME);
	return fullPath;
}

/*
 *	Assembles the shell command string to be installed into the registry for 
 *	the bigworld:// protocol.
 */
tstring getProtocolShellCmd( bool allUsers )
{
	return _T("\"") + getLauncherInstalledExePath( allUsers ) + _T("\" \"%1\"");
}

/*
 *	Determines if the given file exists on the system.
 */
bool fileExists(const tstring& fileName)
{
	return GetFileAttributes(fileName.c_str()) != INVALID_FILE_ATTRIBUTES;
}

/*
 *	Restarts the process with the appropriate installation switch, using elevated
 *	administrator priviliges.
 */
void restartWithPrivileges( const tstring& sw )
{
	SHELLEXECUTEINFO sei = { sizeof( sei ) };
	
	tstring fullPath = getAppFullPath();
	tstring fullParams = sw + _T( " /restartedWithPrivileges" );

	sei.fMask = SEE_MASK_NOCLOSEPROCESS;
	sei.nShow = SW_HIDE;

	sei.lpFile = fullPath.c_str();
	sei.lpVerb = _T( "runas" );

	sei.lpParameters = fullParams.c_str();
	
	DEBUG_MSG( _T("Restarting with elevated privileges.\n") );
	ShellExecuteEx( &sei );
}

void runProgram( const tstring& cmd, const tstring& params )
{
	SHELLEXECUTEINFO sei = { sizeof( sei ) };
	
	sei.fMask = SEE_MASK_NOCLOSEPROCESS;
	sei.nShow = SW_HIDE;

	sei.lpFile = cmd.c_str();
	sei.lpVerb = _T( "open" );

	sei.lpParameters = params.c_str();
	
	ShellExecuteEx( &sei );
}

/*
 *	Helper function that extracts a value out of the registry and compares it against
 *	a reference value. Returns true if the value in the registry matches the reference.
 */
bool verifyRegistry( HKEY baseKey, LPCTSTR key, LPCTSTR name, LPCTSTR value )
{
	DWORD type;
	TCHAR v[ 1024 ];
	DWORD size = sizeof( v ) / sizeof( *v );
	HKEY hkey;
	
	if (RegOpenKey( baseKey, key, &hkey ) == ERROR_SUCCESS)
	{
		if (RegQueryValueEx( hkey, name, NULL, &type, (LPBYTE)v, &size )
			== ERROR_SUCCESS && _tcscmp( v, value ) == 0 &&
			type == REG_SZ)
		{
			RegCloseKey( hkey );
			return true;
		}

		RegCloseKey( hkey );
	}

	return false;
}

/*
 *	Helper function that writs the given key/value pair into the Window registry.
 *	Raises UnprivilegedException if we couldn't write due to a permissions problem.
 */
bool writeToRegistry( HKEY baseKey, LPCTSTR key, LPCTSTR name, LPCTSTR value )
{
	HKEY hkey;
	LONG result = RegCreateKey( baseKey, key, &hkey );

	if (result != ERROR_SUCCESS)
	{
		if (result == ERROR_ACCESS_DENIED)
		{
			throw UnprivilegedException();
		}

		return false;
	}

	result = RegSetValueEx( hkey, name, 0, REG_SZ, (BYTE*)value, ( (DWORD)_tcslen( value ) + 1 ) * sizeof( TCHAR ) );
	RegCloseKey( hkey );

	if (result != ERROR_SUCCESS)
	{
		if (result == ERROR_ACCESS_DENIED)
		{
			throw UnprivilegedException();
		}

		return false;
	}

	return true;
}

/*
 *	Gets the value of the given registery key. Returns false if it wasn't found.
 */
bool getRegistryValue( HKEY baseKey, LPCTSTR key, LPCTSTR name, tstring& outValue )
{
	DWORD type;
	TCHAR v[ 1024 ];
	DWORD size = sizeof( v ) / sizeof( *v );
	HKEY hkey;
	
	if (RegOpenKey( baseKey, key, &hkey ) == ERROR_SUCCESS)
	{
		if (RegQueryValueEx( hkey, name, NULL, &type, (LPBYTE)v, &size )
			== ERROR_SUCCESS &&	type == REG_SZ)
		{
			outValue = v;
			RegCloseKey( hkey );
			return true;
		}

		RegCloseKey( hkey );
	}

	return false;
}

//*************************************************************
//
//  RegDelnodeRecurse()
//
//  Purpose:    Deletes a registry key and all its subkeys / values.
//
//  Parameters: hKeyRoot    -   Root key
//              lpSubKey    -   SubKey to delete
//
//  Return:     TRUE if successful.
//              FALSE if an error occurs.
//
//*************************************************************
BOOL RegDelnodeRecurse (HKEY hKeyRoot, LPTSTR lpSubKey)
{
	LPTSTR lpEnd;
	LONG lResult;
	DWORD dwSize;
	TCHAR szName[MAX_PATH];
	HKEY hKey;
	FILETIME ftWrite;

	// First, see if we can delete the key without having
	// to recurse.

	lResult = RegDeleteKey(hKeyRoot, lpSubKey);

	if (lResult == ERROR_SUCCESS) 
		return TRUE;

	lResult = RegOpenKeyEx (hKeyRoot, lpSubKey, 0, KEY_READ, &hKey);

	if (lResult != ERROR_SUCCESS) 
	{
		if (lResult == ERROR_ACCESS_DENIED)
		{
			throw UnprivilegedException();
		}

		if (lResult == ERROR_FILE_NOT_FOUND) {
			//printf("Key not found.\n");
			return TRUE;
		} 
		else {
			//printf("Error opening key.\n");
			return FALSE;
		}
	}

	// Check for an ending slash and add one if it is missing.

	lpEnd = lpSubKey + lstrlen(lpSubKey);

	if (*(lpEnd - 1) != _T('\\')) 
	{
		*lpEnd =  _T('\\');
		lpEnd++;
		*lpEnd =  _T('\0');
	}

	// Enumerate the keys

	dwSize = MAX_PATH;
	lResult = RegEnumKeyEx(hKey, 0, szName, &dwSize, NULL,
						   NULL, NULL, &ftWrite);

	if (lResult == ERROR_SUCCESS) 
	{
		do {

			StringCchCopy (lpEnd, MAX_PATH*2, szName);

			if (!RegDelnodeRecurse(hKeyRoot, lpSubKey)) {
				break;
			}

			dwSize = MAX_PATH;

			lResult = RegEnumKeyEx(hKey, 0, szName, &dwSize, NULL,
								   NULL, NULL, &ftWrite);

		} while (lResult == ERROR_SUCCESS);
	}

	lpEnd--;
	*lpEnd = _T('\0');

	RegCloseKey (hKey);

	// Try again to delete the key.

	lResult = RegDeleteKey(hKeyRoot, lpSubKey);

	if (lResult == ERROR_SUCCESS) 
		return TRUE;

	if (lResult == ERROR_ACCESS_DENIED)
	{
		throw UnprivilegedException();
	}

	return FALSE;
}

//*************************************************************
//
//  RegDelnode()
//
//  Purpose:    Deletes a registry key and all its subkeys / values.
//
//  Parameters: hKeyRoot    -   Root key
//              lpSubKey    -   SubKey to delete
//
//  Return:     TRUE if successful.
//              FALSE if an error occurs.
//
//*************************************************************
BOOL RegDelnode (HKEY hKeyRoot, LPTSTR lpSubKey)
{
	TCHAR szDelKey[MAX_PATH*2];

	StringCchCopy (szDelKey, MAX_PATH*2, lpSubKey);
	return RegDelnodeRecurse(hKeyRoot, szDelKey);

}


/**
 *	Split the given input string into a list of strings, similar to Python split().
 */
StringVector splitString( const tstring& input, const tstring& delim )
{
	StringVector output;

	size_t pos = 0;
	size_t next = 0;
	while( (next = input.find(delim, pos)) != tstring::npos )
	{
		output.push_back( input.substr(pos, next-pos) );
		pos = next + 1;
	}

	if (pos < input.size())
	{
		output.push_back( input.substr(pos) );
	}

	if (output.empty())
	{
		output.push_back( _T("") );
	}

	return output;
}

/**
 *	Replace sub strings with other strings.
 */
tstring replaceSubStrings( const tstring& input, const StringMap& tags )
{
	tstring output = input;

	for(StringMap::const_iterator it = tags.begin(); it != tags.end(); it++)
	{
		size_t pos = 0;
		while( (pos = output.find(it->first, pos)) != tstring::npos )
		{
			output.replace( pos, it->first.size(), it->second );
			pos += it->second.size();
		}
	}

	return output;
}
