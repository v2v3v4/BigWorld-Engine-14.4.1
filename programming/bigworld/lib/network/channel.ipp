#ifdef CODE_INLINE
    #define INLINE    inline
#else
	/// INLINE macro.
    #define INLINE
#endif

namespace Mercury
{

// -----------------------------------------------------------------------------
// Section: Channel
// -----------------------------------------------------------------------------

/**
 *	The window size maintained by this channel, for both outgoing resends
 *	and incoming reordering.
 */
INLINE int Channel::windowSize() const
{
	return windowSize_;
}


/**
 * 	This method returns the peer address of the channel.
 * 	The address is const, and may not be modified.
 *
 * 	@return	The address of the peer.
 */
INLINE const Address & Channel::addr() const
{
	return addr_;
}


/**
 * 	This method masks the given integer with the sequence mask.
 */
INLINE SeqNum Channel::seqMask( SeqNum x )
{
	return x & SEQ_MASK;
}


/**
 *	This method returns whether or not the first argument is "before" the second
 *	argument.
 */
INLINE bool Channel::seqLessThan( SeqNum a, SeqNum b )
{
	return seqMask( a - b ) > SEQ_SIZE/2;
}


/**
 *	This method returns whether a sequence number is within the window of
 *	unacked packets.
 */
INLINE bool Channel::isInSentWindow( SeqNum seq ) const
{
	return (oldestUnackedSeq_ != SEQ_NULL) &&
		!seqLessThan( seq, oldestUnackedSeq_ ) &&
		seqLessThan( seq, smallOutSeqAt_ );
}

}; // namespace Mercury

// channel.ipp
