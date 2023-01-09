#include "pch.hpp"

#include "bw_string.hpp"

#include "debug_filter.hpp"

#include "debug_message_callbacks.hpp"
#include "watcher.hpp"

#include "cstdmf/bw_map.hpp"
#include "cstdmf/bw_vector.hpp"

BW_BEGIN_NAMESPACE


#if !defined( CODE_INLINE )
#include "debug_filter.ipp"
#endif

/*static*/ DebugFilter * DebugFilter::s_instance_ = NULL;

DebugFilter::FileInitWrapper BW_STATIC_INIT_PRIORITY( 1300 )
	DebugFilter::s_consoleOutputFile_( stderr );

bool DebugFilter::s_shouldWriteTimePrefix_ = false;
bool DebugFilter::s_shouldOutputErrorBackTrace_ = false;

#define BW_PLATFORM_SUPPORTS_CONSOLE (defined( MF_SERVER ) || defined( PLAYSTATION3 ) || defined( __APPLE__ ))

#if BW_PLATFORM_SUPPORTS_CONSOLE
bool DebugFilter::s_shouldWriteToConsole_ = true;
#else
bool DebugFilter::s_shouldWriteToConsole_ = false;
#endif

#if defined( _WIN32 )
/*static*/ bool DebugFilter::alwaysHandleMessage_ = false;
#endif

/**
 *	Constructor.
 */
DebugFilter::DebugFilter() :
	filterThreshold_( MESSAGE_PRIORITY_TRACE ),
	hasDevelopmentAssertions_( false )
{
	this->addWatchers();
}


/**
 *	Destructor.
 */
DebugFilter::~DebugFilter()
{
	this->removeWatchers();
}


/**
 *	This method	returns the threshold for the filter. Only messages with
 *	a message priority greater than or equal to this limit will be
 *	displayed.
 */
DebugMessagePriority DebugFilter::filterThreshold() const
{
	return filterThreshold_;
}


/**
 *	This method sets the threshold for the filter.
 *
 *	@see filterThreshold
 */
void DebugFilter::filterThreshold( DebugMessagePriority value )
{
	filterThreshold_ = value;
}


/**
 *	This method	returns whether or not development time assertions should
 *	cause a real assertion.
 */
bool DebugFilter::hasDevelopmentAssertions() const
{
	return hasDevelopmentAssertions_;
}


/**
 *	This method	sets whether or not development time assertions should
 *	cause a real assertion.
 */
void DebugFilter::hasDevelopmentAssertions( bool value )
{
	hasDevelopmentAssertions_ = value;
}


/**
 *	This method sets a callback associated with a critical message.
 *	DebugFilter now owns the object pointed to by pCallback.
 *
 *	@param pCallback A pointer to the callback to add.
 */
void DebugFilter::addCriticalCallback( CriticalMessageCallback * pCallback )
{
	RecursiveMutexHolder lock( criticalCallbackMutex_ );
	pCriticalCallbacks_.push_back( pCallback );
}


/**
 *	This method removes a callback associated with a critical message.
 *
 *	@param pCallback A pointer to the callback to remove.
 */
void DebugFilter::deleteCriticalCallback( CriticalMessageCallback * pCallback )
{
	RecursiveMutexHolder lock( criticalCallbackMutex_ );
	CriticalCallbacks::iterator it = std::find( pCriticalCallbacks_.begin(),
		pCriticalCallbacks_.end(), pCallback );
	if (it != pCriticalCallbacks_.end())
	{
		pCriticalCallbacks_.erase( it );
	}
}


/**
 *	Pass critical message through to handlers.
 */
void DebugFilter::handleCriticalMessage( const char * message )
{
	RecursiveMutexHolder lock( criticalCallbackMutex_ );
	// iterate by index in case a callback modifies the list
	for (size_t i = 0; i < pCriticalCallbacks_.size(); ++i)
	{
		pCriticalCallbacks_[i]->handleCritical( message );
	}
}


/**
 *	This method sets a callback associated with a debug message. If the
 *	callback is set and returns true, the default behaviour for displaying
 *	messages is not done.
 *
 * @param pCallback A pointer to the callback class to add to the filter.
 * @param ignoreIfMsgLoggingIsDisabled, if this is true then callback will
 * not be added if message logging is disabled.
 */
void DebugFilter::addMessageCallback( DebugMessageCallback * pCallback,
	bool ignoreIfMsgLoggingIsDisabled )
{
#if !ENABLE_MSG_LOGGING
	if (ignoreIfMsgLoggingIsDisabled)
	{
		return;
	}
#endif
	RecursiveMutexHolder lock( messageCallbackMutex_ );
	pMessageCallbacks_.push_back( pCallback );
}


/**
 *	This method removes a callback associated with a debug message.
 *
 *	@param pCallback A pointer to the callback to remove.
 */
void DebugFilter::deleteMessageCallback( DebugMessageCallback * pCallback )
{
	RecursiveMutexHolder lock( messageCallbackMutex_ );
	DebugCallbacks::iterator it = std::find( pMessageCallbacks_.begin(),
		pMessageCallbacks_.end(), pCallback );
	if (it != pMessageCallbacks_.end())
	{
		pMessageCallbacks_.erase( it );
	}
}


/**
 *	Passes message through to message handlers
 */
bool DebugFilter::handleMessage(
	DebugMessagePriority messagePriority, const char * pCategory,
	DebugMessageSource messageSource, const LogMetaData & metaData,
	const char * pFormat, va_list argPtr )
{
	RecursiveMutexHolder lock( messageCallbackMutex_ );
	bool handled = false;

	// iterate by index in case a callback modifies the list
	for (size_t i = 0; !handled && i < pMessageCallbacks_.size(); ++i)
	{
		// Make a copy of the va_list here to avoid crashing on 64bit
		va_list tmpArgPtr;
		bw_va_copy( tmpArgPtr, argPtr );
		handled = pMessageCallbacks_[i]->handleMessage(
			messagePriority, pCategory,
			messageSource, metaData, pFormat, tmpArgPtr );
		va_end( tmpArgPtr );
	}
	return handled;
}


/**
 *	This method adds a category to the suppression list.
 *
 * 	@param categoryName	The category name to suppress.
 * 	@param minAcceptedPriorityLevel The minimum priority level of a message
 * 			to allow for the category.
 */
void DebugFilter::setCategoryFilter( const BW::string & categoryName,
	DebugMessagePriority minAcceptedPriorityLevel )
{
	// Ignore requests to filter non-categorised messages
	if (categoryName.empty())
	{
		return;
	}

	ReadWriteLock::WriteGuard writeGuard( suppressedCategoriesLock_ );
	suppressedCategories_[ categoryName ] = minAcceptedPriorityLevel;
}


/**
 *
 */
bool DebugFilter::shouldAcceptCategory( const char * pCategoryName,
		DebugMessagePriority messagePriority )
{
	// Non-categorised messages are not filtered
	if ((pCategoryName == NULL) || (pCategoryName[0] == '\0'))
	{
		return true;
	}

	BW::string categoryAsString( pCategoryName );
	{
		ReadWriteLock::ReadGuard readGuard( suppressedCategoriesLock_ );
		CategoriesMap::const_iterator it = suppressedCategories_.find(
				categoryAsString );

		if (it != suppressedCategories_.end())
		{ 
			return messagePriority >= it->second;
		}
	}

	{
		// NOTE: Unfortunately we can't use the iterator as a hint due
		// as we would have to keep the read lock to keep the iterator valid
		ReadWriteLock::WriteGuard writeGuard( suppressedCategoriesLock_ );
		return messagePriority >= suppressedCategories_[ categoryAsString ];
	}
}


/**
 *	This method sets up the global watchers. It should only be called on the
 *	singleton instance of DebugFilter.
 */
void DebugFilter::addWatchers()
{
#if ENABLE_WATCHERS

	MapWatcher< DebugFilter::CategoriesMap > * pCategorySuppressionWatcher =
		new MapWatcher< DebugFilter::CategoriesMap >( suppressedCategories_ );

	pCategorySuppressionWatcher->setComment( "Adjust the minimum priority "
		"level for output of a message in each category" );

	DataWatcher< DebugMessagePriority > * pChildWatcher =
		new DataWatcher< DebugMessagePriority >( *(DebugMessagePriority*)NULL,
			Watcher::WT_READ_WRITE );

	pChildWatcher->setComment( "0:TRACE; 1:DEBUG; 2:INFO; 3:NOTICE; 4:WARNING; "
		"5:ERROR; 6:CRITICAL; 7:HACK; 9: ASSET; 10: NOTHING" );

	pCategorySuppressionWatcher->addChild( "*", pChildWatcher );

	WatcherPtr pReadWriteWatcher = new ReadWriteLockWatcher( 
		pCategorySuppressionWatcher, suppressedCategoriesLock_ );

	MF_VERIFY( Watcher::rootWatcher().addChild( "logger/categorySupression",
		pReadWriteWatcher ) );
		
	MF_WATCH( "logger/shouldOutputErrorBackTrace",
		DebugFilter::s_shouldOutputErrorBackTrace_ );

#endif /* ENABLE_WATCHERS */
}


/**
 *	This method removes the watchers set up by addWatchers. It should only be
 *	called on the singleton instance of DebugFilter.
 */
void DebugFilter::removeWatchers() const
{
#if ENABLE_WATCHERS

	// Don't recreate the rootWatcher if it's gone.
	if (!Watcher::hasRootWatcher())
	{
		return;
	}

	MF_VERIFY( Watcher::rootWatcher().removeChild(
		"logger/categorySupression" ) );

	MF_VERIFY( Watcher::rootWatcher().removeChild(
		"logger/shouldOutputErrorBackTrace" ) );

#endif /* ENABLE_WATCHERS */
}


/**
 *
 */
/*static*/ void DebugFilter::setAlwaysHandleMessage( bool enable )
{
#if defined( _WIN32 )
	DebugFilter::alwaysHandleMessage_ = enable;
#endif // defined( _WIN32 )
}


/**
 *
 */
/*static*/ bool DebugFilter::alwaysHandleMessage()
{
#if defined( _WIN32 )
	return DebugFilter::alwaysHandleMessage_;
#else
	return true;
#endif // defined( _WIN32 )
}


BW_END_NAMESPACE

#undef BW_PLATFORM_SUPPORTS_CONSOLE 

// debug_filter.cpp
