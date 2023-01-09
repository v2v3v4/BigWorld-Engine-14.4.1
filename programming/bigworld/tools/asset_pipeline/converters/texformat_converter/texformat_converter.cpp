#include "texformat_converter.hpp"

#include "asset_pipeline/compiler/compiler.hpp"
#include "asset_pipeline/dependency/dependency_list.hpp"
#include "cstdmf/bw_hash64.hpp"
#include "cstdmf/bw_string_ref.hpp"
#include "cstdmf/guard.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/datasection.hpp"
#include "moo/texture_detail_level.hpp"


BW_BEGIN_NAMESPACE

const uint64 TexFormatConverter::s_TypeId = Hash64::compute( TexFormatConverter::getTypeName() );
#define TO_STRING(X) #X
const char * TexFormatConverter::s_Version = "2.7v" TO_STRING(TEXFORMAT_CONVERTER_VERSION);
#undef TO_STRING

/* construct a converter with parameters. */
TexFormatConverter::TexFormatConverter( const BW::string& params )
	: Converter( params )
{
}

TexFormatConverter::~TexFormatConverter()
{
}

bool TexFormatConverter::createDependencies( const BW::string & sourcefile, 
									   const Compiler & compiler,
									   DependencyList & dependencies )
{
	return true;
}

bool TexFormatConverter::convert( const BW::string & sourcefile,
						    const Compiler & compiler,
							BW::vector< BW::string > & intermediateFiles,
							BW::vector< BW::string > & outputFiles )
{
	BW_GUARD;

	// Resolve the path we wish to save this bsp file to
	BW::string outputfile = sourcefile;
	compiler.resolveOutputPath( outputfile );
	BWResource::ensureAbsolutePathExists( outputfile );

	// Read in the source texture format file
	DataSectionPtr pSection = BWResource::instance().rootSection()->openSection( sourcefile );
	if (pSection == NULL)
	{
		ERROR_MSG( "Could not open source file.\n" );
		return false;
	}

	Moo::TextureDetailLevel detailLevel;
	detailLevel.init( pSection );
	detailLevel.write( pSection );
	
	// Save out the texture format file
	if ( !pSection->save( outputfile ) )
	{
		ERROR_MSG( "Could not save output file.\n" );
		return false;
	}

	// Add it to the list of generated files
	outputFiles.push_back( outputfile );
	return true;
}

BW_END_NAMESPACE
