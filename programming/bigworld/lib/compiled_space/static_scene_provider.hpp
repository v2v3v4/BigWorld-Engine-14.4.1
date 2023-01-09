#ifndef STATIC_SCENE_PROVIDER_HPP
#define STATIC_SCENE_PROVIDER_HPP

#include "cstdmf/lookup_table.hpp"
#include "cstdmf/fourcc.hpp"

#include "scene/scene_provider.hpp"
#include "scene/tick_scene_view.hpp"
#include "scene/intersect_scene_view.hpp"
#include "scene/collision_scene_view.hpp"

#include "loader.hpp"
#include "binary_format.hpp"
#include "static_scene_types.hpp"
#include "static_scene_model.hpp"
#include "static_scene_speed_tree.hpp"
#include "static_scene_water.hpp"
#include "static_scene_flare.hpp"
#include "static_scene_decal.hpp"

namespace BW {

class Sphere;

namespace CompiledSpace {

class StringTable;
class StaticSceneProvider;

class StaticSceneProvider :
	public ILoader,
	public SceneProvider,
	public ITickSceneProvider,
	public IIntersectSceneViewProvider,
	public ICollisionSceneViewProvider
{
public:
	static void registerHandlers( Scene & scene );

public:
	StaticSceneProvider( FourCC formatMagic );
	virtual ~StaticSceneProvider();

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
	
	const SceneObject& getObject( size_t idx ) const;

	void addTypeHandler( IStaticSceneTypeHandler* pHandler );
	void removeTypeHandler( IStaticSceneTypeHandler* pHandler );

private:
	
	// Scene view implementations
	virtual void * getView(
		const SceneTypeSystem::RuntimeTypeID & sceneInterfaceTypeID);

	virtual void tick( float dTime );
	virtual void updateAnimations( float dTime );
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

private:
	class Detail;
	class DebugDraw;

	typedef BW::ExternalArray< AABB > ObjectBounds;
	typedef BW::ExternalArray< Sphere > ObjectSphereBounds;

	FourCC formatMagic_;

	BinaryFormat* pReader_;
	BinaryFormat::Stream* pStream_;

	ExternalArray<StaticSceneTypes::TypeHeader> typeHeaders_;

	LookUpTable<IStaticSceneTypeHandler*> typeHandlers_;

	BW::vector<SceneObject> objects_;
	ObjectBounds sceneObjectBounds_;
	ObjectSphereBounds sceneObjectSphereBounds_;

	typedef StaticLooseOctree Octree;
	typedef BW::ExternalArray<StaticLooseOctree::NodeDataReference> OctreeContents;
	typedef LookUpTable< DataSpan, BW::ExternalArray<DataSpan> > 
		OctreeNodeContents;

	Octree sceneOctree_;
	OctreeContents sceneOctreeContents_;
	OctreeNodeContents sceneOctreeNodeContents_;

public:
};

} // namespace CompiledSpace
} // namespace BW

#endif // STATIC_SCENE_PROVIDER_HPP
