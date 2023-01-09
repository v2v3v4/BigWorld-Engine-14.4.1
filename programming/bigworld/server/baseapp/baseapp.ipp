#ifdef CODE_INLINE
#define INLINE    inline
#else
#define INLINE
#endif

/**
 *	This method returns whether the next packet forwarded to the client should
 *	be made reliable. It is used by CellApp to force volatile messages to be
 *	reliable if they are being used to "select" the entity that subsequent
 *	messages are destined for.
 *
 *	Note that this state is cleared on read.
 */
INLINE bool BaseApp::shouldMakeNextMessageReliable()
{
	bool result = shouldMakeNextMessageReliable_;
	shouldMakeNextMessageReliable_ = false;

	return result;
}

// baseapp.ipp
