#include "pch.hpp"
#include <afxpriv.h>
#include <afxwin.h>
#include "page_flora_setting.hpp"
#include "ual_manager.hpp"
#include "romp/flora.hpp"
#include "romp/flora_texture.hpp"
#include "chunk/chunk_manager.hpp"
#include "guimanager/gui_toolbar.hpp"
#include "cstdmf/message_box.hpp" 
#include "resmgr/auto_config.hpp"
#include "common/utilities.hpp"


#ifndef CODE_INLINE
#include "page_flora_setting.ipp"
#endif

DECLARE_DEBUG_COMPONENT2( "WorldEditor", 0 )

BW_BEGIN_NAMESPACE

static const int WND_BORDER_SPACE = 3;
static AutoConfigString s_floraXML( "environment/floraXML" );


////////////////////////////////////// PageFloraSetting/////////////////////////

// GUITABS content ID ( declared by the IMPLEMENT_BASIC_CONTENT macro )
const BW::wstring PageFloraSetting::contentID = L"PageFloraSetting";



BEGIN_MESSAGE_MAP(PageFloraSetting, CFormView)
	ON_WM_SIZE()
	ON_WM_CREATE()
	ON_MESSAGE( WM_UPDATE_CONTROLS, OnUpdateControls )
	ON_MESSAGE( WM_NEW_SPACE, OnNewSpace )
	ON_COMMAND_RANGE( GUI_COMMAND_START, GUI_COMMAND_END, OnGUIManagerCommand )
END_MESSAGE_MAP()


PageFloraSetting::PageFloraSetting():
	GUI::ActionMaker<PageFloraSetting>( "ActionRefresh|ActionNewEcotypy|"
		"ActionMoveUpItem|ActionMoveDownItem|ActionDeleteItem|"
		"ActionTurnOn3DView|ActionTurnOff3DView", 
		&PageFloraSetting::handleGUIAction ),
	GUI::UpdaterMaker<PageFloraSetting>( "Toggle3DView", 
		&PageFloraSetting::toggle3DView ),
	CFormView(PageFloraSetting::IDD),
	floraSettingTree_( this ),
	secondaryWnd_( this ),
	layoutVertical_( false ),
	toggle3DView_( false ),
	changed_( false ),
	initialised_( false )
{
	BW_GUARD;
}


PageFloraSetting::~PageFloraSetting()
{
	BW_GUARD;
}

void PageFloraSetting::DoDataExchange(CDataExchange* pDX)
{
	BW_GUARD;
	CFormView::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_FLORA_SETTING_TREE, floraSettingTree_);
	DDX_Control(pDX, IDC_FLORA_SETTING_RIGHT_CHILD_WND, secondaryWnd_);
}



/**
 *	This is called when a new space is loaded.
 *
 *  @param wparam	Not used.  Here to match a function definition.
 *  @param lparam	Not used.  Here to match a function definition.
 *	@return			0.  Not used.  Here to match a function definition.
 */
/*afx_msg*/ 
LRESULT PageFloraSetting::OnNewSpace(WPARAM wParam, LPARAM lParam )
{
	BW_GUARD;
	if (this->isReady())
	{
		this->reload();// re-populate tree due to the new space
		floraSettingTree_.UpdateWindow();
	}
	return 0;
}


/*afx_msg*/
LRESULT PageFloraSetting::OnUpdateControls( WPARAM wParam, LPARAM lParam )
{
	BW_GUARD;
	if (!IsWindowVisible())
	{
		return 0;
	}

	if (!initialised_)
	{
		this->initPage();
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


int PageFloraSetting::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	BW_GUARD;
	int ret = CFormView::OnCreate( lpCreateStruct );

	secondaryWnd_.Create( _T("STATIC"), NULL, WS_CHILD | WS_VISIBLE,
		CRect(0, 0, 500, 500), this, IDC_FLORA_SETTING_RIGHT_CHILD_WND );
	return ret;
}


/**
 *	This method re-populate the tree, and reset member variables.
 */
void PageFloraSetting::reload()
{
	BW_GUARD;

	toggle3DView_ = false;
	changed_ = false;

	DataSectionPtr pSpaceSettings = 
		ChunkManager::instance().cameraSpace()->enviro().pData();
	if (pSpaceSettings)
	{
		floraXML_ = pSpaceSettings->readString( "flora", s_floraXML );
	}
	else
	{
		floraXML_ = s_floraXML;
	}

	DataSectionPtr pFloraXMLDataSect = 
						BWResource::instance().openSection( floraXML_, false );
	floraSettingTree_.DeleteItem( floraSettingTree_.GetRootItem() );
	floraSettingTree_.populate( pFloraXMLDataSect );

	FloraTexture* pFloraTexture = 
		ChunkManager::instance().cameraSpace()->enviro().flora()->floraTexture();
	if (pFloraTexture)
	{
		pFloraTexture->loadHighLightTexture();
	}
}


/**
 *	This method does initialization for the flora setting panel.
 */
/*virtual*/ void PageFloraSetting::initPage()
{
	BW_GUARD;
	MF_ASSERT( !initialised_ );

	INIT_AUTO_TOOLTIP();
	
	listXmlSectionProvider_ = new ListXmlSectionProvider();
	fileScanProvider_ = new ListFileProvider(
		UalManager::instance().thumbnailManager().postfix() );
	existingResourceProvider_ = new FixedListFileProvider( 
		UalManager::instance().thumbnailManager().postfix()	);

	DataSectionPtr pFloraSettingXMLDataSect = 
		BWResource::openSection( FLORA_SETTING_PAGE_XML );
	floraSettingTree_.init( 
		pFloraSettingXMLDataSect->openSection( "tree_view_items" ) );

	this->reload();

	// Minimum size of a splitter pane
	static const int MIN_SPLITTER_PANE_SIZE = 16;

	// create new splitter
	splitterBar_.setMinRowSize( MIN_SPLITTER_PANE_SIZE );
	splitterBar_.setMinColSize( MIN_SPLITTER_PANE_SIZE );

	CRect rect;
	this->GetClientRect( &rect );
	int id2;
	if ( layoutVertical_ )
	{
		splitterBar_.CreateStatic( this, 2, 1, WS_CHILD );
		id2 = splitterBar_.IdFromRowCol( 1, 0 );

		int idealHeight = rect.Height()/2;
		splitterBar_.SetRowInfo( 0, idealHeight, 1 );
		splitterBar_.SetRowInfo( 1, idealHeight, 1 );
	}
	else
	{
		splitterBar_.CreateStatic( this, 1, 2, WS_CHILD );
		id2 = splitterBar_.IdFromRowCol( 0, 1 );

		int idealWidth = rect.Width()/2;
		splitterBar_.SetColumnInfo( 0, idealWidth, 1 );
		splitterBar_.SetColumnInfo( 1, idealWidth, 1 );
	}


	floraSettingTree_.SetDlgCtrlID( splitterBar_.IdFromRowCol( 0, 0 ) );
	floraSettingTree_.SetParent( &splitterBar_ );


	secondaryWnd_.SetDlgCtrlID( id2 );
	secondaryWnd_.SetParent( &splitterBar_ );
	secondaryWnd_.ShowWindow( SW_SHOW );

	floraSettingTree_.ShowWindow( SW_SHOW );
	splitterBar_.ShowWindow( SW_SHOW );

	loadToolbar( pFloraSettingXMLDataSect->openSection( "Toolbar" ) );

	this->layOut();

	this->assetList().setEventHandler( this );
	this->existingResourceList().setEventHandler( this );


	initialised_ = true;
}


/**
 *	This method loads the GUIMANAGER toolbar used in the dialog for the refresh
 *	and layout buttons.
 *
 *	@param section	Data section corresponding to the "Toolbar" section.
 */
void PageFloraSetting::loadToolbar( DataSectionPtr section )
{
	BW_GUARD;

	if ( !section || !section->countChildren() )
	{
		return;
	}

	for( int i = 0; i < section->countChildren(); ++i )
	{
		GUI::Manager::instance().add( new GUI::Item( section->openChild( i ) ) );
	}

	toolbar_.Create( CCS_NODIVIDER | CCS_NORESIZE | CCS_NOPARENTALIGN |
		TBSTYLE_FLAT | WS_CHILD | WS_VISIBLE | TBSTYLE_TOOLTIPS | CBRS_TOOLTIPS,
		CRect(0,0,1,1), this, 0 );
	toolbar_.SetBitmapSize( CSize( 16, 16 ) );
	toolbar_.SetButtonSize( CSize( 24, 22 ) );

	CToolTipCtrl* tc = toolbar_.GetToolTips();
	if ( tc )
	{
		tc->SetWindowPos( &CWnd::wndTopMost, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE );
	}

	GUI::Toolbar* guiTB = new GUI::Toolbar( "FloraSettingToolbar", toolbar_ );
	GUI::Manager::instance().add( guiTB );

	SIZE tbSize = guiTB->minimumSize();
	toolbar_.SetWindowPos( 0, 0, 0, tbSize.cx, tbSize.cy, SWP_NOMOVE|SWP_NOZORDER );
}


/**
 *	This method is called from outside of the panel, 
 *	ie. when the user select a terrain texture for paint brush.
 *	@param	textureName				the name of the tree view item as a texture item
 */
void PageFloraSetting::selectTextureItem( const BW::string& textureName )
{
	BW_GUARD;
	if (!initialised_)
	{
		return;
	}

	wchar_t wTextureName[256];
	bw_utf8tow( textureName, wTextureName, 256 );
	HTREEITEM hItem = 
		floraSettingTree_.findTextureItem( wTextureName, NULL, NULL, true );
	if (hItem)
	{
		floraSettingTree_.SelectItem( hItem );
	}
}


/**
 *	This method save the tree view into data section
 *	and refresh the flora.
 *	@param	hItem		if not NULL, then high light the visuals under hItem.
 *	@param saveToDisk	save to disk if true.
 *	@return				the data section of "flora.xml"
 */
DataSectionPtr PageFloraSetting::refreshFloraXMLDataSection( 
				HTREEITEM hHighLightItem /*=NULL*/, bool saveToDisk /*=true*/ )
{
	BW_GUARD;
	// Save to flora.xml
	DataSectionPtr pFloraXMLDataSect = 
						BWResource::instance().openSection( floraXML_, false );
	DataSectionPtr pEcotypesDataSect = 
						pFloraXMLDataSect->openSection( "ecotypes", false );
	if (!pEcotypesDataSect)
	{
		// no ecotypes.
		return pFloraXMLDataSect;
	}

	char ecotypesPath[256];
	bw_snprintf( ecotypesPath, sizeof( ecotypesPath ), "%s/ecotypes", floraXML_.c_str());
	BWResource::instance().purge( ecotypesPath, true, pEcotypesDataSect );
	pFloraXMLDataSect->delChild( pEcotypesDataSect );

	if (hHighLightItem != NULL)
	{
		FloraSettingTreeSection *pVisuaSection = 
									floraSettingTree_.section( "visual", true );
		MF_ASSERT( pVisuaSection );
		BW::vector<HTREEITEM> visualItems;
		floraSettingTree_.findChildItemsBySection( hHighLightItem, 
										pVisuaSection, visualItems, false );
		// include itself.
		if (floraSettingTree_.getItemSection( hHighLightItem ) == pVisuaSection)
		{
			visualItems.push_back( hHighLightItem );
		}

		floraSettingTree_.save( pFloraXMLDataSect, &visualItems );
	}
	else
	{
		floraSettingTree_.save( pFloraXMLDataSect );
	}

	if (saveToDisk)
	{
		pFloraXMLDataSect->save();
	}
	return pFloraXMLDataSect;
}


/**
 *	When clicking Refresh button.
 */
bool PageFloraSetting::actionRefresh( GUI::ItemPtr item )
{
	BW_GUARD;
	DataSectionPtr pFloraXMLDataSect = this->refreshFloraXMLDataSection( 
																	NULL, true );
	// Refresh flora generation
	uint32 version = Terrain::TerrainSettings::TERRAIN2_VERSION;
	DataSectionPtr pSpaceSettings = 
				ChunkManager::instance().cameraSpace()->enviro().pData();
	if (pSpaceSettings)
	{
		version = pSpaceSettings->readInt( "terrain/version",
			Terrain::TerrainSettings::TERRAIN2_VERSION );
	} 
	ChunkManager::instance().cameraSpace()->enviro().flora()->init( 
													pFloraXMLDataSect, version );

	changed_ = false;
	return true;
}


/**
 *	When clicking create new ecotype.
 */
bool PageFloraSetting::actionNewEcotypy( GUI::ItemPtr item )
{
	BW_GUARD;
	floraSettingTree_.createNewEcotype();
	return true;
}


/**
 *	This method check if the tree view is
 *	focus and return the current selected tree view item
 */
HTREEITEM PageFloraSetting::currentFocusItem()
{
	BW_GUARD;
	if (this->GetFocus() == &floraSettingTree_)
	{
		return floraSettingTree_.GetSelectedItem();
	}
	return NULL;
}


/**
 *	This method try to move up the current tree view item
 *
 *	@param item		GUIMANAGER item to pass to individual handlers
 *	@return			The result of handled actions. False is no handler is called
 */
bool PageFloraSetting::actionMoveUpItem( GUI::ItemPtr item )
{
	BW_GUARD;
	HTREEITEM hItem = this->currentFocusItem();
	if (hItem != NULL)
	{
		floraSettingTree_.moveItem( hItem, -1 );
	}
	return true;
}


/**
 *	This method try to move down the current tree view item
 *
 *	@param item		GUIMANAGER item to pass to individual handlers
 *	@return			The result of handled actions. False is no handler is called
 */
bool PageFloraSetting::actionMoveDownItem( GUI::ItemPtr item )
{
	BW_GUARD;
	HTREEITEM hItem = this->currentFocusItem();
	if (hItem != NULL)
	{
		floraSettingTree_.moveItem( hItem, 1 );
	}
	return true;
}


/**
 *	This method try to delete the current tree view item
 *
 *	@param item		GUIMANAGER item to pass to individual handlers
 *	@return			The result of handled actions. False is no handler is called
 */
bool PageFloraSetting::actionDeleteItem( GUI::ItemPtr item )
{
	BW_GUARD;
	// If focus is on tree view window, 
	// delete tree view item.
	HTREEITEM hItem = this->currentFocusItem();
	if (hItem != NULL)
	{
		floraSettingTree_.deleteItem( hItem, true );
		return true;
	}
	
	// If focus is on exist resource window,
	// delete the selected items from current
	// active tree view item.
	if (GetFocus() == &this->existingResourceList())
	{
		this->deleteSelectedExistingResource();
	}
	return true;
}


uint32 PageFloraSetting::floraVerticesPerBlock()
{
	BW_GUARD;
	return ChunkManager::instance().cameraSpace()->enviro().flora()->verticesPerBlock();
}


/**
 *	This method try to delete the selected resources in the properties window.
 */
void PageFloraSetting::deleteSelectedExistingResource()
{
	BW_GUARD;
	HTREEITEM hCurItem = floraSettingTree_.GetSelectedItem();
	MF_ASSERT(hCurItem);
	ListFileProviderSection* pSection = dynamic_cast<ListFileProviderSection*>
		(floraSettingTree_.getItemSection( hCurItem ));
	if (pSection)
	{
		bool userConfirmed = false;
		BW::vector<AssetInfo> assets;
		this->fillSelectedItemsIntoAssetsInfo( this->existingResourceList(), assets );
		BW::vector<AssetInfo>::iterator it = assets.begin();
		for (; it != assets.end(); ++it)
		{
			wchar_t text[256];
			pSection->generateItemText( (*it), text, 256, false );
			HTREEITEM hItem = floraSettingTree_.findChildItem( hCurItem, text );
			if (hItem)
			{
				if (!floraSettingTree_.deleteItem( hItem, !userConfirmed ))
				{
					// User selected cancel.
					return;
				}
				userConfirmed = true;
			}
		}
	}
}


/**
 *	This method is called from smartlist, as handle the key event "Delete"
 */
/*virtual*/ void PageFloraSetting::listItemDelete()
{
	BW_GUARD;
	// If focus is on exist resource window,
	// delete the selected items from current
	// active tree view item.
	if (GetFocus() == &this->existingResourceList())
	{
		this->deleteSelectedExistingResource();
	}
}


/**
 *	This method is called before something is about to change.
 */
void PageFloraSetting::preChange()
{
	BW_GUARD;
	// Turn off 3D view first cause the structure will be changed.
	if (toggle3DView_)
	{
		this->actionTurnOff3DView( NULL );
		//update the button.
		GUI::Manager::instance().update();
	}
}


/**
 *	This method is called after something has been changed.
 */
void PageFloraSetting::postChange()
{
	BW_GUARD;
	MF_ASSERT(!toggle3DView_);
	changed_ = true;
	//update undo
	GUI::Manager::instance().update();
}

/**
 *	This method try to turn on the 3D View
 *
 *	@param item		GUIMANAGER item to pass to individual handlers
 *	@return			The result of handled actions. False is no handler is called
 */
bool PageFloraSetting::actionTurnOn3DView( GUI::ItemPtr item )
{
	BW_GUARD;
	if (toggle3DView_)
	{
		return true;
	}
	// If the tree has been changed then we need regenerate flora
	// before doing this.
	if (changed_)
	{
		MsgBox mb( L"", Localise(L"WORLDEDITOR/GUI/PAGE_FLORA_SETTING/SAVE_PROMPT"),
			Localise(L"WORLDEDITOR/GUI/PAGE_FLORA_SETTING/YES") );
		mb.doModal( NULL );
		return true;
	}
	HTREEITEM hItem = this->currentFocusItem();
	if (hItem != NULL)
	{
		floraSettingTree_.highLightVisuals( hItem, true );
	}

	toggle3DView_ = true;
	return true;
}

/**
 *	This method try to turn off the 3D View
 *
 *	@param item		GUIMANAGER item to pass to individual handlers
 *	@return			The result of handled actions. False is no handler is called
 */
bool PageFloraSetting::actionTurnOff3DView( GUI::ItemPtr item )
{
	BW_GUARD;
	if (!toggle3DView_)
	{
		return true;
	}

	MF_ASSERT( !changed_ );
	HTREEITEM hItem = this->currentFocusItem();
	if (hItem != NULL)
	{
		floraSettingTree_.highLightVisuals( hItem, false );
	}
	toggle3DView_ = false;

	return true;
}


/**
 *	This method distributes the GUI action to turn on/off 3D view
 *
 *	@param item		GUIMANAGER item to pass to individual handlers
 */
unsigned int PageFloraSetting::toggle3DView( GUI::ItemPtr item )
{
	BW_GUARD;
	return toggle3DView_ == ( (*item)["highLightFloraIn3DView"] == "On" );
}


/**
 *	This method distributes the GUI action calls to individual handlers
 *
 *	@param item		GUIMANAGER item to pass to individual handlers
 *	@return			The result of handled actions. False is no handler is called
 */
bool PageFloraSetting::handleGUIAction( GUI::ItemPtr item )
{
	BW_GUARD;

	if (item->action() == "ActionRefresh")	
	{
		return	actionRefresh( item );
	}
	else if (item->action() == "ActionNewEcotypy")
	{
		return	actionNewEcotypy( item );
	}
	else if (item->action() == "ActionMoveUpItem")
	{
		return	actionMoveUpItem( item );
	}
	else if (item->action() == "ActionMoveDownItem")
	{
		return	actionMoveDownItem( item );
	}
	else if (item->action() == "ActionDeleteItem")
	{
		return	actionDeleteItem( item );
	}
	else if (item->action() == "ActionTurnOn3DView")
	{
		return	actionTurnOn3DView( item );
	}
	else if (item->action() == "ActionTurnOff3DView")
	{
		return	actionTurnOff3DView( item );
	}

	return false;
}

/**
 *	This event handler simply forwards toolbar events to the GUIMANAGER.
 *
 *	@param nID	Toolbar button id.
 */
void PageFloraSetting::OnGUIManagerCommand(UINT nID)
{
	BW_GUARD;
	GUI::Manager::instance().act( nID );
}


/**
 *	This MFC event handler changes the layout of the dialog's controls to best
 *	fit the window shape and size.
 *
 *	@param nType	MFC resize type
 *	@param cx		New dialog width.
 *	@param cy		New dialog height.
 */
void PageFloraSetting::OnSize( UINT nType, int cx, int cy )
{
	BW_GUARD;
	if (!initialised_)
	{
		return;
	}

	CView::OnSize( nType, cx, cy );
	this->layOut();
}



/**
 *	This method aids in adjusting the size of the splitter bar when the dialog
 *	is resized, in order to avoid completely collapsing a splitter pane.
 */
void PageFloraSetting::layOut()
{
	BW_GUARD;
	// recalc layout and update
	CRect rect;
	toolbar_.GetClientRect( &rect );

	// Split window
	int toolbarHeight = rect.Height() + WND_BORDER_SPACE;
	this->GetClientRect( &rect );

	//Calculate client area, so it scrolls rather than resize if less than the minimum.
	int currentWidth, minimumWidth;
	splitterBar_.GetColumnInfo( 0, currentWidth, minimumWidth );
	int newWidth = currentWidth + secondaryWnd_.minimumWidth();
	int newHeight = secondaryWnd_.minimumHeight();

	//Add offsets around the splitter
	newWidth += WND_BORDER_SPACE * 2;
	newHeight += toolbarHeight + WND_BORDER_SPACE * 2;

	newWidth = rect.Width() > newWidth ? rect.Width() : newWidth;
	newHeight = rect.Height() > newHeight ? rect.Height() : newHeight;

	SIZE newSize = {newWidth, newHeight};
	SetScrollSizes( MM_TEXT, newSize, sizeDefault, sizeDefault );

	Utilities::moveAndSize( splitterBar_, WND_BORDER_SPACE,
			toolbarHeight + WND_BORDER_SPACE,
			newWidth - WND_BORDER_SPACE * 2,
			newHeight - toolbarHeight - WND_BORDER_SPACE * 2 );

	splitterBar_.RecalcLayout();
	floraSettingTree_.RedrawWindow();
	secondaryWnd_.RedrawWindow();
}


/**
 *	This method is called when the user starts dragging an item from the list.
 *
 *	@param index	Index of the dragged item on the list.
 */
void PageFloraSetting::listStartDrag( int index )
{
	BW_GUARD;
	// We only handle drag for asset list, not exist resource list.
	POINT pt;
	GetCursorPos( &pt );
	HWND hwnd = ::WindowFromPoint( pt );
	if ( hwnd != this->assetList().GetSafeHwnd() )
	{
		return;
	}

	if ( index < 0 || index >= this->assetList().GetItemCount() )
	{
		return;
	}

	BW::vector<AssetInfo> assets;
	this->fillSelectedItemsIntoAssetsInfo( this->assetList(), assets );
	this->dragLoop( assets );
}


/**
 *	This method fills in a vector with all the assets selected in the list.
 *
 *	@param listWnd		which smart list window we are working on.
 *	@param assetsInfo	Return vector that will contain the selected assets.
 */
void PageFloraSetting::fillSelectedItemsIntoAssetsInfo(
					SmartListCtrl& listWnd, BW::vector<AssetInfo>& assetsInfo )
{
	BW_GUARD;
	const int MAX_SELECTED_ITEMS = 500;
	int numSel = listWnd.GetSelectedCount();
	if (numSel > MAX_SELECTED_ITEMS)
	{
		numSel = MAX_SELECTED_ITEMS;
		ERROR_MSG( "Dragging too many items, only taking the first %d.\n", 
															MAX_SELECTED_ITEMS );
	}
	assetsInfo.reserve( numSel );
	int item = listWnd.GetNextItem( -1, LVNI_SELECTED );
	while (item > -1 && numSel > 0)
	{
		assetsInfo.push_back( listWnd.getAssetInfo( item ) );
		item = listWnd.GetNextItem( item, LVNI_SELECTED );
		--numSel;
	}
}


/**
 *	This method keeps looping as long as the user is still dragging an item,
 *	updating the item's drag image and checking for drop locations.
 *
 *	@param assetsInfo	List of items being dragged.
 */
void PageFloraSetting::dragLoop( BW::vector<AssetInfo>& assetsInfo )
{
	BW_GUARD;
	if (assetsInfo.empty())
	{
		return;
	}

	POINT pt;
	GetCursorPos( &pt );

	UpdateWindow();
	SetCapture();

	// send at least one update drag message
	this->handleDragMouseMove( assetsInfo, pt, true );

	while (CWnd::GetCapture() == this)
	{
		MSG msg;
		if (!::GetMessage( &msg, NULL, 0, 0 ))
		{
			AfxPostQuitMessage( (int)msg.wParam );
			break;
		}

		if (msg.message == WM_LBUTTONUP)
		{
			// END DRAG
			POINT pt = { (short)LOWORD( msg.lParam ), (short)HIWORD( msg.lParam ) };
			ClientToScreen( &pt );
			bool dragged = this->updateDrag( assetsInfo, pt, true );
			this->stopDrag();

			if (dragged)
			{
				floraSettingTree_.RedrawWindow();
			}
			return;
		}
		else if (msg.message == WM_MOUSEMOVE)
		{
			// UPDATE DRAG
			POINT pt = { (short)LOWORD( msg.lParam ), (short)HIWORD( msg.lParam ) };
			this->handleDragMouseMove( assetsInfo, pt );
		}
		else if (msg.message == WM_KEYUP || msg.message == WM_KEYDOWN)
		{
			if (msg.wParam == VK_ESCAPE)
			{
				break; // CANCEL DRAG
			}

			if (msg.message == WM_KEYUP || !(msg.lParam & 0x40000000))
			{
				// send update messages, but not if being repeated
				this->updateDrag( assetsInfo, pt, false );
			}
		}
		else if (msg.message == WM_RBUTTONDOWN)
		{
			break; // CANCEL DRAG
		}
		else
		{
			DispatchMessage( &msg );
		}
	}
	this->cancelDrag();
}


/**
 *	This method is called frequently from the dragLoop to keep track of the
 *	dragging item's image and to relay drag updates to the appropriate functor.
 *
 *	@param assetsInfo	List of items being dragged.
 *	@param srcPt	Mouse position.
 *	@param isScreenCoords	True if the mouse position is in screen coordinates
 *							or false if it's in client area coordinates.
 */
void PageFloraSetting::handleDragMouseMove( BW::vector<AssetInfo>& assets, 
						const CPoint& srcPt, bool isScreenCoords /*= false*/ )
{
	BW_GUARD;

	CPoint pt = srcPt;
	if (!isScreenCoords)
	{
		ClientToScreen( &pt );
	}
	if (this->assetList().isDragging())
	{
		this->assetList().updateDrag( pt.x, pt.y );
	}
	this->updateDrag( assets, pt, false );
}


/**
 *	This method updates a drag and drop operation, finds out if the mouse is
 *	hovering on top of the list or the tree view, and if so, it lets them know.
 *
 *	@param assetsInfo	List of items being dragged.
 *	@param srcPt	Mouse position in screen.
 *	@param endDrag	True if this is the last update, false otherwise.
 *	return			True if handled, false to continue looking for drop targets.
 */
bool PageFloraSetting::updateDrag( BW::vector<AssetInfo>& assetsInfo,
											const CPoint& srcPt, bool endDrag )
{
	BW_GUARD;
	CPoint pt = srcPt;
	HWND hwnd = ::WindowFromPoint( pt );

	if (hwnd == this->assetList().GetSafeHwnd())
	{
		SetCursor( AfxGetApp()->LoadStandardCursor( IDC_NO ) );
		return true;
	}

	this->assetList().clearDropTarget();

	if (hwnd == floraSettingTree_.GetSafeHwnd())
	{
		this->updateFloraSettingTreeDrag( assetsInfo, srcPt, endDrag );
		return true;
	}

	if (hwnd == this->existingResourceList().GetSafeHwnd())
	{
		this->updateExistingResourceListDrag( assetsInfo, srcPt, endDrag );
		return true;
	}

	SetCursor( AfxGetApp()->LoadStandardCursor( IDC_NO ) );

	if (::IsChild( GetSafeHwnd(), hwnd ))
	{
		return true;
	}

	return false;
}


/**
 *	This method updates a drag and drop operation when the mouse is hovering
 *	over the tree view.
 *
 *	@param assetsInfo	List of items being dragged.
 *	@param srcPt	Mouse position in screen.
 *	@param endDrag	True if this is the last update, false otherwise.
 */
void PageFloraSetting::updateFloraSettingTreeDrag( 
										BW::vector<AssetInfo>& assetsInfo,
										 const CPoint& srcPt, bool endDrag )
{
	BW_GUARD;
	CPoint pt = srcPt;
	floraSettingTree_.ScreenToClient( &pt );
	HTREEITEM  dropItem = floraSettingTree_.HitTest( pt );

	if (dropItem != NULL)
	{
		FloraSettingTreeSection* pSection = 
			floraSettingTree_.getItemSection( dropItem );

		if (pSection)
		{
			if ( pSection->isAssetConsumable( dropItem, assetsInfo ))
			{
				if (!endDrag)
				{
					floraSettingTree_.SelectDropTarget( dropItem );
				}
				else
				{
					pSection->consumeAssets( dropItem, assetsInfo );
				}

				SetCursor( AfxGetApp()->LoadStandardCursor( IDC_ARROW ) );
				floraSettingTree_.UpdateWindow();
				return;
			}
		}
	}

	SetCursor( AfxGetApp()->LoadStandardCursor( IDC_NO ) );
	// nothing consumable.
	floraSettingTree_.SelectDropTarget( NULL );
}


/**
 *	This method updates a drag and drop operation when the mouse is hovering
 *	over the exist resrouce window.
 *
 *	@param assetsInfo	List of items being dragged.
 *	@param srcPt	Mouse position in screen.
 *	@param endDrag	True if this is the last update, false otherwise.
 */
void PageFloraSetting::updateExistingResourceListDrag( 
										BW::vector<AssetInfo>& assetsInfo,
										 const CPoint& srcPt, bool endDrag )
{
	BW_GUARD;
	SetCursor( AfxGetApp()->LoadStandardCursor( IDC_ARROW ) );
	if (endDrag)
	{
		HTREEITEM  dropItem = floraSettingTree_.GetSelectedItem();
		if (dropItem != NULL)
		{
			FloraSettingTreeSection*pSection = 
									floraSettingTree_.getItemSection( dropItem );
			if (pSection)
			{
				if ( pSection->isAssetConsumable( dropItem, assetsInfo ))
				{
					pSection->consumeAssets( dropItem, assetsInfo );
					return;
				}
			}
		}
	}
}


/**
 *	This method is called when the drag & drop operation completes successfully
 *	in which case temporary resources such as drag images are cleared.
 */
void PageFloraSetting::stopDrag()
{
	BW_GUARD;

	if ( this->assetList().isDragging() )
	{
		this->assetList().endDrag();
	}
	this->resetDragDropTargets();
	ReleaseCapture();
}


/**
 *	This method resets any visual feedback elements used during a drag
 *	operation such as highlight rectangles for drop targets.
 */
void PageFloraSetting::resetDragDropTargets()
{
	BW_GUARD;
	floraSettingTree_.SelectDropTarget( 0 );
	floraSettingTree_.SetInsertMark( 0 );
	floraSettingTree_.UpdateWindow();
	this->assetList().clearDropTarget();
}


/**
 *	This method is called when the drag & drop operation is canceled.
 */
void PageFloraSetting::cancelDrag()
{
	BW_GUARD;
	this->stopDrag();
}


/**
 *	This method is called when the tooltip for a list item is needed.
 *
 *	@param index	Index of the clicked item on the list.
 *	@param info		Return string that will contain the tooltip for the item.
 */
/*virtual*/ void PageFloraSetting::listItemToolTip( int index, BW::wstring& info )
{
	BW_GUARD;
	this->secondaryWnd_.listItemToolTip( index, info );
}


/**
 *	This MFC method is overridden to handle special key presses such as Esc.
 *
 *	@param pMsg		Windows message to handle.
 *	@return			TRUE to stop further handling, FALSE to continue handling it.
 */
BOOL PageFloraSetting::PreTranslateMessage( MSG * pMsg )
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
		CEdit* edit = floraSettingTree_.GetEditControl();
		if (edit)
		{
			edit->SendMessage( WM_KEYDOWN, pMsg->wParam, pMsg->lParam );
			return TRUE; // Handled
		}
	}
	return CFormView::PreTranslateMessage( pMsg );
}

BW_END_NAMESPACE

