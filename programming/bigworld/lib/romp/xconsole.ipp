
#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif


/**
 *	This method returns whether or not the cursor is showing.
 */
INLINE bool XConsole::isCursorShowing() const
{
	BW_GUARD;
	return showCursor_;
}


/**
 *  This method returns the color of the console text.
 */
INLINE uint32 XConsole::consoleColour() const
{
	BW_GUARD;
	return consoleColour_;
}


/**
 *	This method sets the position of the cursor.
 */
INLINE void XConsole::setCursor( uint x, uint y )
{
	BW_GUARD;
	this->cursorX( x );
	this->cursorY( y );
}


/**
 *	This method returns the x position (or column) of the cursor.
 */
INLINE uint XConsole::cursorX() const
{
	BW_GUARD;
	return cursorX_;
}


/**
 *	This method returns the y position (or row) of the cursor.
 */
INLINE uint XConsole::cursorY() const
{
	BW_GUARD;
	return cursorY_;
}



/**
 *	This method sets the x position (or column) of the cursor.
 */
INLINE void XConsole::cursorX( uint x )
{
	BW_GUARD;
	cursorX_ = max( 0, min( (int)x, MAX_CONSOLE_WIDTH - 1 ) );
}


/**
 *	This method sets the y position (or row) of the cursor.
 */
INLINE void XConsole::cursorY( uint y )
{
	BW_GUARD;	
//	cursorY_ = max( 0, min( (int)y, MAX_CONSOLE_HEIGHT - 1 ) );
	cursorY_ = y;
}


/**
 *	This method returns the scroll offset of this console.
 */
INLINE int XConsole::scrollOffset() const
{
	BW_GUARD;
	return scrollOffset_;
}


/**
 *	This method returns the scroll offset of this console.
 */
INLINE void XConsole::scrollOffset( int offset )
{
	BW_GUARD;
	scrollOffset_ = offset;
	this->onScroll();
}


/**
 *	This method returns the scrolls the console down.
 */
INLINE void XConsole::scrollDown()
{
	BW_GUARD;
	scrollOffset_++;
	this->onScroll();
}


/**
 *	This method returns the scrolls the console up.
 */
INLINE void XConsole::scrollUp()
{
	BW_GUARD;
	scrollOffset_--;

	if (scrollOffset_ < 0)
	{
		scrollOffset_ = 0;
	}
	this->onScroll();
}

/**
 *	Gets the buffer for the given line.
 */
INLINE const wchar_t * XConsole::line( int line ) const
{
	BW_GUARD;
	MF_ASSERT( line >= 0);
	MF_ASSERT( line < MAX_CONSOLE_HEIGHT );
	return this->buffer_[ line ];
}

/**
 *	Gets the buffer for the given line.
 */
INLINE wchar_t * XConsole::line( int line )
{
	BW_GUARD;
	MF_ASSERT( line >= 0);
	MF_ASSERT( line < MAX_CONSOLE_HEIGHT );
	return this->buffer_[ line ];
}


/**
 *	Gets the colour for this line
 *
 *	@return true if this line had a colour override
 */
INLINE bool XConsole::lineColourRetrieve( int line, uint32 & rColour ) const
{
	BW_GUARD;
	if (lineColours_[line].inUse)
	{
		rColour = lineColours_[line].colour;
		return true;
	}
	else
	{
		rColour = consoleColour_;
		return false;
	}
}


// xconsole.ipp
