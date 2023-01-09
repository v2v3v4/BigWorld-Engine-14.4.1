#ifndef TERRAIN_VERTEX_LOD_CONSTS_HPP
#define TERRAIN_VERTEX_LOD_CONSTS_HPP
#include "terrain_index_buffer.hpp"


BW_BEGIN_NAMESPACE

namespace Terrain
{
struct TerrainVertexLodConsts
{
	// The size of the block we are calculated for
	float blockSize;

	// The last lod is set to be really far away, Not FLT_MAX as shaders may 
	// have different max limits to regular floats
	float LAST_LOD_DIST;

	// The minimum distance from the corner of a block to the first neighbour 
	// sub-block that can influence the degenerates
	float minNeighbourDistance;

	// The distance from a corner of the block to the centre of the block
	float blockCentreDistance;

	// offsets to blocks-neighbours
	Vector3 blockNegativeX;
	Vector3 blockPositiveX;
	Vector3 blockNegativeZ;
	Vector3 blockPositiveZ;

	// 4 sub-blocks per block
	Vector3 subBlockOffset0;
	Vector3 subBlockOffset1;
	Vector3 subBlockOffset2;
	Vector3 subBlockOffset3;

	// Positions and corresponding masks
	struct PositionsAndMasks
	{
		uint32	sbIndex;	// this sub-block we're testing.
		uint8	mask;		// direction from this to neighbour sub-block
		Vector3 sbOffset;	// world position of the neighbour sub-block.
	};

	PositionsAndMasks testData[8];

	TerrainVertexLodConsts( float size ) :
		blockSize( size ),
		LAST_LOD_DIST( 1000000.f ),
		minNeighbourDistance( sqrtf(blockSize*blockSize + blockSize*blockSize) ),
		blockCentreDistance( minNeighbourDistance / 2.f ),
		blockNegativeX( Vector3( -blockSize, 0.f, 0.f ) ),
		blockPositiveX( Vector3( blockSize, 0.f, 0.f ) ),
		blockNegativeZ( Vector3( 0.f, 0.f, -blockSize ) ),
		blockPositiveZ( Vector3( 0.f, 0.f, blockSize ) ),
		subBlockOffset0( Vector3( 0.f, 0.f, 0.f ) ),
		subBlockOffset1( Vector3( blockSize / 2.f, 0.f, 0.f ) ),
		subBlockOffset2( Vector3( 0.f, 0.f, blockSize / 2.f ) ),
		subBlockOffset3( Vector3( blockSize / 2.f, 0.f, blockSize / 2.f) )
	{

		testData[0].sbIndex = 2;
		testData[0].mask = TerrainIndexBuffer::DIRECTION_NEGATIVEX;
		testData[0].sbOffset = blockNegativeX + subBlockOffset3;

		testData[1].sbIndex = 0;
		testData[1].mask = TerrainIndexBuffer::DIRECTION_NEGATIVEX;
		testData[1].sbOffset = blockNegativeX + subBlockOffset1;

		testData[2].sbIndex = 0;
		testData[2].mask = TerrainIndexBuffer::DIRECTION_NEGATIVEZ;
		testData[2].sbOffset = blockNegativeZ + subBlockOffset2;

		testData[3].sbIndex = 1;
		testData[3].mask = TerrainIndexBuffer::DIRECTION_NEGATIVEZ;
		testData[3].sbOffset = blockNegativeZ + subBlockOffset3;

		testData[4].sbIndex = 1;
		testData[4].mask = TerrainIndexBuffer::DIRECTION_POSITIVEX;
		testData[4].sbOffset = blockPositiveX + subBlockOffset0;

		testData[5].sbIndex = 3;
		testData[5].mask = TerrainIndexBuffer::DIRECTION_POSITIVEX;
		testData[5].sbOffset = blockPositiveX + subBlockOffset2;

		testData[6].sbIndex = 3;
		testData[6].mask = TerrainIndexBuffer::DIRECTION_POSITIVEZ;
		testData[6].sbOffset = blockPositiveZ + subBlockOffset1;

		testData[7].sbIndex = 2;
		testData[7].mask = TerrainIndexBuffer::DIRECTION_POSITIVEZ;
		testData[7].sbOffset = blockPositiveZ + subBlockOffset0;
	}
};
} // namespace Terrain

BW_END_NAMESPACE

// terrain_vertex_lod_consts.hpp
#endif // TERRAIN_VERTEX_LOD_CONSTS_HPP