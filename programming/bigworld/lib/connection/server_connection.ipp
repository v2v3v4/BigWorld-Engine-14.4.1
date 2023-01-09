#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif


/**
 * 	This method changes the default inactivity timeout. If no packets are
 *	received for this amount of time, the connection will timeout and
 *	disconnect. This period is also used to calculate average packet loss. This
 *	method should be called before logging in.
 *
 * 	@param seconds Inactivity timeout in seconds
 */
INLINE
void ServerConnection::setInactivityTimeout( float seconds )
{
	inactivityTimeout_ = seconds;
}


/**
 *	This method is used to set the pointer to current time so that this object
 *	has access to the application's time. It is used for server statistics and
 *	for syncronising between client and server time.
 *
 *	TODO: This needs to be reviewed.
 */
INLINE void ServerConnection::pTime( const double * pTime )
{
	pTime_ = pTime;
}


/**
 *	This method returns the current time that the application thinks it is.
 */
INLINE double ServerConnection::appTime() const
{
	return (pTime_ != NULL) ? *pTime_ : 0.f;
}


// server_connection.ipp
