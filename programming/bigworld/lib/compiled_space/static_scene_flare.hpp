#ifndef STATIC_SCENE_FLARE_HPP
#define STATIC_SCENE_FLARE_HPP

#include "cstdmf/stdmf.hpp"
#include "cstdmf/bw_vector.hpp"

#include "compiled_space/forward_declarations.hpp"
#include "static_scene_flare_types.hpp"
#include "static_scene_type_handler.hpp"
#include "binary_format.hpp"

#include "romp/lens_effect.hpp"

namespace BW {
namespace CompiledSpace {

class COMPILED_SPACE_API StaticSceneFlareHandler : public IStaticSceneTypeHandler
{
public:
	static void registerHandlers( Scene & scene );

public:
	StaticSceneFlareHandler();
	virtual ~StaticSceneFlareHandler();

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
	void loadFlares( const StringTable& strings,
		SceneObject* pLoadedObjects, size_t maxObjects );
	void freeFlares();

	struct Instance;

	bool loadFlare(
		StaticSceneFlareTypes::Flare& def,
		const StringTable& strings,
		Instance& instance, size_t index );

private:
	BinaryFormat* reader_;
	BinaryFormat::Stream* pStream_;

	ExternalArray<StaticSceneFlareTypes::Flare> flareData_;

private:
	class DrawHandler;

	struct Instance
	{
		StaticSceneFlareTypes::Flare* pDef_;
		LensEffect lensEffect_;
		size_t lensEffectID_;
		bool active_;
	};

	BW::vector<Instance> instances_;
};

} // namespace CompiledSpace
} // namespace BW


#endif // STATIC_SCENE_FLARE_HPP
