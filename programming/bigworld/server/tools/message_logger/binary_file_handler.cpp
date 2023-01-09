#include "binary_file_handler.hpp"


BW_BEGIN_NAMESPACE

/**
 * Constructor.
 */
BinaryFileHandler::BinaryFileHandler() :
	pFile_( NULL )
{ }


/**
 * Destructor.
 */
BinaryFileHandler::~BinaryFileHandler()
{
	if (pFile_)
	{
		delete pFile_;
	}
}


/**
 * Initialises the BinaryFileHandler with a new FileStream.
 */
bool BinaryFileHandler::init( const char *path, const char *mode )
{
	if ( pFile_ != NULL )
	{
		ERROR_MSG( "BinaryFileHandler::init: Already initialised.\n" );
		return false;
	}

	pFile_ = new FileStream( path, mode );
	if (pFile_->error())
	{
		ERROR_MSG( "BinaryFileHandler::init: "
			"Unable to open '%s' in mode %s: %s\n",
			path, mode, pFile_->strerror() );
		return false;
	}

	return this->FileHandler::init( path, mode );
}


/**
 * Returns the length of the currently open file.
 *
 * @returns Current file length on success, -1 on error.
 */
long BinaryFileHandler::length()
{
	if (pFile_ == NULL)
	{
		return -1;
	}

	return pFile_->length();
}

BW_END_NAMESPACE

// binary_file_handler.cpp
