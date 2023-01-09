#include "pch.hpp"

#include "checksum_stream.hpp"

#include "cstdmf/md5.hpp"

#include <string.h>

#include <memory>


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: ChecksumScheme
// -----------------------------------------------------------------------------


/**
 *	This method is used to verify a checksum on a stream.
 *
 *	Subclasses can override this default implementation to provide a faster
 *	implementation.
 *
 *	@param in 			The input stream containing the checksum to check. The
 *						readBlob() method must have been called over the data
 *						to be checksummed.
 *	@param pChecksum 	The checksum that was read from the stream.
 */
bool ChecksumScheme::verifyFromStream( BinaryIStream & in,
		BinaryOStream * pChecksum )
{
	if (in.error() || (size_t(in.remainingLength()) < this->streamSize()))
	{
		errorString_ = "Insufficient data on stream";
		return false;
	}

	BinaryIStream * pChecksumStream = &in;
	std::auto_ptr< MemoryOStream > pChecksumBuffer;

	if (pChecksum)
	{
		pChecksumBuffer.reset( new MemoryOStream( (int)(this->streamSize()) ) );
		pChecksumBuffer->transfer( in, (int)(this->streamSize()) );
		pChecksum->transfer( *pChecksumBuffer, (int)(this->streamSize()) );
		pChecksumBuffer->rewind();
		pChecksumStream = pChecksumBuffer.get();
	}

	if (!this->doVerifyFromStream( *pChecksumStream ))
	{
		if (errorString_.empty())
		{
			errorString_ = "Unknown error (scheme failed to verify "
				"but did not provide error)";
		}
		return false;
	}

	if (in.error())
	{
		errorString_ = "Stream error";
		return false;
	}

	return true;
}


/**
 *	This method is the default implementation, which can be overridden by
 *	subclasses to supply faster implementations.
 *
 *	For generic checksum mismatches, use the setErrorMismatched() method.
 *
 *	@param in 	The stream to read from.
 *
 *	@return 	True if verification was successful, false otherwise.
 */
bool ChecksumScheme::doVerifyFromStream( BinaryIStream & in )
{
	const int streamSize = static_cast<int>(this->streamSize());
	const void * streamSum = in.retrieve( streamSize );

	MemoryOStream computedSumStream( streamSize );
	this->addToStream( computedSumStream );
	const void * computedSum = computedSumStream.retrieve( streamSize );

	if (computedSumStream.error())
	{
		errorString_ = "Checksum scheme did not add necessary bytes to stream";
		return false;
	}

	if (0 != memcmp( computedSum, streamSum, streamSize ))
	{
		this->setErrorMismatched();
		return false;
	}

	return true;
}


/**
 *	This method sets the default error string for when there is a checksum
 *	mismatch.
 */
void ChecksumScheme::setErrorMismatched()
{
	errorString_ = "Checksum mismatch";
}


// -----------------------------------------------------------------------------
// Section: XORChecksumScheme
// -----------------------------------------------------------------------------


/**
 *	Constructor.
 */
XORChecksumScheme::XORChecksumScheme( Checksum initialValue ) :
	value_( initialValue ),
	remainder_( 0 ),
	remainderLength_( 0 )
{}


/*
 *	Override from ChecksumScheme.
 */
void XORChecksumScheme::doReset()
{
	value_ = 0;
	remainder_ = 0;
	remainderLength_ = 0;
}


/*
 *	Override from ChecksumScheme.
 */
void XORChecksumScheme::readBlob( const void * rawData, size_t size )
{
	const uint8 * byteData = reinterpret_cast< const uint8 * >( rawData );

	// Leftover from last blob added.
	Checksum word = remainder_;
	uint wordLength = remainderLength_;

	while (size > 0)
	{
		// Note, little endian. Only really significant for the last
		// potentially padded word.
		word |= *(byteData++) << (wordLength * 8);
		--size;

		if (++wordLength == sizeof( Checksum ))
		{
			value_ ^= word;
			// We should now be on a word boundary. Clear for the next
			// word.
			word = 0;
			wordLength = 0;
		}
	}

	remainder_ = word;
	remainderLength_ = wordLength;
}


/*
 *	Override from ChecksumScheme.
 */
void XORChecksumScheme::addToStream( BinaryOStream & out )
{
	this->padRemainder();
	out << value_;
}


/*
 *	Override from ChecksumScheme.
 */
bool XORChecksumScheme::doVerifyFromStream( BinaryIStream & in )
{
	this->padRemainder();

	Checksum value;
	in >> value;

	if (value != value_)
	{
		this->setErrorMismatched();
		return false;
	}

	return true;
}


/**
 *	This method pads out the received data with zeroes so that there are
 *	exactly sizeof(Checksum) words processed.
 */
void XORChecksumScheme::padRemainder()
{
	if (remainderLength_)
	{
		// Note that the padding word is treated as little-endian.
		value_ ^= remainder_;
		remainder_ = 0;
		remainderLength_ = 0;
	}
}


// -----------------------------------------------------------------------------
// Section: MD5SumScheme
// -----------------------------------------------------------------------------

/**
 *	This class implements the MD5SumScheme.
 */
class MD5SumScheme::Impl
{
public:
	/**
	 *	Constructor.
	 */
	Impl() :
		md5_()
	{}

	void reset();
	void readBlob( const void * data, size_t size );
	void addToStream( BinaryOStream & out );
	bool verifyFromStream( BinaryIStream & in );

private:
	MD5 md5_;
};


/**
 *	Factory method.
 */
ChecksumSchemePtr MD5SumScheme::create()
{
	return new MD5SumScheme();
}


/**
 *	Constructor.
 */
MD5SumScheme::MD5SumScheme() :
	pImpl_( new Impl() )
{}


/**
 *	Destructor.
 */
MD5SumScheme::~MD5SumScheme()
{
	delete pImpl_;
}


/*
 *	Override from ChecksumScheme.
 */
size_t MD5SumScheme::streamSize() const
{
	return MD5::Digest::NUM_BYTES;
}


/*
 *	Override from ChecksumScheme.
 */
void MD5SumScheme::doReset()
{
	pImpl_->reset();
}


/**
 *	This method implements the MD5SumScheme::doReset() method.
 */
void MD5SumScheme::Impl::reset()
{
	md5_ = MD5();
}


/*
 *	Override from ChecksumScheme.
 */
void MD5SumScheme::readBlob( const void * data, size_t size )
{
	pImpl_->readBlob( data, size );
}


/**
 *	This method implements the MD5SumScheme::readBlob() method.
 */
void MD5SumScheme::Impl::readBlob( const void * data, size_t size )
{
	md5_.append( data, static_cast<int>(size) );
}


/*
 *	Override from ChecksumScheme.
 */
void MD5SumScheme::addToStream( BinaryOStream & out )
{
	pImpl_->addToStream( out );
}


/**
 *	This method implements the MD5SumScheme::addToStream() method.
 */
void MD5SumScheme::Impl::addToStream( BinaryOStream & out )
{
	MD5::Digest digest;
	md5_.getDigest( digest );
	out << digest;
}


/*
 *	Override from ChecksumScheme.
 */
bool MD5SumScheme::doVerifyFromStream( BinaryIStream & in )
{
	if (!pImpl_->verifyFromStream( in ))
	{
		this->setErrorMismatched();
		return false;
	}

	return true;
}


/**
 *	This method implements the MD5SumScheme::doVerifyFromStream() method.
 */
bool MD5SumScheme::Impl::verifyFromStream( BinaryIStream & in )
{
	MD5::Digest streamDigest;
	in >> streamDigest;

	MD5::Digest myDigest;
	md5_.getDigest( myDigest );

	return (streamDigest == myDigest);
}


// -----------------------------------------------------------------------------
// Section: ChainedChecksumScheme
// -----------------------------------------------------------------------------

/*
 *	Override from ChecksumScheme.
 */
void ChainedChecksumScheme::addToStream( BinaryOStream & out )
{
	MemoryOStream firstOutput( (int)(pFirst_->streamSize()) );
	pFirst_->addToStream( firstOutput );
	MF_ASSERT( pFirst_->streamSize() == size_t(firstOutput.remainingLength()) );
	pSecond_->readBlob( firstOutput.retrieve( (int)(pFirst_->streamSize()) ),
		(int)(pFirst_->streamSize()) );
	pSecond_->addToStream( out );
}


/*
 *	Override from ChecksumScheme.
 */
bool ChainedChecksumScheme::doVerifyFromStream( BinaryIStream & in )
{
	MemoryOStream firstSum( (int)(pFirst_->streamSize()) );
	pFirst_->addToStream( firstSum );

	pSecond_->readBlob( firstSum.retrieve( (int)(pFirst_->streamSize()) ),
		(int)(pFirst_->streamSize()) );

	if (!pSecond_->verifyFromStream( in ))
	{
		errorString_ = pSecond_->errorString();
		return false;
	}

	return true;
}


// -----------------------------------------------------------------------------
// Section: ChecksumOStream
// -----------------------------------------------------------------------------

/**
 *	This is the interface for ChecksumOStream implementations.
 *
 *	The two implementations differ on whether an additional buffer is used, or
 *	not, and the implementation is chosen based on whether the underlying
 *	stream is a concrete MemoryOStream or an abstract BinaryOStream.
 */
class ChecksumOStream::Impl
{
public:
	/** Constructor. */
	Impl()
	{}

	/** Destructor. */
	virtual ~Impl()
	{}


	/**
	 *	This method returns the stream for calling code to use to stream on
	 *	data.
	 */
	virtual BinaryOStream & stream() = 0;


	/**
	 *	This method returns the stream for calling code to use to stream on
	 *	data (const version).
	 */
	virtual const BinaryOStream & stream() const = 0;

	/**
	 *	This method finalises the checksumming.
	 *
	 *	@param pScheme 		The checksum scheme to finalise with.
	 *	@param pChecksum 	This is filled with the checksum that will be
	 *						written out.
	 */
	virtual void finalise( ChecksumSchemePtr pScheme,
		BinaryOStream * pChecksum ) = 0;
};

namespace // (anonymous)
{

/**
 *	This CheckOStream::Impl subclass is used where the underlying stream is a
 *	MemoryOStream. No extra buffer is needed in this case, and the checksum
 *	stream passes through data straight to the underlying stream.
 */
class UnbufferedChecksumOStreamImpl : public ChecksumOStream::Impl
{
public:

	/**
	 *	Constructor.
	 *
	 *	@param wrappedStream 	The wrapped MemoryOStream.
	 */
	UnbufferedChecksumOStreamImpl( MemoryOStream & wrappedStream ) :
			ChecksumOStream::Impl(),
			startOffset_( wrappedStream.size() ),
			wrappedStream_( wrappedStream )
	{}

	/**
	 *	Destructor.
	 */
	virtual ~UnbufferedChecksumOStreamImpl()
	{}


	// Overrides from ChecksumOStream::Impl.
	virtual BinaryOStream & stream() 				{ return wrappedStream_; }
	virtual const BinaryOStream & stream() const 	{ return wrappedStream_; }

	/*
	 *	Override from ChecksumOStream::Impl.
	 */
	virtual void finalise( ChecksumSchemePtr pScheme,
			BinaryOStream * pChecksum )
	{
		// Optimised for MemoryOStreams, no need for an extra buffer, can
		// take advantage of rewind to reset the stream to where checksumming
		// was started.

		int payloadLength = wrappedStream_.size() - startOffset_;

		char * pCurrentRead = (char *)wrappedStream_.retrieve( 0 );
		int readProgress = (int)(pCurrentRead - (char *)wrappedStream_.data());

		wrappedStream_.retrieve( startOffset_ - readProgress );

		pScheme->readBlob( (char *)wrappedStream_.retrieve( 0 ),
			payloadLength );

		wrappedStream_.rewind();

		// Reset the read pointer of our underlying stream.
		wrappedStream_.retrieve( readProgress );

		if (pChecksum)
		{
			MemoryOStream checksum( (int)(pScheme->streamSize()) );
			pScheme->addToStream( checksum );
			pChecksum->transfer( checksum,  (int)(pScheme->streamSize()) );
			checksum.rewind();
			wrappedStream_.transfer( checksum,  (int)(pScheme->streamSize()) );
		}
		else
		{
			pScheme->addToStream( wrappedStream_ );
		}
	}

private:
	int 				startOffset_;
	MemoryOStream & 	wrappedStream_;
};


/**
 *	This CheckOStream::Impl subclass is used where the underlying stream is not
 *	known to be a MemoryOStream. Stream data is stored in a buffer, to be
 *	summed over and transferred on finalisation.
 */
class BufferedChecksumOStreamImpl : public ChecksumOStream::Impl
{
public:

	/**
	 *	Constructor.
	 *
	 *	@param wrappedStream 	The wrapped stream.
	 */
	BufferedChecksumOStreamImpl( BinaryOStream & wrappedStream ) :
			ChecksumOStream::Impl(),
			wrappedStream_( wrappedStream ),
			buffer_()
	{}

	/** Destructor. */
	virtual ~BufferedChecksumOStreamImpl()
	{}

	// Overrides from ChecksumOStream::Impl.
	virtual BinaryOStream & stream() 				{ return buffer_; }
	virtual const BinaryOStream & stream() const 	{ return buffer_; }

	/*
	 *	Override from ChecksumOStream::Impl.
	 */
	virtual void finalise( ChecksumSchemePtr pScheme,
			BinaryOStream * pChecksum )
	{
		// Everything given to the stream since checksumming started is in the
		// buffer, add a checksum onto the end of the buffer and then transfer
		// data to the wrapped stream.
		int payloadLength = buffer_.remainingLength();
		pScheme->readBlob( buffer_.retrieve( 0 ), payloadLength );

		if (pChecksum)
		{
			MemoryOStream checksum( (int)(pScheme->streamSize()) );
			pScheme->addToStream( checksum );
			pChecksum->transfer( checksum, (int)(pScheme->streamSize()) );
			checksum.rewind();
			buffer_.transfer( checksum, (int)(pScheme->streamSize()) );
		}
		else
		{
			pScheme->addToStream( buffer_ );
		}

		wrappedStream_.transfer( buffer_, buffer_.remainingLength() );
	}

private:
	BinaryOStream & 	wrappedStream_;
	MemoryOStream 		buffer_;
};


} // end namespace (anonymous)


/**
 *	Constructor method for MemoryOStreams where additional buffering is not
 *	required.
 *
 *	@param wrappedStream 	The MemoryOStream to wrap.
 *	@param pChecksumScheme 	The checksum scheme to use.
 *	@param shouldReset 		Whether the checksum scheme will be reset() before
 *							use.
 */
ChecksumOStream::ChecksumOStream( MemoryOStream & wrappedStream,
		ChecksumSchemePtr pChecksumScheme, bool shouldReset /* = true */ ) :
	pChecksumScheme_( pChecksumScheme ),
	pImpl_( new UnbufferedChecksumOStreamImpl( wrappedStream ) )
{
	if (pChecksumScheme_.get() && shouldReset)
	{
		pChecksumScheme_->reset();
	}
}


/**
 *	Constructor for general BinaryOStream objects, and buffering is
 *	required.
 *
 *	@param wrappedStream 	The MemoryOStream to wrap.
 *	@param pChecksumScheme 	The checksum scheme to use.
 *	@param shouldReset 		Whether the checksum scheme will be reset() before
 *							use.
 */
ChecksumOStream::ChecksumOStream( BinaryOStream & wrappedStream,
		ChecksumSchemePtr pChecksumScheme, bool shouldReset /* = true */ ) :
	pChecksumScheme_( pChecksumScheme ),
	pImpl_( new BufferedChecksumOStreamImpl( wrappedStream ) )
{
	if (pChecksumScheme_.get() && shouldReset)
	{
		pChecksumScheme_->reset();
	}
}


/**
 *	Destructor.
 *
 *	If it has not finalised when the destructor is called, it will finalise
 *	before it is destroyed.
 */
ChecksumOStream::~ChecksumOStream()
{
	this->finalise();

	bw_safe_delete( pImpl_ );
}


/* 
 *	Override from BinaryOStream.
 */
void * ChecksumOStream::reserve( int nBytes )
{
	return pImpl_->stream().reserve( nBytes );
}


/* 
 *	Override from BinaryOStream.
 */
int ChecksumOStream::size() const
{
	return pImpl_->stream().size();
}


/*
 *	Override from BinaryOStream.
 */
void ChecksumOStream::addBlob( const void * pBlob, int size )
{
	return pImpl_->stream().addBlob( pBlob, size );
}


/**
 *	This method cancels adding of the checksum when the object is destroyed.
 */
void ChecksumOStream::cancel()
{
	pChecksumScheme_ = NULL;
}


/**
 *	This method adds the computed checksum to the wrapped stream.
 *
 *	@param pChecksum 	If not NULL, this is filled with a copy of checksum
 *						that was written out.
 */
void ChecksumOStream::finalise( BinaryOStream * pChecksum /* = NULL */ )
{
	if (this->hasFinalised())
	{
		return;
	}

	pImpl_->finalise( pChecksumScheme_, pChecksum );
	pChecksumScheme_ = NULL;
}


// -----------------------------------------------------------------------------
// Section: ChecksumIStream
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 *
 *	@param wrappedStream 	The underlying stream to verify the checksum for.
 *	@param pChecksum		The checksum scheme.
 *	@param shouldReset 		Whether the checksum scheme should be reset()
 *							before use.
 */
ChecksumIStream::ChecksumIStream( BinaryIStream & wrappedStream,
			ChecksumSchemePtr pChecksumScheme,
			bool shouldReset /* = true */ ) :
		pChecksumScheme_( pChecksumScheme ),
		wrappedStream_( wrappedStream ),
		hasVerified_( false )
{
	if (pChecksumScheme.get() && shouldReset)
	{
		pChecksumScheme->reset();
	}
}


/**
 *	Destructor.
 */
ChecksumIStream::~ChecksumIStream()
{
}


/*
 *	Override from BinaryIStream.
 */
const void * ChecksumIStream::retrieve( int nBytes )
{
	if (nBytes > this->remainingLength())
	{
		error_ = true;
		return NULL;
	}

	const void * data = wrappedStream_.retrieve( nBytes );

	if (pChecksumScheme_.get())
	{
		pChecksumScheme_->readBlob( data, nBytes );
	}

	return data;
}


/**
 *	This method verifies the checksum, which is expected to be next on the
 *	underlying stream.
 *
 *	@param pChecksum 	If not NULL, this is filled with the checksum that was
 *						read from the stream.
 */
bool ChecksumIStream::verify( BinaryOStream * pChecksum )
{
	if (error_ || wrappedStream_.error())
	{
		error_ = true;
		return false;
	}

	if (hasVerified_)
	{
		return !error_;
	}

	if (pChecksumScheme_.get() &&
			!pChecksumScheme_->verifyFromStream( wrappedStream_, pChecksum ))
	{
		error_ = true;
	}

	hasVerified_ = true;

	return !error_;
}


/*
 *	Override from BinaryIStream.
 */
int ChecksumIStream::remainingLength() const
{
	size_t streamSize = (pChecksumScheme_.get() ?
		pChecksumScheme_->streamSize() : 0U);

	return (wrappedStream_.remainingLength() == 0) ?
		0 :
		(wrappedStream_.remainingLength() - int( streamSize ));
}


/*
 *	Override from BinaryIStream.
 */
void ChecksumIStream::finish()
{
	// Make sure the checksum is checked, error() might be queried after
	// calling finish().

	int remainingLength = this->remainingLength();

	if (remainingLength)
	{
		this->retrieve( remainingLength );
	}
}


/*
 *	Override from BinaryIStream.
 */
char ChecksumIStream::peek()
{
	return wrappedStream_.peek();
}


BW_END_NAMESPACE


// checksum_stream.cpp

