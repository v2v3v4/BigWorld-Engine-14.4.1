#include "pch.hpp"
#include "worldeditor/gui/pages/page_terrain_height.hpp"
#include "worldeditor/gui/pages/panel_manager.hpp"
#include "worldeditor/framework/world_editor_app.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "worldeditor/scripting/we_python_adapter.hpp"
#include "worldeditor/terrain/editor_chunk_terrain.hpp"
#include "terrain/terrain_height_filter_functor.hpp"
#include "appmgr/options.hpp"
#include "common/user_messages.hpp"
#include "guimanager/gui_manager.hpp"
#include "controls/user_messages.hpp"
#include "chunk/chunk.hpp"
#include "chunk/chunk_manager.hpp"
#include "chunk/chunk_space.hpp"
#include "gizmo/tool_manager.hpp"

BW_BEGIN_NAMESPACE

// GUITABS content ID ( declared by the IMPLEMENT_BASIC_CONTENT macro )
const BW::wstring PageTerrainHeight::contentID = L"PageTerrainHeight";


PageTerrainHeight::PageTerrainHeight()
	: PageTerrainBase(PageTerrainHeight::IDD),
	pageReady_( false ),
	filterControls_( 0 )
{
	strengthEdit_.SetNumericType(controls::EditNumeric::ENT_INTEGER);
}

PageTerrainHeight::~PageTerrainHeight()
{
}

void PageTerrainHeight::DoDataExchange(CDataExchange* pDX)
{
	PageTerrainBase::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TERRAIN_SIZE_SLIDER, sizeSlider_);
	DDX_Control(pDX, IDC_TERRAIN_SIZE_EDIT, sizeEdit_);
	DDX_Control(pDX, IDC_TERRAIN_STRENGTH_SLIDER, strengthSlider_);
	DDX_Control(pDX, IDC_TERRAIN_STRENGTH_EDIT, strengthEdit_);
	DDX_Control(pDX, IDC_TERRAIN_HEIGHT_SLIDER, heightSlider_);
	DDX_Control(pDX, IDC_TERRAIN_HEIGHT_EDIT, heightEdit_);
	DDX_Control(pDX, IDC_TERRAIN_FALLOFF_LINEAR, falloffLinear_);
	DDX_Control(pDX, IDC_TERRAIN_FALLOFF_CURVE, falloffCurve_);
	DDX_Control(pDX, IDC_TERRAIN_FALLOFF_FLAT, falloffFlat_);
	DDX_Control(pDX, IDC_TERRAIN_FALLOFF_AVERAGE, falloffAverage_);
	DDX_Control(pDX, IDC_TERRAIN_ABSOLUTEHEIGHT, absolute_);
	DDX_Control(pDX, IDC_TERRAIN_RELATIVEHEIGHT, relative_);
	DDX_Control(pDX, IDC_SIZERANGE, mSizeRange);
	DDX_Control(pDX, IDC_STRENGTHRANGE, mStrengthRange);
	DDX_Control(pDX, IDC_HEIGHTRANGE, mHeightRange);
}


BOOL PageTerrainHeight::OnInitDialog()
{
	BW_GUARD;

	PageTerrainBase::OnInitDialog();

	++filterControls_;

	sizeSlider_.setDigits(2);
	sizeSlider_.setRangeLimit( Options::getOptionFloat( "terrain/height/minsizelimit", 0.8f ),
		Options::getOptionFloat( "terrain/height/maxsizelimit", 4000 ) );
	sizeSlider_.setRange( Options::getOptionFloat( "terrain/height/minsize", 1 ),
		Options::getOptionFloat( "terrain/height/maxsize", 800 ) );
	sizeSlider_.setValue( Options::getOptionFloat( "terrain/height/size", 1 ) );	
	sizeSlider_.setPageSteps( 20 );
	sizeEdit_.SetNumDecimals(2);
	sizeEdit_.SetValue(sizeSlider_.getValue());

	strengthSlider_.setRangeLimit( Options::getOptionFloat( "terrain/height/minstrengthlimit", 1 ),
		Options::getOptionFloat( "terrain/height/maxstrengthlimit", 4000 ) );
	strengthSlider_.setRange( Options::getOptionFloat( "terrain/height/minstrength", 1 ),
		Options::getOptionFloat( "terrain/height/maxstrength", 800 ) );
	strengthSlider_.setValue( Options::getOptionFloat( "terrain/height/strength", 1 ) );
	strengthSlider_.setPageSteps( 20 );
	strengthEdit_.SetIntegerValue( BW_ROUND_TO_INT( strengthSlider_.getValue() ) );

	heightSlider_.setDigits(2);
	heightSlider_.setRangeLimit( Options::getOptionFloat( "terrain/height/minheightlimit", 1 ),
		Options::getOptionFloat( "terrain/height/maxheightlimit", 4000 ) );
	heightSlider_.setRange( Options::getOptionFloat( "terrain/height/minheight", 1 ),
		Options::getOptionFloat( "terrain/height/maxheight", 800 ) );
	heightSlider_.setValue( Options::getOptionFloat( "terrain/height/height", 1 ) );	
	heightSlider_.setPageSteps( 20 );
	heightEdit_.SetValue( heightSlider_.getValue() );
	heightEdit_.SetNumDecimals(2);

	OnUpdateControls(0, 0);

	mSizeRange.SetWindowPos( NULL, 0, 0, 16, 16, SWP_NOMOVE | SWP_NOZORDER );
	mStrengthRange.SetWindowPos( NULL, 0, 0, 16, 16, SWP_NOMOVE | SWP_NOZORDER );
	mHeightRange.SetWindowPos( NULL, 0, 0, 16, 16, SWP_NOMOVE | SWP_NOZORDER );
	mSizeRange.setBitmapID( IDB_RANGESLIDER, IDB_RANGESLIDER );
	mStrengthRange.setBitmapID( IDB_RANGESLIDER, IDB_RANGESLIDER );
	mHeightRange.setBitmapID( IDB_RANGESLIDER, IDB_RANGESLIDER );

	--filterControls_;

	pageReady_ = true;
	INIT_AUTO_TOOLTIP();
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}


BEGIN_MESSAGE_MAP(PageTerrainHeight, PageTerrainBase)
	ON_WM_SIZE()
	ON_WM_HSCROLL()
	ON_WM_CTLCOLOR()
	ON_MESSAGE (WM_ACTIVATE_TOOL, OnActivateTool)
	ON_MESSAGE (WM_UPDATE_CONTROLS, OnUpdateControls)
	ON_BN_CLICKED(IDC_TERRAIN_FALLOFF_LINEAR, OnBnClickedTerrainFalloffLinear)
	ON_BN_CLICKED(IDC_TERRAIN_FALLOFF_CURVE, OnBnClickedTerrainFalloffCurve)
	ON_BN_CLICKED(IDC_TERRAIN_FALLOFF_FLAT, OnBnClickedTerrainFalloffFlat)
	ON_BN_CLICKED(IDC_TERRAIN_FALLOFF_AVERAGE, OnBnClickedTerrainFalloffAverage)
	ON_MESSAGE (WM_EDITNUMERIC_CHANGE, OnChangeEditNumeric)
	ON_BN_CLICKED(IDC_TERRAIN_ABSOLUTEHEIGHT, OnBnClickedTerrainAbsoluteheight)
	ON_BN_CLICKED(IDC_TERRAIN_RELATIVEHEIGHT, OnBnClickedTerrainRelativeheight)
	ON_BN_CLICKED(IDC_SIZERANGE, OnBnClickedSizerange)
	ON_BN_CLICKED(IDC_STRENGTHRANGE, OnBnClickedStrengthrange)
	ON_BN_CLICKED(IDC_HEIGHTRANGE, OnBnClickedHeightrange)
	ON_EN_CHANGE(IDC_TERRAIN_SIZE_EDIT, OnEnChangeTerrainSizeEdit)
	ON_EN_CHANGE(IDC_TERRAIN_STRENGTH_EDIT, OnEnChangeTerrainStrengthEdit)
	ON_EN_CHANGE(IDC_TERRAIN_HEIGHT_EDIT, OnEnChangeTerrainHeightEdit)
END_MESSAGE_MAP()


// PageTerrainHeight message handlers

LRESULT PageTerrainHeight::OnActivateTool(WPARAM wParam, LPARAM lParam)
{
	BW_GUARD;

	const wchar_t * activePageId = (const wchar_t *)(wParam);
	if (getContentID() == activePageId)
	{
		if (WorldEditorApp::instance().pythonAdapter())
		{
			WorldEditorApp::instance().pythonAdapter()->onPageControlTabSelect("pgcTerrain", "TerrainBrushes");
		}
	}

	return 0;
}


afx_msg void PageTerrainHeight::OnSize( UINT nType, int cx, int cy )
{
	BW_GUARD;

	PageTerrainBase::OnSize( nType, cx, cy );

	if ( !pageReady_ )
		return;

	// move the controls as appropriate
	// stretch the file selector to be long enough to fit into the window
	CRect rect;
	GetWindowRect(rect);
	static const int MARGIN = 10;
	const int newWidth = std::max(1, rect.Width() - MARGIN * 2 );
	
	CRect sliderRect, buttonRect;
	sizeSlider_.GetWindowRect(sliderRect);
	mSizeRange.GetWindowRect(buttonRect);
	ScreenToClient( &sliderRect );
	ScreenToClient( &buttonRect );

	mSizeRange.SetWindowPos(NULL, rect.Width() - buttonRect.Width(), buttonRect.top,
		0, 0, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSIZE);

	sizeSlider_.SetWindowPos(NULL, 0, 0,
		newWidth - buttonRect.Width() - sliderRect.left, sliderRect.Height(),
		SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE);

	strengthSlider_.GetWindowRect(sliderRect);
	mStrengthRange.GetWindowRect(buttonRect);
	ScreenToClient( &sliderRect );
	ScreenToClient( &buttonRect );

	mStrengthRange.SetWindowPos(NULL, rect.Width() - buttonRect.Width(), buttonRect.top,
		0, 0, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSIZE);

	strengthSlider_.SetWindowPos(NULL, 0, 0,
		newWidth - buttonRect.Width() - sliderRect.left, sliderRect.Height(),
		SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE);

	heightSlider_.GetWindowRect(sliderRect);
	mHeightRange.GetWindowRect(buttonRect);
	ScreenToClient( &sliderRect );
	ScreenToClient( &buttonRect );

	mHeightRange.SetWindowPos(NULL, rect.Width() - buttonRect.Width(), buttonRect.top,
		0, 0, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSIZE);

	heightSlider_.SetWindowPos(NULL, 0, 0,
		newWidth - buttonRect.Width() - sliderRect.left, sliderRect.Height(),
		SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE);
}

void PageTerrainHeight::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	BW_GUARD;

	// this captures all the scroll events for the page
	// upadate all the slider buddy windows
	updateSliderEdits();

	Options::setOptionFloat( "terrain/height/size", sizeSlider_.getValue() );
	Options::setOptionFloat( "terrain/height/strength", strengthSlider_.getValue() );
	Options::setOptionFloat( "terrain/height/height", heightSlider_.getValue() );

	BW::ToolPtr tool = ToolManager::instance().tool();

	if (tool)
	{
		tool->findRelevantChunks();
	}

	PageTerrainBase::OnHScroll(nSBCode, nPos, pScrollBar);
}

void PageTerrainHeight::updateSliderEdits()
{
	BW_GUARD;

	if (filterControls_ == 0)
	{
		++filterControls_;
		float new_val = sizeSlider_.getValue();
		float old_val = sizeEdit_.GetValue();
		if (GetFocus() != &sizeEdit_ && new_val != old_val)
		{
			sizeEdit_.SetValue( new_val );
		}
		new_val = strengthSlider_.getValue();
		old_val = strengthEdit_.GetValue();
		if (GetFocus() != &strengthEdit_ && new_val != old_val)
		{
			strengthEdit_.SetValue( new_val );
		}
		new_val = heightSlider_.getValue();
		old_val = heightEdit_.GetValue();
		if (GetFocus() != &heightEdit_ && new_val != old_val)
		{
			heightEdit_.SetValue( new_val );
		}
		--filterControls_;
	}
}

afx_msg LRESULT PageTerrainHeight::OnUpdateControls(WPARAM wParam, LPARAM lParam)
{
	BW_GUARD;

	if( !ChunkManager::instance().cameraChunk() )
		return 0;
	EditorChunkTerrain* pEct = static_cast<EditorChunkTerrain*>(
		ChunkTerrainCache::instance( *ChunkManager::instance().cameraChunk() ).pTerrain());
	if( !pEct )
		return 0;

	++filterControls_;
	sizeSlider_.setValue( Options::getOptionFloat( "terrain/height/size", 1 ) );
	strengthSlider_.setValue( Options::getOptionFloat( "terrain/height/strength", 1 ) );
	heightSlider_.setValue( Options::getOptionFloat( "terrain/height/height", 1 ) );
	--filterControls_;

	updateSliderEdits();

	Options::setOptionFloat( "terrain/height/minsize", sizeSlider_.getMinRange() );
	Options::setOptionFloat( "terrain/height/maxsize", sizeSlider_.getMaxRange() );

	Options::setOptionFloat( "terrain/height/minstrength", strengthSlider_.getMinRange() );
	Options::setOptionFloat( "terrain/height/maxstrength", strengthSlider_.getMaxRange() );

	Options::setOptionFloat( "terrain/height/minheight", heightSlider_.getMinRange() );
	Options::setOptionFloat( "terrain/height/maxheight", heightSlider_.getMaxRange() );

	++filterControls_;

	falloffFlat_.SetCheck( Options::getOptionInt( "terrain/height/brushFalloff" ) == TerrainHeightFilterFunctor::FLAT_FALLOFF );
	falloffLinear_.SetCheck( Options::getOptionInt( "terrain/height/brushFalloff" ) == TerrainHeightFilterFunctor::LINEAR_FALLOFF );
	falloffCurve_.SetCheck( Options::getOptionInt( "terrain/height/brushFalloff" ) == TerrainHeightFilterFunctor::CURVE_FALLOFF );
	falloffAverage_.SetCheck( Options::getOptionInt( "terrain/height/brushFalloff" ) == TerrainHeightFilterFunctor::AVERAGE_FALLOFF );

	absolute_.SetCheck( !Options::getOptionInt( "terrain/height/relative" ) );
	relative_.SetCheck( Options::getOptionInt( "terrain/height/relative" ) );

    if (PanelManager::instance().isCurrentTool( PageTerrainHeight::contentID ))
    {
	    if( InputDevices::isAltDown() &&			
		    ToolManager::instance().tool() && ToolManager::instance().tool()->locator() )
	    {
		    static HCURSOR cursor = AfxGetApp()->LoadCursor( IDC_HEIGHTPICKER );
		    WorldManager::instance().setCursor( cursor );
			if (InputDevices::isKeyDown( KeyCode::KEY_LEFTMOUSE ))
			{
				Vector3 pos = ToolManager::instance().tool()->locator()->transform().applyToOrigin();
				heightEdit_.SetValue(pos.y);
				heightSlider_.setValue(pos.y);
			}
	    }
	    else 
        {
		    WorldManager::instance().resetCursor();
        }
    }

	--filterControls_;

	return 0;
}

void PageTerrainHeight::OnBnClickedTerrainFalloffLinear()
{
	BW_GUARD;

	Options::setOptionInt( "terrain/height/brushFalloff", TerrainHeightFilterFunctor::LINEAR_FALLOFF );
}

void PageTerrainHeight::OnBnClickedTerrainFalloffCurve()
{
	BW_GUARD;

	Options::setOptionInt( "terrain/height/brushFalloff", TerrainHeightFilterFunctor::CURVE_FALLOFF );
}

void PageTerrainHeight::OnBnClickedTerrainFalloffFlat()
{
	BW_GUARD;

	Options::setOptionInt( "terrain/height/brushFalloff", TerrainHeightFilterFunctor::FLAT_FALLOFF );
}

void PageTerrainHeight::OnBnClickedTerrainFalloffAverage()
{
	BW_GUARD;

	Options::setOptionInt( "terrain/height/brushFalloff", TerrainHeightFilterFunctor::AVERAGE_FALLOFF );
}

afx_msg LRESULT PageTerrainHeight::OnChangeEditNumeric(WPARAM wParam, LPARAM lParam)
{
	BW_GUARD;

	if (filterControls_ == 0)
	{
		++filterControls_;
		heightSlider_.setValue( static_cast<float>( heightEdit_.GetIntegerValue() ) );
		Options::setOptionFloat( "terrain/height/height", heightSlider_.getValue() );
		--filterControls_;
	}

	return 0;
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
afx_msg HBRUSH PageTerrainHeight::OnCtlColor( CDC* pDC, CWnd* pWnd, UINT nCtlColor )
{
	BW_GUARD;

	HBRUSH brush = PageTerrainBase::OnCtlColor( pDC, pWnd, nCtlColor );

	sizeEdit_.SetBoundsColour( pDC, pWnd,
		sizeSlider_.getMinRangeLimit(),	sizeSlider_.getMaxRangeLimit() );
	strengthEdit_.SetBoundsColour( pDC, pWnd,
		strengthSlider_.getMinRangeLimit(), strengthSlider_.getMaxRangeLimit() );
	heightEdit_.SetBoundsColour( pDC, pWnd,
		heightSlider_.getMinRangeLimit(), heightSlider_.getMaxRangeLimit() );

	return brush;
}

void PageTerrainHeight::OnBnClickedTerrainAbsoluteheight()
{
	BW_GUARD;

	Options::setOptionInt( "terrain/height/relative", 0 );
}

void PageTerrainHeight::OnBnClickedTerrainRelativeheight()
{
	BW_GUARD;

	Options::setOptionInt( "terrain/height/relative", 1 );
}

void PageTerrainHeight::OnBnClickedSizerange()
{
	BW_GUARD;

	sizeSlider_.beginEdit();
}

void PageTerrainHeight::OnBnClickedStrengthrange()
{
	BW_GUARD;

	strengthSlider_.beginEdit();
}

void PageTerrainHeight::OnBnClickedHeightrange()
{
	BW_GUARD;

	heightSlider_.beginEdit();
}

void PageTerrainHeight::OnEnChangeTerrainSizeEdit()
{
	BW_GUARD;

	// This can be called very early on, before the page has been initialised.
	// This occurs when the page is created and it gets focus.
	if (!pageReady_)
		return;

	if (filterControls_ == 0)
	{
		++filterControls_;
		sizeSlider_.setValue( sizeEdit_.GetValue() );
		Options::setOptionFloat( "terrain/height/size", sizeSlider_.getValue() );
		--filterControls_;
	}
}

void PageTerrainHeight::OnEnChangeTerrainStrengthEdit()
{
	BW_GUARD;

	if (filterControls_ == 0)
	{
		++filterControls_;
		strengthSlider_.setValue( static_cast<float>(strengthEdit_.GetIntegerValue()) );
		Options::setOptionFloat( "terrain/height/strength", strengthSlider_.getValue() );
		--filterControls_;
	}
}

void PageTerrainHeight::OnEnChangeTerrainHeightEdit()
{
	BW_GUARD;

	if (filterControls_ == 0)
	{
		++filterControls_;
		float height = heightEdit_.GetValue();
		heightSlider_.setValue( height );
		Options::setOptionFloat( "terrain/height/height", height );
		--filterControls_;
	}
}
BW_END_NAMESPACE

