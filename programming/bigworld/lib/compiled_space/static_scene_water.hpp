#ifndef STATIC_SCENE_WATER_HPP
#define STATIC_SCENE_WATER_HPP

#include "cstdmf/stdmf.hpp"
#include "cstdmf/bw_vector.hpp"

#include "compiled_space/forward_declarations.hpp"
#include "static_scene_water_types.hpp"
#include "static_scene_type_handler.hpp"
#include "binary_format.hpp"


BW_BEGIN_NAMESPACE

BW_END_NAMESPACE

namespace BW {
namespace CompiledSpace {

class COMPILED_SPACE_API StaticSceneWaterHandler : public IStaticSceneTypeHandler
{
public:
	static void registerHandlers( Scene & scene );

public:
	StaticSceneWaterHandler();
	virtual ~StaticSceneWaterHandler();

private:
	
	// IStaticSceneTypeHandler
	virtual SceneTypeSystem::RuntimeTypeID typeID() const;
	virtual SceneTypeSystem::RuntimeTypeID runtimeTypeID() const;

	virtual bool load( BinaryFormat& binFile,
		StaticSceneProvider& staticScene,
		const StringTable& strings,
		SceneObject* pLoadedObjects, size_t maxObjects );

	virtual void unload();

	virtual float loadPercent() const;

private:
	void loadWaterInstances( const StringTable& strings,
		SceneObject* pLoadedObjects, size_t maxObjects );
	void freeWaterInstances();

	struct Instance;

	bool loadWaterObject(
		StaticSceneWaterTypes::Water& def,
		const StringTable& strings,
		Instance& instance );

private:
	BinaryFormat* reader_;
	BinaryFormat::Stream* pStream_;

	ExternalArray<StaticSceneWaterTypes::Water> waterData_;

private:
	class DrawHandler;

	struct Instance
	{
		StaticSceneWaterTypes::Water* pDef_;
		Water::WaterResources resources_;
		BW::Water* pWater_;
	};

	BW::vector<Instance> instances_;
};

} // namespace CompiledSpace
} // namespace BW


#endif // STATIC_SCENE_WATER_HPP
