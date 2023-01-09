#pragma once

#include "cstdmf/config.hpp"

#if ENABLE_BW_NVAPI_WRAPPER

#include "device_callback.hpp"
#include "base_texture.hpp"
#include "nvapi/nvapi.h"
#include "nvapi/nvStereo.h"
#include "math/vector4.hpp"

//-- include library file.
#pragma comment(lib, "nvapi.lib")

BW_BEGIN_NAMESPACE

namespace Moo
{
	//-- Tiny wrapper around NvApi and NvStereo libraries.
	//-- Note: We don't return any error code because we guaranty that any call
	//--	   of any interface function will be valid even if we were failed to initialize.
	//----------------------------------------------------------------------------------------------
	class NvApiWrapper : public DeviceCallback
	{
	private:
		//-- make non-copyable.
		NvApiWrapper(const NvApiWrapper&);
		NvApiWrapper& operator = (const NvApiWrapper&);

	public:
		void init();

		//-- Interface for everyone who wants to receive state changes from the nvapi library.
		//-- For example enable/disable stereo.
		class ICallback
		{
		public:
			ICallback();
			virtual ~ICallback();

			virtual void onStereoEnabled(bool /*isEnabled*/) { }
			//-- add some another events here...
		};

	public:
		NvApiWrapper();
		~NvApiWrapper();

		//-- update interface in case when we create or recreate device.
		void				updateDevice(IDirect3DDevice9* device);

		bool 				isInited()					const { return m_isInited; } 
		bool 				isStereoInited()			const { return m_isStereoInited; }
		bool				is3DSurroundEnabled()		const { return m_is3DSurroundEnabled; }
		bool 				isStereoEnabled()			const { return (m_stereoParams.w != 0.0f); }
		Vector4				stereoParams()				const { return m_stereoParams; }
		RECT				get3DSurroundSafeUIRegion() const { return m_3DSurroundSafeUIRegion; }
		DX::BaseTexture*	stereoParamsMap()		  { return m_stereoParamsTexture.getStereoParamMapTexture(); }
		void				disableSLI( bool disable );

		void				getGPUTemperaturesStatistics(BW::stringstream& ss) const;

							// Enable/disable driver resource change tracking. 
							// Used to prevent copying resources from one AFR froup to another on change. 
		bool				enableD3DResourceSLITracking(IUnknown* pResource, bool enable);

		int					getCurAFRGroupIndex();


		//-- interface DeviceCallback.
		virtual void createManagedObjects();
		virtual void deleteManagedObjects();
		virtual void createUnmanagedObjects();
		virtual void deleteUnmanagedObjects();
		virtual bool recreateForD3DExDevice() const;

	private:
		bool						m_isInited;
		bool						m_isStereoInited;
		bool						m_is3DSurroundEnabled;
		StereoHandle				m_stereoHandle;
		nv::StereoParametersD3D9	m_stereoParamsTexture;
		Vector4						m_stereoParams;
		HWND						m_hwndStereo;
		RECT						m_3DSurroundSafeUIRegion;
		bool						m_disableSLI;

		//-- use built-in nvidia's approach to receive messages about stereo changes. 
		bool		initWindow();
		void		updateStereoStatus();
		void		update3DSurroundStatus();
		void		fetchStereoParams();
		void		onStereoNotification( WPARAM, LPARAM );
		static		LRESULT CALLBACK WndProc( HWND, UINT, WPARAM, LPARAM );
		bool		setSLIRenderingMode(bool returnToDefault);
	};

} //-- Moo

BW_END_NAMESPACE
#else

BW_BEGIN_NAMESPACE

namespace Moo
{

	//-- Empty wrapper.
	//----------------------------------------------------------------------------------------------
	class NvApiWrapper
	{
	private:
		//-- make non-copyable.
		NvApiWrapper(const NvApiWrapper&);
		NvApiWrapper& operator = (const NvApiWrapper&);

	public:
		class ICallback
		{
		public:
			ICallback() { }
			virtual ~ICallback() { }

			virtual void onStereoEnabled(bool /*isEnabled*/) { }
			//-- add some another events here...
		};

	public:
		NvApiWrapper() { }
		~NvApiWrapper() { }

		void				init() { }

		void				updateDevice(IDirect3DDevice9* /*device*/)	{ }

		bool 				isInited()			const { return false; } 
		bool 				isStereoInited()	const { return false; }
		bool				is3DSurroundEnabled() const { return false; }
		bool 				isStereoEnabled()	const { return false;}
		Vector4				stereoParams()		const { return Vector4(0,0,0,0); }
		DX::BaseTexture*	stereoParamsMap()		  { return NULL; }
		RECT				get3DSurroundSafeUIRegion() const {	RECT r;	memset(&r, 0, sizeof(RECT)); return r; 	}
		void				disableSLI( bool disable ) {};
		void				getGPUTemperaturesStatistics(BW::stringstream& ss) const {};
		bool				enableD3DResourceSLITracking(IUnknown* pResource, bool enable) { return true; };
		int					getCurAFRGroupIndex() { return 0; };
	};

} //-- Moo

BW_END_NAMESPACE

#endif //-- ENABLE_BW_NVAPI_WRAPPER

