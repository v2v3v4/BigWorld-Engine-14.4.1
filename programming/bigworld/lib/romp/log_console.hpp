#pragma once
#ifndef LOG_CONSOLE_HPP
#define LOG_CONSOLE_HPP

#include "cstdmf/config.hpp"

#if ENABLE_MSG_LOGGING && ENABLE_CONSOLES

#include "console.hpp"

#include "cstdmf/bw_string.hpp"
#include "cstdmf/circular_queue.hpp"
#include "cstdmf/debug_message_callbacks.hpp"

#include <bitset>
#include "cstdmf/bw_set.hpp"
#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_vector.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class is a console for showing a filtered collection of debug
 *	messages. It self-registers with DebugFilter on instantiation.
 *	
 *	Currently supported keyboard inputs:
 *	Up, Down, PgUp, PgDown, Home, End: Scroll through the log buffer
 *	W, C, T: Toggle line-wrapping, colourisation and timestamp display
 *	Control-Z: Toggle monitor mode, disables all other input except Control-Z
 *	S: Toggle auto-scrolling when at the most recent message
 *	0..9: Toggle display of the message priorities
 *	Shift-0..9: Display only that priority
 *	Grave/Tilde: Display all priorities
 *	Period, Comma: Move forward and backward through categories to show
 *	Tab: Show all categories of messages
 *	Return: Edit the current string-match filter
 *	Escape: Abort editing of the string-match filter
 */
class LogConsole : public EditConsole, public DebugMessageCallback
{
public:
	LogConsole( double creationTime );
	~LogConsole();

private:
	/// DebugMessageCallback implementation
	virtual bool handleMessage(
		DebugMessagePriority messagePriority, const char * pCategory,
		DebugMessageSource messageSource, const LogMetaData & metaData,
		const char * pFormat, va_list formatArgs );


	/// EditConsole implementation
	void activate( bool isReactivate );
	bool allowDarkBackground() const;
	bool handleKeyEvent( const KeyEvent& event );
	void processLine( const BW::wstring & rLine );
	void update();
	void draw( float dTime );

	/// Seen categories
	typedef BW::set< BW::string > CategorySet;

	CategorySet categorySet_;

	/// Circular message buffer
	struct MessageBufferEntry
	{
		DebugMessagePriority priority;
		CategorySet::const_iterator iCategory;
		BW::wstring message;
		float timestamp;
	};

	typedef CircularQueue< MessageBufferEntry > MessageBuffer;

	// A circular buffer of debug messages, begin() is the oldest message,
	// and rbegin() is the most recent.
	MessageBuffer messageBuffer_;
	// A mutex to protect updates to messageBuffer_
	SimpleMutex messageBufferMutex_;
	// A timestamp relative to the creation timestamp, for setting the timestamp
	// on messages as they are added to the buffer
	TimeStamp creationTime_;

	/// Rendering buffer interface
	bool renderLine( const MessageBufferEntry & rEntry, int & rBaseLine );
	void colourLine( int line, const MessageBufferEntry & rEntry );
	void drawStatusLine();
	int drawStatusWString( const BW::wstring & rString, int column,
		uint32 colour );

	void scrollToNewest();
	void scrollForwards( int messageCount );
	void scrollBackwards( int messageCount );
	void scrollToOldest();

	/// Filtering helper methods
	bool shouldFilterMessage( const MessageBufferEntry & rMessage ) const;

	void nextCategoryFilter();
	void prevCategoryFilter();
	void clearCategoryFilter();

	// This is the message at the bottom of the log display screen. We try to
	// hold it steady as filter changes. Paging down should move this to the
	// top of the screen, and paging up should replace this with the current
	// top line of the screen.
	MessageBuffer::const_reverse_iterator iDisplayBase_;

	// Do we need to re-render our output?
	bool isDirty_;

	// Are we in monitor mode, ignoring all input except the key to disable
	// monitor mode, and not darkening the screen
	bool isMonitoring_;

	// How many messages did we render last time we updated?
	int lastRenderMessageCount_;

	/// Toggles for render settings
	bool isWrapping_;
	bool isColoured_;
	bool isAutoscrolling_;
	bool isShowingTimestamp_;

	/// Message filtering settings
	std::bitset< NUM_MESSAGE_PRIORITY > isPriorityShown_;
	CategorySet::const_iterator selectedCategory_;
	BW::wstring stringMatchFilter_;
};

BW_END_NAMESPACE

#endif // ENABLE_MSG_LOGGING && ENABLE_CONSOLES

#endif // LOG_CONSOLE_HPP
