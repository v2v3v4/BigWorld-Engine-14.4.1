#ifndef ASSET_COMPILER_OPTIONS
#define ASSET_COMPILER_OPTIONS

#include "cstdmf/smartpointer.hpp"
#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_namespace.hpp"

BW_BEGIN_NAMESPACE

class AssetCompiler;
class DataSection;
typedef SmartPointer<DataSection> DataSectionPtr;

class AssetCompilerOptions
{
public:
	AssetCompilerOptions();
	explicit AssetCompilerOptions( const AssetCompiler & compiler );

	virtual bool readFromDataSection( const DataSectionPtr & dataSection );
	virtual bool writeToDataSection( DataSectionPtr & dataSection ) const;

	virtual bool parseCommandLine( const BW::string & commandString );

	virtual bool apply( AssetCompiler & compiler ) const;

	// Accessors
	const BW::string & cachePath() const;
	void cachePath( const BW::string & path );

	const BW::string & intermediatePath() const;
	void intermediatePath( const BW::string & path );

	const BW::string & outputPath() const;
	void outputPath( const BW::string & path );

	int threadCount() const;
	void threadCount( int numThreads );

	bool forceRebuild() const;
	void forceRebuild( bool rebuild );

	bool recursive() const;
	void recursive( bool recurse );

	bool enableCacheRead() const;
	void enableCacheRead( bool enable );

	bool enableCacheWrite() const;
	void enableCacheWrite( bool enable );

protected:

	bool sanitisePath( const BW::string & path, BW::string & cleanPath );

protected:
	BW::string cachePath_;
	BW::string intermediatePath_;
	BW::string outputPath_;
	int numThreads_;
	bool forceRebuild_;
	bool recursive_;
	
	bool enableCacheRead_;
	bool enableCacheWrite_;

	mutable bool cachePathChanged_;
	mutable bool intermediatePathChanged_;
	mutable bool outputPathChanged_;
	mutable bool numThreadsChanged_;
};

BW_END_NAMESPACE

#endif // ASSET_COMPILER_OPTIONS
