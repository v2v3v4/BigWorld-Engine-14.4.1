#ifdef CODE_INLINE
    #define INLINE    inline
#else
	/// INLINE macro.
    #define INLINE
#endif

namespace Mercury
{

// -----------------------------------------------------------------------------
// Section: UDPChannel
// -----------------------------------------------------------------------------

/**
 *	The window size maintained by this channel, for both outgoing resends
 *	and incoming reordering.
 */
INLINE int UDPChannel::windowSize() const
{
	return windowSize_;
}


/**
 *	This method returns whether a sequence number is within the window of
 *	unacked packets.
 */
INLINE bool UDPChannel::isInSentWindow( SeqNum seq ) const
{
	return (oldestUnackedSeq_ != SEQ_NULL) &&
		!seqLessThan( seq, oldestUnackedSeq_ ) &&
		seqLessThan( seq, smallOutSeqAt_ );
}

}; // namespace Mercury

// udp_channel.ipp
