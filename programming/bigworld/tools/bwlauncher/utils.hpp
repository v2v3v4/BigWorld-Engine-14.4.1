#ifndef BWLAUNCHER_UTILS__HPP
#define BWLAUNCHER_UTILS__HPP

#include <exception>

typedef BW::vector<tstring> StringVector;
typedef BW::map<tstring, tstring> StringMap;

/**
 *	Custom exception class, raised when a permissions problem is encountered.
 */
class UnprivilegedException : std::exception
{
};

void launchURL( const tstring& url );
void launchGameLibrary( const tstring& gameID );

size_t splitCommandArgs (const tstring& str, StringVector& out, 
						 const tstring& delim = _T( " \t\r\n" ), 
						 const tstring& bind = _T( "\"" ),
						 wchar_t escape = _T( '\\' ));

int findArg( const tstring& arg, const StringVector& args );
bool gotArg( const tstring& arg, const StringVector& args );

tstring getFilename( const tstring& src );
tstring getBasePath( const tstring& src );
tstring getAppFullPath();
tstring getAppPath();
bool splitUrl( const tstring & url, tstring & cmd, tstring & remainder );
tstring getWindowsPath( int cisdl );

tstring getLauncherInstalledPath( bool allUsers );
tstring getLauncherInstalledExePath( bool allUsers );
tstring getProtocolShellCmd( bool allUsers );

bool fileExists(const tstring& fileName);
void restartWithPrivileges( const tstring& sw );
void runProgram( const tstring& cmd, const tstring& params=_T("") );

bool verifyRegistry( HKEY baseKey, LPCTSTR key, LPCTSTR name, LPCTSTR value=_T("") );
bool writeToRegistry( HKEY baseKey, LPCTSTR key, LPCTSTR name, LPCTSTR value );
bool getRegistryValue( HKEY baseKey, LPCTSTR key, LPCTSTR name, tstring& outValue );

BOOL RegDelnodeRecurse (HKEY hKeyRoot, LPTSTR lpSubKey);
BOOL RegDelnode (HKEY hKeyRoot, LPTSTR lpSubKey);

StringVector splitString( const tstring& input, const tstring& delim );
tstring replaceSubStrings( const tstring& input, const StringMap& tags );

int displayMessage( const tstring& msg, int mbFlags );

#define DEBUG_MSG(msg) OutputDebugString( (tstring(_T("BWLauncher: ")) + tstring(msg)).c_str() )
#define ERROR_MSG(msg) { ::MessageBox( NULL, _T(msg), _T("BigWorld Launcher"), MB_ICONERROR ); }

#endif
