#ifndef STATIC_SCENE_MODEL_HPP
#define STATIC_SCENE_MODEL_HPP


#include "cstdmf/stdmf.hpp"
#include "cstdmf/bw_vector.hpp"
#include "model/fashion.hpp"

#include "compiled_space/forward_declarations.hpp"
#include "static_scene_model_types.hpp"
#include "static_scene_type_handler.hpp"
#include "binary_format.hpp"


BW_BEGIN_NAMESPACE

	class SuperModel;

BW_END_NAMESPACE

namespace BW {
namespace CompiledSpace {

class COMPILED_SPACE_API StaticSceneModelHandler : 
	public IStaticSceneTypeHandler
{
public:
	static void registerHandlers( Scene & scene );

public:
	StaticSceneModelHandler();
	virtual ~StaticSceneModelHandler();

	size_t numModels() const;

protected:
	
	// IStaticSceneTypeHandler
	virtual SceneTypeSystem::RuntimeTypeID typeID() const;
	virtual SceneTypeSystem::RuntimeTypeID runtimeTypeID() const;

	virtual bool load( BinaryFormat& binFile,
		StaticSceneProvider& staticScene,
		const StringTable& strings,
		SceneObject* pLoadedObjects, size_t maxObjects );
	virtual bool bind();
	virtual void unload();

	virtual float loadPercent() const;

	void loadModelInstances( const StringTable& strings,
		SceneObject* pLoadedObjects, size_t maxObjects );
	void freeModelInstances();

	struct Instance;

	bool loadSuperModel(
		StaticSceneModelTypes::Model& def,
		const StringTable& strings,
		Instance& instance );

	BinaryFormat* reader_;
	BinaryFormat::Stream* pStream_;

	ExternalArray<StaticSceneModelTypes::Model> modelData_;
	ExternalArray<StringTableTypes::Index> resourceRefs_;

	class DrawHandler;
	class TickHandler;
	class CollisionHandler;
	class SpatialQueryHandler;
	class TextureStreamingHandler;

	struct Instance
	{
		StaticSceneModelTypes::Model* pDef_;
		SuperModel* pSuperModel_;
		SuperModelAnimationPtr pAnimation_;
		MaterialFashionVector materialFashions_;
		Matrix transformInverse_;
		StaticSceneModelHandler * pOwner_;

		void setSuperModel( SuperModel * pModel );
		void setTransform( const Matrix & transform );
	};

	BW::vector<Instance> instances_;
};

} // namespace CompiledSpace
} // namespace BW


#endif // STATIC_SCENE_MODEL_HPP
