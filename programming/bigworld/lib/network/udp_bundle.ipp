#ifdef CODE_INLINE
    #define INLINE    inline
#else
	/// INLINE macro.
    #define INLINE
#endif


namespace Mercury
{

// -----------------------------------------------------------------------------
// Section: UDPBundle
// -----------------------------------------------------------------------------

/**
 * Sets the default reliability policy of the current message.
 */
INLINE void UDPBundle::reliable( ReliableType currentMsgReliable )
{
	msgIsReliable_ = currentMsgReliable.isReliable();
	reliableDriver_ |= currentMsgReliable.isDriver();
}


/**
 *  Returns true if this bundle has any reliable driver messages on it.
 */
INLINE bool UDPBundle::isReliable() const
{
	return reliableDriver_;
}


/*
 *	Override from Bundle.
 */
INLINE int UDPBundle::freeBytesInLastDataUnit() const
{
	return pCurrentPacket_->freeSpace();
}


/**
 *	This method returns whether or not this bundle has interesting footers to
 *	send.
 */
INLINE bool UDPBundle::hasDataFooters() const
{
	for (const Packet * p = pFirstPacket_.get(); p; p = p->next())
	{
		if (p->hasFlags( Packet::FLAG_HAS_ACKS ) ||
			p->hasFlags( Packet::FLAG_HAS_PIGGYBACKS ))
		{
			return true;
		}
	}

	return false;
}

/**
 * 	This method reserves the given number of bytes in this bundle.
 */
INLINE void * UDPBundle::reserve( int nBytes )
{
	return qreserve( nBytes );
}

/**
 * 	This method gets a pointer to this many bytes quickly
 * 	(non-virtual function)
 */
INLINE void * UDPBundle::qreserve( int nBytes )
{
	if (nBytes <= pCurrentPacket_->freeSpace())
	{
		void * writePosition = pCurrentPacket_->back();
		pCurrentPacket_->grow( nBytes );
		return writePosition;
	}
	else
	{
		return this->sreserve( nBytes );
	}
}


/**
 *	This convenience method is used to add a block of memory to this stream.
 */
INLINE
void UDPBundle::addBlob( const void * pBlob, int size )
{
	const char * pCurr = (const char *)pBlob;

	while (size > 0)
	{
		// If there isn't any more space on this packet, force a new one to be
		// allocated to this bundle.
		if (pCurrentPacket_->freeSpace() == 0)
		{
			this->sreserve( 0 );
		}

		int currSize = std::min( size, int( pCurrentPacket_->freeSpace() ) );
		MF_ASSERT( currSize > 0 );

		memcpy( this->qreserve( currSize ), pCurr, currSize );
		size -= currSize;
		pCurr += currSize;
	}
}

} // namespace Mercury

// udp_bundle.ipp
