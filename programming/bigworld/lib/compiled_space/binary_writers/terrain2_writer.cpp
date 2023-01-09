#include "pch.hpp"

#include "terrain2_writer.hpp"
#include "string_table_writer.hpp"
#include "binary_format_writer.hpp"
#include "octree_writer.hpp"
#include "chunk_converter.hpp"

#include "resmgr/quick_file_writer.hpp"

#include "chunk/chunk_grid_size.hpp"
#include "cstdmf/lookup_table.hpp"

#include "math/loose_octree.hpp"
#include "math/boundbox.hpp"

#include "math/boundbox.hpp"
#include "math/loose_octree.hpp"
#include "math/math_extra.hpp"
#include "../terrain2_types.hpp"

#include "terrain/terrain2/terrain_height_map2.hpp"

namespace BW {
namespace CompiledSpace {

class StringTableWriter;

namespace {

AABB terrainBlockBounds( const float blockSize,
	const Matrix & chunkTransform,
	const DataSectionPtr & pCDataSection )
{
	const DataSectionPtr pHeightsSection =
		pCDataSection->openSection(StringRef("terrain2/heights"));
	Terrain::TerrainHeightMap2 terrainHMap(blockSize);
	terrainHMap.load( pHeightsSection );
	const float minHeight = terrainHMap.minHeight();
	float maxHeight = terrainHMap.maxHeight();
	MF_ASSERT( maxHeight >= minHeight );
	if (minHeight == maxHeight)
	{
		// make sure the bounding box has some volume
		maxHeight += 1.0f;
	}
	const Vector3 bbMin( 0, minHeight, 0 ),
	              bbMax( blockSize, maxHeight, blockSize );
	AABB bb( bbMin, bbMax );
	bb.transformBy( chunkTransform );
	return bb;
}

} // End unnamed namespace


// ----------------------------------------------------------------------------
Terrain2Writer::Terrain2Writer() : supportedVersion_( false )
{
	header_.blockSize_ = DEFAULT_GRID_RESOLUTION;
	// Turn the min/max inside out by default so no blocks will
	// be considered valid.
	header_.gridMaxX_ = 0;
	header_.gridMinX_ = 1;
	header_.gridMaxZ_ = 0;
	header_.gridMinZ_ = 1;
}


// ----------------------------------------------------------------------------
Terrain2Writer::~Terrain2Writer()
{
	
}


// ----------------------------------------------------------------------------
bool Terrain2Writer::write( BinaryFormatWriter& writer )
{
	if (!supportedVersion_ || blocks_.empty())
	{
		return false;
	}

	DynamicLooseOctree terrainOctree;
	NodeContentsTable sceneOctreeContents;
	generateOctree(terrainOctree, sceneOctreeContents);

	BinaryFormatWriter::Stream* stream =
		writer.appendSection(
		Terrain2Types::FORMAT_MAGIC,
		Terrain2Types::FORMAT_VERSION );
	MF_ASSERT( stream != NULL );
	
	stream->write( header_ );
	stream->write( blocks_ );
	stream->write( blockGridLookup_ );

	// Output the octree
	if (!writeOctree( stream, terrainOctree, sceneOctreeContents ))
	{
		return false;
	}

	return true;
}


// ----------------------------------------------------------------------------
bool Terrain2Writer::initialize( const DataSectionPtr& pSpaceSettings,
	const CommandLine& commandLine )
{
	supportedVersion_ = pSpaceSettings->readInt( "terrain/version", 0 ) == 200;
	header_.blockSize_ = pSpaceSettings->readFloat(
		"chunkSize", DEFAULT_GRID_RESOLUTION );
	return true;
}


// ----------------------------------------------------------------------------
void Terrain2Writer::convertTerrain( const ConversionContext& ctx,
	const DataSectionPtr& pItemDS, const BW::string& uid )
{
	addFromChunk( ctx.gridX, ctx.gridZ, pItemDS, strings(),
		ctx.pCDataDS, ctx.chunkTransform );
}


// ----------------------------------------------------------------------------
void Terrain2Writer::addFromChunk( int gridX, int gridZ,
	const DataSectionPtr& pDS, StringTableWriter& stringTable,
	const DataSectionPtr & pCDataSection, const Matrix & chunkTransform )
{
	if (!supportedVersion_)
	{
		return;
	}

	BW::string resourceID = pDS->readString( "resource" );
	if (resourceID.empty())
	{
		return;
	}

	Terrain2Types::BlockData data;
	data.resourceID_ = stringTable.addString( resourceID );
	data.x_ = gridX;
	data.z_ = gridZ;

	blocks_.push_back( data );
	
	const AABB blockBB = terrainBlockBounds( header_.blockSize_,
		chunkTransform, pCDataSection );
	blockBounds_.push_back( blockBB );
	terrainBounds_.addBounds( blockBB );
}


// ----------------------------------------------------------------------------
void Terrain2Writer::postProcess()
{
	// Calculate the grid that the terrain blocks reside in.
	Vector3 minBounds = terrainBounds_.minBounds();
	Vector3 maxBounds = terrainBounds_.maxBounds();
	float halfBlockSize = header_.blockSize_ / 2.0f;

	int32 minX = Terrain2Types::pointToGrid( 
		minBounds.x + halfBlockSize, header_.blockSize_ );
	int32 minZ = Terrain2Types::pointToGrid( 
		minBounds.z + halfBlockSize, header_.blockSize_ );
	int32 maxX = Terrain2Types::pointToGrid( 
		maxBounds.x - halfBlockSize, header_.blockSize_ );
	int32 maxZ = Terrain2Types::pointToGrid( 
		maxBounds.z - halfBlockSize, header_.blockSize_ );

	header_.gridMaxX_ = maxX;
	header_.gridMinX_ = minX;
	header_.gridMaxZ_ = maxZ;
	header_.gridMinZ_ = minZ;

	int32 width = maxX - minX + 1;
	int32 height = maxZ - minZ + 1;
	uint32 numGridEntries = width * height;

	// Now rearrange all the terrain blocks into their appropriate locations
	// in the terrain array
	blockGridLookup_.resize( numGridEntries, Terrain2Types::INVALID_BLOCK_REFERENCE );
	
	// Iterate over each block and place it within the apporpriate block
	// grid lookup location.
	for (uint32 index = 0; index < blocks_.size(); ++index)
	{
		const AABB & bounds = blockBounds_[index];
		Vector3 centre = bounds.centre();
		uint32 gridIndex = blockIndexFromPoint( centre, header_ );

		MF_ASSERT( gridIndex < numGridEntries );
		MF_ASSERT( blockGridLookup_[gridIndex] == 
			Terrain2Types::INVALID_BLOCK_REFERENCE );
		blockGridLookup_[gridIndex] = index;
	}

	TRACE_MSG( "%d terrain blocks.\n", numBlocks() );
}


// ----------------------------------------------------------------------------
size_t Terrain2Writer::numBlocks() const
{
	return blocks_.size();
}


// ----------------------------------------------------------------------------
AABB Terrain2Writer::boundBox() const
{
	return terrainBounds_;
}


// ----------------------------------------------------------------------------
void Terrain2Writer::generateOctree( DynamicLooseOctree & terrainOctree,
	NodeContentsTable & sceneOctreeContents )
{
	const size_t numberOfBlocks = blocks_.size();
	const size_t numberOfBlockBounds = blockBounds_.size();
	MF_ASSERT( numberOfBlockBounds == numberOfBlocks );

	if (numberOfBlocks == 0)
	{
		return;
	}

	// We can calculate an ideal depth for the octree by finding the
	// conservative power of two that fits the number of blocks along the
	// dimension with the most blocks.
	const float invBlockSize = 1.f / header_.blockSize_;
	const float numBlocksInWidth = terrainBounds_.width() * invBlockSize;
	const float numBlocksInHeight = terrainBounds_.height() * invBlockSize;
	const float numBlocksInDepth = terrainBounds_.depth() * invBlockSize;
	float maxBlocksInADimension = max( numBlocksInWidth,
		max( numBlocksInHeight, numBlocksInDepth ) );
	uint32 maxOctreeDepth = log2ceil( (uint32)maxBlocksInADimension );

	const uint32 MAX_TREE_DEPTH = 8;
	maxOctreeDepth = Math::clamp( 1u, maxOctreeDepth, MAX_TREE_DEPTH );

	terrainOctree.initialise(terrainBounds_.centre(), terrainBounds_.width(),
		maxOctreeDepth );

	for (size_t i = 0; i < numberOfBlocks; ++i)
	{
		DynamicLooseOctree::NodeIndex nodeId =
			terrainOctree.insert(blockBounds_[i]);
		DynamicLooseOctree::NodeDataReference dataRef = 
			terrainOctree.dataOnLeaf( nodeId );

		NodeContents & contents = sceneOctreeContents[dataRef];
		contents.push_back( (DynamicLooseOctree::NodeDataReference)i );
	}

	terrainOctree.updateHeirarchy();

	LooseOctreeHelpers::prepareStaticTreeStorage( terrainOctree );
}

} // namespace CompiledSpace
} // namespace BW

