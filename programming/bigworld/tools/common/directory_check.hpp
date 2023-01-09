#pragma once

BW_BEGIN_NAMESPACE

class DirectoryCheck
{
public:
	DirectoryCheck( const BW::wstring& appName )
	{
		BW_GUARD;

		static const int MAX_PATH_SIZE = 8192;
		wchar_t curDir[ MAX_PATH_SIZE ];
		GetCurrentDirectory( MAX_PATH_SIZE, curDir );

		BW::wstring directorPath = curDir;

		if (!directorPath.empty() && *directorPath.rbegin() != '\\')
		{
			directorPath += '\\';
		}

		directorPath = directorPath + L"resources\\scripts\\" + appName + L"Director.py";

		if( GetFileAttributes( directorPath.c_str() ) == INVALID_FILE_ATTRIBUTES )
		{// we are not running in the proper directory
			GetModuleFileName( NULL, curDir, MAX_PATH_SIZE );
			if( _tcsrchr( curDir, '\\' ) )
				*_tcsrchr( curDir, '\\' ) = 0;

			directorPath = curDir;
			directorPath = directorPath + L"\\resources\\scripts\\" + appName + L"Director.py";

			if( GetFileAttributes( directorPath.c_str() ) != INVALID_FILE_ATTRIBUTES )
			{
				SetCurrentDirectory( curDir );
			}
			else
			{
				GetCurrentDirectory( MAX_PATH_SIZE, curDir );
				MessageBox( NULL,
					Localise(L"COMMON/DIRECTORY_CHECK/DIR_CHECK" , appName, curDir, appName ),
					appName.c_str(), MB_OK | MB_ICONASTERISK );
			}
		}
	}
};
BW_END_NAMESPACE

