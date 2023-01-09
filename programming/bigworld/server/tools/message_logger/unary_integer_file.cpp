#include "unary_integer_file.hpp"

#include "cstdmf/debug.hpp"

#include <unistd.h>
#include <errno.h>
#include <string.h>


BW_BEGIN_NAMESPACE

UnaryIntegerFile::UnaryIntegerFile() :
	v_( -1 )
{ }


/**
 *  UnaryIntegerFile's init() accepts an extra int arg unlike the init()
 *  methods of related classes, and it means a few things.  In read mode, it is
 *  the value *  you expect the file to have in it.  In append mode, if the
 *  file doesn't exist, the value will be written to the file, and if it does
 *  exist, it means the same as it does in read mode (i.e. a sync check).
 */
bool UnaryIntegerFile::init( const char *path, const char *mode, int v )
{
	if (!TextFileHandler::init( path, mode ))
	{
		return false;
	}

	if ((!strcmp( mode, "r" )  && v_ != v) ||
		(!strcmp( mode, "a+" ) && v_ != -1 && v_ != v))
	{
		ERROR_MSG( "UnaryIntegerFile::init: "
			"Value in %s (%d) does not match %d\n", path, v_, v );
		return false;
	}

	if (!strcmp( mode, "a+" ) && v_ == -1)
	{
		return this->set( v );
	}
	else
	{
		return true;
	}
}


bool UnaryIntegerFile::handleLine( const char *line )
{
	if (v_ != -1)
	{
		ERROR_MSG( "UnaryIntegerFile::handleLine: "
			"There is more than one number in %s!\n", filename_.c_str() );
		return false;
	}

	return sscanf( line, "%d", &v_ ) == 1;
}


void UnaryIntegerFile::flush()
{
	v_ = -1;
}


bool UnaryIntegerFile::set( int v )
{
	if (fprintf( fp_, "%d\n", v ) < 0)
		return false;
	fflush( fp_ );
	v_ = v;
	return true;
}


int UnaryIntegerFile::getValue() const
{
	return v_;
}

/**
 * Removes the file the class owns.
 */
bool UnaryIntegerFile::deleteFile()
{
	if (unlink( this->filename() ) && errno != ENOENT)
	{
		ERROR_MSG( "UnaryIntegerFile::deleteFile: "
			"Failed to remove 'pid': %s\n", strerror( errno ) );
		return false;
	}

	return true;
}

BW_END_NAMESPACE

// unary_integer_file.cpp
