// -----------------------------------------------------------------------------
// Section: MemoryOStream
// -----------------------------------------------------------------------------


/**
 * 	This method returns the number of bytes that have been
 * 	added to this stream.
 *
 * 	@return The number of bytes in the stream.
 */
MY_INLINE int MemoryOStream::size() const
{
	return (int)(pCurr_ - pBegin_);
}

/**
 *	This method returns true if the memory stream owns the
 *	data buffer associated with it. If this is the case, the
 *	buffer will be deleted when the stream is destroyed.
 *
 *	@return true if the stream owns the buffer, false otherwise.
 */
MY_INLINE bool MemoryOStream::shouldDelete() const
{
	return shouldDelete_;
}

/**
 * 	This method sets the buffer ownership flag. If this is true,
 * 	the buffer will be deleted when this stream's destructor is
 * 	called.
 */
MY_INLINE void MemoryOStream::shouldDelete( bool value )
{
	// Make sure we don't try to set this to false twice.
	MF_ASSERT( shouldDelete_ || value );
	shouldDelete_ = value;
}

/**
 *	This method returns true if the memory stream is required to
 *	keep the ability to rewind, this is true by default.
 *
 *	@return true if rewind is required, false otherwise
 */
MY_INLINE bool MemoryOStream::canRewind() const
{
	return canRewind_;
}

/**
 *	This method sets the can rewind flag. If this is true when
 *	the buffer resizes it will work from the read pointer, rather
 *	then the original begining of the buffer.
 */
MY_INLINE void MemoryOStream::canRewind( bool value )
{
	canRewind_ = value;
}

/**
 * 	This method returns a pointer to the data associated with
 * 	this stream.
 *
 * 	@return Pointer to stream data.
 */
MY_INLINE void * MemoryOStream::data()
{
	return pBegin_;
}

/**
 *	This method reserves a given number of bytes on the stream.
 *	All other stream write operations use this method.
 *
 *	@param nBytes	Number of bytes to reserve.
 *
 *	@return Pointer to the reserved data in the stream.
 */
MY_INLINE void * MemoryOStream::reserve( int nBytes )
{
	if (pCurr_ + nBytes <= pEnd_)
	{
		char * pOldCurr = pCurr_;
		pCurr_ += nBytes;

		return pOldCurr;
	}
	else
	{
		return grow( nBytes );
	}
}

/**
 * 	This method retrieves a given number of bytes from the stream.
 * 	All other stream read operations use this method.
 *
 * 	@param nBytes	Number of bytes to retrieve.
 *
 * 	@return Pointer to the retrieved data in the stream.
 */
MY_INLINE const void * MemoryOStream::retrieve( int nBytes )
{
	char * pOldRead = pRead_;

	IF_NOT_MF_ASSERT_DEV( pRead_ + nBytes <= pCurr_ )
	{
		error_ = true;

		pRead_ = pCurr_;

		// If someone has asked for more data than can fit on a packet this is a
		// pretty dire error
		return nBytes <= int( sizeof( errBuf ) ) ? errBuf : NULL;
	}

	pRead_ += nBytes;

	return pOldRead;
}


/**
 *	This method resets this stream.
 */
MY_INLINE void MemoryOStream::reset()
{
	pRead_ = pCurr_ = pBegin_;
}


/**
 *	This method rewinds the read point
 */
MY_INLINE void MemoryOStream::rewind()
{
	pRead_ = pBegin_;
}


/**
 *	This method returns the number of bytes remaining to be read.
 */
MY_INLINE int MemoryOStream::remainingLength() const
{
	return (int)(pCurr_ - pRead_);
}

/**
 *  Returns the next character to be read without removing it from the stream.
 */
MY_INLINE char MemoryOStream::peek()
{
	IF_NOT_MF_ASSERT_DEV( pRead_ < pCurr_ )
	{
		error_ = true;
		return -1;
	}

	return *(char*)pRead_;
}


// -----------------------------------------------------------------------------
// Section: MemoryIStream
// -----------------------------------------------------------------------------

MY_INLINE MemoryIStream::~MemoryIStream()
{
	if (pCurr_ != pEnd_)
	{
		WARNING_MSG( "MemoryIStream::~MemoryIStream: "
			"There are still %d bytes left\n", (int)(pEnd_ - pCurr_) );
	}
}

MY_INLINE const void * MemoryIStream::retrieve( int nBytes )
{
	const char * pOldRead = pCurr_;

	IF_NOT_MF_ASSERT_DEV( pCurr_ + nBytes <= pEnd_ )
	{
		pCurr_ = pEnd_;
		error_ = true;
		return nBytes <= int( sizeof( errBuf  ) ) ? errBuf : NULL;
	}

	pCurr_ += nBytes;
	return pOldRead;
}

/**
 *  Returns the next character to be read without removing it from the stream.
 */
MY_INLINE char MemoryIStream::peek()
{
	if (pCurr_ >= pEnd_)
	{
		error_ = true;
		return -1;
	}

	return *pCurr_;
}

// -----------------------------------------------------------------------------
// Section: MessageStream
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
MY_INLINE MessageStream::MessageStream( int size ) : MemoryOStream( size )
{
}


/**
 *	This method adds a message to the input stream.
 *
 *	@return True if successful, otherwise false.
 */
MY_INLINE
bool MessageStream::addToStream( BinaryOStream & stream, uint8 messageID )
{
	int size = this->size();

	if (size >= 256)
	{
		// Size is too big
		return false;
	}

	stream << messageID << (uint8)size;

	memcpy( stream.reserve( size ), this->retrieve( size ), size );

	this->reset();

	return true;
}


/**
 *	This method retrieves a message from the bundle.
 */
MY_INLINE bool MessageStream::getMessage( BinaryIStream & stream,
		int & messageID, int & length )
{
	this->reset();

	IF_NOT_MF_ASSERT_DEV( length >= 2 )
	{
		// Length is too short
		return false;
	}

	uint8 size;
	uint8 id;

	stream >> id >> size;
	length -= sizeof( id ) + sizeof( size );

	messageID = id;

 	IF_NOT_MF_ASSERT_DEV( length >= (int)size )
	{
		return false;
	}

	length -= size;

	memcpy( this->reserve( size ), stream.retrieve( size ), size );

	return true;
}

// memory_stream.ipp
