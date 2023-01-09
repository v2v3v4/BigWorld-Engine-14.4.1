#include "pch.hpp"
#include "panel_manager.hpp"
#include "main_frame.hpp"
#include "action_selection.hpp"
#include "particle_editor.hpp"
#include "gui/action_selection.hpp"
#include "guitabs/guitabs.hpp"
#include "appmgr/options.hpp"
#include "common/user_messages.hpp"
#include "common/page_messages.hpp"
#include "resmgr/string_provider.hpp"
#include "ual/ual_manager.hpp"
#include "ual/ual_dialog.hpp"
#include "ual/ual_callback.hpp"
#include "cstdmf/message_box.hpp"
#include "cstdmf/restart.hpp"
#include "editor_shared/pages/gui_tab_content.hpp"

BW_BEGIN_NAMESPACE

BW_SINGLETON_STORAGE( PanelManager )


PanelManager::~PanelManager()
{
}


/*static*/ bool PanelManager::init(CFrameWnd* mainFrame, CWnd* mainView, const wchar_t* layoutFilename)
{
	BW_GUARD;

	PanelManager* manager = new PanelManager();

    instance().m_mainFrame = mainFrame;
    instance().panels().insertDock(mainFrame, mainView);

	if ( !instance().initPanels(layoutFilename) )
		return false;

    return true;
}


/*static*/ void PanelManager::fini()
{
	BW_GUARD;

	if (pInstance() != NULL)
	{
		instance().panels().broadcastMessage(WM_CLOSING_PANELS, 0, 0);

		instance().m_ready = false;
	}

	delete pInstance();
}


bool PanelManager::initPanels(const wchar_t* layoutFilename)
{
	BW_GUARD;

    if (m_ready)
        return false;

    CWaitCursor wait;

    // UAL Setup
    for (int i = 0; i < BWResource::getPathNum(); i++)
    {
        BW::string path = BWResource::getPath(i);
        if ( path.find("particleeditor") != BW::string::npos )
            continue;
        UalManager::instance().addPath(bw_utf8tow( path ));
    }
    UalManager::instance().setConfigFile
    (
        bw_utf8tow( Options::getOptionString
        (
            "ualConfigPath",
            "resources/ual/ual_config.xml" 
        ) )
    );

    UalManager::instance().setItemDblClickCallback
    (
        new UalFunctor1<PanelManager, UalItemInfo*>(pInstance(), &PanelManager::ualItemDblClick)
    );
    UalManager::instance().setStartDragCallback
    (
        new UalFunctor1<PanelManager, UalItemInfo*>(pInstance(), &PanelManager::ualStartDrag)
    );
    UalManager::instance().setUpdateDragCallback
    (
        new UalFunctor1<PanelManager, UalItemInfo*>(pInstance(), &PanelManager::ualUpdateDrag)
    );
    UalManager::instance().setEndDragCallback
    (
        new UalFunctor1<PanelManager, UalItemInfo*>(pInstance(), &PanelManager::ualEndDrag)
    );
    
    this->panels().registerFactory(new UalDialogFactory());
	this->panels().registerFactory( new PageMessagesFactory );

    // Setup the map which is used for python
    m_contentID[L"ActionSelection"] = ActionSelection::contentID;
	m_contentID[L"UAL"] = UalDialog::contentID;
	m_contentID[L"Messages"] = PageMessages::contentID;

    // other panels setup
    this->panels().registerFactory(new ActionSelectionFactory);

    if ( ( (MainFrame*)m_mainFrame )->verifyBarState( L"TBState" ) )
        m_mainFrame->LoadBarState( L"TBState" );

    if (!this->panels().load( layoutFilename ))
    {
        loadDefaultPanels(NULL);
    }

    // Show the particle editor panel by default:
    this->panels().showPanel(ActionSelection::contentID, true);

    // BAn - this is something of a hack - ActionSelection1::OnInitialUpdate is
    // not called.  I do this manually:
    ActionSelection *actionSelection =
            (ActionSelection*)this->panels().getContent
            (
                ActionSelection::contentID
            );

	const BW::list< GUITABS::PanelPtr > & panels = this->panels().dock()->getPanels();
	for( BW::list< GUITABS::PanelPtr >::const_iterator it = panels.begin();
		it != panels.end(); ++it )
	{
		GUITABS::PanelPtr panel = *it;
		const BW::list< GUITABS::TabPtr > & tabs = panel->getTabs();
		for( BW::list< GUITABS::TabPtr >::const_iterator tabIt = tabs.begin();
			tabIt != tabs.end(); ++tabIt )
		{
			GUITABS::TabPtr tab = *tabIt;
			GuiTabContent * guiTabContent = 
				dynamic_cast< GuiTabContent * >( tab->getContent().get() );
			if (guiTabContent != NULL)
			{
				guiTabContent->setMainFrame( ( IMainFrame * ) m_mainFrame );
			}
		}
	}

    finishLoad();

    return true;
}

bool PanelManager::ready()
{
    return m_ready;
}

void PanelManager::showPanel(BW::wstring const &pyID, int show)
{
	BW_GUARD;

    this->panels().showPanel(pyID, show != 0);
}

bool PanelManager::isPanelVisible(BW::wstring const& pyID)
{
	BW_GUARD;

    return this->panels().isContentVisible(pyID);
}

bool PanelManager::showSidePanel(GUI::ItemPtr item)
{
	BW_GUARD;

    bool isDockVisible = this->panels().isDockVisible();

    if (!isDockVisible)
    {
        this->panels().showDock(!isDockVisible);
        this->panels().showFloaters(!isDockVisible);
    }
    return true;
}

bool PanelManager::hideSidePanel(GUI::ItemPtr item)
{
	BW_GUARD;

    bool isDockVisible = this->panels().isDockVisible();

    if (isDockVisible)
    {
        this->panels().showDock(!isDockVisible);
        this->panels().showFloaters(!isDockVisible);
    }
    return true;
}


bool PanelManager::setLanguage(GUI::ItemPtr item)
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
	if (MainFrame::instance()->PotentiallyDirty())
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
		MainFrame::instance()->ForceSave();
		startNewInstance();
		AfxGetApp()->GetMainWnd()->PostMessage( WM_COMMAND, ID_APP_EXIT );
		break;
	case 1:
		Options::setOptionString( "currentLanguage", languageName );
		Options::setOptionString( "currentCountry", countryName );
		MainFrame::instance()->PotentiallyDirty( false );
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


unsigned int PanelManager::updateSidePanel(GUI::ItemPtr item)
{
	BW_GUARD;

	if (this->panels().isContentVisible(ActionSelection::contentID))
        return 0;
    else
        return 1;
}

unsigned int PanelManager::updateUalPanel(GUI::ItemPtr item)
{
	BW_GUARD;

	if (this->panels().isContentVisible(UalDialog::contentID))
        return 0;
    else
        return 1;
}


unsigned int PanelManager::updateMsgsPanel(GUI::ItemPtr item)
{
	BW_GUARD;

	if (this->panels().isContentVisible(PageMessages::contentID))
        return 0;
    else
        return 1;
}


unsigned int PanelManager::updateLanguage(GUI::ItemPtr item)
{
	BW_GUARD;

	if (currentLanguageName_.empty())
	{
		currentLanguageName_ = StringProvider::instance().currentLanguage()->getIsoLangNameUTF8();
		currentCountryName_ = StringProvider::instance().currentLanguage()->getIsoCountryNameUTF8();
	}
	return currentLanguageName_ == (*item)[ "LanguageName" ] && currentCountryName_ == (*item)[ "CountryName" ];
}


void PanelManager::updateControls()
{
	BW_GUARD;

    this->panels().broadcastMessage(WM_UPDATE_CONTROLS, 0, 0);
    ActionSelection *actionSelection =
        (ActionSelection*)this->panels().getContent
        (
            ActionSelection::contentID
        );
    actionSelection->SendMessageToDescendants
    (
        WM_IDLEUPDATECMDUI,
        (WPARAM)TRUE, 
        0, 
        TRUE, 
        TRUE
    );
}

void PanelManager::onClose()
{
	BW_GUARD;

    if (Options::getOptionBool("panels/saveLayoutOnExit", true))
    {
        this->panels().save();
        m_mainFrame->SaveBarState( L"TBState" );
    }
    this->panels().showDock( false );
	UalManager::instance().fini();
}

void PanelManager::ualAddItemToHistory(BW::string filePath)
{
	BW_GUARD;

    // called from python
    BW::string fname    = BWResource::getFilename(filePath).to_string();
    BW::string longText = BWResource::resolveFilename(filePath);
    replace(longText.begin(), longText.end(), '/', '\\');
    UalManager::instance().history().add(AssetInfo(L"FILE", bw_utf8tow( fname ), bw_utf8tow( longText )));
}

bool PanelManager::loadDefaultPanels(GUI::ItemPtr item)
{
	BW_GUARD;

    CWaitCursor wait;
    bool isFirstCall = true;
    if (m_ready)
    {
        if 
        (
            MessageBox
            (
                m_mainFrame->GetSafeHwnd(),
                Localise(L"PARTICLEEDITOR/GUI/PANEL_MANAGER/LOAD_DEFAULT_Q"),
                Localise(L"PARTICLEEDITOR/GUI/PANEL_MANAGER/LOAD_DEFAULT"),
                MB_YESNO | MB_ICONQUESTION 
            ) != IDYES 
        )
        {
            return false;
        }

        m_ready = false;
		UalManager::instance().dropManager().clear();
		isFirstCall = false;
        // already has something in it, so clean up first
        this->panels().removePanels();

        // not first panel load, so rearrange the toolbars
        ((MainFrame*)m_mainFrame)->defaultToolbarLayout();
    }

    this->panels().insertPanel( ActionSelection::contentID, GUITABS::RIGHT );
    GUITABS::PanelHandle bottomPanel = this->panels().insertPanel( UalDialog::contentID, GUITABS::BOTTOM );
	this->panels().insertPanel( PageMessages::contentID, GUITABS::TAB, bottomPanel );

    if (!isFirstCall)
        finishLoad();

    return true;
}

bool PanelManager::loadLastPanels(GUI::ItemPtr item, const wchar_t* defaultLayoutFilename)
{
	BW_GUARD;

    CWaitCursor wait;
    if 
    ( 
        MessageBox
        ( 
            m_mainFrame->GetSafeHwnd(),
            Localise(L"PARTICLEEDITOR/GUI/PANEL_MANAGER/LOAD_RECENT_Q"),
            Localise(L"PARTICLEEDITOR/GUI/PANEL_MANAGER/LOAD_RECENT"),
            MB_YESNO | MB_ICONQUESTION 
        ) != IDYES 
    )
    {
        return false;
    }

    m_ready = false;

    if ( ( (MainFrame*)m_mainFrame )->verifyBarState( L"TBState" ) )
        m_mainFrame->LoadBarState( L"TBState" );

    if (!this->panels().load( defaultLayoutFilename ))
        loadDefaultPanels(NULL);

    finishLoad();

    return true;
}

PanelManager::PanelManager() :
    GUI::ActionMaker<PanelManager   >
    (
        "doShowSidePanel", 
        &PanelManager::showSidePanel
    ),
    GUI::ActionMaker<PanelManager, 1>
    (
        "doHideSidePanel", 
        &PanelManager::hideSidePanel
    ),
    GUI::ActionMaker<PanelManager, 2>
    (
        "setLanguage",
		&PanelManager::setLanguage
    ),
    GUI::UpdaterMaker<PanelManager>
    (
        "updateSidePanel", 
        &PanelManager::updateSidePanel
    ),
	GUI::UpdaterMaker<PanelManager, 1>
    (
        "updateUalPanel", 
        &PanelManager::updateUalPanel
    ),
	GUI::UpdaterMaker<PanelManager, 2>
    (
        "updateMsgsPanel", 
        &PanelManager::updateMsgsPanel
    ),
	GUI::UpdaterMaker<PanelManager, 3>
    (
        "updateLanguage",
		&PanelManager::updateLanguage
    ),
    m_contentID(),
    m_currentTool(-1),
    m_mainFrame(NULL),
    m_mainView(NULL),
    m_ready(false),
    m_dropable(false)
{
}

void PanelManager::finishLoad()
{
	if (this->panels().getContent( PageMessages::contentID ) == NULL)
	{
		// If upgrading to the new PE with page messages, make sure the panel is there
		GUITABS::Content * ualPanel = this->panels().getContent( UalDialog::contentID );
		this->panels().insertPanel( PageMessages::contentID, GUITABS::TAB, ualPanel );
	}

	PageMessages* msgs = (PageMessages*)(this->panels().getContent(PageMessages::contentID ));
	if (msgs)
	{
		msgs->mainFrame( MainFrame::instance() );
	}

	// Make sure the UAL is shown first in its panel
	this->panels().showPanel( UalDialog::contentID, true );

    m_ready = true;
}

void PanelManager::ualItemDblClick(UalItemInfo* ii)
{
	BW_GUARD;

    const BW::string fullname = bw_wtoutf8( ii->longText() );
    BWResource::dissolveFilename(fullname);
    BW::string dir       = BWResource::getFilePath(fullname);
    BW::StringRef file      = BWResource::getFilename(fullname);
    BW::StringRef extension = BWResource::getExtension(fullname);
    if (extension.equals_lower( "xml" ))
    {
        ParticleEditorApp *app = (ParticleEditorApp *)AfxGetApp();
        app->openDirectory(dir);
        file = BWResource::removeExtension(file);
        ActionSelection *as = ActionSelection::instance();
        if (as != NULL)
		{
            as->selectMetaParticleSystem( file.to_string() );
			UalManager::instance().history().add( ii->assetInfo() );
		}
    }
}

void PanelManager::ualStartDrag(UalItemInfo* ii)
{
	BW_GUARD;

    if (ii == NULL)
        return;

    UalManager::instance().dropManager().start
    ( 
        BWResource::getExtension( bw_wtoutf8( ii->longText() ) ).to_string()
    );
}

void PanelManager::ualUpdateDrag(UalItemInfo* ii)
{
	BW_GUARD;

    if (ii == NULL)
        return;

    CWnd *activeView = MainFrame::instance()->GetActiveView();
    if (activeView == NULL)
        return;

    bool inView = 
        ::WindowFromPoint
        (
            CPoint(ii->x(), ii->y())
        ) 
        == 
        activeView->m_hWnd;

    if ((ii->isFolder()) || (UalManager::instance().dropManager().test(ii)))
        SetCursor(AfxGetApp()->LoadStandardCursor(IDC_ARROW));
    else
        SetCursor(AfxGetApp()->LoadStandardCursor(IDC_NO));
}

void PanelManager::ualEndDrag(UalItemInfo* ii)
{
	BW_GUARD;

    SetCursor(AfxGetApp()->LoadStandardCursor(IDC_ARROW));

    if (ii == NULL)
        return;

    if (ii->isFolder())
    {
        // folder drag
        CPoint pt;
        GetCursorPos(&pt);
        AfxGetMainWnd()->ScreenToClient(&pt);
        this->panels().clone
        (
            (GUITABS::Content*)(ii->dialog()),
            pt.x - 5, 
            pt.y - 5 
        );
    }
    else
    {
        UalManager::instance().dropManager().end(ii);
    }
}
BW_END_NAMESPACE

