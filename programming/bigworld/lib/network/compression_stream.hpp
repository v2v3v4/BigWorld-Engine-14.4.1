#ifndef COMPRESSION_STREAM_HPP
#define COMPRESSION_STREAM_HPP

#if !defined( __APPLE__ ) && !defined( __ANDROID__ )
#define HAS_ZIP_STREAM
#endif

#include "compression_type.hpp"
#if defined( HAS_ZIP_STREAM )
#include "zip_stream.hpp"
#endif

BW_BEGIN_NAMESPACE

/**
 *	This class is used to encapsulate streaming from a potentially compressed
 *	stream.
 */
class CompressionIStream
{
public:
	CompressionIStream( BinaryIStream & stream );

	operator BinaryIStream &()	{ return *pCurrStream_; }

private:
	BinaryIStream * pCurrStream_;
#if defined( HAS_ZIP_STREAM )
	ZipIStream zipStream_;
#endif
};


/**
 *	This class is used to encapsulate streaming to a potentially compressed
 *	stream.
 */
class CompressionOStream
{
public:
	CompressionOStream( BinaryOStream & stream,
		BWCompressionType compressType = BW_COMPRESSION_DEFAULT_INTERNAL );

	operator BinaryOStream &()	{ return *pCurrStream_; }

	static bool initDefaults( DataSectionPtr pSection );

private:

	BinaryOStream * pCurrStream_;
#if defined( HAS_ZIP_STREAM )
	ZipOStream zipStream_;
#endif

	static BWCompressionType s_defaultInternalCompression;
	static BWCompressionType s_defaultExternalCompression;
};

BW_END_NAMESPACE

#endif // COMPRESSION_STREAM_HPP
