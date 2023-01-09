
#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif

// closed_captions.ipp
BW_BEGIN_NAMESPACE

/**
 *	This method sets the visibility of the closed captions.
 *
 *	@param	state	The new visibility state.
 */
INLINE void ClosedCaptions::visible( bool state )
{
	root_->visible( state );
}


/**
 *	This method returns the visibility of the closed captions.
 *
 *	@return True if the closed captions are visibile.
 */
INLINE bool ClosedCaptions::visible() const
{
	return root_->visible();
}

BW_END_NAMESPACE

// closed_captions.ipp
