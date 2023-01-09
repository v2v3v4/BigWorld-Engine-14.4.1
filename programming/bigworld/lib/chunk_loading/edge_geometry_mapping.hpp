#ifndef EDGE_GEOMETRY_MAPPING_HPP
#define EDGE_GEOMETRY_MAPPING_HPP

#include "loading_edge.hpp"

#include "chunk/geometry_mapping.hpp"

#include "math/vector3.hpp"
#include "math/rectt.hpp"


BW_BEGIN_NAMESPACE

class Chunk;
class EdgeGeometryMappings;
class Vector3;


/**
 *	This class handles the loading of one GeometryMapping in a Space.
 *	Chunks on the grids edges are loaded and unloaded as required.
 */
class EdgeGeometryMapping : public GeometryMapping
{
public:
	EdgeGeometryMapping( ChunkSpacePtr pSpace, const Matrix & m,
		const BW::string & path, DataSectionPtr pSettings,
		EdgeGeometryMappings & mappings, const Vector3 * pInitialPoint );
	~EdgeGeometryMapping();

	bool tick( const Vector3 & minB, const Vector3 & maxB, bool unloadOnly );

	bool hasFullyLoaded();
	bool isFullyUnloaded() const;
	float getLoadProgress() const;

	void calcLoadedRect( BW::Rect & loadedRect,
		   bool isFirst, bool isLast ) const;

	bool getLoadableRects( BW::list< BW::Rect > & rects ) const;

	void prepareNewlyLoadedChunksForDelete();

	void bind( Chunk * pChunk );
	void unbindAndUnload( Chunk * pChunk );

	Chunk * findOrAddOutsideChunk( int xGrid, int yGrid );
	Chunk * findOutsideChunk( int xGrid, int yGrid );

	// The following accessors return the bounds of the sub grid loaded
	int minSubGridX() const { return minLGridX(); }
	int maxSubGridX() const { return maxLGridX(); }
	int minSubGridY() const { return minLGridY(); }
	int maxSubGridY() const { return maxLGridY(); }

private:
	void postBoundChunk( Chunk * pChunk );
	void preUnbindChunk( Chunk * pChunk );

   	LoadingEdge edge_[4];	// in order L,B,R,T

	enum EdgeType
	{
		LEFT = 0,
		BOTTOM = 1,
		RIGHT = 2,
		TOP = 3
	};

	// Loaded rect is (left, bottom) to (right, top):
	//  (edge_[0].currPos_, edge_[1].currPos_) to:
	//  (edge_[2].currPos_, edge_[3].currPos_)

	// Destination rect to be loaded is (left, bottom) to (right, top):
	//  (edge_[0].destPos_, edge_[1].destPos_) to:
	//  (edge_[2].destPos_, edge_[3].destPos_)

	bool currLoadedAll_;	// do we think we have it all loaded

	EdgeGeometryMappings & mappings_;
};


BW_END_NAMESPACE

#endif // EDGE_GEOMETRY_MAPPING_HPP
