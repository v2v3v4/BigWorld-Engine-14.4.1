#include "pch.hpp"

#include "elliptic_curve_checksum_scheme.hpp"


namespace BWOpenSSL
{
#include "openssl/bio.h"
#include "openssl/bn.h"
#include "openssl/evp.h"
#include "openssl/ec.h"
#include "openssl/ecdsa.h"
#include "openssl/err.h"
#include "openssl/pem.h"
#include "openssl/rand.h"
}


// -----------------------------------------------------------------------------
// Section: OpenSSL object management conveniences
// -----------------------------------------------------------------------------

// TODO: It'd be nice to clean this up and have this reuseable in places where
// we use OpenSSL.

namespace BWOpenSSL
{

	/**
	 *	This function returns the error string for the last error as reported
	 *	by OpenSSL.
	 */
	const BW::string errorString()
	{
		// These should be loaded somewhere, and also freed somewhere.
		BWOpenSSL::ERR_load_crypto_strings();

		char buf[120]; // length specified by ERR_error_string docs

		BWOpenSSL::ERR_error_string_n( ERR_get_error(), buf, sizeof( buf ) );

		return buf;
	}

} // end namespace BWOpenSSL


BW_BEGIN_NAMESPACE


/**
 *	Traits type for OpenSSL objects.
 */
template< typename T >
struct OpenSSLObjectTraits
{
	/**
	 *	This method deletes the object of the templated OpenSSL type.
	 */
	static void deleteObj( T * pObject )
	{
		BW_STATIC_ASSERT( false, 
			bw_openssl_object_deletion_function_not_defined );
	}
};

/**
 *	This macro specialises the deletion function for a particular OpenSSL type.
 *
 *	@param TYPE 	The OpenSSL type.
 *	@param DELETE 	The deletion function.
 */
#define OPENSSL_SPECIALISE_DELETION_FUNCTION( TYPE, DELETE )				\
	template<>																\
	struct OpenSSLObjectTraits< TYPE >										\
	{																		\
		static void deleteObj( TYPE * pObject )								\
		{ DELETE( pObject ); }												\
	};																		\


// Here we specify which objects have which deletion functions.
OPENSSL_SPECIALISE_DELETION_FUNCTION( BWOpenSSL::BIO, BWOpenSSL::BIO_free_all )
OPENSSL_SPECIALISE_DELETION_FUNCTION( BWOpenSSL::EC_KEY,
	BWOpenSSL::EC_KEY_free )
OPENSSL_SPECIALISE_DELETION_FUNCTION( BWOpenSSL::BIGNUM, BWOpenSSL::BN_free )
OPENSSL_SPECIALISE_DELETION_FUNCTION( BWOpenSSL::EVP_MD_CTX,
	BWOpenSSL::EVP_MD_CTX_destroy ) // Non-conforming anarchist.
#undef OPENSSL_SPECIALISE_DELETION_FUNCTION


/**
 *	This template class holds a pointer to an OpenSSL object, and deletes it
 *	appropriately when the instance goes out of scope.
 *
 *	A conversion operator exists so that instances can be used wherever the
 *	underlying pointer type is expected.
 */
template< typename T >
class OpenSSLObject
{
public:
	typedef T * 		ObjectPointerType;
	typedef const T * 	ConstObjectPointerType;

	/**
	 *	Constructor.
	 *
	 *	@param pObject 	The OpenSSL object.
	 */
	OpenSSLObject( T * pObject = NULL ) :
		pObject_( pObject )
	{}


	/**
	 *	Destructor.
	 */
	~OpenSSLObject()
	{
		if (pObject_)
		{
			OpenSSLObjectTraits< T >::deleteObj( pObject_ );
			pObject_ = NULL;
		}
	}


	/**
	 *	Assignment operator.
	 */
	OpenSSLObject & operator=( T * pObject )
	{
		if (pObject_)
		{
			OpenSSLObjectTraits< T >::deleteObj( pObject_ );
		}
		pObject_ = pObject;
		return *this;
	}


	/**
	 *	Conversion operator for the wrapped object.
	 */
	operator ObjectPointerType() 			{ return pObject_; }
	operator ConstObjectPointerType() const { return pObject_; }

	/**
	 *	Conversion operator for boolean types.
	 */
	operator bool() const 					{ return (pObject_ != NULL); }

private:
	// Disabled
	OpenSSLObject( const OpenSSLObject & other );
	OpenSSLObject & operator== ( const OpenSSLObject & other );


	T * pObject_;
};


// -----------------------------------------------------------------------------
// Section: EllipticCurveChecksumScheme implementation
// -----------------------------------------------------------------------------


/**
 *	This class implements EllipticCurveChecksumScheme.
 */
class EllipticCurveChecksumScheme::Impl
{
public:
	Impl( const BW::string & keyPEM, bool isPrivate, BW::string & errorString );
	~Impl();

	/**
	 *	This method returns whether the scheme is in a good state, ready for
	 *	signature operations.
	 */
	bool isGood() const { return pKey_; }

	/**
	 *	This method implements the EllipticCurveChecksumScheme::streamSize()
	 *	method.
	 * */
	size_t streamSize() const { return maxSignatureSize_; }

	void readBlob( const void * data , size_t size );
	void addToStream( BinaryOStream & out );
	bool verifyFromStream( BinaryIStream & in );

	void reset();


private:

	OpenSSLObject< BWOpenSSL::EC_KEY > 	pKey_;

	// These two values are precomputed values that speed up signing.
	OpenSSLObject< BWOpenSSL::BIGNUM >	kinv_;	
	OpenSSLObject< BWOpenSSL::BIGNUM >	rp_;

	bool								isPrivate_;

	MemoryOStream						digest_;
	uint8 *								signatureBuffer_;
	uint 								maxSignatureSize_;

	BW::string &						errorString_;
};


/**
 *	Factory method.
 *
 *	@param keyPEM 		The key string in PEM format.
 *	@param isPrivate 	Whether the key is a public or private key.
 *
 *	@return 			A new ECSignature scheme object.
 */
ChecksumSchemePtr EllipticCurveChecksumScheme::create(
		const BW::string & keyPEM, bool isPrivate )
{
	return new EllipticCurveChecksumScheme( keyPEM, isPrivate );
}


/**
 *	Constructor.
 *
 *	@param keyPEM 		The key string in PEM format.
 *	@param isPrivate 	Whether the key is a public or private key.
 */
EllipticCurveChecksumScheme::EllipticCurveChecksumScheme(
			const BW::string & keyPEM, bool isPrivate ) :
		pImpl_( new Impl( keyPEM, isPrivate, errorString_ ) )
{}


/**
 *	Constructor.
 *
 *	@param keyPEM 		The key string in PEM format.
 *	@param isPrivate 	Whether the key is a public or private key.
 *	@param errorString 	The error string object to fill in the case of error.
 */
EllipticCurveChecksumScheme::Impl::Impl( const BW::string & keyPEM,
			bool isPrivate, BW::string & errorString ) :
		pKey_(),
		kinv_(),
		rp_(),
		isPrivate_( isPrivate ),
		digest_(),
		signatureBuffer_( NULL ),
		maxSignatureSize_( 0U ),
		errorString_( errorString )
{
	OpenSSLObject< BWOpenSSL::BIO > pBinaryBuffer(
			BWOpenSSL::BIO_new( BWOpenSSL::BIO_s_mem() ) );

	BWOpenSSL::BIO_write( pBinaryBuffer, keyPEM.data(), 
		static_cast<int>(keyPEM.size()) );

	if (isPrivate)
	{
		pKey_ = BWOpenSSL::PEM_read_bio_ECPrivateKey(
			pBinaryBuffer, NULL, NULL, NULL );
	}
	else
	{
		pKey_ = BWOpenSSL::PEM_read_bio_EC_PUBKEY(
			pBinaryBuffer, NULL, NULL, NULL );
	}

	if (!pKey_)
	{
		errorString_ = "Could not read key: " + BWOpenSSL::errorString();
		pKey_ = NULL;
		return;
	}

	BWOpenSSL::BIGNUM * kinv = NULL;
	BWOpenSSL::BIGNUM * rp = NULL;

	if (!BWOpenSSL::ECDSA_sign_setup( pKey_, /* BN_CTX * ctx */ NULL, 
			&kinv, &rp ))
	{
		errorString_ = "Could not setup key computed values: " + 
			BWOpenSSL::errorString();
		pKey_ = NULL;
		return;
	}

	kinv_ = kinv;
	rp_ = rp;

	maxSignatureSize_ = uint( ECDSA_size( pKey_ ) );
	MF_ASSERT( maxSignatureSize_ > 0 );

	signatureBuffer_ = new unsigned char[maxSignatureSize_];
}


/*
 *	Override from ChecksumScheme.
 */
void EllipticCurveChecksumScheme::doReset()
{
	pImpl_->reset();
}


/**
 *	This method implements the EllipticCurveChecksumScheme::doReset() method.
 */
void EllipticCurveChecksumScheme::Impl::reset()
{
	digest_.reset();
}


/**
 *	Destructor.
 */
EllipticCurveChecksumScheme::~EllipticCurveChecksumScheme()
{
	delete pImpl_;
}


/**
 *	Destructor.
 */
EllipticCurveChecksumScheme::Impl::~Impl()
{
	delete [] signatureBuffer_;
}


/*
 *	Override from ChecksumScheme.
 */
size_t EllipticCurveChecksumScheme::streamSize() const
{
	return pImpl_->streamSize();
}


/*
 *	Override from ChecksumScheme.
 */
void EllipticCurveChecksumScheme::readBlob( const void * data, size_t size )
{
	pImpl_->readBlob( data, size );
}


/**
 *	This method implements the EllipticCurveChecksumScheme::readBlob() method.
 *
 *	@param data 	The data to read.
 *	@param size 	The size of the data to read.
 */
void EllipticCurveChecksumScheme::Impl::readBlob( const void * data,
		size_t size )
{
	digest_.addBlob( data, int( size ) );
}


/*
 *	Override from ChecksumScheme.
 */
void EllipticCurveChecksumScheme::addToStream( BinaryOStream & out )
{
	pImpl_->addToStream( out );
}


/**
 *	This method implements the
 *	EllipticCurveChecksumScheme::doVerifyFromStream() method.
 *
 *	@param out 	The output stream.
 */
void EllipticCurveChecksumScheme::Impl::addToStream( BinaryOStream & out )
{
	if (!isPrivate_)
	{
		errorString_ = "Attempted to sign with public key";
		return;
	}

	uint signatureSizeReported = maxSignatureSize_;

	if (-1 == BWOpenSSL::ECDSA_sign_ex( 0, 
			(const unsigned char *)digest_.retrieve( digest_.size() ), 
			digest_.size(), 
			signatureBuffer_, &signatureSizeReported, kinv_, rp_, pKey_ ) )
	{
		errorString_ = BWOpenSSL::errorString();
	}

	// Pad out the rest with random data, the digest length itself is encoded
	// in ASN.1, no need for us to embed it.
	BWOpenSSL::RAND_bytes( signatureBuffer_ + signatureSizeReported,
		maxSignatureSize_ - signatureSizeReported );

	out.addBlob( signatureBuffer_, maxSignatureSize_ );
}


/*
 *	Override from ChecksumScheme.
 */
bool EllipticCurveChecksumScheme::doVerifyFromStream( BinaryIStream & in )
{
	return pImpl_->verifyFromStream( in );
}


/**
 *	This method implements the
 *	EllipticCurveChecksumScheme::doVerifyFromStream() method.
 *
 *	@param in 	The input stream.
 */
bool EllipticCurveChecksumScheme::Impl::verifyFromStream( BinaryIStream & in )
{
	int res = BWOpenSSL::ECDSA_verify( 0,
		(const unsigned char *)digest_.retrieve( digest_.size() ), 
		digest_.size(),
		(unsigned char * )in.retrieve( maxSignatureSize_ ), maxSignatureSize_,
		pKey_ );

	if (res == 0)
	{
		errorString_ = "Signature mismatch";
	}
	else if (res == -1)
	{
		errorString_ = "Error while verifying EC signature: " + 
			BWOpenSSL::errorString();
	}

	return (res == 1);
}


BW_END_NAMESPACE


// elliptic_curve_checksum_scheme.cpp
