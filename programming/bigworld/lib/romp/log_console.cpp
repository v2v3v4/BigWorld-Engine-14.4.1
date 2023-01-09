#include "pch.hpp"

#include "log_console.hpp"

#if ENABLE_MSG_LOGGING && ENABLE_CONSOLES

#include "font_manager.hpp"

#include "cstdmf/debug_filter.hpp"
#include "cstdmf/log_msg.hpp"

#include <cstdio>
#include <cstdarg>

DECLARE_DEBUG_COMPONENT2( "UI", 0 );


BW_BEGIN_NAMESPACE

MEMTRACKER_DECLARE( LogConsole, "LogConsole", 0 );

// -----------------------------------------------------------------------------
// Section: LogConsole
// -----------------------------------------------------------------------------

// Anonymous namespace for wrapping some compile-time "constants"
namespace
{
static const size_t BUFFERSIZE = 50000;

// White
static const uint32 defaultConsoleColour = 0xffffffff;
// Green
static const uint32 statusOnColour = 0xff00ff00;
// Grey
static const uint32 statusOffColour = 0xff808080;

// Bitwise-&d into consoleColour when in monitor mode
static const uint32 monitorFade = 0xffffffff;

static inline uint32 getColour( DebugMessagePriority priority )
{
	if (priority == MESSAGE_PRIORITY_WARNING)
	{
		// Yellow
		return 0xffffff20;
	}
	else if (priority == MESSAGE_PRIORITY_ERROR ||
		priority == MESSAGE_PRIORITY_CRITICAL)
	{
		// Red
		return 0xffff2020;
	}

	return defaultConsoleColour;
}


} // namespace


/**
 *	Constructor.
 *	
 *	@param creationTime The application's current time, in seconds.
 */
LogConsole::LogConsole( double creationTime ) :
	EditConsole(),
	DebugMessageCallback(),
	categorySet_(),
	messageBuffer_( BUFFERSIZE ),
	messageBufferMutex_(),
	creationTime_( timestamp() - TimeStamp::fromSeconds( creationTime ) ),
	iDisplayBase_( messageBuffer_.rend() ),
	isDirty_( true ),
	isMonitoring_( false ),
	lastRenderMessageCount_( 1 ),
	isWrapping_( false ),
	isColoured_( false ),
	isAutoscrolling_( true ),
	isShowingTimestamp_( true ),
	isPriorityShown_( (2 << NUM_MESSAGE_PRIORITY) - 1 ),
	selectedCategory_( categorySet_.end() ),
	stringMatchFilter_()
{
	BW_GUARD_MEMTRACKER( LogConsole );

	DebugFilter::instance().addMessageCallback( this );

	this->prompt( BW::wstring( L"Filter: " ) );
	this->setConsoleColour( defaultConsoleColour );
}


/**
 *	Destructor
 */
LogConsole::~LogConsole()
{
	BW_GUARD_MEMTRACKER( LogConsole );

	DebugFilter::instance().deleteMessageCallback( this );
}


/**
 *	Handle a debug message broadcast by buffering it if it's not filtered.
 *	
 *	@param messagePriority	The priority of this message
 *	@param pCategory	The category this message is to be logged against
 *	@param messageSource	The source subsystem of this message
 *	@param pFormat	A printf-style format string for this message
 *	@param formatArgs	The stdarg parameters to use with pFormat, owned by the
 *		caller.
 *
 *	@return true if the message has been handled and should not be processed
 *		further, false otherwise.
 */
bool LogConsole::handleMessage(
	DebugMessagePriority messagePriority, const char * pCategory,
	DebugMessageSource messageSource, const LogMetaData & metaData,
	const char * pFormat, va_list formatArgs )
{
	BW_GUARD_MEMTRACKER( LogConsole );

	MF_VERIFY( DebugFilter::instance().shouldAccept(
		messagePriority, pCategory ));

	SimpleMutexHolder messageBufferMutexHolder( messageBufferMutex_ );

	// Will iDisplayBase_ be invalidated after this push?
	// If we're currently empty, then it's _already_ invalid.
	bool shouldResetDisplayBase = (messageBuffer_.empty() ||
			(messageBuffer_.full() &&
				(iDisplayBase_ == (messageBuffer_.rend() - 1))));

	bool isAtBottom = false;
	if (!shouldResetDisplayBase && isAutoscrolling_)
	{
		if (iDisplayBase_ == messageBuffer_.rbegin())
		{
			// We're at the bottom of the buffer
			isAtBottom = true;
		}
		else
		{
			// Check if we're on the last _visible_ message
			MessageBuffer::const_reverse_iterator iLastDisplayBase(
				iDisplayBase_ );

			this->scrollForwards( 1 );

			// If we didn't move...
			if (iDisplayBase_ == iLastDisplayBase)
			{
				isAtBottom = true;
			}
			else
			{
				iDisplayBase_ = iLastDisplayBase;
			}
		}
	}

	// Make room for the new message
	if (messageBuffer_.full())
	{
		if (!shouldResetDisplayBase)
		{
			// Popping an element from the messageBuffer_ moves the buffer
			// under our iterator. This keeps us pointing at the same
			// message.
			iDisplayBase_++;
		}
		messageBuffer_.pop();
	}

	MessageBufferEntry & rNewTail = messageBuffer_.push();

	// push() moves rbegin(), but the iterators don't move with it.
	MF_ASSERT( iDisplayBase_ != messageBuffer_.rbegin() );

	// As per vdprintf. C++11 provides std::vsnprintf to avoid the buffer
	static const int MAX_MESSAGE_BUFFER = 2048;
	static char buf[ MAX_MESSAGE_BUFFER ];
	LogMsg::formatMessage( buf, MAX_MESSAGE_BUFFER, pFormat, formatArgs );

	MF_VERIFY( bw_utf8tow( buf, rNewTail.message ) );

	rNewTail.priority = messagePriority;
	rNewTail.iCategory = categorySet_.insert(
		(pCategory) ? pCategory : BW::string() ).first;
	rNewTail.timestamp = float(creationTime_.ageInSeconds());

	isDirty_ = true;

	if (shouldResetDisplayBase)
	{
		// Current base is invalid
		// Point at the new oldest message in the buffer
		iDisplayBase_ = messageBuffer_.rend() - 1;
	}
	else if (isAutoscrolling_ && isAtBottom)
	{
		this->scrollForwards( 1 );
	}

	return false;
}


/**
 *	This method is called when the ConsoleManager activates this console. The
 *	default behaviour is to no longer be the active console is this call is a
 *	reactivate.
 *
 *	@param isReactivate	True if the console is currently active, otherwise
 *		false.
 */
void LogConsole::activate( bool isReactivate )
{
	BW_GUARD_MEMTRACKER( LogConsole );

	this->EditConsole::activate( isReactivate );
	isDirty_ = true;
}


/**
 *	This method indicates if the current console allows the screen to be
 *	darkened before it is drawn
 *
 *	@return	Returns true if background can be darkened.
 */
bool LogConsole::allowDarkBackground() const
{
	BW_GUARD_MEMTRACKER( LogConsole );

	return !isMonitoring_ && this->EditConsole::allowDarkBackground();
}


/**
 *	This method overrides the InputHandler method to handle key presses.
 *
 *	@param event	The KeyEvent to be handled
 *
 *	@return			True if it was handled, otherwise false.
 */
bool LogConsole::handleKeyEvent( const KeyEvent& event )
{
	BW_GUARD_MEMTRACKER( LogConsole );

	// Monitor mode only looks for one input.
	if (isMonitoring_)
	{
		if (event.isKeyDown() && event.key() == KeyCode::KEY_Z &&
			event.isCtrlDown())
		{
			isMonitoring_ = false;
			this->setConsoleColour( defaultConsoleColour );
			return true;
		}

		return false;
	}

	if (this->EditConsole::handleKeyEvent( event ))
	{
		return true;
	}

	if (!event.isKeyDown())
	{
		return false;
	}

	bool handled = false;

	switch (event.key())
	{
		// Scrolling: Up, Down, PgUp, PgDown, Home, End
		case KeyCode::KEY_UPARROW: // Backwards one message
		{
			this->scrollBackwards( 1 );
			handled = true;
		}
		break;

		case KeyCode::KEY_DOWNARROW: // Forwards one message
		{
			this->scrollForwards( 1 );
			handled = true;
		}
		break;

		case KeyCode::KEY_PGUP: // Backwards a full page
		{
			this->scrollBackwards( lastRenderMessageCount_ - 1 );
			handled = true;
		}
		break;

		case KeyCode::KEY_PGDN: // Forwards a full page
		{
			// TODO: This value isn't right, in the presence of word-wrap
			this->scrollForwards( lastRenderMessageCount_ - 1 );
			handled = true;
		}
		break;

		case KeyCode::KEY_HOME: // Bring oldest message into view
		{
			this->scrollToOldest();
			handled = true;
		}
		break;

		case KeyCode::KEY_END: // Bring newest message into view
		{
			this->scrollToNewest();
			handled = true;
		}
		break;

		// Display toggles: W, C, T
		case KeyCode::KEY_W: // Wrapping toggle
		{
			isWrapping_ = !isWrapping_;
			isDirty_ = true;
			handled = true;
		}
		break;

		case KeyCode::KEY_C: // Colourisation toggle
		{
			isColoured_ = !isColoured_;
			isDirty_ = true;
			handled = true;
		}
		break;

		case KeyCode::KEY_T: // Timestamp toggle
		{
			isShowingTimestamp_ = !isShowingTimestamp_;
			isDirty_ = true;
			handled = true;
		}
		break;

		// Engage monitor mode: Control-Z
		case KeyCode::KEY_Z: // Monitor mode start
		{
			if (event.isCtrlDown())
			{
				isMonitoring_ = true;
				isAutoscrolling_ = true;
				this->scrollToNewest();
				if (this->isCursorShowing())
				{
					this->hideCursor();
				}
				// Fade the console
				this->setConsoleColour( defaultConsoleColour & monitorFade );
				handled = true;
			}
		}
		break;

		// Autoscrolling toggle: S
		case KeyCode::KEY_S: // Autoscroll toggle
		{
			isAutoscrolling_ = !isAutoscrolling_;
			// Not dirtying, this doesn't affect the actual display.
			handled = true;
		}
		break;

		// Priority filtering: 0..9 (+shifted versions), Grave/Tilde
		case KeyCode::KEY_0: // TRACE
		{
			if (event.isShiftDown())
			{
				isPriorityShown_.reset();
			}
			isPriorityShown_[ 0 ].flip();
			isDirty_ = true;
			handled = true;
		}
		break;

		case KeyCode::KEY_1: // DEBUG
		case KeyCode::KEY_2: // INFO
		case KeyCode::KEY_3: // NOTICE
		case KeyCode::KEY_4: // WARNING
		case KeyCode::KEY_5: // ERROR
		case KeyCode::KEY_6: // CRITICAL
		case KeyCode::KEY_7: // HACK
		case KeyCode::KEY_8: // DEPRECATED: SCRIPT
		case KeyCode::KEY_9: // ASSET
		{
			if (event.isShiftDown())
			{
				isPriorityShown_.reset();
			}
			isPriorityShown_[ event.key() - KeyCode::KEY_1 + 1 ].flip();
			isDirty_ = true;
			handled = true;
		}
		break;

		case KeyCode::KEY_GRAVE: // All priorities back on
		{
			isPriorityShown_.set();
			isDirty_ = true;
			handled = true;
		}
		break;

		// Category filtering: Period, Comma, Tab
		case KeyCode::KEY_COMMA:
		{
			this->prevCategoryFilter();
			handled = true;
		}
		break;

		case KeyCode::KEY_PERIOD:
		{
			this->nextCategoryFilter();
			handled = true;
		}
		break;

		case KeyCode::KEY_TAB:
		{
			this->clearCategoryFilter();
			handled = true;
		}
		break;

		// String-match filtering: Return, Escape
		case KeyCode::KEY_RETURN:
		{
			lineEditor_.editString( stringMatchFilter_ );
			this->showCursor();
			handled = true;
		}
		break;

		case KeyCode::KEY_ESCAPE:
		{
			if (this->isCursorShowing())
			{
				this->hideCursor();
				handled = true;
			}
		}
		break;

		default:
		{
		}
		break;
	}

	return handled;
}


/**
 *	This method is called whenever there is a line for this console to process.
 *
 *	@param line	A string containing the current line to be processed.
 */
void LogConsole::processLine( const BW::wstring & rLine )
{
	BW_GUARD_MEMTRACKER( LogConsole );

	stringMatchFilter_ = rLine;

	this->hideCursor();
	isDirty_ = true;
	return;
}


/**
 *	This method is called to update this console.
 */
void LogConsole::update()
{
	BW_GUARD_MEMTRACKER( LogConsole );

	if (isDirty_)
	{
		this->clear();
		lastRenderMessageCount_ = 0;

		// Leaving the editLine alone
		int baseLine = this->editLine() - 1;

		if (this->shouldFilterMessage( *iDisplayBase_ ))
		{
			// Filters changed, our displayBase is invalid
			MessageBuffer::const_reverse_iterator iLast = iDisplayBase_;

			// Search towards now first, then if nothing found search backwards
			this->scrollForwards( 0 );

			// No non-filtered messages between iLast and present.
			if (iLast == iDisplayBase_)
			{
				iDisplayBase_ = iLast;
				this->scrollBackwards( 0 );

				// Nothing at all to display
				if (iLast == iDisplayBase_)
				{
					lastRenderMessageCount_ = 0;
					isDirty_ = false;
					this->EditConsole::update();
					return;
				}
			}
		}

		MessageBuffer::const_reverse_iterator iMessage;
		for( iMessage = iDisplayBase_;
			iMessage != messageBuffer_.rend();
			++iMessage)
		{
			if (this->shouldFilterMessage( *iMessage ))
			{
				continue;
			}

			if (!this->renderLine( *iMessage, baseLine ))
			{
				break;
			}
			lastRenderMessageCount_++;
		}

		// If we ran out of messages, count blank lines as rendered messages
		if (iMessage == messageBuffer_.rend())
		{
			lastRenderMessageCount_ += baseLine + 1;
		}

		isDirty_ = false;
	}

	this->EditConsole::update();
}


/**
 *	This method draws this console.
 *
 *	@param dTime	The elapsed time since the last time the console was drawn.
 */
void LogConsole::draw( float dTime )
{
	BW_GUARD_MEMTRACKER( LogConsole );

	this->EditConsole::draw( dTime );

	this->drawStatusLine();
}


/**
 *	This method renders the given MessageBufferEntry based at the given line
 *	if there's enough room, and updates the line entry to the next unused line.
 *	
 *	It doesn't render partial-lines when wrapping.
 *	
 *	@param rEntry		The MessageBufferEntry to render
 *	@param rBaseLine	The screen line at which to start rendering, moving
 *		upwards. It will be left pointing to the next screen line to be rendered
 *		to.
 *	
 *	@return	True if there was enough room to render the MessageBufferEntry
 *		according to the current settings, otherwise false.
 */
bool LogConsole::renderLine( const MessageBufferEntry & rEntry,
	int & rBaseLine )
{
	BW_GUARD_MEMTRACKER( LogConsole );

	if (rBaseLine < 0)
	{
		return false;
	}

	BW::wstringstream outputStream;
	if (isShowingTimestamp_)
	{
		outputStream << std::fixed << std::showpoint;
		outputStream.precision( 3 );
		outputStream << rEntry.timestamp << L" ";
	}

	outputStream << messagePrefix( rEntry.priority ) << L": ";

	if (!rEntry.iCategory->empty())
	{
		outputStream << L"[" << bw_utf8tow( *(rEntry.iCategory) ) << L"] ";
	}

	outputStream << rEntry.message;

	BW::wstring outputLine = outputStream.str();

	const unsigned int wrapWidth = this->visibleWidth();

	if (isWrapping_)
	{
		const int lineLength = static_cast<int>( outputLine.length() );
		int lines = ((lineLength - 1) / wrapWidth) + 1;
		if (rBaseLine < lines - 1)
		{
			return false;
		}

		rBaseLine -= lines;

		for (int i = 0; i < lines; ++i)
		{
			this->line( rBaseLine + i + 1,
				outputLine.c_str() + i * wrapWidth, wrapWidth );
			this->colourLine( rBaseLine + i + 1, rEntry );
		}

	}
	else
	{
		this->line( rBaseLine, outputLine.c_str(), outputLine.length() );
		this->colourLine( rBaseLine, rEntry );

		if (outputLine.length() > wrapWidth)
		{
			// TODO: Find a way to colour this differently
			this->setChar( rBaseLine, wrapWidth-1, L'>' );
		}

		--rBaseLine;
	}

	return true;
}


/**
 *	This method sets the appropriate colour for a line
 *	
 *	@param line		The screen line to set the colour for
 *	@param rEntry	The MessageBufferEntry on whose behalf this line is to be
 *		coloured.
 */
void LogConsole::colourLine( int line, const MessageBufferEntry & rEntry )
{
	BW_GUARD_MEMTRACKER( LogConsole );

	if (isColoured_)
	{
		this->lineColourOverride( line, getColour( rEntry.priority ) );
	}
	else
	{
		this->lineColourOverride( line );
	}
}


/**
 *	This method draws a status line if the EditConsole input line is not
 *	currently visible.
 */
void LogConsole::drawStatusLine()
{
	BW_GUARD_MEMTRACKER( LogConsole );

	if (this->isCursorShowing())
	{
		return;
	}

/* Format:
Scroll Wrap Colour Priorities: DINWECHSAT Category Filter: <FilterString>
<---- Toggles ---> <- Deflt -> <-Toggles> <Toggle> <Deflt> <- StatusOn ->
Toggles are StatusOn when on, StatusOff when off.
*/

	// Liberally borrowed from XConsole::draw, assumes it has been run first
	if (!FontManager::instance().begin( *font_ ) )
	{
		return;
	}

	int column = 0;

	uint32 currConsoleColour = this->consoleColour();
	float mainAlpha = (currConsoleColour >> 24) / 255.f;

	uint32 currStatusOnColour = ( (statusOnColour & 0xffffff) |
			uint32( (statusOnColour >> 24) * mainAlpha ) << 24 );
	uint32 currStatusOffColour = ( (statusOffColour & 0xffffff) |
		uint32( (statusOffColour >> 24) * mainAlpha ) << 24 );

	if (isMonitoring_)
	{
		column += this->drawStatusWString(
			BW::wstring( L"Monitor mode: Ctrl-Z to exit" ), column,
			currStatusOnColour );
		column++;
	}
	else
	{
		column += this->drawStatusWString( BW::wstring( L"Scroll" ), column,
			(isAutoscrolling_ ? currStatusOnColour : currStatusOffColour) );
		column++;
	}

	column += this->drawStatusWString( BW::wstring( L"Wrap" ), column,
		(isWrapping_ ? currStatusOnColour : currStatusOffColour) );
	column++;

	column += this->drawStatusWString( BW::wstring( L"Colour" ), column,
		(isColoured_ ? currStatusOnColour : currStatusOffColour) );
	column++;

	// Deliberately not showing Timestamp toggle here, as it should be obvious
	// from the actual displayed messages.

	column += this->drawStatusWString( BW::wstring( L"Priorities:" ), column,
		currConsoleColour );
	column++;

	column += this->drawStatusWString( BW::wstring( L"D" ), column,
		(isPriorityShown_[1] ? currStatusOnColour : currStatusOffColour) );
	column += this->drawStatusWString( BW::wstring( L"I" ), column,
		(isPriorityShown_[2] ? currStatusOnColour : currStatusOffColour) );
	column += this->drawStatusWString( BW::wstring( L"N" ), column,
		(isPriorityShown_[3] ? currStatusOnColour : currStatusOffColour) );
	column += this->drawStatusWString( BW::wstring( L"W" ), column,
		(isPriorityShown_[4] ? currStatusOnColour : currStatusOffColour) );
	column += this->drawStatusWString( BW::wstring( L"E" ), column,
		(isPriorityShown_[5] ? currStatusOnColour : currStatusOffColour) );
	column += this->drawStatusWString( BW::wstring( L"C" ), column,
		(isPriorityShown_[6] ? currStatusOnColour : currStatusOffColour) );
	column += this->drawStatusWString( BW::wstring( L"H" ), column,
		(isPriorityShown_[7] ? currStatusOnColour : currStatusOffColour) );
	column += this->drawStatusWString( BW::wstring( L"S" ), column,
		(isPriorityShown_[8] ? currStatusOnColour : currStatusOffColour) );
	column += this->drawStatusWString( BW::wstring( L"A" ), column,
		(isPriorityShown_[9] ? currStatusOnColour : currStatusOffColour) );
	column += this->drawStatusWString( BW::wstring( L"T" ), column,
		(isPriorityShown_[0] ? currStatusOnColour : currStatusOffColour) );
	column++;

	column += this->drawStatusWString( BW::wstring( L"Category" ), column,
		((selectedCategory_ != categorySet_.end()) ?
			currStatusOnColour : currStatusOffColour) );
	column++;

	column += this->drawStatusWString( BW::wstring( L"Filter:" ), column,
		currConsoleColour );
	column++;

	column += this->drawStatusWString( stringMatchFilter_, column,
		currStatusOnColour );

	FontManager::instance().end();
}


/**
 *	This method renders a given string to the given column of the status line,
 *	in the given colour, assuming our font_ is already active.
 *	
 *	@param rString	The wide-character string to be rendered
 *	@param column	The column at which to start rendering
 *	@param colour	The colour in which to render the string
 *	
 *	@return	The number of characters written
 */
int LogConsole::drawStatusWString( const BW::wstring & rString, int column,
	uint32 colour )
{
	BW_GUARD_MEMTRACKER( LogConsole );

	int visWidth = this->visibleWidth();

	if (column >= visWidth)
	{
		return 0;
	}

	int row = this->editLine();

	font_->colour( colour );

	unsigned int availableSpace = visWidth - column;

	if (rString.length() <= availableSpace)
	{
		font_->drawWConsoleString( rString, column, row, int(offset_.x),
			int(offset_.y) );
		return static_cast<int>( rString.length() );
	}
	else
	{
		const BW::wstring buffer( rString, 0, availableSpace - 1 );
		font_->drawWConsoleString( buffer, column, row, int(offset_.x),
			int(offset_.y) );
		font_->colour( this->consoleColour() );
		font_->drawWConsoleString( BW::wstring( L">" ), visWidth - 1, row,
			int(offset_.x), int(offset_.y) );
		return availableSpace;
	}
}


/**
 *	This method scrolls the virtual buffer's view of the message history log to
 *	the present.
 */
void LogConsole::scrollToNewest()
{
	BW_GUARD_MEMTRACKER( LogConsole );

	MessageBuffer::const_reverse_iterator iRNewest = messageBuffer_.rbegin();

	bool found = false;

	while (iRNewest != messageBuffer_.rend())
	{
		if (!this->shouldFilterMessage( *iRNewest ))
		{
			found = true;
			break;
		}

		++iRNewest;
	}

	if (!found)
	{
		return;
	}

	if (iDisplayBase_ != iRNewest)
	{
		iDisplayBase_ = iRNewest;
		isDirty_ = true;
	}
}


/**
 *	This method scrolls the virtual buffer's view of the message history log
 *	towards the present.
 *	
 *	@param messageCount	The number of visible messages to scroll by.
 */
void LogConsole::scrollForwards( int messageCount )
{
	BW_GUARD_MEMTRACKER( LogConsole );

	MessageBuffer::const_iterator iCandidate( iDisplayBase_.base() );
	--iCandidate;

	bool found = false;

	MessageBuffer::const_iterator iLastGood;

	while (iCandidate != messageBuffer_.end())
	{
		if (!this->shouldFilterMessage( *iCandidate ))
		{
			iLastGood = iCandidate;
			found = true;

			// If messageCount was already 0, take this candidate
			// Otherwise, decrement it and search onwards.
			if (messageCount-- == 0)
			{
				break;
			}
		}
		++iCandidate;
	}

	if (!found)
	{
		return;
	}

	MessageBuffer::const_reverse_iterator iRLastGood( iLastGood );
	--iRLastGood;

	if (iDisplayBase_ != iRLastGood)
	{
		iDisplayBase_ = iRLastGood;
		isDirty_ = true;
	}
}


/**
 *	This method scrolls the virtual buffer's view of the message history log
 *	towards the past.
 *	
 *	@param messageCount	The number of visible messages to scroll by.
 */
void LogConsole::scrollBackwards( int messageCount )
{
	BW_GUARD_MEMTRACKER( LogConsole );

	MessageBuffer::const_reverse_iterator iRCandidate = iDisplayBase_;

	bool found = false;

	MessageBuffer::const_reverse_iterator iRLastGood;

	while (iRCandidate != messageBuffer_.rend())
	{
		if (!this->shouldFilterMessage( *iRCandidate ))
		{
			iRLastGood = iRCandidate;
			found = true;

			// If messageCount was already 0, take this candidate
			// Otherwise, decrement it and search onwards.
			if (messageCount-- == 0)
			{
				break;
			}
		}
		++iRCandidate;
	}

	if (!found)
	{
		return;
	}

	if (iDisplayBase_ != iRLastGood)
	{
		iDisplayBase_ = iRLastGood;
		isDirty_ = true;
	}
}


/**
 *	This method scrolls the virtual buffer's view of the message history log to 
 *	the oldest point possible.
 */
void LogConsole::scrollToOldest()
{
	BW_GUARD_MEMTRACKER( LogConsole );

	MessageBuffer::const_iterator iOldest = messageBuffer_.begin();

	bool found = false;

	while (iOldest != messageBuffer_.end())
	{
		if (!this->shouldFilterMessage( *iOldest ))
		{
			found = true;
			break;
		}

		++iOldest;
	}

	if (!found)
	{
		return;
	}

	MessageBuffer::const_reverse_iterator iROldest( iOldest );
	--iROldest;

	if (iDisplayBase_ != iROldest)
	{
		iDisplayBase_ = iROldest;
		isDirty_ = true;
	}
}


/**
 *	This method indicates if the given message should be filtered out.
 *	
 *	@param rMessage	The message to consider for filtering
 *	
 *	@return	True if the message should be filtered out (not displayed),
 *	otherwise false.
 */
bool LogConsole::shouldFilterMessage(
	const MessageBufferEntry & rMessage ) const
{
	BW_GUARD_MEMTRACKER( LogConsole );

	if (!isPriorityShown_[ rMessage.priority ])
	{
		return true;
	}

	bool shouldCategoryFilter = (selectedCategory_ != categorySet_.end());

	if (shouldCategoryFilter &&
		rMessage.iCategory != selectedCategory_ )
	{
		return true;
	}

	bool shouldStringMatchFilter = !stringMatchFilter_.empty();

	// TODO: Case-insensitive matching?
	// http://stackoverflow.com/questions/3152241/case-insensitive-stdstring-find
	// http://stackoverflow.com/questions/4943770/case-insensitive-stringfind
	if (shouldStringMatchFilter &&
		rMessage.message.find( stringMatchFilter_ ) ==
		rMessage.message.npos )
	{
		return true;
	}

	return false;
}


/**
 *	This method advances to the next category filter
 */
void LogConsole::nextCategoryFilter()
{
	BW_GUARD_MEMTRACKER( LogConsole );

	if (selectedCategory_ == categorySet_.end())
	{
		selectedCategory_ = categorySet_.begin();
	}
	else
	{
		selectedCategory_++;
	}

	isDirty_ = true;
}


/**
 *	This method retreats to the previous category filter
 */
void LogConsole::prevCategoryFilter()
{
	BW_GUARD_MEMTRACKER( LogConsole );

	if (selectedCategory_ == categorySet_.begin())
	{
		selectedCategory_ = categorySet_.end();
	}
	else
	{
		selectedCategory_--;
	}

	isDirty_ = true;
}


/**
 *	This method clears all category filtering
 */
void LogConsole::clearCategoryFilter()
{
	BW_GUARD_MEMTRACKER( LogConsole );

	if (selectedCategory_ != categorySet_.end())
	{
		selectedCategory_ = categorySet_.end();
		isDirty_ = true;
	}
}

BW_END_NAMESPACE

#endif // ENABLE_MSG_LOGGING && ENABLE_CONSOLES

// log_console.cpp
