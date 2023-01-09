#include "bsp_conversion_rule.hpp"
#include "cstdmf/bw_string_ref.hpp"
#include "cstdmf/guard.hpp"
#include "resmgr/bwresource.hpp"

#include "asset_pipeline/converters/bsp_converter/bsp_converter.hpp"
#include "physics2/bsp.hpp"

BW_BEGIN_NAMESPACE

bool BSPConversionRule::createRootTask( const BW::StringRef& sourceFile,
									    ConversionTask& task )
{
	BW_GUARD;

	if (!isOutOfDateBSP( sourceFile ))
	{
		// BSP doesn't need conversion
		return false;
	}

	// Create a task using the BSPConverter
	task.converterId_ = BSPConverter::getTypeId();
	task.converterVersion_ = BSPConverter::getVersion();
	task.converterParams_ = "";
	return true;
}

bool BSPConversionRule::getSourceFile( const BW::StringRef& file,
									   BW::string& sourcefile ) const
{
	BW_GUARD;

	if (!isOutOfDateBSP( file ))
	{
		// BSP is already up to date - it doesnt have a source file.
		return false;
	}

	// The source file for an out of date BSP is itself
	sourcefile = BWResource::resolveFilename( file );
	return true;
}

bool BSPConversionRule::isOutOfDateBSP( const BW::StringRef & resourceID ) const
{
	BinaryPtr bp;

	BW::StringRef ext = BWResource::getExtension( resourceID );
	if (ext == "primitives")
	{
		DataSectionPtr primitive = BWResource::openSection( resourceID );
		if (!primitive)
		{
			return false;
		}

		bp = primitive->readBinary( "bsp2" );
	}
	else if (ext == "bsp2" )
	{
		DataSectionPtr bsp = BWResource::openSection( resourceID );
		if (!bsp)
		{
			return false;
		}

		bp = bsp->asBinary();
	}

	if (!bp)
	{
		return false;
	}

	// BSP is out of date if its version is not the currently supported version
	return BSPTreeTool::getBSPVersion(bp) != BSPTreeTool::getCurrentBSPVersion();
}

BW_END_NAMESPACE
