#include "content_addressable_cache.hpp"

#include "asset_pipeline/compiler/compiler.hpp"
#include "cstdmf/string_builder.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/multi_file_system.hpp"

BW_BEGIN_NAMESPACE

BW::string ContentAddressableCache::cachePath_ = "";
bool ContentAddressableCache::readFromCache_ = true;
bool ContentAddressableCache::writeToCache_ = true;

const BW::string & ContentAddressableCache::getCachePath()
{
	return cachePath_;
}

bool ContentAddressableCache::getReadFromCache()
{
	return readFromCache_;
}

bool ContentAddressableCache::getWriteToCache()
{
	return writeToCache_;
}

void ContentAddressableCache::setCachePath( const BW::string & cachePath )
{
	cachePath_ = cachePath;
}

void ContentAddressableCache::setReadFromCache( bool readFromCache )
{
	readFromCache_ = readFromCache;
}

void ContentAddressableCache::setWriteToCache( bool writeToCache )
{
	writeToCache_ = writeToCache;
}

bool ContentAddressableCache::readFromCache( const BW::string & filename, uint64 hash, Compiler & compiler )
{
	if (!readFromCache_ || cachePath_.empty())
	{
		return false;
	}

	const BW::string file = BWResource::getFilename( filename ).to_string();
	const BW::string directory = BWResource::getFilePath( filename );

	// Determine the cache file
	char cacheFilename[MAX_PATH];
	sprintf_s( cacheFilename, MAX_PATH, "%s%hhX/%s.%llX", 
		cachePath_.c_str(), static_cast< uint8 >( hash ), file.c_str(), hash );

	// Check if the file exists in the cache
	if (!BWResource::instance().fileExists( cacheFilename ))
	{
		compiler.onCacheReadMiss( filename );
		return false;
	}

	// Determine a temporary file name
	char tempFilename[MAX_PATH];
	if (!GetTempFileNameA( directory.c_str(), file.c_str(), 0, tempFilename ))
	{
		compiler.onCacheReadMiss( filename );
		return false;
	}

	// Copy from the cache file to the temporary file
	if (!BWResource::instance().fileSystem()->copyFileOrDirectory( cacheFilename, tempFilename ))
	{
		// Erase the temporary file
		BWResource::instance().fileSystem()->eraseFileOrDirectory( tempFilename );
		compiler.onCacheReadMiss( filename );
		return false;
	}

	// Move the temporary file to the actual file
	if (!BWResource::instance().fileSystem()->moveFileOrDirectory( tempFilename, filename ))
	{
		// Erase the temporary file
		BWResource::instance().fileSystem()->eraseFileOrDirectory( tempFilename );
		compiler.onCacheReadMiss( filename );
		return false;
	}

	compiler.onCacheRead( filename );
	return true;
}

bool ContentAddressableCache::writeToCache( const BW::string & filename, uint64 hash, Compiler & compiler )
{
	if (!writeToCache_ || cachePath_.empty())
	{
		return false;
	}

	const BW::string file = BWResource::getFilename( filename ).to_string();

	// Create a temporary file name on the cache
	char tempCacheFileName[MAX_PATH];
	if (!GetTempFileNameA( cachePath_.c_str(), file.c_str(), 0, tempCacheFileName ))
	{
		compiler.onCacheWriteMiss( filename );
		return false;
	}

	// Copy the file to the temporary cache file
	if (!BWResource::instance().fileSystem()->copyFileOrDirectory( filename, tempCacheFileName ))
	{
		// Erase the temporary file
		BWResource::instance().fileSystem()->eraseFileOrDirectory( tempCacheFileName );
		compiler.onCacheWriteMiss( filename );
		return false;
	}

	// Determine the actual cache file name
	char cacheFilename[MAX_PATH];
	sprintf_s( cacheFilename, MAX_PATH, "%s%hhX/%s.%llX", 
		cachePath_.c_str(), static_cast< uint8 >( hash ), file.c_str(), hash );

	// Make sure the folder we wish to copy to on the cache exists.
	BWResource::ensureAbsolutePathExists( cacheFilename );

	// Move the temporary cache file to the actual cache file
	if (!BWResource::instance().fileSystem()->moveFileOrDirectory( tempCacheFileName, cacheFilename ))
	{
		// Erase the temporary file
		BWResource::instance().fileSystem()->eraseFileOrDirectory( tempCacheFileName );
		compiler.onCacheWriteMiss( filename );
		return false;
	}

	compiler.onCacheWrite( filename );
	return true;
}

BW_END_NAMESPACE
