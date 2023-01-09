#ifndef DEVICE_APP_HPP
#define DEVICE_APP_HPP

#include "pathed_filename.hpp"
#include "cstdmf/main_loop_task.hpp"


BW_BEGIN_NAMESPACE

class ConnectionControl;
class ProgressDisplay;
class GUIProgressDisplay;
class InputHandler;
class ProgressTask;
class Renderer;
class FontManager;
class TextureFeeds;
class LensEffectManager;
class AssetClient;

#if SCALEFORM_SUPPORT
namespace ScaleformBW
{
	class Manager;
}
#endif

namespace PostProcessing
{
	class Manager;
}

/**
 *	Device task
 */
class DeviceApp : public MainLoopTask
{
public:
	DeviceApp();
	~DeviceApp();

	virtual bool init();
	virtual void fini();
	virtual void tick( float dGameTime, float dRenderTime );
	virtual void inactiveTick( float dGameTime, float dRenderTime );
	virtual void draw();

	void deleteGUI();
	bool savePreferences();

	void assetClient( AssetClient* pAssetClient );

private:
	float								dRenderTime_;
	Vector3								bgColour_;
	bool								soundEnabled_;

	PathedFilename						preferencesFilename_;
	typedef std::auto_ptr< FontManager > FontManagerPtr;
	FontManagerPtr pFontManager_;
	
	typedef std::auto_ptr< TextureFeeds > TextureFeedsPtr;
	TextureFeedsPtr pTextureFeeds_;

	typedef std::auto_ptr< PostProcessing::Manager > PostProcessingManagerPtr;
	PostProcessingManagerPtr pPostProcessingManager_;

	typedef std::auto_ptr< LensEffectManager > LensEffectManagerPtr;
	LensEffectManagerPtr pLensEffectManager_;

public:
	static DeviceApp					instance;
	static HINSTANCE					s_hInstance_;
	static HWND							s_hWnd_;

	static ProgressDisplay *			s_pProgress_;
	static GUIProgressDisplay *			s_pGUIProgress_;
	static ProgressTask *				s_pStartupProgTask_;

	std::auto_ptr<Renderer>				renderer_;


	InputHandler* pInputHandler_;
	AssetClient* pAssetClient_;

	ConnectionControl * pConnectionControl_;

#if SCALEFORM_SUPPORT	
	typedef std::auto_ptr< ScaleformBW::Manager > ScaleFormBWManagerPtr;
	ScaleFormBWManagerPtr pScaleFormBWManager_;
#endif
};

BW_END_NAMESPACE

#endif // DEVICE_APP_HPP
// device_app.hpp
