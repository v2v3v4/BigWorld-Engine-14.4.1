#include "pch.hpp"

#include "replay_header.hpp"

#if defined( MF_SERVER )

#include <sys/time.h>

namespace BWOpenSSL
{
#include "openssl/rand.h"
}
#endif // defined( MF_SERVER )


BW_BEGIN_NAMESPACE


/**
 *	Constructor, used for reading a replay header using initFrom().
 *
 *	@param pChecksumScheme 	The checksum scheme used.
 */
ReplayHeader::ReplayHeader( ChecksumSchemePtr pChecksumScheme ) :
		pChecksumScheme_( pChecksumScheme ),
		version_(),
		digest_(),
		gameUpdateFrequency_( 0U ),
		timestamp_( 0U ),
		nonce_(),
		reportedSignatureLength_( 0U ),
		numTicks_( 0U )
{
	digest_.clear();
}


/**
 *	Constructor, used for writing out a replay header. The header is written
 *	using the current protocol version.
 *
 *	@param pChecksumScheme 		The checksum scheme used.
 *	@param digest 				The digest.
 *	@param gameUpdateFrequency 	The game update frequency.
 *	@param nonce				The nonce to use. If empty, a new one will be
 *								generated. Must be NONCE_SIZE bytes long.
 */
ReplayHeader::ReplayHeader(
			ChecksumSchemePtr pChecksumScheme,
			const MD5::Digest & digest,
			uint gameUpdateFrequency,
			const BW::string & nonce ) :
		pChecksumScheme_( pChecksumScheme ),
		version_( ClientServerProtocolVersion::currentVersion() ),
		digest_( digest ),
		gameUpdateFrequency_( gameUpdateFrequency ),
		timestamp_( 0U ),
		nonce_( nonce ),
		reportedSignatureLength_( 0U ),
		numTicks_( 0U )
{
	MF_ASSERT( pChecksumScheme.exists() );
	reportedSignatureLength_ = 
		static_cast<SignatureLength>( pChecksumScheme->streamSize() );

#if defined( MF_SERVER )
	timeval tv;
	gettimeofday( &tv, NULL );
	timestamp_ = static_cast<ReplayData::Timestamp>( tv.tv_sec );
#endif

	this->initNonce();
}


/**
 *	Copy constructor.
 */
ReplayHeader::ReplayHeader( const ReplayHeader & other ) :
		pChecksumScheme_( other.pChecksumScheme_ ),
		version_( other.version_ ),
		digest_( other.digest_ ),
		gameUpdateFrequency_( other.gameUpdateFrequency_ ),
		timestamp_( other.timestamp_ ),
		nonce_( other.nonce_ ),
		reportedSignatureLength_( other.reportedSignatureLength_ ),
		numTicks_( other.numTicks_ )
{
}


/**
 *	Assignment operator.
 */
ReplayHeader & ReplayHeader::operator=( const ReplayHeader & other )
{
	pChecksumScheme_			= other.pChecksumScheme_;
	version_					= other.version_;
	digest_						= other.digest_;
	gameUpdateFrequency_		= other.gameUpdateFrequency_;
	timestamp_					= other.timestamp_;
	nonce_						= other.nonce_;
	reportedSignatureLength_ 	= other.reportedSignatureLength_;
	numTicks_ 					= other.numTicks_;

	return *this;
}


/**
 *	This method initialises the nonce, if required.
 */
void ReplayHeader::initNonce()
{
#if defined( MF_SERVER )
	if (nonce_.empty())
	{
		unsigned char nonceBuffer[NONCE_SIZE];
		BWOpenSSL::RAND_bytes( nonceBuffer, NONCE_SIZE );

		nonce_.assign( (char *)nonceBuffer, NONCE_SIZE );
	}

	MF_ASSERT( int( nonce_.size() ) == NONCE_SIZE );

#endif // defined( MF_SERVER )
}


/**
 *	Initialises this object from a data stream.
 *
 *	@param data 		The input stream to read from.
 *	@param pSignature 	The signature that was read from the stream.
 *	@param pErrorString If present, this will be filled with an error string in
 *						the event of an error.
 *	@param shouldVerify Whether the signature should be verified.
 *	@return 			True if the data was successfully read, false otherwise.
 */
bool ReplayHeader::initFromStream( BinaryIStream & data,
		BinaryOStream * pSignature /* = NULL */,
		BW::string * pErrorString /* = NULL */,
		bool shouldVerify /* = true */ )
{
	if (pChecksumScheme_ == NULL)
	{
		shouldVerify = false;
	}

	ClientServerProtocolVersion version;
	MD5::Digest digest;
	uint8 gameUpdateFrequency;
	ReplayData::Timestamp timestamp;
	BW::string nonce;
	SignatureLength signatureLength;
	uint32 numTicks;

	{
		ChecksumIStream checksumStream( data,
			shouldVerify ? pChecksumScheme_ : NULL );

		checksumStream >> version >> digest >> gameUpdateFrequency >> timestamp;

		if (!version.supports( ClientServerProtocolVersion::currentVersion() ))
		{
			if (pErrorString)
			{
				*pErrorString = "Read replay header error: version mismatch";
			}
			return false;
		}
		
		nonce.assign( (char *)(checksumStream.retrieve( NONCE_SIZE )),
			NONCE_SIZE );

		checksumStream >> signatureLength;

		if (shouldVerify && !checksumStream.verify( pSignature ))
		{
			if (pErrorString)
			{
				*pErrorString = checksumStream.errorString();
			}
			return false;
		}
	}

	if (!shouldVerify && pSignature)
	{
		pSignature->transfer( data, static_cast<int>( signatureLength ) );
	}

	data >> numTicks;

	version_					= version;
	digest_						= digest;
	gameUpdateFrequency_		= gameUpdateFrequency;
	timestamp_ 					= timestamp;
	nonce_ 						= nonce;
	reportedSignatureLength_ 	= signatureLength;
	numTicks_ 					= numTicks;

	return true;
}


/**
 *	This method returns the stream size of this replay header.
 */
size_t ReplayHeader::streamSize() const
{
	return ClientServerProtocolVersion::streamSize() + 	// version
		MD5::Digest::streamSize() +  					// digest
		sizeof(uint8) + 								// gameUpdateFrequency
		sizeof(ReplayData::Timestamp) +					// timestamp
		NONCE_SIZE +	 								// nonce
		sizeof(SignatureLength) + 						// signature length
		reportedSignatureLength_ +						// signature
		sizeof(GameTime);								// numTicks
}


size_t ReplayHeader::calculateStreamSize( ChecksumSchemePtr pChecksumScheme )
{
	return ClientServerProtocolVersion::streamSize() + 	// version
		MD5::Digest::streamSize() +  					// digest
		sizeof(uint8) + 								// gameUpdateFrequency
		sizeof(ReplayData::Timestamp) +					// timestamp
		NONCE_SIZE +	 								// nonce
		sizeof(SignatureLength) + 						// signature length
		pChecksumScheme->streamSize() +					// signature
		sizeof(GameTime);								// numTicks
}


/**
 *	This method returns the offset of the signature length field.
 */
off_t ReplayHeader::signatureLengthFieldOffset()
{
	return ClientServerProtocolVersion::streamSize() + 	// version
		MD5::Digest::streamSize() +  					// digest
		sizeof(uint8) + 								// gameUpdateFrequency
		sizeof(ReplayData::Timestamp) +					// timestamp
		NONCE_SIZE;		 								// nonce
}


/**
 *	This method returns the offset of the game time field with respect to the
 *	start of the header.
 *
 *	@param pChecksumScheme 	The checksum scheme.
 */
off_t ReplayHeader::numTicksFieldOffset( ChecksumSchemePtr pChecksumScheme )
{
	MF_ASSERT( pChecksumScheme.exists() );
	size_t retVal = ReplayHeader::calculateStreamSize( pChecksumScheme );
	retVal -= sizeof(GameTime);
	return static_cast<off_t>(retVal);
}


/**
 *	This method returns whether the given header fragment is sufficient to read
 *	off the header completely. If available, the expected total size of the
 *	header can also be output.
 *
 *	@param data 		The header fragment data.
 *	@param size 		The size of the header fragment.
 *	@param headerSize 	This is filled with the size of the header, if known.
 *
 *	@return 			true if the fragment is of sufficient size to stream
 *						off a complete header, false otherwise.
 */
bool ReplayHeader::checkSufficientLength( const void * data, size_t size,
		size_t & headerSize )
{
	if (size < (ReplayHeader::signatureLengthFieldOffset() +
			sizeof(SignatureLength)))
	{
		return false;
	}

	const char * cursor = reinterpret_cast< const char * >( data );
	cursor += ReplayHeader::signatureLengthFieldOffset();
	MemoryIStream signatureLengthStream( cursor, sizeof(SignatureLength) );

	SignatureLength signatureLength;
	signatureLengthStream >> signatureLength;

	headerSize = (ReplayHeader::signatureLengthFieldOffset() +
		sizeof(SignatureLength) + signatureLength +
		sizeof(GameTime));

	return (size >= headerSize);
}


BW_END_NAMESPACE


// replay_header.cpp
