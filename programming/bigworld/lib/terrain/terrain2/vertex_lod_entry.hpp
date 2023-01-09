#ifndef __TERRAIN_VERTEX_LOD_ENTRY_HPP__
#define __TERRAIN_VERTEX_LOD_ENTRY_HPP__

#include "resmgr/datasection.hpp"

#ifndef MF_SERVER
#include "terrain_index_buffer.hpp"
#endif

/**
 * Enable this to watch vertex lod memory usage and outstanding load requests.
 */
#define WATCH_TERRAIN_VERTEX_LODS


BW_BEGIN_NAMESPACE

namespace Moo
{
	class EffectMaterial;
}

namespace Terrain
{
	class   TerrainIndexBuffer;
	class   TerrainVertexBuffer;
	class	AliasedHeightMap;

	typedef SmartPointer<TerrainIndexBuffer>    TerrainIndexBufferPtr;
	typedef SmartPointer<TerrainVertexBuffer>   TerrainVertexBufferPtr;

	/**
	 * This class holds index and vertex info for a single terrain LOD.
	 * @see LodManager
	 */
	class VertexLodEntry : public SafeReferenceCount
	{
	public:

		VertexLodEntry();
		~VertexLodEntry();

		/** 
		* Initialise this LODEntry from height map at a given grid size.
		*/
		bool init(	const AliasedHeightMap* baseMap, 
					const AliasedHeightMap* previousMap,
					uint32					gridSize );

		/**
		 * Load vertices and grid size from data section
		 */
		bool load(	DataSectionPtr pTerrain, uint32 level, 
					BinaryPtr& pVertices, 
					uint32& gridSize );

#ifdef EDITOR_ENABLED
		/**
		 * Save vertices to a data section.
		 */
		bool save(  DataSectionPtr pTerrain, uint32 level,
					uint32 gridSize );
#endif // EDITOR_ENABLED

#ifndef MF_SERVER
		/**
		 * This method draws the LodEntry using current render context.
		 */
		bool draw(	const Moo::EffectMaterialPtr& pMaterial,
					const Vector2&			morphRanges,
					const NeighbourMasks&	neighbourMasks,
					uint8					subBlockMask  );

		void getVerticesMemoryMap(BW::map<void*, int>& verts);
#endif // MF_SERVER

#ifdef WATCH_TERRAIN_VERTEX_LODS
		/** Debugging values (for watch) */
		uint32					sizeInBytes_;
		static float			s_totalMB_;
#endif
		/**
		* Get section name for a given lod level.
		*/
		static void getSectionName( uint32 level, BW::string& sectionName );

	private:

		/** A pointer to the indexes for this LOD */
		TerrainIndexBufferPtr	pIndexes_;

		/** A pointer to the vertices for this LOD */
		TerrainVertexBufferPtr	pVertices_;

		// no copying
		VertexLodEntry( const VertexLodEntry& );
		VertexLodEntry& operator=( const VertexLodEntry& );
	};
	typedef SmartPointer<VertexLodEntry> LodEntryPtr;

} // namespace Terrain

BW_END_NAMESPACE

#endif // TERRAIN_LOD_ENTRY_HPP
