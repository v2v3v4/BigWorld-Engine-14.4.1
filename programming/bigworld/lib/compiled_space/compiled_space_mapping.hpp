#ifndef COMPILED_SPACE_MAPPING_HPP
#define COMPILED_SPACE_MAPPING_HPP

#include "compiled_space/forward_declarations.hpp"

#include "cstdmf/bw_string.hpp"
#include "resmgr/datasection.hpp"
#include "math/matrix.hpp"

#include "compiled_space_settings.hpp"
#include "string_table.hpp"
#include "asset_list.hpp"
#include "static_geometry.hpp"
#include "entity_list.hpp"

#include "static_scene_provider.hpp"
#include "static_scene_model.hpp"
#include "static_scene_speed_tree.hpp"
#include "static_scene_decal.hpp"
#include "static_scene_water.hpp"
#include "static_scene_flare.hpp"
#include "terrain2_scene_provider.hpp"
#include "entity_udo_factory.hpp"
#include "particle_system_loader.hpp"
#include "light_scene_provider.hpp"
#include "static_texture_streaming_scene_provider.hpp"

BW_BEGIN_NAMESPACE
	
	class TaskManager;
	class AssetClient;

BW_END_NAMESPACE

namespace BW {
namespace CompiledSpace {

class CompiledSpace;
class ILoader;

class COMPILED_SPACE_API CompiledSpaceMapping
{
public:
	enum State
	{
		STATE_UNLOADED,
		STATE_LOADING,
		STATE_FAILED_TO_LOAD,
		STATE_LOADED
	};

public:
	CompiledSpaceMapping( const BW::string& path,
		const Matrix& transform,
		const DataSectionPtr& pSettings,
		CompiledSpace& space,
		AssetClient* pAssetClient );
	virtual ~CompiledSpaceMapping();

	void addLoader( ILoader* pLoader );
	void removeLoader( ILoader* pLoader );

	bool loadAsync( TaskManager& taskMgr );
	void unloadSync();

	State state() const;

	float loadStatus() const;

	const AABB& boundingBox() const;

private:
	void loadBG();
	void loadBGFinished();

	void ensureSpaceCompiled();

	void spawnEntities();
	void removeEntities();

	void spawnUserDataObjects();
	void removeUserDataObjects();

private:
	BW::string path_;
	Matrix transform_;
	DataSectionPtr pSettings_;

	State state_;
	AssetClient* pAssetClient_;

	class LoaderTask;
	friend class LoaderTask;
	LoaderTask* pTask_;
	uint32 numFailures_;

	uint64 loadStartTimestamp_;

	EntityList entities_;
	EntityList userDataObjects_;
	
	BW::vector<IEntityUDOFactory::EntityID> entityInstances_;
	BW::vector<IEntityUDOFactory::UDOHandle> userDataObjectInstances_;

protected:
	CompiledSpace& space_;
	BinaryFormat reader_;
	CompiledSpaceSettings settings_;
	StringTable stringTable_;
	AssetList assetList_;
	StaticGeometry staticGeometry_;

private:
	typedef BW::vector<ILoader*> Loaders;
	Loaders loaders_;
};


class COMPILED_SPACE_API DefaultCompiledSpaceMapping :
	public CompiledSpaceMapping
{
public:
	DefaultCompiledSpaceMapping( const BW::string& path,
		const Matrix& transform,
		const DataSectionPtr& pSettings,
		CompiledSpace& space,
		AssetClient* pAssetClient );
	virtual ~DefaultCompiledSpaceMapping();

	static CompiledSpaceMapping* create( 
		const BW::string& path,
		const Matrix& transform,
		const DataSectionPtr& pSettings,
		CompiledSpace& space,
		AssetClient* pAssetClient );

protected:
	StaticSceneProvider staticScene_;
	StaticSceneProvider vloScene_;
	Terrain2SceneProvider terrain2_;
	ParticleSystemLoader particleLoader_;
	LightSceneProvider lightScene_;
	StaticTextureStreamingSceneProvider textureStreamingScene_;

	StaticSceneModelHandler staticModels_;
#if SPEEDTREE_SUPPORT
	StaticSceneSpeedTreeHandler speedTrees_;
#endif
	StaticSceneFlareHandler flares_;
	StaticSceneDecalHandler decals_;
	StaticSceneWaterHandler water_;
};



} // namespace CompiledSpace
} // namespace BW


#endif // COMPILED_SPACE_MAPPING_HPP
