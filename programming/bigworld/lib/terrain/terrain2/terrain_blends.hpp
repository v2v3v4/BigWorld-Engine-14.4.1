#ifndef __TERRAIN_BLENDS_HPP__
#define __TERRAIN_BLENDS_HPP__

#ifndef MF_SERVER
#include "moo/com_object_wrap.hpp"
#include "moo/moo_dx.hpp"
#include "resource.hpp"
#include "dominant_texture_map2.hpp"
#include "terrain_renderer2.hpp"


BW_BEGIN_NAMESPACE

namespace Terrain
{
	class TerrainBlendsResource;
	class TerrainBlock2;
	class TerrainSettings;

	typedef SmartPointer<TerrainBlendsResource> TerrainBlendsResourcePtr;
	typedef SmartPointer<DominantTextureMap2>	DominantTextureMap2Ptr;
	typedef SmartPointer<TerrainSettings>		TerrainSettingsPtr;

	struct CombinedLayer
	{
		CombinedLayer();

		uint32						width_;
		uint32						height_;
		ComObjectWrap<DX::Texture>	pBlendTexture_;
		TextureLayers				textureLayers_;
		bool						smallBlended_;
	};

	struct TerrainBlends : SafeReferenceCount
	{	
		TerrainBlends();
		virtual ~TerrainBlends();

		bool init( TerrainBlock2& owner, bool loadBumpMaps );
		void createCombinedLayers( bool compressTextures,
									 DominantTextureMap2Ptr* newDominantTexture);

		float						blockSize_;
		TextureLayers			    textureLayers_;
		BW::vector<CombinedLayer>  combinedLayers_;
	};

	class TerrainBlendsResource : public Resource< TerrainBlends >
	{
	public:
		TerrainBlendsResource( TerrainBlock2& owner, bool loadBumpMaps );

		// Specialise evaluate and loading methods
		inline ResourceRequired		evaluate( uint8 renderTextureMask );
		virtual bool				load();
		virtual void				stream( ResourceStreamType streamType = defaultStreamType() );

		virtual void preAsyncLoad();
		virtual void postAsyncLoad();

#ifdef EDITOR_ENABLED
		virtual bool rebuild(	bool compressTextures, 
								DominantTextureMap2Ptr* newDominantTexture );
		virtual void unload();
		virtual ResourceState		getState() const;
		virtual ObjectTypePtr		getObject();
#endif

	protected:
		virtual void				startAsyncTask();

		bool						loadBumpMaps_;
		TerrainBlock2&				owner_;
	};

	inline ResourceRequired TerrainBlendsResource::evaluate( 
													uint8 renderTextureMask )
	{
		required_ = RR_No;

		if (renderTextureMask & 
			(TerrainRenderer2::RTM_PreLoadBlend | TerrainRenderer2::RTM_DrawBlend))
		{
			required_ = RR_Yes;
		}

		return required_;
	}

} // namespace Terrain

BW_END_NAMESPACE

#endif // MF_SERVER

#endif
