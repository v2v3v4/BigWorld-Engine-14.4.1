#include "pch.hpp"
#include "scene_listener.hpp"

namespace BW
{


SceneListener::~SceneListener()
{

};


void SceneListener::onAddObject( SceneObject & object )
{
	// Default do nothing
};


void SceneListener::onRemoveObject( SceneObject & object )
{
	// Default do nothing
};


void SceneListener::onChangeObject( SceneObject & object )
{
	// Default do nothing
};


} // namespace BW
