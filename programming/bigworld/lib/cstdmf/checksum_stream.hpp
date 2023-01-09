#ifndef CHECKSUM_STREAM_HPP
#define CHECKSUM_STREAM_HPP


#include "cstdmf/memory_stream.hpp"
#include "cstdmf/smartpointer.hpp"

BW_BEGIN_NAMESPACE

/**
 *	Abstract class for a scheme to computing checksums/digests and add to
 *	binary streams.
 */
class ChecksumScheme : public SafeReferenceCount
{
public:

	/**
	 *	This method returns whether the checksum scheme has an error. Call
	 *	reset() to reset the error state.
	 */
	bool isGood() const 					{ return errorString_.empty(); }

	/**
	 *	This method returns a description of the last error.
	 */
	const BW::string & errorString() const 	{ return errorString_; }

	/**
	 *	This method resets the object so it can be re-used for a new round of
	 *	checksumming.
	 */
	void reset()
	{
		errorString_.clear();
		this->doReset();
	}

	/**
	 *	Return the size of the checksum that will be added to streams.
	 */
	virtual size_t streamSize() const = 0;

	/**
	 *	This method updates the scheme object with data to be summed.
	 *
	 *	@param data 	The data to sum.
	 *	@param size		The size of the data.
	 */
	virtual void readBlob( const void * data, size_t size ) = 0;

	/**
	 *	This method adds the checksum to the given output stream.
	 *
	 *	@param out 	The output stream.
	 */
	virtual void addToStream( BinaryOStream & out ) = 0;

	bool verifyFromStream( BinaryIStream & in,
		BinaryOStream * pChecksum = NULL );

protected:

	/**
	 *	This method peforms any resetting of state for this scheme object so it
	 *	can be re-used for another round of checksumming.
	 */
	virtual void doReset() = 0;
	CSTDMF_DLL virtual bool doVerifyFromStream( BinaryIStream & in );
	CSTDMF_DLL void setErrorMismatched();

	/**
	 *	Constructor.
	 *
	 *	Typically schemes should have a static create() method that returns a
	 *	ChecksumSchemePtr.
	 */
	ChecksumScheme() :
		SafeReferenceCount()
	{}


	/** Destructor. */
	virtual ~ChecksumScheme() {}

	BW::string errorString_;
};

typedef SmartPointer< ChecksumScheme > ChecksumSchemePtr;


/**
 *	Use an XOR checksum at the word level. For padding, 0 bytes are added to
 *	the end of the byte stream, and the word is interpreted as a little-endian
 *	word for summation.
 */
class XORChecksumScheme : public ChecksumScheme
{
private:
	typedef uint32 Checksum;

public:

	/**
	 *	Factory method.
	 */
	static ChecksumSchemePtr create( Checksum initialValue = 0 )
	{
		return new XORChecksumScheme( initialValue );
	}


	// Overrides from ChecksumScheme
	virtual void doReset();
	virtual size_t streamSize() const { return sizeof( Checksum ); }
	virtual void readBlob( const void * rawData, size_t size );
	virtual void addToStream( BinaryOStream & out );

private:
	virtual bool doVerifyFromStream( BinaryIStream & in );

	void padRemainder();

	XORChecksumScheme( Checksum initialValue );

	/** Destructor. */
	virtual ~XORChecksumScheme()
	{}

	Checksum 	value_;
	Checksum 	remainder_;
	uint 		remainderLength_;
};


/**
 *	This class implements a MD5 sum checksum scheme.
 *
 *	In general, we shouldn't be using this where having good crypto matters.
 */
class MD5SumScheme : public ChecksumScheme
{
public:
	static ChecksumSchemePtr create();

	// Overrides from ChecksumScheme
	virtual size_t streamSize() const;
	virtual void readBlob( const void * data , size_t size );
	virtual void addToStream( BinaryOStream & out );

private:
	virtual bool doVerifyFromStream( BinaryIStream & in );
	virtual void doReset();

	MD5SumScheme();
	virtual ~MD5SumScheme();

	class Impl;
	Impl * pImpl_;
};


/**
 *	This class is used to compose two checksum schemes together, with the output
 *	of the first checksum scheme given to the second checksum scheme. The
 *	output checksum can be described as g( f ( x ) ), where f is the first
 *	checksum scheme and g is the second checksum scheme.
 */
class ChainedChecksumScheme : public ChecksumScheme
{
public:
	/**
	 *	Factory method.
	 */
	static ChecksumSchemePtr create( ChecksumSchemePtr pFirst,
			ChecksumSchemePtr pSecond )
	{
		if (!pFirst || !pSecond)
		{
			return ChecksumSchemePtr();
		}

		return new ChainedChecksumScheme( pFirst, pSecond );
	}

	// Overrides from ChecksumScheme

	/* Override from ChecksumScheme. */
	virtual size_t streamSize() const
	{
		return pSecond_->streamSize();
	}

	/* Override from ChecksumScheme. */
	virtual void readBlob( const void * data, size_t size )
	{
		pFirst_->readBlob( data, size );
	}

	virtual void addToStream( BinaryOStream & out );

protected:
	virtual bool doVerifyFromStream( BinaryIStream & in );

	/* Override from ChecksumScheme. */
	virtual void doReset()
	{
		pFirst_->reset();
		pSecond_->reset();
	}


	/**
	 *	Constructor.
	 *
	 *	@param pFirst 		The first scheme.
	 *	@param pSecond 		The second scheme that wraps the first scheme.
	 */
	ChainedChecksumScheme( ChecksumSchemePtr pFirst,
			ChecksumSchemePtr pSecond ) :
		pFirst_( pFirst ),
		pSecond_( pSecond )
	{}

	/** Destructor. */
	virtual ~ChainedChecksumScheme()
	{}

private:
	ChecksumSchemePtr pFirst_;
	ChecksumSchemePtr pSecond_;
};

/**
 *	This class is used to add a checksum to the end of a streamed sequence of
 *	bytes.
 */
class ChecksumOStream : public BinaryOStream, public ReferenceCount
{
public:
	ChecksumOStream( MemoryOStream & wrappedStream,
			ChecksumSchemePtr pChecksumScheme,
			bool shouldReset = true );

	ChecksumOStream( BinaryOStream & wrappedStream,
			ChecksumSchemePtr pChecksumScheme,
			bool shouldReset = true );

	virtual ~ChecksumOStream();

	/**
	 *	This method returns whether finalise() has been called already.
	 */
	bool hasFinalised() const { return (pChecksumScheme_ == NULL); }
	void cancel();
	void finalise( BinaryOStream * pChecksum = NULL );


	// Overrides from BinaryOStream.
	virtual void * reserve( int nBytes );
	virtual int size() const;
	virtual void addBlob( const void * pBlob, int size );


	class Impl; // public because this is actually a base class

protected:
	ChecksumSchemePtr 	pChecksumScheme_;
	Impl * 				pImpl_;
};


/**
 *	This class is used to verify a checksum present at the end of a streamed
 *	sequence of bytes.
 */
class ChecksumIStream : public BinaryIStream
{
public:
	ChecksumIStream( BinaryIStream & wrappedStream,
			ChecksumSchemePtr pChecksumScheme,
			bool shouldReset = true );

	virtual ~ChecksumIStream();

	/**
	 *	This method cancels verification when the object goes out of scope.
	 */
	void cancel() { hasVerified_ = true; }

	bool verify( BinaryOStream * pChecksum = NULL );

	/**
	 *	This method returns the error string, or an empty string if no error.
	 */
	const BW::string & errorString() const
	{
		return pChecksumScheme_->errorString();
	}

	// Overrides from BinaryIStream
	virtual const void * retrieve( int nBytes );
	virtual int remainingLength() const;
	virtual void finish();
	virtual char peek();


private:
	ChecksumSchemePtr 		pChecksumScheme_;
	BinaryIStream & 		wrappedStream_;
	bool 					hasVerified_;
};


BW_END_NAMESPACE

#endif // CHECKSUM_STREAM_HPP
