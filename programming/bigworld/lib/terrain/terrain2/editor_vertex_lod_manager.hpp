#ifndef TERRAIN_EDITOR_TERRAIN_LOD_MANAGER_HPP
#define TERRAIN_EDITOR_TERRAIN_LOD_MANAGER_HPP

#ifdef EDITOR_ENABLED

#include "vertex_lod_manager.hpp"
#include "aliased_height_map.hpp"


BW_BEGIN_NAMESPACE

class BackgroundTask;

namespace Terrain
{
	/**
	 * This class manages LODs in an editor context, if source height map 
	 * changes, required LODs are also regenerated.
	 */
	class EditorVertexLodManager : public VertexLodManager
	{
		friend class TerrainBlock2;

	public:

		/**
		 * Construct an empty EditorTerrainLodManager with space for a given 
		 * number of LODs.
		 *
		 * @param numLods How many LODs to create.
		 */
		EditorVertexLodManager( TerrainBlock2& owner, uint32 numLods );

		/**
		 * Return given lod, if heightMap is dirty this will regenerate all 
		 * required. This always streams synchronously.
		 */
		virtual void stream( ResourceStreamType streamType 
			= defaultStreamType() );

		/**
		 * Flag the height map as being dirty so lods will be regenerated.
		 */
		void setDirty() { isDirty_ = true; }

		/**
		* Save all lods to this terrain section using this height map. 
		*/
		static bool save(	const BW::string &		terrainSectionPath, 
							TerrainHeightMap2Ptr	pSourceHeightMap );
		static bool save(	DataSectionPtr			pTerrainSection,
							TerrainHeightMap2Ptr	pSourceHeightMap );

		static ResourceStreamType & defaultStreamType()
		{
			static ResourceStreamType s_type = RST_Syncronous;

			return s_type;
		}

	protected:
		virtual bool load();

	private:

		// Data flag
		bool					isDirty_;

		// no copying
		EditorVertexLodManager( const EditorVertexLodManager& );
		EditorVertexLodManager& operator=( const EditorVertexLodManager& );
	};

} // namespace Terrain

BW_END_NAMESPACE

#endif //EDITOR_ENABLED

#endif // TERRAIN_LOD_MANAGER_HPP
