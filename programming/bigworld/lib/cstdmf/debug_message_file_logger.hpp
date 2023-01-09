#ifndef DEBUG_MESSAGE_FILE_LOGGER_HPP
#define DEBUG_MESSAGE_FILE_LOGGER_HPP

#include "bw_string.hpp"
#include "bw_vector.hpp"
#include "concurrency.hpp"
#include "debug_message_callbacks.hpp"
#include "debug_message_priority.hpp"
#include "debug_message_source.hpp"

BW_BEGIN_NAMESPACE

class StringBuilder;


/**
 *	This class receive debug messages in virtual handleMessage 
 *	and log messages into file.
 */
class DebugMessageFileLogger: 
	public DebugMessageCallback
{

public:
	CSTDMF_DLL DebugMessageFileLogger();
	CSTDMF_DLL virtual ~DebugMessageFileLogger();


	CSTDMF_DLL virtual bool handleMessage(
		DebugMessagePriority messagePriority, const char * pCategory,
		DebugMessageSource messageSource, const LogMetaData & metaData,
		const char * pFormat, va_list argPtr );


	enum Severity
	{
		SERVERITY_TRACE = 1,
		SERVERITY_DEBUG = 2,
		SERVERITY_INFO = 4,
		SERVERITY_NOTICE = 8,
		SERVERITY_WARNING = 16,
		SERVERITY_ERROR = 32,
		SERVERITY_CRITICAL = 64,
		SERVERITY_HACK = 128,
		SERVERITY_ASSET = 256,
		SERVERITY_ALL = SERVERITY_TRACE | SERVERITY_DEBUG | SERVERITY_INFO |
			SERVERITY_NOTICE | SERVERITY_WARNING | SERVERITY_ERROR |
			SERVERITY_CRITICAL | SERVERITY_HACK | SERVERITY_ASSET
	};

	enum Source
	{
		SOURCE_CPP = 1,
		SOURCE_SCRIPT = 2,
		SOURCE_ALL = SOURCE_CPP | SOURCE_SCRIPT
	};

	enum FileOpenMode
	{
		APPEND,
		OVERWRITE
	};

	CSTDMF_DLL static uint16 convertSeverities( 
							const BW::vector<BW::string> & severityArray,
							StringBuilder& errorMsg );
	CSTDMF_DLL static uint8 convertSources( 
							const BW::vector<BW::string> & sourceArray, 
							StringBuilder& errorMsg );
	CSTDMF_DLL static FileOpenMode convertOpenMode( const BW::string& openMode,
							StringBuilder& errorMsg );

	CSTDMF_DLL void config( const BW::string & logFileName = BW::string(),
				uint16 severities = SERVERITY_ALL,
				const BW::string & category = BW::string(),
				uint8 sources = SOURCE_ALL, FileOpenMode openMode =  APPEND,
				bool enableLog = false );

	CSTDMF_DLL void enable( bool enable );
	CSTDMF_DLL bool enable() const;
	CSTDMF_DLL BW::string fileName() const;
	CSTDMF_DLL BW::string openMode() const;
	CSTDMF_DLL BW::string severities() const;
	CSTDMF_DLL BW::string sources() const;
	CSTDMF_DLL BW::string category() const	{ return category_; }

	CSTDMF_DLL virtual void onLogFileChanged( std::ofstream * pNewOutFile ) {};

private:
	mutable SimpleMutex mutex_;
	bool enable_;
	bool hasWrittenBanner_;
	std::ofstream * pOutFile_;

	// Configurations:
	uint16 severities_;
	uint8 sources_;
	BW::string category_;
	FileOpenMode openMode_;

	const static int MAX_LOG_NAME = 8192;
	char fileName_[ MAX_LOG_NAME ];


	bool matchSeverity( DebugMessagePriority priority ) const;
	bool matchSource( DebugMessageSource messageSource ) const;
	bool matchCategory( const char * pCategory ) const;
	void addLogHeader();
};


BW_END_NAMESPACE
#endif// DEBUG_MESSAGE_FILE_LOGGER_HPP
