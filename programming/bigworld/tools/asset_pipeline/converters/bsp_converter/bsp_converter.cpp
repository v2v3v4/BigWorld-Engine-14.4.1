#include "bsp_converter.hpp"

#include "asset_pipeline/compiler/compiler.hpp"
#include "asset_pipeline/dependency/dependency_list.hpp"
#include "cstdmf/bw_hash64.hpp"
#include "cstdmf/bw_string_ref.hpp"
#include "cstdmf/guard.hpp"
#include "physics2/bsp.hpp"
#include "resmgr/bwresource.hpp"


BW_BEGIN_NAMESPACE

const uint64 BSPConverter::s_TypeId = Hash64::compute( BSPConverter::getTypeName() );
const char * BSPConverter::s_Version = "2.7.1";

/* construct a converter with parameters. */
BSPConverter::BSPConverter( const BW::string& params )
	: Converter( params )
{
}

BSPConverter::~BSPConverter()
{
}

bool BSPConverter::createDependencies( const BW::string & sourcefile, 
									   const Compiler & compiler,
									   DependencyList & dependencies )
{
	return true;
}

bool BSPConverter::convert( const BW::string & sourcefile,
						    const Compiler & compiler,
							BW::vector< BW::string > & intermediateFiles,
							BW::vector< BW::string > & outputFiles )
{
	BW_GUARD;

	// Resolve the path we wish to save this bsp file to
	BW::string outputfile = sourcefile;
	compiler.resolveOutputPath( outputfile );
	BWResource::ensureAbsolutePathExists( outputfile );

	BW::StringRef ext = BWResource::getExtension( sourcefile );
	if (ext == "primitives")
	{
		if (!convertBSPInPrimitives( sourcefile, outputfile ))
		{
			ERROR_MSG("convertBSPInPrimitives failed\n");
			return false;
		}
	}
	else if (ext == "bsp2" )
	{
		if (!convertBSPFile( sourcefile, outputfile ))
		{
			ERROR_MSG("convertBSPFile failed\n");
			return false;
		}
	}
	else
	{
		ERROR_MSG( "Unsupported file extension %s\n", ext.to_string().c_str() );
	}

	// Add it to the list of generated files
	outputFiles.push_back( outputfile );
	return true;
}

bool BSPConverter::convertBSPInPrimitives( const BW::string sourcefile, const BW::string & outputfile )
{
	DataSectionPtr primDS = BWResource::openSection( sourcefile );
	if (!primDS)
	{
		ERROR_MSG( "Unable to open primitives %s\n", sourcefile.c_str() );
		return false;
	}

	BinaryPtr bp = primDS->readBinary( "bsp2" );
	if (!bp)
	{
		ERROR_MSG( "Primitives does not contain bsp section %s\n", sourcefile.c_str() );
		return false;
	}

	BSPTree* pBSP = BSPTreeTool::loadBSP( bp );
	if (!pBSP)
	{
		ERROR_MSG( "Could not load bsp from primitives %s\n", sourcefile.c_str() );
		return false;
	}

	bp = BSPTreeTool::saveBSPInMemory( pBSP );
	bw_safe_delete( pBSP );
	MF_ASSERT( bp );
	bool bRes = primDS->writeBinary( "bsp2", bp );
	if (!bRes)
	{
		ERROR_MSG( "Failed to write bsp to primitives %s\n", outputfile.c_str() );
		return false;
	}

	bRes = primDS->save( outputfile );
	if (!bRes)
	{
		ERROR_MSG( "Failed to save primitives %s\n", outputfile.c_str() );
		return false;
	}

	return true;
}

bool BSPConverter::convertBSPFile( const BW::string sourcefile, const BW::string & outputfile )
{
	DataSectionPtr bspSect = BWResource::openSection( sourcefile );
	if ( !bspSect.exists() )
	{
		ERROR_MSG( "Could not open file %s\n", sourcefile.c_str() );
		return false;
	}

	BinaryPtr bp = bspSect->asBinary();
	if ( !bp.exists() )
	{
		ERROR_MSG( "Could not load bsp from %s\n", sourcefile.c_str() );
		return false;
	}

	BSPTree* pBSP = BSPTreeTool::loadBSP( bp );
	BSPTreeTool::saveBSPInFile( pBSP, outputfile.c_str() );
	bw_safe_delete( pBSP );
	return true;
}

BW_END_NAMESPACE
