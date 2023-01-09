#include "pch.hpp"

#include "basictypes.hpp"
#include "encryption_filter.hpp"
#include "packet.hpp"
#include "packet_receiver.hpp"

#include "cstdmf/profile.hpp"

BW_BEGIN_NAMESPACE

namespace Mercury
{

typedef uint32	MagicType;

const MagicType ENCRYPTION_MAGIC = 0xdeadbeef;

/**
 *	EncryptionFilter Factory static method
 *
 *	@param pCipher The block cipher to use.
 */
/*static*/ EncryptionFilterPtr EncryptionFilter::create(
		BlockCipherPtr pCipher )
{
	return new EncryptionFilter( pCipher );
}


/**
 *  Create an encryption filter using the supplied key.
 */
EncryptionFilter::EncryptionFilter( BlockCipherPtr pCipher ) :
	pBlockCipher_( pCipher )
{
}


/**
 *  Destructor.
 */
EncryptionFilter::~EncryptionFilter()
{
}


/**
 *  This method encrypts the packet and sends it to the provided address.
 */
Reason EncryptionFilter::send( PacketSender & packetSender,
		const Address & addr, Packet * pPacket )
{
	size_t len = 0;
	PacketPtr toSend = NULL;
	uint8 wastage = 0;
	const size_t BLOCK_SIZE = pBlockCipher_->blockSize();

	{
		AUTO_SCOPED_PROFILE( "encryptSend" )

		// Bail if this filter is invalid.
		if (pBlockCipher_->key().empty())
		{
			WARNING_MSG( "EncryptionFilter::send: "
				"Dropping packet to %s due to invalid filter\n",
				addr.c_str() );

			return REASON_GENERAL_NETWORK;
		}

		// Grab a new packet.  Remember we have to leave the packet in its
		// original state so we can't just modify its data in-place.
		toSend = new Packet();

		// Work out the number of pad bytes required to make the data size a
		// multiple of the key size (required for most block ciphers).  The
		// magic +1 is for the wastage count that we must write to the end of
		// the packet so the receiver can figure out how big the unencrypted
		// stream is.
		len = pPacket->totalSize();

		// We append some magic to the end of the packet for validation
		len += sizeof( ENCRYPTION_MAGIC );
		wastage = static_cast<uint8>(
			((BLOCK_SIZE - ((len + 1) % BLOCK_SIZE)) % BLOCK_SIZE) + 1);
		// Don't consider the magic size as part of the wastage, it will be
		// removed separately.

		len += wastage;

		MF_ASSERT( len <= PACKET_MAX_SIZE );

		// len is greater than the actual packet's size but we've ensured that
		// data_ contains enough space to handle the filters extra data (padding
		// and encryption magic).
		// Set up the output packet to match.
		toSend->msgEndOffset( static_cast<int>(len) );

		// Write the wastage count into the last byte of the input packet.
		// Since wastage >= 1, we are not writing over any of the original data.
		uint32 startWastage = static_cast<uint32>(len - 1);
		uint32 startMagic = startWastage - sizeof( ENCRYPTION_MAGIC );

		pPacket->data()[ startWastage ] = wastage;
		*(MagicType *)(pPacket->data() + startMagic) = ENCRYPTION_MAGIC;

		this->encrypt( (const unsigned char*)pPacket->data(),
			(unsigned char*)toSend->data(), static_cast<int>(len) );
	}

	return this->PacketFilter::send( packetSender, addr, toSend.get() );
}


/**
 *	This method processes an incoming encrypted packet.
 */
Reason EncryptionFilter::recv( PacketReceiver & receiver, const Address & addr,
		Packet * pPacket, ProcessSocketStatsHelper * pStatsHelper )
{
	{
		AUTO_SCOPED_PROFILE( "encryptRecv" )

		// Bail if this filter is invalid.
		if (pBlockCipher_->key().empty())
		{
			if (receiver.isVerbose())
			{
				WARNING_MSG( "EncryptionFilter::recv: "
					"Dropping packet from %s due to invalid filter\n",
					addr.c_str() );
			}

			return REASON_GENERAL_NETWORK;
		}

		// Decrypt the data in place.  This is fine for Blowfish, but may not be
		// safe for other encryption algorithms.
		int decryptLen = this->decrypt( (const unsigned char*)pPacket->data(),
			(unsigned char*)pPacket->data(), pPacket->totalSize(),
			receiver.isVerbose() );

		if (decryptLen == -1)
		{
			receiver.stats().incCorruptedPackets();
			return REASON_CORRUPTED_PACKET;
		}

		// Read the wastage amount
		uint32 startWastage = pPacket->totalSize() - 1;

		if (startWastage < sizeof( ENCRYPTION_MAGIC ))
		{
			MF_ASSERT_DEV( !"Not enough wastage" );
			receiver.stats().incCorruptedPackets();
			return REASON_CORRUPTED_PACKET;
		}

		uint32 startMagic   = startWastage - sizeof( ENCRYPTION_MAGIC );
		uint8 wastage = pPacket->data()[ startWastage ];

		MagicType &packetMagic = *(MagicType *)(pPacket->data() + startMagic);

		// Check the ENCRYPTION_MAGIC is as we expect
		if (packetMagic != ENCRYPTION_MAGIC)
		{
			if (receiver.isVerbose())
			{
				WARNING_MSG( "EncryptionFilter::recv: "
					"Dropping packet with invalid magic 0x%x (expected 0x%x)\n",
					packetMagic, ENCRYPTION_MAGIC );
			}
			receiver.stats().incCorruptedPackets();
			return REASON_CORRUPTED_PACKET;
		}


		// Sanity check the wastage
		int footerSize = wastage + sizeof( ENCRYPTION_MAGIC );
		if (wastage > pBlockCipher_->blockSize() || 
				footerSize > pPacket->totalSize())
		{
			if (receiver.isVerbose())
			{
				WARNING_MSG( "EncryptionFilter::recv: "
						"Dropping packet from %s "
						"due to illegal wastage count (%d)\n",
					addr.c_str(), wastage );
			}

			receiver.stats().incCorruptedPackets();
			return REASON_CORRUPTED_PACKET;
		}

		// Set the packet length correctly
		pPacket->shrink( footerSize );
	}

	return this->PacketFilter::recv( receiver, addr, pPacket, pStatsHelper );
}


/**
 *  This method encrypts the data in the input stream and writes it to the
 *  output stream.  The reason it takes a MemoryOStream as the input parameter
 *  instead of a BinaryIStream like you'd expect is that it pads the input
 *  stream out to the correct size in place.
 */
void EncryptionFilter::encryptStream( MemoryOStream & clearStream,
	BinaryOStream & cipherStream )
{
	const size_t BLOCK_SIZE = pBlockCipher_->blockSize();
	const size_t remainderLength = 
		(static_cast<size_t>( clearStream.remainingLength() ) % BLOCK_SIZE);

	// Pad the input stream to the required size
	if (remainderLength != 0)
	{
		const size_t padSize = BLOCK_SIZE - remainderLength;
		void * padding = clearStream.reserve( static_cast<int>( padSize ) );
		memset( padding, 0, padSize );
	}

	int size = clearStream.remainingLength();
	this->encrypt( (unsigned char*)clearStream.data(), cipherStream, size );
}


/**
 *  This method decrypts data from the input stream and writes it to the output
 *  stream.  Note that the data may well have been padded when it was encrypted
 *  and you may have dangling bytes left over.
 */
bool EncryptionFilter::decryptStream( BinaryIStream & cipherStream,
	BinaryOStream & clearStream )
{
	int size = cipherStream.remainingLength();
	unsigned char * cipherText = (unsigned char*)cipherStream.retrieve( size );
	unsigned char * clearText =	(unsigned char*)clearStream.reserve( size );

	return (-1 != this->decrypt( cipherText, clearText, size ));
}


/*
 *	Override from PacketFilter.
 */
int EncryptionFilter::maxSpareSize()
{
	// This is an allowance for networks that cannot handle "full-sized" UDP
	// packets. This can happen, for example, on a VPN at has some overhead per
	// packet. Ideally, this should be calculated dynamically per connection.
	const size_t MTU_ALLOWANCE = 200;

	// Might need BLOCK_SIZE more bytes when (packetlen % BLOCK_SIZE == 1)
	return static_cast<int>(pBlockCipher_->blockSize() + 
			sizeof( ENCRYPTION_MAGIC ) + MTU_ALLOWANCE);
}


/**
 *  This method encrypts the provided data, given a contiguous destination
 *  array.
 */
int EncryptionFilter::encrypt( const unsigned char * src, unsigned char * dest,
		int length )
{
	const size_t BLOCK_SIZE = pBlockCipher_->blockSize();

	if (length % BLOCK_SIZE != 0)
	{
		CRITICAL_MSG( "EncryptionFilter::encrypt: "
				"Input length (%d) is not a multiple of block size (%zu)\n",
			length, BLOCK_SIZE );
	}

	// We XOR each block after the first with the previous one, prior to
	// encryption.  This prevents people reassembling blocks from different
	// packets into new ones, i.e. it prevents replay attacks.
	const unsigned char * pPrevBlock = NULL;

	for (size_t i=0; i < static_cast<size_t>(length); i += BLOCK_SIZE)
	{
		if (pPrevBlock)
		{
			pBlockCipher_->combineBlocks( src + i, pPrevBlock, dest + i );
		}
		else
		{
			memcpy( dest + i, src + i, BLOCK_SIZE );
		}

		pBlockCipher_->encryptBlock( dest + i, dest + i );
		pPrevBlock = src + i;
	}

	return length;
}


/**
 *  This method encrypts the provided data into a binary stream with
 *  the likelihood of it being unable to reserve the whole amount
 *  in one go.
 */
int EncryptionFilter::encrypt( const unsigned char * src, 
	BinaryOStream & cipherStream, int length)
{
	const size_t BLOCK_SIZE = pBlockCipher_->blockSize();

	if (length % BLOCK_SIZE != 0)
	{
		CRITICAL_MSG( "EncryptionFilter::encrypt: "
			"Input length (%d) is not a multiple of block size (%zu)\n",
			length, BLOCK_SIZE );
	}

	// We XOR each block after the first with the previous one, prior to
	// encryption.  This prevents people reassembling blocks from different
	// packets into new ones, i.e. it prevents replay attacks.
	const unsigned char * pPrevBlock = NULL;
	unsigned char * dest = NULL;

	for (size_t i=0; i < static_cast<size_t>(length); i += BLOCK_SIZE)
	{
		dest = (unsigned char*)cipherStream.reserve(
			static_cast<int>(BLOCK_SIZE) );

		if (pPrevBlock)
		{
			pBlockCipher_->combineBlocks( src + i, pPrevBlock, dest );
		}
		else
		{
			memcpy( dest, src + i, BLOCK_SIZE );
		}

		pBlockCipher_->encryptBlock( dest, dest );
		pPrevBlock = src + i;
	}

	return length;
}


/**
 *  This method decrypts the provided data.
 */
int EncryptionFilter::decrypt( const unsigned char * src, 
		unsigned char * dest, int length, bool isVerbose )
{
	const size_t BLOCK_SIZE = pBlockCipher_->blockSize();

	// If the packet length is not an exact multiple of the key length, abort.
	if (static_cast< size_t >( length ) % BLOCK_SIZE != 0)
	{
		if (isVerbose)
		{
			WARNING_MSG( "EncryptionFilter::decrypt: "
					"Input stream size (%d) is not a multiple "
					"of the block size (%zu)\n",
				length, BLOCK_SIZE );
		}

		return -1;
	}

	// Inverse of the XOR logic in encrypt() above.
	const unsigned char * pPrevBlock = NULL;

	for (size_t i = 0; i < static_cast<size_t>(length); i += BLOCK_SIZE)
	{
		unsigned char * pOutBlock = dest + i;
		pBlockCipher_->decryptBlock( src + i, pOutBlock );

		if (pPrevBlock)
		{
			pBlockCipher_->combineBlocks( pPrevBlock, pOutBlock, pOutBlock );
		}

		pPrevBlock = dest + i;
	}

	return length;
}


} // namespace Mercury

BW_END_NAMESPACE
