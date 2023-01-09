#ifndef OBJECT_CHANGE_SCENE_VIEW_HPP
#define OBJECT_CHANGE_SCENE_VIEW_HPP

#include "forward_declarations.hpp"
#include "scene_event_view.hpp"

#include "cstdmf/event.hpp"

namespace BW
{

class SCENE_API ObjectChangeSceneView :
	public SceneEventView
{
public:
	
	struct BaseObjectsEventArgs
	{
		SceneProvider * pProvider_;
		const SceneObject * pObjects_;
		size_t numObjects_;
	};

	typedef BaseObjectsEventArgs ObjectsAddedEventArgs;
	typedef BaseObjectsEventArgs ObjectsRemovedEventArgs;
	typedef BaseObjectsEventArgs ObjectsChangedEventArgs;

	typedef Event< Scene, const ObjectsAddedEventArgs& > ObjectsAddedEvent;
	typedef Event< Scene, const ObjectsRemovedEventArgs& > ObjectsRemovedEvent;
	typedef Event< Scene, const ObjectsChangedEventArgs& > ObjectsChangedEvent;

	ObjectsAddedEvent::EventDelegateList& objectsAdded();
	ObjectsRemovedEvent::EventDelegateList& objectsRemoved();
	ObjectsChangedEvent::EventDelegateList& objectsChanged();

	void notifyObjectsAdded( SceneProvider* pProvider, 
		const SceneObject* pObjects, size_t numObjects ) const;
	void notifyObjectsRemoved( SceneProvider* pProvider, 
		const SceneObject* pObjects, size_t numObjects ) const;
	void notifyObjectsChanged( SceneProvider* pProvider, 
		const SceneObject* pObjects, size_t numObjects ) const;

private:
	ObjectsAddedEvent onObjectsAdded_;
	ObjectsRemovedEvent onObjectsRemoved_;
	ObjectsChangedEvent onObjectsChanged_;
};

} // namespace BW

#endif // OBJECT_CHANGE_SCENE_VIEW_HPP
