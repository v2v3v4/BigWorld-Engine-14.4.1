#include "pch.hpp"
#include "command.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/auto_config.hpp"
#include "cstdmf/bw_util.hpp"
#include "cstdmf/debug_filter.hpp"
#include "cstdmf/debug.hpp"

DECLARE_DEBUG_COMPONENT2( "CommandManager", 0 )

BW_BEGIN_NAMESPACE

namespace OfflineProcessor
{
/**
 * Default constructor for the base Command class
 */
Command::Command( const CommandLine& commandLine ):
commandLine_( commandLine )
{
}

/**
 * Default constructor for the base BatchCommand class
 */
BatchCommand::BatchCommand( const CommandLine& commandLine,
							bool allowMisssingFiles ):
Command( commandLine ),
allowMisssingFiles_( allowMisssingFiles ),
numErrors_( 0 )
{
}

/**
 *	This function initialised resource system for batch processing
 *  If file name is specified initialise BWResource using file's folder,
 *  otherwise just use fileName as BWResource path
 *	@param fileName		name of the file or folder to process
 */
BatchCommand::ResourceSystemInitResult	BatchCommand::initResourceSystem( const BW::string& fileName )
{
	// don't pass in the path args, as we need the paths.xml paths added 
	//	so we can initialise the resources.xml & the texture_detail_files.xml
	const char* cmdLine[] = { "" };
	int num = 1;

	// load up any paths.xml that we can find.
	TRACE_MSG("looking for paths to include by looking for paths.xml in: \n");
	TRACE_MSG("%s\n", BWResource::getCurrentDirectory().c_str() );
	TRACE_MSG("%s\n", BWUtil::executableDirectory().c_str() );

	if (!BWResource::init( num, cmdLine, false ))
	{
		TRACE_MSG("failed to load paths.xml, no processing");
		return RES_INIT_FAILED;
	}

	TRACE_MSG("successfully found paths.xml, paths now =\n");
	for ( int i = 0; i < BWResource::getPathNum(); ++i )
	{
		TRACE_MSG("%s\n", BWResource::getPath( i ).c_str() );
	}

	MultiFileSystem* fs = BWResource::instance().fileSystem();

	BW::string resName = fileName;
	IFileSystem::FileType ft = fs->getFileType( resName );

	if (ft == IFileSystem::FT_NOT_FOUND &&
		BWResource::pathIsRelative( resName ) )
	{
		// Could not find the file but its a relative path so lets try looking for it in the current directory
		BW::string resNameCurrentDir = BWResource::getCurrentDirectory() + resName;
		ft = fs->getFileType( resNameCurrentDir );
		if (ft != IFileSystem::FT_NOT_FOUND)
		{
			TRACE_MSG("Path is relative, found in current directory, now converting: %s.\n", resNameCurrentDir.c_str());
			resName = resNameCurrentDir;
		}
	}

	ResourceSystemInitResult res = RES_INIT_FAILED;	

	if (ft == IFileSystem::FT_FILE)
	{
		TRACE_MSG("input file is a single file, converting only that file.\n");
		res = RES_INIT_SUCCESS_FILE;
		// strip file name from path
		resName = BWResource::getFilePath( resName );
	}
	else if (ft == IFileSystem::FT_DIRECTORY)
	{
		TRACE_MSG("input file is a directory, converting all files within this directory\n");
		res = RES_INIT_SUCCESS_DIRECTORY;
	}
	else if (ft == IFileSystem::FT_NOT_FOUND && allowMisssingFiles_)
	{
		TRACE_MSG("Could not find input file, assuming it is a single file and converting only that file.\n");
		res = RES_INIT_SUCCESS_FILE;
		// strip file name from path
		resName = BWResource::getFilePath( resName );
	}
	else
	{
		TRACE_MSG("trying to initialise resource system, but cannot find file %s.\n", resName.c_str() );
		
		ERROR_MSG(" trying to initialise resource system, but cannot find file %s\n",resName.c_str());
		// file not found
		return RES_INIT_FAILED;
	}

	// add the source file path if it doesn't exist already
	BW::string relatvefilename = BWResource::instance().dissolveFilename( resName );
	if (relatvefilename == "")
	{
		TRACE_MSG("input file is not within the paths, adding it\n");
		BWResource::addPath( resName );
	}	

#ifndef MF_SERVER
	// init auto-config strings
	if ( !AutoConfig::configureAllFrom( "resources.xml" ) )
	{
		// this sets up the config variable that points the texture conversion rules to the
		//	texturedetails defined in the resources.xml that appears in the root of the filenames.
		//	If the texture details cannot be initialised, it is bad for texture-convert, but not necesarily 
		//	bad for other commands. So it doesnt reprot a failure here.
		//		Possibly should differentiate between commands?
		ERROR_MSG("[OfflineProcessor] Error: Couldn't load auto-config strings from resource.xml\n" );
	}
#endif

	return res;
}

/**
 *	This function processes file(s) in the given path.
 *  Path might be a file path or a folder path.
 *  If it's folder path function will process all files inside this folder,
 *  and if recursive flag set it will process all children folders as well
 *  If file name is a simple file it will process this one file and return
 *
 *	@param fileName		name of the file or folder to process
 *	@param bool			recursive flag
 *  @param outFileName  name of the output file or folder to process
 *
 *	@retrun bool		returns number of processed files.
 */
int BatchCommand::processFiles( const BW::string& fileName,
							    bool processRecursively,
								const BW::string& outFileName )
{
	int numFilesProcessed = 0;
	MultiFileSystem* fs = BWResource::instance().fileSystem();
	IFileSystem::FileType ft = fs->getFileType( fileName, NULL );
	if ( ft == IFileSystem::FT_FILE ||
		(ft == IFileSystem::FT_NOT_FOUND && allowMisssingFiles_))
	{
		FileProcessResult pr = this->processFile( fileName, outFileName );
		if (pr == RESULT_SUCCESS)
		{
			numFilesProcessed++;
		}
		else if (pr == RESULT_FAILURE)
		{
			numErrors_++;
		}
	}
	else
	{
		IFileSystem::Directory dirList;
		fs->readDirectory( dirList, fileName );

		bool bRes = true;
		for( IFileSystem::Directory::iterator it = dirList.begin();
			it != dirList.end(); ++it )
		{
			BW::string subName =  BWUtil::formatPath( fileName ) + (*it);
			const char * rootPrefix = "./";
			if (strncmp( subName.c_str(), rootPrefix, strlen( rootPrefix ) ) == 0)
			{
				// Remove ./ prefix so that the filename is the format expected by
				// the texture level details manager
				subName = subName.substr( strlen( rootPrefix ), subName.length() );
			}

       		// check if it is a directory
			bool bIsDirectory = (fs->getFileType( subName, NULL ) == IFileSystem::FT_DIRECTORY) ? true: false;
			if ( processRecursively && bIsDirectory )
			{
				BW::string outPathName = outFileName;

				// add the current directory onto the output file so we output the directory structure from the input
				if (!outPathName.empty())
				{
					outPathName += (*it);
					outPathName = BWResource::formatPath( outPathName );
				}

				// recurse into the directory, and start it all again
				
				numFilesProcessed += this->processFiles( subName, processRecursively, outPathName );
			}
			else if(bIsDirectory == false) // don't want to process directories if we are not recursive
			{
				
				BWResource::ensureAbsolutePathExists(outFileName);

				FileProcessResult pr = this->processFile( subName, outFileName );
				if (pr == RESULT_SUCCESS)
				{
					numFilesProcessed++;
				}
				else if (pr == RESULT_FAILURE)
				{
					numErrors_++;
				}
			}
		}
	}
	return numFilesProcessed;
}

/**
 *	This function initialised BWResource and runs batch processing
 *  using command argument as a path.
 *  Path might point to a file or folder.\
 *  If -recusive option is set, processing runs recursively.
 *  At the end it outputs number of processed files and number of errors.
 */
bool	BatchCommand::process()
{
	BW::string path(commandLine_.getFullParam( strCommand(), 1 ));
	BW::string outPath;
	if(commandLine_.size() > 3)
	{
		if (commandLine_.hasParam( "output" ))
		{
			outPath = commandLine_.getParam( "output" );
		}
	}

	bool bCommandParamValid = path.length() > 0 && !CommandLine::hasSwitch( path.c_str() );
	bool processRecursively = commandLine_.hasParam("recursive");
	
	if (!bCommandParamValid && processRecursively)
	{
		path = ".";
		bCommandParamValid = true;
	}

	if (!bCommandParamValid)
	{
		this->showDetailedHelp();
		return false;
	}

	ResourceSystemInitResult resInit = initResourceSystem( path );

	if (!outPath.empty() && BWResource::pathIsRelative( outPath ))
	{
		// Convert the relative outPath to an absolute path
		BW::string outFileName = "";
		IFileSystem::FileType ft = BWResource::resolveToAbsolutePath( outPath );
		while (ft == IFileSystem::FT_NOT_FOUND)
		{
			// while the outPath does not exist in any of our paths
			// truncate the outPath and try again
			outFileName = "/" +	BWResource::getFilename( outPath ) + outFileName;
			outPath = BWResource::getFilePath( outPath );

			if (outPath.empty())
			{
				// No subset of the outPath exists in any of our known paths
				// So make the outPath relative to the current directory
				outPath = BWResource::getCurrentDirectory();
				break;
			}

			// remove trailing "/\\"
			outPath.pop_back();
			ft = BWResource::resolveToAbsolutePath( outPath );
		}

		if (!outFileName.empty())
		{
			outPath.pop_back();
			outPath += outFileName;
		}

		// ensure the outPath exists
		BWResource::ensureAbsolutePathExists( outPath );
	}

	int numFilesProcessed = 0;

	if (resInit != RES_INIT_FAILED)
	{
		if (this->initCommand())
		{
			if (outPath == "")
			{
				TRACE_MSG("Processing %s.\n", path.c_str());
			}
			else
			{
				TRACE_MSG("Processing (%s) to (%s).\n", path.c_str(), outPath.c_str());
			}
			if (resInit == RES_INIT_SUCCESS_DIRECTORY && processRecursively)
			{
				TRACE_MSG("recursively \n");
			}
			
			if ((resInit == RES_INIT_SUCCESS_FILE) || (resInit == RES_INIT_SUCCESS_DIRECTORY) )
			{
				// all input files have their paths added, so each filename needs to be relative to that now.
				path = BWResource::dissolveFilename( path );
			}
			else
			{
				path = "";
			}
			numFilesProcessed = this->processFiles( path, processRecursively, outPath );
			this->finiCommand();
		}
	}
	else
	{
		ERROR_MSG("[OfflineProcessor] ERROR: Can' find path %s.\n", path.c_str() );
		numErrors_ = 1;
	}
	TRACE_MSG("%d files processed.\n", numFilesProcessed );
	TRACE_MSG("%d error (s).\n", numErrors_);
	if (resInit == RES_INIT_SUCCESS_DIRECTORY && numFilesProcessed == 0 && !processRecursively)
	{
		TRACE_MSG("Did you forget to add -recursive option ?\n");
	}

	

	BWResource::fini();

	return numErrors_ == 0;
}

} // namespace OfflineProcessor

BW_END_NAMESPACE

