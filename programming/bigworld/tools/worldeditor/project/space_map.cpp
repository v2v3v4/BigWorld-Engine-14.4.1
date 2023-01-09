#include "pch.hpp"
#include "worldeditor/misc/progress_bar_helper.hpp"
#include "worldeditor/project/space_map.hpp"
#include "worldeditor/project/space_helpers.hpp"
#include "worldeditor/project/space_map_debug.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "worldeditor/world/editor_chunk_cache.hpp"
#include "worldeditor/world/editor_chunk_thumbnail_cache.hpp"
#include "chunk/chunk_space.hpp"
#include "chunk/chunk_manager.hpp"
#include "moo/base_texture.hpp"
#include "moo/texture_manager.hpp"
#include "moo/vertex_formats.hpp"
#include "moo/texture_compressor.hpp"
#include "moo/effect_material.hpp"
#include "moo/custom_mesh.hpp"
#include "common/material_utility.hpp"
#include "cstdmf/debug.hpp"


DECLARE_DEBUG_COMPONENT2( "WorldEditor", 0 )

BW_BEGIN_NAMESPACE

SpaceMap* SpaceMap::s_instance_ = NULL;

namespace {
	DataSectionPtr localSettingsDataSection( const BW::string& spaceName )
	{
		return BWResource::openSection( 
			spaceName + "/" + SPACE_LOCAL_SETTING_FILE_NAME, true );
	}
	
	DataSectionPtr spaceSettingsDataSection( const BW::string& spaceName )
	{
		return BWResource::openSection( 
			spaceName + "/" + SPACE_SETTING_FILE_NAME, true );
	}
}


SpaceMap& SpaceMap::instance()
{
	if (s_instance_ == NULL)
	{
		BW_GUARD;

		s_instance_ = new SpaceMap();
	}
	return *s_instance_;
}


/*static*/ void SpaceMap::deleteInstance()
{ 
	BW_GUARD;

	bw_safe_delete(s_instance_);

	SpaceMapDebug::deleteInstance();
}


SpaceMap::SpaceMap():
	nTexturesPerFrame_(1),
	nPhotosPerFrame_(1),	
	material_( NULL ),
	cacheNeedsRetrieval_( true ),
	modifiedThumbnails_(20),
    deviceReset_(false),
	mark_(0)
{
	BW_GUARD;
	map_ = new Moo::RenderTarget( "spaceMap" );
	material_ = new Moo::EffectMaterial();
	material_->load( BWResource::openSection( "resources/materials/space_map.mfm" ));
	MaterialUtility::viewTechnique( material_, "spaceMap" );	
}


SpaceMap::~SpaceMap()
{
}


void SpaceMap::setTexture( uint8 textureStage )
{
	BW_GUARD;

	Moo::rc().setTexture( textureStage, map_->pTexture() );
}


DX::BaseTexture *SpaceMap::texture()
{
	BW_GUARD;

	return map_->pTexture();
}


void SpaceMap::spaceInformation( const SpaceInformation& info )
{
	BW_GUARD;

	if ( info != info_ )
	{
		info_ = info;

		this->timestampCache().spaceInformation( info_ );
		SpaceMapDebug::instance().spaceInformation( info_ );
		allThumbnails_.spaceInformation( info_ );
		modifiedThumbnails_.spaceInformation( info_ );
		dirtyThumbnails_.spaceInformation( info_ );
		updatedThumbnails_.spaceInformation( info_ );

		dirtyThumbnails_.reset();
		updatedThumbnails_.reset();

		this->load();
	}
}


bool SpaceMap::init( DataSectionPtr pSection )
{
	return true;
}


void SpaceMap::createRenderTarget()
{
	BW_GUARD;

	if ( !map_->pTexture() )
	{
		int width = 2048;
		int height = 2048;

		DataSectionPtr pSpaceSettings = spaceSettingsDataSection( info_.spaceName_ );
		if (pSpaceSettings)
		{
			width = pSpaceSettings->readInt( "projectViewMapResolution/width", 2048 );
			height = pSpaceSettings->readInt( "projectViewMapResolution/height", 2048 );
		}

		map_->create( width, height );
        map_->clearOnRecreate(true, 0xffffffff);
	}
}


void SpaceMap::createTextures()
{
	BW_GUARD;

	this->createRenderTarget();
    if (!deviceReset_)
	{
		this->loadTemporaryCache();
        cacheNeedsRetrieval_ = false;
	}
}


void SpaceMap::releaseTextures()
{
	BW_GUARD;

	if ( map_->pTexture() != NULL )
	{
        if (!deviceReset_)
		    this->saveTemporaryCache();
		map_->release();
	}
}


void SpaceMap::createUnmanagedObjects()
{
	BW_GUARD;

	createTextures();
}


void SpaceMap::deleteUnmanagedObjects()
{
	BW_GUARD;

    deviceReset_ = Moo::rc().device()->TestCooperativeLevel() != D3D_OK;
	releaseTextures();
}


bool SpaceMap::recreateForD3DExDevice() const
{
	return true;
}


void SpaceMap::update( float dTime, bool fullUpdate /*= false*/ )
{
	BW_GUARD;

	static enum
	{
		PHOTOGRAPH_DIRTY_THUMBNAIL,
		SWAPIN_UPDATED_THUMBNAIL,
		SWAPIN_MODIFIED_THUMBNAIL,
		INSPECT_TILES
	}
	currentTask = PHOTOGRAPH_DIRTY_THUMBNAIL;

	// Rendering is done here, so we need to have a valid device:
	if (Moo::rc().device()->TestCooperativeLevel() != D3D_OK)
	{
		return;
	}

    // If the device was reset then the best we can do is to redraw everything.
    if (deviceReset_)
    {
        recreateAfterReset();
        deviceReset_ = false;
    }
    // If the window was resized then load the cached image (this is not 
    // possible on a device reset).
	else if (cacheNeedsRetrieval_)
    {
		this->loadTemporaryCache();
        cacheNeedsRetrieval_ = false;
    }

	WorldManager::instance().markChunks();
	ChunkManager::instance().tick( dTime );

	WorldManager::instance().checkMemoryLoad();

	// Now if for whatever reason the cacheNeedsRetrieval_ is still true,
	// make sure we don't do anything this frame!  It could overwrite useful
	// data.
	if (cacheNeedsRetrieval_)
	{
		return;
	}

	// Any dirty chunks should be photographed first.  they should be in memory
	if (fullUpdate || currentTask == PHOTOGRAPH_DIRTY_THUMBNAIL)
	{
		currentTask = SWAPIN_UPDATED_THUMBNAIL;
		updateChunksThumbnail( nPhotosPerFrame_, dirtyThumbnails_ );
		if (!fullUpdate)
		{
			return;
		}
	}
	// Any recently photographed thumbnails get added onto the map.
	if (fullUpdate || currentTask == SWAPIN_UPDATED_THUMBNAIL)
	{
		currentTask = SWAPIN_MODIFIED_THUMBNAIL;
		swapInTextures( nTexturesPerFrame_, updatedThumbnails_, NULL, 0xffffffff );
		if (!fullUpdate)
		{
			return;
		}
	}

	// Then look on disk to see if any thumbnails have been saved to disk
	// but not written into the space map as recently. (should not happen)
	if (fullUpdate || currentTask == SWAPIN_MODIFIED_THUMBNAIL)
	{
		currentTask = INSPECT_TILES;
		swapInTextures( nTexturesPerFrame_, modifiedThumbnails_, NULL, 0xffffffff );
		if (!fullUpdate)
		{
			return;
		}
	}

	if (fullUpdate || currentTask == INSPECT_TILES)
	{
		currentTask = PHOTOGRAPH_DIRTY_THUMBNAIL;
		if (dirtyThumbnails_.size() == 0)
		{
			// Inspect tiles finds chunks that have missing thumbnails (should not happen)
			if (!inspectTiles( 1, allThumbnails_ ))
			{
				allThumbnails_.reset();
			}
			if (!fullUpdate)
			{
				return;
			}
		}
	}
}


/**
 *	This method looks at all the tiles provided by the ChunkWalker,
 *	and checks for thumbnail data section existence.
 *
 *	Any missing maps go into the missingThumbnails cache.
 */
bool SpaceMap::inspectTiles( uint32 n, IChunkWalker& chunkWalker )
{
	BW_GUARD;

	int16 gridX, gridZ;
	BW::string mapName;
	BW::string pathName, chunkName;
	uint32 num = n;

	GeometryMapping* dirMap = WorldManager::instance().geometryMapping();
	pathName= dirMap->path();

	while ( num > 0 )
	{
		if (!chunkWalker.nextTile(chunkName, gridX, gridZ))
			break;

		if (!dirtyThumbnails_.added( gridX, gridZ ))
		{
			Chunk * pChunk = 
				ChunkManager::instance().findChunkByName(chunkName, dirMap, false);

			if (pChunk && pChunk->isBound())
			{
				bool ectcHasThumbnail = 
					EditorChunkThumbnailCache::instance( *pChunk ).hasThumbnail();
				if (!ectcHasThumbnail)
				{
					dirtyThumbnails_.add( gridX, gridZ );
				}
			}
			else if (!thumbnailExists( pathName, chunkName ))
			{
				dirtyThumbnails_.add( gridX, gridZ );
			}
			else
			{
				// Black - considered, but nothing to do.
				SpaceMapDebug::instance().onConsidered( gridX, gridZ, 0x00000000 );
			}
		}
		num--;
	}

	return (num<n);
}


/**
 *	This method swaps n textures into the large bitmap, given
 *	a chunk walker.
 */
bool SpaceMap::swapInTextures
( 
    uint32              n, 
    IChunkWalker        &chunkWalker, 
    CacheChunkWalker    *removeCache,
	uint32				hintColour
)
{
	BW_GUARD;

	// Now if for whatever reason the cacheNeedsRetrieval_ is true,
	// make sure we don't actually write anything into the space map,
	// It could overwrite useful data.
	if (cacheNeedsRetrieval_)
		return false;

	BW::string mapName;
	BW::string pathName, chunkName;
	int16 gridX, gridZ;

	GeometryMapping* dirMap = WorldManager::instance().geometryMapping();
	pathName= dirMap->path();

	uint32 num = n;

	Moo::rc().beginScene();
	if (Moo::rc().mixedVertexProcessing())
		Moo::rc().device()->SetSoftwareVertexProcessing( TRUE );

	bool didOne = false;
	if ( map_->pTexture() && map_->push() )
	{
		if (material_->begin())
		{
			for ( uint32 i=0; i<material_->numPasses(); i++ )
			{
				material_->beginPass(i);

				while ( num > 0 )
				{
					if (!chunkWalker.nextTile( chunkName, gridX, gridZ ))
						break;

					bool triedLoad = false;
					Moo::BaseTexturePtr pTexture = this->thumbnail(chunkName, &triedLoad);

					if ( pTexture )
					{
						Moo::rc().setTexture(0,pTexture->pTexture());
						this->drawGridSquare( gridX, gridZ );
						SpaceMapDebug::instance().onDraw(gridX, gridZ, hintColour);						
						++mark_;
						didOne = true;
					}
					else
					{
						Moo::rc().setTexture(0,NULL);
						if (triedLoad)
						{
							ERROR_MSG( "SpaceMap::swapInTextures(...) - Could not load bmp %s, even though the resource exists\n", chunkName.c_str() );
						}
						else
						{
							ERROR_MSG( "SpaceMap::swapInTextures(...) - Could not load cdata file %s%s.cdata\n", pathName.c_str(), chunkName.c_str() );
						}
					}
					if (removeCache != NULL)
						removeCache->erase( gridX, gridZ );				

					--num;
				}
				material_->endPass();
			}
			material_->end();
		}

		map_->pop();
	}

    Moo::rc().endScene();

	return didOne;
}


/**
 *	This method takes photographs of all chunks in our dirtyThumbnails_ list,
 *	possibly shows a progress bar for the entire operation,
 *	and draws them onto the space map (done by photographChunks).
 */
void SpaceMap::regenerateAllDirty(bool showProgress)
{
	BW_GUARD;

	ProgressBarTask       *genHMTask = NULL;

    try
    { 
        size_t numOperationsDone = 0; // Number of steps calculated so far.
        size_t numOperations     = 0; // Number of steps in total.

        // Create a progress indicator if requested:
        if (showProgress)
        {
            float numOperations = (float)dirtyThumbnails_.size();

            genHMTask =
                new ProgressBarTask
                ( 
                    LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/PROJECT/SPACE_MAP/UPDATE_PROJECT_VIEW"), 
                    (float)(numOperations), false
                );
		}

        // Photograph dirty thumbnails.  The photograph chunks method
		// also swaps them into the space map.
        while (dirtyThumbnails_.size() != 0)
        {
            updateChunksThumbnail( 1, dirtyThumbnails_ );            
            if (genHMTask != NULL)
            {
                genHMTask->step();
                ++numOperationsDone;
            }
        }

        // Update the progress indicator to look as though everything was done:
        if (genHMTask != NULL)
        {
            while (numOperationsDone < numOperations)
            {
                genHMTask->step();
                ++numOperationsDone;
            }
        }

        // Cleanup:
        bw_safe_delete(genHMTask);

        // Save the result:
        save();
    }
    catch (...)
    {
        bw_safe_delete(genHMTask); 
        throw;
    }
}


/**
 *	This returns a token that is incremented each time the space map changes
 *	in some way.
 *
 *  @returns		An integer that can be used to determine whether the
 *					space map has changed.
 */
uint32 SpaceMap::mark() const
{
	return mark_;
}


bool SpaceMap::swapInTextures( Chunk* chunk )
{
	BW_GUARD;

	if (cacheNeedsRetrieval_)
		return false;

	if( !chunk->isOutsideChunk() )
		return false;

	bool success = false;

	BW::string mapName;
	BW::string pathName, chunkName;
	int16 gridX, gridZ;

	GeometryMapping* dirMap = WorldManager::instance().geometryMapping();
	pathName= dirMap->path();

	Moo::rc().beginScene();
	if (Moo::rc().mixedVertexProcessing())
		Moo::rc().device()->SetSoftwareVertexProcessing( TRUE );

	if ( map_->pTexture() && map_->push() )
	{
		if (material_->begin())
		{
			for ( uint32 i=0; i<material_->numPasses(); i++ )
			{
				if( !WorldManager::instance().geometryMapping()->gridFromChunkName( chunk->identifier(), gridX, gridZ ) )
					break;
				chunkID( chunkName, gridX, gridZ );

				if (!chunkName.empty())
				{
					material_->beginPass(i);

					bool triedLoad = false;
					Moo::BaseTexturePtr pTexture = thumbnail(chunk->identifier(), &triedLoad);

					if ( pTexture != NULL )
					{
						Moo::rc().setTexture(0,pTexture->pTexture());
						this->drawGridSquare( gridX, gridZ );
						SpaceMapDebug::instance().onDraw(gridX, gridZ, 0x000000ff);				
						++mark_;
						success = true;				
					}
					else
					{
						Moo::rc().setTexture(0,NULL);
						if (triedLoad)
							ERROR_MSG( "SpaceMap::swapInTextures(chunk) - Could not load bmp %s, even though the resource exists\n", mapName.c_str() );
						else
							ERROR_MSG( "SpaceMap::swapInTextures(chunk) - Could not load cdata file %s%s.cdata\n", pathName.c_str(), chunkName.c_str() );
					}

					material_->endPass();
				}
				else
				{
					WARNING_MSG( "SpaceMap::swapInTextures: Grid coords for chunk '%s' not valid (%d,%d). Skipping.\n",
						chunk->identifier().c_str(), gridX, gridZ );
				}
			}
			material_->end();
		}

		map_->pop();
	}

	Moo::rc().endScene();	

	return success;	
}


/**
 *	This method recalculate the next n appropriate chunks and saves
 *	their thumbnails.
 *
 *	For each chunk photographed, the tile is swapped in immediately.
 */
bool SpaceMap::updateChunksThumbnail( uint32 n, IChunkWalker& chunkWalker )
{
	BW_GUARD;

	CacheChunkWalker ccw;

	BW::string chunkName;
	int16 gridX, gridZ;

	uint32 numCalculated = 0;
	for (; numCalculated < n; ++numCalculated)
	{
		if (!chunkWalker.nextTile( chunkName, gridX, gridZ ))
		{
			break;
		}

		WorldManager::instance().recalcThumbnail( chunkName, true );

		ccw.add( gridX, gridZ );
		++mark_;
	}

	if (numCalculated > 0)
	{
		swapInTextures( numCalculated, ccw, NULL, 0xffffffff );
		return true;
	}
	return false;
}


/**
 *	This method calcualtes the screen-space area
 *	for the given grid square, and calls drawTile with those
 *	values.
 */
void SpaceMap::drawGridSquare( int16 gridX, int16 gridZ )
{
	BW_GUARD;

	if (cacheNeedsRetrieval_)
		return;

	uint16 biasedX,biasedZ;
	biasGrid(info_.localToWorld_,gridX,gridZ,biasedX,biasedZ);	

	float dx = Moo::rc().screenWidth() / float(info_.gridWidth_);
	float dy = Moo::rc().screenHeight() / float(info_.gridHeight_);

	float x = (float(biasedX) / float(info_.gridWidth_)) * (Moo::rc().screenWidth());
	float y = (float(biasedZ) / float(info_.gridHeight_))* (Moo::rc().screenHeight());

	drawTile( x, y, dx, dy );

	this->timestampCache().touch( gridX, gridZ );
}


/**
 *	Draws a single tile ( i.e quad ) in screen space.
 *
 *	Pass in non-texel aligned screen space coordinates.
 */
void SpaceMap::drawTile( float x, float y, float dx, float dy )
{
	BW_GUARD;

	static CustomMesh< Moo::VertexTUV > s_gridSquare( D3DPT_TRIANGLEFAN );

	Vector2 uvStart( 0.f, 0.f );
	Vector2 uvEnd( 1.f, 1.f );

	if ( dy < 0.f )
	{
		//handle case of flipped textures.
		uvEnd.y = 0.f;
		uvStart.y = 1.f;
		y += dy;
		dy = -dy;
	}

	//screen/texel alignment	
	x -= 0.5f;
	y -= 0.5f;	

	s_gridSquare.clear();
	Moo::VertexTUV v;
	v.pos_.set(x,y,1.f,1.f);
	v.uv_.set(uvStart.x,uvEnd.y);
	s_gridSquare.push_back(v);

	v.pos_.set(x+dx,y,1.f,1.f);
	v.uv_.set(uvEnd.x,uvEnd.y);
	s_gridSquare.push_back(v);

	v.pos_.set(x+dx,y+dy,1.f,1.f);
	v.uv_.set(uvEnd.x,uvStart.y);
	s_gridSquare.push_back(v);

	v.pos_.set(x,y+dy,1.f,1.f);
	v.uv_.set(uvStart.x,uvStart.y);
	s_gridSquare.push_back(v);

	s_gridSquare.drawEffect();	
}


bool SpaceMap::cacheName( BW::string& ret )
{
	BW_GUARD;

	GeometryMapping* dirMap = WorldManager::instance().geometryMapping();
	if ( dirMap )
	{
		ret = dirMap->path() + "space";
		return true;
	}
	return false;
}


void SpaceMap::load()
{
	BW_GUARD;

	CWaitCursor wait;

	BW::string name;
	if ( !cacheName( name ) )
		return;

	// Ensure that the render target is ok.
	this->createRenderTarget();

	Moo::rc().beginScene();
	if (Moo::rc().mixedVertexProcessing())
		Moo::rc().device()->SetSoftwareVertexProcessing( TRUE );

	// First load the cache texture.
	if ( map_->pTexture() && map_->push() )
	{
		if (material_->begin())
		{
			for ( uint32 i=0; i<material_->numPasses(); i++ )
			{
				material_->beginPass(i);
				Moo::rc().setTexture(0,NULL);

				BW::string mapName = name + ".thumbnail.dds";

				if (BWResource::openSection(mapName,false))
				{
					Moo::BaseTexturePtr pTexture = Moo::TextureManager::instance()->get(
						mapName, true, false, true, "texture/project" );

					if ( pTexture )
					{
						Moo::rc().setTexture(0,pTexture->pTexture());
					}
					++mark_;
				}

				this->drawTile( 0.f, Moo::rc().screenHeight(), Moo::rc().screenWidth(), -Moo::rc().screenHeight() );

				material_->endPass();
			}
			material_->end();
		}
		map_->pop();
	}

	Moo::rc().endScene();

	cacheNeedsRetrieval_ = false;

	//load the thumbnail modification date cache.
	this->timestampCache().load();

	// Remember where we were up to in the traversals from localsettings.
	DataSectionPtr pLocalSettings = localSettingsDataSection( info_.spaceName_ );
	if (pLocalSettings)
	{
		DataSectionPtr pDS = pLocalSettings->openSection( "spaceMap", true );
		if (pDS)
		{
			modifiedThumbnails_.load( pDS );
			allThumbnails_.load( pDS );
		}
	}
}


void SpaceMap::save()
{
	BW_GUARD;

	if (cacheNeedsRetrieval_)
		return;	

	if ( !map_->pTexture() )
	{
		return;
	}

	BW::string name;
	if (!cacheName( name ))
		return;

	CWaitCursor wait;

	BW::string mapName = name + ".thumbnail.dds";

	//save the thumbnail modification date cache.
	this->timestampCache().save();

	// Save where the traversals were up to in localsettings.
	DataSectionPtr pLocalSettings = localSettingsDataSection( info_.spaceName_ );
	if (pLocalSettings)
	{
		DataSectionPtr pDS = pLocalSettings->openSection( "spaceMap", true );
		if (pDS)
		{
			modifiedThumbnails_.save( pDS );
			allThumbnails_.save( pDS );
		}

		pLocalSettings->save();
	}
	
	//Then save out the texture
	Moo::TextureCompressor tc( static_cast<DX::Texture*>(map_->pTexture()) );
	tc.save( mapName, D3DFMT_DXT1, 1 );
}


/**
 *	Load temporary cached map off disk, and apply to render target.
 */
void SpaceMap::loadTemporaryCache()
{
	BW_GUARD;

	CWaitCursor wait;

	BW::string name;
	if ( !cacheName( name ) )
		return;

	Moo::rc().beginScene();
	if (Moo::rc().mixedVertexProcessing())
		Moo::rc().device()->SetSoftwareVertexProcessing( TRUE );

	//First load the cache texture
	if ( map_->pTexture() && map_->push() )
	{
		if (material_->begin())
		{
			for ( uint32 i=0; i<material_->numPasses(); i++ )
			{
				material_->beginPass(i);
				Moo::rc().setTexture(0,NULL);

				BW::string mapName = name + ".temp_thumbnail.dds";

				//try retrieving our temporary cache
				if (BWResource::openSection(mapName,false))
				{
					Moo::BaseTexturePtr pTexture = Moo::TextureManager::instance()->get(
							mapName, true, false, true, "texture/project" );

					if ( pTexture )
					{
						Moo::rc().setTexture(0,pTexture->pTexture());
					}

					BW::wstring wmapName;
					bw_utf8tow( BWResource::resolveFilename( mapName ), wmapName );
					::DeleteFile( wmapName.c_str() );

					++mark_;
				}

				this->drawTile( 0.f, Moo::rc().screenHeight(), Moo::rc().screenWidth(), -Moo::rc().screenHeight() );
				material_->endPass();
			}
			material_->end();
		}
		map_->pop();
	}

	Moo::rc().endScene();

	//And load the temporary timestamps too
	this->timestampCache().loadTemporaryCopy();

	cacheNeedsRetrieval_ = false;
}


/**
 *	Save cached large space map temporarily to disk, because
 *	we are about to recreate the underlying texture.
 */
void SpaceMap::saveTemporaryCache()
{
	BW_GUARD;

	if ( cacheNeedsRetrieval_ )
		return;	

	CWaitCursor wait;

	BW::string name;
	if (!cacheName( name ))
		return;
	BW::string tempMapName = name + ".temp_thumbnail.dds";

	//Then save out the texture
	Moo::TextureCompressor tc( static_cast<DX::Texture*>(map_->pTexture()) );
	tc.save( tempMapName, D3DFMT_DXT1, 1 );

	//And save out the timestamps too
	this->timestampCache().saveTemporaryCopy();
}


void SpaceMap::dirtyThumbnail( Chunk* pChunk )
{
	BW_GUARD;

	// Add to list of dirty thumbnails, so we eventually photograph the 
    // chunk.
    // Be careful not to re-add an existing chunk.
    if (!dirtyThumbnails_.added( pChunk ))
		dirtyThumbnails_.add( pChunk );
}


void SpaceMap::chunkThumbnailUpdated( Chunk* pChunk )
{
	BW_GUARD;

	//TODO : don't necessarily need to swap it in right now.
	//we can just put it on the updatedThumbnails list instead,
	//and when the space map is next visible, or saved, we call
	//swapInTextures for each of these updated ones.

	//HOWEVER doing it right now gives the space map a greater
	//chance of being up-to-date when the user first looks at it,
	//and in fact while the chunk is loaded we may as well use it
	//now.

	// Remove from list of dirty thumbnails, so we don't re-photograph it.
	dirtyThumbnails_.erase( pChunk );

	if (!swapInTextures( pChunk ))
	{
		if (!updatedThumbnails_.added( pChunk ))
			updatedThumbnails_.add( pChunk );
	}
}


void SpaceMap::invalidateAllChunks()
{
	BW_GUARD;

	CWaitCursor wait;
	allThumbnails_.reset();

	int16 gridX, gridZ;
	BW::string mapName;
	BW::string pathName, chunkName;	

	GeometryMapping* dirMap = WorldManager::instance().geometryMapping();
	pathName= dirMap->path();

	while ( 1 )
	{
		if (!allThumbnails_.nextTile(chunkName, gridX, gridZ))
			break;

		DataSectionPtr pSection = BWResource::openSection( pathName + chunkName + ".cdata", false );
		if ( pSection )
		{
			pSection->delChild( "thumbnail.dds" );
			pSection->save();
		}		
	}	

	allThumbnails_.reset();
}


void SpaceMap::recreateAfterReset()
{
	BW_GUARD;

    allThumbnails_.reset();	

	this->timestampCache().spaceInformation( info_ );
	SpaceMapDebug::instance().spaceInformation( info_ );
	allThumbnails_.spaceInformation( info_ );
	modifiedThumbnails_.spaceInformation( info_ );

	this->load();
}

void SpaceMap::spaceExpanded()
{
	return timestampCache_.deleteCache();
}

/**
 *	This gets either the cached, in-memory thumbnail or the thumbnail off disk.
 *	If neither of these exists then it returns NULL.
 *
 *  @param chunk		The chunk to get the thumbnail for.
 *  @param triedLoad	If this is not null then it will be set to true if
 *						the thumbnail should have existed, but didn't.
 *	@return				The thumbnail for the chunk.
 */
Moo::BaseTexturePtr SpaceMap::thumbnail(const BW::string& chunkName, bool *triedLoad) const
{
	BW_GUARD;

	Moo::BaseTexturePtr result;

	if (triedLoad != NULL) *triedLoad = false;

	// If the chunk is in memory get the in-memory thumbnail:
	GeometryMapping* dirMap = WorldManager::instance().geometryMapping();
	Chunk *chunk = 
		ChunkManager::instance().findChunkByName(chunkName, dirMap, false);
	if (chunk && chunk->loaded())
	{
		result = EditorChunkThumbnailCache::instance(*chunk).thumbnail();
	}

	// If not yet in memory then load the thumbnail from disk:
	if (result == NULL)
	{			
		BW::string pathName= dirMap->path();
		if (thumbnailExists(pathName, chunkName))
		{
			BW::string mapName = 
				thumbnailFilename(pathName, chunkName);
			result = 
				Moo::TextureManager::instance()->get(mapName, true, false, true, "texture/project");
			if (triedLoad != NULL) *triedLoad = true;
			if (!result)
			{
				//There was a bug that put the thumbnail.dds files as a child
				//of the thumbnail.dds section,
				//i.e. *.cdata/thumbnail.dds/thumbnail.dds instead of 
				//*.cdata/thumbnail.dds.
				//Here we try to load these as well.
				mapName = mapName + "/thumbnail.dds";
				result = Moo::TextureManager::instance()->get(mapName, true, false, true, "texture/project");
			}
		}		
	}

	return result;
}


/**
 *  This method just returns the SpaceMapTimestampCache instance.
 */
SpaceMapTimestampCache & SpaceMap::timestampCache()
{
	return timestampCache_;
}

BW_END_NAMESPACE


