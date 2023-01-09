#include "pch.hpp"

#include "endpoint.hpp"
#include "public_key_cipher.hpp"

#include "cstdmf/guard.hpp"
#include "cstdmf/timestamp.hpp"

#if defined( USE_OPENSSL )
// Namespace to wrap OpenSSL symbols
namespace BWOpenSSL
{
#include "openssl/bio.h"
#include "openssl/err.h"
#include "openssl/pem.h"
#include "openssl/rand.h"
#include "openssl/rsa.h"
}
#endif // defined( USE_OPENSSL )

BW_BEGIN_NAMESPACE

#if defined( USE_OPENSSL )

// See RSA_public_encrypt(3) for the origin of the magic 41.
// RSA_PKCS1_OAEP_PADDING padding mode is used.
#define	RSA_PADDING	41	


#if defined( EMSCRIPTEN )
namespace // (anonymous)
{

class OpenSSLInitialiser
{
public:
	OpenSSLInitialiser();
	~OpenSSLInitialiser();
};


OpenSSLInitialiser::OpenSSLInitialiser()
{
	// TODO: Emscripten requires this as OpenSSL has no access to /dev/random
	// under JavaScript.

	// Need to find some good way of feeding entropy to JavaScript clients. Not
	// a whole lot exists when the page loads initially. Could use mouse
	// events, but not much available initially. Could have some entropy seed
	// data sent from the server, but this needs to be transmitted securely.

	::srand( uint32( timestamp() & 0xFFFFFFFF ) );

	while (!BWOpenSSL::RAND_status())
	{
		// TODO: For now, just get this working, and acknowledge that this is
		// *not* cryptographically secure.
		int entropy = rand();

		BWOpenSSL::RAND_add( &entropy, sizeof( entropy ), 32 );
	}
	
	BWOpenSSL::ERR_load_crypto_strings();
}


OpenSSLInitialiser::~OpenSSLInitialiser()
{
	BWOpenSSL::ERR_free_strings();
}

OpenSSLInitialiser s_openSSLInitialiser;


} // end namespace (anonymous)


#endif // defined( EMSCRIPTEN )

namespace Mercury
{
// OpenSSL RSA PublicKeyCipher implementation
class PublicKeyCipherImpl : public PublicKeyCipher
{
	friend class PublicKeyCipher;

	explicit PublicKeyCipherImpl( bool isPrivate ) :
		PublicKeyCipher( isPrivate ),
		pRSA_( NULL )
	{}


	~PublicKeyCipherImpl()
	{
		RSA_free( pRSA_ );
		pRSA_ = NULL;
	}


	/**
	 *	This method returns the last OpenSSL error message.
	 *	The char* is static, so be a little careful with it.
	 */
	static const char * lastError()
	{
		static bool errorStringsLoaded = false;

		if (!errorStringsLoaded)
		{
			BWOpenSSL::ERR_load_crypto_strings();

			errorStringsLoaded = true;
		}

		return BWOpenSSL::ERR_error_string( BWOpenSSL::ERR_get_error(), 0 );
	}


	/**
	 *	This method generates our OpenSSL RSA from the given key.
	 *
	 *	@param	rKey		The Key to generate from
	 *
	 *	@return	true if key generation succeeded, false if it failed
	 */
	bool readKey( const Key & rKey )
	{
		// Clean up an existing key first
		if (pRSA_ != NULL)
		{
			RSA_free( pRSA_ );
			pRSA_ = NULL;
		}

		// Construct an in-memory BIO to the key data
		BWOpenSSL::BIO *bio = BWOpenSSL::BIO_new( BWOpenSSL::BIO_s_mem() );
		BWOpenSSL::BIO_puts( bio, rKey.c_str() );

		// Read the key into the RSA struct
		pRSA_ = this->hasPrivate() ?
			BWOpenSSL::PEM_read_bio_RSAPrivateKey( bio, NULL, NULL, NULL ) :
			BWOpenSSL::PEM_read_bio_RSA_PUBKEY( bio, NULL, NULL, NULL );

		if (pRSA_ == NULL)
		{
			ERROR_MSG( "PublicKeyCipherImpl::readKey: "
					"Failed to read %s key: %s\n",
				this->type(), PublicKeyCipherImpl::lastError() );
		}

		BWOpenSSL::BIO_free( bio );

		return pRSA_ != NULL;
	}


	/**
	 *	This method returns the size of the key in the OpenSSL RSA in bytes.
	 */
	int keySize() const
	{
		return BWOpenSSL::RSA_size( pRSA_ );
	}


	/**
	 *	This method fills in rStringifiedKey with a stringified version of
	 *	our public key
	 */
	void stringifyKey( Key & rStringifiedKey )
	{
		BWOpenSSL::BIO * bio = BWOpenSSL::BIO_new( BWOpenSSL::BIO_s_mem() );
		BWOpenSSL::PEM_write_bio_RSA_PUBKEY( bio, pRSA_ );

		char buf[ 1024 ];
		while (BWOpenSSL::BIO_gets( bio, buf, sizeof( buf ) - 1 ) > 0)
		{
			rStringifiedKey.append( buf );
		}

		BWOpenSSL::BIO_free( bio );
	}


	/// The signature of RSA encryption/decryption functions
	typedef int (*EncryptionFunctionPtr)( int flen, const unsigned char * from,
		unsigned char * to, BWOpenSSL::RSA * pRSA, int padding );

	/**
	 *	This method encrypts the given clearStream into the given cipherStream.
	 *	It returns the size of the encrypted stream, or -1 on error.
	 */
	int encryptStream( BinaryIStream & clearStream,
		BinaryOStream & cipherStream, bool usePrivateKey ) const
	{
		EncryptionFunctionPtr pEncryptionFunc = usePrivateKey ?
			BWOpenSSL::RSA_private_encrypt : BWOpenSSL::RSA_public_encrypt;
		int padding = usePrivateKey ?
			RSA_PKCS1_PADDING : RSA_PKCS1_OAEP_PADDING;

		// Size of the modulus used in the RSA algorithm.
		const int rsaModulusSize = BWOpenSSL::RSA_size( pRSA_ );	

		// If the size of cleartext is equals to or greater than (the size 
		// of RSA modulus - RSA_PADDING), cleartext will be split into smaller 
		// parts.  These smaller parts are encrypted and written to 
		// cipherStream.  

		// Ensures each part to be encrypted is less than the size of the RSA 
		// modulus minus padding.  partSize must be less than 
		// (rsaModulusSize - RSA_PADDING).
		int partSize = rsaModulusSize - RSA_PADDING - 1;


		// Encrypt clearText.
		int encryptLen = 0;
		while (clearStream.remainingLength())
		{
			// If remaining cleartext to be encrypted is too large, 
			// we retrieve what can be encrypted in a block.
			int numBytesToRetrieve =
				std::min( partSize, clearStream.remainingLength() );

			unsigned char * clearText = 
				(unsigned char*)clearStream.retrieve( numBytesToRetrieve );

			unsigned char * cipherText =
				(unsigned char *)cipherStream.reserve( rsaModulusSize );

			int curEncryptLen = (*pEncryptionFunc)( numBytesToRetrieve,
				clearText, cipherText, pRSA_, padding );

			MF_ASSERT( curEncryptLen == rsaModulusSize );

			encryptLen += curEncryptLen; 
		}


		return encryptLen;
	}


	/**
	 *	This method decrypts the given cipherStream into the given clearStream.
	 *	It returns the size of the decrypted stream, or -1 on error.
	 */
	int decryptStream( BinaryIStream & cipherStream,
		BinaryOStream & clearStream, bool usePrivateKey ) const
	{
		EncryptionFunctionPtr pEncryptionFunc = usePrivateKey ?
			BWOpenSSL::RSA_private_decrypt : BWOpenSSL::RSA_public_decrypt;
		int padding = usePrivateKey ?
			RSA_PKCS1_OAEP_PADDING : RSA_PKCS1_PADDING;

		// Size of the modulus used in the RSA algorithm.
		const int rsaModulusSize = BWOpenSSL::RSA_size( pRSA_ );	

		if (cipherStream.remainingLength() % rsaModulusSize != 0)
		{
			// probably not encrypted
			return -1;
		}

		// Decrypt the encrypted blocks and write cleartext to clearStream.
		unsigned char * buf = (unsigned char *)bw_alloca( rsaModulusSize );

		while (cipherStream.remainingLength())	
		{
			unsigned char * cipherText =
				(unsigned char *)cipherStream.retrieve( rsaModulusSize  );

			if (cipherStream.error())
			{
				return -1;
			}	

			int bytesDecrypted = (*pEncryptionFunc)( rsaModulusSize,
				cipherText, buf, pRSA_, padding );

			if (bytesDecrypted < 0)
			{
				return -1;
			}

			clearStream.addBlob( buf, bytesDecrypted );
		}

		return clearStream.size();
	}


	BWOpenSSL::RSA * pRSA_;

};

} // namespace Mercury

#else // No Crypto library defined
namespace Mercury
{
// OpenSSL RSA PublicKeyCipher implementation
class PublicKeyCipherImpl : public PublicKeyCipher
{
	friend class PublicKeyCipher;

	explicit PublicKeyCipherImpl( bool isPrivate ) :
		PublicKeyCipher( isPrivate )
	{}


	~PublicKeyCipherImpl()
	{}


	/**
	 *	This method returns the last error message.
	 *	The char* is static, so be a little careful with it.
	 */
	static const char * lastError()
	{
		// This is always the problem.
		return "No cryptographic library available";
	}


	/**
	 *	This method returns failure to generate a cipher object
	 *
	 *	@return false
	 */
	bool readKey( const Key & rKey )
	{
		return false;
	}


	/**
	 *	This method returns -1 because we have no key
	 */
	int keySize() const
	{
		return -1;
	}


	/**
	 *	This method does nothing because we have no key
	 */
	void stringifyKey( Key & rStringifiedKey )
	{
	}


	/**
	 *	This method encrypts the given clearStream into the given cipherStream.
	 *	It returns the size of the encrypted stream, or -1 on error.
	 */
	int encryptStream( BinaryIStream & clearStream,
		BinaryOStream & cipherStream, bool usePrivateKey ) const
	{
		return -1;
	}


	/**
	 *	This method decrypts the given cipherStream into the given clearStream.
	 *	It returns the size of the decrypted stream, or -1 on error.
	 */
	int decryptStream( BinaryIStream & cipherStream,
		BinaryOStream & clearStream, bool usePrivateKey ) const
	{
		return -1;
	}

};

} // namespace Mercury

#endif // defined( USE_OPENSSL )

namespace Mercury
{

// -----------------------------------------------------------------------------
// Section: PublicKeyCipher
// -----------------------------------------------------------------------------

/**
 *	PublicKeyCipher Factory static method
 *
 *	@param	hasPrivate	Whether or not this PublicKeyCipher has a private key
 */
/*static*/ PublicKeyCipher * PublicKeyCipher::create( bool isPrivate )
{
	return new PublicKeyCipherImpl( isPrivate );
}


/**
 *	This method returns the size of the key in bytes.
 */
int PublicKeyCipher::size() const
{
	return isGood_ ? this->impl().keySize() : -1;
}


/**
 *  Sets the key to be used by this object.
 */
bool PublicKeyCipher::setKey( const BW::string & key )
{
	if (this->impl().readKey( key ))
	{
		isGood_ = true;
		this->setReadableKey();
		return true;
	}
	else
	{
		isGood_ = false;
		return false;
	}
}


/**
 *  This method encrypts a string using this key.  It returns the size of the
 *  encrypted stream, or -1 on error.
 */
int PublicKeyCipher::encrypt( BinaryIStream & clearStream,
	BinaryOStream & cipherStream, bool usePrivateKey ) const
{
	return this->impl().encryptStream( clearStream, cipherStream,
		usePrivateKey );
}


/**
 *  This method decrypts data encrypted with this key.  It returns the length of
 *  the decrypted stream, or -1 on error.
 */
int PublicKeyCipher::decrypt( BinaryIStream & cipherStream,
	BinaryOStream & clearStream, bool usePrivateKey ) const
{
	int result = this->impl().decryptStream( cipherStream, clearStream,
		usePrivateKey );

	if (result == 0)
	{
		// Silently fail if we decrypted nothing. (empty input?)
		return -1;
	}
	else if (result == -1)
	{
		if (cipherStream.error())
		{
			// decryptStream tried to read off the end of the cipherStream
			ERROR_MSG( "PublicKeyCipher::decrypt: "
				"Not enough data on stream for encrypted data chunk\n" );
		}
		else
		{
			// Something went wrong in decryption itself
			cipherStream.finish();
			TRACE_MSG( "PublicKeyCipher::decrypt: "
				"Error encountered while decrypting stream.\n" );
		}
	}
	return result;
}


/**
 *  This method writes the standard base64 encoded representation of the public
 *  key into readableKey_.
 */
void PublicKeyCipher::setReadableKey()
{
	readableKey_.clear();
	this->impl().stringifyKey( readableKey_ );
}


/**
 *	This method gets a const PublicKeyCipherImpl from an PublicKeyCipher
 */
inline
const PublicKeyCipherImpl & PublicKeyCipher::impl() const
{
	return static_cast< const PublicKeyCipherImpl & >( *this );
}


/**
 *	This method gets an PublicKeyCipherImpl from an PublicKeyCipher
 */
inline
PublicKeyCipherImpl & PublicKeyCipher::impl()
{
	return static_cast< PublicKeyCipherImpl & >( *this );
}


} // namespace Mercury

BW_END_NAMESPACE

// public_key_cipher.cpp
