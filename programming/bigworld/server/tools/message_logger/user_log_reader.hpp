#ifndef USER_LOG_READER_HPP
#define USER_LOG_READER_HPP

#include "cstdmf/smartpointer.hpp"

#include "cstdmf/bw_map.hpp"

BW_BEGIN_NAMESPACE

class UserLogReader;
typedef SmartPointer< UserLogReader > UserLogReaderPtr;
typedef BW::map< uint16, UserLogReaderPtr > UserLogs;

BW_END_NAMESPACE


#include "user_log.hpp"
#include "query_range.hpp"
#include "user_segment_reader.hpp"

#include "cstdmf/bw_vector.hpp"


BW_BEGIN_NAMESPACE

class UserSegmentVisitor
{
public:
	virtual bool onSegment( const UserSegmentReader * pSegment ) = 0;
};


class UserLogReader : public UserLog
{
public:
	UserLogReader( uint16 uid, const BW::string &username );

	bool init( const BW::string rootPath );

	// Log retrieval
	bool getEntryAndSegment( const LogEntryAddress & logEntryAddress,
		LogEntry & result, UserSegmentReader * & pSegment,
		bool shouldEmitWarnings = true );
	bool getEntryAndQueryRange( const LogEntryAddress & logEntryAddress,
		LogEntry & result, QueryRangePtr pRange );

	bool getFirstLogEntryFromStartOfLogs( LogEntry & result );
	bool getLastLogEntryFromEndOfLogs( LogEntry & result );
	bool getEntry( const LogEntryAddress & logEntryAddress, LogEntry & result );

	// This method is public to allow QueryRange to be able to
	// retrieve segments.
	// Candidate for cleanup. A longer term improvement would separate the
	// knowledge in QueryRange of how UserLog actually stores the UserSegments.
	int getSegmentIndexFromSuffix( const char *suffix ) const;
	int getNumSegments() const;
	const UserSegmentReader *getUserSegment( int segmentIndex ) const;

	const LoggingComponent *getComponentByID(
		MessageLogger::UserComponentID userComponentID ) const;

	bool reloadFiles();

	bool getUserComponents( UserComponentVisitor &visitor ) const;

	bool visitAllSegmentsWith( UserSegmentVisitor &visitor ) const;

private:
	// UserLogReaders should always be passed around as SmartPointers
	//~UserLogReader();

	bool loadSegments();
};

BW_END_NAMESPACE

#endif // USER_LOG_READER_HPP
