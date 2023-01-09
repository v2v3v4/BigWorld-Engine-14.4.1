#ifdef CODE_INLINE
#define INLINE    inline
#else
/// INLINE macro.
#define INLINE
#endif

// -----------------------------------------------------------------------------
// Section: Accessors
// -----------------------------------------------------------------------------



/**
 *	This method returns the name of this description.
 */
INLINE const BW::string& BaseUserDataObjectDescription::name() const
{
	return name_;
}

// base_user_data_object_description.ipp
