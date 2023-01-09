#ifndef BLOCK_CIPHER_HPP
#define BLOCK_CIPHER_HPP

#include "cstdmf/bw_namespace.hpp"
#include "cstdmf/bw_string.hpp"
#include "cstdmf/smartpointer.hpp"

#include "network/basictypes.hpp"

#include <string.h>

BW_BEGIN_NAMESPACE

namespace Mercury
{

/**
 *	Abstract class for block ciphers.
 */
class BlockCipher : public ReferenceCount
{
public:
	typedef BW::string Key;
	
	/**
	 *	This function encrypts the given block.
	 *
	 *	@param	pInputBuffer	A buffer of bytes to encrypt from.
	 *	@param	pOutputBuffer	A buffer of bytes to encrypt to.
	 *
	 */
	virtual bool encryptBlock( const uint8 * pInputBuffer,
		uint8 * pOutputBuffer ) = 0;


	/**
	 *	This function decrypts the given block.
	 *
	 *	@param	pInputBuffer	A buffer of bytes to decrypt from.
	 *	@param	pOutputBuffer	A buffer of bytes to decrypt to.
	 */
	virtual bool decryptBlock( const uint8 * pInputBuffer,
		uint8 * pOutputBuffer ) = 0; 

	/**
	 *	This method returns the key used.
	 */
	virtual const Key & key() const = 0;


	inline const BW::string readableKey() const;


	/**
	 *	This method returns the block size in bytes.
	 */
	virtual size_t blockSize() const = 0;


	inline void combineBlocks( const uint8 * pIn1, 
		const uint8 * pIn2, uint8 * pOut );

protected:
	/**
	 *	Constructor.
	 */
	BlockCipher() :
			ReferenceCount()
	{}

	/** Destructor. */
	virtual ~BlockCipher() {}

private:
};

typedef SmartPointer< BlockCipher > BlockCipherPtr;


/**
 *	This class implements a no-op block cipher.
 */
class NullCipher : public BlockCipher
{
public:
	static BlockCipherPtr create( size_t blockSize = 8 )
	{
		return BlockCipherPtr( new NullCipher( blockSize ) );
	}

	/** Override from BlockCipher. */
	virtual bool encryptBlock( const unsigned char * pInputBuffer,
		uint8 * pOutputBuffer )
	{
		memcpy( pOutputBuffer, pInputBuffer, blockSize_ );
		return true;
	}

	/** Override from BlockCipher. */
	virtual bool decryptBlock( const unsigned char * pInputBuffer,
		uint8 * pOutputBuffer )
	{
		memcpy( pOutputBuffer, pInputBuffer, blockSize_ );
		return true;
	}

	virtual const Key & key() const { return key_; }


	virtual size_t blockSize() const { return blockSize_; }

private:
	/** Destructor. */
	virtual ~NullCipher()
	{}


	/**
	 *	Constructor.
	 *
	 *	@param blockSize 	The block size.
	 */
	NullCipher( size_t blockSize = 8 ) : 
		BlockCipher(),
		key_(),
		blockSize_( blockSize )
	{}

	Key key_;
	size_t blockSize_;
};

} // end namespace Mercury

#include "block_cipher.ipp"

BW_END_NAMESPACE

#endif // BLOCK_CIPHER_HPP
