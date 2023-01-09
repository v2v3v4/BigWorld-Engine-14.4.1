#include "asset_compiler_options.hpp"

#include "asset_compiler.hpp"

#include "asset_pipeline/conversion/content_addressable_cache.hpp"

#include "resmgr/datasection.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/multi_file_system.hpp"

#include "cstdmf/command_line.hpp"

BW_BEGIN_NAMESPACE

AssetCompilerOptions::AssetCompilerOptions()
	: cachePath_()
	, intermediatePath_()
	, outputPath_()
	, numThreads_(0)
	, forceRebuild_(false)
	, recursive_(false)
	, enableCacheRead_(true)
	, enableCacheWrite_(true)
	, cachePathChanged_(false)
	, intermediatePathChanged_(false)
	, outputPathChanged_(false)
	, numThreadsChanged_(false)
{
}


AssetCompilerOptions::AssetCompilerOptions( const AssetCompiler & compiler )
	: cachePath_( ContentAddressableCache::getCachePath() )
	, intermediatePath_( compiler.getIntermediatePath() )
	, outputPath_( compiler.getOutputPath() )
	, numThreads_( compiler.getThreadCount() )
	, forceRebuild_( compiler.getForceRebuild() )
	, recursive_( compiler.getRecursive() )
	, enableCacheRead_( ContentAddressableCache::getReadFromCache() )
	, enableCacheWrite_( ContentAddressableCache::getWriteToCache() )
	, cachePathChanged_(false)
	, intermediatePathChanged_(false)
	, outputPathChanged_(false)
	, numThreadsChanged_(false)
{
}

bool AssetCompilerOptions::readFromDataSection( const DataSectionPtr & dataSection )
{
	MF_ASSERT(false); // Unimplemented
	return false;
}

bool AssetCompilerOptions::writeToDataSection( DataSectionPtr & dataSection ) const
{
	MF_ASSERT(false); // Unimplemented
	return false;
}

bool AssetCompilerOptions::parseCommandLine( const BW::string & commandString )
{
	BW::CommandLine commandLine( commandString.c_str() );

	if (commandLine.hasParam( "intermediatePath" ))
	{
		intermediatePath( commandLine.getParam( "intermediatePath" ) );
	}

	if (commandLine.hasParam( "outputPath" ))
	{
		outputPath( commandLine.getParam( "outputPath" ) );
	}

	if (commandLine.hasParam( "cachePath" ))
	{
		cachePath( commandLine.getParam( "cachePath" ) );
	}

	if (commandLine.hasParam( "j" ))
	{
		threadCount( atoi( commandLine.getParam( "j" ) ) );
	}

	recursive_ = commandLine.hasParam( "recursive" );
	forceRebuild_ = commandLine.hasParam( "forceRebuild" );

	return true;
}

bool AssetCompilerOptions::apply( AssetCompiler & compiler ) const
{
	if (intermediatePathChanged_)
	{
		compiler.setIntermediatePath(intermediatePath_);
		intermediatePathChanged_ = false;
	}

	if (outputPathChanged_)
	{
		// Must be set after Intermediate path
		compiler.setOutputPath(outputPath_);
		outputPathChanged_ = false;
	}

	if (numThreadsChanged_)
	{
		compiler.setThreadCount(numThreads_);
		numThreadsChanged_ = false;
	}

	compiler.setForceRebuild(forceRebuild_);
	compiler.setRecursive(recursive_);

	if (cachePathChanged_)
	{
		ContentAddressableCache::setCachePath(cachePath_);
		cachePathChanged_ = false;
	}

	ContentAddressableCache::setReadFromCache(enableCacheRead_);
	ContentAddressableCache::setWriteToCache(enableCacheWrite_);

	return true;
}

const BW::string & AssetCompilerOptions::cachePath() const
{
	return cachePath_;
}

void AssetCompilerOptions::cachePath( const BW::string & path )
{
	BW::string cleanPath;
	if (sanitisePath( path, cleanPath ) && cleanPath != cachePath_)
	{
		cachePath_ = cleanPath;
		cachePathChanged_ = true;
	}
}

const BW::string & AssetCompilerOptions::intermediatePath() const
{
	return intermediatePath_;
}

void AssetCompilerOptions::intermediatePath( const BW::string & path )
{
	BW::string cleanPath;
	if (sanitisePath( path, cleanPath ) && cleanPath != intermediatePath_)
	{
		intermediatePath_ = cleanPath;
		intermediatePathChanged_ = true;
	}
}

const BW::string & AssetCompilerOptions::outputPath() const
{
	return outputPath_;
}

void AssetCompilerOptions::outputPath( const BW::string & path )
{
	BW::string cleanPath;
	if (sanitisePath( path, cleanPath ) && cleanPath != outputPath_)
	{
		outputPath_ = cleanPath;
		outputPathChanged_ = true;
	}
}

int AssetCompilerOptions::threadCount() const
{
	return numThreads_;
}

void AssetCompilerOptions::threadCount( int numThreads )
{
	if (numThreads != numThreads_)
	{
		numThreads_ = numThreads;
		numThreadsChanged_ = true;
	}
}

bool AssetCompilerOptions::forceRebuild() const
{
	return forceRebuild_;
}

void AssetCompilerOptions::forceRebuild( bool rebuild )
{
	forceRebuild_ = rebuild;
}

bool AssetCompilerOptions::recursive() const
{
	return recursive_;
}

void AssetCompilerOptions::recursive( bool recurse )
{
	recursive_ = recurse;
}

bool AssetCompilerOptions::enableCacheRead() const
{
	return enableCacheRead_;
}

void AssetCompilerOptions::enableCacheRead( bool enable )
{
	enableCacheRead_ = enable;
}

bool AssetCompilerOptions::enableCacheWrite() const
{
	return enableCacheWrite_;
}

void AssetCompilerOptions::enableCacheWrite( bool enable )
{
	enableCacheWrite_ = enable;
}


bool AssetCompilerOptions::sanitisePath( const BW::string & path, BW::string & cleanPath )
{
	cleanPath = "";

	if (path.empty())
	{
		// An empty path is valid for our purposes
		return true;
	}

	BW::string sanitisedPath = AssetCompiler::sanitisePathParam( path.c_str() );
	BW::BWResource::ensureAbsolutePathExists( sanitisedPath );

	cleanPath = sanitisedPath;

	BW::MultiFileSystem* fs = BW::BWResource::instance().fileSystem();
	BW::IFileSystem::FileType ft = fs->getFileType( sanitisedPath );
	if (ft != BW::IFileSystem::FT_DIRECTORY)
	{
		return false;
	}

	return true;
}



BW_END_NAMESPACE
