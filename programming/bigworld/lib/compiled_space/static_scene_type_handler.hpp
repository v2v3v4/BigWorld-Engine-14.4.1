#ifndef STATIC_SCENE_TYPE_HANDLER_HPP
#define STATIC_SCENE_TYPE_HANDLER_HPP

#include "cstdmf/bw_namespace.hpp"

#include "scene/scene_type_system.hpp"

BW_BEGIN_NAMESPACE

	class SceneObject;

BW_END_NAMESPACE

namespace BW {
namespace CompiledSpace {

class BinaryFormat;
class StringTable;
class StaticSceneProvider;

class IStaticSceneTypeHandler
{
public:
	virtual ~IStaticSceneTypeHandler() {}

	virtual SceneTypeSystem::RuntimeTypeID typeID() const = 0;
	virtual SceneTypeSystem::RuntimeTypeID runtimeTypeID() const = 0;

	virtual bool load( BinaryFormat& binFile,
		StaticSceneProvider& staticScene,
		const StringTable& strings,
		SceneObject* pLoadedObjects,
		size_t maxObjects ) = 0;
	virtual bool bind(){ return true; }
	virtual void unload() = 0;

	virtual float loadPercent() const = 0;
};

} // namespace CompiledSpace
} // namespace BW


#endif // STATIC_SCENE_TYPE_HANDLER_HPP
