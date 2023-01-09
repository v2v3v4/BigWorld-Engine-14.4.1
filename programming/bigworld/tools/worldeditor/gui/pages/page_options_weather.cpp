#include "pch.hpp"
#include "worldeditor/gui/pages/page_options_weather.hpp"
#include "worldeditor/gui/pages/panel_manager.hpp"
#include "worldeditor/framework/world_editor_app.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "worldeditor/scripting/we_python_adapter.hpp"
#include "appmgr/options.hpp"
#include "pyscript/py_data_section.hpp"
#include "chunk/chunk_manager.hpp"
#include "chunk/chunk_space.hpp"
#include "chunk/geometry_mapping.hpp"
#include "romp/time_of_day.hpp"
#include "editor_shared/dialogs/file_dialog.hpp"
#include "common/string_utils.hpp"
#include "common/user_messages.hpp"
#include "common/utilities.hpp"
#include "cstdmf/debug.hpp"
#include "cstdmf/Watcher.hpp"
#include "gizmo/gizmo_manager.hpp"
#include "resmgr/xml_section.hpp"
#include <afxpriv.h>


DECLARE_DEBUG_COMPONENT2( "WorldEditor", 0 )

BW_BEGIN_NAMESPACE

namespace
{
	/**
	 *	This method retrieves the weather XML file.  The location of the XML file
	 *	is specified in the weather	python module.
	 *
	 *	@return DataSectionPtr	The global weather systems data section.
	 */
	DataSectionPtr weatherXML()
	{
		BW_GUARD;

		DataSectionPtr ret = NULL;

		PyObject* pModule = PyImport_ImportModule( "Weather" );
		if (pModule != NULL)
		{
			PyObject* pWeatherDS = PyObject_GetAttrString( pModule, "weatherXML" );
			if ( pWeatherDS != NULL )
			{
				MF_ASSERT( PyDataSection::Check(pWeatherDS) );
				ret = static_cast<PyDataSection*>(pWeatherDS)->pSection();
				Py_DECREF( pWeatherDS );
			}	
			Py_DECREF( pModule );
		}
		return ret;
	}

	//-----------------------------------------------------------------------------
	// Section - Weather System Undo/Redo
	//-----------------------------------------------------------------------------

    // This class is for weather undo/redo operations.
    class WeatherUndo : public UndoRedo::Operation
    {
    public:
        WeatherUndo();

        /*virtual*/ void undo();

        /*virtual*/ bool iseq(UndoRedo::Operation const &other) const;

    private:
        XMLSectionPtr       ds_;
    };

	/**
	 * Constructor
	 */
    WeatherUndo::WeatherUndo():
		UndoRedo::Operation((size_t)(typeid(WeatherUndo).name())),
		ds_(new XMLSection("WeatherUndo_undo_redo"))
    {
		BW_GUARD;

		//Take a copy of the entire weather XML file for undo/redo.		
		DataSectionPtr weather = weatherXML();
		ds_->copy(weather);
    }	


	/**
	 * This method undoes the last weather operation.  It replaces the entire weather
	 * data section.
	 */
    /*virtual*/ void WeatherUndo::undo()
    {
		BW_GUARD;

        // Save the current state to the undo/redo stack:
        UndoRedo::instance().add(new WeatherUndo());
		DataSectionPtr weather = weatherXML();
		weather->copy(ds_);
		bool ok = WorldEditorApp::instance().pythonAdapter()->call( "onUndoWeather" );
    }

    /*virtual*/bool WeatherUndo::iseq( UndoRedo::Operation const &other ) const    
    {
        return false;
    }
}


//-----------------------------------------------------------------------------
// Section - Weather System List - Tree View Control notifications
//-----------------------------------------------------------------------------


/**
 * This method is called in response to the weather system rename edit box, and
 * invokes the python script action actRenameWeatherSystem.
 */
/*afx_msg*/ void WeatherSystemsList::OnTvnEndlabeleditWeatherSystemsList(NMHDR *pNMHDR, LRESULT *pResult)
{
	BW_GUARD;

	bool needsRedraw = false;
	
	LPNMTVDISPINFO pTVDispInfo = reinterpret_cast<LPNMTVDISPINFO>(pNMHDR);
	HTREEITEM item = pTVDispInfo->item.hItem;

	if (!item)
	{
		return;
	}

	BW::wstring oldWeatherSystemName = GetItemText( item );	

	if (!pTVDispInfo->item.pszText)
	{
		return;
	}
	
	BW::wstring weatherSystemName = pTVDispInfo->item.pszText;

	/*if (locations_.find(newLocationName) != locations_.end())
	{
		if (::MessageBox( AfxGetApp()->m_pMainWnd->GetSafeHwnd(),
			L("WORLDEDITOR/GUI/PAGE_OPTIONS_NAVIGATION/OVERWRITTEN_LOCATION_MARK_TEXT", newLocationName),
			L("WORLDEDITOR/GUI/PAGE_OPTIONS_NAVIGATION/OVERWRITTEN_LOCATION_MARK_TITLE"),
			MB_YESNO) == IDNO)
			return;

		needsRedraw = true;
	}*/

	SetItemText( item, weatherSystemName.c_str() );
	*pResult = 0;

	WorldEditorApp::instance().pythonAdapter()->callString2( "actRenameWeatherSystem", bw_wtoutf8( oldWeatherSystemName ), bw_wtoutf8( weatherSystemName ) );

	return;
}


/**
 * This method is called in response to double clicking an item in the weather system
 * list, and invokes the rename edit box.
 */
/*afx_msg*/ void WeatherSystemsList::OnNMDblclkWeatherSystemsList(NMHDR *pNMHDR, LRESULT *pResult)
{
	BW_GUARD;

	SetFocus();
	EditLabel( GetSelectedItem() );
	*pResult = 0;
}


/**
 * This method is called in response to pressing a key in the weather system
 * list, and if the key is F2, it invokes the rename edit box.
 */
/*afx_msg*/ void WeatherSystemsList::OnTvnKeydownWeatherSystemsList(NMHDR *pNMHDR, LRESULT *pResult)
{
	BW_GUARD;

	LPNMTVKEYDOWN pTVKeyDown = reinterpret_cast<LPNMTVKEYDOWN>(pNMHDR);
	
	if (pTVKeyDown->wVKey == VK_F2)
	{
		SetFocus();
		EditLabel( GetSelectedItem() );
	}

	*pResult = 0;
}


/**
 * This method is called in response to selecting a new item in the weather system
 * list.  It invokes the python lbnWeatherSystemItemSelect action.
 */
/*afx_msg*/ void WeatherSystemsList::OnLbnSelchangeWeatherSystemsList(NMHDR *pNMHDR, LRESULT *pResult)
{
	BW_GUARD;

	if (!PanelManager::pInstance())
		return;

	GUITABS::Content* content = PanelManager::instance().panels().getContent( PageOptionsWeather::contentID );
	if (content)
	{
		PageOptionsWeather* w = static_cast<PageOptionsWeather*>(content);

		if (!w->refillingWeatherSystems_)
		{
			if (HTREEITEM hItem = GetSelectedItem())
			{
				DWORD_PTR idx = GetItemData(hItem);
				if (WorldEditorApp::instance().pythonAdapter())
				{
					MF_ASSERT( idx <= INT_MAX );
					WorldEditorApp::instance().pythonAdapter()->onListItemSelect(
						"lbnWeatherSystem", (int ) idx );
				}
				w->onSelectWeatherSystem();
			}
		}
	}
}


/**
 * This method is called in response to moving the mouse over the weather
 * system list.  It records the mouse movements so we can later use it
 * for hit testing.
 */
void WeatherSystemsList::OnMouseMove(UINT nFlags, CPoint point)
{
	BW_GUARD;

	mousePos_ = point;
	CTreeCtrl::OnMouseMove(nFlags, point);
}


/**
 * This method is called in response to clicking an in the weather system
 * list.  It implements checking / unchecking of the weather system exclusion
 * check boxes.  It also invokes the python lbnWeatherSystemItemToggleState
 * action.
 */
void WeatherSystemsList::OnNMClick(NMHDR *pNMHDR, LRESULT *pResult)
{
	BW_GUARD;

	CPoint point = mousePos_;
	UINT uFlags;
	HTREEITEM hItem = HitTest(point, &uFlags);

	if ((hItem == NULL) || !( TVHT_ONITEMSTATEICON & uFlags ))
	{
		return;
	}

	unsigned int state = TreeView_GetCheckState( this->GetSafeHwnd(), hItem );
	DWORD_PTR idx = GetItemData(hItem);
	//note - this message is called just before the checkbox state is toggled,
	//so the state retrieved is the old state.  that's why for the negate below.
	bool checked = ( state != 1 );

	if (WorldEditorApp::instance().pythonAdapter())
	{
		MF_ASSERT( idx <= UINT_MAX );
		WorldEditorApp::instance().pythonAdapter()->onListItemToggleState( "lbnWeatherSystem", ( uint32 ) idx, checked );
	}	

	*pResult = 0;
}



BEGIN_MESSAGE_MAP(WeatherSystemsList, CTreeCtrl)
	ON_NOTIFY_REFLECT(TVN_ENDLABELEDIT, &WeatherSystemsList::OnTvnEndlabeleditWeatherSystemsList)
	ON_NOTIFY_REFLECT(NM_DBLCLK, &WeatherSystemsList::OnNMDblclkWeatherSystemsList)
	ON_NOTIFY_REFLECT(TVN_KEYDOWN, &WeatherSystemsList::OnTvnKeydownWeatherSystemsList)
	ON_NOTIFY_REFLECT(TVN_SELCHANGED, &WeatherSystemsList::OnLbnSelchangeWeatherSystemsList)
	ON_WM_MOUSEMOVE()
	ON_NOTIFY_REFLECT(NM_CLICK, &WeatherSystemsList::OnNMClick)
END_MESSAGE_MAP()



//-----------------------------------------------------------------------------
// Section - Weather System Page
//-----------------------------------------------------------------------------


// GUITABS content ID ( declared by the IMPLEMENT_BASIC_CONTENT macro )
const BW::wstring PageOptionsWeather::contentID = L"PageOptionsWeather";


/**
 *	Constructor.
 */
PageOptionsWeather::PageOptionsWeather()
	: WeatherSettingsTable(PageOptionsWeather::IDD),
	pageReady_( false ),
	rclickItem_( NULL ),
	changingWeatherSettings_( false ),
	refillingWeatherSystems_( false ),
	lastWeatherSystemSelection_( NULL )
{
}


/**
 *	This message is an override for the underlying MFC method.
 */
BOOL PageOptionsWeather::PreTranslateMessage( MSG* pMsg )
{
	BW_GUARD;

	//Handle tooltips first...
	CALL_TOOLTIPS( pMsg );
	
	// If edit control is visible in tree view control, when you send a
	// WM_KEYDOWN message to the edit control it will dismiss the edit
	// control. When the ENTER key was sent to the edit control, the
	// parent window of the tree view control is responsible for updating
	// the item's label in TVN_ENDLABELEDIT notification code.
	if (pMsg->message == WM_KEYDOWN &&
		(pMsg->wParam == VK_RETURN || pMsg->wParam == VK_ESCAPE))
	{
		CEdit* edit = weatherSystemsList_.GetEditControl();
		if (edit)
		{
			edit->SendMessage( WM_KEYDOWN, pMsg->wParam, pMsg->lParam );
			return TRUE; // Handled
		}
	}
	return WeatherSettingsTable::PreTranslateMessage( pMsg );
}


/**
 *	This message is an override for the underlying MFC method.
 */
void PageOptionsWeather::DoDataExchange(CDataExchange* pDX)
{
	BW_GUARD;

	WeatherSettingsTable::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_WEATHER_SYSTEMS_LIST, weatherSystemsList_);
	DDX_Control(pDX, IDC_SKY_BOXES_LIST, skyBoxesList_);
}


/**
 *	This message is an override for the underlying MFC method.
 */
BOOL PageOptionsWeather::InitPage()
{
	BW_GUARD;

	INIT_AUTO_TOOLTIP();
	CString name;
	GetWindowText(name);
	if (WorldEditorApp::instance().pythonAdapter())
	{
		WorldEditorApp::instance().pythonAdapter()->onPageControlTabSelect("pgc", bw_wtoutf8( name.GetString() ));
	}

	WeatherSettingsTable::initDragDrop();

	UalManager::instance().dropManager().add(
		new UalDropFunctor< PageOptionsWeather >( &skyBoxesList_, "model", this, &PageOptionsWeather::skyBoxDrop ) );

	// pre allocate 1000 strings of about 16 char per string
	propertyList()->InitStorage(1000, 16);
	refillWeatherSystemsList();
	if (weatherSystemItems_.size()>0)
	{
		bool ok = WorldEditorApp::instance().pythonAdapter()->call( "onInitWeatherUI" );
	}
	weatherSystemsList_.OnLbnSelchangeWeatherSystemsList();

	// Initialise the toolbar
    if (skyBoxesTB_.GetSafeHwnd() == NULL)
    {
        skyBoxesTB_.CreateEx
        (
            this, 
            TBSTYLE_FLAT, 
            WS_CHILD | WS_VISIBLE | CBRS_ALIGN_TOP
        );
        skyBoxesTB_.LoadToolBarEx(IDR_SKYBOXES_TB, IDR_SKYBOXES_DIS_TB);
        skyBoxesTB_.SetBarStyle(CBRS_ALIGN_TOP | CBRS_TOOLTIPS | CBRS_FLYBY);
        skyBoxesTB_.Subclass(IDC_SKYBOXES_TB);
        skyBoxesTB_.ShowWindow(SW_SHOW);
    }

	return true;
}


/**
 *	This message refills the weather system list, it is called in response
 *	to changes such as editing, or entering a new space.
 */
void PageOptionsWeather::refillWeatherSystemsList()
{
	BW_GUARD;

	BW::string space = this->spaceName();
	refillingWeatherSystems_ = true;
	weatherSystemsList_.DeleteAllItems();
	weatherSystemItems_.clear();
	DataSectionPtr pDS = weatherXML();
	if (pDS)
	{	
		DataSection::iterator it = pDS->begin();
		DataSection::iterator en = pDS->end();
		while (it != en)
		{
			DataSectionPtr pSect = *it;
			BW::string name = pSect->sectionName();

			BW::vector<BW::string> excludes;
			pSect->readStrings("exclude", excludes);
			bool included = std::find( excludes.begin(), excludes.end(), space ) == excludes.end();

			BW::vector<BW::string> defaults;
			pSect->readStrings("default", defaults);
			bool isDefault = std::find( defaults.begin(), defaults.end(), space ) != defaults.end();

			if (isDefault)
			{
				name = name + " " + LocaliseUTF8(L"WORLDEDITOR/GUI/PAGE_OPTIONS_WEATHER/WEATHER_DEFAULT");
			}

			HTREEITEM item = weatherSystemsList_.InsertItem( bw_utf8tow( name ).c_str() );
			TreeView_SetCheckState( weatherSystemsList_.GetSafeHwnd(), item, included );
			weatherSystemItems_.push_back(item);
			weatherSystemsList_.SetItemData(item,weatherSystemItems_.size()-1);			
			++it;
		}
	}
	refillingWeatherSystems_ = false;
}


/**
 *	This message returns the data section for the currently selected weahter
 *	system.
 *
 *	@return DataSectionPtr	data section of the currently selected system.
 */
DataSectionPtr PageOptionsWeather::systemDataSection() const
{
	BW_GUARD;

	if (HTREEITEM hItem = weatherSystemsList_.GetSelectedItem())
	{
		DWORD_PTR idx = weatherSystemsList_.GetItemData(hItem);
		DataSectionPtr pDS = weatherXML();
		if (pDS && idx >= 0)
		{
			MF_ASSERT( idx <= INT_MAX );
			return pDS->openChild( ( int ) idx );
		}
	}

	return NULL;
}


/**
 *	This message is called when a weather system is selected in the list pane.
 */
void PageOptionsWeather::onSelectWeatherSystem()
{
	BW_GUARD;

	//Hack : make the selected item bold.  The tree view has no way of forcing
	//the highlight to be a certain colour - it uses the system colours always.
	//In Vista, the non-focus highlight colour appears to be the same colour
	//as the background, thus making it hard to see what is selected when the
	//control has lost focus.  Bold works though.
	HTREEITEM hItem = weatherSystemsList_.GetSelectedItem();
	if (lastWeatherSystemSelection_)
		weatherSystemsList_.SetItem(lastWeatherSystemSelection_, TVIF_STATE, NULL, 0, 0, 0, TVIS_BOLD, 0);
	weatherSystemsList_.SetItem(hItem, TVIF_STATE, NULL, 0, 0, TVIS_BOLD, TVIS_BOLD, 0);
	lastWeatherSystemSelection_ = hItem;

	DataSectionPtr pSystem = this->systemDataSection();

	this->populateSkyBoxes(pSystem);
	this->addPropertyItems(pSystem);
	this->selectSkyBoxByIdx(0);
	this->OnLbnSelchangeSkyBoxList();
}


/**
 *	This method populates the sky boxes list, given a data section describing
 *	a weather system.
 *
 *	@param	DataSectionPtr	DataSection describing the weather system.
 */
void PageOptionsWeather::populateSkyBoxes(DataSectionPtr pSystem)
{
	BW_GUARD;

	skyBoxesList_.ResetContent();

	if (pSystem)
	{
		BW::vector<BW::string> skyBoxes;
		pSystem->readStrings( "skyBox", skyBoxes );
		for (size_t i=0; i<skyBoxes.size(); i++)
		{
			skyBoxesList_.AddString(bw_utf8tow( skyBoxes[i] ).c_str());
		}
	}
}

/**
 *	This message adds property items to the weather settings table.
 *
 *	@param	DataSectionPtr	DataSection describing the weather system.
 */

void PageOptionsWeather::addPropertyItems(DataSectionPtr pSystem)
{
	BW_GUARD;

	WeatherSettingsTable::init( pSystem, true );	
}


BEGIN_MESSAGE_MAP(PageOptionsWeather, WeatherSettingsTable)
	ON_WM_SHOWWINDOW()	
	ON_WM_SIZE()
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, OnToolTipText)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, OnToolTipText)
	ON_MESSAGE(WM_UPDATE_CONTROLS, OnUpdateControls)
	ON_BN_CLICKED(IDC_WEATHER_ADD, OnBnClickedWeatherAdd)
	ON_BN_CLICKED(IDC_WEATHER_RENAME, OnBnClickedWeatherRename)
	ON_BN_CLICKED(IDC_WEATHER_REMOVE, OnBnClickedWeatherRemove)
	ON_BN_CLICKED(IDC_WEATHER_DEFAULT, OnBnClickedWeatherDefault)
	ON_BN_CLICKED(IDC_WEATHER_EXCLUDE, OnBnClickedWeatherExclude)
	ON_BN_CLICKED(IDC_WEATHER_SKYBOX_ADD, OnBnClickedSkyBoxAdd)
	ON_BN_CLICKED(IDC_WEATHER_SKYBOX_CLEAR, OnBnClickedSkyBoxClear)
	ON_LBN_SELCHANGE(IDC_SKY_BOXES_LIST, OnLbnSelchangeSkyBoxList)
	ON_UPDATE_COMMAND_UI(IDC_SKYBOX_UP  , OnSkyBoxUpEnable  )
    ON_UPDATE_COMMAND_UI(IDC_SKYBOX_DOWN, OnSkyBoxDownEnable)
    ON_UPDATE_COMMAND_UI(IDC_SKYBOX_DEL , OnSkyBoxDelEnable )
    ON_COMMAND(IDC_SKYBOX_UP  , OnBnClickedSkyBoxUp  )
    ON_COMMAND(IDC_SKYBOX_DOWN, OnBnClickedSkyBoxDown)
    ON_COMMAND(IDC_SKYBOX_DEL , OnBnClickedSkyBoxDel )
	ON_WM_HSCROLL()	
END_MESSAGE_MAP()



//-----------------------------------------------------------------------------
// Section - PageOptionsWeather message handlers
//-----------------------------------------------------------------------------
/*afx_msg*/ void PageOptionsWeather::OnShowWindow( BOOL bShow, UINT nStatus )
{
	BW_GUARD;

	WeatherSettingsTable::OnShowWindow( bShow, nStatus );

	if ( bShow == FALSE )
	{
	}
	else
	{
		OnUpdateControls( 0, 0 );
	}
}


/*afx_msg*/ void PageOptionsWeather::OnSize( UINT nType, int cx, int cy )
{
	BW_GUARD;

	if (pageReady_)
	{
		// totalSize is the size of the form as designed in the resource editor
		CSize totalSize = GetTotalSize();

		// cx, cy is the current width and height of the available client area.	
		cx = max( cx, (int)totalSize.cx );
		cy = max( cy, (int)totalSize.cy );	

		Utilities::stretchToBottomRight(
			this, *propertyList(),
			cx, 12,
			cy, 5 );

		WeatherSettingsTable::OnSize( nType, cx, cy );
	}
}


/**
 *	This is needed for the toolbar tooltips to work.
 */
BOOL PageOptionsWeather::OnToolTipText(UINT, NMHDR* pNMHDR, LRESULT *result)
{
	BW_GUARD;

    // Allow top level routing frame to handle the message
    if (GetRoutingFrame() != NULL)
        return FALSE;

    TOOLTIPTEXTW *pTTTW = (TOOLTIPTEXTW*)pNMHDR;
    CString cstTipText;
    CString cstStatusText;

    UINT_PTR nID = pNMHDR->idFrom;
    if ( pNMHDR->code == TTN_NEEDTEXTW && (pTTTW->uFlags & TTF_IDISHWND) )
    {
        // idFrom is actually the HWND of the tool
        nID = ((UINT)(WORD)::GetDlgCtrlID((HWND)nID));
    }

    if (nID != 0) // will be zero on a separator
    {
		MF_ASSERT( nID <= UINT_MAX );
        cstTipText.LoadString( (UINT) nID);
        cstStatusText.LoadString( (UINT) nID);
    }

	wcsncpy( pTTTW->szText, cstTipText, ARRAY_SIZE(pTTTW->szText));
    *result = 0;

    // bring the tooltip window above other popup windows
    ::SetWindowPos
    (
        pNMHDR->hwndFrom, 
        HWND_TOP, 
        0, 
        0, 
        0, 
        0,
        SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE
    );

    return TRUE;    // message was handled
}


/*afx_msg*/ LRESULT PageOptionsWeather::OnUpdateControls(WPARAM wParam, LPARAM lParam)
{
	BW_GUARD;

	WeatherSettingsTable::OnUpdateControls( wParam, lParam );

	if ( !IsWindowVisible() )
		return 0;
	
	if ( !pageReady_ )
	{
		InitPage();
		pageReady_ = true;		
		SetRedraw();
	}

    SendMessageToDescendants
    (
        WM_IDLEUPDATECMDUI,
        (WPARAM)TRUE, 
        0, 
        TRUE, 
        TRUE
    );

	return 0;
}


/*afx_msg*/ void PageOptionsWeather::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	BW_GUARD;

	if( !changingWeatherSettings_)
	{
		changingWeatherSettings_ = true;		
		changingWeatherSettings_ = false;
	}

	WeatherSettingsTable::OnHScroll(nSBCode, nPos, pScrollBar);
}


/*afx_msg*/ void PageOptionsWeather::OnLbnSelchangeSkyBoxList()
{
	BW_GUARD;

	int idx = skyBoxesList_.GetCurSel();
	if (WorldEditorApp::instance().pythonAdapter())
	{
		WorldEditorApp::instance().pythonAdapter()->onListItemSelect( "lbnSkyBox", idx );
	}
}


/*afx_msg*/ void PageOptionsWeather::OnBnClickedWeatherRename()
{
	BW_GUARD;

	HTREEITEM hItem = weatherSystemsList_.GetSelectedItem();
	weatherSystemsList_.SetFocus();
	weatherSystemsList_.EditLabel(hItem);
}


/*afx_msg*/  void PageOptionsWeather::OnBnClickedSkyBoxAdd()
{
	BW_GUARD;

    wchar_t * filter = L"Models (*.model)|*.model|All Files (*.*)|*.*||";

    BWFileDialog 
        openDlg
        (
            true,
            L"model",
            NULL,
			BWFileDialog::FD_FILE_PATH_MUST_EXIST,
            filter,
            AfxGetMainWnd()
        );

    if (openDlg.showDialog())
    {        
		BW::string filename = bw_wtoutf8( openDlg.getFileName() );
		StringUtils::replace(filename, BW::string("\\"), BW::string("/"));
		BW::string dissolvedFilename = 
            BWResource::dissolveFilename(filename);
        if (strcmpi(dissolvedFilename.c_str(), filename.c_str()) == 0)
        {
			BW::wstring msg = Localise(L"RCST_IDS_NOTRESPATH");
            AfxMessageBox(msg.c_str());
        }
        else
        {
			CWaitCursor waitCursor;
			bool ok = WorldEditorApp::instance().pythonAdapter()->callString( "actAddSkyBox", dissolvedFilename );            
			if (ok)
			{
				populateSkyBoxes( this->systemDataSection() );
			}
			else
            {
				//TODO : test undo here - do we need to call undo?
                waitCursor.Restore();
				BW::wstring msg = Localise(L"RCST_IDS_NOLOADSKYBOX"); 
                AfxMessageBox(msg.c_str());
            }            
        }
    }
}


/*afx_msg*/ void PageOptionsWeather::OnSkyBoxUpEnable(CCmdUI *cmdui)
{
	BW_GUARD;

    int sel = skyBoxesList_.GetCurSel();
    if (sel != 0 && sel != CB_ERR)
        cmdui->Enable(TRUE);
    else
        cmdui->Enable(FALSE);
}


/*afx_msg*/ void PageOptionsWeather::OnSkyBoxDownEnable(CCmdUI *cmdui)
{
	BW_GUARD;

    int sel = skyBoxesList_.GetCurSel();
    if (sel != skyBoxesList_.GetCount() - 1 && sel != CB_ERR)
        cmdui->Enable(TRUE);
    else
        cmdui->Enable(FALSE);
}


/*afx_msg*/ void PageOptionsWeather::OnSkyBoxDelEnable(CCmdUI *cmdui)
{
	BW_GUARD;

    int sel = skyBoxesList_.GetCurSel();
    if (sel != CB_ERR)
        cmdui->Enable(TRUE);
    else
        cmdui->Enable(FALSE);
}


/**
 *	This method is the callback registered for drag/drop to the sky boxes list.
 */
bool PageOptionsWeather::skyBoxDrop( UalItemInfo* ii ) 
{
	BW_GUARD;

	CWaitCursor waitCursor;
	BW::string dissolvedFilename = BWResource::dissolveFilename( bw_wtoutf8( ii->longText() ) );
	bool ok = WorldEditorApp::instance().pythonAdapter()->callString( "actAddSkyBox", dissolvedFilename );
	if (ok)
	{
		populateSkyBoxes( this->systemDataSection() );
		return true;
	}
	else
    {
		//TODO : test undo here - do we need to call undo?
        waitCursor.Restore();
		BW::wstring msg = Localise(L"RCST_IDS_NOLOADSKYBOX");
        AfxMessageBox(msg.c_str());
		return false;
    }
}


/**
 * This method saves the current undo state.  It should be invoked just
 * before an operation is performed on the underlying weather data section.
 *
 *	@param BW::string	description of the edit about to take place.
 */
void PageOptionsWeather::saveUndoState(const BW::string& description)
{
	BW_GUARD;

    // Add a new undo/redo state:
    UndoRedo::instance().add(new WeatherUndo());
    UndoRedo::instance().barrier(description, false);
}


//-----------------------------------------------------------------------------
// Section - Weather System Editor Python API
//-----------------------------------------------------------------------------


/*static*/ void PageOptionsWeather::refreshWeatherSystemList()
{
	BW_GUARD;

	if (!PanelManager::pInstance())
		return;

	GUITABS::Content* content = PanelManager::instance().panels().getContent( PageOptionsWeather::contentID );
	if (content)
	{
		PageOptionsWeather* w = static_cast<PageOptionsWeather*>(content);
		w->refillWeatherSystemsList();
	}
}

PY_MODULE_STATIC_METHOD( PageOptionsWeather, refreshWeatherSystemList, WorldEditor )


/*static*/ void PageOptionsWeather::refreshSkyBoxList()
{
	BW_GUARD;

	if (!PanelManager::pInstance())
		return;

	GUITABS::Content* content = PanelManager::instance().panels().getContent( PageOptionsWeather::contentID );
	if (content)
	{
		PageOptionsWeather* w = static_cast<PageOptionsWeather*>(content);
		w->populateSkyBoxes( w->systemDataSection() );
	}
}

PY_MODULE_STATIC_METHOD( PageOptionsWeather, refreshSkyBoxList, WorldEditor )


/*static*/ void PageOptionsWeather::selectWeatherSystemByIdx( uint32 idx )
{
	BW_GUARD;

	if (!PanelManager::pInstance())
		return;

	GUITABS::Content* content = PanelManager::instance().panels().getContent( PageOptionsWeather::contentID );
	if (content)
	{
		PageOptionsWeather* w = static_cast<PageOptionsWeather*>(content);
		if (!w->weatherSystemItems_.empty())
		{			
			idx = min(idx, (uint32)w->weatherSystemItems_.size()-1);
			w->weatherSystemsList_.SelectItem( w->weatherSystemItems_[idx] );
		}

		// rely on this function to clear the property list
		// even if there is no weather system, so we call it anyway
		w->onSelectWeatherSystem();
	}
}

PY_MODULE_STATIC_METHOD( PageOptionsWeather, selectWeatherSystemByIdx, WorldEditor )


/*static*/ void PageOptionsWeather::selectSkyBoxByIdx( uint32 idx )
{
	BW_GUARD;

	if (!PanelManager::pInstance())
		return;

	GUITABS::Content* content = PanelManager::instance().panels().getContent( PageOptionsWeather::contentID );
	if (content)
	{
		PageOptionsWeather* w = static_cast<PageOptionsWeather*>(content);
		w->skyBoxesList_.SetCurSel( idx );
	}
}

PY_MODULE_STATIC_METHOD( PageOptionsWeather, selectSkyBoxByIdx, WorldEditor )


/*static*/ void PageOptionsWeather::refillWeatherSystemProperties()
{
	BW_GUARD;

	if (!PanelManager::pInstance())
		return;

	GUITABS::Content* content = PanelManager::instance().panels().getContent( PageOptionsWeather::contentID );
	if (content)
	{
		PageOptionsWeather* w = static_cast<PageOptionsWeather*>(content);
		DataSectionPtr pSystem = w->systemDataSection();
		w->addPropertyItems( pSystem );
	}
}

PY_MODULE_STATIC_METHOD( PageOptionsWeather, refillWeatherSystemProperties, WorldEditor )


/*static*/ BW::string PageOptionsWeather::spaceName()
{
	BW_GUARD;

	ChunkSpacePtr pSpace = ChunkManager::instance().cameraSpace();
	if (pSpace != NULL)
	{
		//Hit this assert probably means the weather panel is being
		//activated before a space has been mapped in.
		MF_ASSERT( !pSpace->getMappings().empty() );
		BW::string sName = pSpace->getMappings().begin()->second->path();
		if ( !sName.empty() && sName[sName.size()-1] == '/' )
			sName = sName.substr(0, sName.size()-1);
		return sName;
	}
	return "";
}

PY_MODULE_STATIC_METHOD( PageOptionsWeather, spaceName, WorldEditor )


/*static*/ void PageOptionsWeather::renameCurrentWeatherSystem()
{
	BW_GUARD;

	if (!PanelManager::pInstance())
		return;

	GUITABS::Content* content = PanelManager::instance().panels().getContent( PageOptionsWeather::contentID );
	if (content)
	{
		PageOptionsWeather* w = static_cast<PageOptionsWeather*>(content);
		HTREEITEM hItem = w->weatherSystemsList_.GetSelectedItem();
		w->weatherSystemsList_.SetFocus();
		w->weatherSystemsList_.EditLabel(hItem);
	}
}

PY_MODULE_STATIC_METHOD( PageOptionsWeather, renameCurrentWeatherSystem, WorldEditor )


/*static*/ void PageOptionsWeather::saveWeatherUndoState( const BW::string& desc )
{
	BW_GUARD;

	if (!PanelManager::pInstance())
		return;

	GUITABS::Content* content = PanelManager::instance().panels().getContent( PageOptionsWeather::contentID );
	if (content)
	{
		PageOptionsWeather* w = static_cast<PageOptionsWeather*>(content);
		w->saveUndoState( desc );
	}
}

PY_MODULE_STATIC_METHOD( PageOptionsWeather, saveWeatherUndoState, WorldEditor )
BW_END_NAMESPACE

