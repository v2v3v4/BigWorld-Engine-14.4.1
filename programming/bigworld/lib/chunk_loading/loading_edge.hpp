#ifndef LOADING_EDGE_HPP
#define LOADING_EDGE_HPP

#include "loading_column.hpp"
#include "math/rectt.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class stores information about how much is loaded along one
 *	edge of the loading rectangle. It expands along the edge of the
 *	rectangle in both positive and negative directions.
 */
class LoadingEdge
{
public:
	void init( int edgeType, int edgePos, int chunkPos, int destPos )
	{
		edgeType_ = edgeType;
		currPos_ = edgePos;
		destPos_ = destPos;

		high_.pos_ = chunkPos;
		low_.pos_ = chunkPos - 1;
	}

	bool isFullyLoaded() const
	{
		return destPos_ == currPos_;
	}

	bool tick( EdgeGeometryMapping * pMapping,
			const LoadingEdge & lineLo, const LoadingEdge & lineHi,
			const LoadingEdge & lineOth,
			bool & rAnyColumnsLoaded );

	void tickCondemned( EdgeGeometryMapping * pMapping );

	// Is it the left or bottom edge?
	bool isLow() const	{ return edgeType_ < 2; }

	// Is it the left or right edge?
	bool isVertical() const	{ return !(edgeType_ & 1); }

	bool isLoading() const
	{
		return low_.isLoading() || high_.isLoading();
	}

	bool clipDestTo( int pos, int antiHyst );

	void prepareNewlyLoadedChunksForDelete( EdgeGeometryMapping * pMapping )
	{
		low_.prepareNewlyLoadedChunksForDelete( pMapping );
		high_.prepareNewlyLoadedChunksForDelete( pMapping );
	}

	int currPos() const	{ return currPos_; }
	int destPos() const	{ return destPos_; }

private:
	int	currPos_;		// where does the done rectangle stop
	int destPos_;		// what is the limit of the destination rect

	LoadingColumn low_; // Loading strip in negative direction
	LoadingColumn high_; // Loading strip in positive direction

	int edgeType_;
};

BW_END_NAMESPACE

#endif // LOADING_EDGE_HPP
