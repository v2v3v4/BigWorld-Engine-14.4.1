#ifndef ZIP_STREAM_HPP
#define ZIP_STREAM_HPP

#include "cstdmf/memory_stream.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class is used to read from a stream that has compressed data on it.
 */
class ZipIStream : public MemoryIStream
{
public:
	ZipIStream( BinaryIStream & stream );
	ZipIStream();

	~ZipIStream();

	static int peekOriginalLength( BinaryIStream & stream );

	bool init( BinaryIStream & stream, unsigned char * pBuffer = NULL,
		size_t bufferSize = 0 );


private:
	unsigned char * pBuffer_;
	bool shouldDelete_;
};


/**
 *	This class is used compressed data to write to a stream.
 */
class ZipOStream : public BinaryOStream
{
public:
	ZipOStream( BinaryOStream & dstStream,
			int compressLevel = 1 /* Z_BEST_SPEED */ );
	ZipOStream();

	void init( BinaryOStream & dstStream,
			int compressLevel = 1 /* Z_BEST_SPEED */ );

	virtual ~ZipOStream();

	virtual void * reserve( int nBytes )
	{
		return outStream_.reserve( nBytes );
	}

	virtual int size() const
	{ 
		return outStream_.size();
	} 

private:
	MemoryOStream outStream_;
	BinaryOStream * pDstStream_;
	int compressLevel_;
};

BW_END_NAMESPACE

#endif // ZIP_STREAM_HPP
