#include "pch.hpp"
#include "terrain/terrain2/editor_terrain_block2.hpp"
#include "terrain/terrain2/horizon_shadow_map2.hpp"
#include "terrain/terrain2/terrain_texture_layer2.hpp"
#include "terrain/terrain2/terrain_hole_map2.hpp"
#include "terrain/terrain2/terrain_height_map2.hpp"
#include "terrain/terrain2/terrain_ao_map2.hpp"
#include "terrain/terrain2/terrain_normal_map2.hpp"
#include "terrain/terrain2/dominant_texture_map2.hpp"
#include "terrain/terrain2/terrain_photographer.hpp"
#include "terrain/terrain2/terrain_lod_map2.hpp"
#include "terrain/terrain2/terrain_renderer2.hpp"
#include "terrain/terrain_data.hpp"
#include "resmgr/data_section_census.hpp"
#include "resmgr/bin_section.hpp"
#include "resmgr/xml_section.hpp"
#include "resmgr/zip_section.hpp"
#include "resmgr/multi_file_system.hpp"
#include <limits>


DECLARE_DEBUG_COMPONENT2( "Moo", 0 );


#ifdef EDITOR_ENABLED


BW_BEGIN_NAMESPACE

using namespace Terrain;

namespace
{
	// Frame mark used to prevent rebuildCombinedLayers to be called more than
	// once per frame.
	uint32 s_lastBlendBuildMark = 0;

	// Timestamp to keep track of the last time we rebuilt layers.
	uint64 s_lastBlendBuildTime = 0;
}


// Frame timestamp to prevent rebuildCombinedLayers to be called more than once
// per frame.
/*static*/ uint32 EditorTerrainBlock2::s_nextBlendBuildMark_ = 0;

// Interval between each layer rebuild, in milliseconds.
/*static*/ uint32 EditorTerrainBlock2::s_blendBuildInterval_ = 100;


EditorTerrainBlock2::EditorTerrainBlock2(TerrainSettingsPtr pSettings):
    TerrainBlock2( pSettings ),
	unlockCallback_( NULL )
{
}


EditorTerrainBlock2::~EditorTerrainBlock2()
{
	bw_safe_delete(unlockCallback_);
}


/**
 *  This function creates an EditorTerrainBlock2 using the settings in the
 *  DataSection.
 *
 *  @param settings         DataSection containing the terrain settings.
 *  @param error		    Optional output parameter that receives an error
 *							message if an error occurs.
 *  @returns                True if successful, false otherwise.
 */
bool EditorTerrainBlock2::create( DataSectionPtr settings, BW::string* error /*= NULL*/ )
{
	BW_GUARD;
	unlockCallback_ = new ETB2UnlockCallback( *this );

	heightMap2( new TerrainHeightMap2( blockSize(), 0, 0, unlockCallback_ ) );
	if ( !heightMap2()->create( settings_->heightMapEditorSize(), error ) )
		return false;

	holeMap2( new TerrainHoleMap2(*this) );
	if ( !holeMap2()->create( settings_->holeMapSize(), error ) )
		return false;

	normalMap2( new TerrainNormalMap2 );
	rebuildNormalMap( NMQ_NICE );

	aoMap2( new TerrainAOMap2 );
	rebuildAOMap( settings_->aoMapSize() );

    horizonMap2( new HorizonShadowMap2(this) );
	if ( !horizonMap2()->create( settings_->shadowMapSize(), error ) )
		return false;

	pDetailHeightMapResource_ = new HeightMapResource( blockSize(), fileName_, 0 );

	if ( !initVerticesResource( error ) )
		return false;

	// Force creation of blends:
	evaluate(Vector3::zero());
	stream();

	return true;
}

bool EditorTerrainBlock2::rebuildVerticesResource( uint32 numVertexLods )
{
	BW_GUARD;
	pVerticesResource_ = new VerticesResource( *this, numVertexLods );
	if ( !pVerticesResource_ )
		return false;
	return true;
}


bool EditorTerrainBlock2::resizeHeightMap( uint32 size )
{
	TerrainHeightMap2::ImageType oldImage;
	TerrainHeightMap2Ptr map = heightMap2();

	{
		Terrain::TerrainHeightMapHolder holder( heightMap2().get(), false );
		oldImage = heightMap2()->image();
	}

	if ( !heightMap2()->create( size ) )
		return false;

	{
		Terrain::TerrainHeightMapHolder holder( heightMap2().get(), false );
		// heightMap2()->image().blit( oldImage );
	}

	settings_->heightMapEditorSize( size );

	if ( !initVerticesResource( NULL ) )
		return false;

	// Force creation of blends:
	evaluate( Vector3::zero() );
	stream();

	rebuildNormalMap( NMQ_NICE );
	return true;
}


bool EditorTerrainBlock2::load
(	
	BW::string			const &filename, 
	Matrix				const &worldTransform,
	Vector3				const &cameraPosition,
	BW::string			*error /*= NULL*/
)
{
	BW_GUARD;
	bool ok = 
		TerrainBlock2::load(filename, worldTransform, cameraPosition, error);

	if ( ok )
	{
		worldPos_ = worldTransform.applyToOrigin();

		unlockCallback_ = new ETB2UnlockCallback( *this );
		heightMap2()->unlockCallback( unlockCallback_ );

		// Validate block's map dimensions. This is specially useful for when
		// placing terrain prefabs.
		//if ( heightMap().blocksWidth() != settings_->heightMapSize() )
		//{
		//	ERROR_MSG( "EditorTerrainBlock2::load: "
		//		"Invalid height map for file %s\n", filename.c_str() );
		//	return false;
		//}
		if ( normalMapSize() != settings_->normalMapSize() )
		{
			ERROR_MSG( "EditorTerrainBlock2::load: "
				"Invalid normal map for file %s\n", filename.c_str() );
			return false;
		}
		// Must have at least one blend
		if (numberTextureLayers() == 0)
		{
			ERROR_MSG( "EditorTerrainBlock2::load: "
				"No texture layers for terrain block %s\n", filename.c_str() );
			return false;
		}
		// Assuming all layers are the same size
		if ( numberTextureLayers() > 0 &&
			textureLayer( 0 ).width() != settings_->blendMapSize() )
		{
			ERROR_MSG( "EditorTerrainBlock2::load: "
				"Invalid texture layers for terrain block %s\n",
				filename.c_str() );
			return false;
		}
		if ( holesSize() != settings_->holeMapSize() )
		{
			ERROR_MSG( "EditorTerrainBlock2::load: "
				"Invalid hole map for terrain block %s\n", filename.c_str() );
			return false;
		}
 		if ( horizonMapSize() != settings_->shadowMapSize() )
 		{
 			ERROR_MSG( "EditorTerrainBlock2::load: "
 				"Invalid horizon shadow map for terrain block %s\n",
				filename.c_str() );
 			return false;
 		}
	}

	return ok;
}


bool EditorTerrainBlock2::saveLodTexture( DataSectionPtr dataSection ) const
{
	bool ok = true;

	dataSection->deleteSections( TerrainBlock2::LOD_TEXTURE_SECTION_NAME );

	if (pLodMap_)
	{
		ok = pLodMap_->save( dataSection,
			TerrainBlock2::LOD_TEXTURE_SECTION_NAME );
	}

	return ok;
}


/*virtual*/ bool EditorTerrainBlock2::save(DataSectionPtr dataSection) const
{
	BW_GUARD;
	ensureBlendsLoaded();

	bool ok = true;

	ok = saveHeightmap( dataSection );

	ok &= pNormalMap_->save( dataSection );

	ok &= pAOMap_->save( dataSection );

	dataSection->deleteSections( "horizonShadows" );
	ok &= this->shadowMap().save( dataSection->newSection("horizonShadows",
												 BinSection::creator()) );

	dataSection->deleteSections( "holes" );
	if (!this->holeMap().noHoles())
	{
	   ok &= this->holeMap().save( dataSection->newSection("holes",
	   											 BinSection::creator()) );
	}

	dataSection->deleteSections( LAYER_SECTION_NAME );
	for (size_t i = 0; i < numberTextureLayers(); ++i)
	{
	   ok &= this->textureLayer(i).save( dataSection->newSection(
												LAYER_SECTION_NAME,
	   											BinSection::creator()) );
	}

	dataSection->deleteSections( "dominantTextures" );
	if ( this->dominantTextureMap() )
	{
		ok &= this->dominantTextureMap()->save( dataSection->newSection("dominantTextures",
												 BinSection::creator()) );
	}

	ok &= saveLodTexture( dataSection );

	// remove the seven (old 1.9) vertex lods if they exist.
	for ( uint32 i = 0; i < 7 ; i++ )
	{
		BW::string sectionName;
		VertexLodEntry::getSectionName( i, sectionName );
		dataSection->deleteSections( sectionName.c_str() );
	}

	DataSectionPtr terrainBlockMetaData = dataSection->openSection( TERRAIN_BLOCK_META_DATA_SECTION_NAME, true, XMLSection::creator() );
	if( terrainBlockMetaData != NULL )
	{
		terrainBlockMetaData->writeInt( FORCED_LOD_ATTRIBUTE_NAME, getForcedLod() );
	}

	return ok;
} 


/*virtual*/ bool EditorTerrainBlock2::saveToCData(DataSectionPtr pCDataSection) const
{
	BW_GUARD;
	bool ret = false;
	if (pCDataSection)
	{
		DataSectionPtr dataSection = 
			pCDataSection->openSection( dataSectionName(), true,
										 ZipSection::creator());

		if ( dataSection )
		{
			dataSection->setParent(pCDataSection);
			dataSection = dataSection->convertToZip("",pCDataSection);
			ret = save(dataSection);
			ret &= pCDataSection->save();
			dataSection->setParent( NULL );
		}
	}
    return ret;
}


/*virtual*/ bool EditorTerrainBlock2::save(BW::string const &filename) const
{
	BW_GUARD;
	DataSectionPtr pCDataSection = BWResource::openSection(filename, true,
													ZipSection::creator());
    return saveToCData( pCDataSection );
}


/*virtual*/ size_t EditorTerrainBlock2::numberTextureLayers() const
{
	BW_GUARD;
	ensureBlendsLoaded();

	if (pBlendsResource_->getObject())
	{
		return pBlendsResource_->getObject()->textureLayers_.size();
	}
	else
	{
		return 0;
	}
}


/*virtual*/ size_t EditorTerrainBlock2::maxNumberTextureLayers() const
{
    return std::numeric_limits<size_t>::max();
}


/*virtual*/ size_t EditorTerrainBlock2::insertTextureLayer(
	uint32 blendWidth /*= 0*/, uint32 blendHeight /*= 0*/ )
{
	BW_GUARD;
	ensureBlendsLoaded();

    TextureLayers *layers = textureLayers();
    TerrainTextureLayer2Ptr layer =
		new TerrainTextureLayer2(*this, true, blendWidth, blendHeight);
    layers->push_back( layer );
    return layers->size() - 1;
}


/*virtual*/ bool EditorTerrainBlock2::removeTextureLayer(size_t idx)
{
	BW_GUARD;
	ensureBlendsLoaded();

    TextureLayers* layers = textureLayers();
    layers->erase(layers->begin() + idx);
    return false;
}


/*virtual*/ TerrainTextureLayer &EditorTerrainBlock2::textureLayer(size_t idx)
{
	BW_GUARD;
	ensureBlendsLoaded();

    return *pBlendsResource_->getObject()->textureLayers_[idx];
}


/*virtual*/ TerrainTextureLayer const &EditorTerrainBlock2::textureLayer(size_t idx) const
{
	BW_GUARD;
	ensureBlendsLoaded();

    return *pBlendsResource_->getObject()->textureLayers_[idx];
}


/*virtual*/ bool EditorTerrainBlock2::draw( const Moo::EffectMaterialPtr& pMaterial )
{
    BW_GUARD;
	if( s_drawSelection_ )
	{
		HRESULT hr = Moo::rc().setRenderState( D3DRS_TEXTUREFACTOR, selectionKey_ );
		MF_ASSERT(hr == D3D_OK);
	}
	return TerrainBlock2::draw( pMaterial );
}


void EditorTerrainBlock2::setHeightMapDirty()
{
	BW_GUARD;
	pVerticesResource_->setDirty();
	rebuildNormalMap( NMQ_FAST );
}


void EditorTerrainBlock2::rebuildCombinedLayers(
	bool compressTextures /*= true*/, 
	bool generateDominantTextureMap /*= true */,
	bool invalidateLodMap /*= true */)
{
	BW_GUARD;
	ensureBlendsLoaded();

	DominantTextureMap2Ptr tempDominantMap = dominantTextureMap2();
	pBlendsResource_->rebuild
	(
		compressTextures, 
		generateDominantTextureMap ? &tempDominantMap : NULL
	);
	if (generateDominantTextureMap)
	{
		// ground-types support.
		DominantTextureMap2* curDominantMap = dominantTextureMap2().get();
		if(curDominantMap)
		{
			for(uint y = 0; y < curDominantMap->image().height(); ++y)
				for(uint x = 0; x < curDominantMap->image().width(); ++x)
					tempDominantMap->setGroundType(x, y, curDominantMap->getGroundType(x, y));
		}
		dominantTextureMap2(tempDominantMap);
	}

	if (invalidateLodMap)
	{
		pLodMap_ = NULL;
	}
}

void EditorTerrainBlock2::rebuildAOMap( uint32 newSize )
{
	BW_GUARD;
	pAOMap_->regenerate(newSize);
}

bool EditorTerrainBlock2::importAOMap(const Moo::Image<uint8>& srcImage, uint top, uint left)
{
	BW_GUARD;
	return pAOMap_->import(srcImage, top, left, settings_->aoMapSize());
}

void EditorTerrainBlock2::rebuildNormalMap( NormalMapQuality normalMapQuality, uint32 newSize/* = 0*/ )
{
	BW_GUARD;
	pNormalMap_->generate(
		heightMap2(), normalMapQuality, 
		newSize ? newSize : settings_->normalMapSize()
		);
}

bool EditorTerrainBlock2::rebuildLodTexture( const Matrix& transform, uint32 newSize/*= 0*/ )
{
	BW_GUARD;
	ensureBlendsLoaded();

	bool ok = false;

	if ( pLodMap_ == NULL )
	{
		pLodMap_ = new TerrainLodMap2();
	}

	// Save state of lodding
	uint32	savedStartLod		= settings_->topVertexLod();
	bool	savedConstantLod	= settings_->constantLod();
	bool	savedUseLodTexture	= settings_->useLodTexture();

	// Enable the correct renderer
	settings_->setActiveRenderer();

	// Enable constant max vertex lod, and disable texture lod ( because thats what
	// we are generating ).
	settings_->topVertexLod( 0 );
	settings_->constantLod( true );
	settings_->useLodTexture( false );

	// 'stream' must be called before rendering. The position passed are ignored
	// because we are setting constant LODing to the smallest LOD above.
	evaluate(Vector3::zero() );

	if (pBlendsResource_->getState() == RS_Unloaded)
	{
		// Combining the layers is done over a number of frames, so we need to
		// ensure the combined layers are loaded for the photographer to work,
		// so here we trick "stream" into thinking it should rebuild the blends
		s_lastBlendBuildTime = 0;
		s_lastBlendBuildMark = s_nextBlendBuildMark_ - 1;
	}
	stream();

	// now take photograph
	TerrainPhotographer& photographer = TerrainRenderer2::instance()->photographer();

	bool oldUseHoleMap = TerrainRenderer2::instance()->enableHoleMap( false );

	// HACK - disable special mode rendering
	bool oldUseSpecialMode = TerrainRenderer2::instance()->isSpecialModeEnabled_;
	TerrainRenderer2::instance()->isSpecialModeEnabled_ = false;

	ComObjectWrap<DX::Texture> texture = pLodMap_->pTexture();

	newSize = newSize ? newSize : settings_->lodMapSize();

	//-- if size of the loaded lod texture and desired size is not equal, or the lod texture doesn't
	//-- exist then regenerate it with the right size.
	bool isNewTexture = !texture;
	if (!isNewTexture && pLodMap_->size() != newSize)
	{
		isNewTexture = true;
		texture = NULL;
	}
	
	Moo::rc().beginScene();
	if (photographer.init( newSize ) &&
		photographer.photographBlock( const_cast<EditorTerrainBlock2*>(this),
									transform ))
	{
		if (!texture.hasComObject())
		{
			// Create our output texture here as we want to be able to use it
			// for rendering
			// the photographer uses the texture compressor which uses system
			// memory textures
			// by default
			// If we are using the ex device we also need to make the texture
			// dynamic so it can be locked when saving
			DWORD usage = Moo::rc().usingD3DDeviceEx() ? D3DUSAGE_DYNAMIC : 0;
			texture = Moo::rc().createTexture( newSize, newSize, 
				0, usage, D3DFMT_DXT5, D3DPOOL_MANAGED );
		}

		if (photographer.output( texture, D3DFMT_DXT5 ))
		{
			if ( Moo::rc().device()->TestCooperativeLevel() == D3D_OK )
			{
				// If we didn't lose device during photo (eg from a CTRL+ALT+DEL)
				// then mark as ok.
				ok = true;

				pLodMap_->pTexture( texture );
			}
			else
			{
				// Remove possibly corrupt lod texture.
				pLodMap_ = NULL;
				ERROR_MSG( "EditorTerrainBlock2::rebuildLodTexture: "
					"Failed on terrain block %s, the DX device was lost.\n",
					this->resourceName().c_str() );
			}
		}
		else
		{
			ERROR_MSG( "EditorTerrainBlock2::rebuildLodTexture: "
				"Failed on terrain block %s, failed to photograph the terrain.\n",
				this->resourceName().c_str() );
		}
	}
	Moo::rc().endScene();

	TerrainRenderer2::instance()->enableHoleMap( oldUseHoleMap );

	// HACK - re-enable special mode rendering
	TerrainRenderer2::instance()->isSpecialModeEnabled_ = oldUseSpecialMode;

	// reset lod states
	settings_->topVertexLod( savedStartLod );
	settings_->constantLod( savedConstantLod );
	settings_->useLodTexture( savedUseLodTexture );

	return ok;
}

bool EditorTerrainBlock2::resizeShadowMap(uint newSize)
{
	return horizonMap2()->create(newSize);
}

void EditorTerrainBlock2::dirtyLodTexture()
{
	pLodMap_ = NULL;
}

void EditorTerrainBlock2::streamBlends()
{
	BW_GUARD;
	if 
	( 
		(lodRenderInfo_.renderTextureMask_ & TerrainRenderer2::RTM_DrawBlend) != 0
		||
		(lodRenderInfo_.renderTextureMask_ & TerrainRenderer2::RTM_PreLoadBlend) != 0			
	)	
	{
		ensureBlendsLoaded();
	}

	if (pBlendsResource_->getObject() != NULL)
	{
		// If we are drawing blends, or the lod map is dirty and so we are being
		// forced to draw them (pLodMap_ is NULL) then make sure that there 
		// are combined layers.
		// If the use of LOD textures is turned off then it's likely that the
		// combined layers don't exist for far away chunks.  If this happens
		// then we need to regenerate them.
		if 
		(
			(
				!pLodMap_
				|| 
				(lodRenderInfo_.renderTextureMask_ & TerrainRenderer2::RTM_DrawBlend) != 0
				||
				!settings_->useLodTexture() 
			)
			&&
			pBlendsResource_->getState() == RS_Unloaded
		)
		{
			// Since rebuilding the combined layers is an expensive operation,
			// specially because it ends up calling ttl2Cache::onLock which
			// calls the expensive decompressImage, we only rebuild layers
			// every s_blendBuildInterval milliseconds.
			uint64 blendBuildTimeout = s_lastBlendBuildTime +
				s_blendBuildInterval_ * stampsPerSecond() / 1000;

			if (blendBuildTimeout < timestamp() &&
				s_lastBlendBuildMark != s_nextBlendBuildMark_)
			{
				bool wasLodDirty = !pLodMap_; 
				rebuildCombinedLayers( true, false, false );

				if (wasLodDirty)
				{
					pLodMap_ = NULL;
				}

				// rebuild layers again in s_blendBuildInterval milliseconds
				// from now, but only if it's another frame.
				s_lastBlendBuildMark = s_nextBlendBuildMark_;
				s_lastBlendBuildTime = timestamp();
			}
		}

		// If we are drawing blends (or forced into drawing them) and not drawing
		// the lod texture then get rid of the blend texture to save precious
		// memory.  If we need it again then we regenerate it in the above if
		// statement.
		if 
		(
			pLodMap_
			&&
			(lodRenderInfo_.renderTextureMask_ & TerrainRenderer2::RTM_DrawBlend) == 0
		)
		{
			pBlendsResource_->unload();
		}
	}	
}


void EditorTerrainBlock2::ensureBlendsLoaded() const
{
	BW_GUARD;
	if (pBlendsResource_->getObject() == NULL)
	{
		if (!pBlendsResource_->load())
		{
			ERROR_MSG( "Failed to load blends for terrain EditorTerrainBlock2 "
				" %s\n", fileName_.c_str() );
		}
	}
}


void EditorTerrainBlock2::stream()
{
	BW_GUARD;
	// Stream vertices
	pVerticesResource_->stream();

	// Stream blends if we are not in a reflection. We override it here rather 
	// than in visibility test to stop reflection scene from unloading blends
	// required by main scene render.
	if ( !Moo::rc().reflectionScene() )
	{
		streamBlends();
	}
}


bool EditorTerrainBlock2::canDrawLodTexture() const
{
	return TerrainBlock2::canDrawLodTexture() && !isLodTextureDirty();
}


/**
 *  This method increments the frame mark for rebuilding combined layers.
 */
/*static*/ void EditorTerrainBlock2::nextBlendBuildMark()
{
	s_nextBlendBuildMark_ = Moo::rc().frameTimestamp();
}


/**
 *  This method returns the interval to wait between calls to the
 *	rebuildCombinedLayers method, because it's quite expensive to update all
 *	layers for all blocks that need it at once.
 *
 *	@return Current interval in milliseconds between calls.
 */
/*static*/ uint32 EditorTerrainBlock2::blendBuildInterval()
{
	return s_blendBuildInterval_;
}


/**
 *  This method allows configuring the interval between calls to the
 *	rebuildCombinedLayers method, because it's quite expensive to update all
 *	layers for all blocks that need it at once.
 *
 *	@param interval Interval in milliseconds between calls.
 */
/*static*/ void EditorTerrainBlock2::blendBuildInterval( uint32 interval )
{
	s_blendBuildInterval_ = interval;
}


//#define DEBUG_HEIGHT_MAPS
#ifdef DEBUG_HEIGHT_MAPS

BW_END_NAMESPACE
#include "moo/png.hpp"
BW_BEGIN_NAMESPACE

void WriteHeightImagePNG( const TerrainHeightMap2::ImageType& dimg,
							float minHeight, float maxHeight, 
							uint32 fileCount, uint32 lodLevel )
{
	BW_GUARD;
	Moo::PNGImageData pngData;
	pngData.data_ = new uint8[dimg.width()*dimg.height()];
	pngData.width_ = dimg.width();
	pngData.height_ = dimg.height();
	pngData.bpp_ = 8;
	pngData.stride_ = dimg.width();
	pngData.upsideDown_ = false;
	uint8 *p = pngData.data_;

	for ( uint32 x = 0; x < dimg.width(); x++ )
	{
		for ( uint32 y = 0; y < dimg.height(); y++ )
		{
			float val  = dimg.get(x, y);
			uint8 grey = Math::lerp(val, minHeight, maxHeight, (uint8)0, (uint8)255);
			*p++ = grey; 
		}
	}
	BinaryPtr png = compressPNG(pngData);

	char filename[64]; 
	bw_snprintf( filename, 64,"e:/temp/%d_heights_%d.png",
		fileCount, lodLevel );
	FILE *file = BWResource::instance().fileSystem()->posixFileOpen( filename, "wb");
	fwrite(png->data(), png->len(), 1, file);
	fclose(file); file = NULL;
	bw_safe_delete_array(pngData.data_);
}
#endif

bool EditorTerrainBlock2::saveHeightmap( DataSectionPtr dataSection ) const
{
	BW_GUARD;
	dataSection->deleteSections( "heights" );

	// save first lod of height map
	bool ok = heightMap().save( dataSection->newSection("heights", 
												BinSection::creator()) );

#ifdef DEBUG_HEIGHT_MAPS
	// debug
	static uint32 fileCount = 0;
#endif

	if ( ok )
	{
		// save each lod of height map
		for ( uint32 i = 1; i < settings_->numVertexLods(); i++ )
		{
			// Source size is visible part of main height map
			uint32				srcVisSize	= heightMap2()->verticesWidth();
			uint32				lodSize		= (srcVisSize >> i) + 1;

			// create a down sampled height map - we only sample the visible
			// part of the original height map.
			AliasedHeightMap	ahm( i, heightMap2() );
			TerrainHeightMap2	dhm( blockSize(), lodSize, i );

			dhm.lock( false );
			TerrainHeightMap2::ImageType& dimg = dhm.image();

			for ( uint32 x = 0; x < lodSize; x++ )
			{
				for ( uint32 y = 0; y < lodSize; y++ )
				{
					dimg.set( x, y, ahm.height(x,y) );		
				}
			}
			dhm.unlock();

			// save down sampled height map
			char sectionName[64];
			bw_snprintf( sectionName, ARRAY_SIZE(sectionName), "heights%d", i );
			ok = dhm.save( dataSection->openSection( sectionName, true,
												BinSection::creator() ) );

#ifdef DEBUG_HEIGHT_MAPS
			if ( i == 1 )
			{
				// write source map only once as lod 0
				WriteHeightImagePNG( simg, heightMap2()->minHeight(), 
					heightMap2()->maxHeight(), fileCount, 0 );
			}
			// write lod height map
			WriteHeightImagePNG(dimg, heightMap2()->minHeight(), 
				heightMap2()->maxHeight(), fileCount, i );
#endif
			if ( !ok ) break;
		}
	}

#ifdef DEBUG_HEIGHT_MAPS
	//debug
	fileCount++;
#endif

	return ok;
}

BW_END_NAMESPACE

#endif // EDITOR_ENABLED

// editor_terrain_block2.cpp
