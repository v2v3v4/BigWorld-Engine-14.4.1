#ifndef SHA_CHECKSUM_SCHEME_HPP
#define SHA_CHECKSUM_SCHEME_HPP

#include "cstdmf/checksum_stream.hpp"


BW_BEGIN_NAMESPACE


/**
 *	This class implements a SHA-256 digest scheme.
 */
class SHAChecksumScheme : public ChecksumScheme
{
public:
	static ChecksumSchemePtr create();


	// Overrides from ChecksumScheme.
	virtual size_t streamSize() const;
	virtual void readBlob( const void * rawData, size_t size );
	virtual void addToStream( BinaryOStream & out );

protected:
	virtual void doReset();

	SHAChecksumScheme();
	virtual ~SHAChecksumScheme();

private:
	class Impl;
	Impl * pImpl_;
};


BW_END_NAMESPACE

#endif // SHA_CHECKSUM_SCHEME_HPP

