#ifdef CODE_INLINE
#define INLINE	inline
#else
/// INLINE macro.
#define INLINE
#endif

// -----------------------------------------------------------------------------
// Section: Accessors
// -----------------------------------------------------------------------------

/**
 *	This method returns the priority associated with this method. It is used for
 *	deciding whether or not to send to the client.
 */
INLINE float MethodDescription::priority() const
{
	return priority_;
}

// method_description.ipp
