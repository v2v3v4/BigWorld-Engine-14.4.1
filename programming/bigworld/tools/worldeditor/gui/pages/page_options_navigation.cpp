#include "pch.hpp"
#include "worldeditor/gui/pages/page_options_navigation.hpp"
#include "worldeditor/gui/pages/panel_manager.hpp"
#include "worldeditor/framework/world_editor_app.hpp"
#include "worldeditor/scripting/world_editor_script.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "worldeditor/misc/world_editor_camera.hpp"
#include "appmgr/options.hpp"
#include "common/user_messages.hpp"
#include "resmgr/sanitise_helper.hpp"
#include "romp/time_of_day.hpp"
#include "controls/user_messages.hpp"
#include "chunk/chunk.hpp"
#include "chunk/chunk_manager.hpp"
#include "chunk/chunk_space.hpp"
#include "chunk/geometry_mapping.hpp"
#include "cstdmf/debug.hpp"
#include "cstdmf/watcher.hpp"


DECLARE_DEBUG_COMPONENT2( "WorldEditor", 0 )

BW_BEGIN_NAMESPACE

namespace
{
	// The number used to convert heights to the height slider fixed point
	// value:
	const unsigned int	HEIGHT_SLIDER_PREC	= 10;
}


// PageOptionsNavigation

// GUITABS content ID ( declared by the IMPLEMENT_BASIC_CONTENT macro )
const BW::wstring PageOptionsNavigation::contentID = L"PageOptionsNavigation";


PageOptionsNavigation::PageOptionsNavigation()
	: CFormView(PageOptionsNavigation::IDD),
	pageReady_( false ),
	dontUpdateHeightEdit_(false),
	cameraHeightEdited_(false)
{
}

PageOptionsNavigation::~PageOptionsNavigation()
{
}

void PageOptionsNavigation::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_OPTIONS_LOCATION_SEARCH, search_);

	DDX_Control(pDX, IDC_OPTIONS_LOCATION_LIST, locationList_);

	DDX_Control(pDX, IDC_OPTIONS_LOCATION_RENAME, locationRename_);
	DDX_Control(pDX, IDC_OPTIONS_LOCATION_UPDATE, locationUpdate_);
	DDX_Control(pDX, IDC_OPTIONS_LOCATION_REMOVE, locationRemove_);
	DDX_Control(pDX, IDC_OPTIONS_LOCATION_MOVE, locationMoveTo_);

	DDX_Control(pDX, IDC_OPTIONS_POS_X, posXEdit_);
	DDX_Control(pDX, IDC_OPTIONS_POS_Y, posYEdit_);
	DDX_Control(pDX, IDC_OPTIONS_POS_Z, posZEdit_);

	DDX_Control(pDX, IDC_OPTIONS_CHUNK, posChunkEdit_);

	DDX_Control(pDX, IDC_OPTIONS_CAMERAHEIGHT_SLIDER, cameraHeightSlider_);
	DDX_Control(pDX, IDC_OPTIONS_CAMERAHEIGHT_EDIT, cameraHeightEdit_);

	DDX_Control(pDX, IDC_PLAYER_PREVIEW_MODE, isPlayerPreviewModeEnabled_);
}

BOOL PageOptionsNavigation::PreTranslateMessage( MSG* pMsg )
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
		CEdit* edit = locationList_.GetEditControl();
		if (edit)
		{
			edit->SendMessage( WM_KEYDOWN, pMsg->wParam, pMsg->lParam );
			return TRUE; // Handled
		}
	}
	return CFormView::PreTranslateMessage( pMsg );
}

void PageOptionsNavigation::InitPage()
{
	BW_GUARD;

	INIT_AUTO_TOOLTIP();

	search_.init( MAKEINTRESOURCE( IDB_SEARCHMAGNIFIER ),
				MAKEINTRESOURCE( IDB_SEARCHCLOSE ), 
				Localise( L"RCST_IDC_OPTIONS_SEARCH_FIELD_IDLE_TEXT" ),
				Localise( L"RCST_IDC_OPTIONS_SEARCH_FIELD_SEARCH_FILTER" ), 
				Localise( L"RCST_IDC_OPTIONS_SEARCH_FIELD_SEARCH_TEXT" ) );

	posXEdit_.SetNumericType(controls::EditNumeric::ENT_FLOAT);
	posXEdit_.SetNumDecimals(1);
	posXEdit_.commitOnFocusLoss(false);

	posYEdit_.SetNumericType(controls::EditNumeric::ENT_FLOAT);
	posYEdit_.SetNumDecimals(1);
	posYEdit_.commitOnFocusLoss(false);

	posZEdit_.SetNumericType(controls::EditNumeric::ENT_FLOAT);
	posZEdit_.SetNumDecimals(1);
	posZEdit_.commitOnFocusLoss(false);

	posChunkEdit_.commitOnFocusLoss(false);

	Matrix view = WorldEditorCamera::instance().currentCamera().view();
	view.invert();
	Vector3 pos = view.applyToUnitAxisVector(3);
	lastPos_ = pos;

	posXEdit_.SetValue( pos.x );
	posYEdit_.SetValue( pos.y );
	posZEdit_.SetValue( pos.z );

	Chunk* chunk = ChunkManager::instance().cameraSpace()->findChunkFromPoint( pos );
	if (chunk)
	{
		posChunkEdit_.SetWindowText( bw_utf8tow( chunk->identifier() ).c_str() );
	}

	locationList_.updateLocationsList( WorldManager::instance().getCurrentSpace() );

	int maxCameraHeight = Options::getOptionInt( "graphics/maxCameraHeight", 200 );

	cameraHeightSlider_.SetRangeMin( 1 * HEIGHT_SLIDER_PREC );
	cameraHeightSlider_.SetRangeMax( maxCameraHeight * HEIGHT_SLIDER_PREC );
	cameraHeightSlider_.SetPageSize(0);

	cameraHeightEdit_.SetNumericType( controls::EditNumeric::ENT_FLOAT );
	cameraHeightEdit_.SetAllowNegative( false );
	cameraHeightEdit_.SetMinimum( 1 );
	cameraHeightEdit_.SetMaximum( maxCameraHeight );
	cameraHeightEdit_.SetValue( Options::getOptionFloat( "graphics/cameraHeight", 2.f ) );
	cameraHeightSlider_.SetPos( (int)(HEIGHT_SLIDER_PREC*Options::getOptionFloat( "graphics/cameraHeight", 2.f )) );

	updateSliderEdits();
}


BEGIN_MESSAGE_MAP(PageOptionsNavigation, CFormView)
	ON_WM_SHOWWINDOW()
	ON_WM_HSCROLL()
	ON_WM_CTLCOLOR()
	ON_MESSAGE(WM_UPDATE_CONTROLS, OnUpdateControls)
	ON_MESSAGE(WM_SEARCHFIELD_CHANGE, OnSearchFieldChanged)
	ON_EN_CHANGE(IDC_OPTIONS_CAMERAHEIGHT_EDIT, OnEnChangeOptionsCameraHeightEdit)
	ON_BN_CLICKED(IDC_PLAYER_PREVIEW_MODE, OnBnClickedPlayerPreviewMode)
	ON_BN_CLICKED(IDC_OPTIONS_POS_MOVE, OnBnClickedOptionsPosMove)
	ON_BN_CLICKED(IDC_OPTIONS_CHUNK_MOVE, OnBnClickedOptionsChunkMove)
	ON_BN_CLICKED(IDC_OPTIONS_LOCATION_ADD, OnBnClickedOptionsLocationAdd)
	ON_BN_CLICKED(IDC_OPTIONS_LOCATION_REMOVE, OnBnClickedOptionsLocationRemove)
	ON_BN_CLICKED(IDC_OPTIONS_LOCATION_MOVE, OnBnClickedOptionsLocationMove)
	ON_BN_CLICKED(IDC_OPTIONS_LOCATION_RENAME, OnBnClickedOptionsLocationRename)
	ON_BN_CLICKED(IDC_OPTIONS_LOCATION_UPDATE, &PageOptionsNavigation::OnBnClickedOptionsLocationUpdate)
END_MESSAGE_MAP()


// PageOptionsNavigation message handlers
afx_msg void PageOptionsNavigation::OnShowWindow( BOOL bShow, UINT nStatus )
{
	BW_GUARD;

	CFormView::OnShowWindow( bShow, nStatus );

	if ( bShow == FALSE )
	{
	}
	else
	{
		OnUpdateControls( 0, 0 );
	}
}

afx_msg LRESULT PageOptionsNavigation::OnUpdateControls(WPARAM wParam, LPARAM lParam)
{
	BW_GUARD;

	if ( !pageReady_ )
	{
		InitPage();
		pageReady_ = true;
	}

	if ( !IsWindowVisible() )
		return 0;

	int enabled = 0;
	int checked = 0;

	isPlayerPreviewModeEnabled_.SetCheck( WorldManager::instance().isInPlayerPreviewMode() );

	static float s_lastCameraHeight = -1.f;
	float cameraHeight = Options::getOptionFloat( "graphics/cameraHeight", 2.f );
	if ( cameraHeightEdited_ )
	{
		s_lastCameraHeight = cameraHeight;
		cameraHeightEdited_ = false;
	}
	if (cameraHeight != s_lastCameraHeight)
	{
		cameraHeightSlider_.SetPos( (int)(HEIGHT_SLIDER_PREC*cameraHeight ));
		cameraHeightEdit_.SetValue( cameraHeight );
		s_lastCameraHeight = cameraHeight;
	}

	if ((posXEdit_.doUpdate()) || (posYEdit_.doUpdate()) || (posZEdit_.doUpdate()))
	{
		OnBnClickedOptionsPosMove();
	}

	if (posChunkEdit_.doUpdate())
	{
		OnBnClickedOptionsChunkMove();
	}

	Matrix view = WorldEditorCamera::instance().currentCamera().view();
	view.invert();
	Vector3 pos = view.applyToUnitAxisVector(3);
	if ((fabsf( pos.x - lastPos_.x ) > 0.1f) ||
		(fabsf( pos.y - lastPos_.y ) > 0.1f) ||
		(fabsf( pos.z - lastPos_.z ) > 0.1f))
	{
		posXEdit_.SetValue( pos.x );
		posYEdit_.SetValue( pos.y );
		posZEdit_.SetValue( pos.z );

		Chunk* chunk = ChunkManager::instance().cameraSpace()->findChunkFromPoint( pos );
		if (chunk)
		{
			posChunkEdit_.SetWindowText( bw_utf8tow( chunk->identifier() ).c_str() );
		}

		lastPos_ = pos;
	}

	static BW::string s_lastSpaceName = "";
	BW::string spaceName = WorldManager::instance().getCurrentSpace();
	if (spaceName != s_lastSpaceName)
	{
		locationList_.updateLocationsList( spaceName );

		s_lastSpaceName = spaceName;
	}

	HTREEITEM item = locationList_.GetSelectedItem();
	static HTREEITEM s_lastItem = (HTREEITEM)(-1);
	if (item != s_lastItem)
	{
		BOOL enabled = item ? TRUE : FALSE;
		locationRename_.EnableWindow( enabled );
		locationUpdate_.EnableWindow( enabled );
		locationRemove_.EnableWindow( enabled );
		locationMoveTo_.EnableWindow( enabled );
		RedrawWindow();
		s_lastItem = item;
	}

	return 0;
}

void PageOptionsNavigation::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	BW_GUARD;

	// this captures all the scroll events for the page
	// upadate all the slider buddy windows
	updateSliderEdits();

	Options::setOptionFloat
	( 
		"graphics/cameraHeight", 
		(float)cameraHeightSlider_.GetPos()/HEIGHT_SLIDER_PREC 
	);

	CFormView::OnHScroll(nSBCode, nPos, pScrollBar);
}

void PageOptionsNavigation::updateSliderEdits()
{
	BW_GUARD;

	if (!dontUpdateHeightEdit_)
	{
		cameraHeightEdit_.SetValue( (float)cameraHeightSlider_.GetPos()/HEIGHT_SLIDER_PREC );
	}
}

void PageOptionsNavigation::OnEnChangeOptionsCameraHeightEdit()
{
	BW_GUARD;

	cameraHeightEdited_ = true;

	dontUpdateHeightEdit_ = true;
	cameraHeightSlider_.SetPos( (int)(HEIGHT_SLIDER_PREC*cameraHeightEdit_.GetValue()) );
	dontUpdateHeightEdit_ = false;

	Options::setOptionFloat( "graphics/cameraHeight", cameraHeightEdit_.GetValue() );
}

void PageOptionsNavigation::OnBnClickedPlayerPreviewMode()
{
	BW_GUARD;

	WorldManager::instance().setPlayerPreviewMode( isPlayerPreviewModeEnabled_.GetCheck() == TRUE );
}

void PageOptionsNavigation::OnBnClickedOptionsPosMove()
{
	BW_GUARD;

	Vector3 pos;
	pos.x = posXEdit_.GetValue();
	pos.y = posYEdit_.GetValue();
	pos.z = posZEdit_.GetValue();
	if (!almostEqual( pos, lastPos_))
	{
		Matrix view = WorldEditorCamera::instance().currentCamera().view();
		view.invert();
		view.translation( pos );
		view.invert();
		WorldEditorCamera::instance().currentCamera().view( view );
		lastPos_ = pos;
	}
}

void PageOptionsNavigation::OnBnClickedOptionsChunkMove()
{
	BW_GUARD;

	CString chunkName_cstr;
	posChunkEdit_.GetWindowText( chunkName_cstr );
	BW::string chunkName;
	bw_wtoutf8( chunkName_cstr.GetString(), chunkName );
	GeometryMapping * mapping = WorldManager::instance().geometryMapping();

	if (*chunkName.rbegin() != 'o' && *chunkName.rbegin() != 'i')
	{
		BW::string insideChunkName = chunkName + "i";
		if( BWResource::fileExists( mapping->path() + insideChunkName + ".chunk" ) )
			chunkName = insideChunkName;
		else
			chunkName += 'o';
	}
	if( *chunkName.rbegin() != 'i')
	{
		int16 x = -32768, z = -32768;
		mapping->gridFromChunkName( chunkName, x, z );
		chunkName = mapping->outsideChunkIdentifier( x, z );
	}

	Chunk *chunk = NULL;
	if( BWResource::fileExists( mapping->path() + chunkName + ".chunk" ) )
		chunk = ChunkManager::instance().findChunkByName( chunkName, mapping );
	if( !chunk )
	{
		ERROR_MSG("\"%S\" is not a valid chunk\n", chunkName_cstr.GetString() );
		return;
	}

	if (chunk->loaded())
	{
		ChunkSpacePtr space = ChunkManager::instance().cameraSpace();
		if ( space )
		{
			Matrix view( Matrix::identity );

			Vector3 camPos( chunk->centre() );

			if (chunk->isOutsideChunk())
			{
				const float EXTENT_RANGE = 5000.0f;

				camPos.y = EXTENT_RANGE;

				Vector3 extent = camPos + ( Vector3( 0.f, -2.f*EXTENT_RANGE, 0.f ) );

				float dist = space->collide( camPos, extent);

				camPos = camPos + Vector3( 0.f, 2.f - dist, 0.f );
			}

			view.translation( camPos );
			view.invert();
			WorldEditorCamera::instance().currentCamera().view( view );
		}
	}
	else
	{
		bool got = false;
		Vector3 world;
		if( chunk->isOutsideChunk() )
		{
			BoundingBox bb = chunk->boundingBox();
			world = bb.centre();
			world.y = 0.f;
			got = true;
		}
		else
		{
			DataSectionPtr pBBSect = BWResource::openSection(
				mapping->path() + chunkName + ".chunk/boundingBox" );
			if( pBBSect )
			{
				BoundingBox bb( pBBSect->readVector3( "min" ),
					pBBSect->readVector3( "max" ) );
				world = bb.centre();
				if( bb.minBounds().y + 0.5 < world.y )
					world.y = bb.minBounds().y + 0.5f;
				got = true;
			}
		}
		if( got )
		{
			Matrix view = WorldEditorCamera::instance().currentCamera().view();
			view.setTranslate( world );
			view.preRotateX( DEG_TO_RAD( 90.f ) );
			view.invert();
			WorldEditorCamera::instance().currentCamera().view( view );

			if( Options::getOptionInt( "camera/ortho" ) == WorldEditorCamera::CT_Orthographic )
			{
				WorldEditorCamera::instance().changeToCamera( WorldEditorCamera::CT_MouseLook );
				WorldEditorCamera::instance().changeToCamera( WorldEditorCamera::CT_Orthographic );
			}

			PanelManager::instance().setDefaultToolMode();
		}
		else
		{
			ERROR_MSG("\"%S\" is not a valid chunk\n",
				chunkName_cstr.GetString() );
		}
	}
}


void PageOptionsNavigation::OnBnClickedOptionsLocationAdd()
{
	BW_GUARD;

	locationList_.doAdd();
}

void PageOptionsNavigation::OnBnClickedOptionsLocationRename()
{
	BW_GUARD;

	locationList_.doRename();
}

void PageOptionsNavigation::OnBnClickedOptionsLocationUpdate()
{
	BW_GUARD;

	locationList_.doUpdate();
}


HBRUSH PageOptionsNavigation::OnCtlColor( CDC* pDC, CWnd* pWnd, UINT nCtlColor )
{
	BW_GUARD;

	HBRUSH brush = CFormView::OnCtlColor( pDC, pWnd, nCtlColor );

	cameraHeightEdit_.SetBoundsColour( pDC, pWnd,
		cameraHeightEdit_.GetMinimum(), cameraHeightEdit_.GetMaximum() );

	return brush;
}


LRESULT PageOptionsNavigation::OnSearchFieldChanged( WPARAM wParam, LPARAM lParam )
{
	BW_GUARD;

	//convert it to lowercase
	BW::string lowerSearch = bw_wtoutf8( search_.searchText() );
	std::transform( lowerSearch.begin(), lowerSearch.end(), lowerSearch.begin(), tolower );

	locationList_.setSearchString( lowerSearch );

	locationList_.redrawLocationsList();
	return 0;
}


void PageOptionsNavigation::OnBnClickedOptionsLocationRemove()
{
	BW_GUARD;

	locationList_.doRemove();
}

void PageOptionsNavigation::OnBnClickedOptionsLocationMove()
{
	BW_GUARD;

	locationList_.doMove();
}

IMPLEMENT_DYNAMIC(OptionsLocationsTree, CTreeCtrl)
OptionsLocationsTree::OptionsLocationsTree():
	search_str_("")
{
}

void OptionsLocationsTree::setSearchString( const BW::string& searchString )
{
	search_str_ = searchString;
}

BEGIN_MESSAGE_MAP(OptionsLocationsTree, CTreeCtrl)
	ON_NOTIFY_REFLECT(TVN_ENDLABELEDIT, &OptionsLocationsTree::OnTvnEndlabeleditOptionsLocationList)
	ON_NOTIFY_REFLECT(NM_DBLCLK, &OptionsLocationsTree::OnNMDblclkOptionsLocationList)
	ON_NOTIFY_REFLECT(TVN_KEYDOWN, &OptionsLocationsTree::OnTvnKeydownOptionsLocationList)
END_MESSAGE_MAP()

void OptionsLocationsTree::OnTvnEndlabeleditOptionsLocationList(NMHDR *pNMHDR, LRESULT *pResult)
{
	BW_GUARD;

	bool needsRedraw = false;

	LPNMTVDISPINFO pTVDispInfo = reinterpret_cast<LPNMTVDISPINFO>(pNMHDR);

	HTREEITEM item = pTVDispInfo->item.hItem;

	if (!item) return;

	BW::string oldLocationName = bw_wtoutf8( GetItemText( item ).GetString() );

	if (!pTVDispInfo->item.pszText) return;

	BW::string newLocationName = bw_wtoutf8( pTVDispInfo->item.pszText );

	if (locations_.find(newLocationName) != locations_.end())
	{
		if (::MessageBox( AfxGetApp()->m_pMainWnd->GetSafeHwnd(),
			Localise(L"WORLDEDITOR/GUI/PAGE_OPTIONS_NAVIGATION/OVERWRITTEN_LOCATION_MARK_TEXT", newLocationName),
			Localise(L"WORLDEDITOR/GUI/PAGE_OPTIONS_NAVIGATION/OVERWRITTEN_LOCATION_MARK_TITLE"),
			MB_YESNO) == IDNO)
			return;

		needsRedraw = true;
	}

	SetItemText( item, pTVDispInfo->item.pszText );

	BW::vector<DataSectionPtr> bookmarks;
	locationData_->openSections( "bookmark", bookmarks );
	for ( size_t i = 0; i < bookmarks.size(); i++ )
	{
		if ( bookmarks[i]->readString( "name", "" ) == oldLocationName )
		{
			bookmarks[i]->writeString( "name", newLocationName );
			break;
		}
	}
	locations_.erase( oldLocationName );
	locations_.insert( newLocationName );

	locationData_->save();

	if ( !needsRedraw )
	{
		SortChildren( NULL );
	}
	else
	{
		redrawLocationsList();
	}

	item = GetRootItem();
	// TODO:UNICODE should this be case sensitive?
	while ((item) && (bw_MW_strcmp(GetItemText(item),newLocationName.c_str())))
	{
		item = GetNextSiblingItem( item );
	}
	if (item)
	{
		SelectItem(item);
	}

	*pResult = 0;

	return;
}

void OptionsLocationsTree::OnNMDblclkOptionsLocationList(NMHDR *pNMHDR, LRESULT *pResult)
{
	BW_GUARD;

	doMove();

	*pResult = 0;
}

void OptionsLocationsTree::OnTvnKeydownOptionsLocationList(NMHDR *pNMHDR, LRESULT *pResult)
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

void OptionsLocationsTree::updateLocationsList( const BW::string& spaceName )
{
	BW_GUARD;

	locations_.clear();

	locationData_ = BWResource::openSection(
		spaceName + "/locations.xml", true );

	int numChildren = locationData_->countChildren();
	for( int i = 0; i < numChildren; ++i )
	{
		BW::string locationName = locationData_->openChild(i)->sectionName();
		if ( locationName == "bookmark" )
			locations_.insert( locationData_->openChild(i)->readString( "name", "" ) );
		else
		{
			DataSectionPtr newBookmark = locationData_->newSection( "bookmark" );
			newBookmark->writeString( "name", locationName );
			newBookmark->writeMatrix34( "view", locationData_->readMatrix34( locationName, Matrix::identity ) );
			locations_.insert( locationName );
		}
	}

	for ( int i = 0; i < locationData_->countChildren(); ++i )
	{
		BW::string locationName = locationData_->openChild(i)->sectionName();
		if ( locationName != "bookmark" )
		{
			--i;
			locationData_->deleteSection( locationName );
		}
	}

	redrawLocationsList();
}

void OptionsLocationsTree::redrawLocationsList()
{
	BW_GUARD;

	HTREEITEM oldItem = GetSelectedItem();
	CString oldItemText = L"";
	if (oldItem)
	{
		oldItemText = GetItemText( GetSelectedItem() );
	}

	DeleteAllItems();

	BW::set< BW::string >::iterator it = locations_.begin();
	BW::set< BW::string >::iterator end = locations_.end();
	for(; it != end; ++it)
	{
		BW::string locationLower = *it;

		//convert it to lowercase
		std::transform (
			locationLower.begin(),
			locationLower.end(),
			locationLower.begin(),
			tolower);

		if (locationLower.find( search_str_, 0 ) != BW::string::npos)
		{
			InsertItem( bw_utf8tow(*it).c_str() );
		}
	}

	SortChildren( NULL );

	if (!oldItem) return;

	//Select the previously selected item
	HTREEITEM item = GetRootItem();
	while ((item) && (wcscmp( GetItemText( item ), oldItemText )))
	{
		item = GetNextSiblingItem( item );
	}
	if (item)
	{
		SelectItem( item );
	}
}

void OptionsLocationsTree::doAdd()
{
	BW_GUARD;

	BW::string newName = LocaliseUTF8(L"WORLDEDITOR/GUI/PAGE_OPTIONS_NAVIGATION/UNTITLED");
	int i = 2;
	while (locations_.find( newName ) != locations_.end())
	{
		newName = LocaliseUTF8(L"WORLDEDITOR/GUI/PAGE_OPTIONS_NAVIGATION/UNTITLED_D", i);
		++i;
	}

	HTREEITEM item = InsertItem( bw_utf8tow( newName ).c_str() );

	DataSectionPtr newBookmark = locationData_->newSection( "bookmark" );
	newBookmark->writeString( "name", newName );
	newBookmark->writeMatrix34( "view", WorldEditorCamera::instance().currentCamera().view() );

	locations_.insert( newName );

	locationData_->save();

	EditLabel( item );
}

void OptionsLocationsTree::doRename()
{
	BW_GUARD;

	SetFocus();
	EditLabel( GetSelectedItem() );
}

void OptionsLocationsTree::doUpdate()
{
	BW_GUARD;

	BW::wstring locationName = GetItemText( GetSelectedItem() );

	BW::vector<DataSectionPtr> bookmarks;
	locationData_->openSections( "bookmark", bookmarks );
	for ( size_t i = 0; i < bookmarks.size(); i++ )
	{
		if ( bookmarks[i]->readWideString( "name", L"" ) == locationName )
		{
			bookmarks[i]->writeMatrix34( "view", WorldEditorCamera::instance().currentCamera().view() );
			break;
		}
	}

	locationData_->save();
}

void OptionsLocationsTree::doRemove()
{
	BW_GUARD;

	HTREEITEM item = GetSelectedItem();

	BW::string locationName = bw_wtoutf8( GetItemText( item ).GetString() );

	BW::vector<DataSectionPtr> bookmarks;
	locationData_->openSections( "bookmark", bookmarks );
	for ( size_t i = 0; i < bookmarks.size(); i++ )
	{
		if ( bookmarks[i]->readString( "name", "" ) == locationName )
		{
			locationData_->delChild( bookmarks[i] );
			break;
		}
	}

	DeleteItem( item );

	locations_.erase( locationName );

	locationData_->save();
}

void OptionsLocationsTree::doMove()
{
	BW_GUARD;

	HTREEITEM item = GetSelectedItem();

	BW::wstring locationName = GetItemText( item );

	BW::vector<DataSectionPtr> bookmarks;
	locationData_->openSections( "bookmark", bookmarks );
	for ( size_t i = 0; i < bookmarks.size(); i++ )
	{
		if ( bookmarks[i]->readWideString( "name", L"" ) == locationName )
		{
			WorldEditorCamera::instance().currentCamera().view(
				bookmarks[i]->readMatrix34( "view", WorldEditorCamera::instance().currentCamera().view() ) );
			break;
		}
	}
}
BW_END_NAMESPACE

