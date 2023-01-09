#ifdef CODE_INLINE
    #define INLINE    inline
#else
	/// INLINE macro.
    #define INLINE
#endif

namespace Mercury
{

/**
 *	This method increments the corrupted packet count.
 */
INLINE void PacketReceiverStats::incCorruptedPackets()
{
	numCorruptedPacketsReceived_++;
}


/**
 *	This method increments the number of duplicate packets received.
 */
INLINE void PacketReceiverStats::incDuplicatePackets()
{
	numDuplicatePacketsReceived_++;
}


/**
 *	This method returns the total number of packets received.
 *
 *	@return Number of packets received.
 */
INLINE unsigned int PacketReceiverStats::numPacketsReceived() const
{
	return numPacketsReceived_.total();
}


/**
 *	This method returns the total number of messages received.
 *
 *	@return Number of messages received.
 */
INLINE unsigned int PacketReceiverStats::numMessagesReceived() const
{
	return numMessagesReceived_.total();
}


/**
 *	This method returns the total number of bytes received.
 *
 *	@return Number of bytes received.
 */
INLINE unsigned int PacketReceiverStats::numBytesReceived() const
{
	return numBytesReceived_.total();
}


/**
 *	This method returns the total number of bytes received that are not part of
 *	any message.
 *
 *	@return Number of overhead bytes received.
 */
INLINE unsigned int PacketReceiverStats::numOverheadBytesReceived() const
{
	return numOverheadBytesReceived_.total();
}

} // namespace Mercury

// packet_receiver_stats.ipp
