#include "pch.hpp"

#include "appmgr/options.hpp"
#include "common/user_messages.hpp"
#include "guitabs/guitabs.hpp"
#include "tools/common/python_adapter.hpp"
#include "resmgr/string_provider.hpp"

#include "main_frm.h"
#include "about_box.hpp"
#include "me_error_macros.hpp"

#include "page_display.hpp"
#include "page_object.hpp"
#include "page_animations.hpp"
#include "page_actions.hpp"
#include "page_lod.hpp"
#include "page_lights.hpp"
#include "page_materials.hpp"
#include "tools/common/page_messages.hpp"

#include "ual/ual_history.hpp"
#include "ual/ual_manager.hpp"
#include "ual/ual_dialog.hpp"

#include "controls/user_messages.hpp"
#include "controls/cursor_utils.hpp"
#include "cstdmf/message_box.hpp"
#include "cstdmf/restart.hpp"
#include "panel_manager.hpp"

#include <afxdisp.h>
#include <afxdhtml.h>
#include "editor_shared/cursor/wait_cursor.hpp"
#include "modeleditor_core/App/me_python_adapter.hpp"

BW_BEGIN_NAMESPACE

namespace
{
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
                s_instance = new ShortcutsDlg(IDD_SHORTCUTS);
                s_instance->Create(IDD_SHORTCUTS);
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

struct PanelManager::Impl
{
	Impl( PanelManager* parent ): parent_( parent ) {}

	void setMainFrame()
	{
		const BW::list<GUITABS::PanelPtr>& panels =
			parent_->panels().dock()->getPanels();

		BW::list<GUITABS::PanelPtr>::const_iterator it;

		for (it = panels.begin(); it != panels.end(); ++it)
		{
			GUITABS::PanelPtr panel = *it;
			const BW::list<GUITABS::TabPtr>& tabs = panel->getTabs();
			BW::list<GUITABS::TabPtr>::const_iterator tabIt;

			for (tabIt = tabs.begin(); tabIt != tabs.end(); ++tabIt)
			{
				GUITABS::TabPtr tab = *tabIt;
				GuiTabContent* guiTabContent = 
					dynamic_cast<GuiTabContent*>( tab->getContent().get() );

				if (guiTabContent != nullptr)
				{
					guiTabContent->setMainFrame( parent_->mainFrame_ );
				}
			}
		}
	}

	PanelManager* parent_;
};

BW_SINGLETON_STORAGE( PanelManager )


PanelManager::PanelManager() :
	GUI::ActionMaker<PanelManager>( "doDefaultPanelLayout|doShowSidePanel|doHideSidePanel|doLoadPanelLayout|"
		"recent_models|recent_lights|doAboutApp|doShortcuts|setLanguage",
		&PanelManager::handelGUIAction ),
	GUI::UpdaterMaker<PanelManager>( "updateSidePanel|updateLanguage", &PanelManager::handleGUIUpdate ),
	mainFrame_( NULL ),
	ready_( false ),
	addCursor_( NULL ),
	editorApp_( NULL )
{
	BW_GUARD;

	pImpl = new Impl( this );
	addCursor_ = controls::addPlusSignToCursor( LoadCursor( NULL, IDC_ARROW ) );
	if (!addCursor_)
	{
		ERROR_MSG( "PanelManager: Could not create custom 'Add' cursor. "
					"Falling back to the default cursor instead.\n" );
	}
}


PanelManager::~PanelManager()
{
	BW_GUARD;

	if (addCursor_)
	{
		DestroyCursor( addCursor_ );
		addCursor_ = NULL;
	}

	bw_safe_delete( pImpl );
}


/*static*/ void PanelManager::fini()
{
	BW_GUARD;

	ShortcutsDlg::cleanup();

	if (pInstance())
	{
		instance().ready_ = false;

		delete pInstance();
	}
}


/*static*/ bool PanelManager::init(
	CFrameWnd * mainFrameWnd, CWnd* mainView,
	IModelEditorApp * editorApp, IMainFrame * mainFrame )
{
	BW_GUARD;

	PanelManager* manager = new PanelManager();

	instance().editorApp_ = editorApp;
	instance().mainFrame_ = mainFrame;
	instance().mainFrameWnd_ = mainFrameWnd;
	instance().panels().insertDock( mainFrameWnd, mainView );

	if ( !instance().initPanels() )
		return false;

	return true;
}


void PanelManager::finishLoad()
{
	BW_GUARD;

	// show the default panels
	this->panels().showPanel( UalDialog::contentID, true );
	
	PageMessages* msgs = (PageMessages*)(this->panels().getContent(PageMessages::contentID ));
	if (msgs)
	{
		msgs->mainFrame( mainFrameWnd_ );
	}

	ready_ = true;
}

bool PanelManager::initPanels()
{
	BW_GUARD;

	if ( ready_ )
		return false;

	WaitCursor wait;

	// UAL Setup
	for ( int i = 0; i < BWResource::getPathNum(); i++ )
	{
		BW::string path = BWResource::getPath( i );
		if ( path.find("modeleditor") != BW::string::npos )
			continue;
		UalManager::instance().addPath( bw_utf8tow( path ) );
	}
	UalManager::instance().setConfigFile( bw_utf8tow( 
		Options::getOptionString(
			"ualConfigPath",
			"resources/ual/ual_config.xml" ) ) );

	UalManager::instance().setItemDblClickCallback(
		new UalFunctor1<PanelManager, UalItemInfo*>( pInstance(), &PanelManager::ualItemDblClick ) );
	UalManager::instance().setStartDragCallback(
		new UalFunctor1<PanelManager, UalItemInfo*>( pInstance(), &PanelManager::ualStartDrag ) );
	UalManager::instance().setUpdateDragCallback(
		new UalFunctor1<PanelManager, UalItemInfo*>( pInstance(), &PanelManager::ualUpdateDrag ) );
	UalManager::instance().setEndDragCallback(
		new UalFunctor1<PanelManager, UalItemInfo*>( pInstance(), &PanelManager::ualEndDrag ) );
	UalManager::instance().setPopupMenuCallbacks(
		new UalFunctor2<PanelManager, UalItemInfo*, UalPopupMenuItems&>( pInstance(), &PanelManager::ualStartPopupMenu ),
		new UalFunctor2<PanelManager, UalItemInfo*, int>( pInstance(), &PanelManager::ualEndPopupMenu ));
	
	this->panels().registerFactory( new UalDialogFactory() );

	// Setup the map which is used for python
	contentID_["UAL"] = UalDialog::contentID;
	contentID_["Display"] = PageDisplay::contentID;
	contentID_["Object"] = PageObject::contentID;
	contentID_["Animations"] = PageAnimations::contentID;
	contentID_["Actions"] = PageActions::contentID;
	contentID_["LOD"] = PageLOD::contentID;
	contentID_["Lights"] = PageLights::contentID;
	contentID_["Materials"] = PageMaterials::contentID;
	contentID_["Messages"] = PageMessages::contentID;

	// other panels setup
	this->panels().registerFactory( new PageDisplayFactory );
	this->panels().registerFactory( new PageObjectFactory );
	this->panels().registerFactory( new PageAnimationsFactory );
	this->panels().registerFactory( new PageActionsFactory );
	this->panels().registerFactory( new PageLODFactory );
	this->panels().registerFactory( new PageLightsFactory );
	this->panels().registerFactory( new PageMaterialsFactory );
	this->panels().registerFactory( new PageMessagesFactory );
	
	if (mainFrameWnd_ != NULL)
	{
		if ( ( (CMainFrame*)mainFrameWnd_ )->verifyBarState( L"TBState" ) )
			mainFrameWnd_->LoadBarState( L"TBState" );
	}

	if (this->panels().load( L"modeleditor.layout" ))
	{
		pImpl->setMainFrame();
	}
	else
	{
		loadDefaultPanels( NULL );
	}

	finishLoad();

	return true;
}

bool PanelManager::loadDefaultPanels( GUI::ItemPtr item )
{
	BW_GUARD;

	WaitCursor wait;
	bool isFirstCall = true;
	if ( ready_ )
	{
		if ( MessageBox( mainFrameWnd_->GetSafeHwnd(),
			Localise(L"MODELEDITOR/GUI/PANEL_MANAGER/LOAD_DEFAULT_Q"),
			Localise(L"MODELEDITOR/GUI/PANEL_MANAGER/LOAD_DEFAULT"),
			MB_YESNO | MB_ICONQUESTION ) != IDYES )
			return false;

		ready_ = false;
		isFirstCall = false;
		// already has something in it, so clean up first
		this->panels().removePanels();
	}

	if ( item != 0 && mainFrameWnd_ != NULL)
	{
		// not first panel load, so rearrange the toolbars
		((CMainFrame*) mainFrameWnd_ )->defaultToolbarLayout();
	}

	GUITABS::PanelHandle basePanel = panels().insertPanel( UalDialog::contentID, GUITABS::RIGHT );
	this->panels().insertPanel( PageObject::contentID, GUITABS::TAB, basePanel );
	this->panels().insertPanel( PageDisplay::contentID, GUITABS::TAB, basePanel );
	this->panels().insertPanel( PageAnimations::contentID, GUITABS::TAB, basePanel );
	this->panels().insertPanel( PageActions::contentID, GUITABS::TAB, basePanel );
	this->panels().insertPanel( PageLOD::contentID, GUITABS::TAB, basePanel );
	this->panels().insertPanel( PageLights::contentID, GUITABS::TAB, basePanel );
	this->panels().insertPanel( PageMaterials::contentID, GUITABS::TAB, basePanel );
	this->panels().insertPanel( PageMessages::contentID, GUITABS::TAB, basePanel );
	pImpl->setMainFrame();

	if ( !isFirstCall )
		finishLoad();

	return true;
}

bool PanelManager::loadLastPanels( GUI::ItemPtr item )
{
	BW_GUARD;

	WaitCursor wait;
	if ( MessageBox( mainFrameWnd_->GetSafeHwnd(),
		Localise(L"MODELEDITOR/GUI/PANEL_MANAGER/LOAD_RECENT_Q"),
		Localise(L"MODELEDITOR/GUI/PANEL_MANAGER/LOAD_RECENT"),
		MB_YESNO | MB_ICONQUESTION ) != IDYES )
		return false;

	ready_ = false;

	if ( ( (CMainFrame*)mainFrameWnd_ )->verifyBarState( L"TBState" ) )
		mainFrameWnd_->LoadBarState( L"TBState" );

	if (this->panels().load( L"modeleditor.layout" ))
	{
		pImpl->setMainFrame();
	}
	else
	{
		loadDefaultPanels( NULL );
	}

	finishLoad();

	return true;
}

bool PanelManager::recent_models( GUI::ItemPtr item )
{
	BW_GUARD;

	if (!MeApp::instance().canExit( false ))
		return false;
	
	editorApp_->modelToLoad( (*item)[ "fileName" ] );

	return true;
}

bool PanelManager::recent_lights( GUI::ItemPtr item )
{
	BW_GUARD;

	PageLights* lightPage = (PageLights*)PanelManager::instance().panels().getContent( PageLights::contentID );
	
	bool loaded = lightPage->openLightFile( bw_utf8tow( (*item)[ "fileName" ] ) );

	editorApp_->updateRecentList( "lights" );

	return loaded;
}

bool PanelManager::setLanguage( GUI::ItemPtr item )
{
	BW_GUARD;

	BW::string languageName = (*item)[ "LanguageName" ];
	BW::string countryName = (*item)[ "CountryName" ];

	// Do nothing if we are not changing language
	if (currentLanguageName_ == languageName && currentCountryName_ == countryName)
	{
		return true;
	}

	unsigned int result;
	if (MeApp::instance().isDirty())
	{
		result = MsgBox( Localise(L"RESMGR/CHANGING_LANGUAGE_TITLE"), Localise(L"RESMGR/CHANGING_LANGUAGE"),
			Localise(L"RESMGR/SAVE_AND_RESTART"), Localise(L"RESMGR/DISCARD_AND_RESTART"),
			Localise(L"RESMGR/RESTART_LATER"), Localise(L"RESMGR/CANCEL") ).doModal();
	}
	else
	{
		result = MsgBox( Localise(L"RESMGR/CHANGING_LANGUAGE_TITLE"), Localise(L"RESMGR/CHANGING_LANGUAGE"),
			Localise(L"RESMGR/RESTART_NOW"), Localise(L"RESMGR/RESTART_LATER"), Localise(L"RESMGR/CANCEL") ).doModal() + 1;
	}
	switch (result)
	{
	case 0:
		Options::setOptionString( "currentLanguage", languageName );
		Options::setOptionString( "currentCountry", countryName );
		MeApp::instance().saveModel();
		startNewInstance();
		AfxGetApp()->GetMainWnd()->PostMessage( WM_COMMAND, ID_APP_EXIT );
		break;
	case 1:	
		Options::setOptionString( "currentLanguage", languageName );
		Options::setOptionString( "currentCountry", countryName );
		MeApp::instance().forceClean();
		startNewInstance();
		AfxGetApp()->GetMainWnd()->PostMessage( WM_COMMAND, ID_APP_EXIT );
		break;
	case 2:
		Options::setOptionString( "currentLanguage", languageName );
		Options::setOptionString( "currentCountry", countryName );
		currentLanguageName_ = languageName;
		currentCountryName_ = countryName;
		break;
	case 3:
		break;
	}
	return true;
}

unsigned int PanelManager::updateLanguage( GUI::ItemPtr item )
{
	BW_GUARD;

	if (currentLanguageName_.empty())
	{
		currentLanguageName_ = StringProvider::instance().currentLanguage()->getIsoLangNameUTF8();
		currentCountryName_ = StringProvider::instance().currentLanguage()->getIsoCountryNameUTF8();
	}
	return currentLanguageName_ == (*item)[ "LanguageName" ] && currentCountryName_ == (*item)[ "CountryName" ];
}

// App command to run the about dialog
bool PanelManager::OnAppAbout( GUI::ItemPtr item )
{
	BW_GUARD;

	CAboutDlg().DoModal();
	return true;
}

// App command to show the keyboard shortcuts
bool PanelManager::OnShortcuts( GUI::ItemPtr item )
{
	BW_GUARD;

    ShortcutsDlg::instance()->ShowWindow(SW_SHOW);
	return true;
}

bool PanelManager::ready()
{
	return ready_;
}

void PanelManager::showPanel( const BW::string& pyID, int show /* = 1 */ )
{
	const BW::wstring& panelID = getPanelID( pyID );

	if (!panelID.empty())
	{
		this->panels().showPanel( panelID, show != 0 );
	}
}

int PanelManager::isPanelVisible( const BW::string& pyID )
{
	BW_GUARD;

	const BW::wstring& panelID = getPanelID( pyID );

	if (!panelID.empty())
	{
		return this->panels().isContentVisible( panelID );
	}

	return 0;
}

BW::wstring PanelManager::getPanelID( const BW::string& pyID )
{
	BW_GUARD;

	return contentID_[pyID];
}

void PanelManager::ualItemDblClick( UalItemInfo* ii )
{
	BW_GUARD;

	if ( !ii ) return;

	if (editorApp_->pythonAdapter())
	{
		editorApp_->pythonAdapter()->callString( "openFile", 
			BWResource::dissolveFilename( bw_wtoutf8( ii->longText() ) ) );
	}
}

void PanelManager::ualStartDrag( UalItemInfo* ii )
{
	BW_GUARD;

	if ( !ii ) return;

	UalManager::instance().dropManager().start( 
		BWResource::getExtension( bw_wtoutf8( ii->longText() ) ).to_string() );
}

void PanelManager::ualUpdateDrag( UalItemInfo* ii )
{
	BW_GUARD;

	if ( !ii ) return;

	SmartPointer< UalDropCallback > dropable = UalManager::instance().dropManager().test( ii );
		
	if ((ii->isFolder()) || (dropable))
	{
		if (addCursor_ &&
			(ii->isFolder() ||
			(dropable && dropable->canAdd() &&
				(GetAsyncKeyState(VK_LCONTROL) < 0 ||
				GetAsyncKeyState(VK_RCONTROL) < 0 ||
				GetAsyncKeyState(VK_LMENU) < 0 ||
				GetAsyncKeyState(VK_RMENU) < 0))))
		{
			SetCursor( addCursor_ );
		}
		else
		{
			SetCursor( AfxGetApp()->LoadStandardCursor( IDC_ARROW ) );
		}
	}
	else
		SetCursor( AfxGetApp()->LoadStandardCursor( IDC_NO ) );
}

void PanelManager::ualEndDrag( UalItemInfo* ii )
{
	BW_GUARD;

	SetCursor( AfxGetApp()->LoadStandardCursor( IDC_ARROW ) );

	if ( !ii ) return;

	if ( ii->isFolder() )
	{
		// folder drag
		CPoint pt;
		GetCursorPos( &pt );
		AfxGetMainWnd()->ScreenToClient( &pt );
		this->panels().clone( (GUITABS::Content*)( ii->dialog() ),
			pt.x - 5, pt.y - 5 );
	}
	else
	{
		UalManager::instance().dropManager().end(ii);
	}
}

void PanelManager::ualStartPopupMenu( UalItemInfo* ii, UalPopupMenuItems& menuItems )
{
	BW_GUARD;

	if (!ii) return;
	
	if (!editorApp_->pythonAdapter())
	{
		return;
	}

	BW::map<int,BW::wstring> pyMenuItems;
	editorApp_->pythonAdapter()->contextMenuGetItems(
		ii->type(),
		BWResource::dissolveFilenameW( ii->longText() ).c_str(),
		pyMenuItems );

	if ( !pyMenuItems.size() ) return;

	for( BW::map<int,BW::wstring>::iterator i = pyMenuItems.begin();
		i != pyMenuItems.end(); ++i )
	{
		menuItems.push_back( UalPopupMenuItem( (*i).second, (*i).first ) );
	}
}

void PanelManager::ualEndPopupMenu( UalItemInfo* ii, int result )
{
	BW_GUARD;

	if (!ii) return;
	
	editorApp_->pythonAdapter()->contextMenuHandleResult(
		ii->type(),
		BWResource::dissolveFilenameW( ii->longText() ).c_str(),
		result );
}

void PanelManager::ualAddItemToHistory( BW::string filePath )
{
	BW_GUARD;

	// called from python
	BW::wstring fname = BWResource::getFilenameW( filePath );
	BW::wstring longText = BWResource::resolveFilenameW( filePath );
	std::replace( longText.begin(), longText.end(), L'/', L'\\' );
	UalManager::instance().history().add( AssetInfo( L"FILE", fname, longText ));
}

bool PanelManager::handelGUIAction( GUI::ItemPtr item )
{
	BW_GUARD;

	const BW::string& action = item->action();

	if (action == "doDefaultPanelLayout" )			return loadDefaultPanels( item );
	else if(action == "doShowSidePanel")			return showSidePanel( item );
	else if(action == "doHideSidePanel")			return hideSidePanel( item );
	else if(action == "doLoadPanelLayout")			return loadLastPanels( item );
	else if(action == "recent_models")				return recent_models( item );
	else if(action == "recent_lights")				return recent_lights( item );
	else if(action == "doAboutApp")					return OnAppAbout( item );
	else if(action == "doShortcuts")				return OnShortcuts( item );
	else if(action == "setLanguage")				return setLanguage( item );

	return false;
}

bool PanelManager::showSidePanel( GUI::ItemPtr item )
{
	BW_GUARD;

	bool isDockVisible = this->panels().isDockVisible();

	if ( !isDockVisible )
	{
		this->panels().showDock( !isDockVisible );
		this->panels().showFloaters( !isDockVisible );
	}
	return true;
}

bool PanelManager::hideSidePanel( GUI::ItemPtr item )
{
	BW_GUARD;

	bool isDockVisible = this->panels().isDockVisible();

	if ( isDockVisible )
	{
		this->panels().showDock( !isDockVisible );
		this->panels().showFloaters( !isDockVisible );
	}
	return true;
}

unsigned int PanelManager::handleGUIUpdate( GUI::ItemPtr item )
{
	BW_GUARD;

	const BW::string& updater = item->updater();;

	if (updater == "updateSidePanel")				return updateSidePanel( item );
	else if (updater == "updateLanguage")			return updateLanguage( item );

	return 0;
}

unsigned int PanelManager::updateSidePanel( GUI::ItemPtr item )
{
	BW_GUARD;

	if ( this->panels().isDockVisible() )
		return 0;
	else
		return 1;
}

void PanelManager::updateControls()
{
	BW_GUARD;

	this->panels().broadcastMessage( WM_UPDATE_CONTROLS, 0, 0 );
}

void PanelManager::onClose()
{
	BW_GUARD;

	if ( Options::getOptionBool( "panels/saveLayoutOnExit", true ) )
	{
		this->panels().save();
		mainFrameWnd_->SaveBarState( L"TBState" );
	}
	this->panels().showDock( false );
	UalManager::instance().fini();
}
BW_END_NAMESPACE

