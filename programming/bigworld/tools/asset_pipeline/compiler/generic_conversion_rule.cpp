#include "generic_conversion_rule.hpp"

#include "asset_pipeline/conversion/conversion_task.hpp"
#include "resmgr/bwresource.hpp"
#include "re2/re2.h"

BW_BEGIN_NAMESPACE

GenericConversionRule::GenericConversionRule( ConverterMap & converters )
: converters_( converters )
{
}

void GenericConversionRule::load( const StringRef & rules )
{
	rules_.load( rules, "" );
}

bool GenericConversionRule::createRootTask( const BW::StringRef& sourceFile,
										   ConversionTask& task )
{
	return createTask( sourceFile, task, true );
}


bool GenericConversionRule::createTask( const BW::StringRef& sourceFile,
										ConversionTask& task )
{
	return createTask( sourceFile, task, false );
}


bool GenericConversionRule::getSourceFile( const BW::StringRef& file,
										   BW::string& sourcefile ) const
{
	DataSectionPtr rule = rules_.get( file );

	DataSectionPtr sourcePatternSection = rule->findChild( "sourcePattern" );
	DataSectionPtr sourceFormatSection = rule->findChild( "sourceFormat" );
	if (sourcePatternSection == NULL ||
		sourceFormatSection == NULL)
	{
		return false;
	}

	BW::string pattern = sourcePatternSection->asString();
	BW::string format = sourceFormatSection->asString();
	BW::vector< StringRef > formats;
	bw_tokenise( StringRef( format ), "|", formats );

	bool matchfound = false;
	for ( BW::vector< StringRef >::iterator it = formats.begin();
		it != formats.end(); ++it )
	{
		std::string filename( file.data(), file.length() );
		if (!RE2::Replace( &filename, 
						   re2::StringPiece( pattern.c_str() ),
						   re2::StringPiece( it->data(), static_cast< int >( it->length() ) ) ))
		{
			continue;
		}

		BW::string sourcefilename = BWResource::resolveFilename( filename.c_str() );
		if (BWResource::pathIsRelative( sourcefilename ))
		{
			continue;
		}

		sourcefile = sourcefilename;
		if (BWResource::fileExists( sourcefile ))
		{
			// return true for the first pattern that exists on disk
			return true;
		}
		matchfound = true;
	}
	// if none of the patterns exist on disk return the last pattern that matched.
	// this ensures a task is created for a destination asset even if the source is missing.
	return matchfound;
}

bool GenericConversionRule::createTask( const BW::StringRef & sourceFile,
									   ConversionTask & task,
									   bool root )
{
	char relativePath[MAX_PATH];
	bw_str_copy( relativePath, MAX_PATH, sourceFile );
	if (!BWResource::resolveToRelativePathT( relativePath, MAX_PATH ))
	{
		return false;
	}
	DataSectionPtr rule = rules_.get( relativePath );

	if (root)
	{
		DataSectionPtr rootSection = rule->findChild( "root" );
		if (rootSection == NULL ||
			rootSection->asBool( false ) == false )
		{
			// not a root rule
			return false;
		}
	}

	DataSectionPtr noConversionSection = rule->findChild( "noConversion" );
	if (noConversionSection != NULL &&
		noConversionSection->asBool())
	{
		return false;
	}

	DataSectionPtr converterSection = rule->findChild( "converter" );
	DataSectionPtr converterParamsSection = rule->findChild( "converterParams" );
	if (converterSection == NULL)
	{
		return false;
	}

	BW::string converter = converterSection->asString();
	for (ConverterMap::iterator it = converters_.begin();
		it != converters_.end(); ++it)
	{
		if (it->second->name_ != converter)
		{
			continue;
		}

		task.converterId_ = it->second->typeId_;
		task.converterVersion_ = it->second->version_;
		task.converterParams_ = converterParamsSection != NULL ? 
			converterParamsSection->asString() : "";
		return true;
	}

	return false;
}

BW_END_NAMESPACE
