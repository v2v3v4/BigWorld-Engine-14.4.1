#include "pch.hpp"
#include "pathed_filename.hpp"

#include "resmgr/multi_file_system.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: PathedFilename
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
PathedFilename::PathedFilename() : base_( BASE_EXE_PATH )
{
}

/**
 *	Constructor. Takes an explicit filename and path base type.
 */
PathedFilename::PathedFilename( const BW::string& filename, PathBase base ) :
	filename_( filename ),
	base_( base )
{
}

/**
 *	Constructor. Reads the filename and path base from the given data section.
 */
PathedFilename::PathedFilename( DataSectionPtr ds, 
				const BW::string& defaultFilename, 
				PathBase defaultBase ) :
	filename_( defaultFilename ), 
	base_( defaultBase )
{
	this->init( ds, defaultFilename, defaultBase );
}

/**
 *	Reads the filename and path base from the given data section.
 *	Uses the supplied defaults if the settings cannot be read.
 */
void PathedFilename::init( DataSectionPtr ds, 
				const BW::string& defaultFilename, 
				PathBase defaultBase )
{
	filename_ = defaultFilename;
	base_ = defaultBase;

	if (ds)
	{
		filename_ = ds->asString( defaultFilename );

		BW::string baseStr = ds->readString( "pathBase", "" );

		if (baseStr == "APP_DATA")
		{
			base_ = BASE_APP_DATA;
		}
		else if (baseStr == "LOCAL_APP_DATA")
		{
			base_ = BASE_LOCAL_APP_DATA;
		}
		else if (baseStr == "ROAMING_APP_DATA")
		{
			base_ = BASE_ROAMING_APP_DATA;
		}
		else if (baseStr == "MY_DOCS")
		{
			base_ = BASE_MY_DOCS;
		}
		else if (baseStr == "MY_PICTURES")
		{
			base_ = BASE_MY_PICTURES;
		}
		else if (baseStr == "CWD")
		{
			base_ = BASE_CWD;
		}
		else if (baseStr == "EXE_PATH")
		{
			base_ = BASE_EXE_PATH;
		}
		else if (baseStr == "RES_TREE")
		{
			base_ = BASE_RES_TREE;
		}
		else
		{
			ERROR_MSG( "PathedFilename::init: unknown base path type '%s'.\n", 
				baseStr.c_str() );
		}
	}
}

/**
 *	Returns the full absolute path based on the filename and
 *	path base type. The path base is calculated on demand (i.e.
 *	it isn't cached). This means the more dynamic path base types
 *	(i.e. CWD) may change between calls to this function.
 */
BW::string PathedFilename::resolveName() const
{
	BW::string ret;
	if (BWResource::pathIsRelative( filename_ ))
	{
		ret = PathedFilename::resolveBase( base_ ) + filename_;
	}
	else
	{
		ret = filename_;
	}

	return BWUtil::normalisePath( ret );
}

/**
 *	Static helper which calculates the fully qualified base path
 *	name based on the given PathBase type.
 */
BW::string PathedFilename::resolveBase( PathBase baseType )
{
	switch( baseType )
	{
	case BASE_ROAMING_APP_DATA:	return BWResource::appDataDirectory( true );
	case BASE_APP_DATA:			return BWResource::appDataDirectory( true );
	case BASE_LOCAL_APP_DATA:	return BWResource::appDataDirectory( false );
	case BASE_MY_PICTURES:		return BWResource::userPicturesDirectory();
	case BASE_MY_DOCS:			return BWResource::userDocumentsDirectory();
	case BASE_CWD:				return BWResource::getCurrentDirectory();
	case BASE_EXE_PATH:			return BWResource::appDirectory();
	case BASE_RES_TREE:
		{
			MultiFileSystemPtr pFS = BWResource::instance().fileSystem();
			if (!pFS)
			{
				return "";
			}

			BW::string resolvedPath = "";
			pFS->resolveToAbsolutePath( resolvedPath );
			return resolvedPath;
		}
	}

	return "";
}

BW_END_NAMESPACE

// pathed_filename.cpp
