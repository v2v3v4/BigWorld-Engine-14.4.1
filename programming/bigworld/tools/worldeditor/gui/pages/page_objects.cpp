#include "pch.hpp"
#include "worldeditor/framework/world_editor_app.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "worldeditor/framework/mainframe.hpp"
#include "worldeditor/gui/pages/page_objects.hpp"
#include "worldeditor/scripting/we_python_adapter.hpp"
#include "worldeditor/misc/options_helper.hpp"
#include "worldeditor/misc/placement_presets.hpp"
#include "worldeditor/misc/selection_filter.hpp"
#include "worldeditor/misc/progress_bar_helper.hpp"
#include "worldeditor/world/items/editor_chunk_model.hpp"
#include "worldeditor/world/items/editor_chunk_tree.hpp"
#include "appmgr/options.hpp"
#include "common/user_messages.hpp"
#include "gizmo/item_view.hpp"
#include "guimanager/gui_manager.hpp"
#include "ual/ual_manager.hpp"
#include "ual/ual_dialog.hpp"


DECLARE_DEBUG_COMPONENT( 0 )

BW_BEGIN_NAMESPACE

// PageObjects


// GUITABS content ID ( declared by the IMPLEMENT_BASIC_CONTENT macro )
const BW::wstring PageObjects::contentID = L"PageObjects";


PageObjects::PageObjects()
	: CFormView(PageObjects::IDD),
	resizeReady_( false ),
	pageReady_( false ),
	lastCoordType_(""),
	lastSnapType_( -1 ),
	lastDragOnSelect_( -1 ),
	lastGridSnap_( -1 ),
	useNameAsFilter_(false)
{
	BW_GUARD;

	snapsX_.SetMinimum(0, false);
	snapsY_.SetMinimum(0, false);
	snapsZ_.SetMinimum(0, false);
	snapsAngle_.SetMinimum(0, false);
}

PageObjects::~PageObjects()
{
	BW_GUARD;

	PlacementPresets::instance()->removeComboBox( &placementMethod_ );
}

void PageObjects::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_OBJECTS_SELECTION_FILTER, selectionFilter_);

	DDX_Control(pDX, IDC_OBJECTS_COORDS_WORLD, worldCoords_);
	DDX_Control(pDX, IDC_OBJECTS_COORDS_LOCAL, localCoords_);
	DDX_Control(pDX, IDC_OBJECTS_COORDS_VIEW, viewCoords_);

	DDX_Control(pDX, IDC_OBJECTS_LOCK_FREE, freeSnap_);
	DDX_Control(pDX, IDC_OBJECTS_LOCK_TERRAIN, terrainSnap_);
	DDX_Control(pDX, IDC_OBJECTS_LOCK_OBSTACLE, obstacleSnap_);

	DDX_Control(pDX, IDC_OBJECTS_LOCK_TO_GRID, gridSnap_);
	DDX_Control(pDX, IDC_OPTIONS_SNAPS_X, snapsX_);
	DDX_Control(pDX, IDC_OPTIONS_SNAPS_Y, snapsY_);
	DDX_Control(pDX, IDC_OPTIONS_SNAPS_Z, snapsZ_);
	DDX_Control(pDX, IDC_OPTIONS_SNAPS_ANGLE, snapsAngle_);

	DDX_Control(pDX, IDC_OBJECTS_DRAG_ON_SELECT, dragOnSelect_);

	DDX_Control(pDX, IDC_BUTTON_REPLACE, ctlBtnReplace_);
	DDX_Control(pDX, IDC_EDIT_SELBYNAME, ctlEditSelByName_);
	DDX_Control(pDX, IDC_EDIT_REPLACEWITH, ctlEditReplaceWith_);
	DDX_Control(pDX, IDC_CHECK_FILTERSELECTIONBYNAME, ctlCheckFilterByName_);

	resizeReady_ = true;
	DDX_Control(pDX, IDC_OBJECTS_SELECT_PLACEMENT_SETTING, placementMethod_);
	DDX_Control(pDX, IDC_CHECKBOX_FENCE_SELECTWHOLE, ctlCheckSelectWholeFences_);
}

void PageObjects::InitPage()
{
	BW_GUARD;

	//Handle page initialisation here...

	ctlEditSelByName_.init( 0,
		MAKEINTRESOURCE( IDB_SEARCHCLOSE ), L"", L"", L"" );
	ctlEditReplaceWith_.init( 0,
		MAKEINTRESOURCE( IDB_SEARCHCLOSE ), L"", L"", L"" );

	ctlBtnReplace_.EnableWindow( FALSE );

	if (WorldEditorApp::instance().pythonAdapter())
	{
		WorldEditorApp::instance().pythonAdapter()->selectFilterUpdate( &selectionFilter_ );
	}

	Vector3 movementSnap(1.f, 1.f, 1.f);
	movementSnap = Options::getOptionVector3("snaps/movement", movementSnap);
	snapsX_.SetValue(movementSnap.x);
	snapsY_.SetValue(movementSnap.y);
	snapsZ_.SetValue(movementSnap.z);

	float snapAngle(1.f);
	snapAngle = Options::getOptionFloat("snaps/angle", snapAngle);
	snapsAngle_.SetValue(snapAngle);

	PlacementPresets::instance()->addComboBox( &placementMethod_ );
	PlacementPresets::instance()->readPresets();

	INIT_AUTO_TOOLTIP();
	
	pageReady_ = true;
}


BEGIN_MESSAGE_MAP(PageObjects, CFormView)
	ON_WM_SIZE()
	ON_MESSAGE (WM_ACTIVATE_TOOL, OnActivateTool)
	ON_MESSAGE(WM_UPDATE_CONTROLS, OnUpdateControls)
	ON_WM_HSCROLL()

	ON_MESSAGE(WM_SHOW_TOOLTIP, OnShowTooltip)
	ON_MESSAGE(WM_HIDE_TOOLTIP, OnHideTooltip)

	ON_CBN_SELCHANGE(IDC_OBJECTS_SELECTION_FILTER, OnCbnSelchangeObjectsSelectionFilter)

	ON_BN_CLICKED(IDC_OBJECTS_COORDS_WORLD, OnBnClickedObjectsCoordsWorld)
	ON_BN_CLICKED(IDC_OBJECTS_COORDS_LOCAL, OnBnClickedObjectsCoordsLocal)
	ON_BN_CLICKED(IDC_OBJECTS_COORDS_VIEW, OnBnClickedObjectsCoordsView)

	ON_BN_CLICKED(IDC_OBJECTS_LOCK_FREE, OnBnClickedObjectsLockFree)
	ON_BN_CLICKED(IDC_OBJECTS_LOCK_TERRAIN, OnBnClickedObjectsLockTerrain)
	ON_BN_CLICKED(IDC_OBJECTS_LOCK_OBSTACLE, OnBnClickedObjectsLockObstacle)

	ON_BN_CLICKED(IDC_OBJECTS_LOCK_TO_GRID, OnBnClickedObjectsLockToGrid)
	ON_BN_CLICKED(IDC_OPTIONS_SHELLSNAPS, OnBnClickedOptionsShellsnaps)
	ON_BN_CLICKED(IDC_OPTIONS_UNITSNAPS, OnBnClickedOptionsUnitsnaps)
	ON_BN_CLICKED(IDC_OPTIONS_POINTONESNAPS, OnBnClickedOptionsPointonesnaps)

	ON_MESSAGE(WM_EDITNUMERIC_CHANGE, OnChangeEditNumeric)

	ON_BN_CLICKED(IDC_OBJECTS_DRAG_ON_SELECT, OnBnClickedObjectsDragOnSelect)

	ON_CBN_SELCHANGE(IDC_OBJECTS_SELECT_PLACEMENT_SETTING, OnCbnSelchangeObjectsSelectPlacementSetting)
	ON_BN_CLICKED(IDC_OBJECTS_EDIT_PLACEMENT_SETTINGS, OnBnClickedObjectsEditPlacementSettings)
	ON_BN_CLICKED(IDC_CHECKBOX_FENCE_SELECTWHOLE, &PageObjects::OnBnClickedCheckboxSelectWholeFences)

	ON_BN_CLICKED(IDC_BUTTON_SELECT, &PageObjects::OnBnClickedSelectByName)
	ON_BN_CLICKED(IDC_BUTTON_REPLACE, &PageObjects::OnBnClickedReplace)
	ON_BN_CLICKED(IDC_BUTTON_SELECTIONTONAMEFIELD, &PageObjects::OnBnClickedSelectionToNameField)
	ON_BN_CLICKED(IDC_BUTTON_SELECTIONTOREPLACEWITH, &PageObjects::OnBnClickedSelectionToReplaceWith)
	ON_BN_CLICKED(IDC_CHECK_FILTERSELECTIONBYNAME, &PageObjects::OnBnClickedCheckboxFilterByName)

	ON_MESSAGE( WM_SEARCHFIELD_CLEAR, &PageObjects::OnEditFieldClear )
	ON_MESSAGE( WM_SEARCHFIELD_CHANGE, &PageObjects::OnEditFieldChanged )

END_MESSAGE_MAP()



LRESULT PageObjects::OnActivateTool(WPARAM wParam, LPARAM lParam)
{
	BW_GUARD;

	const wchar_t *activePageId = (const wchar_t*)(wParam);
	if (getContentID() == activePageId)
	{
		if (WorldEditorApp::instance().pythonAdapter())
		{
			WorldEditorApp::instance().pythonAdapter()->onPageControlTabSelect("pgc", "Object");
		}
	}

	ctlCheckSelectWholeFences_.SetCheck(EditorChunkModel::s_autoSelectFenceSections ? BST_CHECKED : BST_UNCHECKED);

	return 0;
}

void PageObjects::stretchToRight( CWnd& widget, int pageWidth, int border )
{
	BW_GUARD;

	CRect rect;
	widget.GetWindowRect( &rect );
    ScreenToClient( &rect );
	widget.SetWindowPos( 0, rect.left, rect.top, pageWidth - rect.left - border, rect.Height(), SWP_NOZORDER );
	widget.RedrawWindow();
}

afx_msg void PageObjects::OnSize( UINT nType, int cx, int cy )
{
	BW_GUARD;

	CFormView::OnSize( nType, cx, cy );

	if ( !resizeReady_ )
		return;
		
	//stretchToRight( selectionFilter_, cx, 12 );
}

void PageObjects::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	BW_GUARD;

	// this captures all the scroll events for the page
	if ( !pageReady_ )
		InitPage();

	//Handle h-scroll events here...

	CFormView::OnHScroll(nSBCode, nPos, pScrollBar);
}

afx_msg LRESULT PageObjects::OnShowTooltip(WPARAM wParam, LPARAM lParam)
{
	char* msg = (char*)lParam;
	//MainFrame::instance().SetMessageText( msg );
	return 0;
}

afx_msg LRESULT PageObjects::OnHideTooltip(WPARAM wParam, LPARAM lParam)
{
	//MainFrame::instance().SetMessageText( "" );
	return 0;
}

afx_msg LRESULT PageObjects::OnEditFieldClear( WPARAM wParam, LPARAM lParam )
{
	BW_GUARD;

	if (ctlEditSelByName_.searchText().length() == 0)
	{
		useNameAsFilter_ = false;
		updateSelectionFilter();
	}
	return 0;
}

afx_msg LRESULT PageObjects::OnUpdateControls(WPARAM wParam, LPARAM lParam)
{
	BW_GUARD;

	if ( !pageReady_ )
		InitPage();

	//Ensure the correct coordinate system is selected
	BW::string coordType = Options::getOptionString( "tools/coordFilter" );
	if ( coordType != lastCoordType_)
	{
		worldCoords_.SetCheck( coordType == "World" ? BST_CHECKED : BST_UNCHECKED );
		localCoords_.SetCheck( coordType == "Local" ? BST_CHECKED : BST_UNCHECKED );
		viewCoords_.SetCheck( coordType == "View" ? BST_CHECKED : BST_UNCHECKED );
		lastCoordType_ = coordType;
	}

	//Ensure the correct snap type is selected
	int snapType = Options::getOptionInt( "snaps/itemSnapMode");
	if ( snapType != lastSnapType_)
	{
		freeSnap_.SetCheck( snapType == 0 ? BST_CHECKED : BST_UNCHECKED );
		terrainSnap_.SetCheck( snapType == 1 ? BST_CHECKED : BST_UNCHECKED );
		obstacleSnap_.SetCheck( snapType == 2 ? BST_CHECKED : BST_UNCHECKED );
		lastSnapType_ = snapType;
	}

	//Update the drag on select checkbox
	int dragOnSelect = Options::getOptionInt( "dragOnSelect" );
	if ( dragOnSelect != lastDragOnSelect_)
	{
		dragOnSelect_.SetCheck( dragOnSelect == 1 ? BST_CHECKED : BST_UNCHECKED );
		lastDragOnSelect_ = dragOnSelect;
	}

	//Update the grid snap checkbox
	int gridSnap = Options::getOptionInt( "snaps/xyzEnabled");
	if ( gridSnap != lastGridSnap_)
	{
		gridSnap_.SetCheck( gridSnap == 1 ? BST_CHECKED : BST_UNCHECKED );
		lastGridSnap_ = gridSnap;
	}

	if (!selectionFilter_.GetDroppedState() && 
		WorldEditorApp::instance().pythonAdapter())
	{
		WorldEditorApp::instance().pythonAdapter()->selectFilterUpdate( &selectionFilter_ );
	}
	return 0;
}

void PageObjects::OnCbnSelchangeObjectsSelectionFilter()
{
	BW_GUARD;

	CString sel;
	int index = selectionFilter_.GetCurSel();
	selectionFilter_.GetLBText( index, sel );

	if (WorldEditorApp::instance().pythonAdapter())
	{
		WorldEditorApp::instance().pythonAdapter()->selectFilterChange( bw_wtoutf8( sel.GetString() ));
	}
}

void PageObjects::OnBnClickedObjectsCoordsWorld()
{
	BW_GUARD;

	if (WorldEditorApp::instance().pythonAdapter())
	{
		WorldEditorApp::instance().pythonAdapter()->coordFilterChange( "World" );
	}
}

void PageObjects::OnBnClickedObjectsCoordsLocal()
{
	BW_GUARD;

	if (WorldEditorApp::instance().pythonAdapter())
	{
		WorldEditorApp::instance().pythonAdapter()->coordFilterChange( "Local" );
	}
}

void PageObjects::OnBnClickedObjectsCoordsView()
{
	BW_GUARD;

	if (WorldEditorApp::instance().pythonAdapter())
	{
		WorldEditorApp::instance().pythonAdapter()->coordFilterChange( "View" );
	}
}
	
void PageObjects::OnBnClickedObjectsLockFree()
{
	BW_GUARD;

	if (WorldEditorApp::instance().pythonAdapter())
	{
		WorldEditorApp::instance().pythonAdapter()->ActionScriptExecute("actXZSnap");
	}
}

void PageObjects::OnBnClickedObjectsLockTerrain()
{
	BW_GUARD;

	if (WorldEditorApp::instance().pythonAdapter())
	{
		WorldEditorApp::instance().pythonAdapter()->ActionScriptExecute("actTerrainSnaps");
	}
}

void PageObjects::OnBnClickedObjectsLockObstacle()
{
	BW_GUARD;

	if (WorldEditorApp::instance().pythonAdapter())
	{
		WorldEditorApp::instance().pythonAdapter()->ActionScriptExecute("actObstacleSnap");
	}
}

void PageObjects::OnBnClickedObjectsDragOnSelect()
{
	BW_GUARD;

	if (WorldEditorApp::instance().pythonAdapter())
	{
		WorldEditorApp::instance().pythonAdapter()->ActionScriptExecute("actDragOnSelect");
	}
}

void PageObjects::OnBnClickedObjectsLockToGrid()
{
	BW_GUARD;

	if (WorldEditorApp::instance().pythonAdapter())
	{
		WorldEditorApp::instance().pythonAdapter()->ActionScriptExecute("actToggleSnaps");
	}
}

void PageObjects::OnBnClickedOptionsShellsnaps()
{
	BW_GUARD;

	Vector3 shellSnap(4.f, 1.f, 4.f);
	shellSnap = Options::getOptionVector3("shellSnaps/movement", shellSnap);

	snapsX_.SetValue(shellSnap.x);
	snapsY_.SetValue(shellSnap.y);
	snapsZ_.SetValue(shellSnap.z);

	float shellSnapAngle(90.f);
	shellSnapAngle = Options::getOptionFloat("shellSnaps/angle", shellSnapAngle);
	snapsAngle_.SetValue(shellSnapAngle);

	OnChangeEditNumeric(0, 0);
}

void PageObjects::OnBnClickedOptionsUnitsnaps()
{
	BW_GUARD;

	snapsX_.SetValue(1.f);
	snapsY_.SetValue(1.f);
	snapsZ_.SetValue(1.f);
	snapsAngle_.SetValue(1.f);

	OnChangeEditNumeric(0, 0);
}

void PageObjects::OnBnClickedOptionsPointonesnaps()
{
	BW_GUARD;

	snapsX_.SetValue(0.1f);
	snapsY_.SetValue(0.1f);
	snapsZ_.SetValue(0.1f);
	snapsAngle_.SetValue(1.f);

	OnChangeEditNumeric(0, 0);
}

void PageObjects::OnBnClickedCheckboxSelectWholeFences()
{
	EditorChunkModel::s_autoSelectFenceSections = (ctlCheckSelectWholeFences_.GetCheck() == BST_CHECKED);
}

LRESULT PageObjects::OnChangeEditNumeric(WPARAM mParam, LPARAM lParam)
{
	BW_GUARD;

	Vector3 movemenSnap(snapsX_.GetValue(), snapsY_.GetValue(), snapsZ_.GetValue());
	Options::setOptionVector3("snaps/movement", movemenSnap);
	Options::setOptionFloat("snaps/angle", snapsAngle_.GetValue());
	return 0;
}

void PageObjects::OnCbnSelchangeObjectsSelectPlacementSetting()
{
	BW_GUARD;

	PlacementPresets::instance()->currentPreset( &placementMethod_ );
}

void PageObjects::OnBnClickedObjectsEditPlacementSettings()
{
	BW_GUARD;

	PlacementPresets::instance()->showDialog();
}

void PageObjects::OnBnClickedCheckboxFilterByName()
{
	BW_GUARD;

	useNameAsFilter_ = (ctlCheckFilterByName_.GetCheck() == BST_CHECKED);

	updateSelectionFilter();
}

void PageObjects::OnBnClickedSelectByName()
{
	BW_GUARD;

	if (ctlEditSelByName_.searchText().length() == 0)
		OnBnClickedSelectionToNameField();

	if (WorldEditorApp::instance().pythonAdapter())
	{
		bool prevValue = useNameAsFilter_;
		useNameAsFilter_ = true;
		updateSelectionFilter();

		WorldEditorApp::instance().pythonAdapter()->selectAll();

		useNameAsFilter_ = prevValue;
		updateSelectionFilter();
	}
}

void PageObjects::OnBnClickedSelectionToNameField()
{
	BW_GUARD;

	BW::wstring name = getSelectedAssetName( true );

	if (name.size() == 0)
		name = getSelectedAssetName( false );

	if (name.size() > 0)
		ctlEditSelByName_.searchText( name.c_str() );
}

void PageObjects::OnBnClickedSelectionToReplaceWith()
{
	BW_GUARD;

	BW::wstring name = getSelectedAssetName( true );

	if (name.size() == 0)
		name = getSelectedAssetName( false );

	if (name.size() > 0)
		ctlEditReplaceWith_.searchText( name.c_str() );
}

afx_msg LRESULT PageObjects::OnEditFieldChanged( WPARAM wParam, LPARAM lParam )
{
	BW_GUARD;

	BOOL enableBtnReplace = TRUE;

	if (ctlEditSelByName_.searchText().length() == 0 ||
		ctlEditReplaceWith_.searchText().length() == 0)
	{
		enableBtnReplace = FALSE;
	}
	ctlBtnReplace_.EnableWindow( enableBtnReplace );
	updateSelectionFilter();

	return 0;
}


void PageObjects::updateSelectionFilter()
{
	BW_GUARD;

	BW::wstring txt;

	if (useNameAsFilter_)
	{
		txt = ctlEditSelByName_.searchText();
		if (ctlCheckFilterByName_.GetCheck() == BST_UNCHECKED)
		{
			ctlCheckFilterByName_.SetCheck( BST_CHECKED );
		}
	}
	else
	{
		if (ctlCheckFilterByName_.GetCheck() == BST_CHECKED)
		{
			ctlCheckFilterByName_.SetCheck( BST_UNCHECKED );
		}
	}

	SelectionFilter::setFilterByName( bw_wtoutf8( txt ) );
}

BW::wstring PageObjects::getSelectedAssetName( bool inScene )
{
	BW_GUARD;

	BW::wstring ret;

	if (inScene)
	{
		const BW::vector<ChunkItemPtr> &selection = WorldManager::instance().selectedItems();
		if (selection.size() > 0)
		{
			ChunkItem *pFirstItem = selection[0].get();
			if (pFirstItem)
			{
				BW::string path = pFirstItem->edFilePath();
				ret = bw_utf8tow(path).c_str();
			}
		}
	}
	else
	{
		UalDialog *dlg = UalManager::instance().getActiveDialog();
		if (dlg != NULL)
		{
			BW::vector<BW::wstring> selAssetPaths = dlg->getSelectedAssetPaths();
			if (selAssetPaths.size() > 0)
				ret = selAssetPaths[0];
		}
	}

	return ret;
	
}

void PageObjects::OnBnClickedReplace()
{
	BW_GUARD;

	// resourceID to replace with
	const BW::wstring strText = ctlEditReplaceWith_.searchText();
	BW::string resourceID = bw_wtoutf8( strText );
	if (resourceID.size() == 0)
		resourceID = bw_wtoutf8( getSelectedAssetName( false ) );

	replaceSelectedItems( resourceID );
}


BW::string strReplace(const BW::string &str, const BW::string &sOld, const BW::string &sNew)
{
	BW_GUARD;

	if (str.empty() || sOld.empty())
		return str;

	BW::string ret;
	ret.reserve(str.size());

	size_t pos = 0;
	while (true)
	{
		BW::string::size_type next = str.find(sOld, pos);
		if (next == str.npos)
			break;

		if (next > pos)
			ret += str.substr(pos, next - pos);

		if (sNew.size() > 0)
			ret += sNew;

		pos = next + sOld.size();
	}

	if (pos < str.size())
		ret += str.substr(pos);

	return ret;
}


//----------------------------------------------------------------------------
// 'Replace' command implementation

void PageObjects::replaceSelectedItems(const BW::string& resourceID)
{
	BW_GUARD;

	MF_ASSERT( (!resourceID.empty()) );

	MainFrame *pMainWnd = ((MainFrame*)AfxGetMainWnd());
	WEPythonAdapter *pAdapter = WorldEditorApp::instance().pythonAdapter();
	if (!pAdapter)
		return;

	if (!resourceID.empty())
	{
		// check if given asset exists
		if (!BWResource::instance().fileExists(resourceID))
		{
			::MessageBox( AfxGetApp()->m_pMainWnd->GetSafeHwnd(),
				Localise(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/REPLACE_WARNING_INVALID", resourceID.c_str()),
				Localise(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/REPLACE_WARNING_TITLE"),
				MB_OK | MB_ICONWARNING );
			PyErr_Format(PyExc_NameError, "Invalid asset path: '%s', please specify correct full asset path", resourceID.c_str());
			return;
		}
	}

	BeginWaitCursor();

	// disable undo tracking
	UndoRedo::instance().disableAdd(true);

	BW::vector<ChunkItemPtr> itemsRemoved = WorldManager::instance().selectedItems();
	if (itemsRemoved.size() == 0)
	{
		EndWaitCursor();
		UndoRedo::instance().disableAdd(false);
		return;	
	}

	// set Free locking mode to allow placing new chunkitems even outside of chunks (we'll assign proper matrix afterwards)
	pAdapter->ActionScriptExecute("actXZSnap");
	bool snapsWasEnabled = !!OptionsSnaps::snapsEnabled();
	if (snapsWasEnabled)
		pAdapter->ActionScriptExecute("actToggleSnaps");
	OptionsHelper::tick();

	// retrieve selected items matrices (world space) and flags
	BW::vector<Matrix> matrices;
	BW::vector<bool> reflectionVisible;
	for (size_t i = 0; i < itemsRemoved.size(); i++)
	{
		Matrix m(itemsRemoved[i]->edTransform());
		m.postMultiply(itemsRemoved[i]->chunk()->transform());
		matrices.push_back(m);
		reflectionVisible.push_back(itemsRemoved[i]->reflectionVisible());		
	}

	// delete selection
	BW::vector<Chunk*> itemsRemovedChunks;
	ChunkItemGroupPtr gr = new ChunkItemGroup();
	for (size_t i = 0; i < itemsRemoved.size(); i++)
	{
		itemsRemovedChunks.push_back(itemsRemoved[i]->chunk());
		gr->add(itemsRemoved[i]);
	}
	pAdapter->deleteChunkItems(gr);

	// add replacement items
	BW::vector<ChunkItemPtr> itemsAdded;
	const type_info& ChunkModelTI = typeid(EditorChunkModel);
	const type_info& ChunkTreeTI = typeid(EditorChunkTree);

	ScopedProgressBar progressTaskHelper( 1 );
	ProgressBarTask replaceProgress(
		LocaliseUTF8( L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/REPLACING_OBJECTS" ),
		(float)matrices.size(), false );
	for (size_t i = 0; i < matrices.size(); i++)
	{
		// create item
		PyObjectPtr pyGroup = pAdapter->addChunkItem( resourceID );
		if (pyGroup != NULL 
			&& PyObject_IsInstance(pyGroup.get(), (PyObject*)&ChunkItemGroup::s_type_))
		{
			// place added item appropriately
			// iterate just for weird case, normally size()==1
			ChunkItemGroup *pGroup = static_cast<ChunkItemGroup*>(pyGroup.get());
			BW::vector<ChunkItemPtr> &items = pGroup->get();
			for (size_t j = 0; j < items.size(); j++)
			{
				ChunkItemPtr pItem = items[j];
				Matrix m(matrices[i]);
				Matrix chunkMatrInv = pItem->chunk()->transform();
				chunkMatrInv.invert();
				m.postMultiply(chunkMatrInv);

				pItem->edTransform(m, false);

				const type_info& curTI = typeid(*pItem);
				if (curTI == ChunkModelTI)
					((ChunkModel*)pItem.get())->setReflectionVis(reflectionVisible[i]);
				else if (curTI == ChunkTreeTI)
					((ChunkTree*)pItem.get())->setReflectionVis(reflectionVisible[i]);

				itemsAdded.push_back(pItem);
			}
		}

		if (i % 5 == 0)
		{
			WorldManager::instance().setStatusMessage(0,
				LocaliseUTF8( L"WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_MODEL/REPLACE_PROGRESS",
				i, matrices.size() ));
			pMainWnd->updateStatusBar();
		}

		replaceProgress.step();
	}

	// post-cleanup
	EndWaitCursor();
	WorldManager::instance().setStatusMessage(0, "");
	pMainWnd->updateStatusBar();

	if (freeSnap_.GetCheck())
		pAdapter->ActionScriptExecute("actXZSnaps");
	else if (terrainSnap_.GetCheck())
		pAdapter->ActionScriptExecute("actTerrainSnaps");
	else
		pAdapter->ActionScriptExecute("actObstacleSnap");
	if (snapsWasEnabled)
		pAdapter->ActionScriptExecute("actToggleSnaps");
	OptionsHelper::tick();

	// turn on undo tracking
	UndoRedo::instance().disableAdd(false);

	// select replaced items
	gr = new ChunkItemGroup();
	for (size_t i = 0; i < itemsAdded.size(); i++)
		gr->add(itemsAdded[i]);
	WorldEditorApp::instance().pythonAdapter()->setSelection(gr);

	// add Undo operation which will revert replace and will add Redo operation
	UndoRedo::instance().add(new UndoRedoReplace(this, itemsRemoved, itemsRemovedChunks, itemsAdded));
	UndoRedo::instance().barrier("Replace group", false);

	WorldManager::instance().addCommentaryMsg( LocaliseUTF8( L"WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_MODEL/REPLACE_FINISHED", itemsAdded.size() ) );
}

PageObjects::UndoRedoReplace::UndoRedoReplace(
	PageObjects *pOwner,
	const BW::vector<ChunkItemPtr>& itemsRemoved,
	const BW::vector<Chunk*>& itemsRemovedChunks,
	const BW::vector<ChunkItemPtr>& itemsAdded)
	:	UndoRedo::Operation(size_t(typeid(UndoRedoReplace).name())),
	pOwner_(pOwner),
	itemsRemoved_(itemsRemoved),
	itemsRemovedChunks_(itemsRemovedChunks),
	itemsAdded_(itemsAdded)
{
	BW::vector<Chunk*> chunks;

	// notify Environment about chunks affected by this change
	for (BW::vector<Chunk*>::iterator it = itemsRemovedChunks_.begin();
		it != itemsRemovedChunks_.end(); it++)
	{
		if (std::find(chunks.begin(), chunks.end(), *it) == chunks.end())
		{
			chunks.push_back(*it);
			addChunk(*it);
		}
	}
	for (size_t i = 0; i < itemsAdded.size(); i++)
	{
		Chunk *chunk = itemsAdded[i]->chunk();
		if (std::find(chunks.begin(), chunks.end(), chunk) == chunks.end())
		{
			chunks.push_back(chunk);
			addChunk(chunk);
		}
	}
}

void PageObjects::UndoRedoReplace::undo()
{
	UndoRedo::instance().disableAdd(true);

	// restore removed items
	for (size_t i = 0; i < itemsRemoved_.size(); i++)
	{
		ChunkItemPtr pItem = itemsRemoved_[i];
		Chunk *pChunk = itemsRemovedChunks_[i];

		pChunk->addStaticItem(pItem);
		pItem->edPostCreate();
	}

	// delete added items
	BW::vector<Chunk*> itemsAddedChunks;
	for (size_t i = 0; i < itemsAdded_.size(); i++)
	{
		ChunkItemPtr pItem = itemsAdded_[i];
		Chunk *pChunk = pItem->chunk();

		pItem->edPreDelete();
		pItem->chunk()->delStaticItem(pItem);
		itemsAddedChunks.push_back(pChunk);
	}

	// re-select items after reverting replace
	ChunkItemGroupPtr gr = new ChunkItemGroup();
	for (size_t i = 0; i < itemsRemoved_.size(); i++)
		gr->add(itemsRemoved_[i]);
	WorldEditorApp::instance().pythonAdapter()->setSelection(gr);

	// add Redo operation for this Undo (or Undo operation for this Redo)
	UndoRedo::instance().disableAdd(false);
	UndoRedo::instance().add(new UndoRedoReplace(pOwner_, itemsAdded_, itemsAddedChunks, itemsRemoved_));
}

BW_END_NAMESPACE

