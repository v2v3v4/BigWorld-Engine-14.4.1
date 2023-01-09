

BW_BEGIN_NAMESPACE


/**
 *	This method adds this header data to the given stream.
 *
 *	@param data 				The output stream to write to.
 *	@param pSignature 			If present, this is filled with the signature
 *								from the checksum scheme.
 */
template< typename STREAM >
void ReplayHeader::addToStream( STREAM & data,
		BinaryOStream * pSignature ) const
{
	// This is templated so that ChecksumOStream is passed the correct type for
	// the BinaryOStream, so that MemoryOStream can be special-cased to not use
	// an additional buffer.

	{
		ChecksumOStream checksumStream( data, pChecksumScheme_ );

		checksumStream << version_ << digest_ <<
			uint8(gameUpdateFrequency_) << timestamp_;

		MF_ASSERT( nonce_.size() == size_t( NONCE_SIZE ) );
		checksumStream.addBlob( nonce_.data(), nonce_.size() );

		checksumStream << reportedSignatureLength_;
		checksumStream.finalise( pSignature );
	}

	// numTicks is not used for the signature because it will change when the
	// file is finalised. There isn't much harm in leaving it out, there's not
	// much benefit to changing it besides screwing up the hinting given to
	// ReplayController.
	// If it gets to be a problem, we can move it inside and rewrite the entire
	// header on finalising the file. ReplayHeader would need to be added to
	// the recovery data to facilitate this.
	data << uint32(numTicks_);
}


BW_END_NAMESPACE


// replay_header.ipp
