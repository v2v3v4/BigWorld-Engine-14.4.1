#include "hierarchical_config_converter.hpp"

#include "asset_pipeline/compiler/compiler.hpp"
#include "asset_pipeline/dependency/dependency_list.hpp"
#include "cstdmf/bw_hash64.hpp"
#include "cstdmf/bw_string_ref.hpp"
#include "cstdmf/guard.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/datasection.hpp"
#include "resmgr/hierarchical_config.hpp"


BW_BEGIN_NAMESPACE

const uint64 HierarchicalConfigConverter::s_TypeId = Hash64::compute( HierarchicalConfigConverter::getTypeName() );
const char * HierarchicalConfigConverter::s_Version = "2.8.0";

/* construct a converter with parameters. */
HierarchicalConfigConverter::HierarchicalConfigConverter( const BW::string& params )
	: Converter( params )
{
	typedef BW::vector< BW::StringRef > StringArray;
	StringArray parameters;
	bw_tokenise( StringRef( params_ ), " ", parameters );
	for (size_t i = 0; i < parameters.size(); ++i)
	{
		if (i + 1 < parameters.size())
		{
			if (parameters[i] == "--file" ||
				parameters[i] == "-f")
			{
				configFile_ = parameters[++i];
			}
			else if (parameters[i] == "--directory" ||
				parameters[i] == "-d")
			{
				configDirectory_ = parameters[++i];
			}
			else if (parameters[i] == "--output" ||
				parameters[i] == "-o")
			{
				outputFile_ = parameters[++i];
			}
		}
	}
}

HierarchicalConfigConverter::~HierarchicalConfigConverter()
{
}

bool HierarchicalConfigConverter::createDependencies( const BW::string & sourcefile, 
												      const Compiler & compiler,
												      DependencyList & dependencies )
{
	BW_GUARD;

	if (configFile_.empty())
	{
		ERROR_MSG( "No config file specified." );
		return false;
	}

	if (outputFile_.empty())
	{
		ERROR_MSG( "No output file specified." );
		return false;
	}

	dependencies.addSecondaryDirectoryDependency( configDirectory_.to_string(), 
		configFile_.to_string(), false, true, true );
	return true;
}

bool HierarchicalConfigConverter::convert( const BW::string & sourcefile,
									       const Compiler & compiler,
									       BW::vector< BW::string > & intermediateFiles,
									       BW::vector< BW::string > & outputFiles )
{
	BW_GUARD;

	if (configFile_.empty())
	{
		ERROR_MSG( "No config file specified." );
		return false;
	}

	if (outputFile_.empty())
	{
		ERROR_MSG( "No output file specified." );
		return false;
	}

	BWResource::instance().purgeAll();

	BW::string outputfile = outputFile_.to_string();
	compiler.resolveOutputPath( outputfile );
	BWResource::ensureAbsolutePathExists( outputfile );

	HierarchicalConfig config( configFile_, "" );
	if (compiler.hasError())
	{
		return false;
	}

	if (!config.getRoot()->save( outputfile ))
	{
		ERROR_MSG( "Could not save output file: %s", outputfile.c_str() );
		return false;
	}

	// Add it to the list of generated files
	outputFiles.push_back( outputfile );
	return true;
}

BW_END_NAMESPACE
