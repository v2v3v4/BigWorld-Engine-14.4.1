#ifdef CODE_INLINE
#define INLINE    inline
#else
#define INLINE
#endif


/**
 *	This method returns the primary Endpoint in use by BWMachined.
 */
INLINE
Endpoint & BWMachined::endpoint()
{
	return ep_;
}


/**
 *	This method returns BWMachined's cluster object.
 */
INLINE
Cluster & BWMachined::cluster()
{
	return cluster_;
}


/**
 *
 */
INLINE
const char * BWMachined::timingMethod() const
{
	return timingMethod_.c_str();
}


/**
 *	This method converts a struct timeval into a time in milliseconds.
 *
 *	Note: Only use this method for small timeval's.
 *
 *	@param tv  The timeval to convert into a TimeStamp in milliseconds.
 *
 *	@return A TimeStamp representation (in milliseconds) of the timeval arg.
 */
INLINE
TimeQueue64::TimeStamp BWMachined::tvToTimeStamp( struct timeval & tv )
{
	return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}


/**
 *	This method sets a time in millseconds into a struct timeval.
 *
 *	@param 
 */
INLINE
void BWMachined::timeStampToTV( const TimeQueue64::TimeStamp & ms,
	struct timeval &tv )
{
	tv.tv_sec = ms / 1000;
	tv.tv_usec = (ms % 1000) * 1000;
}


/**
 *	This method returns the TimeQueue of all callbacks managed by BWMachined.
 *
 * 	@returns the BWMachined TimeQueue.
 */
INLINE
TimeQueue64 & BWMachined::callbacks()
{
	return callbacks_;
}

// bwmachined.ipp
