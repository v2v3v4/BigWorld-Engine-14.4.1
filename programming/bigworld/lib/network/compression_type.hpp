#ifndef COMPRESSION_TYPE_HPP
#define COMPRESSION_TYPE_HPP

#include "resmgr/datasection.hpp"

BW_BEGIN_NAMESPACE

enum BWCompressionType
{
	BW_COMPRESSION_NONE,

	BW_COMPRESSION_ZIP_1,
	BW_COMPRESSION_ZIP_2,
	BW_COMPRESSION_ZIP_3,
	BW_COMPRESSION_ZIP_4,
	BW_COMPRESSION_ZIP_5,
	BW_COMPRESSION_ZIP_6,
	BW_COMPRESSION_ZIP_7,
	BW_COMPRESSION_ZIP_8,
	BW_COMPRESSION_ZIP_9,

	BW_COMPRESSION_ZIP_BEST_SPEED       = BW_COMPRESSION_ZIP_1,
	BW_COMPRESSION_ZIP_BEST_COMPRESSION = BW_COMPRESSION_ZIP_9,

	BW_COMPRESSION_DEFAULT_INTERNAL,
	BW_COMPRESSION_DEFAULT_EXTERNAL,
};


// Implemented in compression_stream.cpp
bool initCompressionTypes( DataSectionPtr pSection,
			BWCompressionType & internalCompressionType,
			BWCompressionType & externalCompressionType );
bool initCompressionType( DataSectionPtr pSection,
			BWCompressionType & compressionType );

BW_END_NAMESPACE

#endif // COMPRESSION_TYPE_HPP
