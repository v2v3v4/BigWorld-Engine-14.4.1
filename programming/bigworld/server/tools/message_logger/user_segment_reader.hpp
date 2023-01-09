#ifndef USER_SEGMENT_READER_HPP
#define USER_SEGMENT_READER_HPP

#include "user_segment.hpp"

#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_vector.hpp"


BW_BEGIN_NAMESPACE

class PyQueryResult;
class LogStringInterpolator;

class UserSegmentReader : public UserSegment
{
public:
	UserSegmentReader( const BW::string userLogPath, const char *suffix );

	bool init();

	static int filter( const struct dirent *ent );

	bool isDirty() const;

	int findEntryNumber( LogTime &time, SearchDirection direction );

	bool seek( int n );

	const LogTime & getStartLogTime() const;
	const LogTime & getEndLogTime() const;


	bool interpolateMessage( const LogEntry &entry,
			const LogStringInterpolator *pHandler, BW::string &result );
	bool metadata( const LogEntry & entry, BW::string & result );

	// Candidate for cleanup. QueryRange currently requires this.
	FileStream * getArgStream();

	int getEntriesLength() const;
	int getArgsLength() const;
	int getMetadataLength() const;

private:
	bool isSegmentOK_;
};

BW_END_NAMESPACE

#endif // USER_SEGMENT_READER_HPP
