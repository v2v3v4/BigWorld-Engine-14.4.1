#include "pch.hpp"
#include "editor_base_terrain_block.hpp"

#include "terrain_texture_layer.hpp"

#ifdef EDITOR_ENABLED

DECLARE_DEBUG_COMPONENT2("Moo", 0)

BW_BEGIN_NAMESPACE

using namespace Terrain;


/*static*/ bool EditorBaseTerrainBlock::s_drawSelection_ = false;


EditorBaseTerrainBlock::EditorBaseTerrainBlock(float blockSize) :
	BaseTerrainBlock( blockSize ),
	selectionKey_( 0 ),
	worldPos_( Vector3::zero() )
{
}


void EditorBaseTerrainBlock::resourceName(BW::string const &id)
{
	resourceName_ = id.substr( 0, id.find_last_of( '/' ) + 1 ) + dataSectionName();
}


BW::string const &EditorBaseTerrainBlock::resourceName() const
{
    return resourceName_;
}


size_t EditorBaseTerrainBlock::findLayer
(
	BW::string				const &texture, 
	BW::string				const &bumpTexture,
	Vector4					const &uProjection,
	Vector4					const &vProjection,
	float					epsilon,
	size_t					startIdx	/*= 0*/	
) const
{
	BW_GUARD;
	for (size_t i = startIdx; i < numberTextureLayers(); ++i)
	{
		TerrainTextureLayer const &layer = textureLayer(i);
		if (layer.sameTexture(texture, bumpTexture, uProjection, vProjection, epsilon))
			return i;
	}
	return INVALID_LAYER;
}


size_t EditorBaseTerrainBlock::findLayer
(
	TerrainTextureLayer		const &layer, 
	float					epsilon,
	size_t					startIdx /*= 0*/
) const
{
	BW_GUARD;
	return 
		findLayer
		(
			layer.textureName(), 
			layer.bumpTextureName(),
			layer.hasUVProjections() ? layer.uProjection() : Vector4::zero(),
			layer.hasUVProjections() ? layer.vProjection() : Vector4::zero(), 
			epsilon,
			startIdx
		);
}


size_t EditorBaseTerrainBlock::findWeakestLayer() const
{
	BW_GUARD;
	size_t layer = INVALID_LAYER;
	int minSum = -1;
	for (uint32 i = 0; i < numberTextureLayers(); ++i)
	{
        const TerrainTextureLayer& ttl = textureLayer( i );
        TerrainTextureLayerHolder 
			holder(const_cast<TerrainTextureLayer *>( &ttl ), true);

		const TerrainTextureLayer::ImageType& image = ttl.image();
		int layerSum = 0;
		const uint8* p = image.getRow( 0 );
		const uint8* end = p + image.rawDataSize(); // assuming no padding
		while (p != end)
		{
			layerSum += *p++;
		}

		if (minSum == -1 || layerSum < minSum)
		{
			// this is the layer that contributes the less so far.
			minSum = layerSum;
			layer  = i;
		}
	}
	return layer;
}


bool EditorBaseTerrainBlock::optimiseLayers(float epsilon)
{
	BW_GUARD;
	typedef TerrainTextureLayer::PixelType BlendPixel;

	bool modified = false;
	for (size_t i = 0; i < numberTextureLayers() - 1; i++ )
	{
		TerrainTextureLayer &dstLayer = textureLayer(i);
		size_t srcLayerIdx = i + 1;
		while (srcLayerIdx != INVALID_LAYER)
		{
			srcLayerIdx = findLayer( dstLayer, epsilon, srcLayerIdx );
			if (srcLayerIdx != INVALID_LAYER)
			{					
				{ // Force a scope for dstHolder and srcHolder
				TerrainTextureLayerHolder dstHolder(&dstLayer, false);
				TerrainTextureLayer::ImageType &dst = dstLayer.image();
				// Source layer (the one being deleted):
				TerrainTextureLayer &srcLayer = textureLayer( srcLayerIdx );
				TerrainTextureLayerHolder srcHolder( &srcLayer, true );
				TerrainTextureLayer::ImageType &src = srcLayer.image();
				if ( src.width() == dst.width() && src.height() == dst.height() )
				{
					// Actual copy, which adds together the blends of both layers				
					for (uint32 y = 0; y < src.height(); ++y)
					{
						BlendPixel const *p = src.getRow(y);
						BlendPixel const *q = p + src.width();
						BlendPixel       *d = dst.getRow(y);
						for( ; p != q; ++p, ++d)
						{
							*d = BlendPixel( min( int( *p ) + int( *d ), 255 ) );
						}
					}
				} 
				} // dstHolder and srcHolders release their lock on the layers here
				removeTextureLayer( srcLayerIdx );
				modified = true;
			}
		}
	}
	return modified;
}

BW_END_NAMESPACE

#else

// Avoid LNK4221 warning - no public symbols. This happens when this file is
// compiled in non-editor build
extern const int dummyPublicSymbol = 0;

#endif // EDITOR_ENABLED
