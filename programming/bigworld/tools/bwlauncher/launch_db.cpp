#include "pch.hpp"
#include "launch_db.hpp"
#include "tinyxml.h"
#include "bwlauncher_config.hpp"
#include "utils.hpp"

#include <fstream>


namespace
{


std::string toAnsi( const tstring& str )
{
	std::string s;

	for (tstring::const_iterator iter = str.begin();
		iter != str.end(); ++iter)
	{
		s += (char)*iter;
	}

	return s;
}


tstring fromAnsi( const std::string& str )
{
	tstring s;

	for (std::string::const_iterator iter = str.begin();
		iter != str.end(); ++iter)
	{
		s += (char)*iter;
	}

	return s;
}


}

void LaunchDB::loadFile( tstring path, tstring file )
{
	TiXmlDocument doc( toAnsi( file ).c_str() );

	if (!doc.LoadFile( toAnsi(path + file).c_str() ) ||
		!doc.RootElement())
	{
		return;
	}

	TiXmlNode* node = doc.RootElement()->FirstChild( "exePath" );

	if (node && node->FirstChild())
	{
		const char* value = node->FirstChild()->Value();
		tstring exePath( fromAnsi( value ) );

		DWORD attr = GetFileAttributes( exePath.c_str() );

		if (attr != INVALID_FILE_ATTRIBUTES &&
			(attr & FILE_ATTRIBUTE_DIRECTORY) == 0)
		{
			applications_[ file.substr( 0, file.size() - 3 ) ] = exePath;
		}
	}
}


void LaunchDB::loadPath( tstring path )
{
	if (!path.empty() && *path.rbegin() != '\\')
		path += '\\';

	WIN32_FIND_DATA findData;
	HANDLE find = FindFirstFile( ( path + _T( "*.db" ) ).c_str(), &findData );

	if (find != INVALID_HANDLE_VALUE)
	{
		do
		{
			if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
			{
				loadFile( path, findData.cFileName );
			}
		}
		while (FindNextFile( find, &findData ));

		FindClose( find );
	}
}


LaunchDB::LaunchDB( tstring path )
{
	loadPath( path );
}


bool LaunchDB::launch(  const tstring& gameID, const tstring& username, const tstring& password ) const
{
	if (applications_.find( gameID ) != applications_.end())
	{
		tstring exePath = applications_.find( gameID )->second;
		SHELLEXECUTEINFO sei = { sizeof( sei ) };

		sei.fMask = SEE_MASK_NOCLOSEPROCESS;
		sei.nShow = SW_SHOW;

		sei.lpFile = exePath.c_str();

		tstring params = _T( "--username " ) + username + _T( " --password " ) + password;
		sei.lpParameters = params.c_str();

		if (ShellExecuteEx( &sei ) <= 32)
		{
			return true;
		}
	}

	DEBUG_MSG( _T( "Cannot launch application with ID " ) + gameID );
	return false;
}


size_t LaunchDB::size() const
{
	return applications_.size();
}


std::pair<tstring, tstring> LaunchDB::operator[]( size_t index ) const
{
	BW::map<tstring, tstring>::const_iterator iter = applications_.begin();

	std::advance( iter, index );

	return *iter;
}
