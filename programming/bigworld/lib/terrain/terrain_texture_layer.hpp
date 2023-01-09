#ifndef TERRAIN_TEXTURE_LAYER_HPP
#define TERRAIN_TEXTURE_LAYER_HPP

#ifndef MF_SERVER
#include "moo/com_object_wrap.hpp"
#include "moo/moo_dx.hpp"
#endif

#include "terrain_map.hpp"
#include "terrain_map_holder.hpp"


BW_BEGIN_NAMESPACE

namespace Moo
{
	class BaseTexture;
	typedef SmartPointer< BaseTexture > BaseTexturePtr;
}

namespace Terrain
{
	class TerrainTextureLayer;
	typedef SmartPointer<TerrainTextureLayer> TerrainTextureLayerPtr;

	/**
     *  This class allows access to a texture layer.
     */
    class TerrainTextureLayer : public TerrainMap<uint8>
    {
    public:
		TerrainTextureLayer( float blockSize ) : TerrainMap<uint8>( blockSize ) {};
        /**
         *  This function gets the name of the texture in the layer.
         *
         *  @returns            The name of the texture used in the layer.
         */
        virtual BW::string textureName() const = 0;

        /**
         *  This function gets the name of the bump texture in the layer.
         *
         *  @returns            The name of the bump texture used in the layer.
         */
        virtual BW::string bumpTextureName() const = 0;

        /**
         *  This function sets the texture used in the layer by loading
         *  the given texture.
         *
         *  @param filename     The name of the texture to use.
         *  @returns            True if the texture can be used, false 
         *                      otherwise.
         */
        virtual bool textureName(BW::string const &filename) = 0;

		/**
         *  This function sets the bump texture used in the layer by loading
         *  the given texture.
         *
         *  @param filename     The name of the bump texture to use.
         *  @returns            True if the bump texture can be used, false 
         *                      otherwise.
         */
        virtual bool bumpTextureName(BW::string const &filename) = 0;

#ifndef MF_SERVER
        /**
         *  This function gets the texture used in the layer.
         *
         *  @returns            The texture used in the layer.
         */
		virtual const Moo::BaseTexturePtr& texture() const = 0;

		/**
         *  This function gets the bump texture used in the layer or NULL if current layer or the
		 *	hole terrain doesn't have any bump texture.
         *
         *  @returns            The bumped texture used in the layer.
         */
		virtual const Moo::BaseTexturePtr& bumpTexture() const = 0;

        /**
         *  This function sets the texture used in the layer.
         *
         *  @param texture      The new texture to be used.
         *  @param textureName  The filename of the texture that is used.
         */
        virtual void texture(Moo::BaseTexturePtr texture, BW::string const &textureName) = 0;  

		/**
         *  This function sets the bump texture used in the layer.
         *
         *  @param texture      The new bump texture to be used.
         *  @param textureName  The filename of the texture that is used.
         */
        virtual void bumpTexture(Moo::BaseTexturePtr texture, BW::string const &textureName) = 0;  
#endif

        /**
         *  This function is used to determine whether the TerrainTextureLayer 
         *  has u and v projections?
         *
         *  @returns            True if setting/getting the u, v projections
         *                      are allowed.
         */
        virtual bool hasUVProjections() const = 0;

        /**
         *  This function gets the u texture projection.  This is undefined if
         *  hasUVProjections returns false.
         *
         *  @returns            The u texture projection.
         */
        virtual Vector4 const &uProjection() const = 0; 

        /**
         *  This function sets the u texture projection.  This is undefined if
         *  hasUVProjections returns false.
         *
         *  @param u            The new u texture projection.
         */
        virtual void uProjection(Vector4 const &u) = 0;

        /**
         *  This function gets the v texture projection.  This is undefined if
         *  hasUVProjections returns false.
         *
         *  @returns            The v texture projection.
         */
        virtual Vector4 const &vProjection() const = 0;

        /**
         *  This function sets the v texture projection.  This is undefined if
         *  hasUVProjections returns false.
         *
         *  @param v            The new v texture projection.
         */
        virtual void vProjection(Vector4 const &v) = 0;

#ifdef EDITOR_ENABLED
        /**
         *  This function saves the texture layer to a DataSection.
         *
         *  @param section      The section to save to.
         *  @returns            True if saved successfully.
         */
        virtual bool save(DataSectionPtr section) const = 0;

		/**
		 *	This function compares this layer with another and returns they
		 *	represent the same texture and projections.
		 */
		bool 
		sameTexture
		(
			TerrainTextureLayer const &other, 
			float				epsilon
		) const;

		/**
		 *	This function compares the texture and projections (if they exist)
		 *	with the give projections.
		 */
		bool sameTexture
		(
			BW::string			const &texture, 
			BW::string			const &bumpTexture,
			Vector4				const &uProj, 
			Vector4				const &vProj,
			float				epsilon
		) const;

		/**
		 *  This function saves a layer to a PNG file for debugging purposes.
		 *
		 *	@param filename		The name of the file to save to.
		 *  @returns			True if saved successfully.
		 */
		bool saveToPNG(const BW::string &filename) const;

	public:
#endif

        /**
         *  This function loads the texture layer from a DataSection.
         *
         *  @param section      The section to load from.
         *  @param error        If an error occurs during loading then this 
         *                      will be set to an error string.
         *  @returns            True if the load was successful.
         */
        virtual bool load(DataSectionPtr section, BW::string *error = NULL) = 0;

#ifndef MF_SERVER
        /**
         *  Blend up to four TerrainTextureLayers to create a single
         *  texture.  Any layer can be missing, but we assume that the if there
         *  is a layer then layer 0 exists.  We also assume that if there are
         *  layers then they are all the same size.
         *
         *  @param layer0       Layer 0. 
         *  @param layer1       Layer 1.  
         *  @param layer2       Layer 2. 
         *  @param layer3       Layer 3. 
		 *	@param allowSmallBlend	If true then we can pack into 8 or 16 bit
		 *							textures if possible.
		 *	@param compressTexture	If true then the texture is compressed.
		 *	@param smallBlended	Set to true if the texture could be packed.
         *  @return				The blended texture.
         */
        static ComObjectWrap<DX::Texture> createBlendTexture
        (
            TerrainTextureLayerPtr  layer0,
            TerrainTextureLayerPtr  layer1,
            TerrainTextureLayerPtr  layer2,
            TerrainTextureLayerPtr  layer3,
			bool					allowSmallBlend,
			bool					compressTexture,
			bool					*smallBlended
		);
#endif

        /**
         *  This gets the default u and v projections.
         *
         *  @param u            This is set to the default u-projection.
         *  @param v            This is set to the default v-projection.
         */
        static void defaultUVProjections(Vector4 &u, Vector4 &v);
    };


    /**
     *  This can be used to iterate over a TerrainTextureLayer.
     */
    typedef TerrainMapIter<TerrainTextureLayer>     TerrainTextureLayerIter;


    /**
     *  This can be used to lock/unlock a TerrainTextureLayer.
     */
    typedef TerrainMapHolder<TerrainTextureLayer>   TerrainTextureLayerHolder;
};

BW_END_NAMESPACE

#endif // TERRAIN_TEXTURE_LAYER_HPP
