#include "file_handler.hpp"

#include "cstdmf/stdmf.hpp"


BW_BEGIN_NAMESPACE

char FileHandler::s_pathBuf_[ 1024 ];


/**
 *  Don't call this from subclass implementations of init() until you are ready
 *  to read (if you're in read mode).
 */
bool FileHandler::init( const char *path, const char *mode )
{
	filename_ = path;
	mode_ = mode;
	length_ = this->length();

	return this->read();
}


/**
 * This method returns whether the file has been modified since last using it.
 *
 * @returns true if the file has been modified, false if not.
 */
bool FileHandler::isDirty()
{
	return length_ != this->length();
}


/**
 * This method returns the filename being referenced by the object instance.
 *
 * @returns The filename as a const char *.
 */
const char *FileHandler::filename() const
{
	return filename_.c_str();
}


/**
 * This method flushes any current output in the buffer and re-reads the
 * contents.
 *
 * @returns true on success, false on error.
 */
bool FileHandler::refresh()
{
	this->flush();
	bool success = this->read();
	length_ = this->length();

	return success;
}


/**
 * This static method provides a convenient way of concatenating the provided
 * directory and filename together.
 *
 * @returns A pointer to the concatenated file path.
 */
const char *FileHandler::join( const char *dir, const char *filename )
{
	bw_snprintf( s_pathBuf_, sizeof( s_pathBuf_ ), "%s/%s", dir, filename );
	return s_pathBuf_;
}

BW_END_NAMESPACE

// file_handler.cpp
