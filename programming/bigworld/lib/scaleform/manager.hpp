#ifndef SCALEFORM_MANAGER_HPP
#define SCALEFORM_MANAGER_HPP
#if SCALEFORM_SUPPORT

#include "cstdmf/init_singleton.hpp"
#include "input/input.hpp"
#include "moo/moo_dx.hpp"
#include "moo/device_callback.hpp"

#include "config.hpp"
#include "log.hpp"
#include "loader.hpp"
#include "py_movie_view.hpp"

BW_BEGIN_NAMESPACE

namespace ScaleformBW
{
	class Manager : public Singleton< Manager >, public Moo::DeviceCallback
	{
	public:
		typedef Manager This;

		Manager();
		virtual ~Manager();

		bool doInit();
		bool doFini();

		//Moo::DeviceCallback virtual methods
		void deleteUnmanagedObjects();
		void createUnmanagedObjects();
		void deleteManagedObjects();
		void createManagedObjects();

		void tick(float elapsedTime, uint32 frameCatchUpCount = 2);
		void draw();
		void onDrawClipboard(HWND hWnd);

		Ptr<Render::Renderer2D> & pRenderer() { return pRenderer_; }
		Render::D3D9::TextureManager *pTexMgr() { return pTexMgr_; }
		Loader* pLoader()					{ return pLoader_; }
		Ptr<Log> pLogger()					{ return pLogger_; }
		Ptr<GFx::FontMap> pFontMap()		{ return pFontMap_; }
		Ptr<GFx::FontLib> pFontLib()		{ return pFontLib_; }
		int lastMouseButtons() const		{ return lastMouseButtons_; }

		Ptr<GFx::DrawTextManager> pTextManager() { return pDrawTextManager_; }
		void setDrawTextManager( Ptr<GFx::DrawTextManager> dtm );

		//Movie View management fns
		void addMovieView(PyMovieView* obj);
		void removeMovieView(PyMovieView* obj);

		//Input fns
		bool onKeyEvent(const KeyEvent &event);
		bool onMouse(int buttons, int nMouseWheelDelta, int xPos, int yPos);
		void enablePygfxMouseHandle(bool state);

		void setMovieSoundMuted(bool mute);

		void needReloadResources() { needsReloadUnmanaged_ = true; };

	private:
		GFx::System system_;
		Loader *pLoader_;
		Ptr<BW::ScaleformBW::Log> pLogger_;
		int lastMouseButtons_;

		Ptr<Render::D3D9::HAL> pRenderHAL_; 
		Ptr<Render::Renderer2D> pRenderer_;
		Render::D3D9::TextureManager *pTexMgr_;

		Ptr<GFx::FontLib> pFontLib_;
		Ptr<GFx::FontMap> pFontMap_;
		Ptr<GFx::DrawTextManager> pDrawTextManager_;

		BW::list<PyMovieView*> movies_;
		
		bool needsReloadUnmanaged_;		
		bool isAmpEnabled_;
		float timeBeforeUpdateLogitechLCD_;

#if ENABLE_WATCHERS
		uint lastNumDrawCalls_;
		uint lastNumTriangles_;
#endif

#if ENABLE_RESOURCE_COUNTERS
		double lastAMPFrameProfileTime_;
		GFx::AMP::ProfileFrame *lastAMPFrameProfile_;
		GFx::AMP::ProfileFrame *currAMPFrameProfile_;

		void updateAMPProfileResCounters();
#endif
	};
} //namespace ScaleformBW

BW_END_NAMESPACE

#endif // #if SCALEFORM_SUPPORT
#endif // SCALEFORM_MANAGER_HPP
