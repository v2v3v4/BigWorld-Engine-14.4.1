#ifndef QUERY_RANGE_HPP
#define QUERY_RANGE_HPP

#include "cstdmf/smartpointer.hpp"

BW_BEGIN_NAMESPACE

class QueryRange;
typedef SmartPointer< QueryRange > QueryRangePtr;

BW_END_NAMESPACE


#include "log_entry_address_reader.hpp"
#include "log_time.hpp"
#include "query_params.hpp"
#include "query_range_iterator.hpp"
#include "user_log_reader.hpp"
#include "log_entry.hpp"


BW_BEGIN_NAMESPACE

/**
 * An iterator over a specified range of a user's log.
 */
class QueryRange : public SafeReferenceCount
{
public:
	typedef QueryRangeIterator iterator;

	//QueryRange( UserLog &userLog, QueryParams &params );
	QueryRange( QueryParamsPtr pParams, UserLogReaderPtr pUserLog );

	const SearchDirection & getDirection() const { return direction_; }

	bool getNextEntry( LogEntry &entry );

	BinaryIStream* getArgStream();
	bool getMetaDataFromEntry( const LogEntry & entry, BW::string & result );

	void resume();
	bool seek( int segmentNum, int entryNum, int metaOffset,
				int postIncrement = 0 );

	// This is exposed to be used by the QueryRangeIterator.
	const UserSegmentReader *getUserSegmentFromIndex( int index ) const;
	int getUserLogNumSegments() const;

	void updateArgs( int segmentNum, int argsOffset );

	iterator iter_curr() const;
	iterator iter_end() const;

	bool areBoundsGood() const;

	int getNumEntriesVisited() const;
	int getTotalEntries() const;

	// Move the curr_ iterator
	void step_back();
	void step_forward();

	BW::string asString() const;

private:
	iterator findSentinel( int direction );
	iterator findLeftSentinel();
	iterator findRightSentinel();
	int findEntryInSegment( const UserSegmentReader *userSegment,
			SearchDirection direction );

	UserLogReaderPtr pUserLog_;

	LogTime startTime_;
	LogTime endTime_;

	LogEntryAddressReader startAddress_;
	LogEntryAddressReader endAddress_;

	SearchDirection direction_;

	iterator begin_;
	iterator curr_;
	iterator end_;
	iterator args_;
};

BW_END_NAMESPACE

#endif // QUERY_RANGE_HPP
