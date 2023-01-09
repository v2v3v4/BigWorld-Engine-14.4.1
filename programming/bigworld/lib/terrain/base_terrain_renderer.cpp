
#include "pch.hpp"
#include "base_terrain_renderer.hpp"
#include "manager.hpp"

BW_BEGIN_NAMESPACE

// TODO: We should get rid of globals and externs!!!
bool g_drawTerrain = true;

namespace Terrain
{

/**
 *	This method is a wrapper for the manager's currentRenderer setter method.
 *
 *	@param newInstance	The new current renderer
 *	@see Terrain::Manager::currentRenderer
 */
/*static*/ void BaseTerrainRenderer::instance( BaseTerrainRenderer* newInstance )
{
	Manager::instance().currentRenderer( newInstance );
}

/**
 *	This method is a wrapper for the manager's currentRenderer getter method.
 *
 *	@param newInstance	The new current renderer
 *	@see Terrain::Manager::currentRenderer
 */
/*static*/ BaseTerrainRenderer* BaseTerrainRenderer::instance()
{
	return Manager::instance().currentRenderer();
}

/**
 *  This method sets a flag that indicates whether or not hole map is used during 
 *  terrain texture rendering.
 * 
 *  @return old value of the flag
 */
bool BaseTerrainRenderer::enableHoleMap( bool value ) 
{ 
	bool ret = holeMapEnabled_;
	holeMapEnabled_ = value; 

	return ret;
}
/**
*  This method initializes visibility watcher fo terrain
*  
* 
*/

#if ENABLE_WATCHERS
void BaseTerrainRenderer::initWatcher()
{
	static bool s_isWatcherAdded = false;
	if (!s_isWatcherAdded)
	{
		s_isWatcherAdded = true;
		MF_WATCH("Visibility/terrain", isEnabledInWatcher, Watcher::WT_READ_WRITE, "Is terrain visible." );
	}
}
bool BaseTerrainRenderer::isEnabledInWatcher = true;
#endif

void BaseTerrainRenderer::enableOverlays(bool isEnabled)
{
	overlaysEnabled_ = isEnabled;
}

} // namespace Terrain

BW_END_NAMESPACE

// base_terrain_renderer.cpp
