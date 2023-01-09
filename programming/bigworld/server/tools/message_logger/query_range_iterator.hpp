#ifndef QUERY_RANGE_ITERATOR_HPP
#define QUERY_RANGE_ITERATOR_HPP

#include "constants.hpp"

#include "log_entry_address.hpp"


BW_BEGIN_NAMESPACE

class QueryRange;
class UserSegmentReader;

class QueryRangeIterator
{
public:
	QueryRangeIterator( const QueryRange *queryRange,
			int segmentNum = -1, int entryNum = -1, int metaOffset = 0 );

	static QueryRangeIterator error( QueryRange &queryRange )
	{
		return QueryRangeIterator( &queryRange );
	}

	bool isGood() const;

	const UserSegmentReader * getSegment() const;

	int getSegmentNumber() const;
	int getEntryNumber() const;
	int getArgsOffset() const;
	int getMetaOffset() const;

	LogEntryAddress getAddress() const;

	BW::string asString() const;

	bool operator<(  const QueryRangeIterator &other ) const;
	bool operator<=( const QueryRangeIterator &other ) const;
	bool operator==( const QueryRangeIterator &other ) const;
	int operator-(   const QueryRangeIterator &other ) const;
	QueryRangeIterator& operator++();
	QueryRangeIterator& operator--();


private:
	void step( SearchDirection direction );

	// The parent QueryRange that we are iterating over
	const QueryRange *pQueryRange_;

	// Index into the UserLog list of UserSegments
	int segmentNum_;

	// TODO: is there any good reason for this to be a union?
	union
	{
		int entryNum_;
		int argsOffset_;
	};

	int metaOffset_;
};

BW_END_NAMESPACE

#endif // QUERY_RANGE_ITERATOR_HPP
