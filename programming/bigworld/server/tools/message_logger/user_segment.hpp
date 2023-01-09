#ifndef USER_SEGMENT_HPP
#define USER_SEGMENT_HPP

#include "cstdmf/bw_namespace.hpp"

#include "cstdmf/bw_vector.hpp"

BW_BEGIN_NAMESPACE

// This is defined prior to 'user_log.hpp' inclusion, so that UserLog will
// know what UserSegments are.
class UserSegment;
typedef BW::vector< UserSegment * > UserSegments;

BW_END_NAMESPACE


#include "user_log.hpp"
#include "log_time.hpp"
#include "mldb/metadata.hpp"

#include "network/file_stream.hpp"

#include <stdlib.h>
#include <dirent.h>

#include "cstdmf/bw_string.hpp"


BW_BEGIN_NAMESPACE

class LogEntry;
class LoggingComponent;
class UserLog;
class LoggingStringHandler;
class MemoryIStream;

/**
 * A segment of a user's log.  This really means a pair of entries and args
 * files.  NOTE: At the moment, each Segment always has two FileStreams
 * open, which means that two Queries can't be executed at the same time in
 * the same process, and also that if a log has many segments, then the
 * number of open file handles for this process will be excessive.
 */
class UserSegment
{
public:
	UserSegment( const BW::string userLogPath, const char *suffix );
	virtual ~UserSegment();

	const BW::string & getSuffix() const { return suffix_; }

	bool isGood() const { return isGood_; }

	bool readEntry( int n, LogEntry &entry );

	bool updateEntryBounds();

	int getNumEntries() const { return numEntries_; }



protected:
	bool buildSuffixFrom( struct tm & pTime, BW::string & newSuffix ) const;

	BW::string suffix_;

	// File streams representing the currently open args.* / entries.* files.
	FileStream *pEntries_;
	FileStream *pArgs_;

	int numEntries_;
	int argsSize_;

	LogTime start_;
	LogTime end_;

	bool isGood_;

	BW::string userLogPath_;

	MetadataMLDB *pMetadataMLDB_;

private:
	friend class UserSegmentComparator;
};


/**
 * This class is a helper to allow std::sort operations on UserSegments.
 */
class UserSegmentComparator
{
public:
	bool operator() ( const UserSegment *a, const UserSegment *b )
	{
		return a->start_ < b->start_;
	}
};

BW_END_NAMESPACE

#endif // USER_SEGMENT_HPP
