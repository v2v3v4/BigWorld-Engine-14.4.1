#include "pch.hpp"
#include "worldeditor/gui/pages/page_terrain_filter.hpp"
#include "worldeditor/framework/world_editor_app.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "worldeditor/misc/matrix_filter.hpp"
#include "worldeditor/scripting/we_python_adapter.hpp"
#include "appmgr/options.hpp"
#include "common/user_messages.hpp"
#include "guimanager/gui_manager.hpp"
#include "resmgr/dataresource.hpp"
#include "gizmo/tool_manager.hpp"


DECLARE_DEBUG_COMPONENT( 0 )

BW_BEGIN_NAMESPACE

// GUITABS content ID ( declared by the IMPLEMENT_BASIC_CONTENT macro )
const BW::wstring PageTerrainFilter::contentID = L"PageTerrainFilter";


PageTerrainFilter::PageTerrainFilter()
	: PageTerrainBase(PageTerrainFilter::IDD),
	pageReady_( false ),
	lastFilterIndex_( -1 )
{
    sizeEdit_.SetNumericType(controls::EditNumeric::ENT_INTEGER);
}

PageTerrainFilter::~PageTerrainFilter()
{
}

void PageTerrainFilter::DoDataExchange(CDataExchange* pDX)
{
	PageTerrainBase::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TERRAIN_SIZE_EDIT, sizeEdit_);
	DDX_Control(pDX, IDC_TERRAIN_FALLOFF_LINEAR, falloffLinear_);
	DDX_Control(pDX, IDC_TERRAIN_FALLOFF_CURVE, falloffCurve_);
	DDX_Control(pDX, IDC_TERRAIN_FALLOFF_FLAT, falloffFlat_);
	DDX_Control(pDX, IDC_TERRAIN_FILTERS_LIST, filtersList_);
	DDX_Control(pDX, IDC_TERRAIN_SIZE_SLIDER, sizeSlider_);
	DDX_Control(pDX, IDC_SIZERANGE, mSizeRange);
}


BOOL PageTerrainFilter::OnInitDialog()
{
	BW_GUARD;

	PageTerrainBase::OnInitDialog();

	sizeSlider_.setRangeLimit( Options::getOptionFloat( "terrain/filter/minsizelimit", 1 ),
		Options::getOptionFloat( "terrain/filter/maxsizelimit", 4000 ) );
	sizeSlider_.setRange( Options::getOptionFloat( "terrain/filter/minsize", 1 ),
		Options::getOptionFloat( "terrain/filter/maxsize", 1000 ) );
	sizeSlider_.setValue( Options::getOptionFloat( "terrain/filter/size", 1 ) );
	sizeSlider_.setPageSteps( 20 );

	loadFilters();

	OnUpdateControls(0, 0);
	updateSliderEdits();

	mSizeRange.SetWindowPos( NULL, 0, 0, 16, 16, SWP_NOMOVE | SWP_NOZORDER );
	mSizeRange.setBitmapID( IDB_RANGESLIDER, IDB_RANGESLIDER );

	pageReady_ = true;
	INIT_AUTO_TOOLTIP();
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}


void PageTerrainFilter::loadFilters()
{
	BW_GUARD;

	filtersList_.ResetContent();
    
	for( size_t i = 0; i < MatrixFilter::instance().size(); ++i )
	{
		if ( MatrixFilter::instance().filter( i ).included_ )
		{
			int idx = filtersList_.AddString( bw_utf8tow(
				MatrixFilter::instance().filter( i ).name_ ).c_str() );
			filtersList_.SetItemData( idx, DWORD_PTR( i ) );
		}
    }
}



BEGIN_MESSAGE_MAP(PageTerrainFilter, PageTerrainBase)
	ON_WM_SIZE()
	ON_WM_HSCROLL()
	ON_WM_CTLCOLOR()
	ON_MESSAGE (WM_ACTIVATE_TOOL, OnActivateTool)
	ON_MESSAGE (WM_UPDATE_CONTROLS, OnUpdateControls)
	ON_LBN_SELCHANGE(IDC_TERRAIN_FILTERS_LIST, OnLbnSelchangeTerrainFiltersList)
	ON_WM_MOUSEMOVE()
	ON_EN_CHANGE(IDC_TERRAIN_SIZE_EDIT, OnEnChangeTerrainSizeEdit)
	ON_BN_CLICKED(IDC_SIZERANGE, OnBnClickedSizerange)
END_MESSAGE_MAP()


// PageTerrainFilter message handlers

LRESULT PageTerrainFilter::OnActivateTool(WPARAM wParam, LPARAM lParam)
{
	BW_GUARD;

	const wchar_t * activePageId = (const wchar_t *)(wParam);
	if (getContentID() == activePageId)
	{
		if (WorldEditorApp::instance().pythonAdapter())
		{
			WorldEditorApp::instance().pythonAdapter()->onPageControlTabSelect("pgcTerrain", "TerrainFilters");
		}
	}
	return 0;	
}


afx_msg void PageTerrainFilter::OnSize( UINT nType, int cx, int cy )
{
	BW_GUARD;

	PageTerrainBase::OnSize( nType, cx, cy );

	if ( !pageReady_ )
		return;

	// move the controls as appropriate
	// stretch the file selector to be long enough to fit into the window
	CRect rect;
	GetClientRect(rect);
	static const int MARGIN = 4;
	const int newWidth = std::max( 1, rect.Width() - MARGIN * 2 );
	
	CRect sliderRect, buttonRect;
	sizeSlider_.GetWindowRect(sliderRect);
	mSizeRange.GetWindowRect(buttonRect);
	ScreenToClient( &sliderRect );
	ScreenToClient( &buttonRect );

	mSizeRange.SetWindowPos(NULL, rect.Width() - MARGIN - buttonRect.Width(), buttonRect.top,
		0, 0, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSIZE);

	sizeSlider_.SetWindowPos(NULL, 0, 0,
		newWidth - buttonRect.Width() - MARGIN - sliderRect.left, sliderRect.Height(),
		SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE);

	filtersList_.GetWindowRect(sliderRect);
	ScreenToClient( &sliderRect );
	filtersList_.SetWindowPos(NULL, 0, 0,
		newWidth, cy - sliderRect.top - MARGIN, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE);
}

void PageTerrainFilter::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	BW_GUARD;

	// this captures all the scroll events for the page
	// upadate all the slider buddy windows
	updateSliderEdits();

	Options::setOptionFloat( "terrain/filter/size", sizeSlider_.getValue() );

	ToolPtr tool = ToolManager::instance().tool();
	
	if (tool)
	{
		tool->findRelevantChunks();
	}

	PageTerrainBase::OnHScroll(nSBCode, nPos, pScrollBar);
}

void PageTerrainFilter::updateSliderEdits()
{
	BW_GUARD;

	int new_val = (int)sizeSlider_.getValue();
	int old_val = (int)sizeEdit_.GetIntegerValue();
	if (GetFocus() != &sizeEdit_ && new_val != old_val)
	{
		sizeEdit_.SetIntegerValue( new_val );
	}
}

afx_msg LRESULT PageTerrainFilter::OnUpdateControls(WPARAM wParam, LPARAM lParam)
{
	BW_GUARD;

	sizeSlider_.setValue( Options::getOptionFloat( "terrain/filter/size", 10 ) );

	if( lastFilterIndex_ != Options::getOptionInt( "terrain/filter/index", 0 ) )
	{
		lastFilterIndex_ = Options::getOptionInt( "terrain/filter/index", 0 );
		bool found = false;
		for ( int i = 0; i < filtersList_.GetCount(); ++i )
		{
			if ( filtersList_.GetItemData( i ) == Options::getOptionInt( "terrain/filter/index", 0 ) )
			{
				filtersList_.SetCurSel( i );
				found = true;
				break;
			}
		}
		if ( !found && filtersList_.GetCount() > 0 )
		{
			// Select the first filter by default.
			filtersList_.SetCurSel( 0 );
			DWORD_PTR itemData = filtersList_.GetItemData( 0 );
			MF_ASSERT( itemData <= UINT_MAX );
			Options::setOptionInt(
				"terrain/filter/index", ( uint ) itemData );
		}
	}

	updateSliderEdits();

	Options::setOptionFloat( "terrain/filter/minsize", sizeSlider_.getMinRange() );
	Options::setOptionFloat( "terrain/filter/maxsize", sizeSlider_.getMaxRange() );

	return 0;
}

void PageTerrainFilter::OnLbnSelchangeTerrainFiltersList()
{
	BW_GUARD;

	DWORD_PTR itemData = filtersList_.GetItemData( filtersList_.GetCurSel() );
	MF_ASSERT( itemData <= UINT_MAX );
	lastFilterIndex_ = ( uint ) itemData;
	Options::setOptionInt( "terrain/filter/index", lastFilterIndex_ );
}

BOOL PageTerrainFilter::PreTranslateMessage(MSG* pMsg)
{
	BW_GUARD;

	//Handle tooltips first...
	CALL_TOOLTIPS( pMsg );

	if (GetFocus() == &filtersList_)
	{
		if(pMsg->message == WM_KEYDOWN)
		{
			if(pMsg->wParam == VK_TAB)
			{
				int current = filtersList_.GetCurSel();
				if (++current >= filtersList_.GetCount())
					current = 0;
                filtersList_.SetCurSel( current );
				return true;
			}
		}
	}

	return PageTerrainBase::PreTranslateMessage(pMsg);
}

void PageTerrainFilter::OnMouseMove(UINT nFlags, CPoint point)
{
	BW_GUARD;

	// TODO: Add your message handler code here and/or call default

	__super::OnMouseMove(nFlags, point);
}

void PageTerrainFilter::OnEnChangeTerrainSizeEdit()
{
	BW_GUARD;

	sizeSlider_.setValue( static_cast<float>(sizeEdit_.GetIntegerValue()) );
	Options::setOptionFloat( "terrain/filter/size", sizeSlider_.getValue() );
}

void PageTerrainFilter::OnBnClickedSizerange()
{
	BW_GUARD;

	sizeSlider_.beginEdit();
}

/**
 *  This is called when each item is about to be drawn.  We want limit slider edit
 *	to be highlighted is they are out of bounds.
 *
 *	@param pDC	Contains a pointer to the display context for the child window.
 *	@param pWnd	Contains a pointer to the control asking for the color.
 *	@param nCtlColor	Specifies the type of control.
 *	@return	A handle to the brush that is to be used for painting the control
 *			background.
 */
HBRUSH PageTerrainFilter::OnCtlColor( CDC* pDC, CWnd* pWnd, UINT nCtlColor )
{
	BW_GUARD;

	HBRUSH brush = PageTerrainBase::OnCtlColor( pDC, pWnd, nCtlColor );

	sizeEdit_.SetBoundsColour( pDC, pWnd,
		sizeSlider_.getMinRangeLimit(),	sizeSlider_.getMaxRangeLimit() );

	return brush;
}

BW_END_NAMESPACE

