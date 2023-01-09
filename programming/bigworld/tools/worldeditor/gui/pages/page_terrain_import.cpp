#include "pch.hpp"
#include "worldeditor/gui/pages/page_terrain_import.hpp"
#include "worldeditor/framework/world_editor_app.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "worldeditor/scripting/we_python_adapter.hpp"
#include "worldeditor/height/height_module.hpp"
#include "worldeditor/import/terrain_utils.hpp"
#include "appmgr/options.hpp"
#include "guimanager/gui_manager.hpp"
#include "controls/utils.hpp"
#include "ual/ual_drop_manager.hpp"
#include "terrain/terrain_texture_layer.hpp"
#include "common/user_messages.hpp"
#include "common/utilities.hpp"
#include "common/string_utils.hpp"
#include "editor_shared/dialogs/file_dialog.hpp"
#include <afxpriv.h>

BW_BEGIN_NAMESPACE

const BW::wstring PageTerrainImport::contentID = L"PageTerrainImport";
PageTerrainImport *PageTerrainImport::s_instance_ = NULL;


namespace
{
    // Mode combobox data:
    std::pair<int, unsigned int> const modeData[] =
    {
        std::make_pair((int)ElevationBlit::REPLACE    , IDS_EBM_REPLACE    ),
        std::make_pair((int)ElevationBlit::ADDITIVE   , IDS_EBM_ADDITIVE   ),
        std::make_pair((int)ElevationBlit::SUBTRACTIVE, IDS_EBM_SUBTRACTIVE),
        std::make_pair((int)ElevationBlit::OVERLAY    , IDS_EBM_OVERLAY    ),
        std::make_pair((int)ElevationBlit::MIN        , IDS_EBM_MIN        ),
        std::make_pair((int)ElevationBlit::MAX        , IDS_EBM_MAX        ),
        std::make_pair(-1, 0) // -1 is terminus
    };

    // Any import that requires more than this amount of memory for undo
    // will not be undoable and will affect things on disk:
    const size_t PROMPT_LARGE_IMPORT_SIZE  = 15*1024*1024; // megabytes

    // Any import/export that requires more than this amount of memory changed
    // will need a progress bar:
    const size_t PROGRESS_LARGE_IMPORT_SIZE = 1024*1024; // megabytes
}


BEGIN_MESSAGE_MAP(PageTerrainImport, CFormView)
	ON_WM_HSCROLL()
	ON_WM_CTLCOLOR()
	ON_MESSAGE(WM_ACTIVATE_TOOL, OnActivateTool)
	ON_MESSAGE(WM_UPDATE_CONTROLS, OnUpdateControls)
    ON_BN_CLICKED(IDC_TERIMP_BROWSE_BTN  , OnBrowseBtn)
    ON_EN_CHANGE(IDC_TERIMP_MIN_EDIT     , OnEditMinHeight)
    ON_EN_CHANGE(IDC_TERIMP_MAX_EDIT     , OnEditMaxHeight)
    ON_EN_CHANGE(IDC_TERIMP_STRENGTH_EDIT, OnEditHeightStrength)
    ON_EN_KILLFOCUS(IDC_TERIMP_MIN_EDIT     , OnEditMinHeightKillFocus)
    ON_EN_KILLFOCUS(IDC_TERIMP_MAX_EDIT     , OnEditMaxHeightKillFocus)
    ON_EN_KILLFOCUS(IDC_TERIMP_STRENGTH_EDIT, OnEditHeightStrengthKillFocus)
    ON_CBN_SELCHANGE(IDC_TERIMP_MODE_CB  , OnHeightModeSel)
	ON_BN_CLICKED(IDC_TERIMP_ABS_CB, OnAbsoluteBtn)
    ON_MESSAGE(WM_RANGESLIDER_CHANGED, OnHeightSliderChanged)
    ON_MESSAGE(WM_RANGESLIDER_TRACK  , OnHeightSliderChanged)
    ON_BN_CLICKED(IDC_TERIMP_ANTICLOCKWISE_BTN, OnAnticlockwiseBtn)
    ON_BN_CLICKED(IDC_TERIMP_CLOCKWISE_BTN    , OnClockwiseBtn    )
    ON_BN_CLICKED(IDC_TERIMP_FLIPX_BTN        , OnFlipXBtn        )
    ON_BN_CLICKED(IDC_TERIMP_FLIPY_BTN        , OnFlipYBtn        )
    ON_BN_CLICKED(IDC_TERIMP_FLIPH_BTN        , OnFlipHeightBtn   )
    ON_BN_CLICKED(IDC_TERIMP_RECALCCLR_BTN    , OnRecalcClrBtn    )
    ON_UPDATE_COMMAND_UI(IDC_TERIMP_ANTICLOCKWISE_BTN, OnAnticlockwiseBtnEnable)
    ON_UPDATE_COMMAND_UI(IDC_TERIMP_CLOCKWISE_BTN    , OnClockwiseBtnEnable    )
    ON_UPDATE_COMMAND_UI(IDC_TERIMP_FLIPX_BTN        , OnFlipXBtnEnable        )
    ON_UPDATE_COMMAND_UI(IDC_TERIMP_FLIPY_BTN        , OnFlipYBtnEnable        )
    ON_UPDATE_COMMAND_UI(IDC_TERIMP_FLIPH_BTN        , OnFlipHeightBtnEnable   )
    ON_UPDATE_COMMAND_UI(IDC_TERIMP_RECALCCLR_BTN    , OnRecalcClrBtnEnable    )
    ON_BN_CLICKED(IDC_TERIMP_PLACE_BTN        , OnPlaceBtn        )
    ON_BN_CLICKED(IDC_TERIMP_CANCEL_BTN       , OnCancelImportBtn )
    ON_BN_CLICKED(IDC_TERIMP_EXPORT_BTN       , OnExportBtn       )
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, OnToolTipText)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, OnToolTipText)
END_MESSAGE_MAP()


/**
 *	This is the PageTerrainImport constructor.
 */
PageTerrainImport::PageTerrainImport() : 
	CFormView(PageTerrainImport::IDD),
	pageReady_(false),
	filterControls_(0),
	loadedTerrain_(false)
{
    s_instance_ = this;
}


/**
 *	This is the PageTerrainImport destructor.
 */
PageTerrainImport::~PageTerrainImport()
{
    s_instance_ = NULL;
}


/**
 *	This is used to set the import height range.
 *
 *  @param minv		The minimum height.
 *  @param maxv		The maximum height.
 */
void PageTerrainImport::setQueryHeightRange(float minv, float maxv)
{
	BW_GUARD;

    ++filterControls_;
    float theMinV = minHeightEdit_.GetValue();
    float theMaxV = maxHeightEdit_.GetValue();
    if
    (
        minv != -std::numeric_limits<float>::max()
        &&
        minv != +std::numeric_limits<float>::max()
    )
    {
        theMinV = minv;
    }
    if
    (
        maxv != -std::numeric_limits<float>::max()
        &&
        maxv != +std::numeric_limits<float>::max()
    )
    {
        theMaxV = maxv;
    }
    if (theMinV > theMaxV) theMaxV = theMinV;
    if (theMaxV < theMinV) theMinV = theMaxV;
    minHeightEdit_.SetValue(theMinV);
    maxHeightEdit_.SetValue(theMaxV);
    heightSlider_.setThumbValues(theMinV, theMaxV);
    updateHMImportOptions();
    --filterControls_;
}


/**
 *	This returns the instance of the PageTerrainImport class.
 *
 *  @returns		The instance of the PageTerrainImport class.
 */
/*static*/ PageTerrainImport *PageTerrainImport::instance() 
{ 
    return s_instance_; 
}


/**
 *	This sets the name of the imported terrain/mask.
 */
void PageTerrainImport::setImportFile(char const *filename)
{
	BW_GUARD;

    importFileEdit_.SetWindowText(bw_utf8tow( filename ).c_str());
}


/**
 *	This is called to initialise the page.
 */	
void PageTerrainImport::InitPage()
{
	BW_GUARD;

    if (pageReady_)
        return;

    ++filterControls_;

	// Height import options:
	minHeightEdit_.SetNumericType(controls::EditNumeric::ENT_INTEGER);
	maxHeightEdit_.SetNumericType(controls::EditNumeric::ENT_INTEGER);
    minHeightEdit_.SetValue(0);
    maxHeightEdit_.SetValue(50);
	heightSlider_ .setRangeLimit(
		TerrainUtils::MIN_TERRAIN_HEIGHT, TerrainUtils::MAX_TERRAIN_HEIGHT );
	heightSlider_ .setRange( 
		TerrainUtils::MIN_TERRAIN_HEIGHT, TerrainUtils::MAX_TERRAIN_HEIGHT );
    heightSlider_ .setThumbValues(0.0f, 50.0f);

	heightStrengthEdit_  .SetNumDecimals( 1 );
	heightStrengthEdit_  .SetValue(100.0f);

	heightStrengthSlider_.setDigits( 1 );
	heightStrengthSlider_.setRangeLimit( 0, 100 );
    heightStrengthSlider_.setRange( 0, 100 );
    heightStrengthSlider_.setValue( 100 );

	modeCB_.Clear();
    for (int i = 0; modeData[i].first >= 0; ++i)
    {
		const UINT_PTR stringID = modeData[i].second;
		// Requirement for CString::CString to treat this as a string ID
		MF_ASSERT( HIWORD( stringID ) == 0 );
        CString str( reinterpret_cast< LPCSTR >( stringID ) );
        modeCB_.AddString(str);
    }
    modeCB_.SetCurSel(0);

    // NOTE: Tooltip initialisation should be done before creating the toolbar
    // below.  If it isn't then a tooltip is created for the toolbar itself
    // which can contain junk.
    INIT_AUTO_TOOLTIP();

    // Initialise the toolbar with flip, rotate etc: 
    actionTB_.CreateEx
    (
        this, 
        TBSTYLE_FLAT, 
        WS_CHILD | WS_VISIBLE | CBRS_ALIGN_TOP
    );
    actionTB_.LoadToolBarEx(IDR_IMP_TER_TOOLBAR, IDR_IMP_TER_DIS_TOOLBAR);
    actionTB_.SetBarStyle(CBRS_ALIGN_TOP | CBRS_TOOLTIPS | CBRS_FLYBY);              
    actionTB_.Subclass(IDC_TERIMP_ACTIONTB);     
    actionTB_.ShowWindow(SW_SHOW);

	// Hand drawn map options:
	blendSlider_.SetRangeMin(1);
	blendSlider_.SetRangeMax(100);
	blendSlider_.SetPageSize(0);

    updateHMImportOptions();

    --filterControls_;    

	pageReady_ = true;
}


/**
 *	This is called the do a DDX synchronization and to subclass child controls.
 *
 *  @param pDX		The DDX synchronization data.
 */
void PageTerrainImport::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_TERIMP_IMPORT_GROUP           , importGrp_           );
    DDX_Control(pDX, IDC_TERIMP_FILE_STATIC            , fileStatic_          );
    DDX_Control(pDX, IDC_TERIMP_IMPORT_FILE            , importFileEdit_      );
    DDX_Control(pDX, IDC_TERIMP_BROWSE_BTN             , importButton_        );
    DDX_Control(pDX, IDC_TERIMP_EXPORT_BTN             , exportBtn_           );
    DDX_Control(pDX, IDC_TERIMP_MIN_EDIT               , minHeightEdit_       );
    DDX_Control(pDX, IDC_TERIMP_MAX_EDIT               , maxHeightEdit_       );
    DDX_Control(pDX, IDC_TERIMP_HEIGHT_SLIDER          , heightSlider_        );
    DDX_Control(pDX, IDC_TERIMP_STRENGTH_EDIT          , heightStrengthEdit_  );
    DDX_Control(pDX, IDC_TERIMP_STRENGTH_SLIDER        , heightStrengthSlider_);
    DDX_Control(pDX, IDC_TERIMP_MODE_CB                , modeCB_              );
	DDX_Control(pDX, IDC_TERIMP_ABS_CB				   , absoluteBtn_		  );

    DDX_Control(pDX, IDC_TERIMP_PLACE_BTN              , placeBtn_            );
    DDX_Control(pDX, IDC_TERIMP_CANCEL_BTN             , cancelBtn_           );
												 
    DDX_Control(pDX, IDC_TERIMP_HANDDRAWN_GROUP        , handrawnGrp_         );
    DDX_Control(pDX, IDC_TERIMP_MAP_ALPHA_SLIDER       , blendSlider_         );
}


/**
 *	This is called when the window is activated.
 *
 *  @param wParam		Ignored.  Here to match a function definition.
 *	@param lParam		Ignored.  Here to match a function definition.
 *	@returns			0.    Here to match a function definition.
 */
LRESULT PageTerrainImport::OnActivateTool(WPARAM wParam, LPARAM /*lParam*/)
{
	BW_GUARD;

	const wchar_t * activePageId = (const wchar_t *)(wParam);
	if (getContentID() == activePageId)
	{
		if (WorldEditorApp::instance().pythonAdapter())
		{
			WorldEditorApp::instance().pythonAdapter()->onPageControlTabSelect("pgc", "TerrainImport");
		}
		UpdateState();
		updateHMImportOptions();
	}
	return 0;
}


/**
 *	This is called when a slider is moved.
 *
 *  @param nSBCode		The code of the slider.
 *  @param nPos			The position of the slider.
 *  @param pScrollBar	The scrollbar/slider moved.
 */
void PageTerrainImport::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	BW_GUARD;

	if ( !pageReady_ )
		InitPage();

    if (pScrollBar != NULL)
    {
        if (pScrollBar->GetSafeHwnd() == blendSlider_.GetSafeHwnd())
        {
	        if (WorldEditorApp::instance().pythonAdapter())
	        {
				WorldEditorApp::instance().pythonAdapter()->onSliderAdjust(
                    "slrProjectMapBlend", 
				    blendSlider_.GetPos(), 
				    blendSlider_.GetRangeMin(), 
				    blendSlider_.GetRangeMax() );		
	        }
        }

        if (pScrollBar->GetSafeHwnd() == heightStrengthSlider_.GetSafeHwnd())
        {
            OnHeightStrengthSlider(nSBCode == TB_ENDTRACK);
        }
    }

	CFormView::OnHScroll(nSBCode, nPos, pScrollBar);
}


/**
 *	This is called every frame to update the control.
 *
 *  @param wParam		Ignored.  Here to match a function definition.
 *	@param lParam		Ignored.  Here to match a function definition.
 *	@returns			0.    Here to match a function definition. 
 */
afx_msg LRESULT PageTerrainImport::OnUpdateControls(WPARAM wParam, LPARAM lParam)
{
	BW_GUARD;

	if ( !pageReady_ )
		InitPage();

	if ( !IsWindowVisible() )
		return 0;

	MF_ASSERT( WorldEditorApp::instance().pythonAdapter() != NULL &&
		"PageTerrainImport::OnUpdateControls: PythonAdapter is NULL" );
	WorldEditorApp::instance().pythonAdapter()->sliderUpdate
    (
        &blendSlider_, 
        "slrProjectMapBlend"
    );

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
HBRUSH PageTerrainImport::OnCtlColor( CDC* pDC, CWnd* pWnd, UINT nCtlColor )
{
	BW_GUARD;

	HBRUSH brush = CFormView::OnCtlColor( pDC, pWnd, nCtlColor );

	minHeightEdit_.SetBoundsColour( pDC, pWnd,
		heightSlider_.getMinRangeLimit(), heightSlider_.getMaxRangeLimit() );
	maxHeightEdit_.SetBoundsColour( pDC, pWnd,
		heightSlider_.getMinRangeLimit(), heightSlider_.getMaxRangeLimit() );
	heightStrengthEdit_.SetBoundsColour( pDC, pWnd,
		heightStrengthSlider_.getMinRangeLimit(), heightStrengthSlider_.getMaxRangeLimit() );

	return brush;
}


/**
 *	This is called when the user selects a terrain height map/mask to browse.
 */
void PageTerrainImport::OnBrowseBtn()
{
	BW_GUARD;

    if (!HeightModule::hasStarted())
        return;

    HeightModule *hm = HeightModule::currentInstance();

	static wchar_t szFilter[] = 
        L"Greyscale Heightmaps (*.raw;*.r16;*.bmp)|*.raw;*.r16;*.bmp|"
        L"Terragen Files (*.ter)|*.ter|"
        L"DTED2 Files (*.dt2)|*.dt2|"
        L"All Files (*.*)|*.*||";

	BWFileDialog openFile(
		true, L"raw", NULL, BWFileDialog::FD_FILEMUSTEXIST, szFilter);

	if (openFile.showDialog()==false)
		return;

	BW::wstring filename = openFile.getFileName();
    ImportImagePtr image = new ImportImage();
    float left     = std::numeric_limits<float>::max();
    float top      = std::numeric_limits<float>::max();
    float right    = std::numeric_limits<float>::max();
    float bottom   = std::numeric_limits<float>::max();
	bool  absolute = absoluteBtn_.GetCheck() == BST_CHECKED; 
	BW::string nfilename;
	bw_wtoutf8( filename, nfilename );
    ImportCodec::LoadResult loadResult = 
        image->load(nfilename, &left, &top, &right, &bottom, &absolute, true);
    switch (loadResult)
    {
    case ImportCodec::LR_OK:
        {
        setImportFile(nfilename.c_str());
        loadedTerrain_ = true;
        ++filterControls_;
        float minv, maxv;
        image->rangeHeight(minv, maxv);
        minHeightEdit_.SetValue(minv);
        maxHeightEdit_.SetValue(maxv);
        heightSlider_.setThumbValues(minv, maxv);
		absoluteBtn_.SetCheck(absolute ? BST_CHECKED : BST_UNCHECKED);
        --filterControls_;
		image->normalise();
        updateHMImportOptions();		
        hm->importData
        (
            image,
            left   != std::numeric_limits<float>::max() ? &left   : NULL,
            top    != std::numeric_limits<float>::max() ? &top    : NULL,
            right  != std::numeric_limits<float>::max() ? &right  : NULL,
            bottom != std::numeric_limits<float>::max() ? &bottom : NULL
        );
        }
        break;
    case ImportCodec::LR_BAD_FORMAT:
		{
		BW::wstring msg = Localise(L"RCST_IDS_FAILED_IMPORT_TERRAIN");
        AfxMessageBox(msg.c_str(), MB_OK);
        setImportFile("");
        loadedTerrain_ = false;
        hm->importData(NULL);
		}
        break;
    case ImportCodec::LR_CANCELLED:            
        setImportFile("");
        loadedTerrain_ = false;
        hm->importData(NULL);
        break;
    case ImportCodec::LR_UNKNOWN_CODEC:
		{
		BW::wstring msg = Localise(L"RCST_IDS_BADCODEC_IMPORT_TERRAIN");
        AfxMessageBox(msg.c_str(), MB_OK);
        setImportFile("");
        loadedTerrain_ = false;
        hm->importData(NULL);
		}
        break;
    }
    UpdateState();
}


/**
 *	This is called when the minimum height field is edited.
 */
void PageTerrainImport::OnEditMinHeight()
{
	BW_GUARD;

    if (filterControls_ == 0)
    {
        ++filterControls_;
		float min = minHeightEdit_.GetValue();
		float max = maxHeightEdit_.GetValue();
		if (min < max)
		{
			heightSlider_.setThumbValues( min, max );
		}
		else
		{
			heightSlider_.setThumbValues( max, min );
		}
        updateHMImportOptions();
        --filterControls_;
    }
}


/**
 *	This is called when the maximum height field is edited.
 */
void PageTerrainImport::OnEditMaxHeight()
{
	BW_GUARD;

    if (filterControls_ == 0)
    {
        ++filterControls_;
		float min = minHeightEdit_.GetValue();
		float max = maxHeightEdit_.GetValue();
		if (min < max)
		{
			heightSlider_.setThumbValues( min, max );
		}
		else
		{
			heightSlider_.setThumbValues( max, min );
		}
        updateHMImportOptions();
        --filterControls_;
    }
}


/**
 *	This is called when the height strength field is edited.
 */
void PageTerrainImport::OnEditHeightStrength()
{
	BW_GUARD;

    if (filterControls_ == 0)
    {
        ++filterControls_;
		heightStrengthSlider_.setValue( heightStrengthEdit_.GetValue() );
        updateHMImportOptions();
        --filterControls_;
    }
}


/**
 *	This is called when the terrain height mode is changed.
 */
void PageTerrainImport::OnHeightModeSel()
{
	BW_GUARD;

    if (filterControls_ == 0)
    {
        ++filterControls_;
        updateHMImportOptions();
        --filterControls_;
    }
}


/**
 *	This is called when the user rotates the imported height/mask 
 *	anti-clockwise.
 */
void PageTerrainImport::OnAnticlockwiseBtn()
{
	BW_GUARD;

    HeightModule *hm = HeightModule::currentInstance();
    if (hm != NULL)
    {
        hm->rotateImportData(false); // false = anticlockwise
    }
}


/**
 *	This is called when the user rotates the imported height/mask clockwise.
 */
void PageTerrainImport::OnClockwiseBtn()
{
	BW_GUARD;

    HeightModule *hm = HeightModule::currentInstance();
    if (hm != NULL)
    {
        hm->rotateImportData(true); // true = clockwise
    }
}


/**
 *	This is called when the user flips the imported height/mask about the 
 *	x-axis.
 */
void PageTerrainImport::OnFlipXBtn()
{
	BW_GUARD;

    HeightModule *hm = HeightModule::currentInstance();
    if (hm != NULL)
    {
        hm->flipImportData(HeightModule::FLIP_X);
    }
}


/**
 *	This is called when the user flips the imported height/mask about the 
 *	y-axis.
 */
void PageTerrainImport::OnFlipYBtn()
{
	BW_GUARD;

    HeightModule *hm = HeightModule::currentInstance();
    if (hm != NULL)
    {
        hm->flipImportData(HeightModule::FLIP_Y);
    }
}


/**
 *	This is called when the user flips the heights.
 */
void PageTerrainImport::OnFlipHeightBtn()
{
	BW_GUARD;

    HeightModule *hm = HeightModule::currentInstance();
    if (hm != NULL)
    {
        hm->flipImportData(HeightModule::FLIP_HEIGHT);
    }
}


/**
 *	This is called when the user presses the recalculate (renormalise height
 *	colour scale).
 */
void PageTerrainImport::OnRecalcClrBtn()
{
	BW_GUARD;

    HeightModule *hm = HeightModule::currentInstance();
    if (hm != NULL)
    {
        hm->updateMinMax();
    }
}


/**
 *	This is called to determine whether the anti-clockwise button should be
 *	enabled.
 *
 *  @param cmdui		The CCmdUI to enable/disable the button.
 */
void PageTerrainImport::OnAnticlockwiseBtnEnable(CCmdUI *cmdui)
{
	BW_GUARD;

    if (!HeightModule::hasStarted())
        return;

    HeightModule *hm = HeightModule::currentInstance();

    BOOL importing = 
		(hm->mode() == HeightModule::IMPORT_TERRAIN)
		||
		(hm->mode() == HeightModule::IMPORT_MASK);
    cmdui->Enable(importing);
}


/**
 *	This is called to determine whether the clockwise button should be
 *	enabled.
 *
 *  @param cmdui		The CCmdUI to enable/disable the button.
 */
void PageTerrainImport::OnClockwiseBtnEnable(CCmdUI *cmdui)
{
	BW_GUARD;

    OnAnticlockwiseBtnEnable(cmdui); // these are enabled/disabled together
}


/**
 *	This is called to determine whether the flip-x button should be
 *	enabled.
 *
 *  @param cmdui		The CCmdUI to enable/disable the button.
 */
void PageTerrainImport::OnFlipXBtnEnable(CCmdUI *cmdui)
{
	BW_GUARD;

    OnAnticlockwiseBtnEnable(cmdui); // these are enabled/disabled together
}


/**
 *	This is called to determine whether the flip-y button should be
 *	enabled.
 *
 *  @param cmdui		The CCmdUI to enable/disable the button.
 */
void PageTerrainImport::OnFlipYBtnEnable(CCmdUI *cmdui)
{
	BW_GUARD;

    OnAnticlockwiseBtnEnable(cmdui); // these are enabled/disabled together
}


/**
 *	This is called to determine whether the flip-height button should be
 *	enabled.
 *
 *  @param cmdui		The CCmdUI to enable/disable the button.
 */
void PageTerrainImport::OnFlipHeightBtnEnable(CCmdUI *cmdui)
{
	BW_GUARD;

    OnAnticlockwiseBtnEnable(cmdui); // these are enabled/disabled together
}


/**
 *	This is called to determine whether the recalculate colours button should 
 *	be enabled.
 *
 *  @param cmdui		The CCmdUI to enable/disable the button.
 */
void PageTerrainImport::OnRecalcClrBtnEnable(CCmdUI *cmdui)
{
	BW_GUARD;

    OnAnticlockwiseBtnEnable(cmdui); // these are enabled/disabled together
}


/**
 *	This is called when the user clicks the place button.
 */
void PageTerrainImport::OnPlaceBtn()
{
	BW_GUARD;

    HeightModule *hm = HeightModule::currentInstance();
    if (hm != NULL)
    {
        size_t sizeUndo = hm->terrainUndoSize();
        bool doUndo   = sizeUndo < PROMPT_LARGE_IMPORT_SIZE;
        bool progress = sizeUndo > PROGRESS_LARGE_IMPORT_SIZE;
        bool doImport = true;
        if (!doUndo)
        {
			BW::wstring msg = Localise(L"RCST_IDS_LARGE_IMPORT");
            doImport =
                AfxMessageBox(msg.c_str(), MB_YESNO)
                ==
                IDYES;
        }
        if (doImport)
        {
            CWaitCursor waitCursor;
			WorldManager::instance().disableClose();
            hm->doTerrainImport(doUndo, progress, doUndo);
            hm->importData(NULL);
            importFileEdit_.SetWindowText(L"");
            UpdateState();
            GUI::Manager::instance().update();
			if (!doUndo)
				WorldManager::instance().quickSave();
			WorldManager::instance().enableClose();
        }
    }
}


/**
 *	This is called when the user clicks the cancel button.
 */
void PageTerrainImport::OnCancelImportBtn()
{
	BW_GUARD;

    HeightModule *hm = HeightModule::currentInstance();
    if (hm != NULL)
    {
        hm->importData(NULL);
        importFileEdit_.SetWindowText(L"");
        UpdateState();
        hm->updateMinMax();
    }
} 


/**
 *	This is called when the user clicks the export button.
 */
void PageTerrainImport::OnExportBtn()
{
	BW_GUARD;

    HeightModule *hm = HeightModule::currentInstance();
    if (hm != NULL)
    {
	    static wchar_t szFilter[] = 
			L"RAW Files (*.raw;*.r16)|*.raw;*.r16|"
            L"BMP Files (*.bmp)|*.bmp|"            
            L"Terragen Files (*.ter)|*.ter|"
			L"TIFF Files (*.tif)|*.tif|"
            L"All Files (*.*)|*.*||";

	    BWFileDialog openFile(
			false, L"raw", NULL, BWFileDialog::FD_OVERWRITEPROMPT, szFilter);

        if (openFile.showDialog() == false)
            return;

        size_t sizeUndo = hm->terrainUndoSize();
        bool   progress = sizeUndo > PROGRESS_LARGE_IMPORT_SIZE;
        
        CWaitCursor waitCursor;
        hm->doTerrainExport
		(
			bw_wtoutf8( openFile.getFileName() ).c_str(), 
			progress
		);
    }
}


/**
 *	This is called when the UI wants to query for a tooltip text.
 */
BOOL PageTerrainImport::OnToolTipText(UINT, NMHDR* pNMHDR, LRESULT *result)
{
	BW_GUARD;

    // Allow top level routing frame to handle the message
    if (GetRoutingFrame() != NULL)
        return FALSE;


    TOOLTIPTEXTW* pTTTW = (TOOLTIPTEXTW*)pNMHDR;
    TCHAR szFullText[256];
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
        AfxLoadString( (UINT) nID, szFullText);
            // this is the command id, not the button index
        cstTipText    = szFullText;
        cstStatusText = szFullText;
    }

    wcsncpy( pTTTW->szText, cstTipText, ARRAY_SIZE( pTTTW->szText ) );
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


/**
 *	This is called to update the state of the controls.
 */
void PageTerrainImport::UpdateState()
{
	BW_GUARD;

    if (!pageReady_)
        InitPage();

    if (!HeightModule::hasStarted())
        return;

    HeightModule *hm = HeightModule::currentInstance();

    BOOL importing = hm->hasImportData();
    BOOL exporting = !importing;

	// Inform the height module about the mode and strengths:
	if (importing)
		hm->mode(HeightModule::IMPORT_TERRAIN);
	else if (exporting)
		hm->mode(HeightModule::EXPORT_TERRAIN);

	// Update subclassed controls:
	exportBtn_           .EnableWindow(exporting);
    heightStrengthEdit_  .EnableWindow(importing);
    heightStrengthSlider_.EnableWindow(importing);
    modeCB_              .EnableWindow(importing);
    placeBtn_            .EnableWindow(importing);
    cancelBtn_           .EnableWindow(importing);
}


/**
 *	This is called when the absolute heights button is pressed.
 */
void PageTerrainImport::OnAbsoluteBtn()
{
	BW_GUARD;

	if (filterControls_ == 0)
    {
        ++filterControls_;
		updateHMImportOptions();
		--filterControls_;
	}
}


/**
 *	Called when the minimum height edit field loses focus.
 */
/*afx_msg*/ void PageTerrainImport::OnEditMinHeightKillFocus()
{
	BW_GUARD;

	if (filterControls_ == 0)
    {
        ++filterControls_;
		
		float min = minHeightEdit_.GetValue();
		float max = maxHeightEdit_.GetValue();

		if ( min < heightSlider_.getMinRangeLimit() )
		{
			min = heightSlider_.getMinRangeLimit();
		}
		if ( min > heightSlider_.getMaxRangeLimit() )
		{
			min = heightSlider_.getMaxRangeLimit();
		}

		if( min < max )
		{
			minHeightEdit_.SetValue( min );
		}
		else
		{
			minHeightEdit_.SetValue( max );
			maxHeightEdit_.SetValue( min );
		}
		
		--filterControls_;
	}
}


/**
 *	Called when the maximum height edit field loses focus.
 */
/*afx_msg*/ void PageTerrainImport::OnEditMaxHeightKillFocus()
{
	BW_GUARD;

	if (filterControls_ == 0)
    {
        ++filterControls_;

		float min = minHeightEdit_.GetValue();
		float max = maxHeightEdit_.GetValue();

		if ( max < heightSlider_.getMinRangeLimit() )
		{
			max = heightSlider_.getMinRangeLimit();
		}
		if ( max > heightSlider_.getMaxRangeLimit() )
		{
			max = heightSlider_.getMaxRangeLimit();
		}

		if( min < max )
		{
			maxHeightEdit_.SetValue( max );
		}
		else
		{
			minHeightEdit_.SetValue( max );
			maxHeightEdit_.SetValue( min );
		}
		--filterControls_;
	}
}


/**
 *	Called when the height strength edit field loses focus.
 */
/*afx_msg*/ void PageTerrainImport::OnEditHeightStrengthKillFocus()
{
	BW_GUARD;

	if (filterControls_ == 0)
    {
        ++filterControls_;
		heightStrengthEdit_.SetValue( heightStrengthSlider_.getValue() );
		--filterControls_;
	}
}


/**
 *	This is called when the height strength slider is changed.
 *
 *  @param endTrack		Has the user released the mouse (not used).
 */
void PageTerrainImport::OnHeightStrengthSlider(bool /*endTrack*/)
{
	BW_GUARD;

    if (filterControls_ == 0)
    {
        ++filterControls_;
        heightStrengthEdit_.SetValue( heightStrengthSlider_.getValue() );
        updateHMImportOptions();
        --filterControls_;
    }
}


/**
 *	This is called when the terrain height slider is changed.
 *
 *  @param wParam		Ignored.  Here to match a function definition.
 *	@param lParam		Ignored.  Here to match a function definition.
 *	@returns			TRUE.  Here to match a function definition.
 */
LRESULT PageTerrainImport::OnHeightSliderChanged
(
	WPARAM	/*wParam*/, 
	LPARAM	/*lParam*/
)
{
	BW_GUARD;

    if (filterControls_ == 0)
    {
        ++filterControls_;
        float minv, maxv;
        heightSlider_.getThumbValues(minv, maxv);
        minHeightEdit_.SetValue(minv);
        maxHeightEdit_.SetValue(maxv);
        updateHMImportOptions();
        --filterControls_;
    }
    return TRUE;
}


/**
 *	This is called to update the height map import options in the HeightModule.
 */
void PageTerrainImport::updateHMImportOptions()
{
	BW_GUARD;

    if (!HeightModule::hasStarted())
        return;

    float minv;
    float maxv;
	heightSlider_.getThumbValues(minv, maxv);
    float strength = heightStrengthSlider_.getValue()/100.0f;
	bool  absolute = absoluteBtn_.GetCheck() == BST_CHECKED;
    ElevationBlit::Mode mode = 
        (ElevationBlit::Mode)modeData[modeCB_.GetCurSel()].first;
    HeightModule::currentInstance()->terrainImportOptions(mode, minv, maxv, strength, absolute);
}
BW_END_NAMESPACE

