#include "pch.hpp"

#include "texture_mask_cache.hpp"

#include "worldeditor/terrain/editor_chunk_terrain.hpp"
#include "worldeditor/terrain/terrain_texture_utils.hpp"
#include "worldeditor/import/terrain_utils.hpp"

#include "terrain/editor_base_terrain_block.hpp"
#include "terrain/terrain_height_map.hpp"
#include "terrain/terrain_texture_layer.hpp"

BW_BEGIN_NAMESPACE

BW_SINGLETON_STORAGE( TextureMaskCache );


/**
 *	This should be called before fuzziness is called.  It is a good practice to
 *	batch call TextureMaskCache::fuziness between calls to beginFuzziness and
 *	endFuzziness.
 *
 *	@param terrain		The terrain to calculate the mask on.
 *	@param brush		The paint brush.
 *	@param maskLayers	This is set to match the layers that mask the texturing
 *						if mask texturing is enabled.
 */
/*static*/ void TextureMaskCache::beginFuzziness
(
	Terrain::EditorBaseTerrainBlockPtr	terrain,
	const TerrainPaintBrush &	brush,
	BW::vector<bool> &			maskLayers
)
{
	BW_GUARD;

	maskLayers.resize( terrain->numberTextureLayers() );
	if (brush.textureMask_)
	{				
		for (size_t i = 0; i < terrain->numberTextureLayers(); ++i)
		{
			Terrain::TerrainTextureLayer &layer = 
				const_cast< Terrain::TerrainTextureLayer & >( terrain->textureLayer( i ) );
			layer.lock( true );
			if (layer.sameTexture(
					brush.paintTexture_, 
					brush.paintBumpTexture_,
					brush.paintUProj_, 
					brush.paintVProj_, 
					TerrainTextureUtils::PROJECTION_COMPARISON_EPSILON ))
			{
				// Always add the paint layer to the mask
				maskLayers[i] = true;
			}
			else if (brush.isMaskLayer( layer ))
			{
				maskLayers[i] = !brush.textureMaskInvert_;
			}
			else
			{
				maskLayers[i] = brush.textureMaskInvert_;
			}
		}
	}
	else
	{
		for (size_t i = 0; i < terrain->numberTextureLayers(); ++i)
		{
			maskLayers[i] = false;
		}
	}
}


/**
 *	This calculates the mask values for the given position within the block.
 *	
 *	@param terrain		The terrain to get the mask value for.
 *	@param blockOffset	The position of the terrain.
 *	@param pos			The position within the terrain.
 *	@param brush		The paint brush.
 *	@param maskLayers	The value calculated in beginFuzziness.
 */
/*static*/ MaskPixel
TextureMaskCache::fuzziness
(
	Terrain::EditorBaseTerrainBlockPtr terrain,
	const Vector3 &			blockOffset,
	const Vector3 &			pos,
	const TerrainPaintBrush & brush,
	const BW::vector<bool> & maskLayers
)
{
	BW_GUARD;

	/*
	 *	The algorithm below is fairly straight-forward.  
	 *
	 *	We calculate the fuzziness of a mask pixel based upon whether it meets 
	 *	the height slope etc criteria.  We allow a linear transition of the 
	 *	mask by specifying, for example, a minimum height where the mask is
	 *	0 and a minimum height where it is 255 and linearly-interpolating
	 *	in between.  We do a similar thing for the maximum height, the
	 *	minimum slope etc.  If you were to apply the mask like this several
	 *	times then the fuzzy region would accumulate quickly and lose its
	 *	fuzziness (e.g. it would become sharp along the minimum height value).
	 *	To prevent this we set a maxValue as well as the fuzziness.  This
	 *	maxValue prevents values from accumulating beyond the 
	 *	linearly-interpolated value, and so the fuzzy region always remains
	 *	fuzzy.
	 */

	MaskPixel result;
	result.maxValue_  = brush.opacity_;
	result.fuzziness_ = 255;

	// Calculate the height mask:
	if (brush.heightMask_)
	{
		float h = terrain->heightMap().heightAt( pos.x, pos.z );
        
		if (h < brush.h1_ || h > brush.h4_)
		{
			result.fuzziness_ = 0;
		}
		else if (brush.h1_ <= h && h <= brush.h2_ && brush.h1_ != brush.h2_)
		{
			int f = Math::lerp( h, brush.h1_, brush.h2_, 0, 255 );
			result.maxValue_  = std::min( result.maxValue_, (uint8)f );
			result.fuzziness_ = (uint8)( result.fuzziness_*f/255 );
		}
		else if (brush.h3_ <= h && h <= brush.h4_ && brush.h3_ != brush.h4_)
		{
			int f = Math::lerp( h, brush.h3_, brush.h4_, 255, 0 );
			result.maxValue_  = std::min( result.maxValue_, (uint8)f );
			result.fuzziness_ = (uint8)( result.fuzziness_*f/255 );
		}
	}

	// Calculate the slope mask:
	if (brush.slopeMask_)
	{
		float angle = terrain->heightMap().slopeAt( pos.x, pos.z );

		if (angle < brush.s1_ || angle > brush.s4_)
		{
			result.fuzziness_ = 0;
		}
		else if (brush.s1_ <= angle && angle <= brush.s2_ && brush.s1_ != brush.s2_)
		{
			int f = Math::lerp( angle, brush.s1_, brush.s2_, 0, 255 );
			result.maxValue_  = std::min( result.maxValue_, (uint8)f );
			result.fuzziness_ = (uint8)( result.fuzziness_*f/255 );
		}
		else if (brush.s3_ <= angle && angle <= brush.s4_ && brush.s3_ != brush.s4_)
		{
			int f = Math::lerp( angle, brush.s3_, brush.s4_, 255, 0 );
			result.maxValue_  = std::min( result.maxValue_, (uint8)f );
			result.fuzziness_ = (uint8)( result.fuzziness_*f/255 );
		}
	}

	// Calculate the texture mask:
	if (brush.textureMask_ && !brush.textureMaskTexture_.empty())
	{
		uint32 value = 0;
		for (size_t i = 0; i < terrain->numberTextureLayers(); ++i)
		{
			const Terrain::TerrainTextureLayer & layer = 
				terrain->textureLayer( i );
			if (maskLayers[ i ])
			{
				int w = (int)layer.width () - 1;
				int h = (int)layer.height() - 1;
				float polesSpacingX = terrain->blockSize()/w;
				float polesSpacingZ = terrain->blockSize()/h;
				int tx = (int)( pos.x/polesSpacingX + 0.5f );
				int ty = (int)( pos.z/polesSpacingZ + 0.5f );
				tx = Math::clamp( 0, tx, w );
				ty = Math::clamp( 0, ty, h );
				value += layer.image().get( tx, ty );
			}
		}
		value = Math::clamp( 0, result.fuzziness_*(int)value/255, 255 );
		result.maxValue_  = std::min( result.maxValue_, (uint8)value );
        result.fuzziness_ = (uint8)( result.fuzziness_*value/255 );
	}

    // Calculate the noise mask;
    if (brush.noiseMask_)
    {
		float wx = blockOffset.x + pos.x;
		float wz = blockOffset.z + pos.z;
        float f = brush.noise_( wx, wz );
		if (f <= brush.noiseMinSat_)
		{
			f = brush.noiseMinStrength_;
		}
		else if (f >= brush.noiseMaxSat_)
		{
			f = brush.noiseMaxStrength_;
		}
		else
		{
			f =
				Math::lerp(
					f, 
					brush.noiseMinSat_, 
					brush.noiseMaxSat_, 
					brush.noiseMinStrength_, 
					brush.noiseMaxStrength_ );
		}
        result.maxValue_  = std::min( result.maxValue_, (uint8)(f*255.0f) );
        result.fuzziness_ = (uint8)( result.fuzziness_*f );
    }
	// Calculate the import mask:
	if (brush.importMask_ && brush.importMaskImage_ != NULL)
	{
		float wx = blockOffset.x + pos.x;
		float wz = blockOffset.z + pos.z;
		if (
			brush.importMaskTL_.x <= wx && wx < brush.importMaskBR_.x 
			&&
			brush.importMaskBR_.y <= wz && wz < brush.importMaskTL_.y )
		{
			// sx, sy is the coordinates in the mask image.  Note
			// that we flip the y coordinate.
			float sx = 
				Math::lerp(
					wx, 
					brush.importMaskTL_.x, 
					brush.importMaskBR_.x, 
					0.0f, 
					(float)brush.importMaskImage_->width() );
			float sy = 
				Math::lerp(
					wz, 
					brush.importMaskBR_.y, 
					brush.importMaskTL_.y, 
					(float)brush.importMaskImage_->height(), 
					0.0f );
			// We have to scale the mask image pixel values linearly to 
			// scale it into the [0, 1] range.
			float maskValue16 = (float)( brush.importMaskImage_->getBilinear( sx, sy ) );
			float maskValue = maskValue16*brush.importMaskMul_ + brush.importMaskAdd_;
			maskValue = Math::clamp( 0.0f, maskValue, 1.0f );
			result.fuzziness_ = (uint8)( result.fuzziness_*maskValue );
			result.maxValue_  = std::min( result.maxValue_, (uint8)( 255.0f*maskValue ) );
		}
		else
		{
			result.fuzziness_ = 0;
		}
	}

	return result;
}


/**
 *	This should be called after fuzziness is called. 
 *
 *	@param terrain		The terrain to calculate the mask on.
 *	@param brush		The paint brush.
 *	@param maskLayers	The value calculated in beginFuzziness.
 */
/*static*/ void TextureMaskCache::endFuzziness
(
	Terrain::EditorBaseTerrainBlockPtr	terrain,
	const TerrainPaintBrush &	brush,
	const BW::vector<bool> &	maskLayers
)
{
	BW_GUARD;

	if (brush.textureMask_)
	{
		for (size_t i = 0; i < terrain->numberTextureLayers(); ++i)
		{
			Terrain::TerrainTextureLayer & layer = 
				const_cast<Terrain::TerrainTextureLayer &>( terrain->textureLayer( i ) );
			layer.unlock();
		}
	}
}


/**
 *	This clears the cache of cached values.
 */
void TextureMaskCache::clear()
{
	BW_GUARD;

	cache_.clear();
}


/**
 *	This gets the mask for the given chunk.  If the value is in the cache
 *	then the cached value is returned.  If the value is not in the cache then
 *	it's calculated.
 *
 *	@param terrain	The terrain to get the mask for.
 *	@return 		A pointer to the mask image for the terrain.
 */
MaskResult * TextureMaskCache::mask( Terrain::EditorBaseTerrainBlockPtr terrain )
{
	BW_GUARD;

	CacheMap::iterator it = cache_.find(terrain);
	if (it == cache_.end())
	{
		return generate(terrain);
	}
	else
	{
		return &it->second;
	}
}


/**
 *	This should be called if the texture layers have been changed around
 *	while the cache is being used.  Typically this is called during painting
 *	to update the mask layers.  These can change if a layer is removed 
 *	due to a low contribution to the blends or if a layer is added.
 *
 *	@param terrain	The terrain that was modified and whose cache values need
 *					to be recalculated.
 */
void TextureMaskCache::changedLayers( Terrain::EditorBaseTerrainBlockPtr terrain )
{
	BW_GUARD;

	CacheMap::iterator it = cache_.find( terrain );
	if (it != cache_.end())
	{
		MaskResult &result = it->second;
		Terrain::EditorBaseTerrainBlockPtr pBlock = it->first;
		result.maskTexLayer_.resize( pBlock->numberTextureLayers() );
		for (size_t i = 0; i < pBlock->numberTextureLayers(); ++i)
		{
			const Terrain::TerrainTextureLayer & layer = pBlock->textureLayer( i );
			result.maskTexLayer_[ i ] = paintBrush_->isMaskLayer(layer);
		}
	}
}


/**
 *	This gets the paint brush that the cache is using.
 *
 *	@return 		The mask brush.
 */
TerrainPaintBrushPtr TextureMaskCache::paintBrush() const
{
	return paintBrush_;
}


/**
 *	This sets the mask options for the cache.  If the options have genuinely
 *	changed then the cache is cleared.
 *
 *	@param brush	The new mask brush.
 */
void TextureMaskCache::paintBrush( TerrainPaintBrushPtr brush )
{
	BW_GUARD;

	if (brush && *paintBrush_ != *brush)
	{
		paintBrush_ = brush;
		clear();
	}
}


/**
 *	This is the TextureMaskCache constructor.
 */
TextureMaskCache::TextureMaskCache():
    paintBrush_( TerrainPaintBrushPtr( new TerrainPaintBrush(), true ) )
{
}


/**
 *	This is the TextureMaskCache destructor.
 */
TextureMaskCache::~TextureMaskCache()
{
}


/**
 *	This function generates a mask image for the given block.
 *
 *	@param terrain	The terrain to get the mask for.
 *	@return 		A pointer to the mask image for the terrain.
 */
MaskResult * TextureMaskCache::generate( Terrain::EditorBaseTerrainBlockPtr terrain )
{
	BW_GUARD;

	if (terrain == NULL)
	{
		return NULL;
	}

	if (terrain->numberTextureLayers() == 0)
	{
		return NULL;
	}

	Vector3 offset = terrain->worldPos();

	// Get the size of the mask:
	Terrain::TerrainTextureLayer const &ttl = terrain->textureLayer( 0 );
	uint32 layerWidth  = ttl.width();
	uint32 layerHeight = ttl.height();

	// Setup the cached image:
	MaskResult &result = cache_[ terrain ];
	MaskImage  &img    = result.image_;
	img.resize( layerWidth, layerHeight );

	// Get the number of poles for the terrain:
	float polesSpacingX = terrain->blockSize()/(layerWidth  - 1);
	float polesSpacingY = terrain->blockSize()/(layerHeight - 1);

	beginFuzziness( terrain, *paintBrush_, result.maskTexLayer_ );

	for (uint32 y = 0; y < img.height(); ++y)
	{
		for (uint32 x = 0; x < img.width(); ++x)
		{
			Vector3 pos(x*polesSpacingX, 0.f, y*polesSpacingY);
			MaskPixel value = 
				fuzziness( terrain, offset, pos, *paintBrush_, result.maskTexLayer_ );
			img.set( x, y, value );
		}
	}

	endFuzziness( terrain, *paintBrush_, result.maskTexLayer_ );

	return &result;
}
BW_END_NAMESPACE

