#ifndef APP_HPP
#define APP_HPP

#include "cstdmf/cstdmf_windows.hpp"

#include "cstdmf/bw_map.hpp"

#include "pyscript/script.hpp"

#include "input/input.hpp"

#include "resmgr/auto_config.hpp"


BW_BEGIN_NAMESPACE

// Forward declarations
class ProgressDisplay;
class ProgressTask;
class EnviroMinder;

class PhotonOccluder;
class EntityPhotonOccluder;

class BaseCamera;
class ProjectionAccess;

class FrameWriter;
class ServerConnection;
class InputCursor;
class InputDevices;
class VersionInfo;
class CameraApp;

class AssetClient;

#if ENABLE_DPRINTF
struct MessageTimePrefix;
#endif // ENABLE_DPRINTF

typedef SmartPointer<InputCursor> InputCursorPtr;

extern float CLODPower;

extern const char * configString;


uint32 memUsed();

uint32 memoryAccountedFor();
int32 memoryUnclaimed();


extern AutoConfigString s_engineConfigXML;
extern AutoConfigString s_scriptsConfigXML;
extern AutoConfigString loadingScreenName;
extern AutoConfigString loadingScreenGUI;
extern AutoConfigString s_graphicsSettingsXML;
extern AutoConfigString s_floraXML;
extern AutoConfigString s_blackTexture;
extern int  s_framesCounter;
extern bool s_usingDeprecatedBigWorldXML;

bool displayLoadingScreen();
void freeLoadingScreen();
void loadingText( const BW::string & s );

extern DataSectionPtr s_scriptsPreferences;
extern BW::string s_configFileName;

extern const float PROGRESS_TOTAL;
extern const float APP_PROGRESS_STEP;

extern void initNetwork();

extern void setupTextureFeedPropertyProcessors();

extern bool gWorldDrawEnabled;


void criticalInitError( const char * format, ... );


extern bool g_drawTerrain;

namespace Moo
{
	class DrawContext;
}

/**
 *	This singleton class is the highest level of the application
 */
class App : public InputHandler,
	public DebugMessageCallback
{
public:
	enum DrawContextType
	{
		COLOUR_DRAW_CONTEXT,
		SHADOW_DRAW_CONTEXT,
		NUM_DRAW_CONTEXTS
	};
	/**
	 * TODO: to be documented.
	 */
	class InitError : public std::runtime_error 
	{
	public:
		InitError( const char* what ) :
			std::runtime_error( what )
		{}
	};

	App( const BW::string & configFilename, 
		 const char * compileTime	= NULL );
	~App();

	bool initLoggers();
	// Init method to be called first
	bool init( HINSTANCE hInstance, HWND hWnd );
	void fini();

	// Main loop method called by our owner
	bool updateFrame(bool active);

	// Make us quit
	void quit( bool restart = false );

	// Overrides from InputHandler
	virtual bool handleKeyEvent( const KeyEvent & event );
	virtual bool handleInputLangChangeEvent();
	virtual bool handleIMEEvent( const IMEEvent & event );
	virtual bool handleMouseEvent( const MouseEvent & event );
	virtual bool handleAxisEvent( const AxisEvent & event );

	//Overrides from DebugMessageCallback
	virtual bool handleMessage(
		DebugMessagePriority messagePriority, const char * pCategory,
		DebugMessageSource messageSource, const LogMetaData & metaData,
		const char * pFormat, va_list argPtr );

	void memoryCriticalCallback();

	void queueWarningMessage( const BW::string& msg );
	void clientChatMsg( const BW::string & msg );

	InputCursorPtr activeCursor();
	void activeCursor( InputCursor * handler );

	bool savePreferences();

	// Windows messages
	void resizeWindow();
	void resizeWindow(int width, int height);
	const POINT windowSize() const;

	void moveWindow( int16 x, int16 y );
	static void handleSetFocus( bool focusState );
	void setWindowTitleNote( int pos, const BW::string & note );

	// Time
	double getRenderTimeNow() const;
	double getRenderTimeFrameStart() const;
	double getGameTimeFrameStart() const;
	float getFrameGameDelta() const;
	void skipGameTimeForward( double dGameTime );


	void checkPython();
	HWND hWnd()	{ return hWnd_; }

	// Camera Methods
	SmartPointer<BaseCamera> camera();
	void camera( SmartPointer<BaseCamera> pCamera );
	bool isQuiting();
	void setQuiting( bool quiting );


	ProjectionAccess * projAccess();

	Moo::DrawContext&	drawContext( DrawContextType drawContextType );

	/// Singleton instance accessors
	static App & instance()		{ return *pInstance_; }
	static App * pInstance()	{ return pInstance_; }

	PY_MODULE_STATIC_METHOD_DECLARE( py_setCursor )

	// Compile time strings
	const char * compileTime() const;

	// Console toggle methods
	BW::string activeConsole() const;
	void activeConsole(BW::string v);

private:
	// Update methods
	void updateScene( float dTime );
	void updateCameras( float dTime );

	// Render methods
	void renderFrame();
	void drawWorld();
	void drawScene();

	// Event handlers
	void handleInputPlayback();
	bool handleKeyDown( const KeyEvent & event );
	bool handleDebugKeyDown( const KeyEvent & event );

	// Checks the debug key state for the given key.
	bool checkDebugKeysState( KeyCode::Key key );

	// Determines if the given key is a debug key
	bool isDebugKey( KeyCode::Key key );


	enum eEventDestination
	{
		EVENT_SINK_NONE = 0,
		EVENT_SINK_CONSOLE,
		EVENT_SINK_SCRIPT,
		EVENT_SINK_APP,
		EVENT_SINK_DEBUG,
		EVENT_SINK_PERSONALITY
	};

	eEventDestination	keyRouting_[KeyCode::NUM_KEYS];

	// Helper methods
	void calculateFrameTime();

	// window handle
	HWND hWnd_;

	// Frame statistics
	uint64 appStartRenderTime_;
	uint64 frameStartRenderTime_;
	float dRenderTime_;
	float dGameTime_;
	double accumulatedTimeSkip_;

	uint64 lastFrameEndTime_;
	uint64 minFrameTime_;
	float minimumFrameRate_;

	static App *		pInstance_;

#if ENABLE_DPRINTF
	MessageTimePrefix * pMessageTimePrefix_;
#endif // ENABLE_DPRINTF

	typedef BW::map<int,BW::string> TitleNotes;
	TitleNotes			titleNotes_;

	bool			debugKeyEnable_;

	InputCursorPtr	activeCursor_;
	POINT           windowSize_;

	BW::string		compileTime_;

	friend class DeviceApp;
	typedef App This;

	typedef BW::vector<KeyCode::KeyArray> DebugKeyArray;
	DebugKeyArray	debugKeys_;

	KeyEvent keyUpChar_;

	int32 handleKeyEventDepth_;

	int sleepTime_;

	class HandleKeyEventHolder
	{
	public:
		HandleKeyEventHolder(App& app) : app_(app) { ++app_.handleKeyEventDepth_; }
		~HandleKeyEventHolder() { --app_.handleKeyEventDepth_; }

	private:
		App& app_;
	};

	/// Use in asserts in init() and fini() to check for multiple calls.
	enum InitState
	{
		STATE_UNINITIALISED = 0,
		STATE_INITIALISED,
		STATE_FINALISED
	};
	InitState currentState_;

	// Init some singletons
	InputDevices inputDevices_;
	typedef std::auto_ptr<CameraApp> CameraAppPtr;
	CameraAppPtr pCameraApp_;

	bool quiting_;

#if ENABLE_ASSET_PIPE
	typedef std::auto_ptr< AssetClient > AssetClientPtr;
	AssetClientPtr pAssetClient_;
#else
	AssetClient* pAssetClient_;
#endif
	Moo::DrawContext*	drawContexts_[NUM_DRAW_CONTEXTS];
};

bool isCameraOutside();
bool isPlayerOutside();

#ifdef CODE_INLINE
	#include "app.ipp"
#endif

BW_END_NAMESPACE

#endif // APP_HPP
