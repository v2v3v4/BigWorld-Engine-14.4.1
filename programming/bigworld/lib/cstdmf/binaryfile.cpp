#include "pch.hpp"

#include <algorithm>
#include "binaryfile.hpp"

BW_BEGIN_NAMESPACE

namespace
{
	///Maximum size in bytes for the write buffer to grow to.
	const size_t MAX_MANAGED_BUFFER_SIZE = 100 * 1048576;
}


///This value for the FILE indicates that the object is backed by
///a memory buffer owned by this object (rather than a file or
///a buffer not owned by this object).
FILE * const BinaryFile::FILE_INTERNAL_BUFFER =
	reinterpret_cast<FILE *>(-1);


/**
*	Creates an object to read/write @a bf. It takes ownership of
*	@a bf and will therefore call fclose on it at destruction.
*	
*	To create a memory-backed object, pass @a bf = NULL then call
*	the reserve member.
*/
BinaryFile::BinaryFile( FILE * bf ) :
	file_( bf ),
	data_( NULL ),
	ptr_( NULL ),
	bytesCached_(0),
	error_( false )
{
}


BinaryFile::~BinaryFile()
{
	if (file_ != NULL)
	{
		if (file_ != FILE_INTERNAL_BUFFER)
		{
			if (fclose( file_ ) != 0)
			{
				//Potential truncated output, etc.
				ERROR_MSG( "BinaryFile error closing output file." );
			}
		}

		if (data_ != NULL)
		{
			bw_free(data_);
			data_ = NULL;
		}
	}
}


/**
 *	Read the whole file into memory (if not already done previously),
 *	set the memory buffer read position to the start.
 *
 *	@return The number of bytes loaded, 0 on error.
 */
long BinaryFile::cache()
{
	MF_ASSERT( file_ != FILE_INTERNAL_BUFFER );
	MF_ASSERT( file_ != NULL );

	if (error_)
	{
		return 0;
	}

	if (data_ == NULL)
	{
		MF_ASSERT(ptr_ == NULL);

		if (fseek( file_, 0, SEEK_END ) != 0)
		{
			error_ = true;
			return 0;
		}
		bytesCached_ = ftell( file_ );
		MF_ASSERT(bytesCached_ >= -1L);

		if (bytesCached_ == -1L)
		{
			error_ = true;
			bytesCached_ = 0;
			return 0;
		}
		if (bytesCached_ == 0)
		{
			return 0;
		}

		data_ = bw_malloc( bytesCached_ );

		if ((fseek( file_, 0, SEEK_SET ) != 0u)
			||
			(static_cast<ptrdiff_t>(fread( data_, 1, bytesCached_, file_ ))
				!= bytesCached_))
		{
			error_ = true;
			bytesCached_ = 0;
			bw_free(data_);
			data_ = NULL;
			return 0;
		}
	}

	ptr_ = reinterpret_cast<char *>(data_);
	return bytesCached_;
}


/**
 *	Wrap this existing data. If @a makeCopy is true, the data is copied
 *	into an internally managed buffer, otherwise @a data is used directly
 *	and must remain valid for this object's lifetime.
 */
void BinaryFile::wrap( void * data, int len, bool makeCopy )
{
	MF_ASSERT( file_ == NULL );
	MF_ASSERT( data_ == NULL );
	MF_ASSERT( ptr_ == NULL );
	MF_ASSERT(data != NULL);
	MF_ASSERT(len >= 0);

	error_ = false;

	if (makeCopy)
	{
		data_ = bw_malloc( len );
		memcpy( data_, data, len );
		file_ = FILE_INTERNAL_BUFFER;	//We only free in the destructor
										//if file_ is non-null.
	}
	else
	{
		data_ = data;
	}
	ptr_ = reinterpret_cast<char *>(data_);
	bytesCached_ = len;
}


/**
 *	Allocate and reserve the given amount of data, and remember
 *	to dispose it on destruction.
 *	This makes this class effectively a memory stream.
 *	@return A pointer to the allocated buffer.
 */
void * BinaryFile::reserve( int len )
{
	MF_ASSERT( file_ == NULL );
	MF_ASSERT( data_ == NULL );
	MF_ASSERT( ptr_ == NULL );
	MF_ASSERT(len >= 0);

	error_ = false;

	data_ = bw_malloc( len );
	file_ = FILE_INTERNAL_BUFFER;
	ptr_ = reinterpret_cast<char *>(data_);
	bytesCached_ = len;

	return data_;
}


/// @return The current cursor position in the stream. 0 on error.
size_t BinaryFile::cursorPosition()
{
	if (error_ || ((ptr_ == NULL) && (file_ == NULL)))
	{
		error_ = true;
		return 0;
	}

	if (ptr_)
	{
		const std::ptrdiff_t result =
			reinterpret_cast<const unsigned char *>(ptr_) -
			reinterpret_cast<const unsigned char *>(data_);
		MF_ASSERT(result >= 0);
		return result;
	}

	const long pos = ftell( file_ );
	if (pos == -1L)
	{
		error_ = true;
		return 0;
	}
	return pos;
}


/// @return How many bytes are left in the stream. 0 on error.
size_t BinaryFile::bytesAvailable()
{
	if (error_ || ((ptr_ == NULL) && (file_ == NULL)))
	{
		return 0;
	}

	if (ptr_)
	{
		return reinterpret_cast<unsigned char *>(data_) +
			bytesCached_ - reinterpret_cast<unsigned char *>(ptr_);
	}

	const long pos = ftell( file_ );
	if (pos == -1L)
	{
		error_ = true;
		return 0;
	}

	if (fseek( file_, 0, SEEK_END ) != 0)
	{
		error_ = true;
		return 0;
	}

	const long size = ftell( file_ );
	if (size == -1L)
	{
		error_ = true;
		return 0;
	}

	if (fseek( file_, pos, SEEK_SET ) != 0)
	{
		error_ = true;
		return 0;
	}

	return size - pos;
}


/**
 *	Moves the current cursor position to 0 and resets the error flags to false.
 */
void BinaryFile::rewind()
{
	error_ = false;

	ptr_ = reinterpret_cast<char *>(data_);
	if ( (file_ != NULL) && (file_ != FILE_INTERNAL_BUFFER) )
	{
		::rewind( file_ );
	}
}


/**
 *	Attempts to read nbytes into buffer.
 *	@return Actual amount of bytes read.
 */
size_t BinaryFile::read( void * buffer, size_t nbytes )
{
	if (error_ || ((ptr_ == NULL) && (file_ == NULL)))
	{
		error_ = true;

		return 0;
	}

	if (ptr_)
	{
		const size_t bytesAvail = bytesAvailable();
		if( bytesAvail < nbytes )
		{
			memcpy( buffer, ptr_, bytesAvail );
			ptr_ += bytesAvail;
			return bytesAvail;
		}
		switch ( nbytes )
		{
		case 1:
			*reinterpret_cast<uint8 *>(buffer) =
				*reinterpret_cast<const uint8 *>(ptr_);
			break;
		case 4:
			*reinterpret_cast<uint32 *>(buffer) =
				*reinterpret_cast<const uint32 *>(ptr_);
			break;
		default:
			memcpy( buffer, ptr_, nbytes );
			break;
		}

		ptr_ += nbytes;
		return nbytes;
	}

	const size_t result = fread( buffer, 1, nbytes, file_ );

	if (result != nbytes)
	{
		//Definitely encountered an error if the end of the file
		//wasn't reached.
		error_ = (feof(file_) == 0);
	}

	return result;
}


/**
 *	Writes nbytes from buffer.
 *	
 *	The cache function must not have been used on this object, this
 *	function requires a memory buffer or a FILE, not both at once.
 * 
 *	For a memory-backed object with an internally managed buffer, writing
 *	may required a reallocation for a larger buffer.
 *	
 *	@return Actual amount of bytes written.
 */
size_t BinaryFile::write( const void * buffer, size_t nbytes )
{
	//Require file-backing or memory-backing. The behaviour is too
	//complicated if we have a file that's cached in memory (this happens
	//when the cache() function is called).
	MF_ASSERT(
		(file_ == NULL)
		||
		(file_ == FILE_INTERNAL_BUFFER)
		||
		(ptr_ == NULL));

	if (error_)
	{
		return 0;
	}

	if (ptr_)
	{
		size_t bytesAvail = bytesAvailable();

		MF_ASSERT(
			((std::numeric_limits<long>::max() / 2) >=
				static_cast<ptrdiff_t>(MAX_MANAGED_BUFFER_SIZE))
			&&
			"MAX_MANAGED_BUFFER_SIZE must be smaller to avoid overflow. ");

		if ( (bytesAvail < nbytes)
			&&
			(file_ == FILE_INTERNAL_BUFFER)
			&&
			(nbytes <= MAX_MANAGED_BUFFER_SIZE) )
		{
			//Since internally managed, can potentially resize to
			//accommodate the write fully.

			const size_t minRequiredBufferSize =
				bytesCached_ + (nbytes - bytesAvail);
			const size_t growBufferSize = std::min(
				MAX_MANAGED_BUFFER_SIZE,
				static_cast<size_t>(bytesCached_) * 2);
			const size_t newBufferSize =
				std::max(minRequiredBufferSize, growBufferSize);
			
			if (newBufferSize <= MAX_MANAGED_BUFFER_SIZE)
			{
				//Buffer is internally managed, we can increase the size
				//of the buffer to accommodate.
				const size_t pos = cursorPosition();
				MF_ASSERT(!error_);	//Expect not possible for memory
									//buffer to go to error here.
				void * newBuffer = bw_malloc( newBufferSize );
				char * newPtr = reinterpret_cast<char *>(newBuffer) + pos;

				memcpy(newBuffer, data_, bytesCached_);

				std::swap(newBuffer, data_);
				ptr_ = newPtr;
				MF_ASSERT( newBufferSize <= LONG_MAX );
				bytesCached_ = ( long )newBufferSize;

				bw_free(newBuffer);

				bytesAvail = bytesAvailable();
				MF_ASSERT(bytesAvail >= nbytes);
			}
		}

		if( bytesAvail < nbytes )
		{
			memcpy( ptr_, buffer, bytesAvail );
			ptr_ += bytesAvail;
			return bytesAvail;
		}
		switch ( nbytes )
		{
		case 1:
			*reinterpret_cast<uint8 *>(ptr_) =
				*reinterpret_cast<const uint8 *>(buffer);
			break;
		case 4:
			*reinterpret_cast<uint32 *>(ptr_) =
				*reinterpret_cast<const uint32 *>(buffer);
			break;
		default:
			memcpy( ptr_, buffer, nbytes );
			break;
		}

		ptr_ += nbytes;
		return nbytes;
	}

	if (file_ == NULL)
	{
		error_ = true;
		return 0;
	}

	const size_t result = fwrite( buffer, 1, nbytes, file_ );

	if (result != nbytes)
	{
		//This means a file write error definitely occurred.
		error_ = true;
	}

	return result;
}

BW_END_NAMESPACE
