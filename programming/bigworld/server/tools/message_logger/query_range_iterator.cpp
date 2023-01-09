#include "script/first_include.hpp"

#include "query_range_iterator.hpp"

#include "query_range.hpp"
#include "log_entry_address.hpp"

#include "cstdmf/debug.hpp"


BW_BEGIN_NAMESPACE

/**
 * This iterator class is intended to remove some of the annoying (and error
 * prone) handling of QueryRange::direction_ from this module's code.
 * As far as this class is concerned, the positive direction is always towards
 * the end of the search, whether the search is running forwards or backwards.
 */
QueryRangeIterator::QueryRangeIterator( const QueryRange *queryRange,
				int segmentNum, int entryNum, int metaOffset ) :
	pQueryRange_( queryRange ),
	segmentNum_( segmentNum ),
	entryNum_( entryNum ),
	metaOffset_( metaOffset )
{ }


/**
 * 'Goodness' is tied into the segmentNum_ field. The entryNum_ field is
 * used to determine that an iterator is out of range to the left or right.
 */
bool QueryRangeIterator::isGood() const
{
	return ((segmentNum_ != -1) && (entryNum_ != -1));
}


bool QueryRangeIterator::operator<( const QueryRangeIterator &other ) const
{
	return (*this) - other < 0;
}


bool QueryRangeIterator::operator<=( const QueryRangeIterator &other ) const
{
	return (*this < other) || (*this == other);
}


bool QueryRangeIterator::operator==( const QueryRangeIterator &other ) const
{
	if (pQueryRange_ != other.pQueryRange_)
	{
		ERROR_MSG( "QueryRangeIterator::operator<:"
			"Trying to compare iterators from two different ranges!\n" );
		return false;
	}

	return segmentNum_ == other.segmentNum_ &&
		entryNum_ == other.entryNum_ &&
		metaOffset_ == other.metaOffset_;
}


QueryRangeIterator& QueryRangeIterator::operator++()
{
	this->step( pQueryRange_->getDirection() );
	return *this;
}


QueryRangeIterator& QueryRangeIterator::operator--()
{
	this->step( (SearchDirection)(-pQueryRange_->getDirection()) );
	return *this;
}


/**
 *  Helper method for operator++ and operator--.
 */
void QueryRangeIterator::step( SearchDirection direction )
{
	SearchDirection d = direction;

	// If we've got a metaoffset and it's in the opposite direction, negate it
	if (metaOffset_ == -d)
	{
		metaOffset_ = 0;
		return;
	}

	entryNum_ += d;

	// If we've gone off the end of a segment, try to select the next one
	if (entryNum_ < 0 || entryNum_ >= this->getSegment()->getNumEntries() )
	{
		segmentNum_ += d;

		// If we go off the end of the log, revert fields
		if (segmentNum_ < 0 ||
			segmentNum_ >= pQueryRange_->getUserLogNumSegments())
		{
			entryNum_ -= d;
			segmentNum_ -= d;

			// If we've hit the end of the search, set metaOffset in case we
			// resume() at some later time.
			if (d == pQueryRange_->getDirection() )
			{
				metaOffset_ = d;
				return;
			}

			// Resuming a query never alters the beginning of the range, so
			// there's not much point doing anything with metaOffset here.
			else
			{
				return;
			}
		}

		// Not off the end of the log, select next segment
		else
		{
			if (d == QUERY_FORWARDS)
			{
				entryNum_ = 0;
			}
			else
			{
				// The segment number was changed above, get the new segment
				entryNum_ = this->getSegment()->getNumEntries() - 1;
			}
		}
	}

	metaOffset_ = 0;
}


/**
 *  This returns the offset (in entries) between this iterator and another
 *  iterator.
 */
int QueryRangeIterator::operator-( const QueryRangeIterator &other ) const
{
	SearchDirection d = pQueryRange_->getDirection();
	if (segmentNum_ == other.segmentNum_)
	{
		return d * (entryNum_ - other.entryNum_	+
			metaOffset_ - other.metaOffset_);
	}

	int count;
	// Count first and last segments
	if (segmentNum_ > other.segmentNum_)
	{
		const UserSegmentReader *pSegment = other.getSegment();
		count = entryNum_ + pSegment->getNumEntries() - other.entryNum_;
	}
	else
	{
		const UserSegmentReader *pSegment = this->getSegment();
		count = pSegment->getNumEntries() - entryNum_ + other.entryNum_;
	}

	// Intervening segments
	if (d == QUERY_FORWARDS)
	{
		int i = other.segmentNum_ + 1;

		while (i < segmentNum_)
		{
			const UserSegmentReader *pSegment =
					 pQueryRange_->getUserSegmentFromIndex( i );
			count += pSegment->getNumEntries();

			++i;
		}
	}
	else
	{
		int i = other.segmentNum_ - 1;
		while (i > segmentNum_)
		{
			const UserSegmentReader *pSegment =
					 pQueryRange_->getUserSegmentFromIndex( i );
			count += pSegment->getNumEntries();

			--i;
		}

	}

	// Flip the count if this iterator precedes the other iterator
	if (d * segmentNum_ < d * other.segmentNum_)
	{
		count = -count;
	}

	return count + metaOffset_ - other.metaOffset_;
}


const UserSegmentReader * QueryRangeIterator::getSegment() const
{
	return pQueryRange_->getUserSegmentFromIndex( segmentNum_ );
}


int QueryRangeIterator::getSegmentNumber() const
{
	return segmentNum_;
}


int QueryRangeIterator::getEntryNumber() const
{
	return entryNum_;
}


int QueryRangeIterator::getMetaOffset() const
{
	return metaOffset_;
}


int QueryRangeIterator::getArgsOffset() const
{
	return argsOffset_;
}


LogEntryAddress QueryRangeIterator::getAddress() const
{
	const UserSegmentReader *pSegment = this->getSegment();
	return LogEntryAddress( pSegment->getSuffix(), entryNum_ );
}


/**
 * Returns the iterator details as a string for debugging and error reporting.
 */
BW::string QueryRangeIterator::asString() const
{
	char buf[ 32 ];
	bw_snprintf( buf, sizeof( buf ), "%d:%d:%d",
		segmentNum_, entryNum_, metaOffset_ );
	return BW::string( buf );
}

BW_END_NAMESPACE

// query_range_iterator.cpp
