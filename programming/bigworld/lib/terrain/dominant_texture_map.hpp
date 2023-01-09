#ifndef TERRAIN_DOMINANT_TEXTURE_MAP_HPP
#define TERRAIN_DOMINANT_TEXTURE_MAP_HPP

#include "terrain_map.hpp"
#include "terrain_map_holder.hpp"
#include "terrain_data.hpp"

BW_BEGIN_NAMESPACE

namespace Terrain
{
	class TerrainTextureLayer;
	typedef uint8 MaterialIndex;

	// types
	typedef SmartPointer<TerrainTextureLayer> TerrainTextureLayerPtr;
	typedef BW::vector<TerrainTextureLayerPtr> TextureLayers;
}

namespace Moo
{	
	//Specialise Image for MaterialIndex type to avoid attempting
	//bicubic interpolation when using parametric indices.  It makes
	//no sense to interpolate material indices.
	template <> inline Terrain::MaterialIndex
		Image<Terrain::MaterialIndex>::getBicubic(float x, float y) const
    {
		return get(int(x), int(y));
	}

	template <> inline double
		Image<Terrain::MaterialIndex>::getBicubic(double x, double y) const
    {
		return (double)get( int(x), int(y) );
	}
}

namespace Terrain
{
    /**
     *  This class allows access to material data of terrain blocks.
     */
    class DominantTextureMap : public TerrainMap<MaterialIndex>
    {
    public:
		DominantTextureMap( float blockSize );

		DominantTextureMap(
			float				blockSize,
			TextureLayers&		layerData,
			float               sizeMultiplier = 0.5f );

        /**
         *  This function interprets the terrain material as a material kind.
         *
         *  @returns            The material kind at x,z
         */
        uint32 materialKind( float x, float z ) const;

		uint8 groundType( float x, float z ) const;

#ifdef EDITOR_ENABLED
		void setGroundType( uint x, uint y, uint8 value );
		uint8 getGroundType( uint x, uint y );
#endif

        /**
         *  This function interprets the terrain material as a texture name.
         *
         *  @returns            The texture name at x,z
         */
		const BW::string& texture( float x, float z ) const;
#ifndef MF_SERVER
		const BW::string& textureNoExt( float x, float z ) const;
#endif

		virtual uint32 width() const;
		virtual uint32 height() const;

#ifdef EDITOR_ENABLED
		virtual bool lock(bool readOnly);		
		virtual ImageType &image();
		virtual bool unlock();
		virtual bool save(DataSectionPtr) const;
#endif
		virtual ImageType const &image() const;

	protected:
		void image( const ImageType& newImage );
		const BW::vector<BW::string>& textureNames() const;
#ifndef MF_SERVER
		const BW::vector<BW::string>& textureNamesNoExt() const { return textureNamesNoExt_; }
#endif
		void textureNames( const BW::vector<BW::string>& names );

	private:
		MaterialIndex materialIndex( float x, float z ) const;

		// stub definition
		virtual bool load(DataSectionPtr dataSection, BW::string *error = NULL)
		{ return false; };

	    Moo::Image<MaterialIndex>	image_;
		BW::vector<BW::string>	textureNames_;
#ifndef MF_SERVER
		BW::vector<BW::string>	textureNamesNoExt_;
#endif
    };


    /**
     *  A TerrainHoleMapIter can be used to iterate over a DominantTextureMap.
     */
    typedef TerrainMapIter<DominantTextureMap>      DominantTextureMapIter;


    /**
     *  A DominantTextureMapHolder can be used to lock/unlock a DominantTextureMap.
     */
    typedef TerrainMapHolder<DominantTextureMap>    DominantTextureMapHolder;
}

BW_END_NAMESPACE

#endif // TERRAIN_DOMINANT_TEXTURE_MAP_HPP
