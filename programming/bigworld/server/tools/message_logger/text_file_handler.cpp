#include "text_file_handler.hpp"

#include "cstdmf/debug.hpp"

#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>


BW_BEGIN_NAMESPACE

TextFileHandler::TextFileHandler() :
	fp_( NULL )
{ }


TextFileHandler::~TextFileHandler()
{
	if (fp_)
		fclose( fp_ );
}


bool TextFileHandler::init( const char *filename, const char *mode )
{
	if (fp_ != NULL)
	{
		ERROR_MSG( "TextFileHandler::init: Unable to re-init, a file "
			"is already open\n" );
		return false;
	}

	fp_ = fopen( filename, mode );
	if (fp_ == NULL)
	{
		ERROR_MSG( "TextFileHandler::init: Unable to open file "
			"'%s' in mode '%s': %s\n", filename, mode, strerror( errno ) );
		return false;
	}

	return FileHandler::init( filename, mode );
}


bool TextFileHandler::close()
{
	if (fp_ == NULL)
	{
		ERROR_MSG( "TextFileHandler::close: Unable to close invalid file "
			"handle (%s)\n", this->filename() );
		return false;
	}

	fclose( fp_ );
	fp_ = NULL;
	return true;
}


/**
 * Closes and reinitialises the file handle.
 * 
 * @returns true on success, false on error.
 */
bool TextFileHandler::reopen()
{
	if ((this->isOpened() && !this->close()) ||
		!this->init( filename_.c_str(), mode_.c_str() ))
	{
		return false;
	}
	return true;
}


/**
 * This method returns whether the file has been replaced since last using it.
 * It checks if the inode has changed (indicating that the old file has been
 * unlinked and replaced by a new file).
 *
 * @returns InodeStatus
 */
TextFileHandler::InodeStatus TextFileHandler::getInodeStatus()
{
	struct stat filenameBuf, fileDescriptorBuf;
	int fd;

	if (stat( filename_.c_str(), &filenameBuf ) == -1) 
	{
		// Unable to access the file any more - may have been unlinked
		ERROR_MSG( "TextFileHandler::getInodeStatus: Unable to stat filename "
			"'%s': %s\n", filename_.c_str(), strerror( errno ) );
		return INODE_STATUS_ERROR;
	}

	// Get the file descriptor for the already-opened file handle
	if (!this->isOpened() || (fd = fileno( fp_ )) == -1)
	{
		// File handle is no longer valid.
		ERROR_MSG( "TextFileHandler::getInodeStatus: Unable to get file "
			"descriptor for invalid file handle (%s)\n", this->filename() );
		return INODE_STATUS_ERROR;
	}

	// Stat the already-opened file descriptor so we can see the original inode
	if (fstat( fd, &fileDescriptorBuf ) == -1)
	{
		// Unable to access the opened file descriptor - serious system error.
		ERROR_MSG( "TextFileHandler::getInodeStatus: Unable to fstat file "
			"descriptor for '%s': %s\n", filename_.c_str(), strerror( errno ) );
		return INODE_STATUS_ERROR;
	}

	// Compare the inode for the filename vs. the inode for the open file
	// descriptor. If they do not match then close and reinit.
	if (filenameBuf.st_ino != fileDescriptorBuf.st_ino)
	{
		return INODE_STATUS_CHANGED;
	}

	return INODE_STATUS_UNCHANGED;
}


/**
 * This method checks the inode status and reopens the file if it has changed.
 *
 * @returns true on success, false on error.
 */
bool TextFileHandler::reopenIfChanged()
{
	TextFileHandler::InodeStatus status = this->getInodeStatus();
	if (status == INODE_STATUS_ERROR)
	{
		return false;
	}
	else if (status == INODE_STATUS_CHANGED)
	{
		return this->reopen();
	}

	return true;
}


bool TextFileHandler::read()
{
	char *line = NULL;
	size_t len = 0;
	bool status = true;

	if (fseek( fp_, 0, 0 ))
	{
		ERROR_MSG( "TextFileHandler::read: Unable to seek to start of file "
			"'%s': %s\n", filename_.c_str(), strerror( errno ) );
		return false;
	}

	while (getline( &line, &len, fp_ ) != -1)
	{
		// Chomp where necessary
		size_t slen = strlen( line );
		if (line[ slen-1 ] == '\n')
		{
			line[ slen-1 ] = '\0';
		}

		if (!this->handleLine( line ))
		{
			status = false;
			ERROR_MSG( "TextFileHandler::read: "
				"Aborting due to failure in handleLine()\n" );
			break;
		}
	}

	if (line)
	{
		free( line );
	}

	return status;
}


long TextFileHandler::length()
{
	if (!fp_)
	{
		return 0;
	}

	long pos = ftell( fp_ );
	MF_VERIFY( fseek( fp_, 0, SEEK_END ) == 0 );

	long currsize = ftell( fp_ );
	MF_VERIFY( fseek( fp_, pos, SEEK_SET ) == 0 );

	return currsize;
}


bool TextFileHandler::writeLine( const char *line ) const
{
	if (mode_.find( 'r' ) != BW::string::npos)
	{
		ERROR_MSG( "TextFileHandler::writeLine: "
			"Can't write to file %s in mode '%s'\n",
			filename_.c_str(), mode_.c_str() );
		return false;
	}

	if (fprintf( fp_, "%s\n", line ) == -1)
	{
		ERROR_MSG( "TextFileHandler::writeLine: "
			"Unable to write line '%s' to file %s: %s\n",
			line, filename_.c_str(), strerror(errno) );
		return false;
	}

	fflush( fp_ );
	return true;
}

BW_END_NAMESPACE

// text_file_handler.cpp

