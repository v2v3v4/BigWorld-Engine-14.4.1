
#ifndef ME_SHELL_HPP
#define ME_SHELL_HPP

#include <iostream>

#include "cstdmf/cstdmf_windows.hpp"

#include "tools/common/romp_harness.hpp"
#include "cstdmf/debug_message_callbacks.hpp"
#include "cstdmf/singleton_manager.hpp"

BW_BEGIN_NAMESPACE

class AssetClient;
class TimeOfDay;
class XConsole;
class Renderer;
class FontManager;
class TextureFeeds;
class LensEffectManager;

namespace Terrain
{
	class Manager;
}

namespace PostProcessing
{
	class Manager;
}

/*
 *	Interface to the BirWorld message handler
 */
struct MeShellDebugMessageCallback : public DebugMessageCallback
{
	MeShellDebugMessageCallback()
	{
	}
	~MeShellDebugMessageCallback()
	{
	}

	virtual bool handleMessage(
		DebugMessagePriority messagePriority, const char * pCategory,
		DebugMessageSource messageSource, const LogMetaData & metaData,
		const char * pFormat, va_list argPtr );
};



/**
 *	This class adds solution specific routines to what already exists in
 *	appmgr/app.cpp
 */
class MeShell
{
public:
						MeShell();
						~MeShell();

	static MeShell & instance() { SINGLETON_MANAGER_WRAPPER( MeShell ) MF_ASSERT(s_instance_); return *s_instance_; }

	static bool			initApp( HINSTANCE hInstance, HWND hWndApp, HWND hWndGraphics );	// so easy to pass the fn pointer around
	
	bool				init( HINSTANCE hInstance, HWND hWndApp, HWND hWndGraphics );
	void				fini();

	HINSTANCE &			hInstance() { return hInstance_; }
	HWND &				hWndApp() { return hWndApp_; }
	HWND &				hWndGraphics() { return hWndGraphics_; }

	RompHarness &		romp() { return *romp_; }

	// get time of day
	TimeOfDay* timeOfDay() { return romp_->timeOfDay(); }

	POINT				currentCursorPosition() const;

	Renderer* renderer() { return renderer_.get(); }

#ifdef ENABLE_ASSET_PIPE
	AssetClient* assetClient() const { return pAssetClient_.get(); }
#endif

private:
	friend std::ostream& operator<<( std::ostream&, const MeShell& );
	friend struct MeShellDebugMessageCallback;

						MeShell(const MeShell&);
						MeShell& operator=(const MeShell&);

	static bool			messageHandler( DebugMessagePriority messagePriority,
										const char * pFormat, va_list argPtr );

	bool				initGraphics();
	void				finiGraphics();

	bool				initScripts();
	void				finiScripts();

	bool				initConsoles();
	bool				initErrorHandling();
	bool				initRomp();
	bool				initCamera();

	bool				initSound();

	bool				initTiming();
	bool				inited();

	MeShellDebugMessageCallback debugMessageCallback_;
	
	static MeShell *		s_instance_;		// the instance of this class (there should be only one!)

	// windows variables
	HINSTANCE					hInstance_;			// the current instance of the application
	HWND						hWndApp_;			// application window
	HWND						hWndGraphics_;		// 3D window

	std::auto_ptr<Renderer> renderer_;

	RompHarness *			romp_;
	bool					inited_;

	std::auto_ptr< AssetClient > pAssetClient_;
	typedef std::auto_ptr< FontManager > FontManagerPtr;
	FontManagerPtr pFontManager_;
	
	typedef std::auto_ptr< TextureFeeds > TextureFeedsPtr;
	TextureFeedsPtr pTextureFeeds_;

	typedef std::auto_ptr< Terrain::Manager > TerrainManagerPtr;
	TerrainManagerPtr pTerrainManager_;

	typedef std::auto_ptr< PostProcessing::Manager > PostProcessingManagerPtr;
	PostProcessingManagerPtr pPostProcessingManager_;

	typedef std::auto_ptr< LensEffectManager > LensEffectManagerPtr;
	LensEffectManagerPtr pLensEffectManager_;
};

BW_END_NAMESPACE

//#ifdef CODE_INLINE
//#include "initialisation.ipp"
//#endif

#endif // ME_SHELL_HPP
