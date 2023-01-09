#ifndef ENCRYPTION_FILTER_HPP
#define ENCRYPTION_FILTER_HPP

#include "basictypes.hpp"
#include "block_cipher.hpp"
#include "packet_filter.hpp"

#include "cstdmf/bw_string.hpp"
#include "cstdmf/smartpointer.hpp"
#include "cstdmf/memory_stream.hpp"


BW_BEGIN_NAMESPACE


namespace Mercury
{

class ProcessSocketStatsHelper;

class EncryptionFilter;
typedef SmartPointer< EncryptionFilter > EncryptionFilterPtr;


/**
 *  A PacketFilter that uses a non-modal block cipher
 */
class EncryptionFilter : public PacketFilter
{
public:
	// Factory method
	static EncryptionFilterPtr create( BlockCipherPtr pCipher );

	// Overrides from PacketFilter 
	virtual Reason send( PacketSender & packetSender,
						const Address & addr, Packet * pPacket );
	virtual Reason recv( PacketReceiver & receiver,
						const Address & addr, Packet * pPacket,
						ProcessSocketStatsHelper * pStatsHelper );

	virtual int maxSpareSize();

	const BlockCipher::Key & key() const { return pBlockCipher_->key(); }

	void encryptStream( MemoryOStream & clearStream,
		BinaryOStream & cipherStream );

	bool decryptStream( BinaryIStream & cipherStream,
		BinaryOStream & clearStream );

protected:
	// These two are protected to ensure that only the factory methods are
	// used.
	EncryptionFilter( BlockCipherPtr pCipher );

	// Only deletable via SmartPointer
	virtual ~EncryptionFilter();

private:
	// Internal methods
	int encrypt( const unsigned char * src, unsigned char * dest, int length );
	int encrypt( const unsigned char * src, BinaryOStream & dest, int length );

	int decrypt( const unsigned char * src, unsigned char * dest, int length,
		bool isVerbose = true );


	// Prevent copy-construct or copy-assignment
	EncryptionFilter( const EncryptionFilter & other );
	EncryptionFilter & operator=( const EncryptionFilter & other );

	BlockCipherPtr pBlockCipher_;
};

} // namespace Mercury


BW_END_NAMESPACE


#endif // ENCRYPTION_FILTER_HPP
