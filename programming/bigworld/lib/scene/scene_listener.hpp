#ifndef SCENE_LISTENER_HPP
#define SCENE_LISTENER_HPP

#include "forward_declarations.hpp"
#include "scene_dll.hpp"

namespace BW
{

class SCENE_API SceneListener
{
public:
	virtual ~SceneListener();

	virtual void onAddObject( SceneObject & object );
	virtual void onRemoveObject( SceneObject & object );
	virtual void onChangeObject( SceneObject & object );
};



} // namespace BW

#endif // SCENE_LISTENER_HPP
