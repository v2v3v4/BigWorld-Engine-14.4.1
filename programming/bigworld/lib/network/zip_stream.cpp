#include "pch.hpp"

#include "zip_stream.hpp"

#include "zip/zlib.h"

BW_BEGIN_NAMESPACE

namespace
{

/**
 *	This helper class is used to manage the static buffer used by ZipIStream and
 *	ZipOStream.
 */
class StaticBuffer
{
public:
	StaticBuffer() :
		pBuf_( NULL ),
		size_( 0 ),
		isCheckedOut_( false )
	{
	}

	~StaticBuffer()
	{
		bw_safe_delete_array(pBuf_);
		size_ = 0;
	}

	unsigned char * getBuffer( int size )
	{
		// Currently only support a single buffer.
		MF_ASSERT( !isCheckedOut_ );

		if (size > size_)
		{
			bw_safe_delete_array(pBuf_);
			pBuf_ = new unsigned char[ size ];
			size_ = size;
		}

		isCheckedOut_ = true;

		return pBuf_;
	}

	void giveBuffer( unsigned char * pBuffer )
	{
		MF_ASSERT( pBuffer == pBuf_ );
		MF_ASSERT( isCheckedOut_ );
		isCheckedOut_ = false;
	}

private:
	unsigned char * pBuf_;
	int size_;
	bool isCheckedOut_;
};

StaticBuffer g_staticBuffer;

} // anonymous namespace


// -----------------------------------------------------------------------------
// Section: ZipIStream
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 *
 *	@param stream	The stream to read the compressed data from. This should
 *					have been written with a ZipOStream.
 */
ZipIStream::ZipIStream( BinaryIStream & stream ) :
	MemoryIStream(),
	pBuffer_( NULL ),
	shouldDelete_( false )
{
	this->init( stream, NULL );
}


/**
 *	Constructor.
 */
ZipIStream::ZipIStream() :
	MemoryIStream(),
	pBuffer_( NULL ),
	shouldDelete_( false )
{
}


/**
 *	This method peeks at the original length embedded in the ZipIStream data
 *	stream. 
 *
 *	This can be used to maintain a buffer in the calling code that can
 *	be reused for smaller subsequent decompression operations.
 *
 *	The given stream is not modified.
 */
/* static */ int ZipIStream::peekOriginalLength( BinaryIStream & stream )
{
	// TODO: Should check length and return appropriately if there is
	// insufficient data.

	// Packed string lengths are either most 4 bytes.
	MemoryIStream stringLengthStream( stream.retrieve( 0 ), 4 );

	int length = stringLengthStream.readStringLength();
	stringLengthStream.finish();

	return length;
}


/**
 *	This method initialises the zip stream.
 */
bool ZipIStream::init( BinaryIStream & stream, unsigned char * pBuffer,
		size_t bufferSize )
{
	error_ = false;

	if (stream.error())
	{
		error_ = true;
		return false;
	}

	uLongf origLen = stream.readStringLength();
	int compressLen = stream.readStringLength();

	if (stream.error() || (compressLen > stream.remainingLength()))
	{
		error_ = true;
		return false;
	}

	if (pBuffer)
	{
		if (static_cast< uLongf >( bufferSize ) < origLen)
		{
			error_ = true;
			return false;
		}

		pBuffer_ = pBuffer;
		shouldDelete_ = false;
	}
	else
	{
		pBuffer_ = new unsigned char[ origLen ];
		shouldDelete_ = true;
	}

	int result = uncompress( pBuffer_, &origLen,
		static_cast< const Bytef * >( stream.retrieve( compressLen ) ),
		compressLen );

	if (result != Z_OK)
	{
		error_ = true;

		if (shouldDelete_)
		{
			bw_safe_delete_array( pBuffer_ );
		}
		else
		{
			pBuffer_ = NULL;
		}

		return false;
	}

	this->MemoryIStream::init( reinterpret_cast< char * >( pBuffer_ ),
			origLen );
	return true;
}


/**
 *	Destructor.
 */
ZipIStream::~ZipIStream()
{
	if (pBuffer_ && shouldDelete_)
	{
		bw_safe_delete_array( pBuffer_ );
	}
}


// -----------------------------------------------------------------------------
// Section: ZipOStream
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 *
 *	@param dstStream	The stream to write the compressed data to. Note that
 *			this class keeps a reference to this stream and only writes to it
 *			in the destructor.
 *	@param compressLevel The level of compression to use.
 */
ZipOStream::ZipOStream( BinaryOStream & dstStream,
		int compressLevel ) :
	outStream_(),
	pDstStream_( NULL ),
	compressLevel_( 0 )
{
	this->init( dstStream, compressLevel );
}


/**
 *	Constructor.
 */
ZipOStream::ZipOStream() :
	outStream_(),
	pDstStream_( NULL ),
	compressLevel_( 0 )
{
}


/**
 *	This method initialises the ZipOStream.
 */
void ZipOStream::init( BinaryOStream & dstStream, int compressLevel )
{
	pDstStream_ = &dstStream;
	compressLevel_ = compressLevel;
}


/**
 *	Destructor
 */
ZipOStream::~ZipOStream()
{
	if (pDstStream_ == NULL)
	{
		// Not initialised.
		return;
	}

	int srcLen = outStream_.size();
	const void * pSrc = outStream_.retrieve( srcLen );

	uLongf dstLen = compressBound( srcLen );
	unsigned char * pDst = g_staticBuffer.getBuffer( dstLen );

	int result = compress2( pDst, &dstLen,
			static_cast< const Bytef * >( pSrc ), srcLen,
			compressLevel_ );

	MF_ASSERT( result == Z_OK );

	pDstStream_->writeStringLength( srcLen );
	pDstStream_->writeStringLength( dstLen );
	pDstStream_->addBlob( pDst, dstLen );

	g_staticBuffer.giveBuffer( pDst );
}

BW_END_NAMESPACE

// zip_stream.cpp
