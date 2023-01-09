#ifndef STATIC_SCENE_SPEED_TREE_HPP
#define STATIC_SCENE_SPEED_TREE_HPP

#include "cstdmf/stdmf.hpp"
#include "cstdmf/bw_vector.hpp"

#include "compiled_space/forward_declarations.hpp"
#include "static_scene_speed_tree_types.hpp"
#include "static_scene_type_handler.hpp"
#include "binary_format.hpp"
#include "speedtree/speedtree_config_lite.hpp"

#if SPEEDTREE_SUPPORT

BW_BEGIN_NAMESPACE

namespace speedtree
{
	class SpeedTreeRenderer;
}

BW_END_NAMESPACE

namespace BW {
namespace CompiledSpace {

class COMPILED_SPACE_API StaticSceneSpeedTreeHandler : public IStaticSceneTypeHandler
{
public:
	static void registerHandlers( Scene & scene );

public:
	StaticSceneSpeedTreeHandler();
	virtual ~StaticSceneSpeedTreeHandler();

protected:
	
	// IStaticSceneTypeHandler
	virtual SceneTypeSystem::RuntimeTypeID typeID() const;
	virtual SceneTypeSystem::RuntimeTypeID runtimeTypeID() const;

	virtual bool load( BinaryFormat& binFile,
		StaticSceneProvider& staticScene,
		const StringTable& strings,
		SceneObject* pLoadedObjects, size_t maxObjects );

	virtual void unload();

	virtual float loadPercent() const;

	void loadTreeInstances( const StringTable& strings,
		SceneObject* pLoadedObjects, size_t maxObjects );
	void freeTreeInstances();

	struct Instance;

	bool loadSpeedTree(
		StaticSceneSpeedTreeTypes::SpeedTree& def,
		const StringTable& strings,
		Instance& instance );

	BinaryFormat* reader_;
	BinaryFormat::Stream* pStream_;

	ExternalArray<StaticSceneSpeedTreeTypes::SpeedTree> treeData_;

	class DrawHandler;
	class CollisionHandler;
	class SpatialQueryHandler;
	class TextureStreamingHandler;

	struct Instance
	{
		StaticSceneSpeedTreeTypes::SpeedTree* pDef_;
		speedtree::SpeedTreeRenderer* pSpeedTree_;
		Matrix transformInverse_;
		StaticSceneSpeedTreeHandler* pOwner_;
	};

	BW::vector<Instance> instances_;
};

} // namespace CompiledSpace
} // namespace BW


#endif // SPEEDTREE_SUPPORT

#endif // STATIC_SCENE_SPEED_TREE_HPP
