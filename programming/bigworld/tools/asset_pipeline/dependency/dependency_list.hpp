#ifndef ASSET_PIPELINE_DEPENDENCY_LIST
#define ASSET_PIPELINE_DEPENDENCY_LIST

#include "cstdmf/bw_namespace.hpp"
#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_unordered_map.hpp"
#include "cstdmf/bw_vector.hpp"
#include "resmgr/datasection.hpp"

BW_BEGIN_NAMESPACE

class Compiler;
class Dependency;

/// Container for a number of input dependencies and the files they build into.
class DependencyList
{
public:
	typedef std::pair< Dependency*, uint64 > Input;
	typedef std::pair< BW::string, uint64 > Output;

public:
	DependencyList( Compiler & compiler );
	~DependencyList();

	const BW::vector< Input > & primaryInputs() const { return primaryInputs_; }
	const BW::vector< Input > & secondaryInputs() const { return secondaryInputs_; }
	const BW::vector< Output > & intermediateOutputs() const { return intermediateOutputs_; }
	const BW::vector< Output > & outputs() const { return outputs_; }

	/// Reset the dependency list. Clears all input dependencies and outputs.
	void reset();
	/// Reset the dependency list and initialise its primary dependencies.
	/// \param source the primary source of this dependency list.
	/// \param converterId the id of the converter used to convert the source.
	/// \param converterVersion the version number of the converter.
	/// \param converterParams the parameters used to construct the converter.
	/// \param compiler the compiler interface to register callbacks on.
	void initialise( const BW::string & source,
				     uint64 converterId,
				     const BW::string & converterVersion,
				     const BW::string & converterParams );

	/// Add a primary source file dependency.
	/// \param filename the name of the file to depend on.
	void addPrimarySourceFileDependency( const BW::string & filename );
	/// Add a primary converter dependency.
	/// \param converterId the id of the converter to depend on.
	/// \param converterVersion the version of the converter to depend on.
	void addPrimaryConverterDependency( uint64 converterId, const BW::string & converterVersion );
	/// Add a primary converter params dependency.
	/// \param converterParams the parameter string to depend on.
	void addPrimaryConverterParamsDependency( const BW::string & converterParams );

	/// Add a secondary source file dependency.
	/// \param filename the name of the file to depend on.
	/// \param critical true if this dependency is a critical dependency.
	void addSecondarySourceFileDependency( const BW::string & filename, bool critical );
	/// Add a secondary intermediate file dependency.
	/// \param filename the name of the compiled file to depend on.
	/// \param critical true if this dependency is a critical dependency.
	void addSecondaryIntermediateFileDependency( const BW::string & filename, bool critical );
	/// Add a secondary output file dependency.
	/// \param filename the name of the compiled file to depend on.
	/// \param critical true if this dependency is a critical dependency.
	void addSecondaryOutputFileDependency( const BW::string & filename, bool critical );
	/// Add a secondary directory dependency.
	/// \param directory the name of the directory to depend on.
	/// \param pattern the pattern to match files within the directory.
	/// \param recursive should we depend on sub directories of the directory.
	/// \param critical true if this dependency is a critical dependency.
	void addSecondaryDirectoryDependency( const BW::string & directory, 
		const BW::string & pattern, bool regex, bool recursive, bool critical );

	/// Get the combined hash of the inputs
	/// \param includeSecondary should the secondary inputs be included in the combined hash
	uint64 getInputHash( bool includeSecondary );

	/// Serialise the dependency list from a data section.
	/// \param pSection the data section to serialise in from.
	bool serialiseIn( DataSectionPtr pSection );
	/// Serialise the dependency list to a data section.
	/// \param pSection the data section to serialise out to.
	bool serialiseOut( DataSectionPtr pSection ) const;

private:
	/// Helper function for serialising in a vector of dependencies from a data section.
	/// \param pSection the data section to serialise in from.
	/// \param dependencies the vector to store the serialised in dependencies.
	bool serialiseIn( DataSectionPtr pSection, BW::vector< Input > & dependencies );
	/// Helper function for serialising out a vector of dependencies to a data section.
	/// \param pSection the data section to serialise out to.
	/// \param dependencies the vector of dependencies to serialise out.
	bool serialiseOut( DataSectionPtr pSection, const BW::vector< Input > & dependencies ) const;
	/// Helper function for serialising in a vector of outputs from a data section.
	/// \param pSection the data section to serialise in from.
	/// \param outputs the vector to store the serialised in outputs.
	bool serialiseIn( DataSectionPtr pSection, BW::vector< Output > & outputs );
	/// Helper function for serialising out a vector of outputs to a data section.
	/// \param pSection the data section to serialise out to.
	/// \param outputs the vector of outputs to serialise out.
	bool serialiseOut( DataSectionPtr pSection, const BW::vector< Output > & outputs ) const;

private:
	Compiler & compiler_;
	BW::vector< Input > primaryInputs_;
	BW::vector< Input > secondaryInputs_;
	BW::vector< Output > intermediateOutputs_;
	BW::vector< Output > outputs_;

	friend class TaskProcessor;
};

BW_END_NAMESPACE

#endif //ASSET_PIPELINE_DEPENDENCY_LIST
