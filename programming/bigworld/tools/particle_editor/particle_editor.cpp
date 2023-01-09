/*~ module ParticleEditor
 *	@components{ particleeditor }
 *
 *	The ParticleEditor Module is a Python module that provides an interface to
 *	the various information about the particles(s) loaded into ParticleEditor.
 *	It also provides an interface to change and edit particle-specific information
 *	and the various ParticleEditor preferences.
 */
#include "pch.hpp"
#include "particle_editor.hpp"
#include "main_frame.hpp"
#include "pe_app.hpp"
#include "particle_editor_doc.hpp"
#include "particle_editor_view.hpp"
#include "about_dlg.hpp"
#include "common/compile_time.hpp"
#include "common/tools_camera.hpp"
#include "shell/pe_module.hpp"
#include "shell/pe_shell.hpp"
#include "gui/panel_manager.hpp"
#include "controls/dir_dialog.hpp"
#include "gui/action_selection.hpp"
#include "appmgr/app.hpp"
#include "appmgr/options.hpp"
#include "guimanager/gui_menu.hpp"
#include "guimanager/gui_toolbar.hpp"
#include "guimanager/gui_functor_python.hpp"
#include "moo/render_context.hpp"
#include "moo/reload.hpp"
#include "common/tools_common.hpp"
#include "common/format.hpp"
#include "common/cooperative_moo.hpp"
#include "common/command_line.hpp"
#include "common/page_messages.hpp"
#include "cstdmf/cstdmf_init.hpp"
#include "cstdmf/debug.hpp"
#include "cstdmf/debug_exception_filter.hpp"
#include "cstdmf/timestamp.hpp"
#include "cstdmf/bgtask_manager.hpp"
#include "chunk/chunk_manager.hpp"
#if UMBRA_ENABLE
#include "chunk/chunk_umbra.hpp"
#endif
#include "particle/meta_particle_system.hpp"
#include "particle/py_meta_particle_system.hpp"
#include "particle/py_particle_system.hpp"
#include "chunk/chunk_loader.hpp"
#include "chunk/user_data_object_link_data_type.hpp"
#include "gizmo/gizmo_manager.hpp"
#include "gizmo/tool_manager.hpp"
#include "ual/ual_dialog.hpp"
#include "pyscript/pyobject_plus.hpp"
#include "pyscript/automation.hpp"
#include "romp/lens_effect_manager.hpp"
#include "resmgr/string_provider.hpp"
#include "resmgr/auto_config.hpp"
#include "common/directory_check.hpp"
#include "common/string_utils.hpp"

#include "cstdmf/allocator.hpp"
#include "cstdmf/restart.hpp"
#include "memhook/memhook.hpp"

#include "cstdmf/cstdmf_windows.hpp"
#include <afxdhtml.h>

#include "tools/editor_shared/mfc/menu_helper.hpp"

DECLARE_DEBUG_COMPONENT2( "ParticleEditor", 0 )

BW_BEGIN_NAMESPACE

DECLARE_WATCHER_DATA( NULL )
DECLARE_COPY_STACK_INFO( false )
DEFINE_CREATE_EDITOR_PROPERTY_STUB

static AutoConfigString s_LanguageFile( "system/language" );

namespace
{
    // A helper class that allows display of a html dialog for the shortcut
    // information.
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
            s_instance = NULL;
        }

        static ShortcutsDlg *instance()
        {
			BW_GUARD;

            if (s_instance == NULL)
            {
                s_instance = new ShortcutsDlg(IDD_KEY_CUTS);
                s_instance->Create(IDD_KEY_CUTS);
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
}

// Make sure that these items are linked in:
extern int PyModel_token;
static int s_particleEditorchunkTokenSet = PyModel_token;

// Update via python script:
static const bool c_useScripting = false;

// The one and only ParticleEditorApp object:
ParticleEditorApp theApp;

BEGIN_MESSAGE_MAP(ParticleEditorApp, CWinApp)
END_MESSAGE_MAP()

/*static*/ ParticleEditorApp *ParticleEditorApp::s_instance = NULL;

ParticleEditorApp::ParticleEditorApp() :
	m_appShell(NULL),
	m_peApp(NULL),
	mfApp_( NULL ),
	m_desiredFrameRate(60.0f),
	m_state(PE_PLAYING)
{
	BW_GUARD;

    ASSERT(s_instance == NULL);
    s_instance = this;

	MsgHandler::instance();  // Init messages early
}

ParticleEditorApp::~ParticleEditorApp()
{
	BW_GUARD;

    ASSERT(s_instance != NULL);
	s_instance = NULL;
}

namespace
{
	wchar_t * s_lpCmdLine = NULL;

}

bool ParticleEditorApp::InitialiseMF( BW::string &openFile )
{
	BW_GUARD;

	// parse command line
	const int MAX_ARGS = 20;
	char * argv[ MAX_ARGS ];
	int argc = 0;

	char cmdline [32768];
	bw_wtoutf8( s_lpCmdLine, wcslen( s_lpCmdLine ), cmdline, ARRAY_SIZE( cmdline ) );
	char * str = cmdline;

	while (char * token = StringUtils::retrieveCmdTokenT( str ))
	{
		if (argc >= MAX_ARGS)
		{
			ERROR_MSG( "ParticleEditor::InitialiseMF: Too many arguments!!\n" );
			return FALSE;
		}
		if (argc && (!strcmp( argv[ argc-1 ], "-o" ) || !strcmp( argv[ argc-1 ], "-O" )))
		{
			openFile = BW::string( token );
		}
		argv[argc++] = token;
	}
	
	return BWResource::init( argc, (const char **)argv ) &&
		Options::init( argc, argv, L"particleeditor.options" );
}


BOOL ParticleEditorApp::InitInstance()
{
	BW::Allocator::setSystemStage( BW::Allocator::SS_MAIN );
	BOOL result = CallWithExceptionFilter( this, &ParticleEditorApp::InternalInitInstance );

	if (!result)
	{
		ERROR_MSG( "ParticleEditor failed to initialise itself correctly\n" );

		if (!CStdMf::checkUnattended())
		{
			MessageBox(
				NULL,
				L"ParticleEditor failed to initialise itself correctly.\n\
Please check the debug log for detailed information.",
				 L"ParticleEditor",
				 MB_OK );
		}
	}

	return result;
}


int ParticleEditorApp::ExitInstance()
{
	BOOL result = CallWithExceptionFilter( this, &ParticleEditorApp::InternalExitInstance );
	BW::Allocator::setSystemStage( BW::Allocator::SS_POST_MAIN );
	return result;
}


int ParticleEditorApp::Run()
{
	return CallWithExceptionFilter( this, &ParticleEditorApp::InternalRun );
}


// ParticleEditorApp initialisation
BOOL ParticleEditorApp::InternalInitInstance()
{
	BW_GUARD;

	Name::init();

	waitForRestarting();

    // InitCommonControls() is required on Windows XP if an application
    // manifest specifies use of ComCtl32.dll version 6 or later to enable
    // visual styles.  Otherwise, any window creation will fail.
    InitCommonControls();

    CWinApp::InitInstance();

	s_lpCmdLine = new wchar_t[ wcslen( m_lpCmdLine ) + 1];
	wcscpy( s_lpCmdLine, m_lpCmdLine );

    // Get the command line before ParseCommandLine has a go at it.
    BW::wstring commandLine = m_lpCmdLine;

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
    // Change the registry key under which our settings are stored
    // TODO: You should modify this string to be something appropriate
    // such as the name of your company or organization
    SetRegistryKey(_T("Local AppWizard-Generated Applications"));
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
        RUNTIME_CLASS(ParticleEditorDoc),
        RUNTIME_CLASS(MainFrame),       // main SDI frame window
        RUNTIME_CLASS(ParticleEditorView));
    AddDocTemplate(pDocTemplate);

#ifdef USE_MEMHOOK
	// Re-enabling custom allocation
	BW::Memhook::allocFuncs( bwAllocFuncs );
#endif

    // Initialise MF things
	BW::string openFile;
    if (!InitialiseMF(openFile))
        return 0;

	// initialise language provider
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
		StringProvider::instance().load( BWResource::openSection("helpers/languages/particleeditor_gui_en.xml"));
		StringProvider::instance().load( BWResource::openSection("helpers/languages/particleeditor_rc_en.xml" ));
		StringProvider::instance().load( BWResource::openSection("helpers/languages/files_en.xml"             ));
	}

	BW::string currentLanguage = Options::getOptionString( "currentLanguage", "" );
	BW::string currentCountry  = Options::getOptionString( "currentCountry", "" );
	if ( currentLanguage != "" )
		StringProvider::instance().setLanguages( bw_utf8tow( currentLanguage ), bw_utf8tow( currentCountry  ) );
	else
		StringProvider::instance().setLanguage();
	
	WindowTextNotifier::instance();
    
    // Parse command line for standard shell commands, DDE, file open
	MFCommandLineInfo cmdInfo;
    ParseCommandLine(cmdInfo);

    // Dispatch commands specified on the command line.  Will return FALSE if
    // app was launcAhed with /RegServer, /Register, /Unregserver or /Unregister.
    if (!ProcessShellCommand(cmdInfo))
    {
        ERROR_MSG("ParticleEditorApp::InitInstance - ProcessShellCommand failed\n");
        return FALSE;
    }

    // The one and only window has been initialised, so show and update it
    m_pMainWnd->ShowWindow(SW_SHOWMAXIMIZED);
    m_pMainWnd->UpdateWindow();
    // call DragAcceptFiles only if there's a suffix
    //  In an SDI app, this should occur after ProcessShellCommand

    MainFrame *mainFrame = (MainFrame *)(m_pMainWnd);
    CView     *mainView  = mainFrame->GetActiveView();

	mainFrame->UpdateTitle();

    // create the app and module
    ASSERT(!mfApp_);
    mfApp_ = new App;

    ASSERT(!m_appShell);
    m_appShell = new PeShell;    

    
    HINSTANCE hInst = AfxGetInstanceHandle();
    if 
    (
        !mfApp_->init
        ( 
            hInst, 
            mainFrame->m_hWnd, 
            mainView->m_hWnd, 
			NULL,
            PeShell::initApp 
        )
    )
    {
        ERROR_MSG( "ParticleEditorApp::InitInstance - init failed\n" );
        return FALSE;
    }

    m_peApp = new PeApp();

	CooperativeMoo::init();

	GUI::Manager::init();

	// Must do this after the panels are inited, they init GUI::Manager.
	GUI::Manager::instance().pythonFunctor().defaultModule( "MenuUIAdapter" );
	GUI::Manager::instance().optionFunctor().setOption(this);
	DataSectionPtr section = BWResource::openSection("resources/data/gui.xml");
	for (int i = 0; i < section->countChildren(); ++i)
		GUI::Manager::instance().add(new GUI::Item(section->openChild(i)));

	menuHelper_.reset( new GUI::MenuHelper( mainFrame->GetSafeHwnd() ) );
	// Setup the main menu:
	GUI::Manager::instance().add
    (
        new GUI::Menu("MainMenu", menuHelper_.get() ) 
    );

	updateLanguageList();

	AfxGetMainWnd()->DrawMenuBar();

	// Add the toolbar(s) using the BaseMainFrame helper method
	mainFrame->createToolbars( "AppToolbars" );

    // GUITABS Tearoff tabs system init and setup
	PanelManager::init(mainFrame, mainView, L"particleeditor.layout");		

    // kick off the chunk loading
	BgTaskManager::instance().init();
	BgTaskManager::instance().initWatchers( "ParticleEditor" );
	BgTaskManager::instance().startThreads( 1 );

	FileIOTaskManager::instance().init();
	FileIOTaskManager::instance().initWatchers( "FileIO" );
	FileIOTaskManager::instance().startThreads( 1 );
	
	/* Disable Umbra if it is enabled
	 * This fixes mouse lag issues caused by the present thread allowing the cpu to 
	 * get a few frames ahead of the GPU and then stalling for it to catch up
	 * Note identical code is set in the Model Editor InitInstance code, please
	 * update it if you update the code below.
	 */
	#if UMBRA_ENABLE
	if (Options::getOptionInt( "render/useUmbra", 1) == 1)
	{
		WARNING_MSG( "Umbra is enabled in Particle Editor, It will now be disabled\n" );
	}
	Options::setOptionInt( "render/useUmbra", 0 );
	ChunkManager::instance().umbra()->umbraEnabled( false );
	#endif

	// Load up particles into GUI
	// If this isn't here, you have to click the window and wait for it to load
	// all of the particles into the tree and it's very annoying
	if ( !c_useScripting )
	{
	    MainFrame::instance()->InitialiseMetaSystemRegister();
		MainFrame::instance()->UpdateGUI();
	}
    
	/**
	 * this loads the selected particle system passed in via the commandline.
	 * This NEEDS to be done after the nodes are loaded as it uses them to
	 * select the correct system.
	 */
	if ( !openFile.empty() )
	{
		openDirectory( BWResource::getFilePath( openFile ) );
		update();
		BW::StringRef psName = BWResource::getFilename( openFile );
		psName = BWResource::removeExtension( psName );
        bool ok =
			((MainFrame*)AfxGetMainWnd())->SelectParticleSystem( psName.to_string() );
        if (!ok)
        {
            AfxMessageBox(Localise(L"RCS_IDS_COULDNTOPENFILE", openFile));
        }
    }

	Automation::parseCommandLine( m_lpCmdLine );

    return TRUE;
}


int ParticleEditorApp::InternalExitInstance() 
{
	BW_GUARD;

	if ( mfApp_ )
	{
		ShortcutsDlg::cleanup();

		GizmoManager::instance().removeAllGizmo();
		while ( ToolManager::instance().tool() )
		{
			ToolManager::instance().popTool();
		}

		MsgHandler::fini();

		PanelManager::fini();

		//GeneralEditor::Editors none;
		//GeneralEditor::currentEditors(none);

		mfApp_->fini();
		delete mfApp_;
		mfApp_ = NULL;

		delete m_peApp; 
		m_peApp = NULL;

		m_appShell->fini();
		delete m_appShell; 
		m_appShell = NULL;

		GUI::Manager::fini();
	
		WindowTextNotifier::fini();	
		Options::fini();

		Name::fini();
	}

	BWResource::fini();
	DataSectionCensus::fini();

	delete [] s_lpCmdLine;
	return CWinApp::ExitInstance();
}


int ParticleEditorApp::InternalRun()
{
	return CWinApp::Run();
}


void ParticleEditorApp::updateLanguageList()
{
	BW_GUARD;

	GUI::ItemPtr languageList = GUI::Manager::instance()( "/MainMenu/Languages/LanguageList" );
	if( languageList )
	{
		while( languageList->num() )
			languageList->remove( 0 );
		for( unsigned int i = 0; i < StringProvider::instance().languageNum(); ++i )
		{
			LanguagePtr l = StringProvider::instance().getLanguage( i );
			BW::wstringstream wname, wdisplayName;
			wname << L"language" << i;
			wdisplayName << L'&' << l->getLanguageName();
			BW::string name, displayName;
			bw_wtoutf8( wname.str(), name );
			bw_wtoutf8( wdisplayName.str(), displayName );
			GUI::ItemPtr item = new GUI::Item( "CHILD", name, displayName,
				"",	"", "", "setLanguage", "updateLanguage", "" );
			item->set( "LanguageName", l->getIsoLangNameUTF8() );
			item->set( "CountryName", l->getIsoCountryNameUTF8() );
			languageList->add( item );
		}
	}
}

BOOL ParticleEditorApp::OnIdle(LONG lCount)
{
	BW_GUARD;

	// The following two lines need to be uncommented for toolbar docking to
	// work properly, and the application's toolbars have to inherit from
	// GUI::CGUIToolBar
	if ( CWinApp::OnIdle( lCount ) )
		return TRUE; // give priority to windows GUI as MS says it should be.

    HWND foreWindow = GetForegroundWindow();
    CWnd *mainFrame = MainFrame::instance();

	bool isWindowActive =
		foreWindow == mainFrame->m_hWnd || GetParent( foreWindow ) == mainFrame->m_hWnd;


	
	// Window is inactive - pause
	if ( !CooperativeMoo::canUseMoo( this, isWindowActive ) || !isWindowActive )
    {
		// If activate failed, because the app is minimised, there's not enough
		// videomem to restore, or the app is in the background and other apps
		// we need to cooperate with are running, we just try again later.
		mfApp_->calculateFrameTime(); // Do this to effectively freeze time
    }
	// Window is active - update
    else
    {
		// measure the update time
		uint64 beforeTime = timestamp();

		update();

		uint64 afterTime = timestamp();
		float lastUpdateMilliseconds = (float) (((int64)(afterTime - beforeTime)) / stampsPerSecondD()) * 1000.f;

        if (m_desiredFrameRate > 0)
        {
            // limit the frame rate
            const float desiredFrameTime = 1000.f/m_desiredFrameRate; // milliseconds
            float lastFrameTime = PeModule::instance().lastTimeStep();

            if (desiredFrameTime > lastUpdateMilliseconds)
            {
                float compensation = desiredFrameTime - lastUpdateMilliseconds;
                compensation = std::min(compensation, 2000.f);
                Sleep((int)compensation);
            }
            MainFrame::instance()->UpdateGUI();
        }
    }

    return TRUE;
}


void ParticleEditorApp::update()
{
	BW_GUARD;

    if (!c_useScripting)
    {
        mfApp_->updateFrame();        
    }
    else
    {
        PyObject *pScript     = PyImport_ImportModule("pe_shell");
        PyObject *pScriptDict = PyModule_GetDict(pScript);
        PyObject *pUpdate     = PyDict_GetItemString(pScriptDict, "update");
        if (pUpdate != NULL)
        {
            PyObject * pResult = PyObject_CallFunction(pUpdate, "");
            if (pResult != NULL)
            {
                Py_DECREF( pResult );
            }
            else
            {
                PyErr_Print();
            }
        }
        else
        {
            PyErr_Print();
        }
    }
}

/*~ function ParticleEditor.update
 *	@components{ particleeditor }
 *
 *	This function forces ParticleEditor to update each of its modules.
 */
PY_MODULE_STATIC_METHOD( ParticleEditorApp, update, ParticleEditor )

PyObject * ParticleEditorApp::py_update( PyObject * args )
{
	BW_GUARD;

	if (ParticleEditorApp::instance().mfApp())
    {
        // update all of the modules
        ParticleEditorApp::instance().mfApp()->updateFrame();
    }
    Py_RETURN_NONE;
}

/*~ function ParticleEditor.particleSystem
 *	@components{ particleeditor }
 *
 *	This function returns the currently selected MetaParticleSystem.
 *
 *	@return Returns the currently selected MetaParticleSystem object.
 */
PY_MODULE_STATIC_METHOD(ParticleEditorApp, particleSystem, ParticleEditor)

PyObject *ParticleEditorApp::py_particleSystem(PyObject * args)
{
	BW_GUARD;

    if (MainFrame::instance()->IsMetaParticleSystem())
	{
        return Script::getData( new PyMetaParticleSystem(MainFrame::instance()->GetMetaParticleSystem()));
	}

    if (MainFrame::instance()->IsCurrentParticleSystem())
	{
        return Script::getData( new PyParticleSystem(MainFrame::instance()->GetCurrentParticleSystem()));
	}

    Py_RETURN_NONE;
}

void ParticleEditorApp::OnDirectoryOpen()
{
	BW_GUARD;

    DirDialog dlg; 

    dlg.windowTitle_ = Localise(L"PARTICLEEDITOR/OPEN");
    dlg.promptText_  = Localise(L"PARTICLEEDITOR/CHOOSE_PS_DIR");
    dlg.fakeRootDirectory_ = dlg.basePath();

    // Set the start directory, check if exists:
    dlg.startDirectory_ = dlg.basePath();
	BW::wstring fullDirectory =
        BWResource::resolveFilenameW
        (
            MainFrame::instance()->ParticlesDirectory().GetString()
        );
    WIN32_FIND_DATA data;
    HANDLE hFind;
    wchar_t lastChar = fullDirectory.at(fullDirectory.length()-1);
    if ( (lastChar != L'/') && (lastChar != L'\\') )
        fullDirectory += L"/";
    
    hFind = FindFirstFile((fullDirectory + L"*").c_str(), &data);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        dlg.startDirectory_ = 
            BWResource::resolveFilenameW
            (
                MainFrame::instance()->ParticlesDirectory().GetString()
            ).c_str();
        FindClose(hFind);
    }

    if (dlg.doBrowse(AfxGetApp()->m_pMainWnd)) 
    {
        openDirectory(bw_wtoutf8( dlg.userSelectedDirectory_.GetString() ));
    }
}

void
ParticleEditorApp::setState(State state)
{
	BW_GUARD;

    MainFrame *mainFrame = MainFrame::instance();
    switch (state)
    {
    case PE_PLAYING:
		mfApp_->pause( false );
		{
			bool restart = m_state != PE_PAUSED;
			if (m_state == PE_PLAYING)
			{
				// try and spawn an additional ps
				size_t numAppendPS = mainFrame->numberAppendPS();
				mainFrame->appendOneShotPS();
				restart = numAppendPS == mainFrame->numberAppendPS();
			}
			if (restart)
			{
				if (mainFrame->IsMetaParticleSystem())
				{
					mainFrame->GetMetaParticleSystem()->clear(); 
					mainFrame->GetMetaParticleSystem()->spawn(); 
				}
			}
		}
        break;
    case PE_STOPPED:
        mfApp_->pause( true );
        if (mainFrame->IsMetaParticleSystem())
		{
			mainFrame->GetMetaParticleSystem()->clear();
			mainFrame->GetMetaParticleSystem()->setFirstUpdate();
			// remove flares when stopping particle editor
			LensEffectManager::instance().clear();
			mainFrame->clearAppendedPS();
		}
        break;
    case PE_PAUSED:
        mfApp_->pause( true );
        break;
    }
    m_state = state;
}

ParticleEditorApp::State ParticleEditorApp::getState() const
{
    return m_state;
}




void ParticleEditorApp::OnAppAbout()
{
	BW_GUARD;

    CAboutDlg aboutDlg;
    aboutDlg.DoModal();
}


// App command to show the keyboard shortcuts
void ParticleEditorApp::OnAppShortcuts()
{
	BW_GUARD;

    ShortcutsDlg::instance()->ShowWindow(SW_SHOW);
}

void 
ParticleEditorApp::openDirectory
(
    BW::string      const &dir_, 
    bool        forceRefresh    /*= false*/
)
{
	BW_GUARD;

    BW::string dir = BWResource::formatPath(dir_);

    BW::string relativeDirectory = BWResource::dissolveFilename(dir);

    // check if directory changed
    if 
    (
        MainFrame::instance()->ParticlesDirectory() 
        != 
        CString(relativeDirectory.c_str())
        ||
        forceRefresh
    )
    {
        MainFrame::instance()->PromptSave(MB_YESNO, true);
        // record the change
        MainFrame::instance()->ParticlesDirectory(bw_utf8tow( relativeDirectory ).c_str());
        ParticleEditorDoc::instance().SetTitle(bw_utf8tow( relativeDirectory ).c_str());
        MainFrame::instance()->PotentiallyDirty( false );       
        // tell the window
        MainFrame::instance()->InitialiseMetaSystemRegister();
    }
}

void ParticleEditorApp::OnFileSaveParticleSystem()
{
	BW_GUARD;

    MainFrame::instance()->ForceSave();
}

void ParticleEditorApp::OnViewShowSide()
{
	BW_GUARD;

    bool vis = PanelManager::instance().isPanelVisible( ActionSelection::contentID );

	PanelManager::instance().showPanel( ActionSelection::contentID, !vis );
}

void ParticleEditorApp::OnViewShowUAL()
{
	BW_GUARD;

	bool vis = PanelManager::instance().isPanelVisible( UalDialog::contentID );

	PanelManager::instance().showPanel( UalDialog::contentID, !vis );
}

void ParticleEditorApp::OnViewShowMsgs()
{
	BW_GUARD;

	bool vis = PanelManager::instance().isPanelVisible( PageMessages::contentID );

	PanelManager::instance().showPanel( PageMessages::contentID, !vis );
}

/*virtual*/ BW::string ParticleEditorApp::get(BW::string const &key) const
{
	BW_GUARD;

    return Options::getOptionString(key);
}

/*virtual*/ bool ParticleEditorApp::exist(BW::string const &key) const
{
	BW_GUARD;

    return Options::optionExists(key);
}

/*virtual*/ void ParticleEditorApp::set(BW::string const &key, BW::string const &value)
{
	BW_GUARD;

    return Options::setOptionString(key, value);
}

/*~ function ParticleEditor.openFile
 *	@components{ particleeditor }
 *
 *	This function enables the Open ParticleSystem dialog, which allows a ParticleSystem to be loaded.
 */
static PyObject *py_openFile(PyObject * /*args*/)
{
	BW_GUARD;

    ParticleEditorApp::instance().OnDirectoryOpen();

	Py_RETURN_NONE;
}
PY_MODULE_FUNCTION(openFile, ParticleEditor)

/*~ function ParticleEditor.savePS
 *	@components{ particleeditor }
 *
 *	This function saves any changes made to the currently selected ParticleSystem.
 */
static PyObject *py_savePS(PyObject * /*args*/)
{
	BW_GUARD;

    ParticleEditorApp::instance().OnFileSaveParticleSystem();

	Py_RETURN_NONE;
}
PY_MODULE_FUNCTION(savePS, ParticleEditor)

/*~ function ParticleEditor.reloadTextures
 *	@components{ particleeditor }
 *
 *	This function forces ParticleEditor to reload all its textures.
 */
static PyObject * py_reloadTextures( PyObject * args )
{
	BW_GUARD;

	CWaitCursor wait;
	
	Moo::ManagedTexture::accErrs(true);
	
	Moo::TextureManager::instance()->reloadAllTextures();

	BW::string errStr = Moo::ManagedTexture::accErrStr();
	if (errStr != "")
	{
		ERROR_MSG
        ( 
            "Moo:ManagedTexture::load, unable to load the following textures:\n"
			"%s\n\nPlease ensure these textures exist.", 
            errStr.c_str() 
        );
	}

	Moo::ManagedTexture::accErrs(false);

	Py_RETURN_NONE;
}
PY_MODULE_FUNCTION(reloadTextures, ParticleEditor)

/*~ function ParticleEditor.exit
 *	@components{ particleeditor }
 *
 *	This function closes ParticleEditor.
 */
static PyObject *py_exit(PyObject * /*args*/)
{
	BW_GUARD;

    AfxGetApp()->GetMainWnd()->PostMessage( WM_COMMAND, ID_APP_EXIT );

	Py_RETURN_NONE;
}
PY_MODULE_FUNCTION(exit, ParticleEditor)

/*~ function ParticleEditor.showToolbar
 *	@components{ particleeditor }
 *
 *	This function shows the specified toolbar.
 *
 *	@param str	The name of the toolbar to show.
 */
static PyObject *py_showToolbar(PyObject* args)
{
	BW_GUARD;

	if ( !MainFrame::instance() )
		return PyInt_FromLong( 0 );

	char * str;
	if (!PyArg_ParseTuple( args, "s", &str ))
	{
		PyErr_SetString( PyExc_TypeError,
			"py_showToolbar: Argument parsing error." );
		return NULL;
	}
	MainFrame::instance()->showToolbarIndex( atoi( str ) );

    Py_RETURN_NONE;
}
PY_MODULE_FUNCTION(showToolbar, ParticleEditor)

/*~ function ParticleEditor.hideToolbar
 *	@components{ particleeditor }
 *
 *	This function hides the specified toolbar.
 *
 *	@param str	The name of the toolbar to hide.
 */
static PyObject *py_hideToolbar(PyObject* args)
{
	BW_GUARD;

	if ( !MainFrame::instance() )
		return PyInt_FromLong( 0 );

	char * str;
	if (!PyArg_ParseTuple( args, "s", &str ))
	{
		PyErr_SetString( PyExc_TypeError,
			"py_showToolbar: Argument parsing error." );
		return NULL;
	}
	MainFrame::instance()->hideToolbarIndex( atoi( str ) );

    Py_RETURN_NONE;
}
PY_MODULE_FUNCTION(hideToolbar, ParticleEditor)

/*~ function ParticleEditor.updateShowToolbar
 *	@components{ particleeditor }
 *
 *	This function updates the status of the tick next to the specified toolbar in the
 *	View->Toolbars menu.
 *
 *	@param str	The name of the toolbar to update status.
 *
 *	@return Returns True (0) if the toolbar is currently shown, False (0) otherwise.
 */
static PyObject *py_updateShowToolbar(PyObject* args)
{
	BW_GUARD;

	if ( !MainFrame::instance() )
		return PyInt_FromLong( 0 );

	char * str;
	if (!PyArg_ParseTuple( args, "s", &str ))
	{
		PyErr_SetString( PyExc_TypeError,
			"py_showToolbar: Argument parsing error." );
		return NULL;
	}

	return PyInt_FromLong(
		MainFrame::instance()->updateToolbarIndex( atoi( str ) ) );
}
PY_MODULE_FUNCTION(updateShowToolbar, ParticleEditor)
 
/*~ function ParticleEditor.showStatusbar
 *	@components{ particleeditor }
 *
 *	This function shows the status bar.
 */
static PyObject *py_showStatusbar(PyObject * /*args*/)
{
	BW_GUARD;

    MainFrame::instance()->ShowControlBar
    (
        &MainFrame::instance()->getStatusBar(),
        TRUE,
        FALSE
    );
    Py_RETURN_NONE;
}
PY_MODULE_FUNCTION(showStatusbar, ParticleEditor)

/*~ function ParticleEditor.hideStatusbar
 *	@components{ particleeditor }
 *
 *	This function hides the status bar.
 */
static PyObject *py_hideStatusbar(PyObject * /*args*/)
{
	BW_GUARD;

    MainFrame::instance()->ShowControlBar
    (
        &MainFrame::instance()->getStatusBar(),
        FALSE,
        FALSE
    );
    Py_RETURN_NONE;
}
PY_MODULE_FUNCTION(hideStatusbar, ParticleEditor)

/*~ function ParticleEditor.updateShowStatusbar
 *	@components{ particleeditor }
 *
 *	This function updates the status of the tick next to the status bar in the view menu.
 *
 *	@return Returns True (0) if the toolbar is currently shown, False (1) otherwise.
 */
static PyObject *py_updateShowStatusbar(PyObject * /*args*/)
{
	BW_GUARD;

    bool valid = 
        MainFrame::instance() != NULL
        &&
        ::IsWindow(MainFrame::instance()->getStatusBar().GetSafeHwnd());
    bool visible = true;
    if (valid)
        visible = MainFrame::instance()->getStatusBar().IsWindowVisible() != FALSE;
    return PyInt_FromLong(visible ? 0 : 1);
}
PY_MODULE_FUNCTION(updateShowStatusbar, ParticleEditor)

/*~ function ParticleEditor.toggleShowPanels
 *	@components{ particleeditor }
 *
 *	This function toggles the panels' visibility.
 */
static PyObject *py_toggleShowPanels(PyObject * /*args*/)
{
	BW_GUARD;

    ParticleEditorApp::instance().OnViewShowSide();
    Py_RETURN_NONE;
}
PY_MODULE_FUNCTION(toggleShowPanels, ParticleEditor)

/*~ function ParticleEditor.toggleShowUALPanel
 *	@components{ particleeditor }
 *
 *	This function toggles the UAL panel's visibility.
 */
static PyObject *py_toggleShowUALPanel(PyObject * /*args*/)
{
	BW_GUARD;

    ParticleEditorApp::instance().OnViewShowUAL();
    Py_RETURN_NONE;
}
PY_MODULE_FUNCTION(toggleShowUALPanel, ParticleEditor)

/*~ function ParticleEditor.toggleShowMsgsPanel
 *	@components{ particleeditor }
 *
 *	This function toggles the UAL panel's visibility.
 */
static PyObject *py_toggleShowMsgsPanel(PyObject * /*args*/)
{
	BW_GUARD;

    ParticleEditorApp::instance().OnViewShowMsgs();
    Py_RETURN_NONE;
}
PY_MODULE_FUNCTION(toggleShowMsgsPanel, ParticleEditor)

/*~ function ParticleEditor.loadDefaultPanels
 *	@components{ particleeditor }
 *
 *	This function loads the default panel arrangement.
 */
static PyObject *py_loadDefaultPanels(PyObject * /*args*/)
{
	BW_GUARD;

    XMLSectionPtr data = new XMLSection("ActionSelection_state");
    ActionSelection::instance()->saveState(data);
    PanelManager   ::instance().loadDefaultPanels(NULL);
    ActionSelection::instance()->restoreState(data);
    Py_RETURN_NONE;
}
PY_MODULE_FUNCTION(loadDefaultPanels, ParticleEditor)

/*~ function ParticleEditor.loadRecentPanels
 *	@components{ particleeditor }
 *
 *	This function loads the most recent panel arrangement.
 */
static PyObject *py_loadRecentPanels(PyObject * /*args*/)
{
	BW_GUARD;

    XMLSectionPtr data = new XMLSection("ActionSelection_state");
    ActionSelection::instance()->saveState(data);
    PanelManager   ::instance().loadLastPanels(NULL, L"particleeditor.layout");
    ActionSelection::instance()->restoreState(data);
    Py_RETURN_NONE;
}
PY_MODULE_FUNCTION(loadRecentPanels, ParticleEditor)



/*~ function ParticleEditor.aboutApp
 *	@components{ particleeditor }
 *
 *	This function displays the ParticleEditor's About box.
 */
static PyObject *py_aboutApp(PyObject * /*args*/)
{
	BW_GUARD;

    ParticleEditorApp::instance().OnAppAbout();
    Py_RETURN_NONE;
}
PY_MODULE_FUNCTION(aboutApp, ParticleEditor)


/*~ function ParticleEditor.doShortcuts
 *	@components{ particleeditor }
 *
 *	This function opens the ParticleEditor's Shortcuts dialog.
 */
static PyObject *py_doShortcuts(PyObject * /*args*/)
{
	BW_GUARD;

    ParticleEditorApp::instance().OnAppShortcuts();
    Py_RETURN_NONE;
}
PY_MODULE_FUNCTION(doShortcuts, ParticleEditor)

/*~ function ParticleEditor.zoomToExtents
 *	@components{ particleeditor }
 *
 *	This function centres the particle system in view and zooms the camera until
 *	the particle system just fits in view.
 */
static PyObject * py_zoomToExtents( PyObject * args )
{
	BW_GUARD;

	PeShell::instance().camera().zoomToExtents( true );
	Py_RETURN_NONE;
}
PY_MODULE_FUNCTION( zoomToExtents, ParticleEditor )

/*~ function ParticleEditor.doViewFree
 *	@components{ particleeditor }
 *
 *	This function enables the free view camera mode.
 */
static PyObject *py_doViewFree(PyObject * /*args*/)
{
	BW_GUARD;

    MainFrame::instance()->OnButtonViewFree();
    Py_RETURN_NONE;
}
PY_MODULE_FUNCTION(doViewFree, ParticleEditor)

/*~ function ParticleEditor.doViewX
 *	@components{ particleeditor }
 *
 *	This function positions the camera to look toward the origin along the X-axis.
 */
static PyObject *py_doViewX(PyObject * /*args*/)
{
	BW_GUARD;

    MainFrame::instance()->OnButtonViewX();
    Py_RETURN_NONE;
}
PY_MODULE_FUNCTION(doViewX, ParticleEditor)

/*~ function ParticleEditor.doViewY
 *	@components{ particleeditor }
 *
 *	This function positions the camera to look toward the origin along the Y-axis.
 */
static PyObject *py_doViewY(PyObject * /*args*/)
{
	BW_GUARD;

    MainFrame::instance()->OnButtonViewY();
    Py_RETURN_NONE;
}
PY_MODULE_FUNCTION(doViewY, ParticleEditor)

/*~ function ParticleEditor.doViewZ
 *	@components{ particleeditor }
 *
 *	This function positions the camera to look toward the origin along the Z-axis.
 */
static PyObject *py_doViewZ(PyObject * /*args*/)
{
	BW_GUARD;

    MainFrame::instance()->OnButtonViewZ();
    Py_RETURN_NONE;
}
PY_MODULE_FUNCTION(doViewZ, ParticleEditor)

/*~ function ParticleEditor.doViewOrbit
 *	@components{ particleeditor }
 *
 *	This function enables the orbit view camera mode.
 */
static PyObject *py_doViewOrbit(PyObject * /*args*/)
{
	BW_GUARD;

    MainFrame::instance()->OnButtonViewOrbit();
    Py_RETURN_NONE;
}
PY_MODULE_FUNCTION(doViewOrbit, ParticleEditor)

/*~ function ParticleEditor.cameraMode
 *	@components{ particleeditor }
 *
 *	This function returns which camera mode is currently being used.
 *
 *	@return Returns 0 if in free view, returns 1 if in x-locked view,
 *			returns 2 if in y-locked view, returns 3 if in z-locked view,
 *			returns 4 if in orbit view.
 */
static PyObject *py_cameraMode(PyObject * /*args*/)
{
	BW_GUARD;

    int result = PeShell::instance().camera().mode();
    return PyInt_FromLong(result);
}
PY_MODULE_FUNCTION(cameraMode, ParticleEditor)

/*~ function ParticleEditor.camera
 *	@components{ particleeditor }
 *
 *	This function returns the ParticleEditor camera.
 *
 *	@return The ParticleEditor camera object.
 */
static PyObject * py_camera( PyObject * args )
{
	BW_GUARD;

	Py_INCREF(&PeShell::instance().camera());
	return &PeShell::instance().camera();
}
PY_MODULE_FUNCTION(camera, ParticleEditor)

/*~ function ParticleEditor.doSetBkClr
 *	@components{ particleeditor }
 *
 *	This function enables ParticleEditor's Colour Picker dialog, which allows the 
 *	background colour to be changed.
 */
static PyObject *py_doSetBkClr(PyObject * /*args*/)
{
	BW_GUARD;

    MainFrame::instance()->OnBackgroundColor();
    Py_RETURN_NONE;
}
PY_MODULE_FUNCTION(doSetBkClr, ParticleEditor)

/*~ function ParticleEditor.updateBkClr
 *	@components{ particleeditor }
 *
 *	This function checks whether a background colour is currently set.
 *
 *	@return Returns True (0) if a background colour is set, False (1) otherwise.
 */
static PyObject *py_updateBkClr(PyObject * /*args*/)
{
	BW_GUARD;

    if (MainFrame::instance() != NULL)
    {
        return 
            PyInt_FromLong
            (
		        MainFrame::instance()->showingBackgroundColor() ? 0 : 1 
            );
    }
    else
    {
        return PyInt_FromLong(1);
    }
}
PY_MODULE_FUNCTION(updateBkClr, ParticleEditor)

/*~ function ParticleEditor.showGrid
 *	@components{ particleeditor }
 *
 *	This function toggles the display of the 1x1m measurement grid.
 */
static PyObject *py_showGrid(PyObject * /*args*/)
{
	BW_GUARD;

	Options::setOptionInt( "render/showGrid", !Options::getOptionInt( "render/showGrid", 0 ));
	GUI::Manager::instance().update();
    Py_RETURN_NONE;
}
PY_MODULE_FUNCTION(showGrid, ParticleEditor)

/*~ function ParticleEditor.isShowingGrid
 *	@components{ particleeditor }
 *
 *	This function checks whether the 1x1m measurement grid is currently being displayed.
 *
 *	@return Returns True (1) if the 1x1m measurement grid is being displayed, False (0) otherwise.
 */
static PyObject *py_isShowingGrid(PyObject * /*args*/)
{
	BW_GUARD;

    return PyInt_FromLong(Options::getOptionInt( "render/showGrid", 0 ));
}
PY_MODULE_FUNCTION(isShowingGrid, ParticleEditor)

/*~ function ParticleEditor.undo
 *	@components{ particleeditor }
 *
 *	This function undoes the most recent operation.
 */
static PyObject *py_undo(PyObject * /*args*/)
{
	BW_GUARD;

	if ( MainFrame::instance()->CanUndo() )
		MainFrame::instance()->OnUndo();
    Py_RETURN_NONE;
}
PY_MODULE_FUNCTION(undo, ParticleEditor)
   
/*~ function ParticleEditor.canUndo
 *	@components{ particleeditor }
 *
 *	This function checks whether it is possible to undo the most recent operation.
 *
 *	@return Returns True (1) if can undo, False (0) otherwise.
 */
static PyObject *py_canUndo(PyObject * /*args*/)
{
	BW_GUARD;

    int result = MainFrame::instance()->CanUndo() ? 1 : 0;
    return PyInt_FromLong(result);
}
PY_MODULE_FUNCTION(canUndo, ParticleEditor)

/*~ function ParticleEditor.redo
 *	@components{ particleeditor }
 *
 *	This function redoes the most recent undo operation.
 */
static PyObject *py_redo(PyObject * /*args*/)
{
	BW_GUARD;

	if ( MainFrame::instance()->CanRedo() )
		MainFrame::instance()->OnRedo();
    Py_RETURN_NONE;
}
PY_MODULE_FUNCTION(redo, ParticleEditor)

/*~ function ParticleEditor.canRedo
 *	@components{ particleeditor }
 *
 *	This function checks whether it is possible to redo the most recent undo operation.
 *
 *	@return Returns True (1) if can redo, False (0) otherwise.
 */
static PyObject *py_canRedo(PyObject * /*args*/)
{
	BW_GUARD;

    int result = MainFrame::instance()->CanRedo() ? 1 : 0;
    return PyInt_FromLong(result);
}
PY_MODULE_FUNCTION(canRedo, ParticleEditor)

/*~ function ParticleEditor.doSpawn
 *	@components{ particleeditor }
 *
 *	This function spawns the currently selected Particle System and sets its state to 'playing'.
 */
static PyObject *py_doSpawn(PyObject * /*args*/)
{
	BW_GUARD;

	if (ParticleEditorApp::instance().getState() == ParticleEditorApp::PE_PAUSED)
	{
		// resume the existing particle system
		ParticleEditorApp::instance().setState(ParticleEditorApp::PE_PLAYING);
	}
	// spawn a new particle system
    ParticleEditorApp::instance().setState(ParticleEditorApp::PE_PLAYING);
    Py_RETURN_NONE;
}
PY_MODULE_FUNCTION(doSpawn, ParticleEditor)

/*~ function ParticleEditor.doRestart
 *	@components{ particleeditor }
 *
 *	This function restarts the currently selected Particle System and sets its state to 'playing'.
 */
static PyObject *py_doRestart(PyObject * /*args*/)
{
	BW_GUARD;

	// stop the existing particle system
	ParticleEditorApp::instance().setState(ParticleEditorApp::PE_STOPPED);
	// spawn a new particle system
    ParticleEditorApp::instance().setState(ParticleEditorApp::PE_PLAYING);
    Py_RETURN_NONE;
}
PY_MODULE_FUNCTION(doRestart, ParticleEditor)

/*~ function ParticleEditor.doPlay
 *	@components{ particleeditor }
 *
 *	This function sets the currently selected Particle System to 'playing'.
 */
static PyObject *py_doPlay(PyObject * /*args*/)
{
	BW_GUARD;

	if (ParticleEditorApp::instance().getState() != ParticleEditorApp::PE_PLAYING)
	{
		ParticleEditorApp::instance().setState(ParticleEditorApp::PE_PLAYING);
	}
    Py_RETURN_NONE;
}
PY_MODULE_FUNCTION(doPlay, ParticleEditor)

/*~ function ParticleEditor.doStop
 *	@components{ particleeditor }
 *
 *	This function sets the currently selected Particle System state to 'stopped'.
 */
static PyObject *py_doStop(PyObject * /*args*/)
{
	BW_GUARD;

    ParticleEditorApp::instance().setState(ParticleEditorApp::PE_STOPPED);
    Py_RETURN_NONE;
}
PY_MODULE_FUNCTION(doStop, ParticleEditor)

/*~ function ParticleEditor.doPause
 *	@components{ particleeditor }
 *
 *	This function sets the the currently selected Particle System state to 'paused'.
 */
static PyObject *py_doPause(PyObject * /*args*/)
{
	BW_GUARD;

	if (ParticleEditorApp::instance().getState() == ParticleEditorApp::PE_PAUSED)
	{
        ParticleEditorApp::instance().setState(ParticleEditorApp::PE_PLAYING);
    }
    else
    {
        ParticleEditorApp::instance().setState(ParticleEditorApp::PE_PAUSED);
    }
    Py_RETURN_NONE;
}
PY_MODULE_FUNCTION(doPause, ParticleEditor)

/*~ function ParticleEditor.getState
 *	@components{ particleeditor }
 *
 *	This function returns the current state of the Particle System, whether it is Playing, Paused or Stopped.
 *
 *	@return Returns 0 if the state is Playing, returns 1 if the state is Paused,
 *			and returns 2 if the state is Stopped.
 */
static PyObject *py_getState(PyObject * /*args*/)
{
	BW_GUARD;

    int result = (int)ParticleEditorApp::instance().getState();
    return PyInt_FromLong(result);
}
PY_MODULE_FUNCTION(getState, ParticleEditor)

BW_END_NAMESPACE

