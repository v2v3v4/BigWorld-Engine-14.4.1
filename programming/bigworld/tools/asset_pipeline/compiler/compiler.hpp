#ifndef ASSET_PIPELINE_COMPILER
#define ASSET_PIPELINE_COMPILER

#include "cstdmf/bw_namespace.hpp"
#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_string_ref.hpp"
#include "cstdmf/bw_vector.hpp"

#define ASSET_PIPELINE_CATEGORY "AssetPipeline"

BW_BEGIN_NAMESPACE

class ConversionRule;
struct ConversionTask;
struct ConverterInfo;
class Dependency;
class DependencyList;
struct ResourceCallbacks;

/// \brief Compiler interface
///
/// Compiler interface for handling asset pipeline callbacks.
/// Do not inherit from this class. Inherit from AssetCompiler instead.
class Compiler
{
private:
	// Only AssetCompiler is allowed to inherit this class
	friend class AssetCompiler;

	Compiler() {}
	virtual ~Compiler() {}

public:
	/// Register conversion rules that will be used by the asset pipeline to discover tasks. 
	/// \param conversionRule the conversion rule to register.
	virtual void registerConversionRule( ConversionRule & conversionRule ) = 0;
	/// Register converters that will be used by the asset pipeline to convert tasks. 
	/// \param converterInfo struct containing the details of the converter to register.
	virtual void registerConverter( ConverterInfo & converterInfo ) = 0;
	/// Register an object to receive callbacks on resource events 
	/// \param ResourceCallbacks struct 
	virtual void registerResourceCallbacks( ResourceCallbacks & resourceCallbacks ) = 0;

	/// Get all of the resource paths from the asset pipeline
	/// \return Vector of paths to add to BWResource
	virtual const BW::vector< BW::string > & getResourcePaths() const = 0;

	/// Ask the asset pipeline to make sure a dependency up to date.
	/// \param dependency the dependency to make up to date.
	/// \param conversionTask an optional conversion task that will return the task associated with this dependency.
	/// \return true if the dependency is up to date at the time of the function returning
	///		  false if the dependency is not yet up to date at the time of the function returning,
	///        or if there was an error processing the dependency.
	virtual bool ensureUpToDate( const Dependency & dependency, const ConversionTask *& conversionTask ) = 0;
	/// Ask the asset pipeline for the source of an file.
	/// \param file the file to find the source for.
	/// \param source output set to true if an error was encountered processing the dependency.
	/// \return true if the source for the file could be found.
	virtual bool getSourceFile( const BW::StringRef & file, BW::string & sourceFile ) = 0;
	/// Get a hash of a dependency.
	/// \param dependency the dependency to hash.
	/// \return the hash of the dependency.
	virtual uint64 getHash( const Dependency & dependency ) = 0;
	/// Get a hash of a file on disk.
	/// \param fileName the name of the file on disk to hash.
	/// \param forceUpdate if false, allow the asset pipeline to return a cached value of the hash
	/// \return the hash of the file.
	virtual uint64 getFileHash( const BW::string & fileName, bool forceUpdate ) = 0;
	/// Check if the stored hash of a file is out of date and update it if it is.
	/// \param fileName the name of the file on disk to hash.
	/// \return true if the stored hash is out of date.
	virtual bool checkFileHashUpToDate( const BW::string & fileName ) = 0;
	/// Get a hash of a directory on disk.
	/// \param directory the name of the directory on disk to hash.
	/// \param pattern the pattern to match files in the directory
	/// \param regex specify whether the pattern provided is a regex pattern
	/// \param recursive specify whether the directory should be recursively hashed
	/// \return the hash of the file.
	virtual uint64 getDirectoryHash( const BW::string & directory, const BW::string & pattern, bool regex, bool recursive ) = 0;

	/// Notify the asset pipeline that an error has occurred.
	virtual void setError() = 0;
	/// Notify the asset pipeline that a warning has occurred.
	virtual void setWarning() = 0;
	/// Clear any error notification on the asset pipeline.
	virtual void resetErrorFlags() = 0;
	/// Check if an error has been reported on the asset pipeline.
	/// \return true if an error has been reported.
	virtual bool hasError() const = 0;
	/// Check if a warning has been reported on the asset pipeline.
	/// \return true if a warning has been reported.
	virtual bool hasWarning() const = 0;

	/// Callback when the asset pipeline attempts to iterate a file searching for tasks. 
	/// \param file the file iterated.
	/// \return true if this file should be iterated
	virtual bool shouldIterateFile( const BW::StringRef & file ) = 0;
	/// Callback when the asset pipeline attempts to iterate a directory searching for tasks. 
	/// \param directory the directory iterated.
	/// \return true if this directory should be iterated
	virtual bool shouldIterateDirectory( const BW::StringRef & directory ) = 0;

	/// Check if the asset pipeline has any unprocessed tasks.
	/// \return true if any unprocessed tasks remain.
	virtual bool hasTasks() = 0;
	/// Get the next unprocessed task.
	/// \return the next unprocessed task
	///        null if there are no unprocessed tasks remaining.
	virtual ConversionTask * getNextTask() = 0;
	/// Queue a task in the asset pipeline for processing.
	/// \param conversionTask the task to queue.
	virtual void queueTask( ConversionTask & conversionTask ) = 0;

	/// Callback when the asset pipeline starts processing of a task. 
	/// \param conversionTask the task started.
	virtual void onTaskStarted( ConversionTask & conversionTask ) = 0;
	/// Callback when the asset pipeline resumes processing of a task. 
	/// \param conversionTask the task resumed.
	virtual void onTaskResumed( ConversionTask & conversionTask ) = 0;
	/// Callback when the asset pipeline suspends processing of a task. 
	/// \param conversionTask the task suspended.
	virtual void onTaskSuspended( ConversionTask & conversionTask ) = 0;
	/// Callback when the asset pipeline completes processing of a task. 
	/// \param conversionTask the task completed.
	virtual void onTaskCompleted( ConversionTask & conversionTask ) = 0;

	/// Callback before the asset pipeline starts creating the dependencies of a task. 
	/// \param conversionTask the task the dependencies will be created for.
	virtual void onPreCreateDependencies( ConversionTask & conversionTask ) = 0;
	/// Callback after the asset pipeline finishes creating the dependencies of a task. 
	/// \param conversionTask the task the dependencies were created for.
	virtual void onPostCreateDependencies( ConversionTask & conversionTask ) = 0;

	/// Callback before the asset pipeline starts converting a task. 
	/// \param conversionTask the task that will be converted.
	virtual void onPreConvert( ConversionTask & conversionTask ) = 0;
	/// Callback after the asset pipeline finishes converting a task. 
	/// \param conversionTask the task that was converted.
	virtual void onPostConvert( ConversionTask & conversionTask ) = 0;

	/// Callback when the asset pipeline generates an output file. 
	/// \param filename the name of the file generated.
	virtual void onOutputGenerated( const BW::string & filename ) = 0;

	/// Callback when the asset pipeline successfully reads a file from the cache. 
	/// \param filename the name of the file read.
	virtual void onCacheRead( const BW::string & filename ) = 0;
	/// Callback when the asset pipeline unsuccessfully reads a file from the cache. 
	/// \param filename the name of the file attempted to be read.
	virtual void onCacheReadMiss( const BW::string & filename ) = 0;
	/// Callback when the asset pipeline successfully writes a file to the cache. 
	/// \param filename the name of the file written.
	virtual void onCacheWrite( const BW::string & filename ) = 0;
	/// Callback when the asset pipeline unsuccessfully writes a file to the cache. 
	/// \param filename the name of the file attempted to be written.
	virtual void onCacheWriteMiss( const BW::string & filename ) = 0;

	// convert the path to a relative path
	virtual bool resolveRelativePath( BW::string & path ) const = 0;
	// convert the path to an absolute source path
	virtual bool resolveSourcePath( BW::string & path ) const = 0;
	// convert the path to an absolute intermediate path
	virtual bool resolveIntermediatePath( BW::string & path ) const = 0;
	// convert the path to an absolute output path
	virtual bool resolveOutputPath( BW::string & path ) const = 0;
};

BW_END_NAMESPACE

#endif // ASSET_PIPELINE_COMPILER
