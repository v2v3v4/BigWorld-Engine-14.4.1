#include "test_conversion_rule.hpp"
#include "test_converter.hpp"
#include "asset_pipeline/conversion/conversion_task.hpp"
#include "resmgr/bwresource.hpp"

BW_BEGIN_NAMESPACE

bool TestConversionRule::createTask( const BW::StringRef& sourceFile,
									 ConversionTask& task )
{
	StringRef ext = BWResource::getExtension( sourceFile );
	if (ext == "testasset")
	{
		task.converterId_ = TestConverter::getTypeId();
		task.converterParams_ = "";
		task.converterVersion_ = TestConverter::getVersion();
		return true;
	}

	return false;
}

bool TestConversionRule::getSourceFile( const BW::StringRef& file,
										BW::string& sourcefile ) const
{
	StringRef ext = BWResource::getExtension( file );
	if (ext == "testcompiledasset")
	{
		BW::string fileName = BWResource::changeExtension( file, ".testasset" );
		if ( BWResource::fileExists( fileName ) )
		{
			sourcefile = BWResource::resolveFilename( fileName );
			return true;
		}
	}

	return false;
}

BW_END_NAMESPACE
