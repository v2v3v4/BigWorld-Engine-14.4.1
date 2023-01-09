#ifndef ASSET_PIPELINE_CONTENT_ADDRESSABLE_CACHE
#define ASSET_PIPELINE_CONTENT_ADDRESSABLE_CACHE

#include "cstdmf/bw_namespace.hpp"
#include "cstdmf/bw_string.hpp"
#include "cstdmf/stdmf.hpp"

BW_BEGIN_NAMESPACE

class Compiler;

/// \brief Content Addressable Cache
///
/// A class for interfacing with a file cache that stores files by their hash
class ContentAddressableCache
{
public:
	static const BW::string & getCachePath();
	static bool getReadFromCache();
	static bool getWriteToCache();
	static void setCachePath( const BW::string & cachePath );
	static void setReadFromCache( bool readFromCache );
	static void setWriteToCache( bool writeToCache );
	static bool readFromCache( const BW::string & filename, uint64 hash, Compiler & compiler );
	static bool writeToCache( const BW::string & filename, uint64 hash, Compiler & compiler );

private:
	static BW::string cachePath_;
	static bool readFromCache_;
	static bool writeToCache_;
};

BW_END_NAMESPACE

#endif // ASSET_PIPELINE_CONTENT_ADDRESSABLE_CACHE
