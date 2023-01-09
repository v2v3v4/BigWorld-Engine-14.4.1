#ifndef TERRAIN2_SCENE_PROVIDER_HPP
#define TERRAIN2_SCENE_PROVIDER_HPP

#include "cstdmf/stdmf.hpp"
#include "cstdmf/lookup_table.hpp"
#include "resmgr/datasection.hpp"

#include "math/matrix.hpp"
#include "math/boundbox.hpp"
#include "math/loose_octree.hpp"

#include "scene/scene_object.hpp"
#include "scene/scene_provider.hpp"
#include "scene/intersect_scene_view.hpp"
#include "scene/collision_scene_view.hpp"

#include "compiled_space/binary_format.hpp"
#include "compiled_space/terrain2_types.hpp"
#include "compiled_space/loader.hpp"

#include "terrain/terrain_settings.hpp"
#include "terrain/terrain_scene_view.hpp"

BW_BEGIN_NAMESPACE

namespace Terrain 
{
	class TerrainBlock2;
	class TerrainSettings;

	typedef SmartPointer<TerrainBlock2> TerrainBlock2Ptr;
}

class ChunkTerrainObstacle;
typedef SmartPointer<ChunkTerrainObstacle> ChunkTerrainObstaclePtr;

BW_END_NAMESPACE

namespace BW {
namespace CompiledSpace {

	class StringTable;

	class Terrain2SceneProvider :
		public ILoader,
		public SceneProvider,
		public IIntersectSceneViewProvider,
		public ICollisionSceneViewProvider,
		public Terrain::ITerrainSceneViewProvider
	{
	public:
		static void registerHandlers( Scene & scene );

	public:
		Terrain2SceneProvider();
		virtual ~Terrain2SceneProvider();

		// ILoader interface
		bool doLoadFromSpace( ClientSpace * pSpace,
			BinaryFormat& reader,
			const DataSectionPtr& pSpaceSettings,
			const Matrix& transform,
			const StringTable& strings );

		bool doBind();
		void doUnload();
		float percentLoaded() const;

		bool isValid() const;

		float blockSize() const;
		size_t numBlocks() const;

	private:
		
		// Scene view implementations
		virtual void * getView(
			const SceneTypeSystem::RuntimeTypeID & sceneInterfaceTypeID);

		virtual size_t intersect( const SceneIntersectContext& context,
			const ConvexHull& hull, 
			IntersectionSet & intersection ) const;

		virtual bool collide( const Vector3 & source,
			const Vector3 & extent,
			const SweepParams& sp,
			CollisionState & state ) const;

		virtual bool collide( const WorldTriangle & source,
			const Vector3 & extent,
			const SweepParams& sp,
			CollisionState & state ) const;

		virtual float findTerrainHeight( const Vector3 & position ) const;

		virtual bool findTerrainBlock( Vector3 const & position,
			Terrain::TerrainFinder::Details & result );

	private:
		BinaryFormat* pReader_;
		BinaryFormat::Stream* pStream_;

		Terrain2Types::Header* pHeader_;
		ExternalArray<Terrain2Types::BlockData> blockData_;
		ExternalArray<uint32> blockGridLookup_;

	private:
		bool loadBlockInstances( const char* spaceDir,
			const Matrix& transform,
			const StringTable& strings );

		void freeBlockInstances();

	private:
		class Detail;
		class DrawHandler;
		class SpatialQueryHandler;
		class TerrainFinder;

		struct BlockInstance
		{
			Matrix transform_;
			Matrix inverseTransform_;
			AABB localBB_;
			AABB worldBB_;
			Terrain::TerrainBlock2Ptr pBlock_;
			SceneObject sceneObject_;
		};

		SmartPointer<Terrain::TerrainSettings> pTerrainSettings_;
		BW::string resourcesPath_;
		BW::vector<BlockInstance> blockInstances_;
		size_t numBlocksLoaded_;

		typedef StaticLooseOctree Octree;
		Octree sceneOctree_;
		typedef BW::ExternalArray<StaticLooseOctree::NodeDataReference> SceneContents_;
		SceneContents_ sceneContents_;
		LookUpTable< DataSpan, BW::ExternalArray<DataSpan> > sceneOctreeContents_;
	};

} // namespace CompiledSpace
} // namespace BW



#endif // TERRAIN2_SCENE_PROVIDER_HPP
