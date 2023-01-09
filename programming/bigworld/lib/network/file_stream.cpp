#include "pch.hpp"

#include "file_stream.hpp"

#include "cstdmf/debug.hpp"
#include "cstdmf/bw_util.hpp"

#if !defined( CODE_INLINE )
BW_BEGIN_NAMESPACE
#include "file_stream.ipp"
BW_END_NAMESPACE
#endif

DECLARE_DEBUG_COMPONENT2( "Network", 0 )

BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 */
FileStream::FileStream( const char * pPath, const char * pMode ) :
	path_( pPath ),
	mode_( pMode ),
	pFile_( NULL ),
	pReadBuf_( new char[ INIT_READ_BUF_SIZE ] ),
	readBufSize_( INIT_READ_BUF_SIZE ),
	//errorMsg_(),
	lastAction_( 0 ),
	isOpen_( false ),
	offset_( 0 ),
	fileStreamsIter_( s_openFiles_.end() )
{
	this->open();
}


/**
 *	Destructor.
 */
FileStream::~FileStream()
{
	this->close();
	bw_safe_delete_array( pReadBuf_ );
}


/**
 *	This method returns a string represenation of the current error status.
 *
 *	@return A pointer to a string describing the error state.
 */
const char * FileStream::strerror() const
{
	if (errno)
	{
		return ::strerror( errno );
	}
	else
	{
		return errorMsg_.c_str();
	}
}


/**
 *	This method returns the stream pointer position within the open file.
 *
 *	@return The position within the open file on success, false otherwise.
 */
long FileStream::tell()
{
	if (!this->open())
	{
		return -1;
	}

	long result = ftell( pFile_ );

	if (result == -1)
	{
		error_ = true;
	}

	return result;
}


/**
 *	This method seeks to an abitrary point in the file.
 *
 *	See the documentation for fseek() for implementation details.
 *
 *	@param offset	The offset into the FileStream to seek to
 *		(relative to whence)
 *	@param whence   The location within the FileStream to seek from.
 *
 *	@return 0 on success, -1 on error.
 */
int FileStream::seek( long offset, int whence )
{
	if (!this->open())
	{
		return -1;
	}

	int result = fseek( pFile_, offset, whence );

	if (result == -1)
	{
		error_ = true;
	}

	return result;
}



/**
 *  Returns the size of this file on disk (i.e. doesn't include data not yet
 *  committed).
 */
long FileStream::length()
{
	if (!this->open())
	{
		return -1;
	}

	struct stat statinfo;
	if (fstat( bw_fileno( pFile_ ), &statinfo ))
	{
		error_ = true;
		return -1;
	}

	return statinfo.st_size;
}


/**
 *  Using the output-streaming operators on FileStreams doesn't actually write
 *  the bytes to disk; it just inserts them into a memory buffer.  This method
 *  must be called to actually do the write (although all unwritten bytes are
 *  automatically committed to disk on destruction anyway).
 */
bool FileStream::commit()
{
	this->setMode( 'w' );

	if (!this->open())
	{
		return false;
	}

	errno = 0;

	if (fwrite( this->data(), 1, this->size(), pFile_ ) !=
		(size_t)this->size())
	{
		error_ = true;
		errorMsg_ = "Couldn't write all bytes to disk";
		return false;
	}

	if (fflush( pFile_ ) == EOF)
	{
		error_ = true;
		return false;
	}

	// Clear the internal memory buffer
	this->reset();
	return true;
}


/**
 *	This method retrieves a number of bytes from the file.
 *
 *	Note: This function may fail to retrieve the requested number of bytes.
 *	You must check the error status of the FileStream after attempting to
 *	retrieve in order to ensure all requested data was obtained.
 *
 *	@param nBytes The number of bytes to read from the FileStream.
 *
 *	@return A pointer to the start of the requested data.
 */
const void * FileStream::retrieve( int nBytes )
{
	this->setMode( 'r' );

	if (readBufSize_ < nBytes)
	{
		bw_safe_delete_array( pReadBuf_ );
		pReadBuf_ = new char[ nBytes ];
		readBufSize_ = nBytes;
	}

	if (!this->open())
	{
		return pReadBuf_;
	}

	errno = 0;

	if (fread( pReadBuf_, 1, nBytes, pFile_ ) != (size_t)nBytes)
	{
		error_ = true;
		errorMsg_ = "Couldn't read desired number of bytes from disk";
	}

	return pReadBuf_;
}


/**
 *	This method performs a stat() on the open file.
 *
 *	@param pStatInfo  A pointer to a struct stat to place the result in.
 *
 *	@return 0 on success, -1 on error.
 */
int FileStream::stat( struct stat * pStatInfo )
{
	if (!this->open())
	{
		return -1;
	}

	return fstat( bw_fileno( pFile_ ),  pStatInfo );
}


/**
 *  Prepares this stream for I/O operations in the specified mode (either 'r' or
 *  'w').  This is necessary due to the ANSI C requirement that file positioning
 *  operations are interleaved between reads and writes, and vice versa.
 */
void FileStream::setMode( char mode )
{
	if (!this->open())
	{
		return;
	}

	if ((mode == 'w' && lastAction_ == 'r') ||
		 (mode == 'r' && lastAction_ == 'w'))
	{
		error_ |= fseek( pFile_, 0, SEEK_CUR ) == -1;
	}

	lastAction_ = mode;
}


/**
 *	This method opens the file specified during construction.
 *
 *	@return true if the file was successfully opened, false otherwise.
 */
bool FileStream::open()
{
	// If we're already at the front of the open handles queue, just return now.
	// This hopefully makes repeated accesses on the same file a bit faster
	if (!s_openFiles_.empty() && s_openFiles_.front() == this)
	{
		return true;
	}

	// If already open but not at the front of the queue, remove the existing
	// reference to this file from the queue
	else if (isOpen_)
	{
		this->remove();
	}

	// Otherwise, really open up the file again
	else
	{
		if ((pFile_ = bw_fopen( path_.c_str(), mode_.c_str() )) == NULL)
		{
			error_ = true;
			return false;
		}

		lastAction_ = 0;
		isOpen_ = true;

		// Restore old position if it was set
		if (offset_)
		{
			if (fseek( pFile_, offset_, SEEK_SET ) == -1)
			{
				error_ = true;
				return false;
			}
		}
	}

	// Each time we call this, this file moves to the front of the queue
	s_openFiles_.push_front( this );
	fileStreamsIter_ = s_openFiles_.begin();

	// If there are too many files open, purge the least-recently used one
	if (s_openFiles_.size() > MAX_OPEN_FILES)
	{
		s_openFiles_.back()->close();
	}

	return true;
}


/**
 *	This method closes the open file.
 */
void FileStream::close()
{
	if (!isOpen_)
	{
		return;
	}

	if (pCurr_ != pBegin_)
	{
		this->commit();
	}

	// Save file offset in case we do an open() later on.  We can't use
	// this->tell() here because that would cause an open()
	offset_ = ftell( pFile_ );
	fclose( pFile_ );
	isOpen_ = false;

	// Remove this object from the handle queue
	this->remove();
}


/**
 *	This method removes the entry for this object in the static open files list.
 */
void FileStream::remove()
{
	s_openFiles_.erase( fileStreamsIter_ );
	fileStreamsIter_ = s_openFiles_.end();
}


FileStream::FileStreams FileStream::s_openFiles_;

BW_END_NAMESPACE

// file_stream.cpp
