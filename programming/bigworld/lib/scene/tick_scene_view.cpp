#include "pch.hpp"
#include "tick_scene_view.hpp"

namespace BW
{

void TickSceneView::tick( float dTime )
{
	for (Listeners::const_iterator iter = listeners_.begin();
		iter != listeners_.end(); ++iter)
	{
		ITickSceneListener* const pListener = *iter;
		pListener->onPreTick();
	}

	for ( ProviderCollection::iterator iter = providers_.begin();
		iter != providers_.end(); ++iter )
	{
		ProviderInterface* pProvider = *iter;
		pProvider->tick( dTime );
	}

	for (Listeners::const_iterator iter = listeners_.begin();
		iter != listeners_.end(); ++iter)
	{
		ITickSceneListener* const pListener = *iter;
		pListener->onPostTick();
	}
}


void TickSceneView::updateAnimations( float dTime )
{
	for ( ProviderCollection::iterator iter = providers_.begin();
		iter != providers_.end(); ++iter )
	{
		ProviderInterface* pProvider = *iter;
		pProvider->updateAnimations( dTime );
	}
}


void TickSceneView::addListener( ITickSceneListener* pListener )
{
	listeners_.push_back( pListener );
}


void TickSceneView::removeListener( ITickSceneListener* pListener )
{
	listeners_.erase( 
		std::remove(
		listeners_.begin(), 
		listeners_.end(), 
		pListener ), 
		listeners_.end() );
}


void TickOperation::tick( SceneObject* pObjects,
						size_t objectCount, float dTime )
{
	for (size_t i = 0; i < objectCount; ++i)
	{
		SceneObject& obj = pObjects[i];
		ITickOperationTypeHandler* pTypeHandler =
			this->getHandler( obj.type() );
		if (pTypeHandler)
		{
			pTypeHandler->doTick( &obj, 1, dTime );
		}
	}
}


void TickOperation::tick( SceneTypeSystem::RuntimeTypeID typeID,
					   SceneObject* pObjects, size_t objectCount, float dTime )
{
	ITickOperationTypeHandler* pTypeHandler =
		this->getHandler( typeID );
	if (pTypeHandler)
	{
		pTypeHandler->doTick( pObjects, objectCount, dTime );
	}
}


void TickOperation::updateAnimations( SceneObject* pObjects, size_t objectCount, float dTime )
{
	for (size_t i = 0; i < objectCount; ++i)
	{
		SceneObject& obj = pObjects[i];
		ITickOperationTypeHandler* pTypeHandler =
			this->getHandler( obj.type() );
		if (pTypeHandler)
		{
			pTypeHandler->doUpdateAnimations( &obj, 1, dTime );
		}
	}
}


void TickOperation::updateAnimations( SceneTypeSystem::RuntimeTypeID typeID,
					  SceneObject* pObjects, size_t objectCount, float dTime )
{
	ITickOperationTypeHandler* pTypeHandler =
		this->getHandler( typeID );
	if (pTypeHandler)
	{
		pTypeHandler->doUpdateAnimations( pObjects, objectCount, dTime );
	}
}

} // namespace BW
