#ifdef CODE_INLINE
	#define INLINE    inline
#else
	/// INLINE macro.
	#define INLINE
#endif


namespace Mercury
{

/**
 * Make using simple messages easier - returns a pointer the
 * size of the message (note: not all fixed length msgs will
 * be simple structs, so startMessage doesn't do it
 * automatically)
 */
INLINE void * Bundle::startStructMessage( const InterfaceElement & ie,
	ReliableType reliable )
{
	this->startMessage( ie, reliable );
	return this->reserve( ie.lengthParam() );
}

/**
 * Make using simple requests easier - returns a pointer the
 * size of the request message.
 */
INLINE void * Bundle::startStructRequest( const InterfaceElement & ie,
	ReplyMessageHandler * handler, void * arg,
	int timeout, ReliableType reliable)
{
	this->startRequest( ie, handler, arg, timeout, reliable );
	return this->reserve( ie.lengthParam() );
}


} // end namespace Mercury


// bundle.ipp

