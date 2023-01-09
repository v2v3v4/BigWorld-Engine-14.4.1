#ifndef REPLAY_HEADER_HPP
#define REPLAY_HEADER_HPP

#include "cstdmf/checksum_stream.hpp"
#include "cstdmf/md5.hpp"

#include "network/basictypes.hpp"

#include "client_server_protocol_version.hpp"
#include "replay_data.hpp"


BW_BEGIN_NAMESPACE

class BinaryIStream;


/**
 *	This class represents the data in the header block for replay data files.
 */
class ReplayHeader
{
public:
	static const int NONCE_SIZE = 8;
	typedef ReplayData::HeaderSignatureLength SignatureLength;

	explicit ReplayHeader( ChecksumSchemePtr pChecksumScheme );

	ReplayHeader( ChecksumSchemePtr pChecksumScheme,
		const MD5::Digest & digest, uint gameUpdateFrequency,
		const BW::string & nonce = BW::string( "" ) );

	ReplayHeader( const ReplayHeader & other );
	ReplayHeader & operator=( const ReplayHeader & other );

	template< typename STREAM >
	void addToStream( STREAM & data, BinaryOStream * pSignature ) const;

	bool initFromStream( BinaryIStream & data,
		BinaryOStream * pSignature = NULL,
		BW::string * pErrorString = NULL,
		bool shouldVerify = true );


	// Accessors

	/** This method returns the version. */
	const ClientServerProtocolVersion & version() const
	{
		return version_;
	}

	/** This method returns the digest. */
	const MD5::Digest & digest() const 	{ return digest_; }

	/** This method returns the reported number of ticks. */
	GameTime numTicks() const 			{ return numTicks_; }

	/** This method returns the game update frequency. */
	uint gameUpdateFrequency() const 	{ return gameUpdateFrequency_; }

	/** This method returns the file creation time, in seconds after epoch. */
	ReplayData::Timestamp timestamp() const { return timestamp_; }

	/** This method returns the reported signature length. */
	SignatureLength reportedSignatureLength() const
	{
		return reportedSignatureLength_;
	}

	static off_t numTicksFieldOffset( ChecksumSchemePtr pChecksumScheme );
	static bool checkSufficientLength( const void * data, size_t size,
		size_t & headerSize );

	size_t streamSize() const;
	static size_t calculateStreamSize( ChecksumSchemePtr pChecksumScheme );

private:
	void initNonce();
	static off_t signatureLengthFieldOffset();

	ChecksumSchemePtr 				pChecksumScheme_;
	ClientServerProtocolVersion 	version_;
	MD5::Digest 					digest_;
	uint 							gameUpdateFrequency_;
	ReplayData::Timestamp			timestamp_;
	BW::string 						nonce_;
	SignatureLength 				reportedSignatureLength_;
	uint 							numTicks_;
};


// Streaming operators

/**
 *	In-streaming operator for ReplayHeader.
 */
template< typename STREAM >
STREAM & operator>>( STREAM & stream, ReplayHeader & header )
{
	header.initFromStream( stream );
	return stream;
}


/**
 *	Out-streaming operator for ReplayHeader.
 */
template< typename STREAM >
STREAM & operator<<( STREAM & stream, const ReplayHeader & header )
{
	header.addToStream( stream );
	return stream;
}


BW_END_NAMESPACE


#include "replay_header.ipp"


#endif // REPLAY_HEADER_HPP
