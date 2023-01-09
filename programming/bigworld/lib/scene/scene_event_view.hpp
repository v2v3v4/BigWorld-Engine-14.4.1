#ifndef SCENE_EVENT_VIEW_HPP
#define SCENE_EVENT_VIEW_HPP

#include "forward_declarations.hpp"
#include "scene_provider.hpp"

namespace BW
{

class SCENE_API SceneEventView : 
	public ISceneView
{
public:
	virtual void addProvider( SceneProvider* pProvider );
	virtual void removeProvider( SceneProvider * pProvider );
};


} // namespace BW

#endif // SCENE_EVENT_VIEW_HPP
