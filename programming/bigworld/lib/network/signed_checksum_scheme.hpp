#ifndef SIGNED_CHECKSUM_SCHEME_HPP
#define SIGNED_CHECKSUM_SCHEME_HPP


#include "cstdmf/checksum_stream.hpp"


BW_BEGIN_NAMESPACE

namespace Mercury
{
	class PublicKeyCipher;
} // end namespace Mercury


/**
 *	This class uses the Mercury::PublicKeyCipher to implement a signature
 *	scheme (RSA-based).
 *
 *	Instances of this class require being composed with a digest checksum
 *	scheme (such as SHAChecksumScheme) using the ChainedChecksumScheme class.
 */
class SignedChecksumScheme : public ChecksumScheme
{
public:
	/**
	 *	This method creates a new smart pointer to this checksum scheme.
	 *
	 *	@param cipher 	The cipher to use.
	 */
	static ChecksumSchemePtr create( Mercury::PublicKeyCipher & cipher )
	{
		return new SignedChecksumScheme( cipher );
	}

	// Overrides from ChecksumScheme
	virtual size_t streamSize() const;
	virtual void readBlob( const void * data, size_t size );
	virtual void addToStream( BinaryOStream & out );

protected:
	virtual bool doVerifyFromStream( BinaryIStream & in );
	virtual void doReset();

	SignedChecksumScheme( Mercury::PublicKeyCipher & cipher );

	/** Destructor. */
	virtual ~SignedChecksumScheme() 
	{}

private:
	MemoryOStream 				buffer_;
	Mercury::PublicKeyCipher & 	cipher_;
};


BW_END_NAMESPACE


#endif // SIGNED_CHECKSUM_SCHEME_HPP

