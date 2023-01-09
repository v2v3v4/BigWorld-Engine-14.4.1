#include "pch.hpp"
#include "worldeditor/misc/cvswrapper.hpp"
#include "appmgr/options.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/multi_file_system.hpp"
#include "common/string_utils.hpp"
#include "cstdmf/message_box.hpp"
#include "resmgr/string_provider.hpp"
#include <fstream>
#include <sstream>

DECLARE_DEBUG_COMPONENT2( "CVSWrapper", 2 );

BW_BEGIN_NAMESPACE

class FileNameListFile
{
	BW::string fileName_;
public:
	template<typename T>
	FileNameListFile( const BW::string& workingFolder, const T& filenames )
	{
		BW_GUARD;

		for( int i = 0;; ++i )
		{
			BW::stringstream ss;
			ss << workingFolder << i << ".txt";
			fileName_ = ss.str();
			std::ofstream ofs( fileName_.c_str() );
			if( ofs )
			{
				for( T::const_iterator iter = filenames.begin(); iter != filenames.end(); ++iter )
				{
					ofs << *iter << std::endl;
				}
				break;
			}
		}
	}
	~FileNameListFile()
	{
		BW_GUARD;

		BW::wstring wfileName;
		bw_utf8tow( fileName_, wfileName );
		DeleteFile( wfileName.c_str() );
	}
	const BW::string& filename() const
	{
		return fileName_;
	}
};

bool CVSWrapper::isFile( const BW::string& pathName )
{
	BW_GUARD;

	IFileSystem::FileType fileType = BWResource::instance().fileSystem()->getFileType( pathName );

	return fileType == IFileSystem::FT_FILE;
}

bool CVSWrapper::isDirectory( const BW::string& pathName )
{
	BW_GUARD;

	IFileSystem::FileType fileType = BWResource::instance().fileSystem()->getFileType( pathName );

	return fileType == IFileSystem::FT_DIRECTORY;
}

bool CVSWrapper::exists( const BW::string& pathName )
{
	BW_GUARD;

	IFileSystem::FileType fileType = BWResource::instance().fileSystem()->getFileType( pathName );

	return fileType != IFileSystem::FT_NOT_FOUND;
}

static BW::string getHKCRValue( const BW::string& name )
{
	BW_GUARD;

	LONG size;
	BW::wstring wname;
	bw_utf8tow( name, wname );
	if( RegQueryValue( HKEY_CLASSES_ROOT, wname.c_str(), NULL, &size ) == ERROR_SUCCESS )
	{
		BW::wstring result( (BW::wstring::size_type)size - 1, ' ' );
		if( RegQueryValue( HKEY_CLASSES_ROOT, wname.c_str(), &result[ 0 ], &size ) == ERROR_SUCCESS )
		{
			return bw_wtoutf8( result );
		}
	}
	return "";
}

BW::string CVSWrapper::cvsPath_;
unsigned int CVSWrapper::batchLimit_;
bool CVSWrapper::directoryCommit_;
bool CVSWrapper::enabled_ = false;
BW::string CVSWrapper::dirToIgnore_;

CVSWrapper::InitResult CVSWrapper::init()
{
	BW_GUARD;

	bool useLegacySection = Options::optionExists( "CVS" );
	BW::string optionPrefix;

	if (useLegacySection)
	{
		optionPrefix = "CVS/";
		WARNING_MSG( "CVSWrapper::init: The CVS setting in options.xml is deprecated,"
			" please use \"versionControl\" instead." );
	}
	else
	{
		optionPrefix = "versionControl/";
	}

	enabled_ = Options::getOptionBool( "bwlockd/use", true ) &&
		Options::getOptionBool( optionPrefix + "enable", true );

	if( enabled_ )
	{
		cvsPath_ =
			BWResource::resolveFilename(
				Options::getOptionString(
					optionPrefix + "path", "resources/scripts/svn_stub.exe" ) );

		batchLimit_ = Options::getOptionInt( optionPrefix + "batchLimit", 128 );
		if( !BWResource::fileExists( cvsPath_ ) )
		{
			MsgBox mb( Localise(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/CVS_WRAPPER/CVS_WRAPPER_TITLE"),
					Localise(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/CVS_WRAPPER/CVS_WRAPPER_CANNOT_FIND_STUB",cvsPath_),
					Localise(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/CVS_WRAPPER/CVS_WRAPPER_EXIT"),
					Localise(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/CVS_WRAPPER/CVS_WRAPPER_CONTINUE_WITHOUT") );
			if( mb.doModal() == 0 )
			{
				return FAILURE;
			}
			enabled_ = false;
		}
		else
		{
			if( cvsPath_.rfind( '.' ) != cvsPath_.npos )
			{
				BW::string ext = cvsPath_.substr( cvsPath_.rfind( '.' ) );
				BW::string type = getHKCRValue( ext );
				if( !type.empty() )
				{
					BW::string openCommand = getHKCRValue( type + "\\shell\\open\\command" );
					if( !openCommand.empty() )
					{
						StringUtils::replace( openCommand, BW::string( "%1" ), cvsPath_ );
						StringUtils::replace( openCommand, BW::string( "%*" ), BW::string( "" ) );
						cvsPath_ = openCommand;
					}
				}
			}
			else
				cvsPath_ = '\"' + cvsPath_ + '\"';

			int exitCode;
			BW::string output;

			if( !exec( cvsPath_ + " check", ".", exitCode, output, NULL ) )
			{
				MsgBox mb( Localise(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/CVS_WRAPPER/CVS_WRAPPER_TITLE"),
					Localise(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/CVS_WRAPPER/CVS_WRAPPER_CANNOT_EXECUTE_STUB",cvsPath_),
					Localise(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/CVS_WRAPPER/CVS_WRAPPER_EXIT"),
					Localise(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/CVS_WRAPPER/CVS_WRAPPER_CONTINUE_WITHOUT") );
				if( mb.doModal() == 0 )
				{
					return FAILURE;
				}
				enabled_ = false;
			}
			else if( exitCode )//failure
			{
				MsgBox mb( Localise(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/CVS_WRAPPER/CVS_WRAPPER_TITLE"),
					Localise(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/CVS_WRAPPER/CVS_WRAPPER_CHECK_FAILED",output),
					Localise(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/CVS_WRAPPER/CVS_WRAPPER_EXIT"),
					Localise(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/CVS_WRAPPER/CVS_WRAPPER_CONTINUE_WITHOUT") );
				if( mb.doModal() == 0 )
				{
					return FAILURE;
				}
				enabled_ = false;
			}
			else
			{
				BW::stringstream ss( output );

				std::getline( ss, dirToIgnore_ );
				std::getline( ss, output );

				directoryCommit_ = !output.empty() && output[0] != '0';

				while( !dirToIgnore_.empty() && isspace( *dirToIgnore_.rbegin() ) )
					dirToIgnore_.resize( dirToIgnore_.size() - 1 );
			}
		}
	}
	return enabled_ ? SUCCESS : DISABLED;
}

CVSWrapper::CVSWrapper( const BW::string& workingPath, CVSLog* log /*= NULL*/ )
	: log_( log )
{
	BW_GUARD;

	workingPath_ = BWResource::resolveFilename( workingPath );
	if( *workingPath_.rbegin() != '/' )
		workingPath_ += '/';
}

void CVSWrapper::refreshFolder( const BW::string& relativePathName )
{
	BW_GUARD;

	if( !enabled_ )
		return;

	BW::string cmd = cvsPath_ + " refreshfolder ";
	cmd += '\"' + relativePathName + '\"';

	int exitCode;

	if( !exec( cmd, workingPath_, exitCode, output_, log_ ) )
		ERROR_MSG( "Couldn't exec %s\n", cmd.c_str() );
	else
		INFO_MSG( "refresh Done, cvs output:\n%s\n", output_.c_str() );
}

bool CVSWrapper::editFiles( BW::vector<BW::string> filesToEdit )
{
	BW_GUARD;

	if( !enabled_ )
		return true;
	bool result = true;
	while( !filesToEdit.empty() )
	{
		unsigned int limit = batchLimit_;
		BW::string cmd = cvsPath_ + " editfile";
		while( !filesToEdit.empty() && limit != 0 )
		{
			--limit;
			cmd += " \"" + filesToEdit.front() + '\"';
			filesToEdit.erase( filesToEdit.begin() );
		}

		int exitCode;

		if( !exec( cmd, workingPath_, exitCode, output_, log_ ) )
		{
			ERROR_MSG( "Couldn't exec %s\n", cmd.c_str() );
			result = false;
		}
		else
			INFO_MSG( "Update Done, cvs output:\n%s\n", output_.c_str() );
		if( exitCode != 0 )
			result = false;
	}

	return result;
}

bool CVSWrapper::revertFiles( BW::vector<BW::string> filesToRevert )
{
	BW_GUARD;

	if( !enabled_ )
		return true;
	bool result = true;
	while( !filesToRevert.empty() )
	{
		unsigned int limit = batchLimit_;
		BW::string cmd = cvsPath_ + " revertfile";
		while( !filesToRevert.empty() && limit != 0 )
		{
			--limit;
			cmd += " \"" + filesToRevert.front() + '\"';
			filesToRevert.erase( filesToRevert.begin() );
		}

		int exitCode;

		if( !exec( cmd, workingPath_, exitCode, output_, log_ ) )
		{
			ERROR_MSG( "Couldn't exec %s\n", cmd.c_str() );
			result = false;
		}
		else
			INFO_MSG( "Update Done, cvs output:\n%s\n", output_.c_str() );
		if( exitCode != 0 )
			result = false;
	}

	return result;
}

bool CVSWrapper::updateFolder( const BW::string& relativePathName )
{
	BW_GUARD;

	if( !enabled_ )
		return true;

	BW::string cmd = cvsPath_ + " updatefolder \"" + relativePathName + '\"';

	int exitCode;

	if( !exec( cmd, workingPath_, exitCode, output_, log_ ) )
	{
		ERROR_MSG( "Couldn't exec %s\n", cmd.c_str() );
		return false;
	}
	else
		INFO_MSG( "Update Done, cvs output:\n%s\n", output_.c_str() );

	return exitCode == 0;
}

bool CVSWrapper::commitFiles( const BW::set<BW::string>& filesToCommit,
		const BW::set<BW::string>& foldersToCommit, const BW::string& commitMsg )
{
	BW_GUARD;

	if( !enabled_ )
		return true;

	BW::set<BW::string> toCommit;
	if( directoryCommit_ )
	{
		toCommit = filesToCommit;
		toCommit.insert( foldersToCommit.begin(), foldersToCommit.end() );
	}
	FileNameListFile fnlf( workingPath_, directoryCommit_ ? toCommit : filesToCommit );

	bool result = true;

	int exitCode;
	BW::string cmd = cvsPath_ + " commitfile \"" + commitMsg + "\"" + " \"" + fnlf.filename() + '\"';

	if ( !exec( cmd, workingPath_, exitCode, output_, log_ ) )
	{
		ERROR_MSG( "Couldn't exec %s\n", cmd.c_str() );
		result = false;
	}
	else
		INFO_MSG( "Commit Done, cvs output:\n%s\n", output_.c_str() );

	return result && exitCode == 0;
}

bool CVSWrapper::isInCVS( const BW::string& relativePathName )
{
	BW_GUARD;

	if( !enabled_ )
		return false;

	int exitCode;
	BW::string cmd = cvsPath_ + " managed \"" + relativePathName + '\"';
	if ( !exec( cmd, workingPath_, exitCode, output_, log_ ) )
	{
		ERROR_MSG( "Couldn't exec %s\n", cmd.c_str() );
		return false;
	}
	INFO_MSG( "%s %s under version control\n", relativePathName.c_str(), exitCode == 0 ? "is" : "isn't" );
	return exitCode == 0;
}

void CVSWrapper::removeFile( const BW::string& relativePathName )
{
	BW_GUARD;

	if( !enabled_ )
		return;

	int exitCode;
	BW::string cmd = cvsPath_ + " removefile \"" + relativePathName + '\"';
	if ( !exec( cmd, workingPath_, exitCode, output_, log_ ) )
		ERROR_MSG( "Couldn't exec %s\n", cmd.c_str() );
}

BW::set<BW::string> CVSWrapper::addFolder( BW::string relativePathName, const BW::string& commitMsg,
											bool checkParent )
{
	BW_GUARD;

	BW::set<BW::string> error;
	BW::set<BW::string> result;

	if( !enabled_ )
		return error;

	if( !isDirectory( workingPath_ + relativePathName ) )
		return error;

	int exitCode = 0;

	if( !relativePathName.empty() && relativePathName[0] == '/' )
		relativePathName.erase( relativePathName.begin() );

	if( checkParent )
	{
		BW::string prefix, suffix = relativePathName;

		while( !suffix.empty() )
		{
			if( !prefix.empty() )
				prefix += '/';
			if( suffix.find( '/' ) != suffix.npos )
			{
				prefix += suffix.substr( 0, suffix.find( '/' ) );
				suffix = suffix.substr( suffix.find( '/' ) + 1 );
			}
			else
			{
				prefix += suffix;
				suffix.clear();
			}

			if( !isInCVS( prefix ) )
			{
				BW::string cmd = cvsPath_ + " addfolder \"" + commitMsg + "\" ";
				cmd += '\"' + prefix + '\"';

				if( !exec( cmd, workingPath_, exitCode, output_, log_ ) || exitCode != 0 )
				{
					ERROR_MSG( "Couldn't exec %s:\n%s\n", cmd.c_str(), output_.c_str() );
					return error;
				}
				result.insert( prefix );
			}
		}
	}
	else
	{
		BW::string cmd = cvsPath_ + " addfolder \"" + commitMsg + "\" ";
		cmd += '\"' + relativePathName + '\"';

		if( !exec( cmd, workingPath_, exitCode, output_, log_ ) || exitCode != 0 )
		{
			ERROR_MSG( "Couldn't exec %s:\n%s\n", cmd.c_str(), output_.c_str() );
			return error;
		}
		result.insert( relativePathName );
	}
	WIN32_FIND_DATA findData;
	HANDLE find = FindFirstFile( bw_utf8tow( workingPath_ + relativePathName + "/*.*" ).c_str(), &findData );
	if( find != INVALID_HANDLE_VALUE )
	{
		BW::wstring wdirToIgnore;
		bw_utf8tow( dirToIgnore_, wdirToIgnore );
		do
		{
			if( !( findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) ||
				wcscmp( findData.cFileName, L"." ) == 0 ||
				wcscmp( findData.cFileName, L".." ) == 0 ||
				wcsicmp( findData.cFileName, wdirToIgnore.c_str() ) == 0 )
				continue;
			BW::set<BW::string> files = addFolder( relativePathName + '/' + bw_wtoutf8( findData.cFileName ), commitMsg, false );
			if( files.empty() )
				return error;
			result.insert( files.begin(), files.end() );
		}
		while( FindNextFile( find, &findData ) );
		FindClose( find );
	}
	return result;
}

bool CVSWrapper::addFile( BW::string relativePathName, bool isBinary, bool recursive )
{
	BW_GUARD;

	if( !enabled_ )
		return true;

	int exitCode = 0;

	BW::string cmd = cvsPath_;
	if( isBinary )
		cmd += " addbinaryfile ";
	else
		cmd += " addfile ";
	cmd += '\"' + relativePathName + '\"';

	if ( !exec( cmd, workingPath_, exitCode, output_, log_ ) || exitCode != 0 )
	{
		ERROR_MSG( "Couldn't exec %s:\n%s\n", cmd.c_str(), output_.c_str() );
		return false;
	}

	if( relativePathName.find( '*' ) != relativePathName.npos && recursive )
	{
		WIN32_FIND_DATA findData;
		HANDLE find = FindFirstFile( bw_utf8tow( workingPath_ + "*.*" ).c_str(), &findData );
		if( find != INVALID_HANDLE_VALUE )
		{
			BW::wstring wdirToIgnore;
			bw_utf8tow( dirToIgnore_, wdirToIgnore );
			do
			{
				if( !( findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) ||
					wcscmp( findData.cFileName, L"." ) == 0 ||
					wcscmp( findData.cFileName, L".." ) == 0 ||
					wcsicmp( findData.cFileName, wdirToIgnore.c_str() ) == 0 )
					continue;
				if( !CVSWrapper( workingPath_ + bw_wtoutf8( findData.cFileName ) ).addFile( relativePathName, isBinary, recursive ) )
					return false;
			}
			while( FindNextFile( find, &findData ) );
			FindClose( find );
		}
	}

	return exitCode == 0;
}

const BW::string& CVSWrapper::output() const
{
	return output_;
}

bool CVSWrapper::exec( BW::string cmd, BW::string workingDir, int& exitCode, BW::string& output, CVSLog* log )
{
	BW_GUARD;

	output.clear();

	if( !enabled_ )
		return true;

	CWaitCursor wait;
	INFO_MSG( "executing %s in %s\n", cmd.c_str(), workingDir.c_str() );

    STARTUPINFO si;
    PROCESS_INFORMATION pi;
	SECURITY_ATTRIBUTES saAttr; 

	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
	saAttr.bInheritHandle = TRUE; 
	saAttr.lpSecurityDescriptor = NULL; 

    ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);
    ZeroMemory( &pi, sizeof(pi) );

	si.dwFlags = STARTF_USESTDHANDLES;

	HANDLE stdErrRead, stdErrWrite;

	if (!CreatePipe( &stdErrRead, &stdErrWrite, &saAttr, 0 )) 
	{
		ERROR_MSG( "Couldn't create pipe\n" );
		return false;
	}

    si.hStdError = stdErrWrite;
	si.hStdOutput = stdErrWrite;

	// Doco for "CreateProcess" says that after Win2K, this can be 32K chars.
	BW::vector< wchar_t > wcmd( cmd.length() + 1 );
	wchar_t * pWcmd = &wcmd.front();
	bw_utf8tow( cmd.c_str(), cmd.length(), pWcmd, cmd.length() );
	wcmd[ cmd.length() ] = L'\0'; // make sure we don't overflow.
	if (cmd.length() != wcslen( pWcmd ))
	{
		ERROR_MSG( "Unable to create process, command line too long (%d chars) for %s\n",
					cmd.length(), cmd.c_str() );
		return false;
	}
	if (!CreateProcess( 0, pWcmd, 0, 0, true, CREATE_NO_WINDOW, 0, bw_utf8tow( workingDir ).c_str(), &si, &pi ))
	{
		ERROR_MSG( "Unable to create process, last error is %u for %s\n", GetLastError(), cmd.c_str() );
		return false;
	}

	// stdErrWrite is used by the new process now, we don't need it
	CloseHandle( stdErrWrite );

	// Read all the output
	char buffer[1024];
	DWORD amntRead;

	while ( ReadFile( stdErrRead, buffer, 1023, &amntRead, 0 ) != 0 )
	{
		MF_ASSERT( amntRead < 1024 );

		buffer[amntRead] = '\0';
		if( log )
			log->add( bw_utf8tow( buffer ) );
		output += buffer;
	}

	CloseHandle( stdErrRead );

    // Wait until child process exits
	if (WaitForSingleObject( pi.hProcess, INFINITE ) == WAIT_FAILED)
	{
		ERROR_MSG( "WaitForSingleObject failed\n" );
		return false;
	}

	// Get the exit code
	if (!GetExitCodeProcess( pi.hProcess, (LPDWORD) &exitCode ))
	{
		ERROR_MSG( "Unable to get exit code\n" );
		return false;
	}

    // Close process and thread handles. 
    CloseHandle( pi.hProcess );
    CloseHandle( pi.hThread );

	return true;
}
BW_END_NAMESPACE

