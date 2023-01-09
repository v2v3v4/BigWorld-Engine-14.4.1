#ifndef ELLIPTIC_CURVE_CHECKSUM_SCHEME_HPP
#define ELLIPTIC_CURVE_CHECKSUM_SCHEME_HPP


#include "cstdmf/bw_string.hpp"
#include "cstdmf/checksum_stream.hpp"


BW_BEGIN_NAMESPACE


/**
 *	This class implements signatures based on elliptic curve public-key
 *	cryptography.
 *
 *	Typically, when compared with RSA, the same level of security can be
 *	attained by smaller key sizes. 1024-bit RSA key is about equivalent to a
 *	160-bit ECC key. The smaller key size also means smaller signatures, with
 *	1024-bit RSA using 256 bytes for signatures to about 40 bytes for 160-bit
 *	ECC.
 */
class EllipticCurveChecksumScheme : public ChecksumScheme
{
public:
	static ChecksumSchemePtr create( const BW::string & keyPEM,
		bool isPrivate );

	// Overrides from ChecksumScheme
	virtual size_t streamSize() const;
	virtual void readBlob( const void * data, size_t size );
	virtual void addToStream( BinaryOStream & out );

private:
	virtual void doReset();
	virtual bool doVerifyFromStream( BinaryIStream & stream );

	EllipticCurveChecksumScheme( const BW::string & keyPEM, bool isPrivate );
	virtual ~EllipticCurveChecksumScheme();

	class Impl;
	Impl * pImpl_;
};


BW_END_NAMESPACE


#endif // ELLIPTIC_CURVE_CHECKSUM_SCHEME_HPP
