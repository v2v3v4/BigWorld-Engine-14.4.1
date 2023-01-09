namespace Mercury
{

/**
 *	This method finds the channel with the given address. If none is found, a
 *	new channel is created.
 */
INLINE UDPChannel & NetworkInterface::findOrCreateChannel( 
		const Address & addr )
{
	return *this->findChannel( addr, /* createAnonymous: */ true );
}


// -----------------------------------------------------------------------------
// Section: Accessors
// -----------------------------------------------------------------------------

/**
 *	This method returns the address of the interface this is bound to or the
 *	address of the first non-local interface if it's bound to all interfaces.
 *	A zero address is returned if the interface couldn't initialise.
 */
INLINE const Address & NetworkInterface::address() const
{
	return address_;
}

} // namespace Mercury

// network_interface.ipp
