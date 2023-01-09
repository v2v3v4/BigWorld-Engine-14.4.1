#include "pch.hpp"

#include "encryption_stream_filter.hpp"
#include "tcp_channel.hpp"

#include "network/symmetric_block_cipher.hpp"


BW_BEGIN_NAMESPACE


namespace Mercury
{

/**
 *	Factory method.
 *
 *	@param stream 	The network steram to filter for.
 *	@param pCipher 	The block cipher to use for encryption.
 */
/* static */ StreamFilterPtr EncryptionStreamFilter::create(
		NetworkStream & stream, BlockCipherPtr pCipher )
{
	return new EncryptionStreamFilter( stream, pCipher );
}


/**
 *	Constructor.
 *
 *	@param stream 	The underlying network stream.
 *	@param pCipher 	The block cipher to use for encryption.
 */
EncryptionStreamFilter::EncryptionStreamFilter( NetworkStream & stream,
			BlockCipherPtr pCipher ) :
		StreamFilter( stream ),
		pBlockCipher_( pCipher ),
		receiveBuffer_(),
		expectedReceiveFrameLength_( 0 ),
		previousReceivedBlock_( new uint8[ pBlockCipher_->blockSize() ]() ),
		previousSentBlock_( new uint8[ pBlockCipher_->blockSize() ]() )
{
}


/**
 *	Destructor.
 */
EncryptionStreamFilter::~EncryptionStreamFilter()
{
	bw_safe_delete_array( previousReceivedBlock_ );
	bw_safe_delete_array( previousSentBlock_ );
}


/*
 *	Override from StreamFilter.
 */
bool EncryptionStreamFilter::writeFrom( BinaryIStream & input, bool shouldCork )
{
	const size_t BLOCK_SIZE = pBlockCipher_->blockSize();

	uint8 * bufferForPaddedBlocks =
		reinterpret_cast<uint8*>(alloca( BLOCK_SIZE ));
	uint8 * out = reinterpret_cast<uint8*>(alloca( BLOCK_SIZE ));
	uint8 * xored = reinterpret_cast<uint8*>(alloca( BLOCK_SIZE ));

	size_t remainingLength = 0;

	while ((remainingLength = static_cast<size_t>(input.remainingLength())) > 0)
	{
		const uint8 * clear = NULL;

		// We should cork the send if the entire input is to be corked, or if
		// we are the last block to be sent.
		bool shouldCorkBlock = (shouldCork || (remainingLength > BLOCK_SIZE));

		if (remainingLength >= BLOCK_SIZE )
		{
			clear = reinterpret_cast< const uint8 * >(
				input.retrieve( static_cast<int>(BLOCK_SIZE) ) );
		}
		else
		{
			memcpy( bufferForPaddedBlocks, 
				input.retrieve( static_cast<int>(remainingLength) ), 
				remainingLength );
			memset( bufferForPaddedBlocks + remainingLength, '\0', 
				BLOCK_SIZE - remainingLength );
			clear = bufferForPaddedBlocks;
		}

		pBlockCipher_->combineBlocks( previousSentBlock_, clear, xored );

		pBlockCipher_->encryptBlock( xored, out );

		memcpy( previousSentBlock_, clear, BLOCK_SIZE );

		MemoryIStream outStream( out, static_cast<int>(BLOCK_SIZE) );

		if (!this->stream().writeFrom( outStream, shouldCorkBlock ))
		{
			return false;
		}
	}

	return true;
}


/*
 *	Override from StreamFilter.
 */
int EncryptionStreamFilter::readInto( BinaryOStream & output )
{
	int numBytesRead = this->stream().readInto( receiveBuffer_ );

	if (numBytesRead <= 0)
	{
		return numBytesRead;
	}

	const size_t BLOCK_SIZE = pBlockCipher_->blockSize();

	while (static_cast<size_t>(receiveBuffer_.remainingLength()) >= BLOCK_SIZE)
	{
		const uint8 * block =
			reinterpret_cast< const uint8 * >( 
				receiveBuffer_.retrieve( 
					static_cast< int >( pBlockCipher_->blockSize() ) ) );
		this->decryptBlock( block, output );
	}

	if (receiveBuffer_.remainingLength() == 0)
	{
		receiveBuffer_.reset();
	}

	return true;
}


/**
 *	This method updates the expected receive frame length from the given
 *	decrypted data block.
 */
void EncryptionStreamFilter::updateExpectedReceiveFrameLength( 
		const uint8 * block )
{
	uint16 smallFrameLength = BW_NTOHS( 
			*reinterpret_cast< const uint16 * >( block ) );

	if (smallFrameLength == 0xFFFF)
	{
		uint32 largeFrameLength = BW_NTOHL(
			*reinterpret_cast< const uint32 * >( 
				block + sizeof( uint16 ) ) );
		largeFrameLength += sizeof( uint16 ) + sizeof( uint32 );

		expectedReceiveFrameLength_ = largeFrameLength;
	}
	else
	{
		smallFrameLength += sizeof( uint16 );
		expectedReceiveFrameLength_ = smallFrameLength;
	}
}


/**
 *	This method decrypts a block into the output buffer and processes it for
 *	length data if necessary (so it can discard any padding from the 64-bit
 *	encryption block boundary).
 *
 *	@param block 	The ciphertext BlowFish block.
 *	@param output 	The output buffer.
 *
 */
void EncryptionStreamFilter::decryptBlock( const uint8 * block,
		BinaryOStream & output )
{
	const size_t BLOCK_SIZE = pBlockCipher_->blockSize();
	size_t dataSize = BLOCK_SIZE;

	uint8 * pOut = NULL;
	uint8 * tempBlock = reinterpret_cast<uint8*>(alloca( BLOCK_SIZE ));
	memset( tempBlock, '\0', BLOCK_SIZE );

	if ((expectedReceiveFrameLength_ != 0) && 
			(expectedReceiveFrameLength_ < dataSize))
	{
		dataSize = expectedReceiveFrameLength_;
		pOut = tempBlock;
	}
	else if (expectedReceiveFrameLength_ == 0)
	{
		// We don't know how long this frame could be yet. Assume more than
		// BLOCK_SIZE for now, but decrypt to tempBlock in case it is less than
		// BLOCK_SIZE and has padding to be discarded.
		pOut = tempBlock;
	}
	else
	{
		pOut = reinterpret_cast< uint8 * >( output.reserve( 
			static_cast<int>(dataSize) ) );
	}

	pBlockCipher_->decryptBlock( block, pOut );

	pBlockCipher_->combineBlocks( previousReceivedBlock_, pOut, pOut );

	memcpy( previousReceivedBlock_, pOut, BLOCK_SIZE );

	if (expectedReceiveFrameLength_ == 0)
	{
		this->updateExpectedReceiveFrameLength( pOut );
		dataSize = std::min( expectedReceiveFrameLength_, dataSize );
	}

	// Discard any data past the dataSize, it should just be padding.
	if (pOut == tempBlock)
	{
		memcpy( output.reserve( static_cast<int>(dataSize) ), pOut, dataSize );
	}

	expectedReceiveFrameLength_ -= dataSize;
}


/*
 *	Override from StreamFilter
 */
BW::string EncryptionStreamFilter::streamToString() const
{
	return "enc+" + this->stream().streamToString();
}


} // end namespace Mercury

BW_END_NAMESPACE


// encryption_stream_filter.cpp
