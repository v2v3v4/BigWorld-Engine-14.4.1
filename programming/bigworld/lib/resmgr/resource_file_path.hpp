#ifndef RESOURCE_FILE_PATH_HPP
#define RESOURCE_FILE_PATH_HPP

#include "cstdmf/background_file_writer.hpp"
#include "cstdmf/bw_string.hpp"

BW_BEGIN_NAMESPACE


/**
 *	This class is used to supply resource paths to BackgroundFileWriter that
 *	require file system resolution in the background thread before the file is
 *	opened.
 */
class ResourceFilePath : public BackgroundFilePath
{
public:
	static BackgroundFilePathPtr create( const BW::string & resourcePath );

	/** Destructor. */
	virtual ~ResourceFilePath() {}

	// Override from BackgroundFilePath.
	virtual bool resolve();


private:
	ResourceFilePath( const BW::string & resourcePath );
};

BW_END_NAMESPACE

#endif // RESOURCE_FILE_PATH_HPP
