#include "texformat_conversion_rule.hpp"
#include "cstdmf/bw_string_ref.hpp"
#include "cstdmf/guard.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/datasection.hpp"
#include "moo/texture_detail_level.hpp"

#include "asset_pipeline/converters/texformat_converter/texformat_converter.hpp"

BW_BEGIN_NAMESPACE

bool TexFormatConversionRule::createRootTask( const BW::StringRef& sourceFile,
										      ConversionTask& task )
{
	BW_GUARD;

	if (!isOutOfDateTexFormat( sourceFile ))
	{
		// Texformat doesn't need conversion
		return false;
	}

	// Create a task using the TexFormatConverter
	task.converterId_ = TexFormatConverter::getTypeId();
	task.converterVersion_ = TexFormatConverter::getVersion();
	task.converterParams_ = "";
	return true;
}

bool TexFormatConversionRule::getSourceFile( const BW::StringRef& file,
									         BW::string& sourcefile ) const
{
	BW_GUARD;

	if (!isOutOfDateTexFormat( file ))
	{
		// Texformat is already up to date - it doesnt have a source file.
		return false;
	}

	// The source file for an out of date BSP is itself
	sourcefile = BWResource::resolveFilename( file );
	return true;
}

bool TexFormatConversionRule::isOutOfDateTexFormat( const BW::StringRef & resourceID ) const
{
	BW::StringRef ext = BWResource::getExtension( resourceID );
	if (ext != "texformat")
	{
		return false;
	}

	DataSectionPtr pSection = BWResource::instance().rootSection()->openSection( resourceID );
	if (pSection == NULL)
	{
		return true;
	}

	int version = pSection->readInt( "version", 0 );
	return version != TEXTURE_DETAIL_LEVEL_VERSION;
}

BW_END_NAMESPACE
