#include "pch.hpp"

#ifdef EDITOR_ENABLED

#include "editor_vertex_lod_manager.hpp"

#include "../terrain_data.hpp"
#include "terrain_block2.hpp"
#include "terrain_height_map2.hpp"


DECLARE_DEBUG_COMPONENT2("Terrain", 0)


BW_BEGIN_NAMESPACE

using namespace Terrain;

EditorVertexLodManager::EditorVertexLodManager( TerrainBlock2& owner, uint32 numLods )
:	VertexLodManager( owner, numLods )
{

}

void EditorVertexLodManager::stream( ResourceStreamType streamType 
	/*= defaultStreamType()*/ )
{
	BW_GUARD;
	// if we're dirty, make currentWorkingSet "wrong" so base stream method is
	// forced to load everything.
	if ( isDirty_ )
	{
		currentWorkingSet_.start_	= 0;
		currentWorkingSet_.end_		= 0;
	}

	VertexLodManager::stream( streamType );
}

bool EditorVertexLodManager::save( const BW::string &		terrainSectionPath,
								   TerrainHeightMap2Ptr	pSourceHeightMap )
{
	BW_GUARD;
	DataSectionPtr pTerrainSection = BWResource::openSection( terrainSectionPath );
	if ( !pTerrainSection )
	{
		WARNING_MSG("Can't save to terrain section %s", terrainSectionPath.c_str() );
		return false;
	}

	return save( pTerrainSection, pSourceHeightMap );
}

bool EditorVertexLodManager::save( DataSectionPtr			pTerrainSection,
									TerrainHeightMap2Ptr	pSourceHeightMap )
{
	BW_GUARD;
	AliasedHeightMap baseHM	= AliasedHeightMap(0, pSourceHeightMap);

	// Get the grid size for this height map
	uint32 gridSize = pSourceHeightMap->blocksWidth();

	// Calculate the number of vertex lods
	uint32 numVertexLods = 1;
	while ((gridSize >> numVertexLods) > 1)
		numVertexLods++;

	for (uint32 i = 0; i < numVertexLods; i++)
	{
		// Create an aliased height map based on the current lod level's heightmap
		AliasedHeightMap previousHM(i + 1, pSourceHeightMap);

		// Create lod level data
		VertexLodEntry lodEntry;
		lodEntry.init( &baseHM, &previousHM, gridSize );

		// Save it
		lodEntry.save( pTerrainSection, i, gridSize );

		// next level
		baseHM = previousHM;
		gridSize /= 2;
	}
	return true;
}

bool EditorVertexLodManager::load()
{
	BW_GUARD;
	if (isDirty_) 
	{
		// Clear the entire array, there may be non-null vertex LODs outside
		// of the working set.
		for 
		( 
			VertexLodArray::iterator it = object_->begin(); 
			it != object_->end(); 
			++it 
		)
		{
			// Remove LODs if they are dirty, to force them to regenerate.
			*it = NULL;
		}
	}

	bool status = VertexLodManager::load();

	// we're clean now if that worked.
	if (status) isDirty_ = false;

	return status;
}

BW_END_NAMESPACE

#endif // EDITOR_ENABLED
