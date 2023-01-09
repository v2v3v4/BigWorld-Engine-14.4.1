#include "pch.hpp"

#include "resource_file_path.hpp"

#include "bwresource.hpp"

BW_BEGIN_NAMESPACE


/**
 *	Static method to create a BackgroundFile object for a resource path that
 *	requires resolution via BWResource::resolveToAbsolutePath().
 *
 *	@param resourcePath 	The resource path to resolve.
 */
BackgroundFilePathPtr ResourceFilePath::create(
		const BW::string & resourcePath )
{
	return new ResourceFilePath( resourcePath );
}


/**
 *	Constructor.
 *
 *	@param resourcePath 	The resource path to resolve.
 */
ResourceFilePath::ResourceFilePath( const BW::string & resourcePath ) :
	BackgroundFilePath( resourcePath )
{
}



/*
 *	Override from BackgroundFilePath.
 */
bool ResourceFilePath::resolve()
{
	// Since the path can point to an as-yet non-existent file path, we resolve
	// the directory it's supposed to be in first, and recombine the resolved
	// directory path with the filename base.

	BW::string dirPath = BWResource::getFilePath( path_ );

	if (IFileSystem::FT_NOT_FOUND ==
			BWResource::resolveToAbsolutePath( dirPath ))
	{
		ERROR_MSG( "BackgroundFileWriterTask::openFile: "
				"Could not resolve directory of output path: %s\n",
			path_.c_str() );

		return false;
	}

	path_ = dirPath + "/" + BWResource::getFilename( path_ );

	return true;
}


BW_END_NAMESPACE


// resource_file_path.cpp

