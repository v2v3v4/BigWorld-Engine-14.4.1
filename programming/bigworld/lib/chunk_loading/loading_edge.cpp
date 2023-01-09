#include "loading_edge.hpp"

#include "cstdmf/debug.hpp"
#include "cstdmf/profiler.hpp"

DECLARE_DEBUG_COMPONENT( 0 )


BW_BEGIN_NAMESPACE

/**
 *
 */
bool LoadingEdge::tick( EdgeGeometryMapping * pMapping,
		const LoadingEdge & lineLo, const LoadingEdge & lineHi,
		const LoadingEdge & lineOth,
		bool & rAnyColumnsLoaded )
{
	PROFILER_SCOPED( LoadingEdge_tick );
	bool currLoadedAll = true;

	// get this line
	const bool isNegativeDir = this->isLow();
	const int loadMoveDir = isNegativeDir ? -1 : 1; // which dir loads
	const int perpPos = currPos_ - isNegativeDir; // negs hang below line

	// first figure out if we want load or unload
	if (currPos_*loadMoveDir < destPos_*loadMoveDir)
	{
		bool allLoaded = true;
		// we want to load some more
		if (low_.pos_ > (lineLo.currPos_-1))
		{
			allLoaded = false;
			// load cur column
			if (low_.stepLoad( pMapping, perpPos, this->isVertical(),
						rAnyColumnsLoaded ))
			{
				low_.pos_--;
				low_.state_ = UNLOADED;
			}
		}
		else if (low_.pos_ < (lineLo.currPos_-1) ||
			 /*equal and*/ low_.state_ != UNLOADED)
		{
			// we've loaded too far, so unload before this col is forgotten
			allLoaded = false;
			if (low_.stepUnload( pMapping, perpPos, this->isVertical() ))
			{
				if (low_.pos_+1 < high_.pos_)
				{
					low_.pos_++;
					low_.state_ = LOADED;
				}
			}
		}
		if (high_.pos_ < lineHi.currPos_)
		{
			allLoaded = false;
			// load cur column
			if (high_.stepLoad( pMapping, perpPos, this->isVertical(),
						rAnyColumnsLoaded ))
			{
				high_.pos_++;
				high_.state_ = UNLOADED;
			}
		}
		else if (high_.pos_ > lineHi.currPos_ ||
			 /*equal and*/ high_.state_ != UNLOADED)
		{
			// we've loaded too far, so unload before this col is forgotten
			allLoaded = false;
			if (high_.stepUnload( pMapping, perpPos, this->isVertical() ))
			{
				if (low_.pos_+1 < high_.pos_)
				{
					high_.pos_--;
					high_.state_ = LOADED;
				}
			}
		}

		// move the whole line over if we have loaded it all
		// adj lines will automatically take on columns
		if (allLoaded)
		{
			if (lineLo.currPos_ >= lineHi.currPos_)
			{
				// except if we are a zero-length line, then
				// don't go making more work for the adjacent lines
				// unless they are also zero-length
				if (lineOth.currPos_ != currPos_)
				{
					//TRACE_MSG( "EdgeGeometryMapping: Line %d "
					//	"not expanding since it is zero-length\n", i );
					return currLoadedAll;
				}
			}

			currPos_ += loadMoveDir;
			low_.pos_ = (lineHi.currPos_ + lineLo.currPos_)/2 - 1;
			high_.pos_ = low_.pos_ + 1;
			low_.state_ = UNLOADED;
			high_.state_ = UNLOADED;

			//TRACE_MSG( "EdgeGeometryMapping: "
			//	"Line %d expanding to donePerp %d\n", i, currPos_ );
		}
	}
	else if (currPos_*loadMoveDir > destPos_*loadMoveDir)
	{				// (comment the if above to test unloading+loading)
		// we want to unload some more

		bool allUnloaded = false;
		if (low_.pos_+1 >= high_.pos_)
		{
			// if the two columns are side-by-side then we are
			// all done if they are both currently unloaded
			if (low_.state_ == UNLOADED &&
				high_.state_ == UNLOADED)
			{
				allUnloaded = true;
			}
		}
		if (!allUnloaded)
		{
			// unload cur column
			if (low_.stepUnload( pMapping, perpPos, this->isVertical() ))
			{
				if (low_.pos_+1 < high_.pos_)
				{
					low_.pos_++;	// don't get into pos's col
					low_.state_ = LOADED;
				}
			}

			// unload cur column
			if (high_.stepUnload( pMapping, perpPos, this->isVertical() ))
			{
				if (low_.pos_+1 < high_.pos_)
				{
					high_.pos_--;	// don't get into low_'s col
					high_.state_ = LOADED;
				}
			}
		}

		if (allUnloaded)
		{
			// ok, great, we have unloaded the whole line
			currLoadedAll = false;
			// make sure we do not overlap the facing line
			if (currPos_*loadMoveDir <= lineOth.currPos_*loadMoveDir)
			{
				return currLoadedAll;
			}

			// set up columns from lineHi and lineLo
			currPos_ -= loadMoveDir;
			low_.pos_ = lineLo.currPos_;
			high_.pos_ = lineHi.currPos_-1;
			low_.state_ = LOADED;
			high_.state_ = LOADED;
			if (low_.pos_ >  high_.pos_)
			{	// missing both
				low_.pos_--;
				high_.pos_++;
				low_.state_ = UNLOADED;
				high_.state_ = UNLOADED;
			}
			else if (low_.pos_ == high_.pos_)
			{	// missing one
				high_.pos_++; // not strictly necessary, but sane
				high_.state_ = UNLOADED;
			}

			// our adj lines will take care of unloading the corner chunks
			// we've just cut off their lines (by moving our donePerp back),
			// if indeed they are loaded, and regardless of whether the
			// adj lines are loading or unloading themselves.

			//TRACE_MSG( "EdgeGeometryMapping: "
			//	"Line %d contracting to donePerp %d\n", i, currPos_ );
		}
	}

	return currLoadedAll;
}


void LoadingEdge::tickCondemned( EdgeGeometryMapping * pMapping )
{
	const bool isNegativeDir = this->isLow();
	const int perpPos = currPos_ - isNegativeDir; // negs hang below line

	low_.stepUnload( pMapping, perpPos, this->isVertical() );
	high_.stepUnload( pMapping, perpPos, this->isVertical() );
}


bool LoadingEdge::clipDestTo( int pos, int antiHyst )
{
	int low = pos;
	int high = pos;

	if (this->isLow())
	{
		low -= antiHyst;
	}
	else
	{
		high += antiHyst;
	}

	if (destPos_ < low)
	{
		destPos_ = low;
	}
	else if (destPos_ > high)
	{
		destPos_ = high;
	}
	else
	{
		return false;
	}

	return true;
}

BW_END_NAMESPACE

// loading_edge.cpp
