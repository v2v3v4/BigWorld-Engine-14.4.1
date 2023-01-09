#ifndef DEBUG_FILTER_HPP
#define DEBUG_FILTER_HPP

#include "stdmf.hpp"

#include "bw_map.hpp"
// TODO: remove this
#include "bw_string.hpp"
#include "bw_string_ref.hpp"
#include "bw_vector.hpp"
#include "concurrency.hpp"
#include "debug_message_callbacks.hpp"
#include "debug_message_priority.hpp"
#include "smartpointer.hpp"

#include <algorithm>

#include <stdio.h>

BW_BEGIN_NAMESPACE

/**
 *	This class is used to help filter debug messages.
 */
class DebugFilter
{
public:

	DebugFilter();
	~DebugFilter();

	CSTDMF_DLL static DebugFilter & instance();

	CSTDMF_DLL static void fini();

	CSTDMF_DLL static bool shouldAccept( DebugMessagePriority messagePriority,
		const char * pCategoryName = NULL );
	bool shouldAcceptCategory( const char * pCategoryName,
		DebugMessagePriority messagePriority );

	static bool shouldWriteTimePrefix();
	static void shouldWriteTimePrefix( bool value );

	static bool shouldWriteToConsole();
	CSTDMF_DLL static void shouldWriteToConsole( bool value );

	static bool shouldOutputErrorBackTrace();
	static void shouldOutputErrorBackTrace( bool value );

	static bool isCreated()
	{
		return s_instance_ != NULL;
	}


	CSTDMF_DLL DebugMessagePriority filterThreshold() const;
	CSTDMF_DLL void filterThreshold( DebugMessagePriority value );


	CSTDMF_DLL bool hasDevelopmentAssertions() const;
	CSTDMF_DLL void hasDevelopmentAssertions( bool value );


	typedef BW::vector< CriticalMessageCallback * > CriticalCallbacks;

	CSTDMF_DLL void addCriticalCallback( CriticalMessageCallback * pCallback );
	CSTDMF_DLL void deleteCriticalCallback( CriticalMessageCallback * pCallback );
	void handleCriticalMessage( const char * message );

	typedef BW::vector< DebugMessageCallback * > DebugCallbacks;

	CSTDMF_DLL void addMessageCallback( DebugMessageCallback * pCallback,
		bool ignoreIfMsgLoggingIsDisabled = true );
	CSTDMF_DLL void deleteMessageCallback( DebugMessageCallback * pCallback );

	bool handleMessage(
		DebugMessagePriority messagePriority, const char * pCategory,
		DebugMessageSource messageSource, const LogMetaData & metaData,
		const char * pFormat, va_list argPtr );

	typedef BW::map< BW::string, DebugMessagePriority > CategoriesMap;

	CSTDMF_DLL void setCategoryFilter( const BW::string & categoryName,
		DebugMessagePriority minAcceptedPriorityLevel );

	static void consoleOutputFile( FILE * value );
	CSTDMF_DLL static FILE * consoleOutputFile();

	static void setAlwaysHandleMessage( bool enable );
	static bool alwaysHandleMessage();

private:
	void addWatchers();
	void removeWatchers() const;

	DebugMessagePriority filterThreshold_;
	bool hasDevelopmentAssertions_;

	CriticalCallbacks pCriticalCallbacks_;
	RecursiveMutex criticalCallbackMutex_;
	DebugCallbacks pMessageCallbacks_;
	RecursiveMutex messageCallbackMutex_;

	// TODO: Improve this from using a ReadWriteLock
	ReadWriteLock suppressedCategoriesLock_;
	CategoriesMap suppressedCategories_;

	static DebugFilter * s_instance_;

	static bool s_shouldWriteTimePrefix_;
	static bool s_shouldWriteToConsole_;
	static bool s_shouldOutputErrorBackTrace_;

	class FileInitWrapper
	{
	public:
		FileInitWrapper( FILE * f ) :
			f_( f ) {};
		FILE * get() const { return f_; };
		void set( FILE * f ) { f_ = f; };
	private:
		FILE * f_;
	};

	static FileInitWrapper s_consoleOutputFile_;

#if defined( _WIN32 )
	static bool alwaysHandleMessage_;
#endif
};

#ifdef CODE_INLINE
#include "debug_filter.ipp"
#endif

BW_END_NAMESPACE

#endif // DEBUG_FILTER_HPP
