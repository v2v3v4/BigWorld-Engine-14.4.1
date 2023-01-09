#include "pch.hpp"
#if SCALEFORM_SUPPORT
#include "manager.hpp"
#include "py_movie_view.hpp"
#include "util.hpp"
#include "moo/render_context.hpp"
#include "ime.hpp"

#include "resmgr/dataresource.hpp"
#include "duplo/pymodel.hpp"

//Scaleform::GFx::AS3::Classes::ClassRegistrationTable singleton, this #include must be put somewhere only once
#include <AS3/AS3_Global.h>
#include <AMP/Amp_Server.h>

#include <GFx/GFx_FilterDesc.h>

#include "sys_alloc.hpp"

#if ENABLE_RESOURCE_COUNTERS
#include "cstdmf/resource_counters.hpp"
#endif

DECLARE_DEBUG_COMPONENT2( "Scaleform", 0 )

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }
#endif

BW_BEGIN_NAMESPACE

BW_INIT_SINGLETON_STORAGE( ScaleformBW::Manager )

namespace ScaleformBW
{
	extern int PyMovieDef_token;
	extern int FlashGUIComponent_token;
	extern int FlashTextGUIComponent_token;

	int tokenSet = PyMovieDef_token | FlashGUIComponent_token | FlashTextGUIComponent_token;

#if SCALEFORM_IME
	extern int IME_token;
	int imeTokenSet = IME_token;
#endif


//-----------------------------------------------------------------------------
// Section - Manager
//-----------------------------------------------------------------------------
	Manager::Manager() :
		pLoader_(NULL),
		lastMouseButtons_(0),
		needsReloadUnmanaged_(false),
		isAmpEnabled_(false),
		timeBeforeUpdateLogitechLCD_(0)
#if ENABLE_WATCHERS
		,lastNumDrawCalls_(0)
		,lastNumTriangles_(0)
#endif
		,system_(&SysAlloc::instance())
#if ENABLE_RESOURCE_COUNTERS
		,lastAMPFrameProfileTime_(-1.0)
		,lastAMPFrameProfile_(NULL)
		,currAMPFrameProfile_(NULL)
#endif
	{
		BW_GUARD;

		MF_WATCH("Render/Scaleform/DrawCalls", lastNumDrawCalls_, Watcher::WT_READ_ONLY);
		MF_WATCH("Render/Scaleform/Triangles", lastNumTriangles_, Watcher::WT_READ_ONLY);

		MF_VERIFY( doInit() );
	}

	Manager::~Manager()
	{
		BW_GUARD;

		doFini();

#if ENABLE_RESOURCE_COUNTERS
		if (lastAMPFrameProfile_ != NULL)
		{
			delete lastAMPFrameProfile_;
			delete currAMPFrameProfile_;
			lastAMPFrameProfile_ = NULL;
			currAMPFrameProfile_ = NULL;
		}
#endif
	}

	bool Manager::doInit()
	{
		BW_GUARD;
		INFO_MSG( "BW::ScaleformBW::Manager initialised\n" );

		// Prior to everything else, do basic setup of restricting flags
		GFx::setRestrictFlashFilters(true);

		pRenderHAL_ = *new Render::D3D9::HAL();
		if (!(pRenderer_ = *new Render::Renderer2D(pRenderHAL_.GetPtr())))
		{
			ERROR_MSG( "%s: %s\n", "BW::Manager::doInit: ", "Couldn't initialize Renderer2D object");
			return false;
		}

		// Configure renderer in "Dependent mode", honoring externally
		// configured device settings.
		if (!pRenderHAL_->InitHAL(Render::D3D9::HALInitParams(Moo::rc().device(),
			Moo::rc().presentParameters(), Render::D3D9::HALConfig_NoSceneCalls)))
		{
			ERROR_MSG( "%s: %s\n", "BW::Manager::doInit: ", "Couldn't initialize RenderHAL object");
			return false;
		}

		pLoader_ = new Loader();

		// Setup logging
		pLogger_ = *new Log();
#ifndef CONSUMER_CLIENT
		pLoader_->SetLog(pLogger_); //this line was commented out in SF 3.3 integration, maybe for sake of optimization
		SF::Log::SetGlobalLog(pLogger_);

		Ptr<GFx::ActionControl> pactControl = *new GFx::ActionControl(
			GFx::ActionControl::Action_LogAllFilenames);
		pLoader_->SetActionControl(pactControl);

		// This would give us A LOT more debugging information - (while GFX/SWF parsing)
		//Ptr<SF::GFx::ParseControl> pparseControl = *new ParseControl(ParseControl::VerboseParse);
		//pLoader_->SetParseControl(pparseControl);
#endif

		if (isAmpEnabled_)
		{
			GFx::AMP::MessageAppControl caps;
			caps.SetCurveToleranceDown(true);
			caps.SetCurveToleranceUp(true);
			caps.SetNextFont(true);
			caps.SetRestartMovie(true);
			caps.SetToggleAaMode(true);
			caps.SetToggleAmpRecording(true);
			caps.SetToggleFastForward(true);
			caps.SetToggleInstructionProfile(true);
			caps.SetToggleOverdraw(true);
			caps.SetToggleBatch(true);
			caps.SetToggleStrokeType(false);
			caps.SetTogglePause(true);
			caps.SetToggleWireframe(true);
			
			AmpServer::GetInstance().SetAppControlCaps(&caps);
			AmpServer::GetInstance().SetMemReports(true, true);
			AmpServer::GetInstance().OpenConnection();

			Ptr<GFx::AMP::ServerState> ampState = *SF_NEW GFx::AMP::ServerState();
			ampState->StateFlags = AmpServer::GetInstance().GetCurrentState();
			ampState->CurveToleranceMin = 0.5f;
			ampState->CurveToleranceMax = 10.0f;
			ampState->CurveToleranceStep = 0.5f;
			ampState->ConnectedApp = L"bwclient";
			AmpServer::GetInstance().UpdateState(ampState);
		}
		else
		{
			Ptr<GFx::AMP::ServerState> ampState = *SF_NEW GFx::AMP::ServerState();
			ampState->StateFlags = GFx::AMP::Amp_Disabled;
			AmpServer::GetInstance().UpdateState(ampState);
			AmpServer::Uninit();
		}

		//Set the loader as our provider of fonts.  When fonts are added to the loader,
		//we inherit these as well.
		pDrawTextManager_ = *new GFx::DrawTextManager(pLoader_);

		// Enabling ActionScript XML Support
		Ptr<GFx::XML::Parser> pexpatXmlParser = *new GFx::XML::ParserExpat;
		Ptr<GFx::XML::SupportBase> pxmlSupport = *new GFx::XML::Support(pexpatXmlParser);
		pLoader_->SetXMLSupport(pxmlSupport);

		// Setup default font provider.  In practice we should be loading these
		// from .swf font libraries, ie. using the font lib below
		Ptr<GFx::FontProviderWin32> fontProvider = *new GFx::FontProviderWin32(::GetDC(0));
		pLoader_->SetFontProvider(fontProvider);

		// ActionScript support
		Ptr<GFx::ASSupport> pASSupport = *new GFx::AS3Support();
		pLoader_->SetAS3Support(pASSupport);
		Ptr<GFx::ASSupport> pAS2Support = *new GFx::AS2Support();
		pLoader_->SetAS2Support(pAS2Support);

		pFontMap_ = *new GFx::FontMap();
		pLoader_->SetFontMap(pFontMap_);
		pFontLib_ = *new GFx::FontLib();
		pLoader_->SetFontLib(pFontLib_);
		pTexMgr_ = (Render::D3D9::TextureManager*)pRenderHAL_->GetTextureManager();


		/* GFx Sound System initialization */
		// check that fmodsound sybsytem initialized
		if (SoundManager::instance().isInitialised())
		{
			Ptr<Sound::SoundRendererFMOD> pSoundRenderer = Sound::SoundRendererFMOD::CreateSoundRenderer();
			
			FMOD::System    *system;
			FMOD::EventSystem* eventSystem = SoundManager::instance().getEventSystemObject();
			if (!eventSystem ||
				eventSystem->getSystemObject( &system ) != FMOD_OK ||
				!pSoundRenderer->Initialize( system ))
			{
				ERROR_MSG( "%s: %s\n", "BW::Manager::doInit: ", "Couldn't initialize GSoundRendererFMOD object");
				return false;
			}
			
			Ptr<GFx::Audio> pAudioState = *new GFx::Audio(pSoundRenderer);
			pLoader_->SetAudio(pAudioState);

		}

		return true;
	}


	bool Manager::doFini()
	{
		BW_GUARD;

#ifndef CONSUMER_CLIENT
		if (isAmpEnabled_)
		{
			isAmpEnabled_= false;
			AmpServer::GetInstance().CloseConnection();
		}
#endif

		pLoader_->SetVideo(NULL);	
		pLoader_->SetAudio(NULL);
		pDrawTextManager_ = NULL;
		delete pLoader_;
		pLoader_ = NULL;

		pRenderer_ = NULL;
		pRenderHAL_ = NULL;
		pLogger_ = NULL;
		pTexMgr_ = NULL;
		
		INFO_MSG( "BW::Manager finalised\n" );

		return true;
	} 


	struct AdvanceFn
	{
		Float time_;
		UINT catchup_;

		AdvanceFn(Float time, UINT catchup): time_(time), catchup_(catchup) {}

		void operator () ( PyMovieView* elem )
		{
			BW_GUARD;
			GFx::Movie * mv = elem->pMovieView();
			if ( mv && mv->GetVisible() )
			{
				mv->Advance(time_, 0/*catchup_*/, false);
			}
		}
	};


	void Manager::tick(float elapsedTime, uint32 frameCatchUp)
	{
		AdvanceFn afn( elapsedTime, (UINT)frameCatchUp );
		std::for_each(movies_.begin(), movies_.end(), afn);

		if (isAmpEnabled_)
			AmpServer::GetInstance().AdvanceFrame();

		if (timeBeforeUpdateLogitechLCD_ >= 0)
			timeBeforeUpdateLogitechLCD_ -= elapsedTime;
	}	

	void Manager::draw()
	{
		BW_GUARD;

		if (!pRenderer_)
			return;

		if(needsReloadUnmanaged_)
		{
			deleteUnmanagedObjects();
			createUnmanagedObjects();
			needsReloadUnmanaged_ = false;
		}

		if (!Moo::rc().checkDevice())
			return;

		GFx::Viewport gvFullScreen;
		fullScreenViewport( gvFullScreen );

		pRenderer_->BeginFrame();

		for (BW::list<PyMovieView*>::iterator it = movies_.begin(); it != movies_.end(); ++it)
		{
			GFx::Movie *pMovieView = (*it)->pMovieView();
			GFx::MovieDisplayHandle &movieDisplayHandle = (*it)->movieDisplayHandle();

			pMovieView->Capture();
			pMovieView->SetViewport(gvFullScreen);
			if (movieDisplayHandle.NextCapture(pRenderer_->GetContextNotify()))
				pRenderer_->Display(movieDisplayHandle);
		}

		pRenderer_->EndFrame();

#if ENABLE_WATCHERS
		Render::HAL::Stats stats;
		pRenderHAL_->GetStats(&stats, false);
		lastNumDrawCalls_ = stats.Primitives;
		lastNumTriangles_ = stats.Triangles;
#endif

#if ENABLE_RESOURCE_COUNTERS
		double currTime = TimeStamp::toSeconds(timestamp());
		if (isAmpEnabled_ && currTime - lastAMPFrameProfileTime_ >= 1.0)
		{
			updateAMPProfileResCounters();
			lastAMPFrameProfileTime_ = currTime;
		}
#endif

		//TODO - take this out when we have re-enabled the D3D wrapper,
		//when we use the wrapper, scaleform will be calling via our
		//own setRenderState methods, instead of going straight to the
		//device and causing issues with later draw calls.
		Moo::rc().initRenderStates();
	}


	void Manager::deleteUnmanagedObjects()
	{
		BW_GUARD;
		TRACE_MSG( "BW::Manager::deleteUnmanagedObjects\n" );

		if (pRenderHAL_)
			pRenderHAL_->PrepareForReset();
	}


	void Manager::createUnmanagedObjects()
	{
		BW_GUARD;
		TRACE_MSG( "BW::Manager::createUnmanagedObjects\n" );

		if (pRenderHAL_)
			pRenderHAL_->RestoreAfterReset();
	}


	void Manager::createManagedObjects()
	{
	}


	void Manager::deleteManagedObjects()
	{
	}



	void Manager::setDrawTextManager(Ptr<GFx::DrawTextManager> dtm)
	{
		BW_GUARD;
		pDrawTextManager_ = dtm;
		//pDrawTextManager_->SetRenderConfig( pRenderConfig_ ); !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! TODO-SCALEFORM
	}


	/*~ module Scaleform
	 *	@components{ client }
	 *
	 *	The Scaleform module provides access to the Scaleform integration, if
	 *	it is enabled.
	 */
	/*~ function Scaleform.mapFont
	 *	This function provides a means by which an alias can be set up for
	 *	font names.  Any fonts mapped to names via this function will be
	 *	used by movies by being loaded.
	 *
	 *	@param	String		font name embedded in movies.
	 *	@param	String		actual font to use at runtime.
	 */
	void mapFont( const BW::string fontA, const BW::string& fontB,
		uint style = GFx::FontMap::MFF_Original, float scaleFactor = 1.0f)
	{
		BW_GUARD;
		Manager::instance().pFontMap()->MapFont(fontA.c_str(), fontB.c_str(),
			(GFx::FontMap::MapFontFlags)style, scaleFactor);
	}
	PY_AUTO_MODULE_FUNCTION(RETVOID, mapFont, ARG(BW::string, ARG(BW::string, OPTARG(uint, 0x0010, OPTARG(float, 1.0f, END)))), _Scaleform )


	//-------------------------------------------------------------------------
	// section - Global movie views
	//-------------------------------------------------------------------------
	void Manager::addMovieView(PyMovieView* obj)
	{
		BW_GUARD;
		//we don't increment the refcount, we are storing
		//raw pointers to these objects, so that the holder
		//of the python movie view object controls its lifetime.
		//removeMovieView is called when the movie view is destructed.
		movies_.push_back( obj );

		// This would give us A LOT more debugging information - (for ActionScript)
		//Ptr<GFxActionControl> pactionControl = *new GFxActionControl();
		//pactionControl->SetVerboseAction( true );
		//pactionControl->SetActionErrorSuppress( false );
		//obj->impl->SetActionControl(pactionControl);
	}


	void Manager::removeMovieView(PyMovieView* obj)
	{
		BW_GUARD;
		BW::list<PyMovieView*>::iterator itr = std::find(movies_.begin(), movies_.end(), obj);
		if (itr != movies_.end())
		{
			movies_.erase(itr);
		}
	}


	//-------------------------------------------------------------------------
	// section - Key Handling
	//-------------------------------------------------------------------------
	HandleEventFn::HandleEventFn(const GFx::Event& gfxevent):
			gfxevent_(gfxevent),
			handled_(false)
	{
		BW_GUARD;
	}


	void HandleEventFn::operator () ( PyMovieView* elem )
	{
		BW_GUARD;
		
		GFx::Movie* mv = elem->pMovieView();
		if ( !handled_ && mv && mv->GetVisible() )
		{
			handled_ = ( mv->HandleEvent(gfxevent_) == GFx::Movie::HE_Completed );
		}
	}


	bool Manager::onKeyEvent(const KeyEvent & event)
	{
		BW_GUARD;
		bool handled = false;

		//BigWorld key events also contain mouse key events.  Scaleform requires
		//key and mouse events to be separate.
		if (event.key() >= KeyCode::KEY_MINIMUM_MOUSE && event.key() <= KeyCode::KEY_MAXIMUM_MOUSE)
		{
			POINT mousePos;
			::GetCursorPos( &mousePos );
			::ScreenToClient( Moo::rc().windowHandle(), &mousePos );
			//TODO - remove below.  build mouse key state dynamically as needed.
			int buttons = mouseButtonsFromEvent( event );
			lastMouseButtons_ = buttons;
			handled = this->onMouse(  buttons, 0, mousePos.x, mousePos.y );
		}
		else
		{
			// keydown events are piggybacked with the char event.
			// keyup events come without a char.
			GFx::KeyEvent gfxevent = translateKeyEvent( event );
			if (gfxevent.KeyCode != SF::Key::None)
			{
				HandleEventFn fn(gfxevent);
				//note - std::for_each copies the fn object passed in, and
				//uses the copy.  thankfully it returns the copy that it used.
				handled |= std::for_each(movies_.begin(), movies_.end(), fn).handled();
			}

			if ( event.utf16Char()[0] != 0 )
			{
				GFx::CharEvent gfxevent( * (const UInt32*)(event.utf16Char()) );
				HandleEventFn fn(gfxevent);
				handled |= std::for_each(movies_.begin(), movies_.end(), fn).handled();
			}
		}

		return handled;
	}


	//-------------------------------------------------------------------------
	// section - Mouse handling
	//-------------------------------------------------------------------------
	NotifyMouseStateFn::NotifyMouseStateFn(float x_, float y_, int buttons_, Float scrollDelta_)
			: x(x_), y(y_), buttons(buttons_), scrollDelta(scrollDelta_), handled_( false )
	{
	}


	void NotifyMouseStateFn::operator () ( PyMovieView* elem )
	{
		BW_GUARD;
		if ( handled_ )
			return;

		GFx::Movie* movie = elem->pMovieView();

		if( !movie->GetVisible() )
			return;

		//TODO - can maybe hittest against the scaleform movie before continuing
		GFx::Viewport view;
		movie->GetViewport(&view);

		x -= view.Left;
		y -= view.Top;

		// Adjust x, y to viewport.
		GSizeF  s;
		s.Width     = ( movie->GetMovieDef()->GetFrameRect().Width() / view.Width );
		s.Height    = ( movie->GetMovieDef()->GetFrameRect().Height() / view.Height );
		Float	mX = ( x * s.Width );
		Float	mY = ( y * s.Height );
		Render::Matrix m;
		// apply viewport transforms here...
		Render::PointF p = m.TransformByInverse( Render::PointF( mX, mY ) );
		float vx = p.x / s.Width;
		float vy = p.y / s.Height;

		if (scrollDelta == 0.0f)
		{
			// Fire event
			movie->NotifyMouseState(vx, vy, buttons);
		}
		else
		{
			// TODO - should be vx, vy?
			if (movie->HitTest(x, y, GFx::Movie::HitTest_ShapesNoInvisible))
			{
				GFx::MouseEvent gfxevent(GFx::Event::MouseWheel, buttons, x, y, scrollDelta);
				handled_ |= ( movie->HandleEvent(gfxevent) == GFx::Movie::HE_Completed );
			}
		}
	}


	bool Manager::onMouse(int buttons, int nMouseWheelDelta, int xPos, int yPos)
	{
		BW_GUARD;
		bool handled = false;
		NotifyMouseStateFn msFn = NotifyMouseStateFn( (float)xPos, (float)yPos, buttons, (Float)((nMouseWheelDelta / WHEEL_DELTA) * 3) );
		handled = std::for_each(movies_.begin(), movies_.end(), msFn).handled();
		return handled;
	}


	void Manager::onDrawClipboard(HWND hWnd) {
		if (!IsClipboardFormatAvailable(CF_UNICODETEXT)) {
			return; 
		}
		if (hWnd == NULL) {
			hWnd = GetOpenClipboardWindow();
		}
		if (!OpenClipboard(hWnd)) {
		    return; 
		}
		HGLOBAL hglb = GetClipboardData(CF_UNICODETEXT); 
		if (hglb != NULL) { 
			const wchar_t* pwstr = (const wchar_t*)GlobalLock(hglb); 
			if (pwstr != NULL) {
				Ptr<GFx::TextClipboard> pClipboard = this->pLoader()->GetTextClipboard();
				if (pClipboard) {
					pClipboard->SetText(pwstr);
				}
			}
            GlobalUnlock(hglb); 
        } 
		CloseClipboard(); 
	}

#if ENABLE_RESOURCE_COUNTERS
	
	void Manager::updateAMPProfileResCounters()
	{
		BW_GUARD;

		static ResourceCounters::DescriptionPool poolDescrGPUImageMem("ScaleformAMP/GPUMemory/Images", (uint)ResourceCounters::MP_DEFAULT, ResourceCounters::RT_TEXTURE);
		static ResourceCounters::DescriptionPool poolDescrGPUMeshCacheMem("ScaleformAMP/GPUMemory/MeshCache", (uint)ResourceCounters::MP_DEFAULT, ResourceCounters::RT_VERTEX_BUFFER);
	
		static ResourceCounters::DescriptionPool poolDescrMovieDataMem("ScaleformAMP/SysMemory/MovieData", (uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER);
		static ResourceCounters::DescriptionPool poolDescrMovieViewMem("ScaleformAMP/SysMemory/MovieView", (uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER);
		static ResourceCounters::DescriptionPool poolDescrSoundMem("ScaleformAMP/SysMemory/Sound", (uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER);
		static ResourceCounters::DescriptionPool poolDescrFontCacheMem("ScaleformAMP/SysMemory/FontCache", (uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER);
		static ResourceCounters::DescriptionPool poolDescrSWFVideoDataMem("ScaleformAMP/SysMemory/SWFVideo", (uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER);

		bool needAddTextureCounters = false;

		if (lastAMPFrameProfile_ == NULL)
		{
			lastAMPFrameProfile_ = new GFx::AMP::ProfileFrame();
			currAMPFrameProfile_ = new GFx::AMP::ProfileFrame();

			MF_WATCH("Render/Scaleform/SysMemoryTotal", currAMPFrameProfile_->TotalMemory, Watcher::WT_READ_ONLY);

			MF_WATCH("Render/Scaleform/GPUMemory/Images", currAMPFrameProfile_->ImageMemory, Watcher::WT_READ_ONLY);
			MF_WATCH("Render/Scaleform/GPUMemory/MeshCache", currAMPFrameProfile_->MeshCacheGraphicsMemory, Watcher::WT_READ_ONLY);

			MF_WATCH("Render/Scaleform/SysMemory/MovieData", currAMPFrameProfile_->MovieDataMemory, Watcher::WT_READ_ONLY);
			MF_WATCH("Render/Scaleform/SysMemory/MovieView", currAMPFrameProfile_->MovieViewMemory, Watcher::WT_READ_ONLY);
			MF_WATCH("Render/Scaleform/SysMemory/Sound", currAMPFrameProfile_->SoundMemory, Watcher::WT_READ_ONLY);
			MF_WATCH("Render/Scaleform/SysMemory/FontCache", currAMPFrameProfile_->FontCacheMemory, Watcher::WT_READ_ONLY);
			MF_WATCH("Render/Scaleform/SysMemory/SWFVideo", currAMPFrameProfile_->VideoMemory, Watcher::WT_READ_ONLY);
		}
		else
		{
			currAMPFrameProfile_->ImageMemory = currAMPFrameProfile_->ImageGraphicsMemory 
				= currAMPFrameProfile_->MeshCacheMemory = currAMPFrameProfile_->MeshCacheGraphicsMemory
				= currAMPFrameProfile_->MovieDataMemory = currAMPFrameProfile_->MovieViewMemory
				= currAMPFrameProfile_->SoundMemory = currAMPFrameProfile_->FontCacheMemory
				= currAMPFrameProfile_->VideoMemory = 0;

			currAMPFrameProfile_->ImageList.Clear();
			currAMPFrameProfile_->MovieStats.Clear();
			currAMPFrameProfile_->SwdHandles.Clear();
			currAMPFrameProfile_->FileHandles.Clear();

			AmpServer &ampServerInst = AmpServer::GetInstance();
			GFx::AMP::Server &gfxAmpServer = *((GFx::AMP::Server*)&ampServerInst);
			gfxAmpServer.CollectMemoryData(currAMPFrameProfile_);
			gfxAmpServer.CollectMeshCacheStats(currAMPFrameProfile_);
			gfxAmpServer.CollectRendererData(currAMPFrameProfile_);
			gfxAmpServer.CollectMovieData(currAMPFrameProfile_);

			RESOURCE_COUNTER_SUB(poolDescrGPUImageMem, lastAMPFrameProfile_->ImageMemory, lastAMPFrameProfile_->ImageMemory);
			RESOURCE_COUNTER_SUB(poolDescrGPUMeshCacheMem, lastAMPFrameProfile_->MeshCacheGraphicsMemory, lastAMPFrameProfile_->MeshCacheGraphicsMemory);
			
			RESOURCE_COUNTER_SUB(poolDescrMovieDataMem, lastAMPFrameProfile_->MovieDataMemory, 0);
			RESOURCE_COUNTER_SUB(poolDescrMovieViewMem, lastAMPFrameProfile_->MovieViewMemory, 0);
			RESOURCE_COUNTER_SUB(poolDescrSoundMem, lastAMPFrameProfile_->SoundMemory, 0);
			RESOURCE_COUNTER_SUB(poolDescrFontCacheMem, lastAMPFrameProfile_->FontCacheMemory, 0);
			RESOURCE_COUNTER_SUB(poolDescrSWFVideoDataMem, lastAMPFrameProfile_->VideoMemory, 0);
		}

		RESOURCE_COUNTER_ADD(poolDescrGPUImageMem, currAMPFrameProfile_->ImageMemory, currAMPFrameProfile_->ImageMemory);
		RESOURCE_COUNTER_ADD(poolDescrGPUMeshCacheMem, lastAMPFrameProfile_->MeshCacheGraphicsMemory, lastAMPFrameProfile_->MeshCacheGraphicsMemory);
		
		RESOURCE_COUNTER_ADD(poolDescrMovieDataMem, currAMPFrameProfile_->MovieDataMemory, 0);
		RESOURCE_COUNTER_ADD(poolDescrMovieViewMem, currAMPFrameProfile_->MovieViewMemory, 0);
		RESOURCE_COUNTER_ADD(poolDescrSoundMem, currAMPFrameProfile_->SoundMemory, 0);
		RESOURCE_COUNTER_ADD(poolDescrFontCacheMem, currAMPFrameProfile_->FontCacheMemory, 0);
		RESOURCE_COUNTER_ADD(poolDescrSWFVideoDataMem, currAMPFrameProfile_->VideoMemory, 0);

		GFx::AMP::ProfileFrame *pTemp = lastAMPFrameProfile_;
		lastAMPFrameProfile_ = currAMPFrameProfile_;
		currAMPFrameProfile_ = pTemp;
	}

#endif // ENABLE_RESOURCE_COUNTERS

	void Manager::setMovieSoundMuted(bool mute)
	{
		if (SoundManager::instance().isInitialised())
		{
			GFx::Audio *pAudio = (GFx::Audio*)pLoader_->GetAudio().GetPtr();
			pAudio->GetRenderer()->SetMasterVolume(mute ? 0.0f : 1.0f);
		}
	}

	static void setMovieSoundMuted(bool mute)
	{
		Manager::instance().setMovieSoundMuted(mute);
	}
	PY_AUTO_MODULE_FUNCTION(RETVOID, setMovieSoundMuted, ARG(bool, END), BigWorld)


} // namespace ScaleformBW

BW_END_NAMESPACE

#endif //#if SCALEFORM_SUPPORT
