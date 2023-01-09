
#include "pch.hpp"
#include "manager.hpp"
#include "terrain_settings.hpp"
#include "resmgr/auto_config.hpp"
#include "physics2/material_kinds.hpp"

#include "cstdmf/guard.hpp"


#ifndef MF_SERVER
#include "terrain2/terrain_vertex_lod_consts.hpp"

BW_BEGIN_NAMESPACE

namespace
{


/**
 *	This class implements a NULL terrain renderer.
 */
class NullTerrainRenderer : public Terrain::BaseTerrainRenderer
{
public:
	uint32 version() const { return 0; };

	void addBlock(
			Terrain::BaseRenderTerrainBlock* pBlock,
			const Matrix& transform)
	{}

	void drawAll(
		Moo::ERenderingPassType passType,
			Moo::EffectMaterialPtr pOverride,
			bool clearList)
	{}

	void drawSingle(
		Moo::ERenderingPassType passType,
		Terrain::BaseRenderTerrainBlock* pBlock,	
		const Matrix& transform, 
		Moo::EffectMaterialPtr altMat
		) { }

	void clearBlocks() { }

	bool canSeeTerrain() const { return false; };
};

BW_END_NAMESPACE

} // anonymous namespace

#endif // MF_SERVER


BW_BEGIN_NAMESPACE

// Implementation of the singleton static pointer.
BW_INIT_SINGLETON_STORAGE( Terrain::Manager );

AutoConfigString g_terrain2Settings( "system/terrain2" );

namespace Terrain
{

/**
 *	Constructor
 */
Manager::Manager() :
	pTerrain2Defaults_( NULL ),
	isValid_( false )
#ifndef MF_SERVER
	, currentRenderer_( NULL )
	, wireFrame_( false )
	, lastVertexLodConsts_( NULL )
#endif // MF_SERVER
{
	init();
}

Manager::~Manager()
{
	fini();
}

/**
 *	This method initialises any objects that require initialisation in the
 *	library.
 *
 *	@return		True if init went on without errors, false otherwise.
 */
bool Manager::init()
{
	BW_GUARD;
	MF_ASSERT( !isValid_ );
	
	// DominantTextureMap uses MaterialKinds, so make sure it's initialised
	if ( !MaterialKinds::init() )
	{
		ERROR_MSG( "Terrain::Manager::init: Unable to load material kinds data.\n" );
		return false;
	}
	instance().pTerrain2Defaults_ = BWResource::instance().openSection( g_terrain2Settings );
	if ( !instance().pTerrain2Defaults_ )
	{
		ERROR_MSG( "Terrain::Manager::init: Unable to load terrain2 settings file specified by "
			"system/terrain2: '%s'.\n", g_terrain2Settings.value().c_str() );
		return false;
	}
#ifndef MF_SERVER

	// Init graphics options
	instance().graphicsOptions_.init( instance().pTerrain2Defaults_ );

#endif // MF_SERVER

	isValid_ = true;
	return true;
}


/**
 *	This method finalises any objects that require finalises in the
 *	library, releasing resources.
 *
 *	@return		True if fini went on without errors, false otherwise.
 */
bool Manager::fini()
{
	BW_GUARD;	
#ifndef MF_SERVER

#ifdef EDITOR_ENABLED
	TerrainSettings::fini();
#endif // EDITOR_ENABLED

	delete lastVertexLodConsts_;
	lastVertexLodConsts_ = NULL;

#endif // MF_SERVER

	MaterialKinds::fini();

	isValid_ = false;

	return true;
}


#ifndef MF_SERVER


/**
 *  This method sets the current terrain renderer instance. At the moment,
 *  TerrainRenderers call this method when they are
 *  inited to set themselves as the current renderer.
 *
 *  @param newInstance      pointer to the current terrain renderer, or 
 *                          NULL to disable terrain rendering.
 */
void Manager::currentRenderer( BaseTerrainRenderer* newInstance )
{
	BW_GUARD;
	MF_ASSERT( pInstance() );
	currentRenderer_ = newInstance;
	if ( currentRenderer_ )
#if ENABLE_WATCHERS
		currentRenderer_->isVisible( !BaseTerrainRenderer::isEnabledInWatcher );
#else
		currentRenderer_->isVisible( false );
#endif
}


/**
 *  Terrain renderer instance get method.
 *
 *  @return          instance of the current terrain renderer. If no
 *                   terrain renderer has been set, it returs a pointer
 *                   to a dummy renderer that does nothing.
 */
BaseTerrainRenderer* Manager::currentRenderer()
{
	BW_GUARD;
	MF_ASSERT( pInstance() );
	if ( currentRenderer_ == NULL )
	{
		// if it's initialised, but the instance is NULL, it means that no valid
		// terrain renderer set. Return the NullTerrainRenderer to avoid a crash.
		// This can happen when rendering the terrain when the blocks haven't yet
		// loaded, which are the ones that set the terrain renderer instance.
		static NullTerrainRenderer nullRenderer;
		return &nullRenderer;
	}
	return currentRenderer_;
}

TerrainVertexLodConsts *Manager::vertexLodConsts( float blockSize ) const {
	BW_GUARD;
	if ( lastVertexLodConsts_ == NULL || lastVertexLodConsts_->blockSize != blockSize ) {
		delete lastVertexLodConsts_;
		lastVertexLodConsts_ = new TerrainVertexLodConsts( blockSize );
	}
	return lastVertexLodConsts_;
}

#endif // MF_SERVER

} // namespace Terrain

BW_END_NAMESPACE

// manager.cpp
