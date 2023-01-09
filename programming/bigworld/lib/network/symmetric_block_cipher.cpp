#include "pch.hpp"

#include "symmetric_block_cipher.hpp"

#if !defined( USE_OPENSSL )

BW_BEGIN_NAMESPACE


/*
 *	This version of SymmetricBlockCipher::create is used when OpenSSL support
 *	is disabled.
 *
 */
BlockCipherPtr SymmetricBlockCipher::create(
		size_t keySize /* = DEFAULT_KEY_SIZE */ )
{
	return new NullCipher();
}


BW_END_NAMESPACE

#else // defined( USE_OPENSSL )


namespace BWOpenSSL
{
#include "openssl/blowfish.h"
#include "openssl/rand.h"
} // end namespace BWOpenSSL

BW_BEGIN_NAMESPACE

namespace Mercury
{


class SymmetricBlockCipher::Impl
{
public:
	static const size_t BLOCK_SIZE = 64 / NETWORK_BITS_PER_BYTE;	

	Impl( size_t keySize );
	Impl( const BlockCipher::Key & keyString );

	~Impl() {}

	// Implementations of SymmetricBlockCipher methods.

	bool encryptBlock( const uint8 * pInputBuffer,
		uint8 * pOutputBuffer );
	bool decryptBlock( const uint8 * pInputBuffer,
		uint8 * pOutputBuffer );
	const BlockCipher::Key & keyString() const	{ return keyString_; }
	size_t blockSize() const 					{ return BLOCK_SIZE; }

private:
	void init();

	BlockCipher::Key keyString_;
	BWOpenSSL::BF_KEY bfKey_;
};


/**
 *	Constructor with a generated key.
 *
 *	@param keySize 	The desired key size.
 */
SymmetricBlockCipher::Impl::Impl( size_t keySize ) :
		keyString_( keySize, '\0' ),
		bfKey_()
{
	BWOpenSSL::RAND_bytes( reinterpret_cast< unsigned char * >( 
			const_cast< char * >( keyString_.data() ) ), 
		static_cast<int>( keySize ) );

	this->init();
}


/**
 *	Constructor with a predefined key.
 *
 *	@param keyString 	The key string.
 */
SymmetricBlockCipher::Impl::Impl( const Key & keyString ) :
		keyString_( keyString ),
		bfKey_()
{
#if !defined( __APPLE__ )
	// The default implementation of combineBlocks() relies on the
	// block size being 64-bit (since it uses uint64's for the XOR operation).
	// If a larger block size is used, the implementation of the XOR operation
	// needs to be modified and this assertion can be changed.
	BW_STATIC_ASSERT( BLOCK_SIZE == sizeof( uint64 ),
		SymmetricBlockCipher_Expected_64_bit_BLOCK_SIZE );
#endif // !defined( __APPLE__ )

	this->init();
}


/**
 *	This method initialises the Blowfish key structure from the key string.
 */
void SymmetricBlockCipher::Impl::init()
{
	BWOpenSSL::BF_set_key( &bfKey_, static_cast<int>(keyString_.size()), 
		reinterpret_cast< const unsigned char * >( keyString_.data() ) );
}


/*
 *	Override from BlockCipher.
 */
bool SymmetricBlockCipher::Impl::encryptBlock( const uint8 * pInputBuffer,
		uint8 * pOutputBuffer )
{
	if (keyString_.empty())
	{
		return false;
	}

	BWOpenSSL::BF_ecb_encrypt( pInputBuffer, pOutputBuffer, &bfKey_,
		BF_ENCRYPT );
	return true;
}


/*
 *	Override from BlockCipher.
 */
bool SymmetricBlockCipher::Impl::decryptBlock( const uint8 * pInputBuffer,
		uint8 * pOutputBuffer )
{
	if (keyString_.empty())
	{
		return false;
	}

	BWOpenSSL::BF_ecb_encrypt( pInputBuffer, pOutputBuffer, &bfKey_,
		BF_DECRYPT );
	return true;
}


/**
 *	This method is a factory method for creating a symmetric block cipher with
 *	a generated key.
 *
 *	@param keySize 	The size of the key to generate, which must be between
 *					MIN_KEY_SIZE and MAX_KEY_SIZE bytes.
 *
 *	@return 		NULL if the key size is invalid, otherwise a new instance
 *					of a SymmetricBlockCipher.
 */
BlockCipherPtr SymmetricBlockCipher::create(
		size_t keySize /* = DEFAULT_KEY_SIZE */ )
{
	if (keySize < MIN_KEY_SIZE || keySize > MAX_KEY_SIZE)
	{
		ERROR_MSG( "SymmetricBlockCipher::create: "
				"Requested invalid key size of %zu bytes\n",
			keySize );
		return BlockCipherPtr();
	}

	return new SymmetricBlockCipher( keySize );
}


/**
 *	This method is a factory method for creating a symmetric block cipher with
 *	a known key.
 *
 *	@param keyString 	The key string, which must have a length of between
 *						MIN_KEY_SIZE and MAX_KEY_SIZE bytes.
 *
 *	@return 			NULL if the key string is invalid, otherwise a new
 *						instance of a SymmetricBlockCipher.
 */
BlockCipherPtr SymmetricBlockCipher::create(
		const BlockCipher::Key & keyString )
{
	if (keyString.size() < MIN_KEY_SIZE || keyString.size() > MAX_KEY_SIZE)
	{
		ERROR_MSG( "SymmetricBlockCipher::create: "
				"Requested invalid known key of invalid size: %zu\n",
			keyString.size() );
		return BlockCipherPtr();
	}
	return BlockCipherPtr( new SymmetricBlockCipher( keyString ) );
}


/** 
 *	Constructor with a generated key.
 */
SymmetricBlockCipher::SymmetricBlockCipher( 
			size_t keySize ) :
		pImpl_( new Impl( keySize ) )
{
}


/**
 *	Constructor with existing key.
 */
SymmetricBlockCipher::SymmetricBlockCipher( const BlockCipher::Key & key ) :
	pImpl_( new Impl( key ) )
{
}


/**
 *	Destructor.
 */
SymmetricBlockCipher::~SymmetricBlockCipher()
{
	bw_safe_delete( pImpl_ );
}


/*
 *	Override from BlockCipher.
 */
bool SymmetricBlockCipher::encryptBlock( 
		const uint8 * pInputBuffer, uint8 * pOutputBuffer )
{
	return pImpl_->encryptBlock( pInputBuffer, pOutputBuffer );
}


/*
 *	Override from BlockCipher.
 */
bool SymmetricBlockCipher::decryptBlock(
		const uint8 * pInputBuffer, uint8 * pOutputBuffer )
{
	return pImpl_->decryptBlock( pInputBuffer, pOutputBuffer );
}


/*
 *	Override from BlockCipher.
 */
const BlockCipher::Key & SymmetricBlockCipher::key() const
{
	return pImpl_->keyString();
}


/*
 *	Override from BlockCipher.
 */
size_t SymmetricBlockCipher::blockSize() const
{
	return pImpl_->blockSize();
}


} // end namespace Mercury

BW_END_NAMESPACE

#endif // defined( USE_OPENSSL )

// symmetric_block_cipher.cpp
