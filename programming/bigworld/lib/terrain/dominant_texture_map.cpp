#include "pch.hpp"
#include "dominant_texture_map.hpp"

#include "terrain_data.hpp"
#include "terrain_texture_layer.hpp"

#include "cstdmf/debug.hpp"
#include "physics2/material_kinds.hpp"

BW_BEGIN_NAMESPACE

namespace Terrain
{

/**
 *  This is the DominantTextureMap default constructor.  This represents
 *  terrain without material information.
 *
 *  @param layers          Texture layers to extract texture info.
 *  @param sizeMultiplier  Multiplier to affect the size of the dominant texture map
 *                         relative to the size of the biggest layer.
 */
/*explicit*/ DominantTextureMap::DominantTextureMap(
	float				blockSize,
	TextureLayers&		layers,
	float               sizeMultiplier /* = 0.5f*/) :
		TerrainMap<MaterialIndex>( blockSize ),
		image_( "Terrain/DominantTextureMap" )
{
	BW_GUARD;
	MF_ASSERT( layers.size() > 0 )

	// Use width and height of the biggest layer multiplied by 'sizeMultiplier'
	// as the width and height of the material ID map.
	uint32 width = 0;
	uint32 height = 0;
	for (uint32 i=0; i<layers.size(); i++)
	{
		width = std::max( width, uint32( layers[i]->width() * sizeMultiplier ) );
		height = std::max( height, uint32( layers[i]->height() * sizeMultiplier ) );
	}

	//Cache multipliers for each layer to convert indices
	//into parametric coordinates for the layer
	BW::vector< std::pair< float, float > > multipliers;
	multipliers.reserve( layers.size() );

	for (uint32 i=0; i<layers.size(); i++)
	{
		float xMul = (layers[i]->width() / (float)width);
		float yMul = (layers[i]->height() / (float)height);
		multipliers.push_back( std::make_pair(xMul, yMul) );
	}

	image_.resize(width, height);

#ifdef EDITOR_ENABLED
	this->lock(false);
	for (uint32 i=0; i<layers.size(); i++)
	{
		layers[i]->lock(true);
	}
#endif

	BW::vector<float> weights;
	weights.reserve(layers.size());

	for (uint32 i=0; i<layers.size(); i++)
	{
		const BW::string& textureName = layers[i]->textureName();
		textureNames_.push_back( textureName );
#ifndef MF_SERVER
		textureNamesNoExt_.push_back( 
			BWResource::removeExtension( textureName ).to_string() );
#endif
		uint32 mKind = MaterialKinds::instance().get( textureName );
		DataSectionPtr ds = MaterialKinds::instance().userData(mKind);
		float weight = 1.f;
		if ( ds.hasObject() )
		{
			//read the weight for the material kind, if available.
			weight = ds->readFloat( "weight", weight );
			DataSectionPtr ts = MaterialKinds::instance().textureData(mKind,textureName);
			if (ts.hasObject())
			{
				//read the weight for the texture map itself, if available.
				weight = ts->readFloat( "weight", weight );
			}
		}
		weights.push_back( weight );
	}

	//For each sample point, find the dominant layer and
	//store the ID of that layer in the image_ map.
	for (uint32 y=0; y<height; y++)
	{
		MaterialIndex* pValues = image_.getRow(y);
		for (uint32 x=0; x<width; x++)
		{
			MaterialIndex index = 0;
			float maxValue = -1.0f;

			for (uint32 i=0; i<layers.size(); i++)
			{
				//get bicubic interpolated blend value at x,y position.
				uint8 blendValue = layers[i]->image().getBicubic(
					(float)x * multipliers[i].first,
					(float)y * multipliers[i].second );
				//adjust for weighting, allow some textures to dominate others.
				float adjustedBlendValue = (float)blendValue * weights[i];
				if (adjustedBlendValue > maxValue)
				{
					index = (MaterialIndex)i;
					maxValue = adjustedBlendValue;
				}
			}

			// hold ground kind in 4 higher bits.
			// May be should think about better way to keep its clean.
			pValues[x] = index & 0x0F;
		}
	}

#ifdef EDITOR_ENABLED
	for (uint32 i=0; i<layers.size(); i++)
	{
		layers[i]->unlock();
	}
	this->unlock();
#endif
}


/**
 *  This is the DominantTextureMap default constructor. Use this
 *	constructor if you want to use the load() interface afterwards. 
 *
 */
DominantTextureMap::DominantTextureMap( float blockSize ) :
	TerrainMap<MaterialIndex>( blockSize ),
	image_( "Terrain/DominantTextureMap" )
{
}

/**
 *	This method returns the index into our material table for the
 *	dominant texture at the given x,z position relative to the
 *	terrain block.
 *
 *	@return				The material index at relative position x,z
 */
MaterialIndex DominantTextureMap::materialIndex( float x, float z ) const
{
	BW_GUARD;
	//this->lock(true);
	x = (x/blockSize()) * (image_.width() - 1) + 0.5f;
	z = (z/blockSize()) * (image_.height()- 1) + 0.5f;

	// hold ground kind in 4 higher bits.
	MaterialIndex id = image_.get( int(x), int(z) ) & 0x0F;
	//this->unlock();
	return id;
}

uint8 DominantTextureMap::groundType( float x, float z ) const
{
	x = (x/blockSize()) * (image_.width() - 1) + 0.5f;
	z = (z/blockSize()) * (image_.height()- 1) + 0.5f;

	MaterialIndex id = image_.get( int(x), int(z) ) >> 4;
	return id;	
}

#ifdef EDITOR_ENABLED
void DominantTextureMap::setGroundType( uint x, uint y, uint8 value )
{
	MaterialIndex cur = image_.get(x, y);
	image_.set(x, y, (cur & 0x0f) | (value << 4));
}

uint8 DominantTextureMap::getGroundType( uint x, uint y )
{
	return image_.get( int(x), int(y) ) >> 4;
}
#endif // EDITOR_ENABLED

/**
 *	This method returns the dominant material kind at x,z position
 *	relative to the terrain block's position.
 *
 *	@return				The texture name at relative position x,z
 */
uint32 DominantTextureMap::materialKind( float x, float z ) const
{
	BW_GUARD;
	MaterialIndex id = this->materialIndex(x,z);
	MF_ASSERT( id < textureNames_.size() )
	return MaterialKinds::instance().get( textureNames_[id] );
}


/**
 *	This method returns the name of the dominant texture at x,z position
 *	relative to the terrain block's position.
 *
 *	@return				The texture name at relative position x,z
 */
const BW::string& DominantTextureMap::texture( float x, float z ) const
{
	BW_GUARD;
	MaterialIndex id = this->materialIndex(x,z);
	MF_ASSERT( id < textureNames_.size() )
	return textureNames_[id];
}

#ifndef MF_SERVER
const BW::string& DominantTextureMap::textureNoExt( float x, float z ) const
{
	BW_GUARD;
	MaterialIndex id = this->materialIndex(x,z);
	MF_ASSERT( id < textureNamesNoExt_.size() )
		return textureNamesNoExt_[id];
}
#endif // not defined MF_SERVER

/**
 *  Allows setting the image from a derived class.
 *
 *  @param newImage     New dominant texture map image
 */
void DominantTextureMap::image( const ImageType& newImage )
{
	image_ = newImage;
}


/**
 *  Allows accessing the texture names from a derived class.
 *
 *  @returns     New texture names
 */
const BW::vector<BW::string>& DominantTextureMap::textureNames() const
{
	return textureNames_;
}


/**
 *  Allows setting the texture names from a derived class.
 *
 *  @param names     New texture names
 */
void DominantTextureMap::textureNames( const BW::vector<BW::string>& names )
{
	textureNames_.assign( names.begin(), names.end() );
#ifndef MF_SERVER
	BW::vector< BW::string >::const_iterator it;
	for( it = names.begin(); it != names.end(); it++ )
		textureNamesNoExt_.push_back( 
		BWResource::removeExtension( *it ).to_string() );
#endif // not defined MF_SERVER
}


/**
 *  This function returns the width of the dominant texture map.
 *
 *  @returns            The width of the dominant texture map.
 */
/*virtual*/ uint32 DominantTextureMap::width() const
{
    return image_.width();
}


/**
 *  This function returns the height of the dominant texture map.
 *
 *  @returns            The height of the dominant texture map.
 */
/*virtual*/ uint32 DominantTextureMap::height() const
{
    return image_.height();
}


#ifdef EDITOR_ENABLED

/**
 *  This function locks the dominant texture map for reading and/or writing.
 */
/*virtual*/ bool DominantTextureMap::lock(bool readonly)
{
	return true;
}


/**
 *  This function returns the underlying image of the dominant texture map.
 *
 *  @returns            The underlying image of the dominant texture map.
 */
/*virtual*/ DominantTextureMap::ImageType &DominantTextureMap::image()
{
    return image_;
}


/**
*  This function unlocks the dominant texture map.
*/
/*virtual*/ bool DominantTextureMap::unlock()
{
	return true;
}


/**
*  This function saves the dominant texture map to a DataSection.
*
*  @returns            A DataSection that can be used to restore the
*                      dominant texture map with DominantTextureMap::Load.
*/
/*virtual*/ bool DominantTextureMap::save( DataSectionPtr ) const
{
	// TODO implement if needed.  Currently the dominant texture map is not saved
	// to disk by design, so we don't need this and shouldn't call this.
	MF_ASSERT(0);
	return false;
}

#endif // EDITOR_ENABLED
/**
 *  This function returns the underlying image of the dominant texture map.
 *
 *  @returns            The underlying image of the dominant texture map.
 */
/*virtual*/ DominantTextureMap::ImageType const &DominantTextureMap::image() const
{
    return image_;
}

} // namespace Terrain

BW_END_NAMESPACE

// dominant_texture_map.cpp
