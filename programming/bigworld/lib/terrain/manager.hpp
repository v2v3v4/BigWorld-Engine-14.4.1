#ifndef TERRAIN_HPP
#define TERRAIN_HPP


#include "cstdmf/init_singleton.hpp"
#include "cstdmf/smartpointer.hpp"
#include "resmgr/datasection.hpp"

#ifndef MF_SERVER

#include "terrain2/terrain_renderer2.hpp"
#include "terrain2/terrain_lod_controller.hpp"
#include "terrain_graphics_options.hpp"

#ifdef EDITOR_ENABLED
#include "terrain2/ttl2cache.hpp"
#endif // EDITOR_ENABLED

#endif // MF_SERVER


BW_BEGIN_NAMESPACE

namespace Terrain
{

#ifndef MF_SERVER
struct TerrainVertexLodConsts;
#endif

/**
 *	This class is the main entry point for the Terrain Library, holding library
 *	objects such as the terrain renderers. It also keeps track of the current
 *	active renderer, and a reference to the terrain2 defaults DataSection.
 *
 *	The init method must be called before using any terrain code, and fini 
 *	must be called at the end of the app to release resources.
 *
 *  NOTE: We shouldn't be adding new global objects here, we should instead try
 *  to keep objects independent of each other and avoid singletons altogether.
 */
class Manager : public Singleton<Manager>
{
public:
	Manager();
	virtual ~Manager();

	const DataSectionPtr pTerrain2Defaults() const { return pTerrain2Defaults_; }
	bool isValid() const { return isValid_; }

#ifndef MF_SERVER

	void currentRenderer( BaseTerrainRenderer* newInstance );
	BaseTerrainRenderer* currentRenderer();

	const BasicTerrainLodController& lodController() const { return lodController_; }
	BasicTerrainLodController& lodController() { return lodController_; }

	const TerrainGraphicsOptions& graphicsOptions() const { return graphicsOptions_; }
	TerrainGraphicsOptions& graphicsOptions() { return graphicsOptions_; }

	void wireFrame( bool wireFrame ) { wireFrame_ = wireFrame; }
	bool wireFrame() const { return wireFrame_; }

	TerrainVertexLodConsts *vertexLodConsts( float blockSize ) const;

#ifdef EDITOR_ENABLED
	TTL2Cache& ttl2Cache() { return ttl2Cache_; }
#endif // EDITOR_ENABLED

#endif // MF_SERVER

protected:

	bool init();
	bool fini();

	DataSectionPtr				pTerrain2Defaults_;
	bool						isValid_; //True if call to init succeeded
#ifndef MF_SERVER

	BaseTerrainRenderer*		currentRenderer_;
	BasicTerrainLodController	lodController_;
	TerrainGraphicsOptions		graphicsOptions_;
	bool						wireFrame_;
	mutable TerrainVertexLodConsts *	lastVertexLodConsts_;
#ifdef EDITOR_ENABLED
	TTL2Cache					ttl2Cache_;
#endif // EDITOR_ENABLED

#endif // MF_SERVER
};


} // namespace Terrain

BW_END_NAMESPACE

#endif // TERRAIN_HPP
