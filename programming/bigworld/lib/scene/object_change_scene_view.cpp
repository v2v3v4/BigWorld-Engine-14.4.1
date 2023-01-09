#include "pch.hpp"
#include "object_change_scene_view.hpp"

#include <algorithm>

namespace BW
{

ObjectChangeSceneView::ObjectsAddedEvent::EventDelegateList& 
	ObjectChangeSceneView::objectsAdded()
{
	return onObjectsAdded_.delegates();
}


ObjectChangeSceneView::ObjectsRemovedEvent::EventDelegateList& 
	ObjectChangeSceneView::objectsRemoved()
{
	return onObjectsRemoved_.delegates();
}


ObjectChangeSceneView::ObjectsChangedEvent::EventDelegateList& 
	ObjectChangeSceneView::objectsChanged()
{
	return onObjectsChanged_.delegates();
}


void ObjectChangeSceneView::notifyObjectsAdded( SceneProvider* pProvider, 
	const SceneObject* pObjects, size_t numObjects ) const
{
	ObjectsAddedEventArgs args;
	args.pProvider_ = pProvider;
	args.numObjects_ = numObjects;
	args.pObjects_ = pObjects;
	onObjectsAdded_.invoke( scene(), args );
}


void ObjectChangeSceneView::notifyObjectsRemoved( SceneProvider* pProvider, 
	const SceneObject* pObjects, size_t numObjects ) const
{
	ObjectsRemovedEventArgs args;
	args.pProvider_ = pProvider;
	args.numObjects_ = numObjects;
	args.pObjects_ = pObjects;
	onObjectsRemoved_.invoke( scene(), args );
}


void ObjectChangeSceneView::notifyObjectsChanged( SceneProvider* pProvider, 
	const SceneObject* pObjects, size_t numObjects ) const
{
	ObjectsChangedEventArgs args;
	args.pProvider_ = pProvider;
	args.numObjects_ = numObjects;
	args.pObjects_ = pObjects;
	onObjectsChanged_.invoke( scene(), args );
}


} // namespace BW
