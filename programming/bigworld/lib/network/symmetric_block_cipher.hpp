#ifndef SYMMETRIC_BLOCK_CIPHER_HPP
#define SYMMETRIC_BLOCK_CIPHER_HPP


#include "network/basictypes.hpp"
#include "network/block_cipher.hpp"


BW_BEGIN_NAMESPACE


namespace Mercury
{

/**
 *	This class implements a symmetric block cipher with a shared private key.
 */
class SymmetricBlockCipher : public BlockCipher
{
public:
	static const size_t MIN_KEY_SIZE = 32 / NETWORK_BITS_PER_BYTE;
	static const size_t MAX_KEY_SIZE = 448 / NETWORK_BITS_PER_BYTE;
	static const size_t DEFAULT_KEY_SIZE = 128 / NETWORK_BITS_PER_BYTE;

	static BlockCipherPtr create( size_t keySize = DEFAULT_KEY_SIZE );

	static BlockCipherPtr create( const Key & keyString );
	
	// Overrides from BlockCipher
	virtual bool encryptBlock( const uint8 * pInputBuffer,
		uint8 * pOutputBuffer );
	virtual bool decryptBlock( const uint8 * pInputBuffer,
		uint8 * pOutputBuffer );

	virtual const Key & key() const;

	virtual size_t blockSize() const;

private:
	explicit SymmetricBlockCipher( size_t keySize );
	explicit SymmetricBlockCipher( const Key & key );
	virtual ~SymmetricBlockCipher();


	class Impl;
	Impl * pImpl_;
};


} // end namespace Mercury

BW_END_NAMESPACE

#endif // SYMMETRIC_BLOCK_CIPHER_HPP
