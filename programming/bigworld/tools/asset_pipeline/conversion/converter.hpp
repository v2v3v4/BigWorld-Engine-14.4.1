#ifndef ASSET_PIPELINE_CONVERTER
#define ASSET_PIPELINE_CONVERTER

#include "cstdmf/bw_namespace.hpp"
#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_vector.hpp"

BW_BEGIN_NAMESPACE

class Compiler;
class DependencyList;

/// \brief Converter interface
///
/// A plugin object to the asset pipeline for generating dependencies of assets
/// and converting assets.
class Converter
{
public:
	/// Constructor
	/// \param params command line parameters for initialising the converter
	Converter( const BW::string& params ) : params_( params ) {};
	virtual ~Converter(){};

	/// builds the dependency list for a source file.
	/// \param sourcefile the name of the source file on disk.
	/// \param dependencies the dependency list to generate.
	/// \return true if the dependency list was successfully generated
	virtual bool createDependencies( const BW::string& sourcefile,
									 const Compiler & compiler,
									 DependencyList & dependencies ) = 0;
	
	/// convert a source file.
	/// \param sourcefile the name of the source file on disk.
	/// \param convertedFiles a list of filenames that were converted from the source file.
	/// \return true if the source file was successfully converted.
	virtual bool convert( const BW::string& sourcefile,
						  const Compiler & compiler,
						  BW::vector< BW::string > & intermediateFiles,
						  BW::vector< BW::string > & outputFiles ) = 0;

protected:
	const BW::string& params_;
};

BW_END_NAMESPACE

#endif //ASSET_PIPELINE_CONVERTER
