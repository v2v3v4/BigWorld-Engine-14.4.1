#include "pch.hpp"

#include "cstdmf/debug.hpp"
#include "cstdmf/memory_stream.hpp"

#include "network/sha_checksum_scheme.hpp"
#include "network/elliptic_curve_checksum_scheme.hpp"


namespace BWOpenSSL
{
#include "openssl/bio.h"
#include "openssl/ec.h"
#include "openssl/err.h"
#include "openssl/evp.h"
#include "openssl/pem.h"
#include "openssl/rand.h"
}


namespace // (anonymous)
{

// This key was generated using a secp384r1: NIST/SECG curve over a 384 bit
// prime field.
//
// Generated using OpenSSL like so:
//
// openssl ecparam -name secp384r1 -genkey > ec-private.key
// openssl ec -in ec-private.key -pubout > ec-public.key


const char PRIVATE_KEY[] = 
"-----BEGIN EC PRIVATE KEY-----\n\
MIGkAgEBBDDPo2svU5j9+Cdd17e13D90CIHDGUgdDu1AnPkqXW7PxvaNRujhWloU\n\
4vQ4ND5NquCgBwYFK4EEACKhZANiAASM5SVuxBBdyM89W24v5AM834DYwbDNFv0S\n\
NlNI88jJrEH03VfCCRHXdCBqBpHGRbJ0db6vyABPbQFB4QpoGLvCgaYoC/whUf22\n\
XRCxr7Wpg+Rqdx6JNXlb+IUPyf0fnys=\n\
-----END EC PRIVATE KEY-----";

const char PUBLIC_KEY[] =
"-----BEGIN PUBLIC KEY-----\n\
MHYwEAYHKoZIzj0CAQYFK4EEACIDYgAEjOUlbsQQXcjPPVtuL+QDPN+A2MGwzRb9\n\
EjZTSPPIyaxB9N1XwgkR13QgagaRxkWydHW+r8gAT20BQeEKaBi7woGmKAv8IVH9\n\
tl0Qsa+1qYPkanceiTV5W/iFD8n9H58r\n\
-----END PUBLIC KEY-----";


const char PUBLIC_KEY_OTHER[] =
"-----BEGIN PUBLIC KEY-----\n\
MHYwEAYHKoZIzj0CAQYFK4EEACIDYgAE8Ol7qFGn4FilfY6bRBCkN93+fD2PIMN5\n\
VRuGWibFlDA1UktBPksjSjIvsDg772UxTcvPgDBe6r6rwhlVA0H95/LrVCQB0ctF\n\
YaXqxzDVkfYAQTLrhusJ9Gsl9Bp5FUx0\n\
-----END PUBLIC KEY-----";


const char BLOB_DATA[] = "Here is a blob";
const BW::int32 testInt = 0xdeadbeef;
const BW::string testString( "some data" );

} // end namespace (anonymous)


namespace BWOpenSSL
{

	/**
	 *	This function returns the error string for the last error as reported
	 *	by OpenSSL.
	 */
	const BW::string errorString();

} // end namespace BWOpenSSL


BW_BEGIN_NAMESPACE


/**
 *	Traits type for OpenSSL objects.
 */
template< typename T >
struct OpenSSLObjectTraits
{
	static void deleteObj( T * pObject );
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
	typedef T * ObjectPointerType;
	typedef const T * ConstObjectPointerType;

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
	operator ObjectPointerType()
	{
		return pObject_;
	}


	operator ConstObjectPointerType() const
	{
		return pObject_;
	}


	/**
	 *	Conversion operator for boolean types.
	 */
	operator bool() const
	{
		return (pObject_ != NULL);
	}

private:
	// Disabled
	OpenSSLObject( const OpenSSLObject & other );
	OpenSSLObject & operator== ( const OpenSSLObject & other );


	T * pObject_;
};


TEST( ECDSA_Sign )
{
	static const char MESSAGE[] = "This is my message.";
	unsigned char * digestBuffer = NULL;
	unsigned int digestLength = 0;
	unsigned char * signatureBuffer = NULL;
	unsigned int signatureLength = 0;

	OpenSSLObject< BWOpenSSL::EC_KEY > privateKey;

	{
		OpenSSLObject< BWOpenSSL::BIO > pem( 
			BWOpenSSL::BIO_new( BWOpenSSL::BIO_s_mem() ) );

		BWOpenSSL::BIO_puts( pem, PRIVATE_KEY );

		privateKey = BWOpenSSL::PEM_read_bio_ECPrivateKey(
				pem, NULL, NULL, NULL );

		CHECK( privateKey );

		// OpenSSLObject< BWOpenSSL::BIO > bioStdOut( 
		//		BWOpenSSL::BIO_new_fp( stdout, BIO_NOCLOSE ) );

		// CHECK( EC_KEY_print( bioStdOut, privateKey, 0 ) );

	}

	{ // Compute SHA256 digest
		OpenSSLObject< BWOpenSSL::EVP_MD_CTX > digestContext(
			BWOpenSSL::EVP_MD_CTX_create() );

		digestLength = 32;
		digestBuffer = new unsigned char[digestLength];

		BWOpenSSL::EVP_DigestInit( digestContext, BWOpenSSL::EVP_sha256() );
		BWOpenSSL::EVP_DigestUpdate( digestContext, 
			MESSAGE, sizeof( MESSAGE ) );
		CHECK( BWOpenSSL::EVP_DigestFinal( digestContext, digestBuffer, 
			&digestLength ) );
	}

	{ // Sign the digest
		signatureLength = BWOpenSSL::ECDSA_size( privateKey );
		signatureBuffer = new unsigned char[signatureLength];
		CHECK( BWOpenSSL::ECDSA_sign( 0, digestBuffer, digestLength, 
				signatureBuffer, &signatureLength, privateKey ) );
		
		CHECK( signatureLength );
	}


	OpenSSLObject< BWOpenSSL::EC_KEY > publicKey;

	{ // Read the public key

		OpenSSLObject< BWOpenSSL::BIO > pem( 
			BWOpenSSL::BIO_new( BWOpenSSL::BIO_s_mem() ) );

		BWOpenSSL::BIO_puts( pem, PUBLIC_KEY );

		publicKey = BWOpenSSL::PEM_read_bio_EC_PUBKEY(
				pem, NULL, NULL, NULL );

		CHECK( publicKey );
	}

	// Verify the signature
	{
		
		CHECK( BWOpenSSL::ECDSA_verify( 0, digestBuffer, digestLength,
			signatureBuffer, signatureLength, publicKey ) );
	}


	// Fail to verify a non-signature
	{
		unsigned char * BAD_SIGNATURE = 
			new unsigned char[signatureLength];

		BWOpenSSL::RAND_bytes( BAD_SIGNATURE, signatureLength );

		CHECK( 1 != BWOpenSSL::ECDSA_verify( 0, digestBuffer, digestLength,
			BAD_SIGNATURE, signatureLength, publicKey ) );

		delete [] BAD_SIGNATURE;
	}

	// Fail to verify correct signature, but wrong key
	{
		
		OpenSSLObject< BWOpenSSL::EC_KEY > wrongKey;

		OpenSSLObject< BWOpenSSL::BIO > pem( 
			BWOpenSSL::BIO_new( BWOpenSSL::BIO_s_mem() ) );

		BWOpenSSL::BIO_puts( pem, PUBLIC_KEY_OTHER );

		wrongKey = BWOpenSSL::PEM_read_bio_EC_PUBKEY(
				pem, NULL, NULL, NULL );

		CHECK( wrongKey );

		CHECK( 0 == BWOpenSSL::ECDSA_verify( 0, digestBuffer, digestLength,
			signatureBuffer, signatureLength, wrongKey ) );
	}

	if (digestBuffer)
	{
		delete [] digestBuffer;
	}

	if (signatureBuffer)
	{
		delete [] signatureBuffer;
		signatureBuffer = NULL;
		signatureLength = 0;
	}
}


TEST( EllipticCurveChecksumScheme )
{
	MemoryOStream base;

	ChecksumSchemePtr pScheme = ChainedChecksumScheme::create(
			SHAChecksumScheme::create(),
			EllipticCurveChecksumScheme::create( 
				PRIVATE_KEY, /* isPrivate */ true ) );
	CHECK( pScheme );
	CHECK( pScheme->isGood() );

	{
		ChecksumOStream writer( base, pScheme );
		BinaryOStream & stream = writer;
		stream << testInt << testString;

		stream.addBlob( BLOB_DATA, sizeof( BLOB_DATA ) );

		int sizeBeforeFinalising = writer.size();

		// When writer is destructed, finalise() is called automatically, test
		// here anyway so we can check sizes.
		writer.finalise();

		size_t checksumSize = writer.size() - sizeBeforeFinalising;

		CHECK_EQUAL( pScheme->streamSize(), checksumSize );
	}

	base.rewind();
	pScheme = ChainedChecksumScheme::create(
		SHAChecksumScheme::create(),
		EllipticCurveChecksumScheme::create( PUBLIC_KEY, 
			/* isPrivate */ false ) );

	{
		ChecksumIStream reader( base, pScheme );

		CHECK( !reader.error() );

		int32 testIntActual = 0;
		BW::string testStringActual;
		reader >> testIntActual >> testStringActual;

		CHECK_EQUAL( testIntActual, testInt );
		CHECK_EQUAL( testStringActual, testString );
		CHECK_EQUAL( reader.remainingLength(), int( sizeof( BLOB_DATA ) ) );

		const void * blobData = reader.retrieve( reader.remainingLength() );
		CHECK_EQUAL( memcmp( blobData, BLOB_DATA, sizeof( BLOB_DATA ) ), 0 );

		CHECK( reader.verify() );
		CHECK( reader.remainingLength() == 0 );
		CHECK( !reader.error() );

		CHECK( base.remainingLength() == 0 );
		CHECK( !base.error() );
	}

	// Now check that verifying with another key fails.
	base.rewind();
	pScheme = ChainedChecksumScheme::create(
		SHAChecksumScheme::create(),
		EllipticCurveChecksumScheme::create( PUBLIC_KEY_OTHER, 
			/* isPrivate */ false ) );

	{
		ChecksumIStream reader( base, pScheme );
		int32 testIntActual = 0;
		BW::string testStringActual;
		reader >> testIntActual;

		CHECK( !reader.error() );

		reader >> testStringActual;

		CHECK( !reader.error() );

		reader.retrieve( reader.remainingLength() );

		CHECK( !reader.verify() );
		CHECK( !reader.errorString().empty() );
		CHECK( reader.error() );

		base.finish();
	}
}


BW_END_NAMESPACE


// test_elliptic_curve.cpp
