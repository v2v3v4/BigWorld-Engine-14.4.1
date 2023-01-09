#ifndef ENCRYPTION_STREAM_FILTER
#define ENCRYPTION_STREAM_FILTER

#include "basictypes.hpp"
#include "block_cipher.hpp"
#include "stream_filter.hpp"

#include "cstdmf/memory_stream.hpp"


BW_BEGIN_NAMESPACE


namespace Mercury
{


/**
 *	This class implements a stream filter that encrypts/decrypts traffic
 *	passing through it.
 */
class EncryptionStreamFilter : public StreamFilter
{
public:
	static StreamFilterPtr create( NetworkStream & stream,
		BlockCipherPtr pCipher );

	// Overrides from StreamFilter

	virtual bool writeFrom( BinaryIStream & input, bool shouldCork );
	virtual int readInto( BinaryOStream & output );

	virtual BW::string streamToString() const;

private:

	EncryptionStreamFilter( NetworkStream & stream,
			BlockCipherPtr cipher );

	virtual ~EncryptionStreamFilter();

	// Disable copy constructor and copy-by-assignment
	EncryptionStreamFilter( const EncryptionStreamFilter & );
	EncryptionStreamFilter & operator=( const EncryptionStreamFilter & );

	void decryptBlock( const uint8 * block, BinaryOStream & output );
	void updateExpectedReceiveFrameLength( const uint8 * block );


	BlockCipherPtr 		pBlockCipher_;
	MemoryOStream 		receiveBuffer_;
	size_t 				expectedReceiveFrameLength_;
	uint8 * 			previousReceivedBlock_;
	uint8 * 			previousSentBlock_;
};


} // end namespace Mercury


BW_END_NAMESPACE

#endif // ENCRYPTION_STREAM_FILTER
