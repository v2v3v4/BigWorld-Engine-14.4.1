#include "pch.hpp"
#include "cstdmf/profiler.hpp"
#include "cstdmf/watcher.hpp"
#include "terrain_vertex_buffer.hpp"
#include "aliased_height_map.hpp"
#include "grid_vertex_buffer.hpp"

#include "cstdmf/watcher.hpp"

BW_BEGIN_NAMESPACE

namespace
{
	static float s_lodGeomorphingFix = 0.007f;
	static bool	 s_terrainVBWatchesAdded = false;
}

namespace Terrain
{

// -----------------------------------------------------------------------------
// Section: TerrainVertexBuffer
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
TerrainVertexBuffer::TerrainVertexBuffer()
{
	if (!s_terrainVBWatchesAdded)
	{
		// We have to add a small value to the height of calculated vertices 
		// for  the next lower LOD to fix the floating point precision error 
		// which causes small bright pixels to appear on the edges of the blocks
		// with different LOD. It raises edges of the block looking towards
		// lower LOD blocks but we can't see that since the camera always looks 
		// from the block with the highest LOD.
		MF_WATCH( "Render/Terrain/Terrain2/Geomorphing Fix", 
			s_lodGeomorphingFix,
			Watcher::WT_READ_WRITE,
			"Fixes floating point precision error slightly raising gemorphed "
			"terrain block with higher LOD." );
		s_terrainVBWatchesAdded = true;
	}
}


/**
 *	Destructor.
 */
TerrainVertexBuffer::~TerrainVertexBuffer()
{
}

/**
 *	This method generates the terrain vertices.
 *	
 *	@param hm            the height map for this LOD.
 *	@param lodHM         the height map for the previous LOD level.
 *	@param resolutionX   the number of x-axis vertices to generate.
 *	@param resolutionZ   the number of z-axis vertices to generate.
 *	@param vertices      is filled with two heights per vertex, the height of
 *                       the vertex in the current LOD and the height of the
 *                       vertex in the next lower LOD.
 */
void TerrainVertexBuffer::generate( const AliasedHeightMap* hm, 
									const AliasedHeightMap* lodHM, 
									uint32 resolutionX, 
									uint32 resolutionZ,
									BW::vector< Vector2 > & vertices )
{
	BW_GUARD;

	// Create the heights for the vertex buffer, the heights are calculated so
	// that the vertex contains the current height and the corresponding
	// height at the same x/z position in the next lower LOD.

	const uint32 nVertices =  resolutionX * resolutionZ;
	vertices.resize(nVertices);

	// Odd and even rows in the height map are treated differently as 
	// they have different bases for calculating the position for the next
	// LOD level down.

	// Calculate even rows first
	for (uint32 z = 0; z < resolutionZ; z += 2)
	{
		// Calculate the even row, even column.
		const BW::vector< Vector2 >::iterator it =
			vertices.begin() + resolutionX * z;
		uint32 lodX = 0;
		const uint32 lodZ = z / 2;
		for (uint32 x = 0; x < resolutionX; x += 2, ++lodX)
		{
			(it + x)->set(
				hm->height( x, z ),
				lodHM->height( lodX, lodZ ) );
		}
		
		// Calculate the even row, odd column.
		lodX = 0;
		for (uint32 x = 1; x < resolutionX; x += 2, ++lodX)
		{
			(it + x)->set(
				hm->height( x, z ),
				( lodHM->height( lodX, lodZ ) + lodHM->height( lodX + 1, lodZ ) 
				) / 2.0f + s_lodGeomorphingFix );
		}
	}

	// Calculate odd rows next
	for (uint32 z = 1; z < resolutionZ; z += 2)
	{
		// Calculate the odd row, even column.
		const BW::vector< Vector2 >::iterator it =
			vertices.begin() + resolutionX * z;
		uint32 lodX = 0;
		const uint32 lodZ = z / 2;
		for (uint32 x = 0; x < resolutionX; x += 2, ++lodX)
		{
			(it + x)->set(
				hm->height( x, z ),
				( lodHM->height( lodX, lodZ ) + lodHM->height( lodX, lodZ + 1) 
				) / 2.0f + s_lodGeomorphingFix );
		}

		// Calculate the odd row, odd column.
		lodX = 0;

		// These indices are for the odd and even rows and columns for creating 
		// the approximate height value of the next LOD down, since we are 
		// using crossed indices we want to alternate the angle of the diagonals
		// the height is calculated from.

		uint32 x1 = 0;
		uint32 x2 = 1;

		if (lodZ & 1)
		{
			x1 = 1;
			x2 = 0;
		}

		for (uint32 x = 1; x < resolutionX; x += 2, ++lodX)
		{
			(it + x)->set(
				hm->height( x, z ),
				( lodHM->height( lodX + x1, lodZ ) +
				  lodHM->height( lodX + x2, lodZ + 1) 
				) / 2.0f + s_lodGeomorphingFix );
			x1 ^= 1;
			x2 ^= 1;
		}
	}
}

/**
 *	This method generates the terrain vertex buffer using given vertices.
 *
 *	@param vertices      the height of vertex this lod and previous lod.
 *	@param resolutionX   the number of x-axis vertices to use.
 *	@param resolutionZ   the number of z-axis vertices to use.
 *	@param usage         D3D usage options that identify how resources are to be used.

 */
bool TerrainVertexBuffer::init( const Vector2 * vertices,
								uint32 resolutionX, uint32 resolutionZ,
								uint32 usage )
{
	BW_GUARD;
	bool res = false;
	const uint32 bufferSize = sizeof( Vector2 ) * resolutionX * resolutionZ;

	if ( Moo::rc().device() )
	{
		// Create the actual vertex buffer.
		pVertexBuffer_.release();
		if (SUCCEEDED( pVertexBuffer_.create( 
					bufferSize,
					usage, 0, D3DPOOL_MANAGED,
					"Terrain/VertexBuffer" ) ) )
		{
			Moo::VertexLock<Vector2> vl( pVertexBuffer_ );
			if (vl)
			{
				memcpy( vl, vertices, bufferSize );
				res = true;
				pGridBuffer_ = GridVertexBuffer::get( uint16(resolutionX), uint16(resolutionZ) );

				// Add the buffer to the preload list so that it can get uploaded
				// to video memory
				pVertexBuffer_.addToPreloadList();
			}
			else
			{
				pVertexBuffer_.release();
			}
		}
	}

	return res;
}

/**
 *	This method sets the terrain vertex buffer on the device
 *	@return true if successful
 */
bool TerrainVertexBuffer::set()
{
	BW_GUARD;
	bool res = false;
	if (pVertexBuffer_.valid())
	{
		pVertexBuffer_.set( 0, 0, sizeof( Vector2 ) );
		pGridBuffer_->pBuffer().set( 1, 0, sizeof( Vector2 ) );
		res = true;
	}

	return res;
}

} // namespace Terrain

BW_END_NAMESPACE

// terrian_vertex_buffer.cpp
