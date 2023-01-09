#include "pch.hpp"
#include "change_scene_view.hpp"

#include <algorithm>

namespace BW
{

ChangeSceneViewListener::~ChangeSceneViewListener()
{
	// Nothing required of base.
}

void ChangeSceneViewListener::onAreaLoaded( const AABB & bb )
{
	// Nothing required of base.
}

void ChangeSceneViewListener::onAreaUnloaded( const AABB & bb )
{
	// Nothing required of base.
}

void ChangeSceneViewListener::onAreaChanged( const AABB & bb )
{
	// Nothing required of base.
}

//-----------------------------------------------------------------------------

void ChangeSceneView::addListener( ChangeSceneViewListener* pListener )
{
	listeners_.push_back( pListener );
}

void ChangeSceneView::removeListener( ChangeSceneViewListener* pListener )
{
	listeners_.erase( 
		std::remove(
			listeners_.begin(), 
			listeners_.end(), 
			pListener ), 
		listeners_.end() );
}

void ChangeSceneView::notifyAreaLoaded( const AABB & bb ) const
{
	for (Listeners::const_iterator iter = listeners_.begin();
		iter != listeners_.end(); ++iter)
	{
		ChangeSceneViewListener* const pListener = *iter;
		pListener->onAreaLoaded( bb );
	}
}

void ChangeSceneView::notifyAreaUnloaded( const AABB & bb ) const
{
	for (Listeners::const_iterator iter = listeners_.begin();
		iter != listeners_.end(); ++iter)
	{
		ChangeSceneViewListener * const pListener = *iter;
		pListener->onAreaUnloaded( bb );
	}
}

void ChangeSceneView::notifyAreaChanged( const AABB & bb ) const
{
	for (Listeners::const_iterator iter = listeners_.begin();
		iter != listeners_.end(); ++iter)
	{
		ChangeSceneViewListener * const pListener = *iter;
		pListener->onAreaChanged( bb );
	}
}


} // namespace BW
