#include "pch.hpp"
#include "terrain_blends.hpp"

#include "cstdmf/watcher.hpp"

#include "terrain_texture_layer2.hpp"
#include "terrain_renderer2.hpp"
#include "terrain_block2.hpp"
#include "../terrain_settings.hpp"
#include "dominant_texture_map2.hpp"

#ifdef EDITOR_ENABLED
#include "editor_terrain_block2.hpp"
#endif


BW_BEGIN_NAMESPACE

using namespace Terrain;

namespace
{
	// These keep track of how many total blocks have blends in memory or are
	// queued to be loaded.	
	static uint32	s_numBlendsLoaded	= 0;
	static uint32	s_numBlendsQueued	= 0;
}

// default constructor for combined layer
CombinedLayer::CombinedLayer()
: width_( 0 ), height_( 0 ), pBlendTexture_( NULL ), smallBlended_(false)
{
}

TerrainBlends::TerrainBlends()
{
	BW_GUARD;
	static bool watchAdded = false;
	if ( !watchAdded)
	{
		MF_WATCH( "Render/Terrain/Terrain2/numBlendsLoaded", s_numBlendsLoaded );
		MF_WATCH( "Render/Terrain/Terrain2/numBlendsQueued", s_numBlendsQueued );
		watchAdded = true;
	}

	s_numBlendsLoaded++;
}

TerrainBlends::~TerrainBlends()
{
	s_numBlendsLoaded--;
}

/**
*	This method loads and initialises rendered (blend) textures.
*
*	@param		filename	The full path of the block to load textures for.
*	@returns	True, if load/creation was successful.
*/
bool TerrainBlends::init( TerrainBlock2& owner, bool loadBumpMaps )
{
	BW_GUARD;
	DataSectionPtr pTerrain = BWResource::openSection( owner.getFileName() );
	if ( !pTerrain )
	{
		ERROR_MSG( "TerrainBlends::init: "
			"Couldn't open terrain section %s\n", owner.getFileName() );
		return false;
	}

	blockSize_ = owner.blockSize();

	// Create the texture layers
	BW::vector<DataSectionPtr> layerSections;
	pTerrain->openSections( "layer", layerSections );
	textureLayers_.reserve( layerSections.size() );

	for (uint32 i = 0; i < layerSections.size(); i++)
	{
		DataSectionPtr pLayerData = layerSections[i];
		TerrainTextureLayerPtr pLayer = new TerrainTextureLayer2(owner, loadBumpMaps);

		if (pLayer->load( pLayerData ))
		{
			textureLayers_.push_back( pLayer );
		}
		else
		{
			ERROR_MSG( "TerrainBlends::init: "
				"Aborting load due to error in texture layer %d\n", i );
			return false;
		}
	}

	if (textureLayers_.size() != 0)
	{
		// If the editor could not load a dominant map, it can regenerate one.
		// Note passed in type is a pointer to smart pointer, so we can pass in 
		// NULL if we don't want a map generated.
		DominantTextureMap2Ptr* dominantTextureMapPP = NULL;
#ifdef EDITOR_ENABLED
		dominantTextureMapPP = 
			owner.dominantTextureMap2().exists() ?
			NULL : &(owner.dominantTextureMap2());
#endif

		createCombinedLayers( true, dominantTextureMapPP );

		for (size_t i = 0; i < textureLayers_.size(); ++i)
		{
			TerrainTextureLayer2 *layer =
				((TerrainTextureLayer2 *)textureLayers_[i].getObject());
			layer->onLoaded();
		}
	}

	return true;
}

void TerrainBlends::createCombinedLayers( bool compressTextures,
										    DominantTextureMap2Ptr* newDominantTexture )
{
	BW_GUARD;
	// Copy layers to mess around with them
	TextureLayers tempTextureLayers = textureLayers_;

	// Empty combined layers.
	combinedLayers_.clear();
	// Reserve enough space for 1 combined for every 4 layers (at least 1).
	combinedLayers_.reserve( ( tempTextureLayers.size() >> 2 ) + 1 );

	// Re-create.
	while (tempTextureLayers.size())
	{
		TerrainTextureLayerPtr layers[4];
		uint32 layerIndex = 0;
		CombinedLayer cl;
		cl.height_ = tempTextureLayers[0]->height();
		cl.width_ = tempTextureLayers[0]->width();
		for (int i = 0; (i < (signed)tempTextureLayers.size()) &&
			(layerIndex != 4); i++)
		{
			if (tempTextureLayers[i]->height() == cl.height_ &&
				tempTextureLayers[i]->width() == cl.width_)
			{
				layers[layerIndex++] = tempTextureLayers[i];
				cl.textureLayers_.push_back( tempTextureLayers[i] );
				tempTextureLayers.erase( tempTextureLayers.begin() + i );
				--i;
			}
		}

		cl.pBlendTexture_ = 
			TerrainTextureLayer::createBlendTexture
			( 
			layers[0], layers[1], layers[2], layers[3], 
			TerrainRenderer2::instance()->supportSmallBlend(),
			compressTextures ,
			&cl.smallBlended_
			);

		if (!cl.pBlendTexture_)
		{
			ERROR_MSG( "TerrainBlends::createCombinedLayers: "
				"Error creating blend texture (see log)." );
		}
		else
		{
			combinedLayers_.push_back( cl );
		}
	}

 	if ( newDominantTexture )
 	{
 		// Update the dominant texture map:
 		*newDominantTexture = new DominantTextureMap2( blockSize_, textureLayers_ );
 	}
}

TerrainBlendsResource::TerrainBlendsResource(TerrainBlock2& owner, bool loadBumpMaps) :
	owner_(owner), loadBumpMaps_(loadBumpMaps)
{
#ifdef EDITOR_ENABLED
	// override stream type in editor
	streamType_ = RST_Syncronous;
#endif
}


void TerrainBlendsResource::stream( ResourceStreamType streamType )
{
	// Remember how we loaded this
	streamType_ = streamType;
	const ResourceState rState = getState();

	if ( required_ == RR_No && rState == RS_Loaded )
	{
		unload();
	}
	else if ( required_ == RR_Yes && rState == RS_Unloaded )
	{
		//-- Note: Editor never unloads bump maps. It only can disable actual drawing but not loading.
#ifndef EDITOR_ENABLED
		loadBumpMaps_ = owner_.settings()->bumpMapping();
#endif //-- EDITOR_ENABLED

		// load object
		MF_ASSERT( object_ == NULL );

		if ( streamType == RST_Asyncronous )
		{
			startAsyncTask();
		}
		else
		{
			load();
		}
	}
	//-- Note: Editor never unloads bump maps. It only can disable actual drawing but not loading.
#ifndef EDITOR_ENABLED
	else if ( required_ == RR_Yes && rState == RS_Loaded )
	{
		if (loadBumpMaps_ != owner_.settings()->bumpMapping())
		{
			//-- 1. remember what we did in the last time.
			loadBumpMaps_ = owner_.settings()->bumpMapping();

			//-- 2. unload terrain blends.
			unload();

			//-- Notice: terrain blends will be loaded again on the next frame with desired
			//--		 loadBumpMaps_ status.
		}
	}
#endif //-- EDITOR_ENABLED
}

bool TerrainBlendsResource::load()
{
	BW_GUARD;
	// Create and assign to temporary
	ObjectTypePtr tempObj = new TerrainBlends();
	bool status = tempObj->init( owner_, loadBumpMaps_ );

	// Assign whole object without any partial initialisation (this can be
	// in thread).
	if ( status )
	{
		object_ = tempObj;
	}

	return status;
}

void TerrainBlendsResource::startAsyncTask()
{
	BW_GUARD;

	// Call base function
	Resource<ObjectType>::startAsyncTask();
}

/** 
 * This is called on the main thread just before we start loading
 * on the background thread. 
 */
void TerrainBlendsResource::preAsyncLoad()
{
	BW_GUARD;
	owner_.incRef();
}

/** 
 * This is called on the main thread once the background loading
 * task has been completed.
 */
void TerrainBlendsResource::postAsyncLoad()
{
	BW_GUARD;
	owner_.decRef();
}

#ifdef EDITOR_ENABLED

bool TerrainBlendsResource::rebuild(bool compressTextures, 
								  DominantTextureMap2Ptr* newDominantTexture )
{
	BW_GUARD;
	object_->createCombinedLayers(compressTextures, newDominantTexture );
	return true;
}

void TerrainBlendsResource::unload()
{
	BW_GUARD;
	if ( getState() != RS_Unloaded )
	{
		object_->combinedLayers_.clear();
	}
}

ResourceState TerrainBlendsResource::getState() const
{
	return ( object_->combinedLayers_.size() > 0 ) ? RS_Loaded : RS_Unloaded;
}

TerrainBlendsResource::ObjectTypePtr TerrainBlendsResource::getObject()
{
	return object_;
}

#endif // EDITOR_ENABLED

BW_END_NAMESPACE

// terrain_blends.cpp
