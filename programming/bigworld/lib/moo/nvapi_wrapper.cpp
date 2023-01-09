#include "pch.hpp"
#include "nvapi_wrapper.hpp"
#include "moo/effect_visual_context.hpp"

#if ENABLE_BW_NVAPI_WRAPPER
#include "nvapi/NvApiDriverSettings.h"

BW_BEGIN_NAMESPACE

//-- start unnamed namespace.
//--------------------------------------------------------------------------------------------------
namespace
{
    //-- watcher's variables.
    bool    g_enableUpdate = true;
    Vector4 g_stereoStatus(0,0,0,0);

    //-- holder of all listeners of the nvApi wrapper.
    typedef BW::vector<Moo::NvApiWrapper::ICallback*> ICallbacks;
    ICallbacks g_callbacks;


    //-- Sets Nvidia 3D Vision stereo parameters to the shader.
    //--     x - separation
    //--     y - eye separation
    //--     z - convergence 
    //--     w - 0 if stereo disabled, and 1 otherwise
    //--------------------------------------------------------------------------------------------------
    class NvStereoParamsConstant : public Moo::EffectConstantValue
    {
    public:
        bool operator() (ID3DXEffect* pEffect, D3DXHANDLE constantHandle)
        {
            BW_GUARD;
            pEffect->SetVector(constantHandle, &Moo::rc().nvapi().stereoParams());
            return true;
        }
    };

    //-- Sets Nvidia 3D Vision texture stereo parameters to the shader.
    //--     Nvidia automatically sets the right texture fro the right eye.
    //--------------------------------------------------------------------------------------------------
    class NvStereoParamsMapContant : public Moo::EffectConstantValue
    {
    public:
        bool operator() (ID3DXEffect* pEffect, D3DXHANDLE constantHandle)
        {
            BW_GUARD;
            pEffect->SetTexture(constantHandle, Moo::rc().nvapi().stereoParamsMap());
            return true;
        }
    };
}
//--------------------------------------------------------------------------------------------------
//-- end unnamed namespace.

#define WM_NV_STEREO_NOTIFICATION ( WM_USER + 1 )

namespace Moo
{
    //----------------------------------------------------------------------------------------------
    NvApiWrapper::ICallback::ICallback()
    {
        g_callbacks.push_back(this);
    }

    //----------------------------------------------------------------------------------------------
    NvApiWrapper::ICallback::~ICallback()
    {
        BW_GUARD;

        for (uint i = 0; i < g_callbacks.size(); ++i)
        {
            if (this == g_callbacks[i])
            {
                g_callbacks[i] = g_callbacks.back();
                g_callbacks.pop_back();
                break;
            }
        }
    }

    //----------------------------------------------------------------------------------------------
    NvApiWrapper::NvApiWrapper()
        :   m_isInited(false), m_isStereoInited(false), m_is3DSurroundEnabled(false), 
            m_stereoHandle(0), m_stereoParams(0,0,0,0), m_hwndStereo(NULL)
    {
        MF_WATCH("Render/nvapi/enable update", g_enableUpdate, Watcher::WT_READ_WRITE,
            "enable/disable stereo params update."
            );

        MF_WATCH("Render/nvapi/stereo status", g_stereoStatus, Watcher::WT_READ_ONLY,
            "stereo status: x - eye separation, y - separation percents, "
            "z - convergence, w - is stereo enabled."
            );
    }

    //----------------------------------------------------------------------------------------------
    NvApiWrapper::~NvApiWrapper()
    {
        BW_GUARD;
        MF_ASSERT(g_callbacks.size() == 0);

        if (m_isStereoInited)
        {
            if (NvAPI_Stereo_DestroyHandle(m_stereoHandle) == NVAPI_OK)
            {
                NOTICE_MSG("nVidia NvApi stereo handle was destroyed.\n");
            }
            else
            {
                ERROR_MSG("nVidia NvAPI stereo handle destroying failed.\n");
            }
        }

        if (m_isInited)
        {
            if (NvAPI_Unload() == NVAPI_OK)
            {
                NOTICE_MSG("nVidia NvAPI was unloaded.\n");
            }
            else
            {
                ERROR_MSG("nVidia NvAPI unloading failed.\n");
            }
        }

        m_isInited        = false;
        m_isStereoInited  = false;
    }

    //----------------------------------------------------------------------------------------------
    void NvApiWrapper::init()
    {
        BW_GUARD;

        //-- initialize nv API.
        if (NvAPI_Initialize() == NVAPI_OK)
        {
            m_isInited = true;
            NOTICE_MSG("nVidia NvAPI was initialised.\n");
        }
    }

    //----------------------------------------------------------------------------------------------
    void NvApiWrapper::updateDevice(IDirect3DDevice9* device)
    {
        BW_GUARD;

        //-- 1. de-init the stereo handle for old device.
        if (m_isStereoInited)
        {
            m_isStereoInited = false;
            NvAPI_Stereo_DestroyHandle(m_stereoHandle);
        }

        //-- 2. create it for new device.
        if (m_isInited && NvAPI_Stereo_CreateHandleFromIUnknown(device, &m_stereoHandle) == NVAPI_OK)
        {
            initWindow();
            NvAPI_Stereo_SetNotificationMessage(m_stereoHandle, (NvU64)m_hwndStereo, WM_NV_STEREO_NOTIFICATION);
            DEBUG_MSG("NvAPI_Stereo_SetNotificationMessage\n");
        
            m_isStereoInited = true;
            NOTICE_MSG("nVidia NvStereo was initialised.\n");
        }

        //-- 3. do first stereo and surround status update.
        updateStereoStatus();
        update3DSurroundStatus();
        setSLIRenderingMode(!m_disableSLI);
    }

    //----------------------------------------------------------------------------------------------
    void NvApiWrapper::update3DSurroundStatus()
    {
        BW_GUARD;

        m_is3DSurroundEnabled = false;

        if (!m_isInited || !g_enableUpdate) return;

        NvAPI_Status retVal;
        NvU32 displayID = 0;
        //We assume that displayArgs, a struct of device properties such as
        //resolution and fullscreen state, has already been initialized with
        //valid data. Also, we assume that Rect is some previously defined
        //struct type which contains member variables to describe location and
        //size of an arbitrary display rect.

        UINT width = Moo::rc().backBufferDesc().Width;
        UINT height = Moo::rc().backBufferDesc().Height;

        m_3DSurroundSafeUIRegion.left = 0;
        m_3DSurroundSafeUIRegion.top = 0;
        m_3DSurroundSafeUIRegion.right = width;
        m_3DSurroundSafeUIRegion.bottom = height;

        retVal = NvAPI_DISP_GetGDIPrimaryDisplayId(&displayID);
        if (retVal == NVAPI_OK) 
        {
            NV_RECT viewports[NV_MOSAIC_MAX_DISPLAYS];
            NvU8 isBezelCorrected;

            retVal = NvAPI_Mosaic_GetDisplayViewportsByResolution(displayID, width, 
                height, viewports, &isBezelCorrected);

            if (retVal == NVAPI_OK && !Moo::rc().windowed()) 
            {

                NvU32 displayCount = 0;

                while (viewports[displayCount].top != viewports[displayCount].bottom) 
                    displayCount++;

                //Ensure there is more than one viewport rect and that there is an odd 
                //number of displays in the Surround configuration if we want to 
                //restrict the UI to the center display.
                if ((displayCount > 1) && (displayCount & 1)) 
                {
                    bool isSingleDisplayRow = true;
                    for (size_t i = 1; i < displayCount; i++) 
                    {
                        if ((viewports[0].top != viewports[i].top) || (viewports[0].bottom != viewports[i].bottom)) 
                        {
                            isSingleDisplayRow = false;
                            break;
                        }
                    }
                    //Check to make sure that the displays are in a single row together.
                    if (isSingleDisplayRow) 
                    {
                        NvU32 centerDisplayIdx = displayCount / 2;
                        //Check which rect dimension of the center display is larger. If the //height is larger than the width, we assume the displays are arranged //in portrait mode and we leave the UI spanning multiple displays (and //possibly occlude parts of the UI in a bezel-corrected configuration). //If the width is larger than the height, we store that center //display's viewport coordinates and size so we can restrict the UI to //that region for the user's convenience and because we know the UI was //probably designed with this display orientation/aspect ratio in mind. //Alternatively, we could choose to just use the rect regions returned //by NvAPI_Mosaic_GetDisplayViewportsByResolution to place UI elements //on more than one display, while still restricting them to the visible //regions.
                        NvU32 centerWidth = viewports[centerDisplayIdx].right - viewports[centerDisplayIdx].left + 1;
                        NvU32 centerHeight = viewports[centerDisplayIdx].bottom - viewports[centerDisplayIdx].top + 1;
                        if (centerWidth > centerHeight) 
                        {
                            //Restricting UI to center display.
                            m_3DSurroundSafeUIRegion.left = viewports[centerDisplayIdx].left;
                            m_3DSurroundSafeUIRegion.top = viewports[centerDisplayIdx].top;
                            m_3DSurroundSafeUIRegion.right = centerWidth;
                            m_3DSurroundSafeUIRegion.bottom = centerHeight;

                            //NVIDIA Surround is enabled.
                            m_is3DSurroundEnabled = true;
                        }
                    }
                }
            }
        }
    }

    //----------------------------------------------------------------------------------------------
    void NvApiWrapper::updateStereoStatus()
    {
        BW_GUARD;

        if (!m_isInited || !m_isStereoInited || !g_enableUpdate) return;

        //-- update stereo enable status.
        {
            NvU8 isStereoEnabled = 0;
            NvAPI_Stereo_IsEnabled(&isStereoEnabled);

            //-- if isStereoEnabled is 0, it means that or stereo driver is not installed, or our
            //-- video card isn't nVidia's, or stereo is disabled for example when we are in windowed
            //-- mode or maybe it's disabled for some another reason.
            bool enabled = /*!Moo::rc().windowed() &&*/ (isStereoEnabled != 0);

            //-- check changes in stereo enable status and notify listeners if needs.
            if (enabled != (m_stereoParams.w != 0))
            {
                m_stereoParams.w = enabled;
                for (uint i = 0; i < g_callbacks.size(); ++i)
                {
                    g_callbacks[i]->onStereoEnabled(enabled);
                }
            }
        }

        fetchStereoParams();
    }
    
    //----------------------------------------------------------------------------------------------
    bool NvApiWrapper::initWindow()
    {
        BW_GUARD;

        if (m_hwndStereo) return true;
        
        //-- Create a new 0x0 window which will be invisible and responsible for receiving messages
        //-- from nvidia's driver about stereo changes.
        HINSTANCE hInstance = (HINSTANCE)::GetWindowLong( Moo::rc().windowHandle(), GWL_HINSTANCE );

        TCHAR *c_szClassName = __TEXT( "WoT nvApi window" );
        WNDCLASS wc = { 0 };
        wc.hInstance = hInstance;
        wc.lpfnWndProc = WndProc;
        wc.lpszClassName = c_szClassName;
        if ( !::RegisterClass( &wc ) ) return false;

        m_hwndStereo = ::CreateWindow( c_szClassName, 0, 0, 0, 0, 0, 0, 0, 0, hInstance, 0 );
        if (m_hwndStereo == 0) return false;

        ::SetWindowLong(m_hwndStereo, GWL_USERDATA, (LONG_PTR)this );

        DEBUG_MSG("nv message window created\n");

        return true;
    }
    
    //----------------------------------------------------------------------------------------------
    LRESULT NvApiWrapper::WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        BW_GUARD;

        if (uMsg == WM_NV_STEREO_NOTIFICATION)
        {
            NvApiWrapper* pThis = (NvApiWrapper*)::GetWindowLongPtr(hwnd, GWL_USERDATA);
            pThis->onStereoNotification(wParam, lParam);
            return 0;
        }

        return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    
    //----------------------------------------------------------------------------------------------
    void NvApiWrapper::onStereoNotification(WPARAM wParam, LPARAM lParam)
    {
        BW_GUARD;

        //-- Note: for debugging.
        //char str[64];
        //snprintf( str, 64, "nv stereo notification: %x %x\n", wParam, lParam );
        //OutputDebugStringA( str );
        //
        //::MessageBeep( MB_OK );

        bool enabled = LOWORD( wParam ) == 1;

        if (enabled != (m_stereoParams.w != 0))
        {
            m_stereoParams.w = enabled;
            for (uint i = 0; i < g_callbacks.size(); ++i)
            {
                g_callbacks[i]->onStereoEnabled(enabled);
            }
        }
        
        //-- update stereo parameters.
        fetchStereoParams();
    }
    
    //----------------------------------------------------------------------------------------------
    void NvApiWrapper::fetchStereoParams()
    {
        BW_GUARD;

        bool isOk = true;
        
        //-- update shader constant stereo parameters.
        isOk &= (NvAPI_Stereo_GetSeparation(m_stereoHandle, &m_stereoParams.x) == NVAPI_OK);
        isOk &= (NvAPI_Stereo_GetEyeSeparation(m_stereoHandle, &m_stereoParams.y) == NVAPI_OK);
        isOk &= (NvAPI_Stereo_GetConvergence(m_stereoHandle, &m_stereoParams.z) == NVAPI_OK);

        if (!isOk)
        {
            m_stereoParams.setZero();
        }

        //-- update texture constant stereo parameters.
        m_stereoParamsTexture.updateStereoParamsMap(
            Moo::rc().device(), m_stereoParams.y, m_stereoParams.x, m_stereoParams.z, m_stereoParams.w
            );

        //-- update watcher.
        g_stereoStatus = m_stereoParams;
    }

    //----------------------------------------------------------------------------------------------
    void NvApiWrapper::createManagedObjects()
    {
        BW_GUARD;

        if (!m_isInited) return;


        //-- register shared auto-constant.
        Moo::rc().effectVisualContext().registerAutoConstant(
            Moo::EffectVisualContext::AUTO_CONSTANT_TYPE_PER_FRAME, "NvStereoParams",
            new NvStereoParamsConstant()
            );

        //-- register shared auto-constant.
        Moo::rc().effectVisualContext().registerAutoConstant(
            Moo::EffectVisualContext::AUTO_CONSTANT_TYPE_PER_FRAME, "NvStereoParamsMap",
            new NvStereoParamsMapContant()
            );
    }

    //----------------------------------------------------------------------------------------------
    void NvApiWrapper::deleteManagedObjects()
    {
        BW_GUARD;

        if (!m_isInited) return;

        //-- unregister shared auto-constant.
        Moo::rc().effectVisualContext().unregisterAutoConstant(
            Moo::EffectVisualContext::AUTO_CONSTANT_TYPE_PER_FRAME, "NvStereoParams"
            );

        //-- unregister shared auto-constant.
        Moo::rc().effectVisualContext().unregisterAutoConstant(
            Moo::EffectVisualContext::AUTO_CONSTANT_TYPE_PER_FRAME, "NvStereoParamsMap"
            );
    }

    //----------------------------------------------------------------------------------------------
    void NvApiWrapper::createUnmanagedObjects()
    {
        BW_GUARD;

        if (!m_isInited || !m_isStereoInited) return;

        m_stereoParamsTexture.createGraphics(Moo::rc().device());

        //-- update stereo texture right after recreation because it's uninitialized.
        m_stereoParamsTexture.updateStereoParamsMap(
            Moo::rc().device(), m_stereoParams.y, m_stereoParams.x, m_stereoParams.z, m_stereoParams.w
            );
    }

    void NvApiWrapper::disableSLI(bool disable)
    { 
        m_disableSLI = disable;
        setSLIRenderingMode(!disable);
    }

    //----------------------------------------------------------------------------------------------
    void NvApiWrapper::deleteUnmanagedObjects()
    {
        BW_GUARD;

        if (!m_isInited || !m_isStereoInited) return;

        m_stereoParamsTexture.destroyGraphics();
    }

    //----------------------------------------------------------------------------------------------
    bool NvApiWrapper::recreateForD3DExDevice() const
    {
        return true;
    }

    bool NvApiWrapper::setSLIRenderingMode(bool returnToDefault)
    {
        NvAPI_Status status;
        // (1) Create the session handle to access driver settings
        NvDRSSessionHandle hSession = 0;
        status = NvAPI_DRS_CreateSession(&hSession);
        if (status == NVAPI_OK)
        {
            // (2) load all the system settings into the session
            status = NvAPI_DRS_LoadSettings(hSession);
            if (status == NVAPI_OK)
            {
                // (3) Obtain the Base profile. Any setting needs to be inside
                // a profile, putting a setting on the Base Profile enforces it
                // for all the processes on the system
                NvDRSProfileHandle hProfile = 0;
                NvU16* bwclientName = (NvU16*)L"bwclient";
                status = NvAPI_DRS_FindProfileByName(hSession, bwclientName, &hProfile);
                if (status == NVAPI_OK)
                {
                    if(returnToDefault)
                    {
                        status = NvAPI_DRS_RestoreProfileDefault(hSession, hProfile);
                    }
                    else
                    {
                        // (4) Get last SLI setting
                        // first we fill the NVDRS_SETTING struct, then we call the function
                        NVDRS_SETTING drsSetting = {0};
                        drsSetting.version = NVDRS_SETTING_VER;
                        //Set predefined mode FORCE SINGLE
                        drsSetting.settingId = SLI_PREDEFINED_MODE_ID;
                        drsSetting.settingType = NVDRS_DWORD_TYPE;
                        drsSetting.u32CurrentValue = SLI_PREDEFINED_MODE_FORCE_SINGLE;
                        status = NvAPI_DRS_SetSetting(hSession, hProfile, &drsSetting);
                        //Set predefined GPU count ONE
                        drsSetting.settingId = SLI_PREDEFINED_GPU_COUNT_ID;
                        drsSetting.settingType = NVDRS_DWORD_TYPE;
                        drsSetting.u32CurrentValue = SLI_PREDEFINED_GPU_COUNT_ONE;
                        status = NvAPI_DRS_SetSetting(hSession, hProfile, &drsSetting);
                        //Override global SLI rendering mode for current profile to SINGLE GPU
                        drsSetting.settingId = SLI_RENDERING_MODE_ID;
                        drsSetting.settingType = NVDRS_DWORD_TYPE;
                        drsSetting.u32CurrentValue = SLI_RENDERING_MODE_FORCE_SINGLE;
                        status = NvAPI_DRS_SetSetting(hSession, hProfile, &drsSetting);
                    }

                    if (status == NVAPI_OK)
                    {
                        // (5) Now we apply (or save) our changes to the system
                        status = NvAPI_DRS_SaveSettings(hSession);

                    }
                }
            }
            // (6) We clean up. This is analogous to doing a free()
            NvAPI_DRS_DestroySession(hSession);
        }
        return false;
    }

    bool NvApiWrapper::enableD3DResourceSLITracking( IUnknown* pResource, bool enable )
    {
        DX::Device* pDevice = Moo::rc().device();

        if(!pDevice || !pResource)
            return false;
        
        NVDX_ObjectHandle hResource;
        NvAPI_Status status;
        status = NvAPI_D3D_GetObjectHandleForResource( pDevice, pResource, &hResource );
        if( status != NVAPI_OK)
            return false;

        NvU32 hintVal = (enable ? 0 : 1);
        status = NvAPI_D3D_SetResourceHint( pDevice, 
                                            hResource, 
                                            NVAPI_D3D_SRH_CATEGORY_SLI, 
                                            NVAPI_D3D_SRH_SLI_APP_CONTROLLED_INTERFRAME_CONTENT_SYNC, 
                                            &hintVal
                                          );
        return (status == NVAPI_OK);
    }

    int NvApiWrapper::getCurAFRGroupIndex()
    {
        DX::Device* pDevice = Moo::rc().device();

        if(!pDevice)
            return 0;

        NV_GET_CURRENT_SLI_STATE sliState;
        sliState.version = NV_GET_CURRENT_SLI_STATE_VER;
        NvAPI_Status status = NvAPI_D3D_GetCurrentSLIState( pDevice, &sliState);
    
        if ( status != NVAPI_OK || sliState.maxNumAFRGroups == 1 ) // SLI is disabled or incorrectly working
            return 0;

        return sliState.currentAFRIndex;
    }

    //bool NvApiWrapper::beginNvAPIResourceRendering( IUnknown* pResource )
    //{
    //  DX::Device* pDevice = Moo::rc().device();

    //  if(!pDevice || !pResource)
    //      return false;

    //  NVDX_ObjectHandle hResource;
    //  NvAPI_Status status;

    //  status = NvAPI_D3D_GetObjectHandleForResource( pDevice, pResource, &hResource );
    //  if( status != NVAPI_OK)
    //      return false;

    //  status = NvAPI_D3D_BeginResourceRendering(pDevice, hResource, g_SLIRenderFlag);

    //  return (status == NVAPI_OK);
    //}


    //bool NvApiWrapper::endNvAPIResourceRendering( IUnknown* pResource )
    //{
    //  DX::Device* pDevice = Moo::rc().device();

    //  if(!pDevice || !pResource)
    //      return false;

    //  NVDX_ObjectHandle hResource;
    //  NvAPI_Status status;

    //  status = NvAPI_D3D_GetObjectHandleForResource( pDevice, pResource, &hResource );
    //  if( status != NVAPI_OK)
    //      return false;

    //  status = NvAPI_D3D_EndResourceRendering(pDevice, hResource, 0);

    //  return (status == NVAPI_OK);    
    //}

    void NvApiWrapper::getGPUTemperaturesStatistics( BW::stringstream& ss ) const
    {
        NvPhysicalGpuHandle gpuHandles[NVAPI_MAX_PHYSICAL_GPUS] = {0};
        NvU32 gpuCount = 0;
        //Get current GPUs
        NvAPI_Status status = NvAPI_EnumPhysicalGPUs(gpuHandles, &gpuCount);

        if(status == NVAPI_OK)
        {
            NV_GPU_THERMAL_SETTINGS thermalSettings = {0};
            thermalSettings.version = NV_GPU_THERMAL_SETTINGS_VER;
            //thermalSettings.version = NVDRS_SETTING_VER;
            NvAPI_ShortString szName = {0};
            for(size_t i = 0; i < gpuCount; ++i)
            {
                ss << "\t";
                if(NvAPI_GPU_GetFullName(gpuHandles[i], szName) == NVAPI_OK)
                    ss << szName << "\n\t\t";
                else
                    ss << "<Unknown GPU>\n\t\t";
                
                //Get all temperature sensors from GPU
                status = NvAPI_GPU_GetThermalSettings(gpuHandles[i], NVAPI_THERMAL_TARGET_ALL, &thermalSettings);
                if(status == NVAPI_OK)
                {
                    for(size_t j = 0; j < thermalSettings.count; ++j)
                    {
                        //Check sensor type
                        switch(thermalSettings.sensor[j].target)
                        {
                        case NVAPI_THERMAL_TARGET_NONE:
                            case NVAPI_THERMAL_TARGET_GPU:          //GPU core temperature
                                ss << "GPU core temperature : ";
                                break;
                            case NVAPI_THERMAL_TARGET_MEMORY:       //GPU memory temperature
                                ss << "GPU memory temperature : ";
                                break;
                            case NVAPI_THERMAL_TARGET_POWER_SUPPLY: //GPU power supply temperature
                                ss << "GPU power supply temperature : ";
                                break;
                            case NVAPI_THERMAL_TARGET_BOARD:        //GPU board ambient temperature
                                ss << "GPU board ambient temperature : ";
                                break;
                            case NVAPI_THERMAL_TARGET_VCD_BOARD:    //Visual Computing Device Board temperature
                                ss << "Visual Computing Device Board temperature : ";
                                break;
                            case NVAPI_THERMAL_TARGET_VCD_INLET:    //Visual Computing Device Inlet temperature
                                ss << "Visual Computing Device Inlet temperature : ";
                                break;
                            case NVAPI_THERMAL_TARGET_VCD_OUTLET:   //Visual Computing Device Outlet temperature
                                ss << "Visual Computing Device Outlet temperature : ";
                                break;
                            case NVAPI_THERMAL_TARGET_UNKNOWN:
                            default:
                                ss << "Unknown sensor temperature : ";
                                break;
                        }
                        // Display current temperature
                        ss << thermalSettings.sensor[j].currentTemp <<
                            " degrees C\n\t\t";
                    }
                }
                else
                    ss << "ERROR displaying GPU sensors\n";
            }
        }
        else
            ss << "ERROR displaying GPUs\n ";
    }
} //-- Moo

BW_END_NAMESPACE

#endif //-- ENABLE_BW_NVAPI_WRAPPER

