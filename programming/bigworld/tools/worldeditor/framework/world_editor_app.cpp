#include "pch.hpp"
#include "worldeditor/framework/world_editor_app.hpp"
#include "worldeditor/framework/world_editor_doc.hpp"
#include "worldeditor/framework/world_editor_view.hpp"
#include "worldeditor/framework/mainframe.hpp"
#include "worldeditor/framework/initialisation.hpp"
#include "worldeditor/scripting/we_python_adapter.hpp"
#include "worldeditor/terrain/texture_mask_cache.hpp"
#include "worldeditor/world/vlo_manager.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "worldeditor/world/items/editor_chunk_particle_system.hpp"
#include "worldeditor/world/items/editor_chunk_model.hpp"
#include "worldeditor/gui/dialogs/wait_dialog.hpp"
#include "worldeditor/gui/pages/panel_manager.hpp"
#include "appmgr/app.hpp"
#include "appmgr/options.hpp"
#include "appmgr/commentary.hpp"
#include "appmgr/module_manager.hpp"
#include "common/compile_time.hpp"
#include "common/tools_common.hpp"
#include "common/directory_check.hpp"
#include "common/cooperative_moo.hpp"
#include "common/command_line.hpp"
#include "chunk/chunk_manager.hpp"
#include "chunk/chunk_loader.hpp"
#include "chunk/chunk_space.hpp"
#include "gizmo/gizmo_manager.hpp"
#include "gizmo/tool_manager.hpp"
#include "guimanager/gui_manager.hpp"
#include "guimanager/gui_menu.hpp"
#include "guimanager/gui_toolbar.hpp"
#include "guimanager/gui_functor.hpp"
#include "guimanager/gui_functor_option.hpp"
#include "common/page_messages.hpp"
#include "resmgr/string_provider.hpp"
#include "resmgr/auto_config.hpp"
#include "resmgr/bwresource.hpp"
#include "moo/texture_streaming_manager.hpp"
#include <afxdhtml.h>
#include "cstdmf/allocator.hpp"
#include "cstdmf/bwversion.hpp" 
#include "cstdmf/debug_exception_filter.hpp"
#include "cstdmf/restart.hpp"
#include "cstdmf/message_box.hpp"
#include "memhook/memhook.hpp"
#include "pyscript/automation.hpp"

#include "tools/editor_shared/mfc/menu_helper.hpp"

DECLARE_DEBUG_COMPONENT2( "WorldEditor2", 0 )

BW_BEGIN_NAMESPACE

DECLARE_WATCHER_DATA( NULL )
DECLARE_COPY_STACK_INFO( false )

static AutoConfigString s_LanguageFile( "system/language" );

WorldEditorApp theApp; // The one and only WorldEditorApp object


//-----------------------------------------------------------------------------
// Section: CAboutDlg
//-----------------------------------------------------------------------------

// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
	CBitmap mBackground;
	CFont mBuildFont;
	CFont mCopyRightFont;
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	BW_GUARD;

	mBackground.LoadBitmap( IDB_ABOUTBOX );
	mBuildFont.CreatePointFont( 90, L"Arial", NULL );
	mCopyRightFont.CreatePointFont( 70, L"Arial", NULL );
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	BW_GUARD;

	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_RBUTTONDOWN()
END_MESSAGE_MAP()

BOOL CAboutDlg::OnInitDialog()
{
	BW_GUARD;

	CDialog::OnInitDialog();

	BITMAP bitmap;
	mBackground.GetBitmap( &bitmap );
	RECT rect = { 0, 0, bitmap.bmWidth, bitmap.bmHeight };
	AdjustWindowRect( &rect, GetWindowLong( m_hWnd, GWL_STYLE ), FALSE );
	MoveWindow( &rect, FALSE );
	CenterWindow();

	SetCapture();

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CAboutDlg::OnPaint()
{
	BW_GUARD;

	CPaintDC dc(this); // device context for painting
	CDC memDC;
	memDC.CreateCompatibleDC( &dc);
	CBitmap* saveBmp = memDC.SelectObject( &mBackground );

	RECT client;
	GetClientRect( &client );

	dc.BitBlt( 0, 0, client.right, client.bottom, &memDC, 0, 0, SRCCOPY );

	memDC.SelectObject( &saveBmp );

	BW::wstring space = L" ";
	CString builtOn = Localise(L"WORLDEDITOR/GUI/BIGBANG/VERSION_BUILT", BWVersion::versionString().c_str(),
		ToolsCommon::isEval() ? space + Localise(L"WORLDEDITOR/GUI/BIGBANG/EVAL" ) : L"",
#ifdef _DEBUG
		space + Localise(L"WORLDEDITOR/GUI/BIGBANG/DEBUG" ),
#else
		"",
#endif
		aboutCompileTimeString );

	dc.SetBkMode( TRANSPARENT );
	dc.SetTextColor( 0x00808080 );
	CFont* saveFont = dc.SelectObject( &mBuildFont );
	dc.ExtTextOut( 72, 290, 0, NULL, builtOn, NULL );

	dc.SelectObject( &mCopyRightFont );
	dc.ExtTextOut( 72, 366, 0, NULL, TEXT( BW_COPYRIGHT_NOTICE ), NULL );
	dc.SelectObject( &saveFont );
}

void CAboutDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
	BW_GUARD;

	CDialog::OnLButtonDown(nFlags, point);
	OnOK();
}

void CAboutDlg::OnRButtonDown(UINT nFlags, CPoint point)
{
	BW_GUARD;

	CDialog::OnRButtonDown(nFlags, point);
	OnOK();
}


//-----------------------------------------------------------------------------
// Section: ShortcutsDlg
//-----------------------------------------------------------------------------

class ShortcutsDlg: public CDHtmlDialog
{
public:
	ShortcutsDlg( int ID ): CDHtmlDialog( ID ) {}

	BOOL ShortcutsDlg::OnInitDialog() 
	{
		BW_GUARD;

		BW::string shortcutsHtml = Options::getOptionString(
			"help/shortcutsHtml",
			"resources/html/shortcuts.html");

		shortcutsHtml = localiseFileName( shortcutsHtml );

		BW::string shortcutsUrl = BWResource::resolveFilename( shortcutsHtml );
		CDHtmlDialog::OnInitDialog();
		Navigate( bw_utf8tow( shortcutsUrl ).c_str() );
		return TRUE; 
	}

    /*virtual*/ void OnCancel()
    {
		BW_GUARD;

        DestroyWindow();
		bw_safe_delete(s_instance);
    }

    static ShortcutsDlg *instance()
    {
        if (s_instance == NULL)
        {
			BW_GUARD;

            s_instance = new ShortcutsDlg(IDD_HTMLDIALOG);
            s_instance->Create(IDD_HTMLDIALOG);
        }
        return s_instance;
    }

    static void cleanup()
    {
		BW_GUARD;

        if (s_instance != NULL)
            s_instance->OnCancel();
    }

private:
    static ShortcutsDlg    *s_instance;
};

ShortcutsDlg *ShortcutsDlg::s_instance = NULL;


//-----------------------------------------------------------------------------
// Section: AppFunctor
//-----------------------------------------------------------------------------

class AppFunctor
	: public ReferenceCount
	, public GUI::ActionMaker<AppFunctor>
	, public GUI::ActionMaker<AppFunctor, 1>
{
public:
	AppFunctor()
		: GUI::ActionMaker<AppFunctor>( "doAboutWorldEditor", &AppFunctor::doAboutWorldEditor )
		, GUI::ActionMaker<AppFunctor, 1>( "doShortcuts", &AppFunctor::OnShortcuts )
	{}

	bool doAboutWorldEditor( GUI::ItemPtr )
	{
		BW_GUARD;

		CAboutDlg().DoModal();
		return true;
	}

	bool OnShortcuts( GUI::ItemPtr )
	{
		BW_GUARD;

        ShortcutsDlg::instance()->ShowWindow(SW_SHOW);
		return true;
	}
};


//-----------------------------------------------------------------------------
// Section: WorldEditorApp
//-----------------------------------------------------------------------------

BEGIN_MESSAGE_MAP(WorldEditorApp, CWinApp)
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
END_MESSAGE_MAP()


// WorldEditorApp construction
WorldEditorApp * WorldEditorApp::s_instance_ = NULL;


WorldEditorApp::WorldEditorApp(): 
	pPythonAdapter_(NULL),
	s_mfApp( NULL )
{
	BW_GUARD;

	ASSERT(s_instance_ == NULL);
	s_instance_ = this;

	// NOTE: Can't set the Low Fragmentation Heap if inside the debugger
	// unless _NO_DEBUG_HEAP is set to "1" in the environment
	TCHAR noDebugHeap[2];
	DWORD numChars = GetEnvironmentVariable( TEXT("_NO_DEBUG_HEAP"),
		noDebugHeap, 2 );
	if (!IsDebuggerPresent() ||
		(numChars == 1 && !StrCmpN( TEXT("1"), noDebugHeap, 2 )))
	{
		// Enable low fragmentation heap
		ULONG  heapFragValue = 2;

		HANDLE heaps[ 1024 ];
		DWORD nheaps = GetProcessHeaps( 1024, heaps );
		for (DWORD i = 0; i < nheaps; i++)
		{
			if (HeapSetInformation( heaps[i], HeapCompatibilityInformation,
									&heapFragValue, sizeof( heapFragValue ) ))
			{
				INFO_MSG( "Success enabling the Low Fragmentation Heap on heap %08x.\n", heaps[i] );
			}
			else
			{
				INFO_MSG( "Failed enabling the Low Fragmentation Heap on heap %08x.\n", heaps[i] );
			}
		}
	}
}


WorldEditorApp::~WorldEditorApp()
{
	BW_GUARD;

	ASSERT(s_instance_);
	s_instance_ = NULL;

	if( updateMailSlot_ != INVALID_HANDLE_VALUE )
		CloseHandle( updateMailSlot_ );
	free( (void *) m_pszAppName);
	/* Restore the old pointer so ~CWINAPP can free
	   later */
	m_pszAppName = oldAppName_;
}


BOOL WorldEditorApp::InitInstance()
{
	BW::Allocator::setSystemStage( BW::Allocator::SS_MAIN );

	BOOL result = CallWithExceptionFilter( this, &WorldEditorApp::InternalInitInstance );

	if (!result)
	{
		if (pWorldManager_.get() != NULL &&
			pWorldManager_->wasExitRequested())
		{
			//Exit requested, just exit gracefully.
			return false;
		}
		MessageBox(
			NULL,
			L"WorldEditor failed to initailise itself correctly, please check the debug log for detailed information.",
			L"WorldEditor",
			MB_OK );
	}

	return result;
}

int WorldEditorApp::ExitInstance()
{
	BOOL result = CallWithExceptionFilter( this, &WorldEditorApp::InternalExitInstance );
	BW::Allocator::setSystemStage( BW::Allocator::SS_POST_MAIN );
	return result;
}

int WorldEditorApp::Run()
{
	return CallWithExceptionFilter( this, &WorldEditorApp::InternalRun );
}

BOOL WorldEditorApp::InternalInitInstance()
{
	BW_GUARD;

	Name::init();

	waitForRestarting();

	// InitCommonControls() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	InitCommonControls();

#if defined(_MFC_VER) && (_MFC_VER <= 0x0A00)
	// This flag is used to turn off 0xC015000F error in MFC which will cause WE crashing in Vista
	afxAmbientActCtx = FALSE;
#endif

	AfxInitRichEdit2();

	CWinApp::InitInstance();

	// Initialise OLE libraries
	if (!AfxOleInit())
	{
		AfxMessageBox(IDP_OLE_INIT_FAILED);
		return FALSE;
	}
	AfxEnableControlContainer();

	// Standard initialisation
	// If you are not using these features and wish to reduce the size
	// of your final executable, you should remove from the following
	// the specific initialisation routines you do not need

	SetRegistryKey(_T("BigWorld-WorldEditor"));
	LoadStdProfileSettings(4);  // Load standard INI file options (including MRU)

#ifdef USE_MEMHOOK
	// Need to disable custom allocation so MFC doesn't get confused on exit
	// trying to delete the doc template created classes below.
	BW::Memhook::AllocFuncs sysAllocFuncs = { ::malloc, ::free, ::_aligned_malloc, ::_aligned_free, ::realloc, ::_msize };
	BW::Memhook::AllocFuncs bwAllocFuncs= BW::Memhook::allocFuncs();
	BW::Memhook::allocFuncs( sysAllocFuncs );
#endif

	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views
	CSingleDocTemplate* pDocTemplate;
	pDocTemplate = new CSingleDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(WorldEditorDoc),
		RUNTIME_CLASS(MainFrame),       // main SDI frame window
		RUNTIME_CLASS(WorldEditorView));
	AddDocTemplate(pDocTemplate);

#ifdef USE_MEMHOOK
	// Re-enabling custom allocation
	BW::Memhook::allocFuncs( bwAllocFuncs );
#endif

	// TODO: It would probably be better if BWResource was just a member of
	// WorldEditorApp.
	// Creating the BWResource instance. The internal instance pointer will
	// hold the object until it is destroyed in ExitInstance.
	BWResource* bwResource = new BWResource();
	MF_ASSERT( BWResource::pInstance() );

	// Parse command line for standard shell commands, DDE, file open
	MFCommandLineInfo cmdInfo;

	// initialise the MF file services (read in the command line arguments too)
	if (!parseCommandLineMF())
		return FALSE;
	
	if( !s_LanguageFile.value().empty() )
		StringProvider::instance().load( BWResource::openSection( s_LanguageFile ) );
	BW::vector<DataSectionPtr> languages;
	Options::pRoot()->openSections( "language", languages );
	if (!languages.empty())
	{
		for( BW::vector<DataSectionPtr>::iterator iter = languages.begin(); iter != languages.end(); ++iter )
			if( !(*iter)->asString().empty() )
				StringProvider::instance().load( BWResource::openSection( (*iter)->asString() ) );
	}
	else
	{
		// Force English:
		StringProvider::instance().load( BWResource::openSection("helpers/languages/worldeditor_gui_en.xml"));
		StringProvider::instance().load( BWResource::openSection("helpers/languages/worldeditor_rc_en.xml" ));
		StringProvider::instance().load( BWResource::openSection("helpers/languages/files_en.xml"          ));
	}

	BW::wstring currentLanguage = bw_utf8tow( Options::getOptionString( "currentLanguage", "" ) );
	BW::wstring currentCountry  = bw_utf8tow( Options::getOptionString( "currentCountry", "" ) );
	if ( currentLanguage != L"" )
		StringProvider::instance().setLanguages( currentLanguage, currentCountry );
	else
		StringProvider::instance().setLanguage();

	WindowTextNotifier::instance();

	ParseCommandLine(cmdInfo);
	/* Save this pointer so we can restore it when the world editor exits
	 * This avoids a bug when using our custom memory allocators (such as memtrackers's)
	 * That do exotic things when allocating memory. It is possible that our
	 * custom free operations will not get called for the new memory */
	this->oldAppName_ = WorldEditorApp::instance().m_pszAppName;	
	WorldEditorApp::instance().m_pszAppName = _wcsdup( Localise(L"WORLDEDITOR/APPLICATION_NAME" ) );

	GUI::Manager::init();

	pAppFunctor_ = new AppFunctor();

	DataSectionPtr guiRoot = BWResource::openSection( "resources/data/gui.xml" );
	if( guiRoot )
		for( int i = 0; i < guiRoot->countChildren(); ++i )
			GUI::Manager::instance().add( new GUI::Item( guiRoot->openChild( i ) ) );

	// Dispatch commands specified on the command line.  Will return FALSE if
	// app was launched with /RegServer, /Register, /Unregserver or /Unregister.
	// This also creates all the GUI windows
	if (!ProcessShellCommand(cmdInfo))
	{
		return FALSE;
	}

	// The one and only window has been initialised, so show and update it
	m_pMainWnd->ShowWindow(SW_SHOWMAXIMIZED);
	m_pMainWnd->UpdateWindow();

	// call DragAcceptFiles only if there's a suffix
	//  In an SDI app, this should occur after ProcessShellCommand

	// initialise the MF App components
	ASSERT( !s_mfApp );
	class WEApp : public App
	{
		virtual void presenting( bool isPresenting )
		{
			static DogWatch s_watchThreadControl( "chunkProcessorThreads" );
			ScopedDogWatch scopedWatchThreadControl( s_watchThreadControl );
			if (!WorldManager::instance().uiBlocked())
			{
				WorldManager::allowChunkProcessorThreadsToRun( isPresenting );
			}
		}
	};
	s_mfApp = new WEApp;

	HINSTANCE hInst = AfxGetInstanceHandle();
	MainFrame * mainFrame = (MainFrame *)(m_pMainWnd);

	// Display an "Initialising WorldEditor..." message
	WaitDlg::show( LocaliseUTF8(L"WORLDEDITOR/INITIALISING_WORLDEDITOR_MSG") );

	pWorldManager_ = WorldManagerPtr( new WorldManager );

	if (!s_mfApp->init( hInst, m_pMainWnd->m_hWnd,
		mainFrame->GetActiveView()->m_hWnd, 
		NULL,
		Initialisation::initApp ))
	{
		return FALSE;
	}

	// All tools should have had texture streaming disabled, so we'll need
	// to explicitly enable it for the world editor
	Moo::TextureStreamingManager * texStreamingManager = Moo::TextureManager::instance()->streamingManager();
	if (!texStreamingManager->isCommandLineDisabled())
	{
		texStreamingManager->streamingMode( Moo::TextureStreamingManager::MODE_CONSERVATIVE_NO_BUDGET );
	}

	CooperativeMoo::init();

	// need to load the adapter before the load thread begins, but after the modules
	pPythonAdapter_ = new WEPythonAdapter();

	// No need to store a pointer because it's already stored inside the
	// Singleton base class (in the pInstance)
	new TextureMaskCache();

	menuHelper_.reset( new GUI::MenuHelper( mainFrame->GetSafeHwnd() ) );

	// toolbar / menu initialisation
	// IMPORTANT: The order of this call is important, leave it here!
	GUI::Manager::instance().add( new GUI::Menu( "MainMenu", menuHelper_.get() ) );
	AfxGetMainWnd()->DrawMenuBar();

	// create Toolbars through the BaseMainFrame createToolbars method
	// IMPORTANT: The order of this call is important, leave it here!
	mainFrame->createToolbars( "AppToolbars" );

	// GUITABS Tearoff tabs system init and setup
	// IMPORTANT: The order of this call is important, leave it here!
	PanelManager::init( mainFrame, mainFrame->GetActiveView() );

	updateMailSlot_ = CreateMailslot( L"\\\\.\\mailslot\\WorldEditorUpdate", 0, 0, NULL );

	if (!Options::optionsFileExisted())
	{
		Options::setOptionInt("messages/errorMsgs", 1); // turn on showing of error messages
		ERROR_MSG("options.xml is missing\n");
	}

	// Destroy the "Initialising WorldEditor..." progress bar
	WaitDlg::hide();

	Automation::parseCommandLine( m_lpCmdLine );

	return TRUE;
}


BOOL WorldEditorApp::parseCommandLineMF()
{
	BW_GUARD;

	// parse command line
	const int MAX_ARGS = 20;
	const char sepsArgs[] = " \t";
	const char sepsFile[] = "\"";
	char * argv[ MAX_ARGS ];
	int argc = 0;

	// parse the arguments
	char cmdLine[ 32767 ];
	bw_wtoutf8( m_lpCmdLine, wcslen( m_lpCmdLine ), cmdLine, 32767 );
	argv[ argc ] = strtok( cmdLine, sepsArgs );

	while (argv[ argc ] != NULL)
	{
		if( ( strcmp( argv[ argc ], "-UID" ) == 0 ) ||
			( strcmp( argv[ argc ], "-uid" ) == 0 ) )
		{
			++argc;
			argv[ argc ] = strtok( NULL, sepsArgs );
			if ( argv[ argc ] == NULL )
			{
				ERROR_MSG( "WorldEditor::parseCommandLineMF: No user ID given\n" );
				return FALSE;
			}
		}
		else if (( strcmp( argv[ argc ], "-r" ) == 0 ) ||
				( strcmp( argv[ argc ], "--res" ) == 0 )||
				( strcmp( argv[ argc ], "--options" ) == 0 ))
		{
			++argc;
			char buff[MAX_PATH];
			char* str = strtok( NULL, sepsArgs );
			if(str != NULL && *str == '\"') //string is in quotes
			{
				if(str[strlen(str)-1] == '\"') //quoted string has no spaces
				{
					str[strlen(str)-1] = '\0'; //remove last quote
					strcpy(buff, str+1); //copy without first quote
				}
				else //quoted string contains spaces
				{
					strcpy(buff, str+1); //copy without first quote
					str = strtok( NULL, sepsFile );
					strcat(buff, " "); //add space back in
					strcat(buff, str);//append rest of string
				}
				argv[ argc ] = new char [ strlen(buff) ];
				strcpy( argv[ argc ], buff );
			}
			else if(str != NULL)
			{
				argv[ argc ] = str;
			}
			else
			{
				ERROR_MSG( "WorldEditor::parseCommandLineMF: No path given for \"%s\" argument.\n", argv[ argc-1 ] );
				return FALSE;
			}
		}
		
		if (++argc >= MAX_ARGS)
		{
			ERROR_MSG( "WorldEditor::parseCommandLineMF: Too many arguments!!\n" );
			return FALSE;
		}

		argv[ argc ] = strtok( NULL, sepsArgs );
	}

	return BWResource::init( argc, (const char **)argv ) && 
		Options::init( argc, argv, L"worldeditor.options"  );
}


// WorldEditorApp message handlers

BOOL WorldEditorApp::OnIdle(LONG lCount)
{
	BW_GUARD;

	// The following two lines need to be uncommented for toolbar docking to
	// work properly, and the application's toolbars have to inherit from
	// GUI::CGUIToolBar
	if ( CWinApp::OnIdle( lCount ) )
		return TRUE; // give priority to windows GUI as MS says it should be.

	// check to see the window is not active
	MainFrame * mainFrame = (MainFrame *)(m_pMainWnd);
	HWND fgWin = GetForegroundWindow();
	DWORD processID;
	GetWindowThreadProcessId( fgWin, &processID );

	if( ::IsWindow( fgWin ) && processID == GetCurrentProcessId() &&
		fgWin != mainFrame->m_hWnd && GetParent( fgWin ) != mainFrame->m_hWnd &&
		( GetWindowLong( fgWin, GWL_STYLE ) & WS_VISIBLE ) == 0 )
	{
		mainFrame->SetForegroundWindow();
		fgWin = GetForegroundWindow();
	}

	bool isWindowActive =
		fgWin == mainFrame->m_hWnd || GetParent( fgWin ) == mainFrame->m_hWnd;

	if (!CooperativeMoo::canUseMoo( this, isWindowActive ))
	{
		// If activate failed, because the app is minimised, there's not enough
		// videomem to restore, or the app is in the background and other apps
		// we need to cooperate with are running, we just try again later.
		s_mfApp->calculateFrameTime(); // Do this to effectively freeze time
	}
	else
	{
		// measure the update time
		uint64 beforeTime = timestamp();

		// do the real update!!
		s_mfApp->updateFrame();

		// update any gui
		mainFrame->frameUpdate();

		uint64 afterTime = timestamp();
		float lastUpdateMilliseconds = (float) (((int64)(afterTime - beforeTime)) / stampsPerSecondD()) * 1000.f;

		float desiredFrameRate_ = 60.f;
		const float desiredFrameTime = 1000.f / desiredFrameRate_;	// milliseconds

		if (desiredFrameTime > lastUpdateMilliseconds)
		{
			float compensation = desiredFrameTime - lastUpdateMilliseconds;
			Sleep((int)compensation);
		}

	}

	DWORD size, num;
	if( updateMailSlot_ != INVALID_HANDLE_VALUE &&
		GetMailslotInfo( updateMailSlot_, 0, &size, &num, 0 ) &&
		num )
	{
		char s[ 1024 * 128 ];// big enough for a file name???
		DWORD bytesRead;
		if( ReadFile( updateMailSlot_, s, size, &bytesRead, 0 ) )
		{
			ChunkManager::instance().switchToSyncMode( true );
			EditorChunkParticleSystem::reload( s );
			ChunkManager::instance().switchToSyncMode( false );
			// Ensure stale columns get recreated, otherwise a crash can occur.
			ChunkManager::instance().cameraSpace()->focus( Moo::rc().invView().applyToOrigin() );
		}
	}

	return TRUE;
}


int WorldEditorApp::InternalExitInstance()
{
	BW_GUARD;

	if ( s_mfApp )
	{
		ShortcutsDlg::cleanup();

		GizmoManager::instance().removeAllGizmo();
		while ( ToolManager::instance().tool() )
			ToolManager::instance().popTool();

		delete TextureMaskCache::pInstance();

		GeneralEditor::currentEditors( GeneralEditor::Editors() );

		PanelManager::fini();

		bw_safe_delete(pPythonAdapter_);

		s_mfApp->fini( Initialisation::finiApp );
		bw_safe_delete(s_mfApp);

		pAppFunctor_ = NULL;

		GUI::Manager::fini();

		WindowTextNotifier::fini();
		Options::fini();

		VLOManager::fini();

		ModuleManager::fini();

		pWorldManager_.reset();
		delete BWResource::pInstance();

		Name::fini();
	}

	BWResource::fini();
	DataSectionCensus::fini();

	return CWinApp::ExitInstance();
}

int WorldEditorApp::InternalRun()
{
	return CWinApp::Run();
}

BOOL WorldEditorApp::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
	BW_GUARD;

//	if (nID == WM_SYSKEYDOWN)
//		return TRUE;

	return CWinApp::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

BOOL WorldEditorApp::PreTranslateMessage(MSG* pMsg)
{
	BW_GUARD;

	return CWinApp::PreTranslateMessage(pMsg);
}


WEPythonAdapter * WorldEditorApp::pythonAdapter() const
{
	BW_GUARD;

	if (!pPythonAdapter_->hasScriptObject())
	{
		return NULL;
	}
	return pPythonAdapter_;
}


BW_END_NAMESPACE

