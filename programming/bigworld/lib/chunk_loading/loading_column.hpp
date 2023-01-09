#ifndef LOADING_COLUMN_HPP
#define LOADING_COLUMN_HPP

#include "cstdmf/stdmf.hpp"

#include "cstdmf/bw_list.hpp"


BW_BEGIN_NAMESPACE

class Chunk;
class ChunkSpace;
class EdgeGeometryMapping;

enum LoadingColumnState
{
	UNLOADED,
	LOADING_OUTSIDE,
	LOADING_INSIDE,
	LOADED
};


/**
 *	This class tracks the loading of one column. That is, one outdoor chunk and
 *	the overlapping indoor chunks.
 */
class LoadingColumn
{
public:
	LoadingColumn();
	~LoadingColumn();

	bool stepLoad( EdgeGeometryMapping * pMapping, int perpPos,
			bool isVerticalEdge, bool & rAnyColumnsLoaded );
	bool stepUnload( EdgeGeometryMapping * pMapping, int perpPos,
			bool isVerticalEdge );

	void prepareNewlyLoadedChunksForDelete( EdgeGeometryMapping * pMapping );

	bool isLoading() const
	{
		return (state_ == LOADING_INSIDE) || (state_ == LOADING_OUTSIDE);
	}

	// The position along the edge of the loading chunk/column.
	int		pos_;

	Chunk * pOutsideChunk_;
	LoadingColumnState state_;

private:
	// Loading
	void loadOutsideChunk( EdgeGeometryMapping * pMapping,
		int edgePos, bool isVerticalEdge );
	void loadOverlappers();
	bool areOverlappersLoaded() const;

	void trimClientOnlyBoundaries( EdgeGeometryMapping * pMapping );
	void trimClientOnlyBoundaries( Chunk * pChunk,
		EdgeGeometryMapping * pMapping );

	void bindOutsideChunk( EdgeGeometryMapping * pMapping );
	void bindOverlappers( EdgeGeometryMapping * pMapping );

	// Unloading
	void unloadOverlappers( EdgeGeometryMapping * pMapping );
	void unloadOutsideChunk( EdgeGeometryMapping * pMapping );
	void cancelLoadingOverlappers( EdgeGeometryMapping * pMapping );
	void cancelLoadingOutside( EdgeGeometryMapping * pMapping );
};

BW_END_NAMESPACE

#endif // LOADING_COLUMN_HPP
