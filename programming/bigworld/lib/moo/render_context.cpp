#include "pch.hpp"
#include "render_context.hpp"

#include <algorithm>
#include <limits>
#include <sstream>
#include <iomanip>
#include <d3dx9.h>

#include "cstdmf/dogwatch.hpp"
#include "cstdmf/watcher.hpp"
#include "cstdmf/string_utils.hpp"

#include "math/math_extra.hpp"

#include "moo/texture_compressor.hpp"

#include "device_callback.hpp"

#include "vertex_declaration.hpp"
#include "vertex_formats.hpp"
#include "dynamic_vertex_buffer.hpp"
#include "dynamic_index_buffer.hpp"
#include "effect_state_manager.hpp"
#include "effect_visual_context.hpp"
#include "mrt_support.hpp"
#include "custom_AA.hpp"
#include "fullscreen_quad.hpp"
#include "visual_manager.hpp"
#include "primitive_manager.hpp"
#include "vertices_manager.hpp"
#include "animation_manager.hpp"
#include "moo/format_name_map.hpp"

DECLARE_DEBUG_COMPONENT2( "Moo", 0 )


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "render_context.ipp"
#endif

namespace // anonymous
{
    struct RenderThreadChecker
    {
    };

bool s_lost = false;
bool s_changingMode = false;

// File scope variables
HCURSOR s_OriginalMouseCursor = NULL;

/**
 * Output a string error message for some DirectX errors.
 */
void printErrorMsg_( HRESULT hr, const BW::string & msg )
{   
    switch ( hr )
    {
    case D3DERR_DRIVERINTERNALERROR:
        CRITICAL_MSG( "%s %s\n", msg.c_str(), DX::errorAsString( hr ).c_str() );
        break;
    default:
        WARNING_MSG( "%s %s\n", msg.c_str(), DX::errorAsString( hr ).c_str() );
        break;
    }
}

uint32 s_numPreloadResources = 0;
const size_t MAX_PRELOAD_COUNT = 2000;

} // namespace anonymous

BW_END_NAMESPACE


// comparison operators for d3ddisplaymodes
inline bool operator == ( const D3DDISPLAYMODE& dm1, const D3DDISPLAYMODE& dm2 )
{
    return ( ( dm1.Width == dm2.Width ) &&
        ( dm1.Height == dm2.Height ) &&
        ( dm1.Format == dm2.Format ) &&
        ( dm1.RefreshRate == dm2.RefreshRate ) );
}

inline bool operator != ( const D3DDISPLAYMODE& dm1, const D3DDISPLAYMODE& dm2 )
{
    return !( dm1 == dm2 );
}

inline bool operator < ( const D3DDISPLAYMODE& dm1, const D3DDISPLAYMODE& dm2 )
{
    if( dm1.Width < dm2.Width )
        return true;
    if( dm1.Width > dm2.Width )
        return false;
    if( dm1.Height < dm2.Height )
        return true;
    if( dm1.Height > dm2.Height )
        return false;
    if( dm1.Format < dm2.Format )
        return true;
    if( dm1.Format > dm2.Format )
        return false;
    if( dm1.RefreshRate < dm2.RefreshRate )
        return true;
    if( dm1.RefreshRate > dm2.RefreshRate )
        return false;
    return false;
}


BW_BEGIN_NAMESPACE

namespace Moo
{

extern class RenderContext* g_RC;

/*static*/ bool RenderContext::forceDisableExDevice_( false );
/*static*/ const uint16 RenderContext::SHADER_VERSION_NOT_INITIALISED =
        std::numeric_limits< uint16 >::max();

/**
 *
 */
class OcclusionQuery
{
public:
    int                 index;
    IDirect3DQuery9*    queryObject;
private:
};

THREADLOCAL( bool ) g_renderThread = false;

/**
 *  This method sets up the gamma correction value on the device.
 */
void RenderContext::setGammaCorrection()
{
    BW_GUARD;
    gammaCorrection_ = max( 0.5f, min( 6.f, gammaCorrection_ ) );
    if (device_)
    {
        D3DGAMMARAMP ramp;

        for (uint32 i = 0; i < 256; i++)
        {
            float f = /*1.f -*/ (float(i) / 255.f);
            ramp.red[i] = ramp.green[i] = ramp.blue[i] = WORD( ( /*1.f -*/ powf( f, 1.f / gammaCorrection_ ) ) * 65535.f );
        }

        device_->SetGammaRamp( 0, D3DSGR_CALIBRATE, &ramp );
    }
}


/**
 *  The render contexts constructor, prepares to create a dx device, and
 *  sets default values.
 */
RenderContext::RenderContext() :
  d3d_( NULL ),
  device_( NULL ),
  camera_( 0.25, 500, DEG_TO_RAD( 60 ), 4.f/3.f ),
  windowed_( true ),
  hideCursor_( true ),
  windowedStyle_( WS_OVERLAPPEDWINDOW ),
  currentFrame_( 0 ),
  primitiveGroupCount_( 0 ),
  halfScreenWidth_( 320.f ),
  halfScreenHeight_( 240.f ),
  fullScreenAspectRatio_( 4.f/3.f ),
  alphaOverride_( 0xff000000 ),
  depthOnly_( false ),
  lodValue_( 1.f ),
  lodFar_( 400 ),
  lodPower_( 10 ),
  zoomFactor_( 1 ),
  lodZoomFactor_( 1 ),
  stencilWanted_( false ),
  stencilAvailable_( false ),
  gammaCorrection_( 1.f ),
  maxZ_( 1.f ),
  cacheValidityId_( 0 ),
  waitForVBL_( true ),
  tripleBuffering_( true ),
  vsVersion_( SHADER_VERSION_NOT_INITIALISED ),
  psVersion_( SHADER_VERSION_NOT_INITIALISED ),
  mixedVertexProcessing_( false ),
  isResetting_( false ),
  mrtSupported_( false ),
  mirroredTransform_(false),
  reflectionScene_(false),
  isDynamicShadowsScene_(false),
  paused_( false ),
  memoryCritical_( false ),
  gpuInfo_( NULL ),
  renderTargetCount_( 0 ),
  backBufferWidthOverride_( 0 ),
  beginSceneCount_(0),
  isValid_( false ),
  pDynamicIndexBufferInterface_( NULL ),
  enablePreloadResources_( false ),
  assetProcessingOnly_( false ),
  d3dDeviceExCapable_( false ),
  usingD3DDeviceEx_( false ),
  deviceExSettings_( NULL ),
  FXAASettings_( NULL ),
  mrtSupport_( NULL ),
  effectVisualContext_( NULL )
{
    BW_GUARD;
    view_.setIdentity();
    projection_.setIdentity();
    viewProjection_.setIdentity();
    lastViewProjection_.setIdentity();
    invView_.setIdentity();
    world_.push_back( projection_ );

    renderTarget_[0] = NULL;
    renderTarget_[1] = NULL;

    Moo::FogParams params = Moo::FogHelper::pInstance()->fogParams();
    params.m_start = 0;
    params.m_end = 500;
    params.m_color = 0x000000FF;
    Moo::FogHelper::pInstance()->fogParams( params );
}

namespace
{

IDirect3D9Ex* createD3DEx()
{
    // Try to create the directx9ex interface
    // it will return valid pointer on Vista+ and return NULL on 
    IDirect3D9Ex *pD3D = NULL;
    HMODULE hDll = LoadLibrary( L"d3d9.dll" );
    if ( hDll )
    {
        typedef HRESULT (WINAPI *LPDIRECT3DCREATE9EX)( UINT,
        IDirect3D9Ex**);

        LPDIRECT3DCREATE9EX func = (LPDIRECT3DCREATE9EX)GetProcAddress( hDll,
        "Direct3DCreate9Ex" );

        if ( func )
        {
            if ( FAILED( func( D3D_SDK_VERSION, &pD3D ) ) )
            {
                FreeLibrary( hDll );
                pD3D = NULL;
            }
        }
        FreeLibrary( hDll );
    }
    return pD3D;
}

}


/**
 *  Graphics setting changed.
 *  @param option new graphics setting option.
 */
void RenderContext::setDeviceExOption( int option )
{
}


/**
 *  Graphics setting changed.
 *  @param option new graphics setting option.
 */
void RenderContext::setFXAAOption( int option )
{
    if(customAA_.get())
    {
        customAA_->mode(static_cast<uint>(option));
        customAA_->apply();
    }
}


bool RenderContext::init( bool d3dExInterface, bool assetProcessingOnly )
{
    BW_GUARD;
    IF_NOT_MF_ASSERT_DEV( isValid_ == false && "RenderContext already initialised")
    {
        return true;
    }

    assetProcessingOnly_ = assetProcessingOnly;

    // Check if Ex device is supported
    IDirect3D9Ex* d3dExTmp = NULL;
    if (!forceDisableExDevice_)
    {
        d3dExTmp = createD3DEx();
        if (d3dExTmp != NULL)
        {
            d3dDeviceExCapable_ = true;
        }
    }

    // Add graphics setting for Ex device
    deviceExSettings_ = Moo::makeCallbackGraphicsSetting(
        "DEVICE_EX",
        "Direct3D 9Ex Device",
        *this, 
        &RenderContext::setDeviceExOption, 
        d3dDeviceExCapable_ ? 0 : 1,
        false,
        true );
    deviceExSettings_->addOption( "ON",  "On", d3dDeviceExCapable_ );
    deviceExSettings_->addOption( "OFF", "Off", true );
    Moo::GraphicsSetting::add( deviceExSettings_ );

    // Add graphics setting for FXAA
    FXAASettings_ = Moo::makeCallbackGraphicsSetting(
        "FXAA_PROCESSING",
        "FXAA Processing",
        *this, 
        &RenderContext::setFXAAOption, 
        -1,
        false,
        false );
    FXAASettings_->addOption( "OFF", "Off", true );
    FXAASettings_->addOption( "LOW", "Low Quality", true );
    FXAASettings_->addOption( "MED", "Medium Quality", true );
    FXAASettings_->addOption( "HIGH", "High Quality", true );
    Moo::GraphicsSetting::add( FXAASettings_ ); 

    // Check option
    // pass this in from BWMain since GraphicsSettings aren't init until later
    usingD3DDeviceEx_ = d3dExInterface && d3dDeviceExCapable_;

    // Try to use the directx9ex interface
    // should only work on Windows Vista +
    HRESULT hr = S_OK;
    d3dEx_ = NULL;
    if (usingD3DDeviceEx_)
    {
        d3dEx_ = d3dExTmp;
    }
    // fallback to non ex d3ddevice
    if (d3dEx_ == NULL)
    {
        usingD3DDeviceEx_ = false;
        d3d_ = Direct3DCreate9( D3D_SDK_VERSION );
    }

    if ( d3d_ )
    {
        // Reduce the reference count, because d3ds_.pComobject( ) increases it.

        // Iterate through the adapters.
        for (uint32 adapterIndex = 0; adapterIndex < d3d_->GetAdapterCount(); adapterIndex++)
        {
            DeviceInfo deviceInfo;
            deviceInfo.windowed_ = false;
            deviceInfo.adapterID_ = adapterIndex;
            if (D3D_OK == d3d_->GetAdapterIdentifier( adapterIndex, 0, &deviceInfo.identifier_ ))
            {
                if (D3D_OK == d3d_->GetDeviceCaps( adapterIndex, D3DDEVTYPE_HAL, &deviceInfo.caps_ ))
                {
                    const uint32 VENDOR_ID_ATI = 0x1002;
                    const uint32 VENDOR_ID_NVIDIA = 0x10de;

                    // set up compatibilty flags for special rendering codepaths
                    deviceInfo.compatibilityFlags_ = 0;
                    if (strstr(deviceInfo.identifier_.Description, "GeForce4 MX") != NULL)
                    {
                        // this card does not support D3DLOCK_NOOVERWRITE in the way flora uses it
                        deviceInfo.compatibilityFlags_ |= COMPATIBILITYFLAG_NOOVERWRITE;
                        NOTICE_MSG("GeForce4 MX compatibility selected\n");
                    }

                    if (deviceInfo.identifier_.VendorId == VENDOR_ID_NVIDIA)
                    {
                        // this card does not support D3DLOCK_NOOVERWRITE in the way flora uses it
                        deviceInfo.compatibilityFlags_ |= COMPATIBILITYFLAG_NVIDIA;
                        NOTICE_MSG("nVidia compatibility selected\n");
                    }

                    if (deviceInfo.identifier_.VendorId == VENDOR_ID_ATI)
                    {
                        // this card does not support D3DLOCK_NOOVERWRITE in the way flora uses it
                        deviceInfo.compatibilityFlags_ |= COMPATIBILITYFLAG_ATI;
                        NOTICE_MSG("ATI compatibility selected\n");
                    }

                    //-- compatibility for deferred shading pipeline.
                    {
                        bool supported = true;

                        supported &= (deviceInfo.caps_.VertexShaderVersion >= 0x300);
                        supported &= (deviceInfo.caps_.PixelShaderVersion >= 0x300);
                        supported &= (deviceInfo.caps_.NumSimultaneousRTs >= 3);
                        supported &= (deviceInfo.caps_.PrimitiveMiscCaps & D3DPMISCCAPS_INDEPENDENTWRITEMASKS) != 0;
                        //supported &= SUCCEEDED(d3d_->CheckDeviceFormat(deviceInfo.adapterID_, D3DDEVTYPE_HAL, D3DFMT_A8R8G8B8, 0, D3DRTYPE_TEXTURE, D3DFMT_A16B16G16R16F));

                        if (supported)
                        {
                            deviceInfo.compatibilityFlags_ |= COMPATIBILITYFLAG_DEFERRED_SHADING;
                            NOTICE_MSG("Deferred shading pipeline compatibile\n");
                        }
                    }

                    D3DFORMAT formats[] = { D3DFMT_X8R8G8B8, D3DFMT_A8R8G8B8, D3DFMT_A2B10G10R10, D3DFMT_X1R5G5B5, D3DFMT_A1R5G5B5, D3DFMT_R5G6B5 };
                    uint32 nFormats = sizeof( formats ) / sizeof(D3DFORMAT);

                    for (uint32 fi = 0; fi < nFormats; fi++)
                    {
                        D3DFORMAT format = formats[fi];
                        // Iterate through adapter modes
                        for (uint32 displayModeIndex = 0; displayModeIndex < d3d_->GetAdapterModeCount( adapterIndex, format ); displayModeIndex++)
                        {
                            D3DDISPLAYMODE mode;
                            if (D3D_OK == d3d_->EnumAdapterModes( adapterIndex, format, displayModeIndex, &mode ))
                            {
                                mode.RefreshRate = 0;
                                if( std::find( deviceInfo.displayModes_.begin(), deviceInfo.displayModes_.end(), mode ) == deviceInfo.displayModes_.end() )
                                {
                                    if( mode.Width >= 640 && mode.Height >= 480 )
                                    {
                                        deviceInfo.displayModes_.push_back( mode );
                                    }
                                }
                            }
                        }
                    }
                    deviceInfo.windowed_ = true; //( deviceInfo.caps_.Caps2 & D3DCAPS2_CANRENDERWINDOWED ) != 0;
                }

                if (deviceInfo.displayModes_.size() != 0 || deviceInfo.windowed_ == true)
                {

                    if( deviceInfo.windowed_ )
                    {
                        d3d_->GetAdapterDisplayMode( deviceInfo.adapterID_, &deviceInfo.windowedDisplayMode_ );
                    }

                    std::sort( deviceInfo.displayModes_.begin(), deviceInfo.displayModes_.end() );
                    devices_.push_back( deviceInfo );
                }
            }
        }

        // New GpuInfo object for querying memory usage.
        gpuInfo_ = new GpuInfo();

        if (devices_.size())
        {
            // We succedeed in initialising the RenderContext.
            isValid_ = true;
        }
        else
        {
            ERROR_MSG( "Moo::RenderContext::RenderContext: No hardware rasterisation devices found.\n" );
        }
    }
    else
    {
        ERROR_MSG( "Moo::RenderContext::RenderContext: Unable to create Directx interface. Is Directx9c installed?\n" );
    }

    return isValid_;
}

void RenderContext::releaseUnmanaged( bool forceRelease )
{
    BW_GUARD;
    renderTargetStack_.clear();
    renderTarget_[0] = NULL;
    renderTarget_[1] = NULL;
    secondRenderTargetTexture_ = NULL;

    DeviceCallback::deleteAllUnmanaged( forceRelease );
    releaseQueryObjects();

    SimpleMutexHolder smh(preloadResourceMutex_);
    while (!preloadResourceList_.empty())
    {
        preloadResourceList_.back()->Release();
        preloadResourceList_.pop_back();
    }

    memoryCritical_=false;
}

void RenderContext::createUnmanaged( bool forceCreate /*= false*/ )
{
    BW_GUARD;

    //-- apply already selected custom anti-aliasing mode.
    setFXAAOption(FXAASettings_->activeOption());

    DeviceCallback::createAllUnmanaged( forceCreate );
    createQueryObjects();
    if (!renderTarget_[0].hasComObject())
    {
        device_->GetRenderTarget(0, &renderTarget_[0]);
        device_->SetRenderTarget(0, renderTarget_[0].pComObject());
    }
    if (mrtSupport_->isEnabled())
    {
        this->createSecondSurface();
        device_->SetRenderTarget(1, renderTarget_[1].pComObject());
    }
}

/**
 *  This method creates the second surface if we are doing MRT.
 */
void RenderContext::createSecondSurface()
{
    renderTarget_[1] = NULL;
    secondRenderTargetTexture_ = NULL;

    //The PC can only create standard 32bit colour render targets.
    secondRenderTargetTexture_ =  this->createTexture(
        (UINT)this->screenWidth(),
        (UINT)this->screenHeight(),
        1,
        D3DUSAGE_RENDERTARGET,
        D3DFMT_A8R8G8B8, 
        D3DPOOL_DEFAULT,
        "texture/render target/second_main_surface" );

    if (secondRenderTargetTexture_.hasComObject())
    {
        HRESULT hr = (secondRenderTargetTexture_->GetSurfaceLevel( 0, &renderTarget_[1] ));
        if ( FAILED( hr ) )
        {
            WARNING_MSG( "Failed to get second RT surface\n" );
            return;
        }
        else
        {
            INFO_MSG( "Created Second RT Surface\n" );
        }
    }
    else
    {
        WARNING_MSG( "Failed to create texture for second RT surface\n" );
    }
}


/**
 *  This method changes the current display mode or windowed/fullscreen
 *  mode on the current device.
 *
 *  @param modeIndex is the index of the displaymode to choose.
 *  @param windowed tells the rendercontext whether to use windowed mode or not.
 *  @param testCooperative if true, tests the device's cooperative level before
 *         changing the mode. The mode is not changed if the device is lost.
 *  @param backBufferWidthOverride  Width to set the back buffer to. 0
 *                                  indicates same width as the visible display.
 *
 *
 *  @return true if the operation was successful, false otherwise.
 */
bool RenderContext::changeMode( uint32 modeIndex, bool windowed, bool testCooperative, uint32 backBufferWidthOverride )
{
    BW_GUARD;
    // Avoid changeMode to be called recursively from 
    // App::resizeWindow. resizeWindow will be triggered 
    // by the SetWindowPos call when going windowed.
    IF_NOT_MF_ASSERT_DEV(!s_changingMode)
    {
        return false;
    }

    s_changingMode = true;

    bool goingFullScreen = windowed != windowed_ && windowed == false;
    bool goingWindowMode = windowed != windowed_ && windowed == true;

    // we don't want to get the current window size if we are currently fullscreen
    //  and are going to windowed mode, as all we will be getting will be the rectangle of the fullscreen window
    if(goingWindowMode == false)
    {
        GetWindowRect( windowHandle_, &windowedRect_ );
    }

    if (goingFullScreen)
    {
        // save the windowed style to restore it later, and set it to
        // WS_POPUP so the fullscreen window doesn't have a caption.
        windowedStyle_ = GetWindowLong( windowHandle_, GWL_STYLE );
        SetWindowLong( windowHandle_, GWL_STYLE, WS_POPUP );
    }
    else if (goingWindowMode)
    {
        // restore the window's windowed style
        SetWindowLong( windowHandle_, GWL_STYLE, windowedStyle_ );
    }

    // switch 
    releaseUnmanaged( !usingD3DDeviceEx_ );
    bool result = this->changeModePriv(
        modeIndex, windowed, testCooperative, 
        backBufferWidthOverride);

    if (result && goingWindowMode)
    {
        // If going back to windowed mode, resize it.
        SetWindowPos( windowHandle_, HWND_NOTOPMOST,
                      windowedRect_.left, windowedRect_.top,
                      ( windowedRect_.right - windowedRect_.left ),
                      ( windowedRect_.bottom - windowedRect_.top ),
                      SWP_SHOWWINDOW );

        // finally, adjust backbuffer size to window
        // equivalent to calling resetDevice, but we
        // don't want to call createAllUnmanaged and 
        // createAllUnmanaged again.
        result = this->changeModePriv(modeIndex_, windowed_, true, 0);
    }
    if (result)
    {
        createUnmanaged( !usingD3DDeviceEx_ );
    }

    s_changingMode = false;
    return result;
}

/**
 *  This method resets the device to the exact same current modeIndex and windowed
 *  mode. Use it whenever the underlying window is resized to adjust the size of the 
 *  frame buffers.
 */
bool RenderContext::resetDevice()
{
    BW_GUARD;
    BWResource::WatchAccessFromCallingThreadHolder holder( false );
    if (s_changingMode)
    {
        return false;
    }

    isResetting_ = true;

    //  store the current window size, so it can be used to check to see if the window has changed in rendercontext::checkdevice() to do a reset there
    //  but only if we are in windowed mode, not fullscreen.
    if(windowed_ == true)   
        GetWindowRect( windowHandle_, &windowedRect_ );

    releaseUnmanaged( !usingD3DDeviceEx_ );

    bool result = this->changeModePriv(modeIndex_, windowed_, true, 0);
    if (result)
    {
        createUnmanaged( !usingD3DDeviceEx_ );
    }

    isResetting_ = false;

    return result;
}

/**
 *  This method resets the current display mode or windowed/fullscreen
 *  mode on the current device. It differs from changeMode by not reseting the 
 *
 *  @param modeIndex is the index of the displaymode to choose.
 *  @param windowed tells the rendercontext whether to use windowed mode or not.
 *  @param testCooperative if true, tests the device's cooperative level before
 *         changing the mode. The mode is not changed if the device is lost.
 *  @param backBufferWidthOverride  Width to set the back buffer to. 0
 *                                  indicates same width as the visible display.
 *
 *  @return true if the operation was successful.
 */
bool RenderContext::changeModePriv( uint32 modeIndex, bool windowed, bool testCooperative, uint32 backBufferWidthOverride )
{
    BW_GUARD;
    if (testCooperative && 
        Moo::rc().device()->TestCooperativeLevel() == D3DERR_DEVICELOST)
    {
        return false;
    }

    screenCopySurface_ = NULL;
    IF_NOT_MF_ASSERT_DEV( device_ )
    {
        return false;
    }

    if (modeIndex != -1)
    {
        modeIndex_ = modeIndex;
        windowed_  = windowed;
    }
    backBufferWidthOverride_ = backBufferWidthOverride;

    // are we changing windowed status?
    fillPresentationParameters();

    // Some drivers have difficulty in restoring a device for a minimised 
    // window unless the width and height of the window are set in the 
    // presentation parameters.
    WINDOWPLACEMENT placement;
    GetWindowPlacement(windowHandle_, &placement);
    if (backBufferWidthOverride_ == 0 && placement.showCmd == SW_SHOWMINIMIZED)
    {
        RECT &rect = placement.rcNormalPosition;
        presentParameters_.BackBufferWidth  = abs(rect.right  - rect.left);
        presentParameters_.BackBufferHeight = abs(rect.bottom - rect.top );
    }

    if (!usingD3DDeviceEx_)
    {
        HRESULT hr = D3DERR_DEVICELOST;
        while( hr == D3DERR_DEVICELOST )
        {
            INFO_MSG( "RenderContext::changeMode - trying to reset \n" );
            hr = device_->Reset( &presentParameters_ );

            s_lost = true;
            if( FAILED( hr ) && hr != D3DERR_DEVICELOST )
            {
                printErrorMsg_( hr, "Moo::RenderContext::changeMode: Unable to reset device" );
                return false;
            }
            Sleep( 100 );
        }
    }
    else
    {
        D3DDISPLAYMODEEX dispModeDesc = { sizeof( dispModeDesc ) };

        D3DDISPLAYMODEEX* pFullScreenDesc = &dispModeDesc;
        if (windowed)
        {
            pFullScreenDesc = NULL;
        }
        else
        {
            dispModeDesc.Width = presentParameters_.BackBufferWidth;
            dispModeDesc.Height = presentParameters_.BackBufferHeight;
            dispModeDesc.Format = presentParameters_.BackBufferFormat;
            dispModeDesc.RefreshRate = presentParameters_.FullScreen_RefreshRateInHz;
            dispModeDesc.ScanLineOrdering = D3DSCANLINEORDERING_PROGRESSIVE;
        }
        deviceEx_->ResetEx( &presentParameters_, pFullScreenDesc );
    }

    s_lost = false;

    beginSceneCount_ = 0;
    UINT availTexMem = device_->GetAvailableTextureMem();
    INFO_MSG( "RenderContext::changeMode - available texture memory after reset: %dkb\n", availTexMem/1024 );

    updateDeviceInfo();
    updateProjectionMatrix();
    initRenderStates();
    if (mixedVertexProcessing_)
        device_->SetSoftwareVertexProcessing( TRUE );

    // save windowed mode size
    if (windowed_)
    {
        RECT clientRect;
        GetClientRect( windowHandle_, &clientRect );
        this->windowedSize_ = Vector2(
            float(clientRect.right - clientRect.left),
            float(clientRect.bottom - clientRect.top));
    }

    return true;
}


/*
 *  this method updates all the info that can be queried from
 *  outside the rendercontext.
 */
void RenderContext::updateDeviceInfo( )
{
    BW_GUARD;
    DX::Surface*  pBackBuffer;
    device_->GetBackBuffer( 0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer );
    pBackBuffer->GetDesc( &backBufferDesc_ );
    pBackBuffer->Release();

    halfScreenWidth_ = float( backBufferDesc_.Width ) * 0.5f;
    halfScreenHeight_ = float( backBufferDesc_.Height ) * 0.5f;

    DX::Surface* surf = NULL;

    D3DDISPLAYMODE smode;
    d3d_->GetAdapterDisplayMode( devices_[ deviceIndex_ ].adapterID_, &smode );

    HRESULT hr;
    if ( SUCCEEDED( hr = device_->CreateOffscreenPlainSurface( backBufferDesc_.Width, backBufferDesc_.Height, 
        D3DFMT_A8R8G8B8, D3DPOOL_SCRATCH, &surf, NULL ) ) )
    {
        screenCopySurface_.pComObject( surf );
        surf->Release();
    }
    else
    {
        screenCopySurface_.pComObject( NULL );
    }

    DEBUG_MSG( "Moo: Changed mode: %d %d\n", backBufferDesc_.Width, backBufferDesc_.Height );
}


/*
 *  This method sets up the presentation parameters, for a reset or a createdevice call.
 */
void RenderContext::fillPresentationParameters( void )
{
    BW_GUARD;
    ZeroMemory( &presentParameters_, sizeof( presentParameters_ ) );

    if( windowed_ )
    {
        presentParameters_.BackBufferWidth = backBufferWidthOverride_;
        presentParameters_.BackBufferHeight = static_cast<uint32>( backBufferWidthOverride_ * ( screenHeight() / screenWidth() ) );
        presentParameters_.BackBufferFormat = devices_[ deviceIndex_ ].windowedDisplayMode_.Format;
        presentParameters_.PresentationInterval = waitForVBL_ ? D3DPRESENT_INTERVAL_ONE : D3DPRESENT_INTERVAL_IMMEDIATE;
    }
    else
    {
        D3DDISPLAYMODE mode = devices_[ deviceIndex_ ].displayModes_[ modeIndex_ ];
        presentParameters_.BackBufferWidth = mode.Width;
        presentParameters_.BackBufferHeight = mode.Height;
        presentParameters_.BackBufferFormat = mode.Format;
        presentParameters_.PresentationInterval = waitForVBL_ ? D3DPRESENT_INTERVAL_DEFAULT : D3DPRESENT_INTERVAL_IMMEDIATE;
    }

    if ( presentParameters_.BackBufferFormat == D3DFMT_X8R8G8B8 )
    {
        if (SUCCEEDED(d3d_->CheckDeviceType(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8, D3DFMT_A8R8G8B8, windowed_)))
        {
            presentParameters_.BackBufferFormat = D3DFMT_A8R8G8B8;
            INFO_MSG( "This adapter supports A8R8G8B8 back buffers\n" );
        }   
        else
        {
            INFO_MSG( "This adapter does not support A8R8G8B8 back buffers\n" );
        }
    }

    presentParameters_.Windowed = windowed_;
    presentParameters_.BackBufferCount = tripleBuffering_ ? 2 : 1;
    presentParameters_.SwapEffect = D3DSWAPEFFECT_DISCARD;
    presentParameters_.MultiSampleType = D3DMULTISAMPLE_NONE;
    presentParameters_.EnableAutoDepthStencil = TRUE;
    if (!s_lost)
        presentParameters_.AutoDepthStencilFormat = 
            getMatchingZBufferFormat( presentParameters_.BackBufferFormat, 
                                      stencilWanted_, stencilAvailable_ );
    else if (stencilAvailable_)
        presentParameters_.AutoDepthStencilFormat = D3DFMT_D24S8;
    else
        presentParameters_.AutoDepthStencilFormat = D3DFMT_D16;
    presentParameters_.Flags = 0;
    presentParameters_.hDeviceWindow = windowHandle_;
}


/**
 *  This method releases the rendercontexts reference to a d3d device, and
 *  tries to clean up all references to d3d objects.
 */
void RenderContext::releaseDevice( void )
{
    BW_GUARD;
    if ( hideCursor_ )
    {
        ShowCursor( TRUE );
        SetCursor( s_OriginalMouseCursor );
    }

    lightContainer_ = NULL;
    specularLightContainer_ = NULL;

    screenCopySurface_ = NULL;

    if (!assetProcessingOnly_)
    {
        MF_ASSERT( mrtSupport_ );
        mrtSupport_->fini();
        bw_safe_delete( mrtSupport_ );
    }

    this->releaseUnmanaged( true ); 
    DeviceCallback::deleteAllManaged();

    // Release all device objects
    if (device_)
    {
        clearBindings();

        //reset window if it is not the correct size
        if( !windowed_ )
        {
            SetWindowLong( windowHandle_, GWL_STYLE, windowedStyle_ );
            SetWindowPos( windowHandle_, HWND_NOTOPMOST,
                          windowedRect_.left, windowedRect_.top,
                          ( windowedRect_.right - windowedRect_.left ),
                          ( windowedRect_.bottom - windowedRect_.top ),
                          SWP_SHOWWINDOW );
        }
    }

    // Release the render device
    device_->Release();
    device_ = NULL;
}

/**
 *  Gets amount of available texture memory.
 */
uint RenderContext::getAvailableTextureMem( ) const
{
    BW_GUARD;
    return device_->GetAvailableTextureMem();
}

/**
 *  Gets Gpu memory info.
 */
void RenderContext::getGpuMemoryInfo( GpuInfo::MemInfo* outMemInfo ) const
{
    assert( outMemInfo != NULL );
    uint32 adapter = devices_[ deviceIndex_ ].adapterID_;
    gpuInfo_->getMemInfo( outMemInfo, adapter );
}

/**
 *  Get a zbuffer format that is allowed with the current render format.
 */
D3DFORMAT RenderContext::getMatchingZBufferFormat( D3DFORMAT colourFormat, bool stencilWanted, bool & stencilAvailable )
{
    BW_GUARD;
    uint32 adapter = devices_[ deviceIndex_ ].adapterID_;
    D3DFORMAT format = this->adapterFormat();

    if( colourFormat == D3DFMT_R8G8B8 || colourFormat == D3DFMT_X8R8G8B8 || colourFormat == D3DFMT_A8R8G8B8 )
    {
        if ( !stencilWanted )
        {
            if( SUCCEEDED( d3d_->CheckDeviceFormat( adapter, deviceType_, format, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE, D3DFMT_D32 ) ) &&
                SUCCEEDED( d3d_->CheckDepthStencilMatch( adapter, deviceType_, format, colourFormat, D3DFMT_D32 ) ) )
                return D3DFMT_D32;
            if( SUCCEEDED( d3d_->CheckDeviceFormat( adapter, deviceType_, format, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE, D3DFMT_D24X8 ) ) &&
                SUCCEEDED( d3d_->CheckDepthStencilMatch( adapter, deviceType_, format, colourFormat, D3DFMT_D24X8 ) ) )
                return D3DFMT_D24X8;
            if( SUCCEEDED( d3d_->CheckDeviceFormat( adapter, deviceType_, format, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE, D3DFMT_D24S8 ) ) &&
                SUCCEEDED( d3d_->CheckDepthStencilMatch( adapter, deviceType_, format, colourFormat, D3DFMT_D24S8 ) ) )
                return D3DFMT_D24S8;
            if( SUCCEEDED( d3d_->CheckDeviceFormat( adapter, deviceType_, format, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE, D3DFMT_D24X4S4 ) ) &&
                SUCCEEDED( d3d_->CheckDepthStencilMatch( adapter, deviceType_, format, colourFormat, D3DFMT_D24X4S4 ) ) )
                return D3DFMT_D24X4S4;
        }
        else
        {
            if( SUCCEEDED( d3d_->CheckDeviceFormat( adapter, deviceType_, format, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE, D3DFMT_D24S8 ) ) &&
                SUCCEEDED( d3d_->CheckDepthStencilMatch( adapter, deviceType_, format, colourFormat, D3DFMT_D24S8 ) ) )
            {
                stencilAvailable = true;
                return D3DFMT_D24S8;
            }
            else
            {
                stencilAvailable = false;
            }
        }
    }

    if ( !stencilWanted )
    {
        if( SUCCEEDED( d3d_->CheckDeviceFormat( adapter, deviceType_, format, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE, D3DFMT_D16 ) ) &&
            SUCCEEDED( d3d_->CheckDepthStencilMatch( adapter, deviceType_, format, colourFormat, D3DFMT_D16 ) ) )
            return D3DFMT_D16;
        if( SUCCEEDED( d3d_->CheckDeviceFormat( adapter, deviceType_, format, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE, D3DFMT_D15S1 ) ) &&
            SUCCEEDED( d3d_->CheckDepthStencilMatch( adapter, deviceType_, format, colourFormat, D3DFMT_D15S1 ) ) )
            return D3DFMT_D15S1;
        if( SUCCEEDED( d3d_->CheckDeviceFormat( adapter, deviceType_, format, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE, D3DFMT_D24X8 ) ) &&
            SUCCEEDED( d3d_->CheckDepthStencilMatch( adapter, deviceType_, format, colourFormat, D3DFMT_D24X8 ) ) )
            return D3DFMT_D24X8;
        if( SUCCEEDED( d3d_->CheckDeviceFormat( adapter, deviceType_, format, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE, D3DFMT_D32 ) ) &&
            SUCCEEDED( d3d_->CheckDepthStencilMatch( adapter, deviceType_, format, colourFormat, D3DFMT_D32 ) ) )
            return D3DFMT_D32;
        if( SUCCEEDED( d3d_->CheckDeviceFormat( adapter, deviceType_, format, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE, D3DFMT_D24S8 ) ) &&
            SUCCEEDED( d3d_->CheckDepthStencilMatch( adapter, deviceType_, format, colourFormat, D3DFMT_D24S8 ) ) )
            return D3DFMT_D24S8;
        if( SUCCEEDED( d3d_->CheckDeviceFormat( adapter, deviceType_, format, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE, D3DFMT_D24X4S4 ) ) &&
            SUCCEEDED( d3d_->CheckDepthStencilMatch( adapter, deviceType_, format, colourFormat, D3DFMT_D24X4S4 ) ) )
            return D3DFMT_D24X4S4;
    }
    else
    {
        if( SUCCEEDED( d3d_->CheckDeviceFormat( adapter, deviceType_, format, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE, D3DFMT_D24S8 ) ) &&
            SUCCEEDED( d3d_->CheckDepthStencilMatch( adapter, deviceType_, format, colourFormat, D3DFMT_D24S8 ) ) )
        {
            stencilAvailable = true;
            return D3DFMT_D24S8;
        }
        else
        {
            stencilAvailable = false;
        }
    }
    return D3DFMT_D16;
}

/**
 *  This method updates the projection matrix based on the current camera.
 */
void RenderContext::updateProjectionMatrix( bool detectAspectRatio )
{
    BW_GUARD;
    if ( detectAspectRatio && !renderTargetStack_.nStackItems() )
    {
        if( !windowed_ )
        {
            camera_.aspectRatio( fullScreenAspectRatio_ );
        }
        else
        {
            camera_.aspectRatio( float( backBufferDesc_.Width ) / float( backBufferDesc_.Height ) );
        }
    }

    zoomFactor_ = camera_.fov() / DEG_TO_RAD( 60 );

    projection_.perspectiveProjection( camera_.fov(), camera_.aspectRatio(), camera_.nearPlane(), camera_.farPlane() );
}

/**
 *  This method checks the device's cooperative level. It returns true if
 *  the device is operational. If it has been lost, it tries to reset it,
 *  returning true if successful. It returns false if the device was lost 
 *  cannot be reset immediatly. CRITICAL_MSG will be triggered if the 
 *  device has entered an unrecoverable state.  
 *
 *  @param      reset set to true if the device was reset.
 *  @return     true if device is operational, false otherwise.
 */
bool RenderContext::checkDevice(bool *reset /*=NULL*/)
{
    BW_GUARD;
    PROFILER_SCOPED(checkDevice);
    bool result = false;
    if (reset != NULL)
        *reset = false;
    HRESULT hr = Moo::rc().device()->TestCooperativeLevel();
    RECT windowedRect;

    GetWindowRect( windowHandle_, &windowedRect );

    switch (hr)
    {
        case D3D_OK:
            result = true;
            break;
        case D3DERR_DEVICELOST:
            result = false;
            break;
        case D3DERR_DEVICENOTRESET:
        {
            s_lost = true;
            result = Moo::rc().resetDevice();
            if (reset != NULL)
                *reset = result;
            break;
        }
        case D3DERR_DRIVERINTERNALERROR:
            CRITICAL_MSG("Internal Driver Error");
            break;
        case D3DERR_OUTOFVIDEOMEMORY:
            CRITICAL_MSG("D3DERR_OUTOFVIDEOMEMORY");
            break;
        default:
            INFO_MSG( "RenderContext::checkDevice - UNHANDLED D3D RESULT : %ld\n", hr );
            break;
    }
    s_lost = !result;
    return result;
}

/**
 *  Tests the cooperative level of the device, seizing control if the 
 *  device has been lost. In this case, starts retesting the device 
 *  cooperative level, at a fixed time intervals, until it can be reset. 
 *  When that finally happens, resets the device and returns true. 
 *
 *  Messages requesting the application to close will make the function 
 *  return false. All Other messages are translated and dispatched to the 
 *  registered WinProc function. 
 *
 *  CRITICAL_MSG will be triggered if the device enters an unrecoverable 
 *  state.  
 *
 *  @param      reset set to true if the device was reset.
 *  @return     true if device is operational. False if a WM_QUIT 
 *              message or ALT+F4 key event has been intercepted.
 */ 
bool RenderContext::checkDeviceYield(bool *reset /*= NULL*/)
{
    BW_GUARD;
    bool result = true;
    while (!(result = this->checkDevice(reset)))
    {
        MSG msg;
        if (::PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
        {
            if (!::IsWindow( windowHandle_ ) ||
                (msg.message == WM_QUIT) ||
                (msg.message == WM_KEYDOWN && 
                    msg.wParam == VK_F4 && ::GetKeyState(VK_MENU)))
            {
                result = false;
                break;
            }

            TranslateMessage( &msg );
            DispatchMessage( &msg );
        }
        ::Sleep(50);
    }
    return result;
}

/**
 *  Creates the d3d device and binds it to a window.
 *
 *  @param hWnd is the windowhandle to bind the d3d device to.
 *  @param deviceIndex is the index of the device to create.
 *  @param modeIndex is the index of the displaymode to initialise.
 *  @param windowed says whether to create a windowed or fullscreen device.
 *  @param requireStencil says whether a stencil buffer is required.
 *  @param windowedSize windowed size.
 *  @param hideCursor says whether the cursor should be hidden.
 *  @param forceRef Boolean indicating whether to force the Direct3D
 *          software reference rasteriser.
 *  @return true if the operation was successful.
 */
bool RenderContext::createDevice( HWND hWnd, uint32 deviceIndex,
                                 uint32 modeIndex, bool windowed,
                                 bool requireStencil, 
                                 const Vector2 & windowedSize,
                                 bool hideCursor,
                                 bool forceRef
#if ENABLE_ASSET_PIPE
                                 , AssetClient* pAssetClient
#endif
                                 )
{
    BW_GUARD;
    // save windowed mode size
    windowedSize_ = windowedSize;
    hideCursor_ = hideCursor;

    REGISTER_SINGLETON_FUNC( RenderThreadChecker, &isRenderThread );
    isRenderThread() = true;
    if( deviceIndex > devices_.size() )
    {
        CRITICAL_MSG( "Moo::RenderContext::createDevice: Tried to select a device that does not exist\n" );
        return false;
    }

    bool requireForceCreate = device_ == NULL;

    // Have we created a device before?
    if ( device_ )
    {
        // If the device is the same as the current device, just change the display mode, else release the device.
        if( deviceIndex == deviceIndex_ && windowHandle_ == hWnd )
        {
            return this->changeMode( modeIndex, windowed );
        }
        else
        {
            requireForceCreate = true;
            releaseDevice();
        }
    }

    // Set up the current device parameters.
    windowHandle_ = hWnd;
    // Assumes that the style the window has when creating the device is the
    // style desired when in windowed mode, and will be restored when the
    // device is released.
    windowedStyle_ = GetWindowLong( windowHandle_, GWL_STYLE );
    if (forceRef)
        deviceType_ = D3DDEVTYPE_REF;
    else
        deviceType_ = D3DDEVTYPE_HAL;
    deviceIndex_ = deviceIndex;
    modeIndex_ = modeIndex;
    windowed_ = windowed;
    stencilWanted_ = requireStencil;

    fillPresentationParameters();

    GetWindowRect( windowHandle_, &windowedRect_ );

    if( windowed_ == false )
    {
        SetWindowLong( windowHandle_, GWL_STYLE, WS_POPUP );
    }

    uint32 vertexProcessing = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
    mixedVertexProcessing_ = false;

    if (devices_[deviceIndex].caps_.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT)
    {
        // Treat hardware with no pixel or vertex shader support as fixed
        // function cards.
        if (uint16(devices_[deviceIndex].caps_.VertexShaderVersion) > 0x100 &&
            uint16(devices_[deviceIndex].caps_.PixelShaderVersion) > 0x100)
        {
            vertexProcessing = D3DCREATE_HARDWARE_VERTEXPROCESSING;
        }
        else
        {
            vertexProcessing = D3DCREATE_MIXED_VERTEXPROCESSING;
            mixedVertexProcessing_ = true;
        }
    }

    D3DDEVTYPE devType = deviceType_;
    if (forceRef)
        devType = D3DDEVTYPE_NULLREF;
    else 
    {
        // Force reference if using NVIDIA NVPerfHUD.
        BW::string description 
            = devices_[ deviceIndex_ ].identifier_.Description;

        if ( description.find("PerfHUD") != BW::string::npos )
            devType = D3DDEVTYPE_REF;
    }

    int retries = 5;
    HRESULT hr = D3D_OK;
    while ( retries > 0 )
    {
        if (usingD3DDeviceEx_)
        {
            INFO_MSG("Using D3DDeviceEx mode\n");
            D3DDISPLAYMODEEX dispModeDesc = { sizeof( dispModeDesc ) };

            D3DDISPLAYMODEEX* pFullScreenDesc = &dispModeDesc;
            if (windowed)
            {
                pFullScreenDesc = NULL;
            }
            else
            {
                dispModeDesc.Width = presentParameters_.BackBufferWidth;
                dispModeDesc.Height = presentParameters_.BackBufferHeight;
                dispModeDesc.Format = presentParameters_.BackBufferFormat;
                dispModeDesc.RefreshRate = presentParameters_.FullScreen_RefreshRateInHz;
                dispModeDesc.ScanLineOrdering = D3DSCANLINEORDERING_PROGRESSIVE;
            }

            hr = d3dEx_->CreateDeviceEx( devices_[ deviceIndex_ ].adapterID_, devType, 
                windowHandle_,
                vertexProcessing | D3DCREATE_DISABLE_DRIVER_MANAGEMENT_EX | 
                    D3DCREATE_MULTITHREADED | D3DCREATE_FPU_PRESERVE, 
                &presentParameters_, pFullScreenDesc, &deviceEx_ );
        }
        else
        {
            INFO_MSG("Using default D3DDevice mode\n");
            hr = d3d_->CreateDevice( devices_[ deviceIndex_ ].adapterID_, devType, 
                windowHandle_,
                vertexProcessing | D3DCREATE_DISABLE_DRIVER_MANAGEMENT | 
                    D3DCREATE_MULTITHREADED | D3DCREATE_FPU_PRESERVE, 
                &presentParameters_, &device_ );
        }

        if (SUCCEEDED( hr ))
        {
            retries = 0;
            break;
        }
        else
        {
            WARNING_MSG( "Moo::RenderContext::createDevice: Unable to create device, retrying (%d retries left)\n", retries );
            // Wait a couple of seconds before reattempting, to give a chance to
            // other D3D apps to free there resources and/or quit properly
            Sleep( 2000 );
            retries--;
        }
    }

    if ( FAILED( hr ) )
    {
        printErrorMsg_( hr, "Moo::RenderContext::createDevice: Unable to create device" );
        return false;
    }

    // succeeded
    MF_ASSERT( vsVersion_ == SHADER_VERSION_NOT_INITIALISED );
    MF_ASSERT( psVersion_ == SHADER_VERSION_NOT_INITIALISED );
    vsVersion_ = uint16( devices_[ deviceIndex_ ].caps_.VertexShaderVersion );
    psVersion_ = uint16( devices_[ deviceIndex_ ].caps_.PixelShaderVersion );
    maxSimTextures_ = uint16( devices_[ deviceIndex_ ].caps_.MaxSimultaneousTextures );
    maxAnisotropy_ = uint16( devices_[ deviceIndex_ ].caps_.MaxAnisotropy );
    beginSceneCount_ = 0;

    mrtSupported_ = (psVersion_ >= 0x300) && 
        (devices_[ deviceIndex_ ].caps_.NumSimultaneousRTs > 1) &&
        (devices_[ deviceIndex_ ].caps_.PrimitiveMiscCaps & D3DPMISCCAPS_INDEPENDENTWRITEMASKS);

    setGammaCorrection();
    UINT availTexMem = device_->GetAvailableTextureMem();
    INFO_MSG( "RenderContext::createDevice - available texture memory after create: %dkb\n", availTexMem/1024 );

    updateDeviceInfo();
    updateProjectionMatrix();
    initRenderStates();
    if (mixedVertexProcessing_)
        device_->SetSoftwareVertexProcessing( TRUE );

    pDynamicIndexBufferInterface_ = new DynamicIndexBufferInterface();

    if ( hideCursor_ )
    {
        s_OriginalMouseCursor = SetCursor( NULL );
        ShowCursor( FALSE );
    }
    else
    {
        s_OriginalMouseCursor = GetCursor();
    }

    // init managers if they have not been inited.
    if (!EffectManager::pInstance())
    {
        new EffectManager();
    }

    if (!TextureManager::instance())
    {
        TextureManager::init();
    }

#if ENABLE_ASSET_PIPE
    EffectManager::instance().setAssetClient( pAssetClient );
    TextureManager::instance()->setAssetClient( pAssetClient );
    TextureManager::instance()->getDetailsManager()->
        setAssetClient( pAssetClient );
#endif

    // init managers if they have not been inited.
    if (!effectVisualContext_)
    {
        effectVisualContext_ = new EffectVisualContext();
    }

    if (!PrimitiveManager::instance())
        PrimitiveManager::init();
    if (!VerticesManager::instance())
        VerticesManager::init();
    if (!VisualManager::instance())
        VisualManager::init();

    if (!NodeCatalogue::pInstance())
    {
        new NodeCatalogue();
    }

    if (!AnimationManager::pInstance())
    {
        new AnimationManager();
    }   


    if (!assetProcessingOnly_)
    {
        effectVisualContext_->init();

        MF_ASSERT(mrtSupport_ == NULL);
        mrtSupport_ = new Moo::MRTSupport();
        mrtSupport_->init();

        customAA_.reset(new CustomAA());
        fsQuad_.reset(new FullscreenQuad());

        DeviceCallback::createAllManaged();
        createUnmanaged( requireForceCreate );

        MF_WATCH(   "Render/backBufferWidthOverride", 
                    *this, 
                    MF_ACCESSORS( uint32, RenderContext, backBufferWidthOverride ),
                    "Resize the back buffer. Only works in windowed mode. 0 matches window size. Aspect ratio is maintained." );

        MF_WATCH( "Render/numPreloadResources",
            s_numPreloadResources, Watcher::WT_READ_ONLY,
            "The number of managed pool resources not yet preloaded" );

        MF_WATCH( "Render/Draw Calls",
            lastFrameProfilingData_.nDrawcalls_,  Watcher::WT_READ_ONLY,
            "DrawCalls of the previous frame." );
        MF_WATCH( "Render/Primitives",
            lastFrameProfilingData_.nPrimitives_,  Watcher::WT_READ_ONLY,
            "Primitives drawn in the previous frame." );

        MF_WATCH( "Render/numTexturesInReuseList",
            textureReuseCache(), &TextureReuseCache::size,
            "How many textures currently cached for future re-use." );

        // If we're not using a d3dex device and we are in fullscreen, we do an
        // extra reset here since the device gets lost because of display uniqueness
        // change
        if (!usingD3DDeviceEx_ && !windowed)
        {
            this->resetDevice();
        }
    }

    return true;
}

/**
*   This method initialises all the render states and TS states in the cache.
*/
void RenderContext::initRenderStates()
{
    BW_GUARD;
    int i,j;

    for ( i = 0; i < D3DFFSTAGES_MAX; i++ )
    {
        for( j = 0; j < D3DTSS_MAX; j++ )
        {
            tssCache_[i][j].Id = 0;
        }
    }

    for( i = 0; i < D3DSAMPSTAGES_MAX; i++ )
    {
        for( j = 0; j < D3DSAMP_MAX; j++ )
        {
            sampCache_[i][j].Id = 0;
        }
        textureCache_[i].Id = 0;
    }

    for( i = 0; i < D3DRS_MAX; i++ )
    {
        rsCache_[i].Id = 0;
    }

    vertexDeclarationId_ = 0;

    // Make all cached states invalid
    cacheValidityId_ = 1;

    // Set some default states explicitly
    this->setRenderState( D3DRS_CLIPPING, TRUE );
    this->setRenderState( D3DRS_COLORWRITEENABLE,
        D3DCOLORWRITEENABLE_ALPHA | D3DCOLORWRITEENABLE_RED |
        D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE );
}

//-- gets access to the custom AA feature.
CustomAA& RenderContext::customAA()
{
    MF_ASSERT(customAA_.get());
    return *customAA_.get();
}

//-- full-screen quad used mainly for post-processing tasks.
FullscreenQuad& RenderContext::fsQuad()
{
    MF_ASSERT(fsQuad_.get());
    return *fsQuad_.get();
}


/**
 *  This method updates the viewprojection and the inverse view matrix,
 *  should be called whenever the view transform or the projection transform
 *  gets changed.
 */
void RenderContext::updateViewTransforms()
{
    viewProjection_.multiply( view_, projection_ );
    invView_.invert( view_ );
}

/**
 * This method sets the screen width in pixels.
 */
void RenderContext::screenWidth( int width )
{
    halfScreenWidth_ = float(width) * 0.5f;
}

/**
* This method sets the screen height in pixels.
*/
void RenderContext::screenHeight( int height )
{
    halfScreenHeight_ = float(height) * 0.5f;
}

/**
 * This function overrides the dimensions of the back buffer when in windowed mode 
 * using a given width.
 * 
 * Given values are first clamped to the maximum width supported by the device. 
 * The height is set so as to maintain the aspect ratio of the front buffer. 
 * A value of 0 means that this feature is disabled and the back buffer will 
 * have the same dimensions as the front.
 * This functionality can be used to produce screenshots at resolutions 
 * greater than that that can be displayed on a monitor.
 * 
 *  @param backBufferWidthOverride  Width to set the back buffer to. 0
 *                                  indicates same width as the visible display.
 */
void RenderContext::backBufferWidthOverride( uint32 backBufferWidthOverride )
{
    BW_GUARD;
    uint32 maxWidth = this->deviceInfo(0).caps_.MaxTextureWidth;

    backBufferWidthOverride = std::min( maxWidth, backBufferWidthOverride );

    if( this->windowed() == false )
        backBufferWidthOverride = 0;

    if( backBufferWidthOverride != this->backBufferWidthOverride_ )
        this->changeMode(this->modeIndex(), this->windowed(), true, backBufferWidthOverride );
}

uint32 RenderContext::backBufferWidthOverride() const
{
    return backBufferWidthOverride_;
}

const Vector2 & RenderContext::windowedModeSize() const
{
    return this->windowedSize_;
}

/**
 *  This method increases the current frame counter.
 */
void RenderContext::nextFrame( )
{
    BW_GUARD;
    DynamicVertexBufferBase::resetLockAll();
    this->dynamicIndexBufferInterface().resetLocks();
//  ZeroMemory( primitiveHistogram_, sizeof( primitiveHistogram_ ) );
    currentFrame_++;
}


/**
 * Takes a screenshot
 *
 * @param fileExt - the extension to save the screenshot as, this can be "bmp",
 *                  "jpg", "tga", "png" or "dds".
 *
 * @param fileName - the root name of the file to save out
 *
 * @param autoNumber - is this is true then try to append a unique 
 *                     identifying number to the shot name(e.g. shot_666.bmp)
 */
const BW::string RenderContext::screenShot(
    const BW::string& fileExt /*= "bmp"*/,
    const BW::string& fileName /*= "shot"*/,
    bool autoNumber /*= true*/ )
{
    BW_GUARD;

    // convert uppercase to lowercase
    BW::string ext ( fileExt );
    std::transform( ext.begin(), ext.end(), ext.begin(), tolower );

    D3DXIMAGE_FILEFORMAT d3dFromat = TextureCompressor::getD3DImageFileFormat( 
        fileName + "." + ext, D3DXIFF_DDS );

    BW::string name;

    if (autoNumber)
    {   
        static uint32 sequence = 1;

        // go through filenames until we find one that has not been created yet.
        bool foundEmptyFile = false;
        while( !foundEmptyFile )
        {
            // Create the filename
            BW::ostringstream findName;
            findName    << fileName 
                        << "_" << std::setfill( '0' ) << std::setw( 3 ) << 
                        std::right << sequence << "." << ext;

            // is there such a file?
            if( !BWResource::fileAbsolutelyExists( findName.str() ) )
            {
                // nope, we have a winner.
                foundEmptyFile = true;
                name = findName.str();
            }
            else
            {
                // try the next file.
                sequence++;
            }
        }
    }
    else
    {
        BW::ostringstream oss;
        oss << fileName << "." << ext;
        name = oss.str();
    }

    ComObjectWrap<DX::Surface> backBuffer;
    if (FAILED( Moo::rc().device()->GetBackBuffer( 
        0, 0, D3DBACKBUFFER_TYPE_MONO, &backBuffer ) ) )
    {
        ERROR_MSG( "Moo::RenderContext::screenShot - unable to get "
                   "backbuffer surface\n" );
        return BW::string();
    }

    D3DSURFACE_DESC backBufferDesc;
    backBuffer->GetDesc( &backBufferDesc );

    ComObjectWrap<DX::Surface> systemMemorySurface;
    if (FAILED( Moo::rc().device()->CreateOffscreenPlainSurface(
            backBufferDesc.Width,
            backBufferDesc.Height,
            D3DFMT_X8R8G8B8,
            D3DPOOL_SYSTEMMEM,
            &systemMemorySurface,
            NULL ) ) )
    {
        ERROR_MSG( "Moo::RenderContext::screenShot - unable to create "
            "surface\n" );
        return BW::string();
    }

    if (FAILED( D3DXLoadSurfaceFromSurface( 
            systemMemorySurface.pComObject(),
            NULL,
            NULL,
            backBuffer.pComObject(),
            NULL,
            NULL, 
            D3DX_FILTER_NONE,
            0xFF000000 ) ) )
    {
        ERROR_MSG( "Moo::RenderContext::screenShot - unable to load "
                   "surface\n" );
        return BW::string();
    }

    BW::wstring wname;
    bw_utf8tow( name, wname );
    if (FAILED ( D3DXSaveSurfaceToFile( 
            wname.c_str(),
            d3dFromat,
            systemMemorySurface.pComObject(),
            NULL,
            NULL) ) )
    {
        ERROR_MSG( "Moo::RenderContext::screenShot - unable to save "
                   "surface to image file\n" );
        return BW::string();
    }

    INFO_MSG( "Moo::RenderContext::screenShot - saved image %s\n",
        name.c_str() );

    return name;
}

/**
 * This method copies the current back buffer into and internal surface, then
 * returns a pointer to the internal surface.
 */
DX::Surface* RenderContext::getScreenCopy()
{
    BW_GUARD;
    DX::Surface* ret = NULL;
    if (screenCopySurface_.hasComObject())
    {
        ComObjectWrap<DX::Surface> backBuffer;
        if( SUCCEEDED( device_->GetBackBuffer( 0, 0, D3DBACKBUFFER_TYPE_MONO, &backBuffer ) ) )
        {
            if (SUCCEEDED( D3DXLoadSurfaceFromSurface( screenCopySurface_.pComObject(), NULL, NULL,
                backBuffer.pComObject(), NULL, NULL, D3DX_FILTER_NONE, 0 ) ) )
            {
                ret = screenCopySurface_.pComObject();
            }
            else
            {
                ERROR_MSG( "RenderContext::getScreenCopy - unable to get copy\n" );
            }
        }
        else
        {
            ERROR_MSG( "RenderContext::getScreenCopy - unable to get backbuffer\n" );
        }
    }
    else
    {
        ERROR_MSG( "RenderContext::getScreenCopy - no screencopy surface available\n" );
    }
    return ret;
}


void RenderContext::fini()
{
    BW_GUARD;
    //-- destroy CustomAA object.
    customAA_.reset();

    //-- destroy Full-screen quad.
    fsQuad_.reset();

    if (VisualManager::instance())
        VisualManager::fini();

    delete EffectManager::pInstance();

    if (PrimitiveManager::instance())
        PrimitiveManager::fini();
    if (VerticesManager::instance())
        VerticesManager::fini();

    delete AnimationManager::pInstance();

    AnimationChannel::fini();

    bw_safe_delete(effectVisualContext_);

    if (TextureManager::instance())
        TextureManager::fini();

    bw_safe_delete(pDynamicIndexBufferInterface_);

    DynamicVertexBufferBase::fini();
    DeviceCallback::fini();

    delete NodeCatalogue::pInstance();

    VertexDeclaration::fini();

    RenderContextCallback::fini();

    delete gpuInfo_;
    gpuInfo_ = NULL;

    // destroy device and d3d obj
    if (device_)
    {
        device_->Release();
        device_ = NULL;
    }

    if (d3d_)
    {
        d3d_->Release();
        d3d_ = NULL;
    }

    isValid_ = false;
}


/**
 * Destruct render context.
 */
RenderContext::~RenderContext()
{
    if ( isValid_ )
        fini();
}

bool RenderContext::supportsTextureFormat( D3DFORMAT fmt ) const
{
    BW_GUARD;

    if (device_)
    {
        if (SUCCEEDED( d3d_->CheckDeviceFormat( devices_[ deviceIndex_ ].adapterID_,
                    deviceType_,
                    adapterFormat(),
                    0,
                    D3DRTYPE_TEXTURE,
                    fmt) ) )
            return true;
    }

    return false;
}

bool RenderContext::isMultisamplingEnabled() const
{
    return false;//(multisamplingType_ != D3DMULTISAMPLE_NONE && multisamplingQuality_ >= 0);
}


/**
 *  This method pushes a render target from the supplied RenderContext
 *  onto the stack.
 */
bool RenderContext::RenderTargetStack::push( RenderContext* rc )
{
    BW_GUARD;
    MF_ASSERT_DEV( rc );

    if (!rc)
        return false;

    DX::Device* device = rc->device();
    MF_ASSERT_DEV( device );

    if (!device)
        return false;

    StackItem si;
    si.cam_ = rc->camera();
    si.projection_ = rc->projection();
    si.view_ = rc->view();
    HRESULT hr;
    ComObjectWrap< DX::Surface > ts = rc->getRenderTarget( 0 );
    if (!ts.hasComObject())
    {
        CRITICAL_MSG( "Moo::RenderContext::RenderTargetStack::push: couldn't get current render target\n" );
        return false;
    }

    si.renderSurfaces_[0] = ts;
    ts = NULL;

    for ( uint32 i=1; i<MAX_CONCURRENT_RTS; i++ )
    {   
        ts = rc->getRenderTarget(i);
        if ( ts )
        {
            si.renderSurfaces_[i] = ts;
            ts = NULL;
        }
    }

    hr = device->GetDepthStencilSurface( &ts );
    if( FAILED( hr ) )
    {
        CRITICAL_MSG( "Moo::RenderContext::RenderTargetStack::push: couldn't get current depth/stencil buffer\n" );
        return false;
    }
    si.zbufferSurface_ = ts;
    ts = NULL;

    rc->getViewport( &si.viewport_ );

    stackItems_.push_back( si );
    return true;
}

/**
 *  This method pops a render target into the supplied render context.
 */
bool RenderContext::RenderTargetStack::pop( RenderContext* rc )
{
    BW_GUARD;
    IF_NOT_MF_ASSERT_DEV( rc )
    {
        return false;
    }

    MF_ASSERT_DEV( stackItems_.size() );
    if (!stackItems_.size())
    {
        CRITICAL_MSG( "Tried to pop the render target when no render target was there.\n" );
        return false;
    }

    DX::Device* device = rc->device();
    MF_ASSERT_DEV( device );
    if (!device)
        return false;

    StackItem si = stackItems_.back();
    stackItems_.pop_back();

    IF_NOT_MF_ASSERT_DEV(si.renderSurfaces_[0])
    {
        return false;
    }

    HRESULT hr = rc->setRenderTarget( 0, si.renderSurfaces_[0] );
    if (SUCCEEDED(hr))
    {
        for ( uint32 i=1; i<MAX_CONCURRENT_RTS; i++ )
        {
            if ( si.renderSurfaces_[i].hasComObject() )
                hr = rc->setRenderTarget( i, si.renderSurfaces_[i] );
            else
                rc->setRenderTarget( i, NULL );
        }
    }

    HRESULT hr2 = device->SetDepthStencilSurface( &*si.zbufferSurface_ );
    if( FAILED( hr ) || FAILED( hr2 ) )
    {
        CRITICAL_MSG( "Moo::RenderContext::RenderTargetStack::pop: couldn't set rendertarget/depth buffer\n" );
        return false;
    }
    rc->setViewport( &si.viewport_ );

    rc->camera( si.cam_ );
    rc->view( si.view_ );
    rc->projection( si.projection_ );
    rc->updateViewTransforms();

    rc->screenWidth( si.viewport_.Width );
    rc->screenHeight( si.viewport_.Height );

    return true;
}


/**
 *  This method returns the rectangle that is the intersection of the near-plane
 *  with the view frustum.
 *
 *  @param corner       The position of the bottom-left corner of the rectangle.
 *  @param xAxis        The length of the horizontal edges.
 *  @param yAxis        The length of the vertical edges.
 *
 *  @note   The invView matrix must be correct before this method is called. You
 *          may need to call updateViewTransforms.
 *
 *  @see invView
 *  @see updateViewTransforms
 */
void RenderContext::getNearPlaneRect( Vector3 & corner,
        Vector3 & xAxis, Vector3 & yAxis ) const
{
    BW_GUARD;
    const Matrix & matrix = this->invView();

    // zAxis is the vector from the camera position to the centre of the
    // near plane of the camera.
    Vector3 zAxis = matrix.applyToUnitAxisVector( Z_AXIS );
    zAxis.normalise();

    // xAxis is the vector from the centre of the near plane to its right edge.
    xAxis = matrix.applyToUnitAxisVector( X_AXIS );
    xAxis.normalise();

    // yAxis is the vector from the centre of the near plane to its top edge.
    yAxis = matrix.applyToUnitAxisVector( Y_AXIS );
    yAxis.normalise();

    const float fov = camera_.fov();
    const float nearPlane = camera_.nearPlane();
    const float aspectRatio = camera_.aspectRatio();

    const float yLength = nearPlane * tanf( fov / 2.0f );
    const float xLength = yLength * aspectRatio;

    xAxis *= xLength;
    yAxis *= yLength;
    zAxis *= nearPlane;

    Vector3 nearPlaneCentre( matrix.applyToOrigin() + zAxis );
    corner = nearPlaneCentre - xAxis - yAxis;
    xAxis *= 2.f;
    yAxis *= 2.f;
}

/**
 *  Determines whether a hardware mouse cursor is available.
 *
 *  @return Whether or not a hardware cursor is available.
 */
bool RenderContext::hardwareCursor() const
{
    return this->windowed() || devices_[ deviceIndex_ ].caps_.CursorCaps != 0;
}


/**
 *  Restores the windows cursor state, hidding or showing it depending on the 
 *  value passed.
 *
 *  @param  state   True will show the windows cursor. False will hide it.
 */
void RenderContext::restoreCursor( bool state )
{
    BW_GUARD;
    if (state)
    {
        while (!(::ShowCursor(true) >= 0));
        if (s_OriginalMouseCursor)
            SetCursor( s_OriginalMouseCursor );
    }
    else
    {
        while (!(::ShowCursor(false) < 0));
        if (device_)
            SetCursor( NULL );
    }
}


/**
 *  Frees DX resources temporarily.
 *  
 *  @see resume
 *  @see paused
 */
void RenderContext::pause()
{
    BW_GUARD;
    if ( Moo::rc().device()->TestCooperativeLevel() == D3D_OK )
    {
        if ( !paused_ )
        {
            releaseUnmanaged( !usingD3DDeviceEx_ );
        }
        device_->EvictManagedResources();
    }
    paused_ = true;
}


/**
 *  Reallocates DX resources freed in pause.
 *  
 *  @see pause
 *  @see paused
 */
void RenderContext::resume()
{
    BW_GUARD;
    if ( !paused_ )
        return;

    bool reset = false;
    if ( checkDevice(&reset) )
    {
        if (!reset)
        {
            createUnmanaged( !usingD3DDeviceEx_ );
        }
        paused_ = false;
    }
}


/**
 *  Checks if the device is paused.
 *  
 *  @return true if the device is paused, false otherwise
 *  
 *  @see pause
 *  @see resume
 */
bool RenderContext::paused() const
{
    return paused_;
}

/**
 *
 */

static BW::vector< std::pair< D3DRENDERSTATETYPE, uint32 > > s_renderStateStackDX;
static BW::vector< std::pair< D3DRENDERSTATETYPE, uint32 > > s_renderStateStackMain;


void RenderContext::pushRenderState( D3DRENDERSTATETYPE state )
{
    s_renderStateStackMain.push_back( std::make_pair( state, rsCache_[state].currentValue ) );
}

/**
 *
 */


void RenderContext::popRenderState()
{
    setRenderState( s_renderStateStackMain.back().first, s_renderStateStackMain.back().second );

    s_renderStateStackMain.pop_back();
}

/**
 *  Wrapper for D3D BeginScene call, this reference counts the Begin/End
 *  Scene pairs, so that we can nest calls to begin and end scene.
 *  @return D3D_OK if successful d3d error code otherwise
 */
HRESULT RenderContext::beginScene()
{
    BW_GUARD;
    HRESULT hr = D3D_OK;
    if (beginSceneCount_ == 0)
    {

        #ifdef EDITOR_ENABLED
            // The editors can spend 10ms preloading resources.
            preloadDeviceResources( 10 );
        #else
            // Spend 2 milliseconds preloading device resources
            preloadDeviceResources( 2 );
        #endif

        hr = device_->BeginScene();
    }

    ++beginSceneCount_;
    return hr;
}

/**
 *  Wrapper for D3D endScene call, this reference counts the Begin/End
 *  Scene pairs, so that we can nest calls to begin and end scene.
 *  @return D3D_OK if successful D3DERR_INVALIDCALL otherwise
 */
HRESULT RenderContext::endScene()
{
    BW_GUARD;
    HRESULT hr = D3D_OK;

    --beginSceneCount_;
    if (beginSceneCount_ == 0)
    {
        hr = device_->EndScene();
        lastFrameProfilingData_ = liveProfilingData_;
        ZeroMemory(&liveProfilingData_, sizeof(liveProfilingData_));
    }
    else if (beginSceneCount_ < 0)
    {
        hr = D3DERR_INVALIDCALL;
        beginSceneCount_ = 0;
    }
    return hr;
}

/**
 *  Clears the currently bound device resources. 
 *  It is D3D recommended behaviour to unbind all resource states after every frame.
 *
 */
void RenderContext::clearBindings()
{
    BW_GUARD;
    PROFILER_SCOPED(clearBingings);
    // unbind all the texture/vert/ind
    setIndices( NULL );
    for( uint32 i = 0; ( i < devices_[ deviceIndex_ ].caps_.MaxStreams ) || ( i < 1 ); i++ )
    {
        device_->SetStreamSource( i, NULL, 0, 0 );
    }

    // release all textures
    for( uint32 i = 0; i < devices_[ deviceIndex_ ].caps_.MaxSimultaneousTextures; i++ )
    {
        setTexture( i, NULL );
    }

    //release shaders
    setPixelShader( NULL );
    setVertexShader( NULL );

    //consider invalidating the render states as well?
}

/**
 *  Wrapper for D3D Present call.
 *
 *  @return D3D_OK if successful D3DERR_INVALIDCALL otherwise
 */

HRESULT RenderContext::present()
{
    BW_GUARD;

    if (beginSceneCount_ != 0)
        return D3DERR_INVALIDCALL;

    HRESULT hr = NULL;
    {
        PROFILER_SCOPED( GPU_Wait );

        clearBindings();
#if ENABLE_GPU_PROFILER
        GPUProfiler::instance().endFrame();
#endif
        {
            PROFILER_SCOPED(Present);
            hr = device_->Present( NULL, NULL, NULL, NULL );
        }
#if ENABLE_GPU_PROFILER
        GPUProfiler::instance().beginFrame();
#endif
    }
    return hr;
}


void RenderContext::releaseQueryObjects(void)
{
    BW_GUARD;
    int nQueries = static_cast<int>(queryList_.size());
    for (int i = 0; i < nQueries; i++)
    {
        OcclusionQuery* query = queryList_[i];
        if (query && query->queryObject)
        {
            IDirect3DQuery9* queryObject = query->queryObject;          
            queryObject->Release();
            query->queryObject = NULL;
        }
    }
}

/*-------------------------------------------------------------------*//*!
 * \brief   
 *
 *//*-------------------------------------------------------------------*/

void RenderContext::createQueryObjects(void)
{
    BW_GUARD;
    int nQueries = static_cast<int>(queryList_.size());
    for (int i = 0; i < nQueries; i++)
    {
        OcclusionQuery* query = queryList_[i];

        IDirect3DQuery9* queryObject = NULL;
        device_->CreateQuery( D3DQUERYTYPE_OCCLUSION, &queryObject );
        query->queryObject = queryObject;
    }
}

/**
 * 
 */
OcclusionQuery* RenderContext::createOcclusionQuery()
{
    BW_GUARD;
    IDirect3DQuery9* pOcclusionQuery = NULL;
    HRESULT res = device_->CreateQuery( D3DQUERYTYPE_OCCLUSION, &pOcclusionQuery );

    if (res != S_OK) // creation failed
        return NULL;

    OcclusionQuery* q = new OcclusionQuery();
    q->queryObject = pOcclusionQuery;
    q->index = static_cast<int>(queryList_.size());
    queryList_.push_back( q );
    return q;
}

/**
 * 
 */
void RenderContext::destroyOcclusionQuery(OcclusionQuery* query)
{
    BW_GUARD;
    // swap query with the last one in the list and delete it
    OcclusionQuery* last = queryList_.back();
    if (query)
    {
        queryList_[query->index] = last;
        last->index = query->index;
        queryList_.pop_back();
        if (query->queryObject)
            query->queryObject->Release();
        bw_safe_delete(query);
    }
}

/**
 * 
 */
void RenderContext::beginQuery(OcclusionQuery* query)
{
    BW_GUARD;
    query->queryObject->Issue( D3DISSUE_BEGIN );
}

/**
 * 
 */
void RenderContext::endQuery(OcclusionQuery* query)
{
    BW_GUARD;
    query->queryObject->Issue( D3DISSUE_END );
}

/**
 * @return true is query result was available, false otherwise
 */
bool RenderContext::getQueryResult(int& visiblePixels, OcclusionQuery* query, bool wait)
{
    BW_GUARD;
    IDirect3DQuery9* pOcclusionQuery = query->queryObject;

    if (wait)
    {
        HRESULT hres = S_FALSE;
        while (S_FALSE == 
            (hres = pOcclusionQuery->GetData( &visiblePixels, sizeof(DWORD), D3DGETDATA_FLUSH )))
        {
            if(Moo::rc().device()->TestCooperativeLevel() == D3DERR_DEVICELOST)
            {
                visiblePixels = 1;
                DEBUG_MSG("Device lost when waiting for occlusion data, default visiblePixels is used\n");
                return true;
            }
        }
        return S_OK == hres;
    }
    else
    {
        return S_OK == pOcclusionQuery->GetData( &visiblePixels, sizeof(DWORD), D3DGETDATA_FLUSH );
    }
}

namespace 
{
char* poolStrings[] = 
{
    "D3DPOOL_DEFAULT",
    "D3DPOOL_MANAGED",
    "D3DPOOL_SYSTEMMEM",
    "D3DPOOL_SCRATCH",
    "INVALID_POOL"
};

char* getPoolString( DWORD pool )
{
    return poolStrings[std::max( pool, DWORD(4) )];
}

/**
 * Output a string error message for some DirectX errors.
 */
void printCreateTextureErrorMsg( HRESULT hr, const BW::string & msg, UINT Width,
                                 UINT Height, UINT Levels, D3DFORMAT Format,
                                 D3DPOOL Pool, const BW::StringRef& allocator )
{   
    const char* poolName = getPoolString( Pool );
    const BW::string& formatName = FormatNameMap::formatString( Format );

    switch ( hr )
    {
    case D3DERR_DRIVERINTERNALERROR:
        CRITICAL_MSG( "%s size %dx%dx%d format %s in pool %s for %s %s\n",
            msg.c_str(), Width, Height, Levels, formatName.c_str(),
            poolName, allocator.data(), DX::errorAsString( hr ).c_str() );
        break;
    default:
        WARNING_MSG( "%s size %dx%dx%d format %s in pool %s for %s %s\n",
            msg.c_str(), Width, Height, Levels, formatName.c_str(), poolName,
            allocator.data(), DX::errorAsString( hr ).c_str() );
        break;
    }
}

}




/**
 * This method wraps the DX CreateTexture call.
 */
ComObjectWrap<DX::Texture>  RenderContext::createTexture( UINT Width, UINT Height, UINT Levels, DWORD Usage,
    D3DFORMAT Format, D3DPOOL Pool, const BW::StringRef& allocator )
{
    BW_GUARD;
    ComObjectWrap<DX::Texture> tex;

    Pool = patchD3DPool(Pool);

    // calculate number of mip levels for reuse call if "Levels" argument is 0
    uint reuseNumLevels = Levels;
    if (reuseNumLevels == 0 && !(Usage & D3DUSAGE_RENDERTARGET))
    {
        // calculate number of mips required
        uint32 w = Width;
        uint32 h = Height;
        while (w >= 1 || h >= 1)
        {
            w /= 2;
            h /= 2;
            reuseNumLevels++;
        }

    }
    // look in the reuse list first
    tex = this->getTextureFromReuseList( Width, Height, reuseNumLevels, Usage, Format, Pool );
    if (tex.hasComObject())
    {
        return tex;
    }
    // nothing in reuse list, have to create a new texture
    HRESULT hr = device_->CreateTexture( Width, Height, Levels, Usage, Format, Pool, &tex, NULL );
    if( SUCCEEDED( hr ) )
    {
#if ENABLE_RESOURCE_COUNTERS
        tex.addAlloc( allocator );
#endif
        return tex;
    }
    else
    {
        if ( hr == E_OUTOFMEMORY && Pool != D3DPOOL_DEFAULT )
        {
            memoryCritical_ = true;
        }
        else
        {
            // Try to remove managed resources from video memory and allocate the texture again
            device_->EvictManagedResources();

            hr = device_->CreateTexture( Width, Height, Levels, Usage, Format, Pool, &tex, NULL );
            if( SUCCEEDED( hr ) )
            {
        #if ENABLE_RESOURCE_COUNTERS
                tex.addAlloc( allocator );
        #endif
                return tex;
            }
            else if (hr == E_OUTOFMEMORY)
            {
                memoryCritical_ = true;
            }
        }

        printCreateTextureErrorMsg( hr, "RenderContext::createTexture - could not create texture map",
            Width, Height, Levels, Format, Pool, allocator );
    }
    return ComObjectWrap<DX::Texture>();
}

/**
 * This method wraps the DX CreateCubeTexture call.
 */
ComObjectWrap<DX::CubeTexture>  RenderContext::createCubeTexture( UINT edgeLength, UINT levels, DWORD usage,
    D3DFORMAT format, D3DPOOL pool, const BW::StringRef& allocator )
{
    BW_GUARD;
    ComObjectWrap<DX::CubeTexture> tex;

    pool = patchD3DPool(pool);

    HRESULT hr = device_->CreateCubeTexture( edgeLength, levels, usage, format, pool, &tex, NULL );
    if( SUCCEEDED( hr ) )
    {
#if ENABLE_RESOURCE_COUNTERS
        tex.addAlloc( allocator );
#endif
        return tex;
    }
    else
    {
        if ( hr == E_OUTOFMEMORY && pool != D3DPOOL_DEFAULT )
        {
            memoryCritical_ = true;
        }
        else
        {
            // Try to remove managed resources from video memory and allocate the texture again
            device_->EvictManagedResources();

            hr = device_->CreateCubeTexture( edgeLength, levels, usage, format, pool, &tex, NULL );
            if( SUCCEEDED( hr ) )
            {
        #if ENABLE_RESOURCE_COUNTERS
                tex.addAlloc( allocator );
        #endif
                return tex;
            }
            else if (hr == E_OUTOFMEMORY)
            {
                memoryCritical_ = true;
            }
        }

        printCreateTextureErrorMsg( hr, "RenderContext::createCubeTexture - could not create texture map",
            edgeLength, edgeLength, levels, format, pool, allocator );
    }
    return ComObjectWrap<DX::CubeTexture>();
}

/**
* This method wraps the D3DXCreateTextureFromFileInMemoryEx call.
*/
ComObjectWrap<DX::Texture>  RenderContext::createTextureFromFileInMemoryEx(
    LPCVOID pSrcData, UINT SrcDataSize, UINT Width, UINT Height,
    UINT MipLevels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool,
    DWORD Filter, DWORD MipFilter, D3DCOLOR ColorKey, D3DXIMAGE_INFO* pSrcInfo,
    PALETTEENTRY* pPalette, const BW::StringRef& allocator )
{
    BW_GUARD;
    SimpleMutexHolder smh( d3dxCreateMutex_ );

    Pool = patchD3DPool(Pool);

    ComObjectWrap<DX::Texture> tex;

    HRESULT hr = D3DXCreateTextureFromFileInMemoryEx( device_, pSrcData, SrcDataSize, Width, Height,
        MipLevels, Usage, Format, Pool, Filter, MipFilter, ColorKey, pSrcInfo, pPalette, &tex );
    if( SUCCEEDED( hr ) )
    {
#if ENABLE_RESOURCE_COUNTERS
        tex.addAlloc( allocator );
#endif
        return tex;
    }
    else
    {
        if ( hr == E_OUTOFMEMORY )
            memoryCritical_ = true;

        printCreateTextureErrorMsg( hr, "RenderContext::createTextureFromFileInMemoryEx - could not create texture map",
            Width, Height, MipLevels, D3DFMT_UNKNOWN, Pool, allocator );
    }
    return ComObjectWrap<DX::Texture>();
}

/**
* This method wraps the D3DXCreateTextureFromFileEx call.
*/
ComObjectWrap<DX::Texture>  RenderContext::createTextureFromFileEx( LPCTSTR pSrcFile, UINT Width, UINT Height, UINT MipLevels,
    DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, DWORD Filter, DWORD MipFilter,
    D3DCOLOR ColorKey, D3DXIMAGE_INFO * pSrcInfo, PALETTEENTRY * pPalette, const BW::StringRef& allocator )
{
    BW_GUARD;
    SimpleMutexHolder smh( d3dxCreateMutex_ );

    Pool = patchD3DPool(Pool);

    ComObjectWrap<DX::Texture> tex;

    HRESULT hr = D3DXCreateTextureFromFileEx( device_, pSrcFile, Width, Height, MipLevels,
        Usage, Format, Pool, Filter, MipFilter, ColorKey, pSrcInfo, pPalette, &tex );
    if( SUCCEEDED( hr ) )
    {
#if ENABLE_RESOURCE_COUNTERS
        tex.addAlloc( allocator );
#endif
        return tex;
    }
    else
    {
        if ( hr == E_OUTOFMEMORY )
            memoryCritical_ = true;

        printCreateTextureErrorMsg( hr, "RenderContext::createTextureFromFileEx - could not create texture map",
            Width, Height, MipLevels, D3DFMT_UNKNOWN, Pool, allocator );
    }
    return ComObjectWrap<DX::Texture>();
}

/**
* This method wraps the D3DXCreateCubeTextureFromFileInMemoryEx call.
*/
ComObjectWrap<DX::CubeTexture>  RenderContext::createCubeTextureFromFileInMemoryEx(
    LPCVOID pSrcData, UINT SrcDataSize, UINT Size, UINT MipLevels, DWORD Usage,
    D3DFORMAT Format, D3DPOOL Pool, DWORD Filter, DWORD MipFilter, D3DCOLOR ColorKey,
    D3DXIMAGE_INFO* pSrcInfo, PALETTEENTRY* pPalette, const BW::StringRef& allocator )
{
    BW_GUARD;
    SimpleMutexHolder smh( d3dxCreateMutex_ );

    Pool = patchD3DPool(Pool);

    ComObjectWrap<DX::CubeTexture> tex;

    HRESULT hr = D3DXCreateCubeTextureFromFileInMemoryEx( device_, pSrcData, SrcDataSize, Size,
        MipLevels, Usage, Format, Pool, Filter, MipFilter, ColorKey, pSrcInfo, pPalette, &tex );
    if( SUCCEEDED( hr ) )
    {
#if ENABLE_RESOURCE_COUNTERS
        tex.addAlloc( allocator );
#endif
        return tex;
    }
    else
    {
        if ( hr == E_OUTOFMEMORY )
            memoryCritical_ = true;

        printCreateTextureErrorMsg( hr, "RenderContext::createCubeTextureFromFileInMemoryEx - could not create texture map",
            Size, Size, MipLevels, D3DFMT_UNKNOWN, Pool, allocator );
    }
    return ComObjectWrap<DX::CubeTexture>();
}

/**
 * This method wraps the DX CreateOffscreenPlainSurface call.
 */
ComObjectWrap<DX::Surface> RenderContext::createOffscreenPlainSurface(
                                            UINT Width, UINT Height, D3DFORMAT Format, D3DPOOL Pool,
                                            const BW::StringRef& allocator )
{
    BW_GUARD;
    ComObjectWrap<DX::Surface> surface;

    Pool = patchD3DPool(Pool);

    HRESULT hr = device_->CreateOffscreenPlainSurface( Width, Height, Format, Pool, &surface, NULL );
    if( SUCCEEDED( hr ) )
    {
#if ENABLE_RESOURCE_COUNTERS
        surface.addAlloc( allocator );
#endif
        return surface;
    }
    else
    {
        if ( hr == E_OUTOFMEMORY )
            memoryCritical_ = true;

        WARNING_MSG( "RenderContext::createOffscreenPlainSurface - could not create memory surface "
            "size %dx%d.\n ( error %lx:%s )\n",
        Width, Height,
        hr, DX::errorAsString( hr ).c_str() );
    }
    return ComObjectWrap<DX::Surface>();
}

RenderContext& rc()
{
    SINGLETON_MANAGER_WRAPPER_FUNC( RenderContext, &Moo::rc )
    MF_ASSERT_DEBUG( g_RC && g_RC->isValid() && "RenderContext not initialised." );
    return *g_RC;
}

bool & isRenderThread()
{
    SINGLETON_MANAGER_WRAPPER_FUNC( RenderThreadChecker, &isRenderThread )
    return g_renderThread;
}

/**
 * This method preloads resources from the preloadlist.
 * It spends up to timeLimit time doing this
 * @param timeLimitMs the max time in milliseconds we want to spend
 * on preloading
 */
void RenderContext::preloadDeviceResources( uint32 timeLimitMs )
{
    BW_GUARD;
    if (!preloadResourceList_.empty() && 
        preloadResourceMutex_.grabTry())
    {
        PROFILER_SCOPED( PreloadResources );

        // Calculate the timelimit in stamps
        uint64 timeLimit = (stampsPerSecond() * timeLimitMs) / uint64(1000);
        uint64 beginStamp = timestamp();

        // We limit the preloadlist to MAX_PRELOAD_COUNT entries so that
        // we don't hold on to too many potentially deleted resources
        // We delete the entries off the front of the list as they are more
        // likely to have been deleted or already preloaded
        while (preloadResourceList_.size() > MAX_PRELOAD_COUNT)
        {
            preloadResourceList_.front()->Release();
            preloadResourceList_.pop_front();
        }

        // Iterate over the list and preload each entry, once the time limit
        // has been reached or the list is empty, continue, leave any resource
        // that has not been preloaded in the list.
        while ( (timestamp() - beginStamp) < timeLimit && 
            !preloadResourceList_.empty())
        {
            IDirect3DResource9* pResource = preloadResourceList_.front();
            preloadResourceList_.pop_front();

            // Only preload the resource if the current reference count is
            // above 1 (if the reference count is 1, the preload list is the
            // only remaining reference to the resource and we can release it)
            // Since AddRef and Release are the only ways of getting
            // the reference count of an IUnknown object we use these methods.
            pResource->AddRef();
            ULONG refCount = pResource->Release();
            if (refCount > 1)
            {
                pResource->PreLoad();
            }
            pResource->Release();
        }

        preloadResourceMutex_.give();
        s_numPreloadResources = (uint32)(preloadResourceList_.size());
    }
    enablePreloadResources_ = true;
}

/**
 *  This method adds a resource to the preload list
 *  @param preloadResource the d3d resource to add to the list
 */
void RenderContext::addPreloadResource(IDirect3DResource9* preloadResource)
{
    BW_GUARD;

    // Don't add it if we are disabled
    if (enablePreloadResources_ && !usingD3DDeviceEx_)
    {
        SimpleMutexHolder smh( preloadResourceMutex_ );
        preloadResource->AddRef();
        preloadResourceList_.push_back( preloadResource );

        if (preloadResourceList_.size() > 2000)
        {
            WARNING_MSG( "RenderContext::addPreloadResource: Too many preload resources, "
                "you may want to call Moo::rc().preloadDeviceResources to flush them.\n" );
        }
    }
}

/**
 *  This method puts a D3D texture into the array for future reuse.
 *  @param ComObjectWrap< DX::Texture > D3D texture.
 */
void RenderContext::putTextureToReuseList(const ComObjectWrap< DX::Texture > & tex)
{
    textureReuseCache_.putTexture( tex );
}

/**
 *  Flush all textures out of the texture reuse list
 */
void RenderContext::clearTextureReuseList()
{
    textureReuseCache_.clear();
}

/**
 * This method fetches the list of textures currently stored
 * in the reuse cache. Used for statistics and reporting.
 */
TextureReuseCache & RenderContext::textureReuseCache()
{
    return textureReuseCache_;
}


/**
 *  This method gets a D3D texture from the reuse list
 *  if texture dimensions, usage, num mips, pool and format match
  * @param uint32 requested texture width
  * @param uint32 requested texture height
  * @param int requested texture numLevels
  * @param uint32 requested texture usage
  * @param D3DFORMAT requested texture format
  * @param D3DPOOL requested texture pool
 */
ComObjectWrap< DX::Texture >    RenderContext::getTextureFromReuseList( 
    uint32 width, uint32 height, int nLevels,
    uint32 usage, D3DFORMAT fmt, D3DPOOL pool)
{
    pool = Moo::rc().patchD3DPool( pool );
    return textureReuseCache_.requestTexture( width, height, nLevels, usage, fmt, pool );
}

/**
 *  This method disables the DX resource preloads,
 *  it also empties the preload list
 *  the preload list is enabled again when 
 *  RenderContext::preloadDeviceResources is called
 */
void RenderContext::disableResourcePreloads()
{
    SimpleMutexHolder smh(preloadResourceMutex_);
    while (!preloadResourceList_.empty())
    {
        preloadResourceList_.back()->Release();
        preloadResourceList_.pop_back();
    }
    enablePreloadResources_ = false;
}


} // namespace Moo

BW_END_NAMESPACE

// render_context.cpp
