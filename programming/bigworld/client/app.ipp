// app.ipp

#ifdef CODE_INLINE
    #define INLINE    inline
#else
    #define INLINE
#endif

/**
 *	This method gets the time that the last frame took
 */
INLINE
float App::getFrameGameDelta( void ) const
{
	return dGameTime_;
}


/**
 * This method returns the compile time string for the application.
 */
INLINE
const char * App::compileTime() const
{
	return compileTime_.c_str();
}

INLINE
Moo::DrawContext&	App::drawContext( App::DrawContextType drawContextType )
{
	return *drawContexts_[drawContextType];
}



// app.ipp
