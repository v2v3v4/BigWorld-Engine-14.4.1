#ifndef RECORDING_RECOVERY_DATA_HPP
#define RECORDING_RECOVERY_DATA_HPP

#include "cstdmf/bw_string.hpp"

#include "network/basictypes.hpp"
#include "network/compression_type.hpp"

BW_BEGIN_NAMESPACE


/**
 *	This class holds the recovery state for recording file writers.
 */
class RecordingRecoveryData
{
public:
	/**
	 *	Default constructor.
	 */
	RecordingRecoveryData() :
		compressionType_( BW_COMPRESSION_NONE ),
		numTicks_( 0 ),
		lastTickWritten_( 0 ),
		nextChunkPosition_( 0 ),
		lastSignature_(),
		numTicksToSign_( 0U )
	{}


	/**
	 *	Constructor.
	 *
	 *	@param compressionType 		The compression type used for the writing.
	 *	@param numTicks				The number of ticks already written.
	 *	@param lastTickWritten 		The last tick written.
	 *	@param nextTickPosition 	The file offset of the next tick to write.
	 *	@param lastSignature 		The signature of the last chunk.
	 *	@param numTicksToSign 		The number of ticks to sign per chunk of 
	 *								ticks.
	 */
	RecordingRecoveryData( BWCompressionType compressionType,
				GameTime numTicks, GameTime lastTickWritten,
				off_t nextTickPosition, const BW::string & lastSignature,
				uint numTicksToSign ) :
			compressionType_( compressionType ),
			numTicks_( numTicks ),
			lastTickWritten_( lastTickWritten ),
			nextChunkPosition_( nextTickPosition ),
			lastSignature_( lastSignature ),
			numTicksToSign_( numTicksToSign )
	{
	}


	/**
	 *	This method implements the out-streaming operator.
	 *
	 *	@param stream 	The output stream.
	 */
	void addToStream( BinaryOStream & stream ) const
	{
		stream << uint8( compressionType_ ) << numTicks_ << lastTickWritten_ <<
			nextChunkPosition_ << lastSignature_ << uint16(numTicksToSign_);
	}


	/**
	 *	This method implements the in-streaming operator.
	 *
	 *	@param stream 	The input stream.

	 *	@return 		True if the streaming was successful, false otherwise.
	 */
	bool initFromStream( BinaryIStream & stream )
	{
		uint8 compressionType;
		uint16 numTicksToSign;
		stream >> compressionType >> numTicks_ >> lastTickWritten_ >>
			nextChunkPosition_ >> lastSignature_ >> numTicksToSign;

		compressionType_ = (BWCompressionType) compressionType;
		numTicksToSign_ = numTicksToSign;

		return !stream.error();
	}


	/**
	 *	This methods serialises this object to a string.
	 */
	const BW::string toString() const
	{
		MemoryOStream stream;
		this->addToStream( stream );
		int length = stream.remainingLength();
		return BW::string( (char *)stream.retrieve( length ), length );
	}


	/**
 	 *	This method initialises from a string.
	 *
	 *	@param s 	The string to initialise from.
	 *	@return 	True if successful, false otherwise.
	 */
	bool initFromString( const BW::string & s )
	{
		MemoryIStream stream( s.data(), s.size() );
		return this->initFromStream( stream );
	}


	// Accessors
	/** This method returns the compression type. */
	const BWCompressionType & compressionType() const
	{
		return compressionType_;
	}

	/** This method returns the number of ticks written. */
	GameTime numTicks() const 					{ return numTicks_; }

	/** This method returns the game time of the last tick written. */
	GameTime lastTickWritten() const 			{ return lastTickWritten_; }

	/** This method returns the file position of the next tick. */
	off_t nextTickPosition() const 				{ return nextChunkPosition_; }

	/** This method returns the signature of the last tick written. */
	const BW::string & lastSignature() const 	{ return lastSignature_; }

	/** This method returns the number of ticks that are signed together. */
	uint numTicksToSign() const 				{ return numTicksToSign_; }

private:
	BWCompressionType 	compressionType_;
	GameTime 			numTicks_;
	GameTime 			lastTickWritten_;
	off_t 				nextChunkPosition_;
	BW::string 			lastSignature_;
	uint 				numTicksToSign_;
};


BW_END_NAMESPACE

#endif // RECORDING_RECOVERY_DATA_HPP
