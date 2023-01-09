#ifndef PY_MODEL_OBSTACLE_ENTITY_EMBODIMENT_HPP
#define PY_MODEL_OBSTACLE_ENTITY_EMBODIMENT_HPP

#include "compiled_space/forward_declarations.hpp"

#include "space/entity_embodiment.hpp"
#include "space/dynamic_scene_provider_types.hpp"
#include "scene/scene_type_system.hpp"
#include "scene/scene_object.hpp"

#include "math/boundbox.hpp"
#include "math/matrix.hpp"
#include "math/matrix_liason.hpp"

#include "duplo/py_model_obstacle.hpp"


BW_BEGIN_NAMESPACE

	class PyModelObstacle;

BW_END_NAMESPACE

namespace BW {
namespace CompiledSpace {

class CompiledSpace;

class COMPILED_SPACE_API PyModelObstacleEntityEmbodiment :
	public IEntityEmbodiment
{
public:
	static void registerHandlers( Scene& scene );

public:
	PyModelObstacleEntityEmbodiment( const PyModelObstaclePtr& pObstacle );
	virtual ~PyModelObstacleEntityEmbodiment();

	virtual void doMove( float dTime );
	virtual void doTick( float dTime );
	virtual void doUpdateAnimations( float dTime );

	virtual void doWorldTransform( const Matrix & transform );
	virtual const Matrix & doWorldTransform() const;
	virtual const AABB & doLocalBoundingBox() const;

	virtual void doDraw( Moo::DrawContext & drawContext );

	virtual bool doIsOutside() const;
	virtual bool doIsRegionLoaded( Vector3 testPos, float radius ) const;

	virtual void doEnterSpace( ClientSpacePtr pSpace, bool transient );
	virtual void doLeaveSpace( bool transient );


private:
	void syncTransforms();

private:
	class TickHandler;
	class DrawHandler;
	class CollisionHandler;
	class TextureStreamingHandler;

	mutable AABB localBB_;
	Matrix worldTransform_;
	Matrix inverseWorldTransform_;
	mutable PyModelObstaclePtr pObstacle_;
	CompiledSpace* pEnteredSpace_;
	DynamicObjectHandle dynamicObjectHandle_;
	SceneObject sceneObject_;
};


} // namespace CompiledSpace
} // namespace BW

#endif // PY_MODEL_OBSTACLE_ENTITY_EMBODIMENT_HPP
