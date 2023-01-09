#include "pch.hpp"
#include "bw_util.hpp"

#include "stdmf.hpp"
#include "bw_string.hpp"
#include "debug.hpp"
#include "guard.hpp"
#include "string_utils.hpp"


BW_BEGIN_NAMESPACE

namespace BWUtil
{

void sanitisePath( BW::string & path )
{
	BW_GUARD;
	std::replace( path.begin(), path.end(), '\\', '/' );
}

void sanitisePath( char * p )
{
	BW_GUARD;
	while (*p)
	{
		if (*p == '\\')
			*p = '/';
		++p;
	}
}

BW::string getFilePath( const BW::StringRef & pathToFile )
{
	BW_GUARD;
	// NB: this does not behave the same way as the linux implementation
	BW::StringRef directory;
	BW::StringRef nFile = pathToFile;

	if ((pathToFile.length() >= 1) &&
		(pathToFile[ pathToFile.length() - 1 ] != '/'))
	{
		BW::string::size_type pos = pathToFile.find_last_not_of( " " );

		if (pos != BW::string::npos)
		{
			nFile = pathToFile.substr( 0, pos );
		}
	}

	BW::string::size_type pos = nFile.find_last_of( "\\/" );

	if (pos != BW::string::npos)
	{
		directory = nFile.substr( 0, pos );
	}

	return BWUtil::formatPath( directory );
}

bool getExecutablePath( char * pathBuffer, size_t pathBufferSize )
{
	BW_GUARD;

	// get application directory
	wchar_t wbuffer[MAX_PATH];

	DWORD result = GetModuleFileNameW( NULL, wbuffer, ARRAY_SIZE( wbuffer ) );
	if ( result == 0 )
	{
		DWORD error = GetLastError();
		if ( error != EXIT_SUCCESS )
		{
			ERROR_MSG(
				"GetModuleFileNameW failed (GetLastError=%lu).\n",
				GetLastError() );
		}
		return false;
	}

	bw_wtoutf8( wbuffer, wcslen( wbuffer ), pathBuffer, pathBufferSize);

	BWUtil::sanitisePath( pathBuffer );

	return true;
}

} // namespace BWUtil

int bw_unlink( const char* filename )
{
	BW_GUARD;
	wchar_t wFilename[ MAX_PATH + 1 ];
	bw_utf8tow( filename, strlen( filename), wFilename, MAX_PATH );
	return _wunlink( wFilename );

}

int bw_rename( const char *oldname, const char *newname )
{
	BW_GUARD;
	wchar_t wOldname[ MAX_PATH + 1 ];
	wchar_t wNewname[ MAX_PATH + 1];
	bw_utf8tow( oldname, strlen( oldname ), wOldname, MAX_PATH );
	bw_utf8tow( newname, strlen( newname), wNewname, MAX_PATH );

	return _wrename( wOldname, wNewname );
}


FILE* bw_fopen( const char* filename, const char* mode )
{
	BW_GUARD;
	wchar_t wFilename[ MAX_PATH + 1 ];
	wchar_t wMode[ MAX_PATH + 1];

	bw_utf8tow( filename, strlen( filename ), wFilename, MAX_PATH );
	bw_utf8tow( mode, strlen( mode ), wMode, MAX_PATH );

	return _wfopen( wFilename, wMode );
}


long bw_fileSize( FILE* file )
{
	BW_GUARD;
	long currentLocation = ftell(file);
	if (currentLocation < 0) 
	{
		currentLocation = 0;
	}

	int res = fseek(file, 0, SEEK_END);
	if (res)
	{
		ERROR_MSG("bw_fileSize: fseek failed\n");
		return -1;
	}
	long length = ftell(file);
	res = fseek(file, currentLocation, SEEK_SET);
	if (res)
	{
		ERROR_MSG("bw_fileSize: fseek failed\n");
		return -1;
	}
	return length;
}

/**
 *	return a temp file name
 */
BW::wstring getTempFilePathName()
{
	BW_GUARD;
	wchar_t tempDir[ MAX_PATH + 1 ];
	wchar_t tempFile[ MAX_PATH + 1 ];

	if (GetTempPathW( MAX_PATH + 1, tempDir ) < MAX_PATH)
	{
		if (GetTempFileNameW( tempDir, L"BWT", 0, tempFile ))
		{
			return tempFile;
		}
	}

	return L"";
}

BW_END_NAMESPACE

// bw_util_windows.cpp
