
#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif

/**
 *	This method returns the font name.
 *
 *	@return	BW::string		The name of the font
 */
INLINE const BW::string PyFont::name() const
{
	BW_GUARD;
	return pFont_->metrics().cache().fontFileName();
}

INLINE uint PyFont::stringWidth(
	const BW::wstring& theString,
	bool multiline,
	bool colourFormatting ) const
{
	BW_GUARD;
	return pFont_->metrics()
		.stringWidth( theString, multiline, colourFormatting );
}

INLINE PyObject* PyFont::stringDimensions(
	const BW::wstring& theString,
	bool multiline,
	bool colourFormatting ) const
{
	BW_GUARD;
	int w = 0;
	int h = 0;
	
	if ( pFont_ )
	{
		pFont_->metrics()
			.stringDimensions( theString, w, h, multiline, colourFormatting );
	}
	
	PyObject* r = PyTuple_New( 2 );
	PyTuple_SetItem( r, 0, PyInt_FromLong( w ) );
	PyTuple_SetItem( r, 1, PyInt_FromLong( h ) );
	return r;
}
