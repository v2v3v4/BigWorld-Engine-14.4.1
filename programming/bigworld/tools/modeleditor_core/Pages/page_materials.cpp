#include "pch.hpp"
#include "page_materials.hpp"

#include "appmgr/commentary.hpp"
#include "appmgr/options.hpp"
#include "atlimage.h"
#include "common/dxenum.hpp"
#include "common/editor_views.hpp"
#include "common/material_editor.hpp"
#include "common/material_utility.hpp"
#include "common/popup_menu.hpp"
#include "common/user_messages.hpp"
#include "editor_shared/dialogs/file_dialog.hpp"
#include "controls/edit_commit.hpp"
#include "controls/edit_numeric.hpp"
#include "controls/slider.hpp"
#include "controls/user_messages.hpp"
#include "guimanager/gui_functor.hpp"
#include "guimanager/gui_functor_option.hpp"
#include "guimanager/gui_manager.hpp"
#include "guimanager/gui_menu.hpp"
#include "guimanager/gui_toolbar.hpp"
#include "main_frm.h"
#include "me_app.hpp"
#include "me_material_proxies.hpp"
#include "me_module.hpp"
#include "me_shell.hpp"
#include "physics2/material_kinds.hpp"
#include "moo/visual_manager.hpp"
#include "mru.hpp"
#include "new_tint.hpp"

#include "tools/common/delay_redraw.hpp"
#include "tools/common/python_adapter.hpp"
#include "tools/common/string_utils.hpp"
#include "tools/common/utilities.hpp" 

#include "texture_feed.hpp"
#include "ual/ual_manager.hpp"
#include "resmgr/auto_config.hpp"

#include "modeleditor_core/Models/mutant.hpp"

DECLARE_DEBUG_COMPONENT( 0 )

BW_BEGIN_NAMESPACE

static AutoConfigString s_default_fx( "system/defaultShaderPath" );
static AutoConfigString s_default_mfm( "system/defaultMfmPath" );
static AutoConfigString s_light_only_fx( "system/lightOnlyEffect" );

struct PageMaterialsImpl: public SafeReferenceCount
{
	static PageMaterials* s_currPage;
	
	bool ready;

	bool inited;
	
	bool updating;
	int updateCount;
	
	CToolBarCtrl toolbar;

	CTreeCtrl materialTree;

	CStatic materialPreviewRect;
	CImage* materialPreview;

	controls::EditCommit material;
	controls::EditCommit matter;
	controls::EditCommit tint;

	CComboBox fxList;
	CButton fxSel;

	CButton previewCheck;
	CComboBox previewList;

	Moo::VisualPtr previewObject;

	Matrix modelView;
	Matrix materialView;

	Moo::ComplexEffectMaterialPtr currMaterial;

	bool ignoreSelChange;

	BW::vector<StringPair*> matterData;

	HTREEITEM selItem;
	HTREEITEM selParent;

	BW::wstring materialDisplayName;
	BW::wstring materialName;
	BW::wstring matterName;
	BW::wstring tintName;

	GeneralEditorPtr editor;

	int pageWidth;
	bool needsUpdate_;
};

PageMaterials* PageMaterialsImpl::s_currPage = NULL;

// PageMaterials

//ID string required for the tearoff tab manager
const BW::wstring PageMaterials::contentID = L"PageMaterialsID";

PageMaterials::PageMaterials():
	PropertyTable( PageMaterials::IDD )
{
	BW_GUARD;

	pImpl_ = new PageMaterialsImpl;

	pImpl_->ready = false;
	pImpl_->inited = false;
	pImpl_->updating = false;
	pImpl_->updateCount = false;
	pImpl_->ignoreSelChange = false;

	pImpl_->materialDisplayName = L"";
	pImpl_->materialName = L"";
	pImpl_->matterName = L"";
	pImpl_->tintName = L"";

	pImpl_->materialPreview = new CImage();

	pImpl_->currMaterial = NULL;

	pImpl_->previewObject = NULL;

	pImpl_->selItem = NULL;
	pImpl_->selParent = NULL;
	
	pImpl_->s_currPage = this;
	pImpl_->needsUpdate_ = true;

	pImpl_->pageWidth = 0;
}

PageMaterials::~PageMaterials()
{
	BW_GUARD;

	MeApp::instance().mutant()->unregisterModelChangeCallback( this );
	delete pImpl_->materialPreview;
}

/*static*/ PageMaterials* PageMaterials::currPage()
{
	BW_GUARD;

	return PageMaterialsImpl::s_currPage;
}

void PageMaterials::DoDataExchange(CDataExchange* pDX)
{
	BW_GUARD;

	PropertyTable::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_MATERIALS_TREE, pImpl_->materialTree);

	DDX_Control(pDX, IDC_MATERIALS_PREVIEW_RECT, pImpl_->materialPreviewRect);

	DDX_Control(pDX, IDC_MATERIALS_MATERIAL, pImpl_->material);
	DDX_Control(pDX, IDC_MATERIALS_MATTER, pImpl_->matter);
	DDX_Control(pDX, IDC_MATERIALS_TINT, pImpl_->tint);

	DDX_Control(pDX, IDC_MATERIALS_FX_LIST, pImpl_->fxList);
	DDX_Control(pDX, IDC_MATERIALS_FX_SEL, pImpl_->fxSel);

	CRect listRect;

	pImpl_->fxList.GetWindowRect(listRect);
    ScreenToClient (&listRect);
	listRect.bottom += 256;
	pImpl_->fxList.MoveWindow(listRect);
	pImpl_->fxList.SelectString(-1, L"");

	DDX_Control( pDX, IDC_MATERIALS_PREVIEW, pImpl_->previewCheck );
	DDX_Control( pDX, IDC_MATERIALS_PREVIEW_LIST, pImpl_->previewList );

	pImpl_->previewList.GetWindowRect(listRect);
    ScreenToClient (&listRect);
	listRect.bottom += 256;
	pImpl_->previewList.MoveWindow(listRect);
	pImpl_->previewList.SetCurSel(0);

	pImpl_->toolbar.Create( CCS_NODIVIDER | CCS_NORESIZE | CCS_NOPARENTALIGN |
		TBSTYLE_FLAT | WS_CHILD | WS_VISIBLE | TBSTYLE_TOOLTIPS | CBRS_TOOLTIPS,
		CRect(0,0,0,0), this, 0 );

	GUI::Manager::instance().add( new GUI::Toolbar( "MaterialsToolbar", pImpl_->toolbar ) );

	CWnd toolbarPos;
	DDX_Control(pDX, IDC_MATERIALS_TOOLBAR, toolbarPos);
	
	CRect toolbarRect;
    toolbarPos.GetWindowRect (&toolbarRect);
    ScreenToClient (&toolbarRect);

	pImpl_->toolbar.MoveWindow(toolbarRect);

	pImpl_->ready = true;
}

BOOL PageMaterials::OnInitDialog()
{
	BW_GUARD;

	drawMaterialsList();

	UalManager::instance().dropManager().add(
		new UalDropFunctor< PageMaterials >( this, "fx", this, &PageMaterials::changeShaderDrop ) );
	UalManager::instance().dropManager().add(
		new UalDropFunctor< PageMaterials >( this, "mfm", this, &PageMaterials::changeMFMDrop ) );

	UalManager::instance().dropManager().add(
		new UalDropFunctor< PageMaterials >( propertyList(), "bmp", this, &PageMaterials::doDrop, false, &PageMaterials::dropTest ) );
	UalManager::instance().dropManager().add(
		new UalDropFunctor< PageMaterials >( propertyList(), "tga", this, &PageMaterials::doDrop, false, &PageMaterials::dropTest ) );
	UalManager::instance().dropManager().add(
		new UalDropFunctor< PageMaterials >( propertyList(), "jpg", this, &PageMaterials::doDrop, false, &PageMaterials::dropTest ) );
	UalManager::instance().dropManager().add(
		new UalDropFunctor< PageMaterials >( propertyList(), "png", this, &PageMaterials::doDrop, false, &PageMaterials::dropTest ) );
	UalManager::instance().dropManager().add(
		new UalDropFunctor< PageMaterials >( propertyList(), "dds", this, &PageMaterials::doDrop, false, &PageMaterials::dropTest ) );
	UalManager::instance().dropManager().add(
		new UalDropFunctor< PageMaterials >( propertyList(), "texanim", this, &PageMaterials::doDrop, false, &PageMaterials::dropTest ) );

	MeApp::instance().mutant()->registerModelChangeCallback(
		new ModelChangeFunctor< PageMaterials >( this, &PageMaterials::clearCurrMaterial ) );
		
	//Disable everything if necessary
	if (pImpl_->selItem == NULL)
	{
		pImpl_->toolbar.ModifyStyle( 0, WS_DISABLED );
		pImpl_->materialTree.ModifyStyle( 0, WS_DISABLED );
		propertyList()->enable( false );
		Utilities::fieldEnabledState( pImpl_->material, false );
		Utilities::fieldEnabledState( pImpl_->matter, false );
		Utilities::fieldEnabledState( pImpl_->tint, false );
		pImpl_->fxList.ModifyStyle( 0, WS_DISABLED );
		pImpl_->fxSel.ModifyStyle( 0, WS_DISABLED );
		pImpl_->previewCheck.ModifyStyle( 0, WS_DISABLED );
		pImpl_->previewCheck.RedrawWindow();
		pImpl_->previewList.ModifyStyle( 0, WS_DISABLED );
	}
		
	INIT_AUTO_TOOLTIP();

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BEGIN_MESSAGE_MAP(PageMaterials, PropertyTable)

	ON_WM_CREATE()

	ON_WM_SIZE()

	ON_MESSAGE(WM_UPDATE_CONTROLS, OnUpdateControls)

	ON_MESSAGE(WM_CHANGE_PROPERTYITEM, OnChangePropertyItem)
	ON_MESSAGE(WM_DBLCLK_PROPERTYITEM, OnDblClkPropertyItem)
	ON_MESSAGE(WM_RCLK_PROPERTYITEM, OnRightClkPropertyItem)

	ON_COMMAND_RANGE(GUI_COMMAND_START, GUI_COMMAND_END, OnGUIManagerCommand)
	ON_UPDATE_COMMAND_UI_RANGE(GUI_COMMAND_START, GUI_COMMAND_END, OnGUIManagerCommandUpdate)

	ON_NOTIFY(TVN_ITEMEXPANDING, IDC_MATERIALS_TREE, OnTvnItemexpandingMaterialsTree)
	ON_NOTIFY(TVN_SELCHANGED, IDC_MATERIALS_TREE, OnTvnSelchangedMaterialsTree)

	ON_EN_CHANGE(IDC_MATERIALS_MATERIAL, OnEnChangeMaterialsMaterial)
	ON_EN_KILLFOCUS(IDC_MATERIALS_MATERIAL, OnEnKillfocusMaterialsMaterial)

	ON_EN_CHANGE(IDC_MATERIALS_MATTER, OnEnChangeMaterialsMatter)
	ON_EN_KILLFOCUS(IDC_MATERIALS_MATTER, OnEnKillfocusMaterialsMatter)

	ON_EN_CHANGE(IDC_MATERIALS_TINT, OnEnChangeMaterialsTint)
	ON_EN_KILLFOCUS(IDC_MATERIALS_TINT, OnEnKillfocusMaterialsTint)
	
	ON_CBN_SELCHANGE(IDC_MATERIALS_FX_LIST, OnCbnSelchangeMaterialsFxList)
	ON_BN_CLICKED(IDC_MATERIALS_FX_SEL, OnBnClickedMaterialsFxSel)

	
	ON_CBN_SELCHANGE(IDC_MATERIALS_PREVIEW_LIST, OnCbnSelchangeMaterialsPreviewList)
	ON_BN_CLICKED(IDC_MATERIALS_PREVIEW, OnBnClickedMaterialsPreview)
	
	ON_MESSAGE(WM_SHOW_TOOLTIP, OnShowTooltip)
	ON_MESSAGE(WM_HIDE_TOOLTIP, OnHideTooltip)
END_MESSAGE_MAP()

void PageMaterials::OnGUIManagerCommand(UINT nID)
{
	BW_GUARD;

	pImpl_->materialTree.SetFocus();
	pImpl_->s_currPage = this;
	GUI::Manager::instance().act( nID );
}

void PageMaterials::OnGUIManagerCommandUpdate(CCmdUI * cmdUI)
{
	BW_GUARD;

	pImpl_->s_currPage = this;
	if( !cmdUI->m_pMenu )                                                   
		GUI::Manager::instance().update( cmdUI->m_nID );
}

afx_msg LRESULT PageMaterials::OnShowTooltip(WPARAM wParam, LPARAM lParam)
{
	BW_GUARD;

	// TODO:UNICODE:CHECK
	LPTSTR* msg = (LPTSTR*)wParam;
	mainFrame_->setMessageText( *msg );
	return 0;
}

afx_msg LRESULT PageMaterials::OnHideTooltip(WPARAM wParam, LPARAM lParam)
{
	BW_GUARD;

	mainFrame_->setMessageText( L"" );
	return 0;
}

// PageMaterials message handlers

int PageMaterials::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	//We might use this later...
	return 1;
}

void PageMaterials::OnSize(UINT nType, int cx, int cy)
{
	BW_GUARD;

	if (!pImpl_->ready) return;
	
	Utilities::stretchToRight( this, pImpl_->materialTree, cx, 12 );

	PropertyTable::OnSize( nType, cx, cy );

	Utilities::centre( this, pImpl_->materialPreviewRect, cx );

	Utilities::stretchToRight( this, pImpl_->material, cx, 12 );
	Utilities::stretchToRight( this, pImpl_->matter, cx, 12 );
	Utilities::stretchToRight( this, pImpl_->tint, cx, 12 );

	Utilities::stretchToRight( this, pImpl_->fxList, cx, 40 );
	Utilities::moveToRight( this, pImpl_->fxSel, cx, 12 );

	Utilities::stretchToRight( this, pImpl_->previewList, cx, 12 );

	pImpl_->pageWidth = cx; //Save the page width for future reference
}

void PageMaterials::drawMaterialsList()
{
	BW_GUARD;

	HTREEITEM firstItem = NULL;
	
	//Need to scope the DelayRedraw since it interfers with EnsureVisible
	{
		DelayRedraw temp( &(pImpl_->materialTree) );

		//Firstly we need to delete any matter data stored
		BW::vector<StringPair*>::iterator matterDataIt = pImpl_->matterData.begin();
		BW::vector<StringPair*>::iterator matterDataEnd = pImpl_->matterData.end();
		while (matterDataIt != matterDataEnd)
		{
			delete (*matterDataIt);
			matterDataIt++;
		}
		pImpl_->matterData.clear();

		//Now delete the old tree
		pImpl_->ignoreSelChange = true;
		pImpl_->materialTree.DeleteAllItems();
		pImpl_->ignoreSelChange = false;

		TreeRoot* treeRoot = MeApp::instance().mutant()->materialTree();
		
		for (unsigned m=0; m<treeRoot->size(); m++)
		{
			BW::string materialDisplayName = MeApp::instance().mutant()->materialDisplayName( (*treeRoot)[m].first.first );
			
			// TODO:UNICODE: Where is this getting deleted?
			StringPair* matterData = new StringPair( (*treeRoot)[m].first.first, (*treeRoot)[m].first.second );
			
			BW::string displayName = materialDisplayName;
			if (matterData->second != "")
			{
				displayName += " ("+matterData->second+")";
			}
			
			HTREEITEM material = pImpl_->materialTree.InsertItem( bw_utf8tow( displayName ).c_str() );
			pImpl_->materialTree.SetItemData( material, (DWORD_PTR)matterData );
			pImpl_->matterData.push_back( matterData );

			if (firstItem == NULL)
			{
				firstItem = material;
			}

			if ((*treeRoot)[m].second.size() == 0)
			{
				if (pImpl_->materialName == bw_utf8tow( matterData->first ) )
				{
					pImpl_->selItem = material;
				}
				pImpl_->materialTree.SetItemState( material, TVIS_BOLD | TVIS_EXPANDED, TVIS_BOLD | TVIS_EXPANDED );
			}
			else
			{
				pImpl_->materialTree.SetItemState( material, TVIS_EXPANDED, TVIS_EXPANDED );
			}

			BW::string matterName = matterData->second;
			BW::wstring wmatterName = bw_utf8tow( matterName );
			
			for (unsigned a=0; a<(*treeRoot)[m].second.size(); a++)
			{
				if (a == 0)
				{
					HTREEITEM item = pImpl_->materialTree.InsertItem( L"Default", material );
					if ((pImpl_->matterName == wmatterName) && (pImpl_->tintName == L"Default"))
					{
						pImpl_->selItem = item;
					}
					if (MeApp::instance().mutant()->getTintName( matterName ) == "Default")
					{
						pImpl_->materialTree.SetItemState( item, TVIS_BOLD | TVIS_EXPANDED, TVIS_BOLD | TVIS_EXPANDED );
					}
				}
				BW::wstring tintName = bw_utf8tow( (*treeRoot)[m].second[a] );
				HTREEITEM item = pImpl_->materialTree.InsertItem( tintName.c_str(), material );
				if ((pImpl_->matterName == wmatterName) && (pImpl_->tintName == tintName))
				{
					pImpl_->selItem = item;
				}
				if (MeApp::instance().mutant()->getTintName( matterName ) == (*treeRoot)[m].second[a])
				{
					pImpl_->materialTree.SetItemState( item, TVIS_BOLD | TVIS_EXPANDED, TVIS_BOLD | TVIS_EXPANDED );
				}
			}
		}
	}

	if (pImpl_->selItem == NULL)
	{
		pImpl_->selItem = firstItem;
	}

	if (pImpl_->selItem != NULL)
	{
		pImpl_->materialTree.SelectItem( pImpl_->selItem );
		pImpl_->materialTree.EnsureVisible( pImpl_->selItem );
	}
}

afx_msg LRESULT PageMaterials::OnUpdateControls(WPARAM wParam, LPARAM lParam)
{
	BW_GUARD;

	pImpl_->updating = pImpl_->updateCount != MeApp::instance().mutant()->updateCount("Materials");
	pImpl_->updateCount = MeApp::instance().mutant()->updateCount("Materials");

	if (!pImpl_->inited)
	{
		OnInitDialog();

		pImpl_->inited = true;
	}

	if (pImpl_->updating)
	{
		pImpl_->selItem = NULL;
		pImpl_->selParent = NULL;
		
		drawMaterialsList();
	}

	PropertyTable::update();

	bool previewMode = MeModule::instance().materialPreviewMode();
	static bool s_lastPreviewMode = !previewMode;
	if (previewMode != s_lastPreviewMode)
	{
		pImpl_->previewCheck.SetCheck( previewMode ? BST_CHECKED : BST_UNCHECKED );
		s_lastPreviewMode = previewMode;
	}

	if (pImpl_->needsUpdate_)
	{
		OnUpdateMaterialPreview();
		pImpl_->needsUpdate_ = false;
	}

	return 0;
}

void PageMaterials::OnUpdateMaterialPreview()
{
	BW_GUARD;
	BW::BinaryBlockPtr pImage = 
		MaterialPreview::instance().generatePreview(
			bw_wtoutf8( materialName() ),
			bw_wtoutf8( matterName() ),
			bw_wtoutf8( tintName() ) );

	HGLOBAL hGlobal = ::GlobalAlloc( GHND, pImage->len() );
	LPBYTE  lpByte  = (LPBYTE)::GlobalLock( hGlobal );
	memcpy( lpByte, pImage->data(), pImage->len() );
	::GlobalUnlock( hGlobal );
	IStream * pStream = NULL;
	if (SUCCEEDED(::CreateStreamOnHGlobal(hGlobal, FALSE, &pStream ) ))
	{
		pImpl_->materialPreview->Destroy();
		pImpl_->materialPreview->Load(pStream);
		pStream->Release();
	}
	GlobalFree( hGlobal );
	UalManager::instance().thumbnailManager().stretchImage( *(pImpl_->materialPreview), 128, 128, true );
	pImpl_->materialPreviewRect.SetBitmap( (HBITMAP)(*(pImpl_->materialPreview)) );
	
	//Do the centering here to make sure that it is done with the correct size
	Utilities::centre( this, pImpl_->materialPreviewRect, pImpl_->pageWidth );
}

afx_msg LRESULT PageMaterials::OnChangePropertyItem(WPARAM wParam, LPARAM lParam)
{
	BW_GUARD;

	if (lParam)
	{
		BaseView* relevantView = (BaseView*)lParam;
		bool transient = !!wParam;
		relevantView->onChange( transient );
	}

	RedrawWindow();

	return 0;
}

afx_msg LRESULT PageMaterials::OnDblClkPropertyItem(WPARAM wParam, LPARAM lParam)
{
	BW_GUARD;

	if (lParam)
	{
		PropertyItem* relevantView = (PropertyItem*)lParam;
		relevantView->onBrowse();
	}
	
	return 0;
}

afx_msg LRESULT PageMaterials::OnRightClkPropertyItem(WPARAM wParam, LPARAM lParam)
{
	BW_GUARD;

	if (lParam)
	{
		PropertyItem* relevantView = (PropertyItem*)lParam;
		BW::wstring menuOptionsList = relevantView->menuOptions();
		if (menuOptionsList != L"")
		{
			BW::vector<BW::wstring> menuOptions;
			bw_tokenise( menuOptionsList, L";,", menuOptions );

			//Make sure we don't try and expose a material's property to Python
			if (((menuOptions[0] == L"PythonOn") || (menuOptions[0] == L"PythonOff")) &&
				((pImpl_->matterName == L"") || (pImpl_->tintName == L"Default")))
					return 0;

			PopupMenu popup;
			for (unsigned i=1; i<menuOptions.size(); i++)
			{
				if (menuOptions[i] != L"-")	
				{
					popup.addItem( menuOptions[i], i );
				}
				else
				{
					popup.addItem( L"", 0 ); // Use a separator
				}
			}
			int sel = popup.doModal( GetSafeHwnd() );

			if (((menuOptions[0] == L"FeedOff") && (sel == 1)) || // Enable a texture feed name
				((menuOptions[0] == L"FeedOn") && (sel == 1)))
			{
				CTextureFeed textureFeedDlg( relevantView->textureFeed() );
				if (textureFeedDlg.DoModal() == IDOK)
				{
					MeApp::instance().mutant()->materialTextureFeedName(
									bw_wtoutf8( pImpl_->materialName ),
									bw_wtoutf8( pImpl_->matterName ),
									bw_wtoutf8( pImpl_->tintName ),
									relevantView->descName().c_str(),
									bw_wtoutf8( textureFeedDlg.feedName() ) );
					OnTvnSelchangedMaterialsTree( NULL, NULL );
				}
			}
			else if ((menuOptions[0] == L"FeedOn") && (sel == 3)) // Remove the texture feed name
			{
				MeApp::instance().mutant()->materialTextureFeedName(
									bw_wtoutf8( pImpl_->materialName ),
									bw_wtoutf8( pImpl_->matterName ),
									bw_wtoutf8( pImpl_->tintName ),
									relevantView->descName().c_str(),
									"");
				OnTvnSelchangedMaterialsTree( NULL, NULL );
			}
			else if (((menuOptions[0] == L"PythonOn") ||
				(menuOptions[0] == L"PythonOff")) &&
				(sel == 1)) // Toggling exposure to python
			{
				MeApp::instance().mutant()->toggleExposed(
									bw_wtoutf8( pImpl_->matterName ),
									bw_wtoutf8( pImpl_->tintName ),
									relevantView->descName().c_str() );
				OnTvnSelchangedMaterialsTree( NULL, NULL );
			}
		}
	}
	
	return 0;
}

bool PageMaterials::clearCurrMaterial()
{
	BW_GUARD;

	pImpl_->currMaterial = NULL;
	return true;
}

void PageMaterials::OnTvnItemexpandingMaterialsTree(NMHDR *pNMHDR, LRESULT *pResult)
{
	*pResult = 1; //Ignore any expand or collapse requests
}


void PageMaterials::OnTvnSelchangedMaterialsTree(NMHDR *pNMHDR, LRESULT *pResult)
{
	BW_GUARD;

	if (pImpl_->ignoreSelChange) return;

	// This makes sure that if we select the material entry then the current tint is selected
	pImpl_->selItem = pImpl_->materialTree.GetSelectedItem();
	HTREEITEM item = pImpl_->materialTree.GetChildItem( pImpl_->selItem );
	if (item != NULL)
	{
		StringPair matterData = *(StringPair*)(pImpl_->materialTree.GetItemData( pImpl_->selItem ));
		BW::wstring tintName = bw_utf8tow( MeApp::instance().mutant()->getTintName( matterData.second ) );
		while (item != NULL)
		{
			if ( pImpl_->materialTree.GetItemText( item ).GetString() == tintName )
			{
				pImpl_->materialTree.SelectItem( item );
				return;
			}
			item = pImpl_->materialTree.GetNextSiblingItem( item );
		}
	}

	DelayRedraw temp( propertyList() );

	PropTable::table( this );
	
	if (pImpl_->editor)
	{
		pImpl_->editor->expel();
	}

	pImpl_->editor = GeneralEditorPtr(new GeneralEditor(), true);

	HTREEITEM parent = pImpl_->materialTree.GetParentItem( pImpl_->selItem );

	pImpl_->selParent = parent ? parent : pImpl_->selItem;

	item = pImpl_->materialTree.GetChildItem( pImpl_->selParent );

	while (item != NULL)
	{
		pImpl_->materialTree.SetItemState( item, 0, TVIS_BOLD | TVIS_EXPANDED );
		item = pImpl_->materialTree.GetNextSiblingItem( item );
	}

	item = pImpl_->materialTree.GetChildItem( pImpl_->selParent );

	if ((item != NULL) && (pImpl_->selItem != pImpl_->selParent))
	{
		pImpl_->materialTree.SetItemState( pImpl_->selItem, TVIS_BOLD | TVIS_EXPANDED, TVIS_BOLD | TVIS_EXPANDED );
	}
	else
	{
		pImpl_->materialTree.SetItemState( item, TVIS_BOLD | TVIS_EXPANDED, TVIS_BOLD | TVIS_EXPANDED );
	}

	BW::set< Moo::ComplexEffectMaterialPtr > effects;

	StringPair matterData = *(StringPair*)(pImpl_->materialTree.GetItemData( pImpl_->selParent ));
	
	pImpl_->materialDisplayName = bw_utf8tow( MeApp::instance().mutant()->materialDisplayName( matterData.first ) );
	pImpl_->materialName = bw_utf8tow( matterData.first );
	pImpl_->matterName = bw_utf8tow( matterData.second );

	BW::string fxFile;
		
	pImpl_->toolbar.ModifyStyle( WS_DISABLED, 0 );
	pImpl_->materialTree.ModifyStyle( WS_DISABLED, 0 );
	propertyList()->enable( true );
	Utilities::fieldEnabledState( pImpl_->material, true, pImpl_->materialDisplayName );
	
	pImpl_->previewCheck.ModifyStyle( WS_DISABLED, 0 );
	pImpl_->previewCheck.RedrawWindow();
	pImpl_->previewList.ModifyStyle( WS_DISABLED, 0 );
	
	if ( pImpl_->matterName != L"" )
	{
		pImpl_->tintName = parent ? pImpl_->materialTree.GetItemText( pImpl_->selItem ).GetString() : L"Default";
		Moo::ComplexEffectMaterialPtr effect;
		MeApp::instance().mutant()->setDye( bw_wtoutf8( pImpl_->matterName ), bw_wtoutf8( pImpl_->tintName ), effect );
		effects.insert( effect );
		fxFile = MeApp::instance().mutant()->materialShader( bw_wtoutf8( pImpl_->materialName ), bw_wtoutf8( pImpl_->matterName ), bw_wtoutf8( pImpl_->tintName ) );
		
		Utilities::fieldEnabledState( pImpl_->matter, true, pImpl_->matterName );
		Utilities::fieldEnabledState( pImpl_->tint, pImpl_->tintName != L"Default", pImpl_->tintName );
	}
	else
	{
		pImpl_->tintName = L"";
		MeApp::instance().mutant()->getMaterial( bw_wtoutf8( pImpl_->materialName ), effects );
		fxFile = MeApp::instance().mutant()->materialShader( bw_wtoutf8( pImpl_->materialName ), bw_wtoutf8( pImpl_->matterName ), bw_wtoutf8( pImpl_->tintName ) );

		Utilities::fieldEnabledState( pImpl_->matter, false );
		Utilities::fieldEnabledState( pImpl_->tint, false );
	}

	if (fxFile != "")
	{
		MRU::instance().update( "fx", fxFile, true );
		redrawList( pImpl_->fxList, "fx", true );
	}
	else
	{
		pImpl_->fxList.SelectString(-1, Localise(L"MODELEDITOR/PAGES/PAGE_MATERIALS/DONT_RENDER"));
	}

	pImpl_->fxList.ModifyStyle( WS_DISABLED, 0 );
	pImpl_->fxSel.ModifyStyle( WS_DISABLED, 0 );

	DataSectionPtr flagsFile = BWResource::openSection( "resources/flags.xml" );
	DataSectionPtr collisionFlags = flagsFile->openSection( "collisionFlags" );

	DataSectionPtr materialKinds;
	materialKinds = flagsFile->newSection( "materialKinds" );
	materialKinds->writeInt( "(Use Model's Default)", 0 );
	MaterialKinds::instance().populateDataSection( materialKinds );

	BW::set< Moo::ComplexEffectMaterialPtr >::iterator materialIt = effects.begin();
	BW::set< Moo::ComplexEffectMaterialPtr >::iterator materialEnd = effects.end();
	
	ChoiceProperty* pProp;
		
	// First lets add the collision flag for the material

	SmartPointer< MeMaterialFlagProxy > cfp =
		new MeMaterialFlagProxy(
			L"collisionFlags",
			pImpl_->materialName,
			&(pImpl_->matterName),
			&(pImpl_->tintName));
	
	pProp = new ChoiceProperty( LocaliseUTF8( "MODELEDITOR/PAGES/PAGE_MATERIALS/COLLISION" ), cfp.get(), collisionFlags, true );
	pProp->UIDesc( LocaliseUTF8( L"MODELEDITOR/PAGES/PAGE_MATERIALS/COLLISION_DESC" ) );
	pImpl_->editor->addProperty( pProp );

	// Now lets add the material type for the material
	
	SmartPointer< MeMaterialFlagProxy > mkp =
		new MeMaterialFlagProxy(
			L"materialKind",
			pImpl_->materialName,
			&(pImpl_->matterName),
			&(pImpl_->tintName)
			);
	
	pProp = new ChoiceProperty( LocaliseUTF8( "MODELEDITOR/PAGES/PAGE_MATERIALS/KIND" ), mkp.get(), materialKinds );
	pProp->UIDesc( LocaliseUTF8( L"MODELEDITOR/PAGES/PAGE_MATERIALS/KIND_DESC" ) );
	pImpl_->editor->addProperty( pProp );

	if (materialIt != materialEnd)
	{
		pImpl_->currMaterial = *materialIt;
	}
	
	if (pImpl_->previewObject)
    {
        BW::vector<Moo::ComplexEffectMaterialPtr> mats;
        int count = pImpl_->previewObject->collateOriginalMaterials( mats );
        if (count > 0 &&
			materialIt != materialEnd)
        {
            Moo::ComplexEffectMaterialPtr mat = mats[0];
            pImpl_->previewObject->overrideMaterial( mat->identifier(), *(pImpl_->currMaterial) );
        }
    }

	MaterialProperties::beginProperties();
	while (materialIt != materialEnd)
	{
		bool canExposeToScript = (pImpl_->matterName != L"") && (pImpl_->tintName != L"Default");

		MaterialProperties::addMaterialProperties(
						(*materialIt)->baseMaterial(), pImpl_->editor.get(), canExposeToScript, true, this );
		++materialIt;
	}
	MaterialProperties::endProperties();

	pImpl_->editor->elect();

	pImpl_->needsUpdate_ = true;

	GUI::Manager::instance().update(); // Update the materials toolbar

	if (pResult)
		*pResult = 0;
}

Moo::ComplexEffectMaterialPtr PageMaterials::currMaterial()
{
	BW_GUARD;

	return pImpl_->currMaterial;
}

const BW::wstring & PageMaterials::materialName() const
{
	BW_GUARD;

	return pImpl_->materialName;
}

const BW::wstring & PageMaterials::matterName() const
{
	BW_GUARD;

	return pImpl_->matterName;
}

const BW::wstring & PageMaterials::tintName() const
{
	BW_GUARD;

	return pImpl_->tintName;
}

void PageMaterials::tintNew()
{
	BW_GUARD;

	BW::vector< BW::string > tintNames;
	MeApp::instance().mutant()->tintNames( bw_wtoutf8( pImpl_->matterName ), tintNames );
	CNewTint newTintDlg( tintNames );
	if (newTintDlg.DoModal() == IDOK)
	{
		BW::string newMatterName = MeApp::instance().mutant()->newTint( bw_wtoutf8( pImpl_->materialName ), bw_wtoutf8( pImpl_->matterName ), bw_wtoutf8( pImpl_->tintName ),
			newTintDlg.tintName(), newTintDlg.fxFile(), newTintDlg.mfmFile() );
		if (!newMatterName.empty())
		{
			pImpl_->matterName = bw_utf8tow( newMatterName );

			pImpl_->tintName = bw_utf8tow( newTintDlg.tintName() );
		}
	}
}

/*~ function ModelEditor.newTint
 *	@components{ modeleditor }
 *
 *	This function enables ModelEditor's Create Tint dialog.
 */
static PyObject * py_newTint( PyObject * args )
{
	BW_GUARD;

	if (PageMaterials::currPage())
		PageMaterials::currPage()->tintNew();

	Py_RETURN_NONE;
}
PY_MODULE_FUNCTION( newTint, ModelEditor )

void PageMaterials::mfmLoad()
{
	BW_GUARD;

	static wchar_t szFilter[] =	L"All (*.mfm;*.fx)|*.mfm;*.fx|"
										L"MFM (*.mfm)|*.mfm|"
										L"Effect (*.fx)|*.fx||";
	
	BWFileDialog fileDlg ( true, L"", L"", BWFileDialog::FD_FILEMUSTEXIST, szFilter);

	BW::string lastDir;
	if (Options::getOptionInt( "settings/lastNewTintFX", 1 ))
		MRU::instance().getDir("fx", lastDir, s_default_fx );
	else
		MRU::instance().getDir("mfm", lastDir, s_default_mfm );
	BW::wstring wlastDir = bw_utf8tow( lastDir );
	fileDlg.initialDir( wlastDir.c_str() );

	if (fileDlg.showDialog())
	{
		BW::wstring fileName = BWResource::dissolveFilenameW( fileDlg.getFileName() );

		if (BWResource::validPathW( fileName ))
		{
			BW::wstring ext = BWResource::getExtensionW( fileName );

			if (ext == L"fx")
			{
				Options::setOptionInt( "settings/lastNewTintFX", 1 );
				changeShader( fileName );
			}
			else if (ext == L"mfm")
			{
				Options::setOptionInt( "settings/lastNewTintFX", 0 );
				changeMFM( fileName );
			}
			else
			{
				::MessageBox( AfxGetApp()->m_pMainWnd->GetSafeHwnd(),
					Localise(L"MODELEDITOR/PAGES/PAGE_MATERIALS/NOT_FX_MFM"),
					Localise(L"MODELEDITOR/PAGES/PAGE_MATERIALS/INVALID_FILE_TYPE"),
					MB_OK | MB_ICONWARNING );
			}
		}
		else
		{
			::MessageBox( AfxGetApp()->m_pMainWnd->GetSafeHwnd(),
				Localise(L"MODELEDITOR/PAGES/PAGE_MATERIALS/BAD_DIR_MATERIAL"),
				Localise(L"MODELEDITOR/PAGES/PAGE_MATERIALS/UNABLE_RESOLVE_MATERIAL"),
				MB_OK | MB_ICONWARNING );
		}
	}

}

/*~ function ModelEditor.loadMFM
 *	@components{ modeleditor }
 *
 *	This function enables the Open File dialog, which allows an MFM to be loaded.
 */
static PyObject * py_loadMFM( PyObject * args )
{
	BW_GUARD;

	if (PageMaterials::currPage())
		PageMaterials::currPage()->mfmLoad();

	Py_RETURN_NONE;
}
PY_MODULE_FUNCTION( loadMFM, ModelEditor )

void PageMaterials::mfmSave()
{
	BW_GUARD;

	static wchar_t szFilter[] =	L"MFM (*.mfm)|*.mfm||";
	
	BWFileDialog fileDlg (false, L"", L"", BWFileDialog::FD_OVERWRITEPROMPT, szFilter);

	BW::string lastDir;
	MRU::instance().getDir("mfm", lastDir, s_default_mfm );
	BW::wstring wlastDir = bw_utf8tow( lastDir );
	fileDlg.initialDir( wlastDir.c_str() );

	if (fileDlg.showDialog())
	{
		BW::wstring mfmName = BWResource::dissolveFilenameW( fileDlg.getFileName() );

		if (BWResource::validPathW( mfmName ))
		{

			if (MeApp::instance().mutant()->saveMFM( bw_wtoutf8( pImpl_->materialName ), 
													 bw_wtoutf8( pImpl_->matterName ), 
													 bw_wtoutf8( pImpl_->tintName ), 
													 bw_wtoutf8( mfmName ) ) )
			{
				MRU::instance().update( "mfm", bw_wtoutf8( mfmName ), true );
			}
			else
			{
				::MessageBox( AfxGetApp()->m_pMainWnd->GetSafeHwnd(),
					Localise(L"MODELEDITOR/PAGES/PAGE_MATERIALS/ERROR_SAVE_MFM"),
					Localise(L"MODELEDITOR/PAGES/PAGE_MATERIALS/NO_SAVE_MFM"),
					MB_OK | MB_ICONWARNING );
			}
		}
		else
		{
			::MessageBox( AfxGetApp()->m_pMainWnd->GetSafeHwnd(),
				Localise(L"MODELEDITOR/PAGES/PAGE_MATERIALS/BAD_DIR_MFM"),
				Localise(L"MODELEDITOR/PAGES/PAGE_MATERIALS/UNABLE_RESOLVE_MFM"),
				MB_OK | MB_ICONWARNING );
		}
	}
}

/*~ function ModelEditor.saveMFM
 *	@components{ modeleditor }
 *
 *	This function saves the currently selected MFM.
 */
static PyObject * py_saveMFM( PyObject * args )
{
	BW_GUARD;

	if (PageMaterials::currPage())
		PageMaterials::currPage()->mfmSave();

	Py_RETURN_NONE;
}
PY_MODULE_FUNCTION( saveMFM, ModelEditor )

void PageMaterials::tintDelete()
{
	BW_GUARD;

	MeApp::instance().mutant()->deleteTint( bw_wtoutf8( pImpl_->matterName ), bw_wtoutf8( pImpl_->tintName ) );
}
/*~ function ModelEditor.deleteTint
 *	@components{ modeleditor }
 *
 *	This function deletes the currently selected tint.
 */
static PyObject * py_deleteTint( PyObject * args )
{
	BW_GUARD;

	if (PageMaterials::currPage())
		PageMaterials::currPage()->tintDelete();

	Py_RETURN_NONE;
}
PY_MODULE_FUNCTION( deleteTint, ModelEditor )

bool PageMaterials::canTintDelete()
{
	BW_GUARD;

	return (pImpl_->tintName != L"") && (pImpl_->tintName != L"Default");
}

/*~ function ModelEditor.canDeleteTint
 *	@components{ modeleditor }
 *
 *	Checks whether the currently selected tint can be deleted.	
 *
 *	@return	Returns True (1) if the tint can be deleted, False (0) otherwise.
 */
static PyObject * py_canDeleteTint( PyObject * args )
{
	BW_GUARD;

	if (PageMaterials::currPage())
		return PyInt_FromLong( PageMaterials::currPage()->canTintDelete() );
	return PyInt_FromLong( 0 );
}
PY_MODULE_FUNCTION( canDeleteTint, ModelEditor )

void PageMaterials::OnEnChangeMaterialsMaterial()
{
	BW_GUARD;

	CString new_name;
	pImpl_->material.GetWindowText( new_name );

	BW::wstring displayName = new_name;
	if (pImpl_->matterName != L"")
		displayName += L" ("+pImpl_->matterName+L")";

	pImpl_->materialTree.SetItemText( pImpl_->selParent, displayName.c_str() );
}

void PageMaterials::OnEnKillfocusMaterialsMaterial()
{
	BW_GUARD;

	CString new_name_cstr;
	pImpl_->material.GetWindowText( new_name_cstr );
	BW::wstring new_name = new_name_cstr.GetString();

	BW::wstring::size_type first = new_name.find_first_not_of(L" ");
	BW::wstring::size_type last = new_name.find_last_not_of(L" ") + 1;
	if (first != BW::string::npos)
	{
		new_name = new_name.substr( first, last-first );
	}
	else
	{
		BW::wstring displayName = pImpl_->materialDisplayName;
		if (pImpl_->matterName != L"")
			displayName += L" ("+pImpl_->matterName+L")";
	
		pImpl_->materialTree.SetItemText( pImpl_->selParent, displayName.c_str() );
		pImpl_->material.SetWindowText( pImpl_->materialDisplayName.c_str() );
		
		::MessageBox( AfxGetApp()->m_pMainWnd->GetSafeHwnd(),
			Localise(L"MODELEDITOR/PAGES/PAGE_MATERIALS/NO_RENAME_MATERIAL"),
			Localise(L"MODELEDITOR/PAGES/PAGE_MATERIALS/INVALID_MATERIAL_NAME"), MB_OK | MB_ICONERROR );

		return;
	}

	if (MeApp::instance().mutant()->materialName( bw_wtoutf8( pImpl_->materialName ) , bw_wtoutf8( new_name ) ))
	{
		pImpl_->materialDisplayName = new_name;

		BW::wstring displayName = pImpl_->materialDisplayName;
		if (pImpl_->matterName != L"")
			displayName += L" ("+pImpl_->matterName+L")";
	
		pImpl_->materialTree.SetItemText( pImpl_->selParent, displayName.c_str() );
		pImpl_->material.SetWindowText( pImpl_->materialDisplayName.c_str() );
		const int diff = static_cast<int>(last-first);
		pImpl_->material.SetSel( diff, diff, 0 );
	}
	else
	{
		BW::wstring displayName = pImpl_->materialDisplayName;
		if (pImpl_->matterName != L"")
			displayName += L" ("+pImpl_->matterName+L")";
	
		pImpl_->materialTree.SetItemText( pImpl_->selParent, displayName.c_str() );
		pImpl_->material.SetWindowText( pImpl_->materialDisplayName.c_str() );

		::MessageBox( AfxGetApp()->m_pMainWnd->GetSafeHwnd(),
			Localise(L"MODELEDITOR/PAGES/PAGE_MATERIALS/MATERIAL_NAME_USED"),
			Localise(L"MODELEDITOR/PAGES/PAGE_MATERIALS/MATERIAL_NAME_EXISTS"), MB_OK | MB_ICONERROR );
	}
}

void PageMaterials::OnEnChangeMaterialsMatter()
{
	BW_GUARD;

	int first, last;
	pImpl_->matter.GetSel(first,last);
	CString new_name_cstr;
	pImpl_->matter.GetWindowText( new_name_cstr );
	BW::string new_name = bw_wtoutf8( new_name_cstr.GetString() );
	new_name = Utilities::pythonSafeName( new_name );
	pImpl_->matter.SetWindowText( bw_utf8tow( new_name ).c_str() );
	BW::wstring displayName = pImpl_->materialDisplayName + L" ("+bw_utf8tow( new_name )+L")";
	pImpl_->materialTree.SetItemText( pImpl_->selParent, displayName.c_str() );
	pImpl_->matter.SetSel(first,last);
}

void PageMaterials::OnEnKillfocusMaterialsMatter()
{
	BW_GUARD;

	CString new_name_cstr;
	pImpl_->matter.GetWindowText( new_name_cstr );
	BW::wstring new_name = new_name_cstr;

	if (new_name == L"")
	{
		BW::wstring displayName = pImpl_->materialDisplayName + L" ("+pImpl_->matterName+L")";
	
		pImpl_->materialTree.SetItemText( pImpl_->selParent, displayName.c_str() );
		pImpl_->matter.SetWindowText( pImpl_->matterName.c_str() );
		
		::MessageBox( AfxGetApp()->m_pMainWnd->GetSafeHwnd(),
			Localise(L"MODELEDITOR/PAGES/PAGE_MATERIALS/NO_RENAME_DYE"),
			Localise(L"MODELEDITOR/PAGES/PAGE_MATERIALS/INVALID_DYE_NAME"), MB_OK | MB_ICONERROR );
	}
	else if (MeApp::instance().mutant()->matterName( bw_wtoutf8( pImpl_->matterName ) , bw_wtoutf8( new_name ) ))
	{
		StringPair* matterData = (StringPair*)(pImpl_->materialTree.GetItemData( pImpl_->selParent ));
		matterData->first = bw_wtoutf8( pImpl_->materialName );
		matterData->second = bw_wtoutf8( new_name );
		pImpl_->matterName = new_name;
	}
	else
	{
		BW::wstring displayName = pImpl_->materialDisplayName + L" ("+pImpl_->matterName+L")";
	
		pImpl_->materialTree.SetItemText( pImpl_->selParent, displayName.c_str() );
		pImpl_->matter.SetWindowText( pImpl_->matterName.c_str() );

		::MessageBox( AfxGetApp()->m_pMainWnd->GetSafeHwnd(),
			Localise(L"MODELEDITOR/PAGES/PAGE_MATERIALS/DYE_NAME_USED"),
			Localise(L"MODELEDITOR/PAGES/PAGE_MATERIALS/DYE_NAME_EXISTS"), MB_OK | MB_ICONERROR );
	}
}

void PageMaterials::OnEnChangeMaterialsTint()
{
	BW_GUARD;

	CString new_name;
	pImpl_->tint.GetWindowText(new_name);
	pImpl_->materialTree.SetItemText( pImpl_->selItem, new_name );
}

void PageMaterials::OnEnKillfocusMaterialsTint()
{
	BW_GUARD;

	CString new_name_cstr;
	pImpl_->tint.GetWindowText( new_name_cstr );
	BW::wstring new_name = new_name_cstr.GetString();

	BW::wstring::size_type first = new_name.find_first_not_of(L" ");
	BW::wstring::size_type last = new_name.find_last_not_of(L" ") + 1;
	if (first != BW::wstring::npos)
	{
		new_name = new_name.substr( first, last-first );
	}
	else
	{
		pImpl_->materialTree.SetItemText( pImpl_->selItem, pImpl_->tintName.c_str() );
		pImpl_->tint.SetWindowText( pImpl_->tintName.c_str() );

		::MessageBox( AfxGetApp()->m_pMainWnd->GetSafeHwnd(),
			Localise(L"MODELEDITOR/PAGES/PAGE_MATERIALS/NO_RENAME_TINT"),
			Localise(L"MODELEDITOR/PAGES/PAGE_MATERIALS/INVALID_DYE_TINT"), MB_OK | MB_ICONERROR );

		return;
	}
	
	if (MeApp::instance().mutant()->tintName( bw_wtoutf8( pImpl_->matterName ), bw_wtoutf8( pImpl_->tintName ) , bw_wtoutf8( new_name ) ))
	{
		pImpl_->tintName = new_name;

		pImpl_->materialTree.SetItemText( pImpl_->selItem, pImpl_->tintName.c_str() );
		pImpl_->tint.SetWindowText( pImpl_->tintName.c_str() );
		const int diff = static_cast<int>(last-first);
		pImpl_->tint.SetSel( diff, diff, 0 );
	}
	else
	{
		pImpl_->materialTree.SetItemText( pImpl_->selItem, pImpl_->tintName.c_str() );
		pImpl_->tint.SetWindowText( pImpl_->tintName.c_str() );

		::MessageBox( AfxGetApp()->m_pMainWnd->GetSafeHwnd(),
			Localise(L"MODELEDITOR/PAGES/PAGE_MATERIALS/TINT_NAME_USED"),
			Localise(L"MODELEDITOR/PAGES/PAGE_MATERIALS/TINT_NAME_EXISTS"), MB_OK | MB_ICONERROR );
	}
}

void PageMaterials::redrawList( CComboBox& list, const BW::string& name, bool sel )
{
	BW_GUARD;

	BW::vector<BW::string> data;
	MRU::instance().read( name, data );
	list.ResetContent();
	for (unsigned i=0; i<data.size(); i++)
	{
		BW::string::size_type first = data[i].rfind("/") + 1;
		BW::string::size_type last = data[i].rfind(".");
		BW::string dataName = data[i].substr( first, last-first );
		list.InsertString( i, bw_utf8tow( dataName ).c_str() );
	}
	list.InsertString( static_cast<int>(data.size()), 
					Localise(L"MODELEDITOR/PAGES/PAGE_MATERIALS/DONT_RENDER") );
	list.InsertString( static_cast<int>(data.size() + 1), 
					Localise(L"MODELEDITOR/OTHER") );
	list.SetCurSel( sel ? 0 : -1 );
}


void PageMaterials::OnCbnSelchangeMaterialsFxList()
{
	BW_GUARD;

	if (pImpl_->fxList.GetCurSel() == pImpl_->fxList.GetCount() - 1) // (Other...)
	{
		redrawList( pImpl_->fxList, "fx", true );
		OnBnClickedMaterialsFxSel();
		return;
	}
	else if (pImpl_->fxList.GetCurSel() == pImpl_->fxList.GetCount() - 2) // (Don't Render)
	{
		MeApp::instance().mutant()->materialShader( bw_wtoutf8( pImpl_->materialName ), bw_wtoutf8( pImpl_->matterName ), bw_wtoutf8( pImpl_->tintName ), "" );
		return;
	}
	
	BW::vector<BW::string> fx;
	MRU::instance().read( "fx", fx );
	changeShader( bw_utf8tow( fx[ pImpl_->fxList.GetCurSel() ] ) );
}

void PageMaterials::OnBnClickedMaterialsFxSel()
{
	BW_GUARD;

	static wchar_t BASED_CODE szFilter[] =	L"Effect (*.fx)|*.fx||";
	BWFileDialog::FDFlags flags = ( BWFileDialog::FDFlags )
		( BWFileDialog::FD_HIDEREADONLY | BWFileDialog::FD_FILEMUSTEXIST );
	BWFileDialog fileDlg (true, L"", L"", flags, szFilter);

	BW::string fxDir;
	MRU::instance().getDir("fx", fxDir, s_default_fx );
	BW::wstring wfxDir = bw_utf8tow( fxDir );
	fileDlg.initialDir( wfxDir.c_str() );

	if (fileDlg.showDialog())
	{
		BW::wstring fxFile = BWResource::dissolveFilenameW( fileDlg.getFileName() );

		if (BWResource::validPathW( fxFile ))
		{
			changeShader( fxFile );
		}
		else
		{
			::MessageBox( AfxGetApp()->m_pMainWnd->GetSafeHwnd(),
				Localise(L"MODELEDITOR/PAGES/PAGE_MATERIALS/BAD_DIR_EFFECT"),
				Localise(L"MODELEDITOR/PAGES/PAGE_MATERIALS/UNABLE_RESOLVE_EFFECT"),
				MB_OK | MB_ICONWARNING );
		}
	}
}

bool PageMaterials::changeShader( const BW::wstring& fxFile )
{
	BW_GUARD;

	if ( MeApp::instance().mutant()->materialShader( bw_wtoutf8( pImpl_->materialName ), bw_wtoutf8( pImpl_->matterName ), bw_wtoutf8( pImpl_->tintName ), bw_wtoutf8( fxFile )))
	{
		MRU::instance().update( "fx", bw_wtoutf8( fxFile ), true );
		redrawList( pImpl_->fxList, "fx", true );

		return true;
	}

	BW::string oldFxFile = MeApp::instance().mutant()->materialShader( bw_wtoutf8( pImpl_->materialName ), bw_wtoutf8( pImpl_->matterName ), bw_wtoutf8( pImpl_->tintName ) );
	if (oldFxFile != "")
	{
		redrawList( pImpl_->fxList, "fx", true );
	}
	else
	{
		pImpl_->fxList.SelectString(-1, Localise(L"MODELEDITOR/PAGES/PAGE_MATERIALS/DONT_RENDER"));
	}

	return false;
}

bool PageMaterials::changeShaderDrop( UalItemInfo* ii ) 
{
	BW_GUARD;

	return changeShader( BWResource::dissolveFilenameW( ii->longText() ));
}

bool PageMaterials::changeMFM( const BW::wstring& mfmFile )
{
	BW_GUARD;

	BW::string fxFile;
	
	if ( MeApp::instance().mutant()->materialMFM( bw_wtoutf8( pImpl_->materialName ), 
												  bw_wtoutf8( pImpl_->matterName ), 
												  bw_wtoutf8( pImpl_->tintName ), 
												  bw_wtoutf8( mfmFile ), &fxFile ))
	{
		MRU::instance().update( "mfm", bw_wtoutf8( mfmFile ), true );		
		MRU::instance().update( "fx", fxFile, true );
		redrawList( pImpl_->fxList, "fx", true );

		return true;
	}
	
	BW::string oldFxFile = MeApp::instance().mutant()->materialShader( bw_wtoutf8( pImpl_->materialName ), bw_wtoutf8( pImpl_->matterName ), bw_wtoutf8( pImpl_->tintName ) );
	if (oldFxFile != "")
	{
		redrawList( pImpl_->fxList, "fx", true );
	}
	else
	{
		pImpl_->fxList.SelectString(-1, Localise(L"MODELEDITOR/PAGES/PAGE_MATERIALS/DONT_RENDER"));
	}

	return false;
}

bool PageMaterials::changeMFMDrop( UalItemInfo* ii ) 
{
	BW_GUARD;

	return changeMFM( BWResource::dissolveFilenameW( ii->longText() ));
}

RectInt PageMaterials::dropTest( UalItemInfo* ii )
{
	BW_GUARD;

	CRect rect = propertyList()->dropTest( CPoint( ii->x(), ii->y() ),
		BWResource::dissolveFilenameW( ii->longText() ) );
	return RectInt( rect.left, rect.bottom, rect.right, rect.top );
}

bool PageMaterials::doDrop( UalItemInfo* ii )
{
	BW_GUARD;

	return propertyList()->doDrop( CPoint( ii->x(), ii->y() ),
		BWResource::dissolveFilenameW( ii->longText() ) );
}

void PageMaterials::OnCbnSelchangeMaterialsPreviewList()
{
	BW_GUARD;

	static BW::vector< BW::string > models;
	if (models.empty())
	{
		models.push_back("resources/models/sphere.visual");
		models.push_back("resources/models/cube.visual");
		models.push_back("resources/models/room.visual");
		models.push_back("resources/models/torus.visual");
		models.push_back("resources/models/teapot.visual");
	}

	static Moo::VisualPtr s_lastGoodObject = pImpl_->previewObject;
	static int s_lastGoodSel = -1;
	
	BW::string name = models[ pImpl_->previewList.GetCurSel() ];
	pImpl_->previewObject = Moo::VisualManager::instance()->get( name );
    if (!pImpl_->previewObject)
    {
        ERROR_MSG( "Couldn't load preview object \"%s\"\n", name.c_str() );
		if (s_lastGoodObject)
		{
			//Return to the last good preview object
			pImpl_->previewObject = s_lastGoodObject;
			pImpl_->previewList.SetCurSel( s_lastGoodSel );
		}
		else
		{
			//Disable the preview mode
			pImpl_->previewCheck.SetCheck( BST_UNCHECKED );
			MeModule::instance().materialPreviewMode( false );
			return;
		}
    }

	//Save these "good" settings now
	s_lastGoodObject = pImpl_->previewObject;
	s_lastGoodSel = pImpl_->previewList.GetCurSel();

	BW::vector<Moo::ComplexEffectMaterialPtr> mats;
	int count = pImpl_->previewObject->collateOriginalMaterials( mats );
	if ( count > 0 )
	{
		Moo::ComplexEffectMaterialPtr mat = mats[0];
		if (pImpl_->currMaterial)
			pImpl_->previewObject->overrideMaterial( mat->baseMaterial()->identifier(), *(pImpl_->currMaterial) );
	}

	if (MeModule::instance().materialPreviewMode())
	{
		MeApp::instance().camera()->boundingBox(
			pImpl_->previewObject->boundingBox());
		MeApp::instance().camera()->zoomToExtents( false );
	}
}

void PageMaterials::OnBnClickedMaterialsPreview()
{
	BW_GUARD;

	static bool s_inited = false;
	if (!s_inited)
	{
		pImpl_->modelView = MeApp::instance().camera()->view();
		pImpl_->materialView = MeApp::instance().camera()->view();
		s_inited = true;
	}

	OnCbnSelchangeMaterialsPreviewList();
	
	bool previewMode = pImpl_->previewCheck.GetCheck() == BST_CHECKED;

	//Make sure we can only go into preview mode if there is a valid preview object
	previewMode = previewMode && pImpl_->previewObject;

	pImpl_->previewCheck.SetCheck( previewMode ? BST_CHECKED : BST_UNCHECKED);

	MeModule::instance().materialPreviewMode( previewMode );

	if (previewMode)
	{
		pImpl_->modelView = MeApp::instance().camera()->view();
		MeApp::instance().camera()->view( pImpl_->materialView );
		MeApp::instance().camera()->boundingBox(
			pImpl_->previewObject->boundingBox());
		MeApp::instance().camera()->zoomToExtents( false );
	}
	else
	{
		pImpl_->materialView = MeApp::instance().camera()->view();
		MeApp::instance().camera()->view( pImpl_->modelView );
		MeApp::instance().camera()->boundingBox(
			MeApp::instance().mutant()->zoomBoundingBox());
	}
}

Moo::VisualPtr PageMaterials::previewObject()
{
	BW_GUARD;

	return pImpl_->previewObject;
}

void PageMaterials::restoreView()
{
	BW_GUARD;

	if (MeModule::instance().materialPreviewMode())
	{
		MeApp::instance().camera()->view( pImpl_->modelView );
		MeApp::instance().camera()->boundingBox(
			MeApp::instance().mutant()->zoomBoundingBox());
	}
}


void PageMaterials::proxySetCallback()
{
	BW_GUARD;

	pImpl_->needsUpdate_ = true;
	// Make sure the actual model's tints and dyes are updated.
	MeApp::instance().mutant()->updateTintFromEffect(
			bw_wtoutf8( pImpl_->matterName ), bw_wtoutf8( pImpl_->tintName ) );
}


BW::string PageMaterials::textureFeed( const BW::string& descName ) const
{
	BW_GUARD;

	return MeApp::instance().mutant()->materialTextureFeedName(
					bw_wtoutf8( pImpl_->materialName ),
					bw_wtoutf8( pImpl_->matterName ),
					bw_wtoutf8( pImpl_->tintName ), descName );
}


BW::string PageMaterials::defaultTextureDir() const
{
	BW_GUARD;

	return MeApp::instance().mutant()->modelName();
}


BW::string PageMaterials::exposedToScriptName( const BW::string& descName ) const
{
	BW_GUARD;

	return MeApp::instance().mutant()->exposedToScriptName(
					bw_wtoutf8( pImpl_->matterName ),
					bw_wtoutf8( pImpl_->tintName ), descName );
}


StringProxyPtr PageMaterials::textureProxy( BaseMaterialProxyPtr proxy,
	GetFnTex getFn, SetFnTex setFn, const BW::string& uiName, const BW::string& descName ) const
{
	BW_GUARD;

	return new MeMaterialTextureProxy( proxy,
					getFn, setFn, uiName,
					bw_wtoutf8( pImpl_->materialName ),
					bw_wtoutf8( pImpl_->matterName ),
					bw_wtoutf8( pImpl_->tintName ), descName );

}


BoolProxyPtr PageMaterials::boolProxy( BaseMaterialProxyPtr proxy,
	GetFnBool getFn, SetFnBool setFn, const BW::string& uiName, const BW::string& descName ) const
{
	BW_GUARD;

	return new MeMaterialBoolProxy( proxy,
					getFn, setFn, uiName,
					bw_wtoutf8( pImpl_->materialName ),
					bw_wtoutf8( pImpl_->matterName ),
					bw_wtoutf8( pImpl_->tintName ), descName );
}


IntProxyPtr PageMaterials::enumProxy( BaseMaterialProxyPtr proxy,
	GetFnEnum getFn, SetFnEnum setFn, const BW::string& uiName, const BW::string& descName ) const
{
	BW_GUARD;

	return new MeMaterialEnumProxy( proxy,
					getFn, setFn, uiName,
					bw_wtoutf8( pImpl_->materialName ),
					bw_wtoutf8( pImpl_->matterName ),
					bw_wtoutf8( pImpl_->tintName ), descName );
}


IntProxyPtr PageMaterials::intProxy( BaseMaterialProxyPtr proxy,
	GetFnInt getFn, SetFnInt setFn, RangeFnInt rangeFn, const BW::string& uiName, const BW::string& descName ) const
{
	BW_GUARD;

	return new MeMaterialIntProxy( proxy,
					getFn, setFn, rangeFn, uiName,
					bw_wtoutf8( pImpl_->materialName ),
					bw_wtoutf8( pImpl_->matterName ),
					bw_wtoutf8( pImpl_->tintName ), descName );
}


FloatProxyPtr PageMaterials::floatProxy( BaseMaterialProxyPtr proxy,
	GetFnFloat getFn, SetFnFloat setFn, RangeFnFloat rangeFn, const BW::string& uiName, const BW::string& descName ) const
{
	BW_GUARD;

	return new MeMaterialFloatProxy( proxy,
					getFn, setFn, rangeFn, uiName,
					bw_wtoutf8( pImpl_->materialName ),
					bw_wtoutf8( pImpl_->matterName ),
					bw_wtoutf8( pImpl_->tintName ), descName );
}


Vector4ProxyPtr PageMaterials::vector4Proxy( BaseMaterialProxyPtr proxy,
	GetFnVec4 getFn, SetFnVec4 setFn, const BW::string& uiName, const BW::string& descName ) const
{
	BW_GUARD;

	return new MeMaterialVector4Proxy( proxy,
					getFn, setFn, uiName,
					bw_wtoutf8( pImpl_->materialName ),
					bw_wtoutf8( pImpl_->matterName ),
					bw_wtoutf8( pImpl_->tintName ), descName );
}

BW_END_NAMESPACE

