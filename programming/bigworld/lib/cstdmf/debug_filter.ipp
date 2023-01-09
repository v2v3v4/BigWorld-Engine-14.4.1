#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif


/**
 *	This method returns the singleton instance of this class.
 */
INLINE /*static*/ DebugFilter & DebugFilter::instance()
{
	if (s_instance_ == NULL)
	{
		s_instance_ = new DebugFilter();
	}

	return *s_instance_;
}


/**
 *	This method finalises (deletes) the singleton instance of this class.
 */
INLINE /*static*/ void DebugFilter::fini()
{
	if (s_instance_)
	{
		bw_safe_delete( s_instance_ );
	}
}


/**
 *	This method returns whether or not a message with the input priorities
 *	should be accepted.
 *
 *	@param messagePriority	The priority of the message to test for acceptance.
 *	@param pCategoryName  The category name to test for acceptance.
 */
INLINE /*static*/ bool DebugFilter::shouldAccept(
	DebugMessagePriority messagePriority, const char * pCategoryName )
{
	if (messagePriority >= static_cast< DebugMessagePriority >(
		DebugFilter::instance().filterThreshold() ))
	{
		return DebugFilter::instance().shouldAcceptCategory( pCategoryName,
				messagePriority );
	}

	return false;
}


/**
 *	
 */
INLINE /*static*/ bool DebugFilter::shouldWriteTimePrefix()
{
	return s_shouldWriteTimePrefix_;
}


/**
 *	
 */
INLINE /*static*/ void DebugFilter::shouldWriteTimePrefix( bool value )
{
	s_shouldWriteTimePrefix_ = value;
}


/**
 *	
 */
INLINE /*static*/ bool DebugFilter::shouldWriteToConsole()
{
	return s_shouldWriteToConsole_;
}


/**
 *	
 */
INLINE /*static*/ void DebugFilter::shouldWriteToConsole( bool value )
{
	s_shouldWriteToConsole_ = value;
}


/**
 *	
 */
INLINE /*static*/ bool DebugFilter::shouldOutputErrorBackTrace()
{
	return s_shouldOutputErrorBackTrace_;
}


/**
 *	
 */
INLINE /*static*/ void DebugFilter::shouldOutputErrorBackTrace( bool value )
{
	s_shouldOutputErrorBackTrace_ = value;
}


/**
 *	
 */
INLINE /*static*/ void DebugFilter::consoleOutputFile( FILE * value )
{
	s_consoleOutputFile_.set( value );
}


/**
 *	
 */
INLINE /*static*/ FILE * DebugFilter::consoleOutputFile()
{
	return s_consoleOutputFile_.get();
}

// debug_filter.ipp
