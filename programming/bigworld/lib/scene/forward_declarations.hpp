#ifndef SCENE_FORWARD_DECLARATIONS_HPP
#define SCENE_FORWARD_DECLARATIONS_HPP

//-----------------------------------------------------------------------------
// System headers
#if !defined( _MSC_VER )
#include <tr1/functional>
#endif // !defined( _MSC_VER )

//-----------------------------------------------------------------------------
// Library Forward declarations

namespace BW
{
class SceneObject;
class SceneProvider;
class Scene;
class ISceneView;
class ISceneObjectOperation;
class SceneListener;
class IntersectionSet;


typedef std::tr1::function< void(const SceneObject&) > ConstSceneObjectCallback;
typedef std::tr1::function< void(SceneObject&) > SceneObjectCallback;

namespace SceneTypeSystem
{

}

} // namespace BW

#endif // SCENE_FORWARD_DECLARATIONS_HPP
