#ifndef STATIC_SCENE_DECAL_HPP
#define STATIC_SCENE_DECAL_HPP

#include "cstdmf/stdmf.hpp"
#include "cstdmf/bw_vector.hpp"

#include "compiled_space/forward_declarations.hpp"
#include "static_scene_decal_types.hpp"
#include "static_scene_type_handler.hpp"
#include "binary_format.hpp"

namespace BW {
namespace CompiledSpace {

class COMPILED_SPACE_API StaticSceneDecalHandler : public IStaticSceneTypeHandler
{
public:
	static void registerHandlers( Scene & scene );

public:
	StaticSceneDecalHandler();
	virtual ~StaticSceneDecalHandler();

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
	void loadDecals( const StringTable& strings,
		SceneObject* pLoadedObjects, size_t maxObjects );
	void freeDecals();

	struct Instance;

	bool loadDecal(
		StaticSceneDecalTypes::Decal& def,
		const StringTable& strings,
		Instance& instance, size_t index );

private:
	BinaryFormat* reader_;
	BinaryFormat::Stream* pStream_;

	ExternalArray<StaticSceneDecalTypes::Decal> decalData_;

private:
	class DrawHandler;

	struct Instance
	{
		StaticSceneDecalTypes::Decal* pDef_;
		int32 instanceID_;		
	};

	BW::vector<Instance> instances_;
};

} // namespace CompiledSpace
} // namespace BW

#endif // STATIC_SCENE_DECAL_HPP
