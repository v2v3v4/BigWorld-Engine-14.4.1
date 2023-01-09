#ifndef RENDER_CONTEXT_HPP
#define RENDER_CONTEXT_HPP

#include "moo_math.hpp"
#include "moo_dx.hpp"

#include "camera.hpp"
#include "com_object_wrap.hpp"
#include "light_container.hpp"
#include "cstdmf/bw_string_ref.hpp"
#include "cstdmf/bw_deque.hpp"
#include "cstdmf/vectornodest.hpp"
#include "cstdmf/concurrency.hpp"
#include "cstdmf/profiler.hpp"
#include "math/matrix.hpp"
#include "moo/graphics_settings.hpp"
#include "moo/gpu_info.hpp"
#include "fog_helper.hpp"
#include <stack>
#include "gpu_profiler.hpp"
#include "moo/texture_reuse_cache.hpp"

#ifndef MAX_CONCURRENT_RTS
// Must be at least 1
#define MAX_CONCURRENT_RTS	4
#endif


BW_BEGIN_NAMESPACE

#if ENABLE_ASSET_PIPE
class AssetClient;
#endif

namespace Moo
{
	class FullscreenQuad;
	class CustomAA;
	class Material;
	class OcclusionQuery;
	class DynamicIndexBufferInterface;
	class MRTSupport;
	class EffectVisualContext;

	/**
	 * Flags for various code paths for different video cards.
	 */
	enum CompatibilityFlag
	{
		COMPATIBILITYFLAG_NOOVERWRITE = 1 << 0,
		COMPATIBILITYFLAG_NVIDIA =		1 << 1,
		COMPATIBILITYFLAG_ATI				= 1 << 2,
		COMPATIBILITYFLAG_DEFERRED_SHADING	= 1 << 3
	};

	/**
	 * This structure holds interesting information about a graphics device. 
	 * The RenderContext holds one of these for each device on the
	 * system.
	 */
	struct DeviceInfo
	{
		D3DADAPTER_IDENTIFIER9			identifier_;
		D3DCAPS9						caps_;
		uint32							adapterID_;
		bool							windowed_;
		D3DDISPLAYMODE					windowedDisplayMode_;
		BW::vector< D3DDISPLAYMODE >	displayModes_;
		uint32							compatibilityFlags_;
	};

	extern bool & isRenderThread();

	/**
	 * Render state cache entry.
	 */
	struct RSCacheEntry
	{
		uint32							currentValue;
		uint							Id;
	};

	/**
	 * Texture stage state cache entry.
	 */
	struct TSSCacheEntry
	{
		uint32							currentValue;
		uint							Id;
	};

	/**
	 * Sample cache entry.
	 */
	struct SampCacheEntry
	{
		uint32							currentValue;
		uint							Id;
	};

	/**
	 * Texture cache entry.
	 */
	struct TextureCacheEntry
	{
		DX::BaseTexture*				pCurrentTexture;
		uint							Id;
	};

	/*
	 * Profiling data struct for drawcalls
	 */
	struct DrawcallProfilingData
	{
		uint32 nDrawcalls_;
		uint32 nPrimitives_;
	};

	/**
	 *	This class creates and manages the d3d device, backbuffer etc, it's
	 *	the current state of rendering in that it contains the current lights,
	 *	matrices, lod state, zoom factor and so forth. Anything that is of global
	 *	importance to rendering should be contained in the render context.
	 */
	class RenderContext
	{
	public:
		static const uint16 SHADER_VERSION_NOT_INITIALISED;

		RenderContext();
		~RenderContext();

		bool init( bool d3dExInterface, bool assetProcessingOnly );
		void fini();
		bool isValid() const { return isValid_; }

		bool assetProcessingOnly() const { return assetProcessingOnly_; }

		uint32						nDevices( ) const;
		const DeviceInfo&			deviceInfo( uint32 i ) const;
		const DeviceInfo&			currentDeviceInfo( ) const;

		bool						checkDevice(bool *reset = NULL);
		bool						checkDeviceYield(bool *reset = NULL);

		bool						createDevice( HWND hWnd,
													uint32 deviceIndex           = 0,
													uint32 modeIndex             = 0,
													bool windowed                = true,
													bool wantStencil             = false,
													const Vector2 & windowedSize = Vector2(0, 0),
													bool hideCursor              = true,
													bool forceRef				 = false
#if ENABLE_ASSET_PIPE
													,  AssetClient* pAssetClient = NULL
#endif
													);
		void						releaseDevice( void );

		bool						changeMode( 
										uint32 modeIndex, 
										bool   windowed, 
										bool   testCooperative = false,
										uint32 backBufferWidthOverride = 0 );

		bool						resetDevice();

		void						releaseUnmanaged( bool forceRelease = false );
		void						createUnmanaged( bool forceCreate = false );

		void						clearBindings();

		DX::Device*					device() const;
		DX::DeviceEx*				deviceEx() const;

		void						restoreCursor( bool state );
		bool						hardwareCursor() const;

		bool						windowed( ) const;
		uint32						deviceIndex( ) const;
		uint32						modeIndex( ) const;
		bool						stencilAvailable( ) const;
		DWORD						maxVertexIndex( ) const;

		D3DFORMAT					adapterFormat() const;

		uint						getAvailableTextureMem( ) const;

		void						getGpuMemoryInfo( GpuInfo::MemInfo* outMemInfo ) const;

		bool						mixedVertexProcessing() const { return mixedVertexProcessing_; }
		bool						mrtSupported() const { return mrtSupported_; }

		D3DFORMAT					getMatchingZBufferFormat( D3DFORMAT colourFormat, bool stencilWanted, bool & stencilAvailable );

		bool						pushRenderTarget();
		bool						popRenderTarget();

		const Camera&				camera( ) const;
		Camera&						camera( );
		void						camera( const Camera& cam );

		const Matrix&				projection( ) const;
		Matrix&						projection( );
		void						projection( const Matrix& m );
		void						scaleOffsetProjection( const float scale, const float x, const float y );

		const Matrix&				view( ) const;
		Matrix&						view( );
		void						view( const Matrix& m );

		const Matrix&				world() const;
		Matrix&						world();
		void						world( const Matrix& m );

		const Matrix&				lastViewProjection( ) const;
		void						lastViewProjection( const Matrix& m  );

		const Matrix&				viewProjection( ) const;
		const Matrix&				invView( ) const;

		void						preMultiply( const Matrix& m );
		void						postMultiply( const Matrix& m );

		void						push( );
		void						pop( );
		void						reset( );
		void						reset( const Matrix& m );

		void						updateProjectionMatrix(bool detectAspectRatio = true);
		void						updateViewTransforms( );

		const LightContainerPtr &	lightContainer( ) const;
		void						lightContainer( const LightContainerPtr & pLC );

		const LightContainerPtr &	specularLightContainer( ) const;
		void						specularLightContainer( const LightContainerPtr & pLC );

		const D3DSURFACE_DESC&		backBufferDesc( ) const;
		const D3DPRESENT_PARAMETERS& presentParameters( ) const;

		uint32						frameTimestamp( ) const;
		bool						frameDrawn( uint32& frame ) const;
		void						nextFrame( );

		float						screenWidth() const;
		float						screenHeight() const;
		float						halfScreenWidth() const;
		float						halfScreenHeight() const;
		void						screenWidth( int width );
		void						screenHeight( int height );
		void						backBufferWidthOverride( uint32 backBufferWidthOverride );
		uint32						backBufferWidthOverride() const;

		const Vector2 &				windowedModeSize() const;

		void						fullScreenAspectRatio( float ratio );
		float						fullScreenAspectRatio() const;

		void						objectAlphaOverride( float alpha );
		uint32						objectAlphaOverride( void ) const;

		void						depthOnly( bool depthOnly );
		bool						depthOnly() const;

		const BW::string			screenShot( const BW::string& fileExt = "bmp", const BW::string& fileName = "shot", bool autoNumber = true );
		DX::Surface*				getScreenCopy();

		float						lodValue() const;
		void						lodValue( float value );

		float						lodFar() const;
		void						lodFar( float value );

		float						lodPower() const;
		void						lodPower( float value );

		float						zoomFactor() const;
		
		const Matrix&				lodView() const;
		const Matrix&				lodInvView() const;
		float						lodZoomFactor() const;
		void						updateLodViewMatrix();

		float						gammaCorrection() const;
		void						gammaCorrection( float gammaCorrection );

		bool						waitForVBL() const;
		void						waitForVBL( bool wait );
		bool						tripleBuffering() const { return tripleBuffering_; }
		void						tripleBuffering( bool enabled ) { tripleBuffering_ = enabled; }

		bool						isMultisamplingEnabled() const; 
		
		//-- gets access to the custom AA feature.
		CustomAA&					customAA();

		//-- full-screen quad used mainly for post-processing tasks.
		FullscreenQuad&				fsQuad();

		bool						supportsTextureFormat( D3DFORMAT fmt ) const;

		uint16						psVersion() const;
		uint16						vsVersion() const;
		uint16						maxSimTextures() const;
		uint16						maxAnisotropy() const;

		bool						memoryCritical() const { return memoryCritical_; }
		void						memoryCritical( bool val ) { memoryCritical_ = val; }

		/*
		 *	pause/resume methods to free/realloc resources. Useful in windowed
		 *	apps that need to be friendly with other DX apps.
		 */
		void						pause();
		void						resume();
		bool						paused() const;

		/*
		 * State caching
		 */

		HRESULT						setViewport( DX::Viewport* viewport );
		HRESULT						getViewport( DX::Viewport* viewport ) const;

		uint32						setRenderState(D3DRENDERSTATETYPE state, uint32 value);
		uint32						setTextureStageState( uint32 stage, D3DTEXTURESTAGESTATETYPE type, uint32 value );
		uint32						setSamplerState( uint32 stage, D3DSAMPLERSTATETYPE type, uint32 value);
		uint32						setTexture(DWORD stage, DX::BaseTexture* pTexture);

		uint32						setVertexShader( DX::VertexShader* pVS );
		uint32						setPixelShader( DX::PixelShader* pPS );
		HRESULT						setIndices( DX::IndexBuffer* pIB );
		ComObjectWrap<DX::IndexBuffer>	getIndices() const;
		uint32						setVertexDeclaration( DX::VertexDeclaration* pVD );
		uint32						setFVF( uint32 fvf );

		uint32						setWriteMask( uint i, uint32 value );

		void						pushRenderState( D3DRENDERSTATETYPE state );
		void						popRenderState();

		uint32						drawIndexedPrimitiveUP( D3DPRIMITIVETYPE PrimitiveType,
															UINT minIndex,
															UINT numVertices,
															UINT primitiveCount,
															CONST void* pIndexData,
															D3DFORMAT indexDataFormat,
															CONST void* pVertexStreamZeroData,
															UINT vertexStreamZeroStride );
		uint32						drawIndexedPrimitive( D3DPRIMITIVETYPE type,
														  INT baseVertexIndex,
														  UINT minIndex,
														  UINT numVertices,
														  UINT startIndex,
														  UINT primitiveCount );

		//-- Note: This function is equal to drawIndexedPrimitive by functionality with only one
		//--		  debug ability to correct doing counting number of drew primitives.
		uint32						drawIndexedInstancedPrimitive( D3DPRIMITIVETYPE type,
															INT baseVertexIndex,
															UINT minIndex,
															UINT numVertices,
															UINT startIndex,
															UINT primitiveCount,
															UINT instanceCount );

		uint32						drawPrimitive( D3DPRIMITIVETYPE primitiveType,
												   UINT startVertex,
												   UINT primitiveCount );
		uint32						drawPrimitiveUP( D3DPRIMITIVETYPE primitiveType,
													 UINT primitiveCount,
													 CONST void* pVertexStreamZeroData,
													 UINT vertexStreamZeroStride );

		void						getNearPlaneRect( Vector3 & corner,
										Vector3 & xAxis, Vector3 & yAxis ) const;

		HRESULT						beginScene();
		HRESULT						endScene();
		HRESULT						present();

		HRESULT						setRenderTarget(uint32 index, ComObjectWrap<DX::Surface> surface);
		ComObjectWrap<DX::Surface>	getRenderTarget(uint32 index);
		ComObjectWrap<DX::Texture>	secondRenderTargetTexture();
		uint32						renderTargetCount() const;

		ComObjectWrap<DX::Texture>	createTexture( UINT Width, UINT Height, UINT Levels, DWORD Usage,
										D3DFORMAT Format, D3DPOOL Pool, const BW::StringRef& allocator = "texture/unknown" );

		ComObjectWrap<DX::CubeTexture>	createCubeTexture( UINT edgeLength, UINT levels, DWORD usage,
										D3DFORMAT format, D3DPOOL pool, const BW::StringRef& allocator = "texture/unknown"  );

		void						putTextureToReuseList( const ComObjectWrap<DX::Texture>& tex );
		void						clearTextureReuseList();
		TextureReuseCache &			textureReuseCache();

		ComObjectWrap<DX::Texture>	createTextureFromFileInMemoryEx(
										LPCVOID pSrcData, UINT SrcDataSize, UINT Width, UINT Height,
										UINT MipLevels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool,
										DWORD Filter, DWORD MipFilter, D3DCOLOR ColorKey, D3DXIMAGE_INFO* pSrcInfo,
										PALETTEENTRY* pPalette, const BW::StringRef& allocator = "texture/unknown" );
		ComObjectWrap<DX::Texture>	createTextureFromFileEx( LPCTSTR pSrcFile, UINT Width, UINT Height, UINT MipLevels,
										DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, DWORD Filter, DWORD MipFilter,
										D3DCOLOR ColorKey, D3DXIMAGE_INFO * pSrcInfo, PALETTEENTRY * pPalette,
										const BW::StringRef& allocator = "texture/unknown" );
		ComObjectWrap<DX::CubeTexture>	createCubeTextureFromFileInMemoryEx(
											LPCVOID pSrcData, UINT SrcDataSize, UINT Size, UINT MipLevels, DWORD Usage,
											D3DFORMAT Format, D3DPOOL Pool, DWORD Filter, DWORD MipFilter, D3DCOLOR ColorKey,
											D3DXIMAGE_INFO* pSrcInfo, PALETTEENTRY* pPalette, const BW::StringRef& allocator = "texture/unknown" );
		ComObjectWrap<DX::Surface> createOffscreenPlainSurface(
											UINT Width, UINT Height, D3DFORMAT Format, D3DPOOL Pool,
											const BW::StringRef& allocator = "texture/unknown" );

		bool						mirroredTransform() const { return mirroredTransform_; }
		void						mirroredTransform(bool mirrored ) { mirroredTransform_ = mirrored; }

		bool						reflectionScene() const { return reflectionScene_; }
		void						reflectionScene(bool reflections ) { reflectionScene_ = reflections; }

		// whether rendering to shadowMap
		bool						dynamicShadowsScene() const { return isDynamicShadowsScene_; }
		void						dynamicShadowsScene(bool shadows ) { isDynamicShadowsScene_ = shadows; }

		OcclusionQuery*				createOcclusionQuery();
		void						destroyOcclusionQuery(OcclusionQuery*);

		void						beginQuery(OcclusionQuery*);
		void						endQuery(OcclusionQuery*);
		bool						getQueryResult(int& visiblePixels, OcclusionQuery*, bool wait);

		SimpleMutex&				getD3DXCreateMutex();
		HWND						windowHandle();

		DynamicIndexBufferInterface & dynamicIndexBufferInterface() const;

		void addPreloadResource(IDirect3DResource9* preloadObject);
		void preloadDeviceResources( uint32 timeLimitMs );
		void disableResourcePreloads();

		const DrawcallProfilingData&	lastFrameProfilingData() const { return lastFrameProfilingData_; }
		const DrawcallProfilingData&	liveProfilingData() const { return liveProfilingData_; }

		void						initRenderStates();
		void						invalidateStateCache() { cacheValidityId_++; }

		static D3DPOOL				patchD3DPool(D3DPOOL pool);
		bool						usingD3DDeviceEx() const;

		static void					forceNoExDevice();
		Moo::MRTSupport&			MRTSupport() {return( *mrtSupport_ );}

		EffectVisualContext&		effectVisualContext() const { return *effectVisualContext_; }

		bool						isHWInstancingAvailable() const;
	private:

		std::auto_ptr<FullscreenQuad>			fsQuad_;
		std::auto_ptr<CustomAA>					customAA_;
		ComObjectWrap< DX::Texture >	getTextureFromReuseList( 
											uint32 width, uint32 height, int nLevels,
											uint32 usage, D3DFORMAT fmt, D3DPOOL pool);

		/**
		 *	This class is a helper class for the RenderContext to push and pop the
		 *	current rendertarget.
		 */
		struct RenderTargetStack
		{
		public:
			bool push( class RenderContext* rc );
			bool pop( class RenderContext* rc );
			int nStackItems();

			void clear() { stackItems_.clear(); }
		private:
			class StackItem
			{
			public:
				StackItem() :
					cam_( 0.5f, 200.f, MATH_PI * 0.5f, 1.f ),
					zbufferSurface_( NULL )
				{
				}
				~StackItem()
				{
				}
				ComObjectWrap< DX::Surface >	renderSurfaces_[MAX_CONCURRENT_RTS];
				ComObjectWrap< DX::Surface >	zbufferSurface_;
				DX::Viewport					viewport_;
				Matrix							view_;
				Matrix							projection_;
				Camera							cam_;
			};

			typedef BW::vector< StackItem > StackItems;
			StackItems stackItems_;
		};

		D3DSURFACE_DESC				backBufferDesc_;

		D3DPRESENT_PARAMETERS		presentParameters_;
		RECT						windowedRect_;
		Vector2						windowedSize_;
		HWND						windowHandle_;
		bool						windowed_;
		bool						hideCursor_;
		LONG						windowedStyle_;
		bool						stencilWanted_;
		bool						stencilAvailable_;
		D3DDEVTYPE					deviceType_;
		uint32						deviceIndex_;
		uint32						modeIndex_;
		uint32						backBufferWidthOverride_;
		float						fullScreenAspectRatio_;
		BW::vector< DeviceInfo >	devices_;

		bool						mrtSupported_;
		void						createSecondSurface();
		bool						mixedVertexProcessing_;

		bool						isResetting_;

		bool						memoryCritical_;

		float						currentObjectID_;


		union
		{
			DX::Interface*		d3d_;
			DX::InterfaceEx*	d3dEx_;
		};

		union
		{
			DX::Device*			device_;
			DX::DeviceEx*		deviceEx_;
		};

		GpuInfo*				gpuInfo_;
		
		ComObjectWrap< DX::Surface >	screenCopySurface_;

		ComObjectWrap< DX::Surface >	renderTarget_[MAX_CONCURRENT_RTS];
		ComObjectWrap<DX::Texture>	secondRenderTargetTexture_;
		RenderTargetStack			renderTargetStack_;
		uint32						renderTargetCount_;

		Camera						camera_;
		Matrix						projection_;
		Matrix						view_;
		Matrix						viewProjection_;
		Matrix						lastViewProjection_;
		Matrix						invView_;
		Matrix						lodView_;
		Matrix						lodInvView_;

		float						projScale_;
		float						projXOffset_;
		float						projYOffset_;

		BW::deque< Matrix >			world_;

		LightContainerPtr			lightContainer_;
		LightContainerPtr			specularLightContainer_;

		uint32						primitiveGroupCount_;
		uint32						primitiveCount_;
		uint32						currentFrame_;

		float						halfScreenWidth_;
		float						halfScreenHeight_;

		bool						changeModePriv(
										uint32 modeIndex,
										bool   windowed,
										bool   testCooperative,
										uint32 backBufferWidthOverride);

		void						fillPresentationParameters( );
		void						updateDeviceInfo( );

		uint32						alphaOverride_;
		bool						depthOnly_;
		float						lodValue_;
		float						lodPower_;
		float						lodFar_;
		float						zoomFactor_;
		float						lodZoomFactor_;

		bool						waitForVBL_;
		bool						tripleBuffering_;

		uint16						psVersion_;
		uint16						vsVersion_;
		uint16                      maxSimTextures_;
		uint16                      maxAnisotropy_;
		float						gammaCorrection_;
		void						setGammaCorrection();

		float						maxZ_;

		uint						cacheValidityId_;

		RSCacheEntry				rsCache_[D3DRS_MAX];
		TSSCacheEntry				tssCache_[D3DFFSTAGES_MAX][D3DTSS_MAX];
		SampCacheEntry				sampCache_[D3DSAMPSTAGES_MAX][D3DSAMP_MAX];
		TextureCacheEntry			textureCache_[D3DSAMPSTAGES_MAX];

		uint						vertexDeclarationId_;
		DX::VertexDeclaration*		vertexDeclaration_;
		uint32						fvf_;

		int							beginSceneCount_;

		bool mirroredTransform_, reflectionScene_;

		bool 						isDynamicShadowsScene_;

		bool						paused_;

		BW::vector<OcclusionQuery*> queryList_;

		// Has this been initialised successfully?
		bool						isValid_;

		FogHelper					fogHelper_;
		
		// Used to prevent certain D3DXCreate operations in loading thread
		// from causing Direct3D9 error:
		// "Already in the state record mode. BeginStateBlock failed.".
		SimpleMutex					d3dxCreateMutex_;

		DynamicIndexBufferInterface * pDynamicIndexBufferInterface_;

		RenderContext(const RenderContext&);
		RenderContext& operator=(const RenderContext&);

		void releaseQueryObjects(void);
		void createQueryObjects(void);

		typedef BW::list<IDirect3DResource9*> PreloadResourceList;
		PreloadResourceList preloadResourceList_;
		SimpleMutex			preloadResourceMutex_;
		bool				enablePreloadResources_;

		DrawcallProfilingData		liveProfilingData_;
		DrawcallProfilingData		lastFrameProfilingData_;

		TextureReuseCache textureReuseCache_;

		bool assetProcessingOnly_;

		bool d3dDeviceExCapable_;
		bool usingD3DDeviceEx_;
		static bool forceDisableExDevice_;
		GraphicsSetting::GraphicsSettingPtr deviceExSettings_;
		void setDeviceExOption( int option );
		GraphicsSetting::GraphicsSettingPtr FXAASettings_;
		void setFXAAOption( int option );
		Moo::MRTSupport*	mrtSupport_;
		EffectVisualContext*	effectVisualContext_;
	};

	// Accessor function
	RenderContext& rc();
};

#ifdef CODE_INLINE
#include "render_context.ipp"
#endif

BW_END_NAMESPACE


#endif // RENDER_CONTEXT_HPP
