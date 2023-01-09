#include "pch.hpp"
#include "worldeditor/gui/pages/page_options_environment.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "worldeditor/framework/world_editor_app.hpp"
#include "worldeditor/scripting/we_python_adapter.hpp"
#include "controls/user_messages.hpp"
#include "appmgr/app.hpp"
#include "appmgr/options.hpp"
#include "gizmo/undoredo.hpp"
#include "resmgr/xml_section.hpp"
#include "resmgr/auto_config.hpp"
#include "romp/enviro_minder.hpp"
#include "romp/sky_gradient_dome.hpp"
#include "romp/time_of_day.hpp"
#include "moo/visual_manager.hpp"
#include "terrain/terrain_settings.hpp"
#include "common/format.hpp"
#include "common/string_utils.hpp"
#include "common/user_messages.hpp"
#include "editor_shared/dialogs/file_dialog.hpp"
#include "editor_shared/cursor/wait_cursor.hpp"
#include <afxpriv.h>

DECLARE_DEBUG_COMPONENT2("WorldEditor", 0)

BW_BEGIN_NAMESPACE

BEGIN_MESSAGE_MAP(PageOptionsEnvironment, CFormView)
    ON_WM_HSCROLL()
	ON_WM_CTLCOLOR()
    ON_MESSAGE  (WM_UPDATE_CONTROLS     , OnUpdateControls  )
    ON_MESSAGE  (WM_NEW_SPACE           , OnNewSpace        )
    ON_MESSAGE  (WM_BEGIN_SAVE          , OnBeginSave       )
    ON_MESSAGE  (WM_END_SAVE            , OnEndSave         )
    ON_COMMAND  (IDC_SKYFILE_BTN        , OnBrowseSkyFile   )
    ON_COMMAND  (IDC_NEWSKYFILE_BTN     , OnCopySkyFile     )
    ON_COMMAND  (IDC_TODFILE_BTN        , OnBrowseTODFile   )
    ON_COMMAND  (IDC_NEWTODFILE_BTN     , OnCopyTODFile     )
    ON_COMMAND  (IDC_ADDSKYDOME_BTN     , OnAddSkyDome      )
    ON_COMMAND  (IDC_CLEARSKYDOME_BTN   , OnClearSkyDomes   )
    ON_COMMAND  (IDC_SB_GRAD_BTN        , OnBrowseSkyGradBtn)
    ON_EN_CHANGE(IDC_HOURLENGTH         , OnHourLengthEdit  )
    ON_EN_CHANGE(IDC_STARTTIME          , OnStartTimeEdit   )
    ON_MESSAGE  (WM_CT_SEL_TIME         , OnCTSelTime       )
    ON_EN_CHANGE(IDC_SUNANGLE_EDIT      , OnSunAngleEdit    )
    ON_EN_CHANGE(IDC_MOONANGLE_EDIT     , OnMoonAngleEdit   )
    ON_MESSAGE  (WM_CT_UPDATE_BEGIN     , OnTimelineBegin   )
    ON_MESSAGE  (WM_CT_UPDATE_MIDDLE    , OnTimelineMiddle  )
    ON_MESSAGE  (WM_CT_UPDATE_DONE      , OnTimelineDone    )
    ON_MESSAGE  (WM_CT_ADDED_COLOR      , OnTimelineAdd     )
    ON_MESSAGE  (WM_CT_NEW_SELECTION    , OnTimelineNewSel  )
    ON_MESSAGE  (WM_CP_LBUTTONDOWN      , OnPickerDown      )
    ON_MESSAGE  (WM_CP_LBUTTONMOVE      , OnPickerMove      )
    ON_MESSAGE  (WM_CP_LBUTTONUP        , OnPickerUp        )
    ON_COMMAND  (IDC_SUNANIM_BTN        , OnSunAnimBtn      )
    ON_COMMAND  (IDC_AMBANIM_BTN        , OnAmbAnimBtn      )
    ON_COMMAND  (IDC_CREATEANIM_BTN     , OnResetBtn        )
    ON_COMMAND  (IDC_ADDCOLOR_BTN       , OnAddClrBtn       )
    ON_COMMAND  (IDC_DELCOLOR_BTN       , OnDelClrBtn       )
    ON_EN_CHANGE(IDC_R_EDIT             , OnEditClrText     )
    ON_EN_CHANGE(IDC_G_EDIT             , OnEditClrText     )
    ON_EN_CHANGE(IDC_B_EDIT             , OnEditClrText     )
    ON_EN_CHANGE(IDC_MIEAMOUNT          , OnEditEnvironText )  
    ON_EN_CHANGE(IDC_TURBOFFS           , OnEditEnvironText )
    ON_EN_CHANGE(IDC_TURBFACTOR         , OnEditEnvironText )
    ON_EN_CHANGE(IDC_VERTHEIGHTEFFECT   , OnEditEnvironText )
    ON_EN_CHANGE(IDC_SUNHEIGHTEFFECT    , OnEditEnvironText )
    ON_EN_CHANGE(IDC_POWER              , OnEditEnvironText )
    ON_EN_KILLFOCUS(IDC_SUNANGLE_EDIT      , OnSunAngleEditKillFocus		)
    ON_EN_KILLFOCUS(IDC_MOONANGLE_EDIT     , OnMoonAngleEditKillFocus		)
    ON_EN_KILLFOCUS(IDC_MIEAMOUNT          , OnMieAmountEditKillFocus		)  
    ON_EN_KILLFOCUS(IDC_TURBOFFS           , OnTurbOffEditKillFocus			)
    ON_EN_KILLFOCUS(IDC_TURBFACTOR         , OnTurbFactEditKillFocus		)
    ON_EN_KILLFOCUS(IDC_VERTHEIGHTEFFECT   , OnVertEffEditKillFocus			)
    ON_EN_KILLFOCUS(IDC_SUNHEIGHTEFFECT    , OnSunEffEditKillFocus			)
    ON_EN_KILLFOCUS(IDC_POWER              , OnPowerEditKillFocus			)

	//Sun
	ON_EN_CHANGE(IDC_SUNCOLOR_R_EDIT								, OnEnChangeSuncolorEdit		)
	ON_EN_CHANGE(IDC_SUNCOLOR_G_EDIT								, OnEnChangeSuncolorEdit		)
	ON_EN_CHANGE(IDC_SUNCOLOR_B_EDIT								, OnEnChangeSuncolorEdit		)
	//Moon
	ON_EN_CHANGE(IDC_MOONCOLOR_R_EDIT								, OnEnChangeMooncolorEdit		)
	ON_EN_CHANGE(IDC_MOONCOLOR_G_EDIT								, OnEnChangeMooncolorEdit		)
	ON_EN_CHANGE(IDC_MOONCOLOR_B_EDIT								, OnEnChangeMooncolorEdit		)

	ON_BN_CLICKED(IDC_SUN_COLOR_BUTTON								, OnBnClickedSunColorButton)
	ON_BN_CLICKED(IDC_MOON_COLOR_BUTTON								, OnBnClickedMoonColorButton)

	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, OnToolTipText)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, OnToolTipText)
    ON_UPDATE_COMMAND_UI(IDC_SKYDOME_UP  , OnSkyboxUpEnable  )
    ON_UPDATE_COMMAND_UI(IDC_SKYDOME_DOWN, OnSkyboxDownEnable)
    ON_UPDATE_COMMAND_UI(IDC_SKYDOME_DEL , OnSkyboxDelEnable )
    ON_COMMAND(IDC_SKYDOME_UP  , OnSkyboxUp  )
    ON_COMMAND(IDC_SKYDOME_DOWN, OnSkyboxDown)
    ON_COMMAND(IDC_SKYDOME_DEL , OnSkyboxDel )
END_MESSAGE_MAP()


const BW::wstring PageOptionsEnvironment::contentID = L"PageOptionsEnvironment";


/*static*/ PageOptionsEnvironment *PageOptionsEnvironment::s_instance_ = NULL;


namespace
{
    const float SLIDER_PREC = 100.0f;   // conversion from slider to float

    // This class is for environment undo/redo operations.
    class EnvironmentUndo : public UndoRedo::Operation
    {
    public:
        EnvironmentUndo();

        /*virtual*/ void undo();

        /*virtual*/ bool iseq(UndoRedo::Operation const &other) const;

    private:
        XMLSectionPtr       ds_;
    };

    EnvironmentUndo::EnvironmentUndo():
		UndoRedo::Operation((size_t)(typeid(EnvironmentUndo).name())),
		ds_(new XMLSection("environmentUndoRedo"))
    {
		BW_GUARD;

        PageOptionsEnvironment *poe = PageOptionsEnvironment::instance();

        EnviroMinder &em = PageOptionsEnvironment::getEnviroMinder();
        em.save(ds_, false); // save everything internally

        // Explicitly save and restore the game time:
        float gameTime = em.timeOfDay()->gameTime();
        ds_->writeFloat("gametime", gameTime);

        // Explicity save the game speed:
        float gameSpeed = WorldManager::instance().secondsPerHour();
        ds_->writeFloat("gamespeed", gameSpeed);

        // Explicitly store any selection in the colour timeline and game 
        // seconds per hour:
        if (poe != NULL)
        {
            float selTime = poe->getSelTime();
            ds_->writeFloat("seltime", selTime);            
        }
    }


    /*virtual*/ void EnvironmentUndo::undo()
    {
		BW_GUARD;

        // Save the current state to the undo/redo stack:
        UndoRedo::instance().add(new EnvironmentUndo());

        // Do the actual undo:
        EnviroMinder &em = PageOptionsEnvironment::getEnviroMinder();
        em.load(ds_, false); // load everything internally

        // Restore the game time:
        float gameTime = ds_->readFloat("gametime", -1.0f);
        if (gameTime != -1.0f)
            em.timeOfDay()->gameTime(gameTime);

        // Restore the game speed:
        float gameSpeed = ds_->readFloat("gamespeed", 0.0f);
        WorldManager::instance().secondsPerHour(gameSpeed);

        // Update the PageOptionsEnvironment page:
        PageOptionsEnvironment *poe = PageOptionsEnvironment::instance();
        if (poe != NULL)
        {
            poe->reinitialise();
            float selTime = ds_->readFloat("seltime", -1.0f);
            poe->setSelTime(selTime);
        }
    }

    /*virtual*/ 
    bool 
    EnvironmentUndo::iseq
    (
        UndoRedo::Operation         const &other
    ) const
    {
        return false;
    }
}


bool PageOptionsEnvironment::wasSlider( CWnd const & testScrollBar,
	CWnd const & scrollBar )
{
	BW_GUARD;

	return 
		testScrollBar.GetSafeHwnd() == scrollBar.GetSafeHwnd()
		&&
		::IsWindow(scrollBar.GetSafeHwnd()) != FALSE;
}


void SkyDomeListBox::MeasureItem( LPMEASUREITEMSTRUCT lpMeasureItemStruct )
{
	LPCTSTR lpszText = _T( "Pp" ); // use this to get the default height

	if ((int)lpMeasureItemStruct->itemID >= 0)
		lpszText = (LPCTSTR) lpMeasureItemStruct->itemData;

	CSize   sz;
	CDC*    pDC = GetDC();

	sz = pDC->GetTextExtent( lpszText );

	ReleaseDC( pDC );

	lpMeasureItemStruct->itemHeight = 2 * sz.cy;
}


void SkyDomeListBox::DrawItem( LPDRAWITEMSTRUCT lpDrawItemStruct )
{
	BW_GUARD;

	if ((int)lpDrawItemStruct->itemID >= 0)
	{
		LPCTSTR lpszText = (LPCTSTR)lpDrawItemStruct->itemData;
		ASSERT(lpszText != NULL);
		CDC dc;
		EnviroMinder &enviroMinder = WorldManager::instance().enviroMinder();
		BW::vector<Moo::VisualPtr> &skyDomes = enviroMinder.skyDomes();
		int idx = lpDrawItemStruct->itemID;
		if (idx < 0 || idx > (int)skyDomes.size() )
		{
			return;
		}

		dc.Attach(lpDrawItemStruct->hDC);

		// Save these value to restore them when done drawing.
		COLORREF crOldTextColor = dc.GetTextColor();
		COLORREF crOldBkColor = dc.GetBkColor();

		// If this item is selected, set the background color 
		// and the text color to appropriate values. Also, erase
		// rect by filling it with the background color.
		if ((lpDrawItemStruct->itemAction | ODA_SELECT) &&
		(lpDrawItemStruct->itemState & ODS_SELECTED))
		{
			dc.SetTextColor(::GetSysColor(COLOR_HIGHLIGHTTEXT));
			dc.SetBkColor(::GetSysColor(COLOR_HIGHLIGHT));
			dc.FillSolidRect(&lpDrawItemStruct->rcItem, 
			::GetSysColor(COLOR_HIGHLIGHT));
		}
		else
			dc.FillSolidRect(&lpDrawItemStruct->rcItem, crOldBkColor);

		// If this item is the partition, draw a red frame around the
		// item's rect.
		int partition = (int)enviroMinder.skyDomesPartition();

		if (idx == partition)
		{
			dc.SetTextColor(::GetSysColor(COLOR_GRAYTEXT));

			dc.DrawText(
				lpszText,
				static_cast<int>(wcslen(lpszText)),
				&lpDrawItemStruct->rcItem,
				DT_LEFT|DT_SINGLELINE|DT_VCENTER);
		}
		else
		{
			//TODO : work out why i need to do this.
			//using lpszText was screwing up badly, even though that
			//code is cut/paste straight from the MFC docs.
			BW::string item;
			if (!skyDomes.empty())
			{
				if (idx > partition)
				{
					item = skyDomes[idx-1]->resourceID();
				}
				else
				{
					item = skyDomes[idx]->resourceID();
				}
				item = BWResource::getFilename(item).to_string();
				dc.DrawText(
					bw_utf8tow( item ).c_str(),
					static_cast<int>(item.length()),
					&lpDrawItemStruct->rcItem,
					DT_LEFT|DT_SINGLELINE|DT_VCENTER);
			}
		}

		// Reset the background color and the text color back to their
		// original values.
		dc.SetTextColor(crOldTextColor);
		dc.SetBkColor(crOldBkColor);

		dc.Detach();
	}
}


PageOptionsEnvironment::PageOptionsEnvironment() :
    CFormView(IDD),
    colourTimeline_(NULL),
    colourPicker_(NULL),
    initialised_(false),
    filterChange_(0),
    mode_(MODE_SUN),
    initialValue_(0.0f),
    sliding_(false),
	terrainTextureLod_( filterChange_, NULL )
{
    s_instance_ = this;
}


/*virtual*/ PageOptionsEnvironment::~PageOptionsEnvironment()
{
	BW_GUARD;

    s_instance_ = NULL;
	
	bw_safe_delete(colourTimeline_);
	bw_safe_delete(colourPicker_);
}


/*static*/ PageOptionsEnvironment *PageOptionsEnvironment::instance()
{
    return s_instance_;
}


void PageOptionsEnvironment::reinitialise()
{
	BW_GUARD;

    ++filterChange_;

    CDataExchange ddx(this, FALSE);
    DoDataExchange(&ddx);

    EnviroMinder    &enviroMinder   = getEnviroMinder();
    TimeOfDay       &tod            = getTimeOfDay();
    SkyGradientDome *skd            = enviroMinder.skyGradientDome();

    skyFileEdit_.SetWindowText(bw_utf8tow(enviroMinder.skyGradientDomeFile()).c_str());
    todFileEdit_.SetWindowText(bw_utf8tow(enviroMinder.timeOfDayFile()).c_str());

    rebuildSkydomeList();

    if (skd != NULL)
        skyBoxGradEdit_.SetWindowText(bw_utf8tow(skd->textureName()).c_str());

    hourLength_.SetValue(WorldManager::instance().secondsPerHour());

    float stime = tod.startTime();
    startTime_.SetValue(stime);

    float sunAngle = tod.sunAngle();
    sunAngleSlider_.setDigits( 2 );
    sunAngleSlider_.setRangeLimit( 0, 90.0f );
    sunAngleSlider_.setRange( 0, 90.0f );
    sunAngleSlider_.setValue( sunAngle );
    sunAngleEdit_.SetNumDecimals( 2 );
    sunAngleEdit_.SetValue( sunAngle );

    float moonAngle = tod.moonAngle();
    moonAngleSlider_.setDigits( 2 );
    moonAngleSlider_.setRangeLimit( 0, 90.0f );
    moonAngleSlider_.setRange( 0, 90.0f );
    moonAngleSlider_.setValue( moonAngle );
    moonAngleEdit_.SetNumDecimals( 2 );
    moonAngleEdit_.SetValue( moonAngle );

	Vector3 col = tod.sunColor();
	sunColorREdit_.SetNumericType(controls::EditNumeric::ENT_INTEGER);
	sunColorREdit_.SetValue(col.x);
	sunColorGEdit_.SetNumericType(controls::EditNumeric::ENT_INTEGER);
	sunColorGEdit_.SetValue(col.y);
	sunColorBEdit_.SetNumericType(controls::EditNumeric::ENT_INTEGER);
	sunColorBEdit_.SetValue(col.z);

	col = tod.moonColor();
	moonColorREdit_.SetNumericType(controls::EditNumeric::ENT_INTEGER);
	moonColorREdit_.SetValue(col.x);
	moonColorGEdit_.SetNumericType(controls::EditNumeric::ENT_INTEGER);
	moonColorGEdit_.SetValue(col.y);
	moonColorBEdit_.SetNumericType(controls::EditNumeric::ENT_INTEGER);
	moonColorBEdit_.SetValue(col.z);

    float gameTime = tod.gameTime();
    timeOfDaySlider_.SetRange(0, BW_ROUND_TO_INT(SLIDER_PREC*24.0f));
    timeOfDaySlider_.SetPos(BW_ROUND_TO_INT(SLIDER_PREC*gameTime));
    BW::string gameTimeStr = tod.getTimeOfDayAsString();
    timeOfDayEdit_.SetWindowText(bw_utf8tow(gameTimeStr).c_str());

    if (skd != NULL)
    {
		mieSlider_.setDigits( 2 );
		mieSlider_.setRangeLimit( 0, 1.0f );
		mieSlider_.setRange( 0, 1.0f );
		mieSlider_.setValue( skd->mieEffect() );
		mieEdit_.SetNumDecimals( 2 );
		mieEdit_.SetValue( skd->mieEffect() );

		turbOffsSlider_.setDigits( 2 );
		turbOffsSlider_.setRangeLimit( 0, 1.0f );
		turbOffsSlider_.setRange( 0, 1.0f );
		turbOffsSlider_.setValue( skd->turbidityOffset() );
		turbOffsEdit_.SetNumDecimals( 2 );
		turbOffsEdit_.SetValue( skd->turbidityOffset() );

		turbFactorSlider_.setDigits( 2 );
		turbFactorSlider_.setRangeLimit( 0.1f, 1.0f );
		turbFactorSlider_.setRange( 0.1f, 1.0f );
		turbFactorSlider_.setValue( skd->turbidityFactor() );
		turbFactorEdit_.SetNumDecimals( 2 );
		turbFactorEdit_.SetValue( skd->turbidityFactor() );

		vertexHeightEffectSlider_.setDigits( 2 );
		vertexHeightEffectSlider_.setRangeLimit( 0, 2.0f );
		vertexHeightEffectSlider_.setRange( 0, 2.0f );
		vertexHeightEffectSlider_.setValue( skd->vertexHeightEffect() );
		vertexHeightEffectEdit_.SetNumDecimals( 2 );
		vertexHeightEffectEdit_.SetValue( skd->vertexHeightEffect() );

		sunHeightEffectSlider_.setDigits( 2 );
		sunHeightEffectSlider_.setRangeLimit( 0, 2.0f );
		sunHeightEffectSlider_.setRange( 0, 2.0f );
		sunHeightEffectSlider_.setValue( skd->sunHeightEffect() );
		sunHeightEffectEdit_.SetNumDecimals( 2 );
		sunHeightEffectEdit_.SetValue( skd->sunHeightEffect() );

		powerSlider_.setDigits( 2 );
		powerSlider_.setRangeLimit( 1.0f, 32.0f );
		powerSlider_.setRange( 1.0f, 32.0f );
		powerSlider_.setValue( skd->power() );
		powerEdit_.SetNumDecimals( 2 );
		powerEdit_.SetValue( skd->power() );
    }

	terrainTextureLod_.reinitialise();
    onModeChanged();

    --filterChange_;
}


float PageOptionsEnvironment::getSelTime() const
{
	BW_GUARD;

    if (colourTimeline_ == NULL)
        return -1.0f;
    controls::ColorScheduleItem *selItem =
        colourTimeline_->getColorScheduleItemSelected();
    if (selItem == NULL)
        return -1.0f;
    return selItem->normalisedTime_;
}


void PageOptionsEnvironment::setSelTime(float time)
{
	BW_GUARD;

    if (colourTimeline_ == NULL)
        return;
    colourTimeline_->setColorScheduleItemSelected(time);
    timelineChanged();
}


/*static*/ EnviroMinder &PageOptionsEnvironment::getEnviroMinder()
{
	BW_GUARD;

    return WorldManager::instance().enviroMinder();
}


/*static*/ TimeOfDay &PageOptionsEnvironment::getTimeOfDay()
{
	BW_GUARD;

    return *WorldManager::instance().timeOfDay();
}


void PageOptionsEnvironment::InitPage()
{
	BW_GUARD;

    ++filterChange_;

    reinitialise();

    skyBrowseFileBtn_.setBitmapID(IDB_OPEN     , IDB_OPEND     , RGB(255, 255, 255));
    skyCopyFileBtn_  .setBitmapID(IDB_DUPLICATE, IDB_DUPLICATED, RGB(  0, 128, 128));
    todBrowseFileBtn_.setBitmapID(IDB_OPEN     , IDB_OPEND     , RGB(255, 255, 255));
    todCopyFileBtn_  .setBitmapID(IDB_DUPLICATE, IDB_DUPLICATED, RGB(  0, 128, 128));

    mode_ = MODE_SUN;
    sunAnimBtn_.SetCheck(BST_CHECKED);
    ambAnimBtn_.SetCheck(BST_UNCHECKED);
    onModeChanged();


	if( IsWindow( terrainTextureLod_ ) == FALSE )
	{
		terrainTextureLod_.SubclassDlgItem( IDC_TERRAIN_TEXTURE_LOD, this );
		terrainTextureLod_.reinitialise();
	}

    if (!initialised_)
	{
        INIT_AUTO_TOOLTIP();
		terrainTextureLod_.initializeToolTips();
		// Initialise the toolbar after INIT_AUTO_TOOLTIP, otherwise it adds a
		// catch-all tooltip for every toolbar command.
		if (skyDomesTB_.GetSafeHwnd() == NULL)
		{
			skyDomesTB_.CreateEx
				(
				this, 
				TBSTYLE_FLAT, 
				WS_CHILD | WS_VISIBLE | CBRS_ALIGN_TOP
				);
			skyDomesTB_.LoadToolBarEx( IDR_SKYDOME_TB, IDR_SKYDOME_DIS_TB );
			skyDomesTB_.SetBarStyle( CBRS_ALIGN_TOP | CBRS_TOOLTIPS | CBRS_FLYBY );              
			skyDomesTB_.Subclass( IDC_SKYDOME_TB );
			skyDomesTB_.ShowWindow( SW_SHOW );
		}
	}

    --filterChange_;

    initialised_ = true;
}


/*afx_msg*/ 
LRESULT 
PageOptionsEnvironment::OnUpdateControls
(
    WPARAM      /*wParam*/, 
    LPARAM      /*lParam*/
)
{
	BW_GUARD;

    if (!initialised_)
        InitPage();

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


/*afx_msg*/ 
LRESULT 
PageOptionsEnvironment::OnNewSpace
(
    WPARAM      /*wParam*/, 
    LPARAM      /*lParam*/
)
{
	BW_GUARD;

    InitPage(); // reinitialise due to the new space
    return 0;
}


/*afx_msg*/ 
LRESULT 
PageOptionsEnvironment::OnBeginSave
(
    WPARAM      /*wParam*/, 
    LPARAM      /*lParam*/
)
{
	BW_GUARD;

    TimeOfDay &tod = getTimeOfDay();
    tod.secondsPerGameHour(WorldManager::instance().secondsPerHour());
    return 0;
}


/*afx_msg*/ 
LRESULT 
PageOptionsEnvironment::OnEndSave
(
    WPARAM      /*wParam*/, 
    LPARAM      /*lParam*/
)
{
	BW_GUARD;

    TimeOfDay &tod = getTimeOfDay();
    tod.secondsPerGameHour(0.0f);
    return 0;
}


/*virtual*/ void PageOptionsEnvironment::DoDataExchange(CDataExchange *dx)
{
    CFormView::DoDataExchange(dx);

    DDX_Control(dx, IDC_SKYFILE_EDIT           , skyFileEdit_             );
    DDX_Control(dx, IDC_SKYFILE_BTN            , skyBrowseFileBtn_        );
    DDX_Control(dx, IDC_NEWSKYFILE_BTN         , skyCopyFileBtn_          );
    DDX_Control(dx, IDC_TODFILE_EDIT           , todFileEdit_             );
    DDX_Control(dx, IDC_TODFILE_BTN            , todBrowseFileBtn_        );
    DDX_Control(dx, IDC_NEWTODFILE_BTN         , todCopyFileBtn_          );
                                                                        
    DDX_Control(dx, IDC_SKYDOME_LIST           , skyDomesList_            );
    DDX_Control(dx, IDC_ADDSKYDOME_BTN         , skyDomesAddBtn_          );
    DDX_Control(dx, IDC_CLEARSKYDOME_BTN       , skyDomesClearBtn_        );
    DDX_Control(dx, IDC_SB_GRAD_EDIT           , skyBoxGradEdit_          );
    DDX_Control(dx, IDC_SB_GRAD_BTN            , skyBoxGradBtn_           );
                                                                        
    DDX_Control(dx, IDC_HOURLENGTH             , hourLength_              );
    DDX_Control(dx, IDC_STARTTIME              , startTime_               );

	//Sun
	DDX_Control(dx, IDC_SUNANGLE_EDIT									, sunAngleEdit_            );
	DDX_Control(dx, IDC_SUNANGLE_SLIDER									, sunAngleSlider_          );
	DDX_Control(dx, IDC_SUNCOLOR_R_EDIT									, sunColorREdit_           );
	DDX_Control(dx, IDC_SUNCOLOR_G_EDIT									, sunColorGEdit_           );
	DDX_Control(dx, IDC_SUNCOLOR_B_EDIT									, sunColorBEdit_           );

	//Moon
	DDX_Control(dx, IDC_MOONANGLE_EDIT									, moonAngleEdit_           );
	DDX_Control(dx, IDC_MOONANGLE_SLIDER								, moonAngleSlider_         );
	DDX_Control(dx, IDC_MOONCOLOR_R_EDIT								, moonColorREdit_          );
	DDX_Control(dx, IDC_MOONCOLOR_G_EDIT								, moonColorGEdit_          );
	DDX_Control(dx, IDC_MOONCOLOR_B_EDIT								, moonColorBEdit_          );

    DDX_Control(dx, IDC_TIMEOFDAY_SLIDER       , timeOfDaySlider_         );
    DDX_Control(dx, IDC_TIMEOFDAY_EDIT         , timeOfDayEdit_           );
                                                                          
    DDX_Control(dx, IDC_SUNANIM_BTN            , sunAnimBtn_              );
    DDX_Control(dx, IDC_AMBANIM_BTN            , ambAnimBtn_              );
    DDX_Control(dx, IDC_CREATEANIM_BTN         , resetBtn_                );
    DDX_Control(dx, IDC_R_EDIT                 , rEdit_                   );
    DDX_Control(dx, IDC_G_EDIT                 , gEdit_                   );
    DDX_Control(dx, IDC_B_EDIT                 , bEdit_                   );
    DDX_Control(dx, IDC_ADDCOLOR_BTN           , addClrBtn_               );
    DDX_Control(dx, IDC_DELCOLOR_BTN           , delClrBtn_               );
                                                                          
    DDX_Control(dx, IDC_MIEAMOUNT              , mieEdit_                 );
    DDX_Control(dx, IDC_MIEAMOUNT_SLIDER       , mieSlider_               );
    DDX_Control(dx, IDC_TURBOFFS               , turbOffsEdit_            );
    DDX_Control(dx, IDC_TURBOFFS_SLIDER        , turbOffsSlider_          );
    DDX_Control(dx, IDC_TURBFACTOR             , turbFactorEdit_          );
    DDX_Control(dx, IDC_TURBFACTOR_SLIDER      , turbFactorSlider_        );
    DDX_Control(dx, IDC_VERTHEIGHTEFFECT       , vertexHeightEffectEdit_  );
    DDX_Control(dx, IDC_VERTHEIGHTEFFECT_SLIDER, vertexHeightEffectSlider_);
    DDX_Control(dx, IDC_SUNHEIGHTEFFECT        , sunHeightEffectEdit_     );
    DDX_Control(dx, IDC_SUNHEIGHTEFFECT_SLIDER , sunHeightEffectSlider_   );
    DDX_Control(dx, IDC_POWER                  , powerEdit_               );
    DDX_Control(dx, IDC_POWER_SLIDER           , powerSlider_             );
}


/*afx_msg*/ void PageOptionsEnvironment::OnBrowseSkyFile()
{
	BW_GUARD;

    wchar_t *filter = L"Sky files (*.xml)|*.xml|All Files (*.*)|*.*||";

    BWFileDialog 
        openDlg
        (
            false,
            L"XML",
            NULL,
            BWFileDialog::FD_FILE_PATH_MUST_EXIST,
            filter,
            AfxGetMainWnd()
        );
    if (openDlg.showDialog())
    { 
        BW::string filename = bw_wtoutf8(openDlg.getFileName());
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
            WaitCursor waitCursor;
            saveUndoState(LocaliseUTF8(L"WORLDEDITOR/GUI/PAGE_OPTIONS_ENVIRONMENT/SET_SKY_FILE"));
            EnviroMinder &enviroMinder = getEnviroMinder();
            enviroMinder.skyGradientDomeFile(dissolvedFilename);
            reinitialise();
            WorldManager::instance().environmentChanged();
        }
    }
}


/*afx_msg*/ void PageOptionsEnvironment::OnCopySkyFile()
{
	BW_GUARD;

    wchar_t *filter = L"Sky files (*.xml)|*.xml|All Files (*.*)|*.*||";

    EnviroMinder &enviroMinder = getEnviroMinder();
    BW::string currentSkyFile = enviroMinder.skyGradientDomeFile();
    currentSkyFile = BWResource::resolveFilename(currentSkyFile);
    StringUtils::replace(currentSkyFile, BW::string("/"), BW::string("\\"));

    BWFileDialog 
        saveDlg
        (
            false,
            L"XML",
            bw_utf8tow(currentSkyFile).c_str(),
            BWFileDialog::FD_OVERWRITEPROMPT,
            filter,
            AfxGetMainWnd()
        );
    if (saveDlg.showDialog())
    { 
        BW::string newFilename = bw_wtoutf8(saveDlg.getFileName());
        StringUtils::replace(newFilename, BW::string("\\"), BW::string("/"));
        BW::string dissolvedFilename = 
            BWResource::dissolveFilename(newFilename);
        // The saved to file must be in BW_RESPATH:
        if (strcmpi(dissolvedFilename.c_str(), newFilename.c_str()) == 0)
        {
			BW::wstring msg = Localise(L"RCST_IDS_NOTRESPATH");
            AfxMessageBox(msg.c_str());
        }
        else
        {   
            // The saved file name must be different to the old file name:
            BW::string source  = currentSkyFile;
            BW::string dest    = newFilename;

            StringUtils::replace(source, BW::string("/"), BW::string("\\"));
            StringUtils::replace(dest  , BW::string("/"), BW::string("\\"));

            if (strcmpi(source.c_str(), dest.c_str()) == 0)
            {
				BW::string msg = sformat(IDS_FILESMUSTDIFFER, newFilename);
                AfxMessageBox(bw_utf8tow(msg.c_str()).c_str());
            }
            else
            {
                CWaitCursor waitCursor;
                saveUndoState(LocaliseUTF8(L"WORLDEDITOR/GUI/PAGE_OPTIONS_ENVIRONMENT/COPY_SKY_FILE"));
                SkyGradientDome *skd = enviroMinder.skyGradientDome();
                DataSectionPtr section = BWResource::openSection(newFilename, true);
                skd->save(section);
                section->save();
                enviroMinder.skyGradientDomeFile(dissolvedFilename);
                reinitialise();
                WorldManager::instance().environmentChanged();
            }
        }
    }
}


/*afx_msg*/ void PageOptionsEnvironment::OnBrowseTODFile()
{
	BW_GUARD;

    wchar_t *filter = L"Time of day files (*.xml)|*.xml|All Files (*.*)|*.*||";

    BWFileDialog 
        openDlg
        (
            false,
            L"XML",
            NULL,
            BWFileDialog::FD_FILE_PATH_MUST_EXIST,
            filter,
            AfxGetMainWnd()
        );
    if (openDlg.showDialog())
    { 
        BW::string filename = bw_wtoutf8(openDlg.getFileName());
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
            saveUndoState(LocaliseUTF8(L"WORLDEDITOR/GUI/PAGE_OPTIONS_ENVIRONMENT/SET_TIME_OF_DAY_FILE"));
            EnviroMinder &enviroMinder = getEnviroMinder();
            enviroMinder.timeOfDayFile(dissolvedFilename);
            reinitialise();
            WorldManager::instance().environmentChanged();
        }
    }
}


/*afx_msg*/ void PageOptionsEnvironment::OnCopyTODFile()
{
	BW_GUARD;

	wchar_t * filter = L"Sky files (*.xml)|*.xml|All Files (*.*)|*.*||";

    EnviroMinder &enviroMinder = getEnviroMinder();
    BW::string currentTODFile = enviroMinder.timeOfDayFile();
    currentTODFile = BWResource::resolveFilename(currentTODFile);
	StringUtils::replace(currentTODFile, BW::string("/"), BW::string("\\"));

    BWFileDialog 
        saveDlg
        (
            false,
            L"XML",
            bw_utf8tow(currentTODFile).c_str(),
            BWFileDialog::FD_OVERWRITEPROMPT,
            filter,
            AfxGetMainWnd()
        );
    if (saveDlg.showDialog())
    { 
        BW::string newFilename = bw_wtoutf8( saveDlg.getFileName());
		StringUtils::replace(newFilename, BW::string("\\"), BW::string("/"));
        BW::string dissolvedFilename = 
            BWResource::dissolveFilename(newFilename);
        if (strcmpi(dissolvedFilename.c_str(), newFilename.c_str()) == 0)
        {
			BW::wstring msg = Localise(L"RCST_IDS_NOTRESPATH");
            AfxMessageBox(msg.c_str());
        }
        else
        {
            // The saved file name must be different to the old file name:
            BW::string source  = currentTODFile;
            BW::string dest    = newFilename;

			StringUtils::replace(source, BW::string("/"), BW::string("\\"));
			StringUtils::replace(dest  , BW::string("/"), BW::string("\\"));

            if (strcmpi(source.c_str(), dest.c_str()) == 0)
            {
				BW::string msg = sformat(IDS_FILESMUSTDIFFER, newFilename);
                AfxMessageBox(bw_utf8tow(msg.c_str()).c_str());
            }
            else
            {
                WaitCursor waitCursor;
                saveUndoState(LocaliseUTF8(L"WORLDEDITOR/GUI/PAGE_OPTIONS_ENVIRONMENT/COPY_TIME_OF_DAY_FILE"));
                TimeOfDay &tod = getTimeOfDay();
                tod.save(newFilename);
                enviroMinder.timeOfDayFile(dissolvedFilename);
                reinitialise();
                WorldManager::instance().environmentChanged();
            }
        }
    }
}


/*afx_msg*/  void PageOptionsEnvironment::OnAddSkyDome()
{
	BW_GUARD;

    wchar_t *filter = L"Visuals (*.visual)|*.visual|All Files (*.*)|*.*||";

    BWFileDialog 
        openDlg
        (
            false,
            L"TGA",
            NULL,
            BWFileDialog::FD_FILE_PATH_MUST_EXIST,
            filter,
            AfxGetMainWnd()
        );
    if (openDlg.showDialog())
    {        
        BW::string filename = bw_wtoutf8(openDlg.getFileName());
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
            Moo::VisualPtr skyDome = 
                Moo::VisualManager::instance()->get(dissolvedFilename);
            if (skyDome == NULL)
            {
                waitCursor.Restore();
				BW::wstring msg = Localise(L"RCST_IDS_NOLOADSKYDOME"); 
                AfxMessageBox(msg.c_str());
            }
            else
            {
                saveUndoState(LocaliseUTF8(L"WORLDEDITOR/GUI/PAGE_OPTIONS_ENVIRONMENT/ADD_SKY_DOME"));
                EnviroMinder &enviroMinder = getEnviroMinder();
                enviroMinder.skyDomes().push_back(skyDome);
                dissolvedFilename = 
                    BWResource::getFilename(dissolvedFilename).to_string();
                skyDomesList_.AddString(bw_utf8tow(dissolvedFilename).c_str());
                WorldManager::instance().environmentChanged();
            }
        }
    }
}


/*afx_msg*/  void PageOptionsEnvironment::OnClearSkyDomes()
{
	BW_GUARD;

    EnviroMinder &enviroMinder = getEnviroMinder();
    if (!enviroMinder.skyDomes().empty())
    {
        saveUndoState(LocaliseUTF8(L"WORLDEDITOR/GUI/PAGE_OPTIONS_ENVIRONMENT/CLEAR_SKY_DOME"));
        enviroMinder.skyDomes().clear();
        skyDomesList_.ResetContent();
		skyDomesList_.AddString( Localise(L"WORLDEDITOR/GUI/PAGE_OPTIONS_ENVIRONMENT/WEATHER_PARTITION") );
		enviroMinder.skyDomesPartition(0);
        WorldManager::instance().environmentChanged();
    }
}


/*afx_msg*/ void PageOptionsEnvironment::OnBrowseSkyGradBtn()
{
	BW_GUARD;

    EnviroMinder    &enviroMinder   = getEnviroMinder();
    TimeOfDay       &tod            = getTimeOfDay();
    SkyGradientDome *skd            = enviroMinder.skyGradientDome();

    if (skd == NULL)
        return;

    BW::string origFilename = skd->textureName();
    BW::string filename     = BWResource::resolveFilename(origFilename);
    wchar_t *filter     = L"TGA Images (*.tga)|*.tga|All Files (*.*)|*.*||";

	StringUtils::replace( filename, BW::string("/"), BW::string("\\") );

    BWFileDialog 
        openDlg
        (
            true,
            L"TGA",
            bw_utf8tow(filename).c_str(),
            BWFileDialog::FD_FILE_PATH_MUST_EXIST,
            filter,
            AfxGetMainWnd()
        );

    if (openDlg.showDialog())
    {
        BW::string newFilename = bw_wtoutf8(openDlg.getFileName());
		StringUtils::replace(newFilename, BW::string("\\"), BW::string("/"));
        BW::string dissolvedFilename = BWResource::dissolveFilename(newFilename);
        if (strcmpi(dissolvedFilename.c_str(), newFilename.c_str()) == 0)
        {
			BW::wstring msg = Localise(L"RCST_IDS_NOTRESPATH");
            AfxMessageBox(msg.c_str());
        }
        else
        {
            saveUndoState(LocaliseUTF8(L"WORLDEDITOR/GUI/PAGE_OPTIONS_ENVIRONMENT/LOAD_SKY_BOX_TEXTURE"));
            bool ok = skd->loadTexture(dissolvedFilename);
            if (ok)
            {
                skyBoxGradEdit_.SetWindowText(bw_utf8tow(dissolvedFilename).c_str());
            }
            else
            {
                UndoRedo::instance().undo();
                skd->loadTexture(origFilename);
				BW::wstring msg = Localise(L"RCST_IDS_NOLOADSKYGRADIENT");
                AfxMessageBox(msg.c_str());
            }
        }
    }
}


/*afx_msg*/ void PageOptionsEnvironment::OnSkyboxUpEnable(CCmdUI *cmdui)
{
	BW_GUARD;

    int sel = skyDomesList_.GetCurSel();
    if (sel != 0 && sel != CB_ERR)
        cmdui->Enable(TRUE);
    else
        cmdui->Enable(FALSE);
}


/*afx_msg*/ void PageOptionsEnvironment::OnSkyboxDownEnable(CCmdUI *cmdui)
{
	BW_GUARD;

    int sel = skyDomesList_.GetCurSel();
    if (sel != skyDomesList_.GetCount() - 1 && sel != CB_ERR)
        cmdui->Enable(TRUE);
    else
        cmdui->Enable(FALSE);
}


/*afx_msg*/ void PageOptionsEnvironment::OnSkyboxDelEnable(CCmdUI *cmdui)
{
	BW_GUARD;

    int sel = skyDomesList_.GetCurSel();
	EnviroMinder &enviroMinder = getEnviroMinder();
	size_t partition = enviroMinder.skyDomesPartition();

	if (sel != CB_ERR && sel != partition)
        cmdui->Enable(TRUE);
    else 
        cmdui->Enable(FALSE);
}


/*afx_msg*/ void PageOptionsEnvironment::OnSkyboxUp()
{
	BW_GUARD;

    // Get the selection:
    int intSel = skyDomesList_.GetCurSel();
    if (intSel == CB_ERR || intSel == 0)
        return;

    // Save current state:
    saveUndoState(LocaliseUTF8(L"WORLDEDITOR/GUI/PAGE_OPTIONS_ENVIRONMENT/MOVE_SKY_DOME_UP"));

	size_t sel = (size_t)intSel;
    
    // Remove the skydome and add it back in at it's new position:
    EnviroMinder &enviroMinder = getEnviroMinder();
	size_t partition = enviroMinder.skyDomesPartition();
	if (sel == partition)
	{
		enviroMinder.skyDomesPartition( partition-1 );
	}
	else if (sel == partition+1)
	{
		//moving the skydome just below the partition, up, is
		//the same as moving the partition down by 1.
		enviroMinder.skyDomesPartition( partition+1 );
	}
	else
	{
		size_t idx = (sel > partition) ? sel-1 : sel;
		BW::vector<Moo::VisualPtr> &skyDomes = enviroMinder.skyDomes();
		Moo::VisualPtr selSkyDome = skyDomes[idx];
		skyDomes.erase(skyDomes.begin() + idx);
		skyDomes.insert(skyDomes.begin() + idx - 1, selSkyDome);
	}

    // Update the control:
    rebuildSkydomeList();

    // Set the selection:
    skyDomesList_.SetCurSel(static_cast<int>(sel - 1));
}


/*afx_msg*/ void PageOptionsEnvironment::OnSkyboxDown()
{
	BW_GUARD;

    // Get the selection:
    int intSel = skyDomesList_.GetCurSel();
    if (intSel == CB_ERR)
        return;

    // Save current state:
    saveUndoState(LocaliseUTF8(L"WORLDEDITOR/GUI/PAGE_OPTIONS_ENVIRONMENT/MOVE_SKY_DOME_DOWN"));

	size_t sel = (size_t)intSel;
    
    // Remove the skydome and add it back in at it's new position:
    EnviroMinder &enviroMinder = getEnviroMinder();
	size_t partition = enviroMinder.skyDomesPartition();
	if (sel == partition)
	{
		enviroMinder.skyDomesPartition( partition+1 );
	}
	else if (sel == partition-1)
	{
		//moving the skydome just above the partition, down, is
		//the same as moving the partition up by 1.
		enviroMinder.skyDomesPartition( partition-1 );
	}
	else
	{
		size_t idx = (sel > partition) ? sel-1 : sel;
		BW::vector<Moo::VisualPtr> &skyDomes = enviroMinder.skyDomes();
		Moo::VisualPtr selSkyDome = skyDomes[idx];
		skyDomes.erase(skyDomes.begin() + idx);
		skyDomes.insert(skyDomes.begin() + idx + 1, selSkyDome);
	}

    // Update the control:
    rebuildSkydomeList();

    // Set the selection:
    skyDomesList_.SetCurSel(static_cast<int>(sel + 1));
}


/*afx_msg*/ void PageOptionsEnvironment::OnSkyboxDel()
{
	BW_GUARD;

	EnviroMinder &enviroMinder = getEnviroMinder();

    // Get the selection:
    int sel = skyDomesList_.GetCurSel();
    if (sel == CB_ERR)
        return;
	int partition = (int)enviroMinder.skyDomesPartition();
	if (sel == partition)
		return;
	if (sel > partition)
		sel--;

    // Save current state:
    saveUndoState(LocaliseUTF8(L"WORLDEDITOR/GUI/PAGE_OPTIONS_ENVIRONMENT/DELETE_SKY_DOME"));
    
    // Remove the skydome:
    BW::vector<Moo::VisualPtr> &skyDomes = enviroMinder.skyDomes();
    skyDomes.erase(skyDomes.begin() + sel);

	if (sel > partition)
	{
		sel++;
	}
	else
	{
		enviroMinder.skyDomesPartition( partition-1 );
	}

    // Update the control:
    rebuildSkydomeList();

    // Set the selection:
    if (sel < skyDomesList_.GetCount())
        skyDomesList_.SetCurSel(sel);
    else if (sel - 1 >= 0)
        skyDomesList_.SetCurSel(sel - 1);
    else
        skyDomesList_.SetCurSel(0);
}


/*afx_msg*/ void PageOptionsEnvironment::OnHourLengthEdit()
{
	BW_GUARD;

    if (filterChange_ == 0)
    {
        ++filterChange_;
        
        TimeOfDay &tod = getTimeOfDay();

        if (WorldManager::instance().secondsPerHour() != hourLength_.GetValue())
        {
            saveUndoState(LocaliseUTF8(L"WORLDEDITOR/GUI/PAGE_OPTIONS_ENVIRONMENT/EDIT_HOUR_LENGTH"));
            WorldManager::instance().secondsPerHour(hourLength_.GetValue());
            WorldManager::instance().environmentChanged();
        }

        --filterChange_;
    }
}


/*afx_msg*/ void PageOptionsEnvironment::OnStartTimeEdit()
{
	BW_GUARD;

    if (filterChange_ == 0)
    {
        ++filterChange_;

        TimeOfDay &tod = getTimeOfDay();

        if (tod.startTime() != startTime_.GetValue())
        {
            saveUndoState(LocaliseUTF8(L"WORLDEDITOR/GUI/PAGE_OPTIONS_ENVIRONMENT/ON_EDIT_START_TIME"));
            tod.startTime(startTime_.GetValue());
            WorldManager::instance().environmentChanged();
        }

        --filterChange_;
    }
}


/*afx_msg*/ 
void 
PageOptionsEnvironment::OnHScroll
(   
    UINT        sbcode, 
    UINT        pos, 
    CScrollBar  *scrollBar
)
{
	BW_GUARD;

    CFormView::OnHScroll(sbcode, pos, scrollBar);

    CWnd *wnd = (CWnd *)scrollBar;

    // Work out whether this is the first part of a slider movement,
    // in the middle of a slider movement or the end of a slider movement:
    SliderMovementState sms = SMS_MIDDLE;
    if (!sliding_)
    {
        sms = SMS_STARTED;
        sliding_ = true;
    }
    if (sbcode == TB_ENDTRACK)
    {
        sms = SMS_DONE;
        sliding_ = false;
    }

    if (wasSlider(*wnd, sunAngleSlider_))
    {
        OnSunAngleSlider(sms);
    }
    else if (wasSlider(*wnd, moonAngleSlider_))
    {
        OnMoonAngleSlider(sms);
    }
    else if (wasSlider(*wnd, timeOfDaySlider_))
    {
        OnTimeOfDaySlider(sms);
    }
    else if (wasSlider(*wnd, mieSlider_))
    {
        OnMIESlider(sms);
    }
    else if (wasSlider(*wnd, turbOffsSlider_))
    {
        OnTurbOffsSlider(sms);
    }
    else if (wasSlider(*wnd, turbFactorSlider_))
    {
        OnTurbFactSlider(sms);
    }
    else if (wasSlider(*wnd, vertexHeightEffectSlider_))
    {
        OnVertEffSlider(sms);
    }
    else if (wasSlider(*wnd, sunHeightEffectSlider_))
    {
        OnSunHeightEffSlider(sms);
    }
    else if (wasSlider(*wnd, powerSlider_))
    {
        OnPowerSlider(sms);
    }
}


/*afx_msg*/ 
LRESULT 
PageOptionsEnvironment::OnCTSelTime(WPARAM /*wParam*/, LPARAM lParam)
{
	BW_GUARD;

    if (filterChange_ == NULL)
    {
        ++filterChange_;
        float gameTime = 0.0001f*lParam;
        TimeOfDay &tod = getTimeOfDay();
        tod.gameTime(gameTime);
        BW::string gameTimeStr = tod.getTimeOfDayAsString();
        timeOfDayEdit_.SetWindowText(bw_utf8tow(gameTimeStr).c_str());
        timeOfDaySlider_.SetPos(BW_ROUND_TO_INT(gameTime*SLIDER_PREC));
        colourTimeline_->showLineAtTime(gameTime);

		MF_ASSERT( WorldEditorApp::instance().pythonAdapter() != NULL &&
			"PageOptionsEnvironment::OnCTSelTime: PythonAdapter is NULL" );
		WorldEditorApp::instance().pythonAdapter()->onSliderAdjust(
            "slrProjectCurrentTime", 
            timeOfDaySlider_.GetPos(), 
            timeOfDaySlider_.GetRangeMin(), 
            timeOfDaySlider_.GetRangeMax() );

        Options::setOptionInt( 
            "graphics/timeofday", 
			int( timeOfDaySlider_.GetPos() * WorldManager::TIME_OF_DAY_MULTIPLIER / SLIDER_PREC ) );

        WorldEditorApp::instance().mfApp()->updateFrame(false);
        --filterChange_;
    }
    return TRUE;
}


void PageOptionsEnvironment::OnSunAngleSlider(SliderMovementState sms)
{
	BW_GUARD;

    if (filterChange_ == 0)
    {
        ++filterChange_;

        TimeOfDay &tod = getTimeOfDay();
		float sunAngle = sunAngleSlider_.getValue();

        if (sms == SMS_STARTED)
        {
            initialValue_ = tod.sunAngle();
        }
        else if (sms == SMS_DONE)
        {
            if (initialValue_ != sunAngle)
            {
                // Save an undo state at the initial value:
                tod.sunAngle(initialValue_);
                saveUndoState(LocaliseUTF8(L"WORLDEDITOR/GUI/PAGE_OPTIONS_ENVIRONMENT/SET_SUN_ANGLE"));
                WorldManager::instance().environmentChanged();                
            }
        }

        tod.sunAngle(sunAngle);
        sunAngleEdit_.SetValue(sunAngle);

        --filterChange_;        
    }
}


/*afx_msg*/ void PageOptionsEnvironment::OnSunAngleEdit()
{
	BW_GUARD;

    if (filterChange_ == 0)
    {
        ++filterChange_;

        TimeOfDay &tod = getTimeOfDay();
        float sunAngle = sunAngleEdit_.GetValue();
        if (tod.sunAngle() != sunAngle)
        {
            saveUndoState(LocaliseUTF8(L"WORLDEDITOR/GUI/PAGE_OPTIONS_ENVIRONMENT/SET_SUN_ANGLE"));  
            sunAngleSlider_.setValue( sunAngle );            
			tod.sunAngle( sunAngleSlider_.getValue() );
            WorldManager::instance().environmentChanged();
        }

        --filterChange_;        
    }
}


void PageOptionsEnvironment::OnMoonAngleSlider(SliderMovementState sms)
{
	BW_GUARD;

    if (filterChange_ == 0)
    {
        ++filterChange_;   

        TimeOfDay &tod = getTimeOfDay();
        float moonAngle = moonAngleSlider_.getValue();

        if (sms == SMS_STARTED)
        {
            initialValue_ = tod.moonAngle();
        }
        else if (sms == SMS_DONE)
        {
            if (initialValue_ != moonAngle)
            {
                // Save an undo state at the initial value:
                tod.moonAngle(initialValue_);
                saveUndoState(LocaliseUTF8(L"WORLDEDITOR/GUI/PAGE_OPTIONS_ENVIRONMENT/SET_MOON_ANGLE"));
                WorldManager::instance().environmentChanged();                
            }
        }                 
        tod.moonAngle(moonAngle);
        moonAngleEdit_.SetValue(moonAngle);

        --filterChange_;
    }
}


/*afx_msg*/ void PageOptionsEnvironment::OnMoonAngleEdit()
{
	BW_GUARD;

    if (filterChange_ == 0)
    {
        ++filterChange_;

        TimeOfDay &tod = getTimeOfDay();

        float moonAngle = moonAngleEdit_.GetValue();
        if (tod.moonAngle() != moonAngle)
        {
            saveUndoState(LocaliseUTF8(L"WORLDEDITOR/GUI/PAGE_OPTIONS_ENVIRONMENT/SET_MOON_ANGLE"));
            moonAngleSlider_.setValue( moonAngle );
            tod.moonAngle( moonAngleSlider_.getValue() );
            WorldManager::instance().environmentChanged();
        }

        --filterChange_;        
    }
}


void PageOptionsEnvironment::OnTimeOfDaySlider(SliderMovementState /*sms*/)
{
	BW_GUARD;

    if (filterChange_ == 0)
    {
        ++filterChange_;

        TimeOfDay &tod = getTimeOfDay();
        float gameTime = timeOfDaySlider_.GetPos()/SLIDER_PREC;
        tod.gameTime(gameTime);
        BW::string gameTimeStr = tod.getTimeOfDayAsString();
        timeOfDayEdit_.SetWindowText(bw_utf8tow(gameTimeStr).c_str());

		MF_ASSERT( WorldEditorApp::instance().pythonAdapter() != NULL &&
			"PageOptionsEnvironment::OnTimeOfDaySlider: PythonAdapter is NULL" );
		WorldEditorApp::instance().pythonAdapter()->onSliderAdjust(
            "slrProjectCurrentTime", 
            timeOfDaySlider_.GetPos(), 
            timeOfDaySlider_.GetRangeMin(), 
            timeOfDaySlider_.GetRangeMax() );

        Options::setOptionInt( 
            "graphics/timeofday", 
            int( timeOfDaySlider_.GetPos() * WorldManager::TIME_OF_DAY_MULTIPLIER / SLIDER_PREC ) );

        if (colourTimeline_ != NULL)
        {
            colourTimeline_->showLineAtTime(gameTime);
        }

        --filterChange_;
    }
}


/*afx_msg*/ void PageOptionsEnvironment::OnSunAnimBtn()
{
	BW_GUARD;

    mode_ = MODE_SUN;
    onModeChanged();
}


/*afx_msg*/ void PageOptionsEnvironment::OnAmbAnimBtn()
{
	BW_GUARD;

    mode_ = MODE_AMB;
    onModeChanged();
}


/*afx_msg*/ void PageOptionsEnvironment::OnResetBtn()
{
	BW_GUARD;

    TimeOfDay &tod = getTimeOfDay();
    saveUndoState(LocaliseUTF8(L"WORLDEDITOR/GUI/PAGE_OPTIONS_ENVIRONMENT/RESET_ENVIRONMENT_ANIMATION"));
    switch (mode_)
    {
    case MODE_SUN:
        tod.clearSunAnimations();
        tod.addSunAnimation( 0.00f, Vector3( 43.0f, 112.0f, 168.0f));
        tod.addSunAnimation( 4.02f, Vector3( 28.0f,  81.0f, 110.0f));
        tod.addSunAnimation( 5.07f, Vector3( 57.0f,  81.0f, 110.0f));
        tod.addSunAnimation( 6.40f, Vector3(207.0f, 157.0f,  90.0f));
        tod.addSunAnimation( 9.30f, Vector3(246.0f, 195.0f, 157.0f));
        tod.addSunAnimation(16.95f, Vector3(246.0f, 195.0f, 157.0f));
        tod.addSunAnimation(17.87f, Vector3(244.0f, 124.0f,   4.0f));
        break;
    case MODE_AMB:
        tod.clearAmbientAnimations();
        tod.addAmbientAnimation( 0.00f, Vector3( 21.0f,  81.0f, 130.0f));
        tod.addAmbientAnimation( 5.67f, Vector3( 21.0f,  81.0f, 130.0f));
        tod.addAmbientAnimation( 8.47f, Vector3( 64.0f,  64.0f,  32.0f));
        tod.addAmbientAnimation(11.37f, Vector3( 64.0f,  64.0f,  32.0f));
        tod.addAmbientAnimation(18.00f, Vector3( 69.0f,  77.0f,  82.0f));
        break;
    default:
        ASSERT(0);
    }
    onModeChanged();
    WorldManager::instance().environmentChanged();
}


/*afx_msg*/ void PageOptionsEnvironment::OnAddClrBtn()
{
	BW_GUARD;

    if (colourTimeline_ != NULL)
    {
        colourTimeline_->addColorAtLButton();
    }
}


/*afx_msg*/ void PageOptionsEnvironment::OnDelClrBtn()
{
	BW_GUARD;

    if (colourTimeline_ != NULL)
    {
        bool ok = colourTimeline_->removeSelectedColor();
        if (ok)
        {
            saveUndoState(LocaliseUTF8(L"WORLDEDITOR/GUI/PAGE_OPTIONS_ENVIRONMENT/DELETE_ENVIRONMENT_ANIMATION_COLOR"));
            rebuildAnimation();            
        }
    }
}


/*afx_msg*/ void PageOptionsEnvironment::OnEditClrText()
{
	BW_GUARD;

    if (filterChange_ == 0)
    {
        ++filterChange_;

        int r = (int)rEdit_.GetValue();
        int g = (int)gEdit_.GetValue();
        int b = (int)bEdit_.GetValue();

        COLORREF curColor = colourPicker_->getRGB();
        COLORREF newColor = RGB(r, g, b);

        if (newColor != curColor)
        {
            if (colourPicker_ != NULL)
            {
                colourPicker_->setRGB(newColor);
            }   
            if (colourTimeline_ != NULL)
            {
                if (colourTimeline_->itemSelected())
                {
                    saveUndoState(LocaliseUTF8(L"WORLDEDITOR/GUI/PAGE_OPTIONS_ENVIRONMENT/EDIT_ENVIRONMENT_COLOR"));
                    colourTimeline_->setColorScheduleItemSelectedColor
                    (
                        Vector4(r/255.0f, g/255.0f, b/255.0f, 1.0f)
                    );
                    rebuildAnimation();
                }
            }   
        }

        --filterChange_;
    }
}


/*afx_msg*/ 
LRESULT 
PageOptionsEnvironment::OnTimelineBegin
(
    WPARAM      wparam, 
    LPARAM      lparam
)
{
	BW_GUARD;

    controls::ColorScheduleItem *item = colourTimeline_->getColorScheduleItemSelected();
    initialValue_ = item->normalisedTime_;
    timelineChanged();
    return TRUE;
}


/*afx_msg*/ 
LRESULT 
PageOptionsEnvironment::OnTimelineMiddle
(
    WPARAM      /*wparam*/, 
    LPARAM      /*lparam*/
)
{
	BW_GUARD;

    timelineChanged();
    return TRUE;
}


/*afx_msg*/ 
LRESULT 
PageOptionsEnvironment::OnTimelineDone
(
    WPARAM wparam, 
    LPARAM lparam
)
{
	BW_GUARD;

    // Create an undo position by going back to the original state:
    controls::ColorScheduleItem *item = colourTimeline_->getColorScheduleItemSelected();
    float thisTime = item->normalisedTime_;
    item->normalisedTime_ = initialValue_;
    rebuildAnimation();
    saveUndoState(LocaliseUTF8(L"WORLDEDITOR/GUI/PAGE_OPTIONS_ENVIRONMENT/EDIT_ENVIRONMENT_TIMELINE"));

    // Now do the actual update:
    item->normalisedTime_ = thisTime;
    timelineChanged();

    return TRUE;
}


/*afx_msg*/ 
LRESULT
PageOptionsEnvironment::OnTimelineAdd
(
    WPARAM      /*wparam*/, 
    LPARAM      /*lparam*/
)
{
	BW_GUARD;

    saveUndoState(LocaliseUTF8(L"WORLDEDITOR/GUI/PAGE_OPTIONS_ENVIRONMENT/ADD_ENVIRONMENT_COLOR"));
    timelineChanged();
    return TRUE;
}


/*afx_msg*/ 
LRESULT
PageOptionsEnvironment::OnTimelineNewSel
(
    WPARAM      /*wparam*/, 
    LPARAM      /*lparam*/
)
{
	BW_GUARD;

    timelineChanged();
    return TRUE;
}


/*afx_msg*/ 
LRESULT
PageOptionsEnvironment::OnPickerDown
(
    WPARAM      /*wparam*/, 
    LPARAM      /*lparam*/
)
{
	BW_GUARD;

    // Remember the initial color:
    if (colourTimeline_->itemSelected())
    {
        controls::ColorScheduleItem *item =
            colourTimeline_->getColorScheduleItemSelected();
        initialColor_ = item->color_;
    }

    pickerChanged();

    return TRUE;
}


/*afx_msg*/ 
LRESULT
PageOptionsEnvironment::OnPickerMove
(
    WPARAM      /*wparam*/, 
    LPARAM      /*lparam*/
)
{
	BW_GUARD;

    pickerChanged();

    return TRUE;
}


/*afx_msg*/ 
LRESULT
PageOptionsEnvironment::OnPickerUp
(
    WPARAM      /*wparam*/, 
    LPARAM      /*lparam*/
)
{
	BW_GUARD;

    pickerChanged();

    if (colourTimeline_->itemSelected())
    {
        // Create an undo position by going back to the original state:
        controls::ColorScheduleItem *item =
            colourTimeline_->getColorScheduleItemSelected();
        Vector4 thisColor = item->color_;
        item->color_ = initialColor_;
        rebuildAnimation();
        saveUndoState(LocaliseUTF8(L"WORLDEDITOR/GUI/PAGE_OPTIONS_ENVIRONMENT/EDIT_ENVIRONMENT_COLOR"));

        // Now change the model back again:
        item->color_ = thisColor;
        rebuildAnimation();
    }

    return TRUE;
}


/*afx_msg*/ void PageOptionsEnvironment::OnEditEnvironText()
{
	BW_GUARD;

    if (filterChange_ == 0)
    {
        ++filterChange_;

        EnviroMinder    &enviroMinder = getEnviroMinder();
        SkyGradientDome *skd          = enviroMinder.skyGradientDome();
        if (skd != NULL)
        {
            bool changed = 
                skd->mieEffect         () != mieEdit_               .GetValue()
                ||
                skd->turbidityOffset   () != turbOffsEdit_          .GetValue()
                ||
                skd->turbidityFactor   () != turbFactorEdit_        .GetValue()
                ||
                skd->vertexHeightEffect() != vertexHeightEffectEdit_.GetValue()
                ||
                skd->sunHeightEffect   () != sunHeightEffectEdit_   .GetValue()
                ||
                skd->power             () != powerEdit_             .GetValue();

            if (changed)
            {
                saveUndoState(LocaliseUTF8(L"WORLDEDITOR/GUI/PAGE_OPTIONS_ENVIRONMENT/EDIT_ATMOSPHERIC_COLOR"));

                mieSlider_               .setValue(mieEdit_               .GetValue());
                turbOffsSlider_          .setValue(turbOffsEdit_          .GetValue());
                turbFactorSlider_        .setValue(turbFactorEdit_        .GetValue());
                vertexHeightEffectSlider_.setValue(vertexHeightEffectEdit_.GetValue());
                sunHeightEffectSlider_   .setValue(sunHeightEffectEdit_   .GetValue());
                powerSlider_             .setValue(powerEdit_             .GetValue());

                skd->mieEffect         (mieSlider_               .getValue());
                skd->turbidityOffset   (turbOffsSlider_          .getValue());
                skd->turbidityFactor   (turbFactorSlider_        .getValue());
                skd->vertexHeightEffect(vertexHeightEffectSlider_.getValue());
                skd->sunHeightEffect   (sunHeightEffectSlider_   .getValue());
                skd->power             (powerSlider_             .getValue());

                WorldManager::instance().environmentChanged();
            }
        }

        --filterChange_;
    }
}


void PageOptionsEnvironment::OnMIESlider(SliderMovementState sms)
{
	BW_GUARD;

    if (filterChange_ == 0)
    {
        ++filterChange_;   

        EnviroMinder    &enviroMinder = getEnviroMinder();
        SkyGradientDome *skd          = enviroMinder.skyGradientDome();
        float           mieValue      = mieSlider_.getValue();

        if (sms == SMS_STARTED)
        {
            initialValue_ = skd->mieEffect();
        }
        else if (sms == SMS_DONE)
        {
            if (initialValue_ != mieValue)
            {
                // Save an undo state at the initial value:
                skd->mieEffect(initialValue_);
                saveUndoState(LocaliseUTF8(L"WORLDEDITOR/GUI/PAGE_OPTIONS_ENVIRONMENT/EDIT_ATMOSPHERIC_COLOR"));
                WorldManager::instance().environmentChanged();                
            }
        }                 
        skd->mieEffect(mieValue);
        mieEdit_.SetValue(mieValue);

        --filterChange_;
    }
}


void PageOptionsEnvironment::OnTurbOffsSlider(SliderMovementState sms)
{
	BW_GUARD;

    if (filterChange_ == 0)
    {
        ++filterChange_;   

        EnviroMinder    &enviroMinder = getEnviroMinder();
        SkyGradientDome *skd          = enviroMinder.skyGradientDome();
        float           turbValue     = turbOffsSlider_.getValue();

        if (sms == SMS_STARTED)
        {
            initialValue_ = skd->turbidityOffset();
        }
        else if (sms == SMS_DONE)
        {
            if (initialValue_ != turbValue)
            {
                // Save an undo state at the initial value:
                skd->turbidityOffset(initialValue_);
                saveUndoState(LocaliseUTF8(L"WORLDEDITOR/GUI/PAGE_OPTIONS_ENVIRONMENT/EDIT_ATMOSPHERIC_COLOR"));
                WorldManager::instance().environmentChanged();                
            }
        }                 
        skd->turbidityOffset(turbValue);
        turbOffsEdit_.SetValue(turbValue);

        --filterChange_;
    }
}


void PageOptionsEnvironment::OnTurbFactSlider(SliderMovementState sms)
{
	BW_GUARD;

    if (filterChange_ == 0)
    {
        ++filterChange_;   

        EnviroMinder    &enviroMinder = getEnviroMinder();
        SkyGradientDome *skd          = enviroMinder.skyGradientDome();
        float           turbValue     = turbFactorSlider_.getValue();

        if (sms == SMS_STARTED)
        {
            initialValue_ = skd->turbidityFactor();
        }
        else if (sms == SMS_DONE)
        {
            if (initialValue_ != turbValue)
            {
                // Save an undo state at the initial value:
                skd->turbidityFactor(initialValue_);
                saveUndoState(LocaliseUTF8(L"WORLDEDITOR/GUI/PAGE_OPTIONS_ENVIRONMENT/EDIT_ATMOSPHERIC_COLOR"));
                WorldManager::instance().environmentChanged();                
            }
        }                 
        skd->turbidityFactor(turbValue);
        turbFactorEdit_.SetValue(turbValue);

        --filterChange_;
    }
}


void PageOptionsEnvironment::OnVertEffSlider(SliderMovementState sms)
{
	BW_GUARD;

    if (filterChange_ == 0)
    {
        ++filterChange_;   

        EnviroMinder    &enviroMinder = getEnviroMinder();
        SkyGradientDome *skd          = enviroMinder.skyGradientDome();
        float           vertValue     = vertexHeightEffectSlider_.getValue();

        if (sms == SMS_STARTED)
        {
            initialValue_ = skd->vertexHeightEffect();
        }
        else if (sms == SMS_DONE)
        {
            if (initialValue_ != vertValue)
            {
                // Save an undo state at the initial value:
                skd->vertexHeightEffect(initialValue_);
                saveUndoState(LocaliseUTF8(L"WORLDEDITOR/GUI/PAGE_OPTIONS_ENVIRONMENT/EDIT_ATMOSPHERIC_COLOR"));
                WorldManager::instance().environmentChanged();                
            }
        }                 
        skd->vertexHeightEffect(vertValue);
        vertexHeightEffectEdit_.SetValue(vertValue);

        --filterChange_;
    }
}


void PageOptionsEnvironment::OnSunHeightEffSlider(SliderMovementState sms)
{
	BW_GUARD;

    if (filterChange_ == 0)
    {
        ++filterChange_;   

        EnviroMinder    &enviroMinder = getEnviroMinder();
        SkyGradientDome *skd          = enviroMinder.skyGradientDome();
        float           sunValue      = sunHeightEffectSlider_.getValue();

        if (sms == SMS_STARTED)
        {
            initialValue_ = skd->sunHeightEffect();
        }
        else if (sms == SMS_DONE)
        {
            if (initialValue_ != sunValue)
            {
                // Save an undo state at the initial value:
                skd->sunHeightEffect(initialValue_);
                saveUndoState(LocaliseUTF8(L"WORLDEDITOR/GUI/PAGE_OPTIONS_ENVIRONMENT/EDIT_ATMOSPHERIC_COLOR"));
                WorldManager::instance().environmentChanged();                
            }
        }                 
        skd->sunHeightEffect(sunValue);
        sunHeightEffectEdit_.SetValue(sunValue);

        --filterChange_;
    }
}


void PageOptionsEnvironment::OnPowerSlider(SliderMovementState sms)
{
	BW_GUARD;

    if (filterChange_ == 0)
    {
        ++filterChange_;   

        EnviroMinder    &enviroMinder = getEnviroMinder();
        SkyGradientDome *skd          = enviroMinder.skyGradientDome();
        float           powerValue    = powerSlider_.getValue();

        if (sms == SMS_STARTED)
        {
            initialValue_ = skd->power();
        }
        else if (sms == SMS_DONE)
        {
            if (initialValue_ != powerValue)
            {
                // Save an undo state at the initial value:
                skd->power(initialValue_);
                saveUndoState(LocaliseUTF8(L"WORLDEDITOR/GUI/PAGE_OPTIONS_ENVIRONMENT/EDIT_ATMOSPHERIC_COLOR"));
                WorldManager::instance().environmentChanged();                
            }
        }                 
        skd->power(powerValue);
        powerEdit_.SetValue(powerValue);

        --filterChange_;
    }
}


BOOL PageOptionsEnvironment::OnToolTipText(UINT, NMHDR* pNMHDR, LRESULT *result)
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
        cstTipText.LoadString( ( UINT ) nID);
        cstStatusText.LoadString( ( UINT ) nID);
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
/*afx_msg*/ HBRUSH PageOptionsEnvironment::OnCtlColor( CDC* pDC, CWnd* pWnd, UINT nCtlColor )
{
	BW_GUARD;

	HBRUSH brush = CFormView::OnCtlColor( pDC, pWnd, nCtlColor );

	sunAngleEdit_.SetBoundsColour( pDC, pWnd,
		sunAngleSlider_.getMinRangeLimit(), sunAngleSlider_.getMaxRangeLimit() );
	moonAngleEdit_.SetBoundsColour( pDC, pWnd,
		moonAngleSlider_.getMinRangeLimit(), moonAngleSlider_.getMaxRangeLimit() );

	sunColorREdit_.SetBoundsColour( pDC, pWnd, 0.0, 255.0 );
	sunColorGEdit_.SetBoundsColour( pDC, pWnd, 0.0, 255.0 );
	sunColorBEdit_.SetBoundsColour( pDC, pWnd, 0.0, 255.0 );
	moonColorREdit_.SetBoundsColour( pDC, pWnd, 0.0, 255.0 );
	moonColorGEdit_.SetBoundsColour( pDC, pWnd, 0.0, 255.0 );
	moonColorBEdit_.SetBoundsColour( pDC, pWnd, 0.0, 255.0 );

	mieEdit_.SetBoundsColour( pDC, pWnd,
		mieSlider_.getMinRangeLimit(), mieSlider_.getMaxRangeLimit() );
	turbOffsEdit_.SetBoundsColour( pDC, pWnd,
		turbOffsSlider_.getMinRangeLimit(), turbOffsSlider_.getMaxRangeLimit() );
	turbFactorEdit_.SetBoundsColour( pDC, pWnd,
		turbFactorSlider_.getMinRangeLimit(), turbFactorSlider_.getMaxRangeLimit() );
	vertexHeightEffectEdit_.SetBoundsColour( pDC, pWnd,
		vertexHeightEffectSlider_.getMinRangeLimit(), vertexHeightEffectSlider_.getMaxRangeLimit() );
	sunHeightEffectEdit_.SetBoundsColour( pDC, pWnd,
		sunHeightEffectSlider_.getMinRangeLimit(), sunHeightEffectSlider_.getMaxRangeLimit() );
	powerEdit_.SetBoundsColour( pDC, pWnd,
		powerSlider_.getMinRangeLimit(), powerSlider_.getMaxRangeLimit() );
	return brush;
}


/**
 *	Called when the sun angle edit field loses focus.
 */
/*afx_msg*/ void PageOptionsEnvironment::OnSunAngleEditKillFocus()
{
	BW_GUARD;

    if (filterChange_ == 0)
    {
        ++filterChange_;

        sunAngleEdit_.SetValue( sunAngleSlider_.getValue() );

        --filterChange_;        
    }
}


/**
 *	Called when the moon angle edit field loses focus.
 */
/*afx_msg*/ void PageOptionsEnvironment::OnMoonAngleEditKillFocus()
{
	BW_GUARD;

	if (filterChange_ == 0)
    {
        ++filterChange_;

        moonAngleEdit_.SetValue( moonAngleSlider_.getValue() );

        --filterChange_;        
    }
}


/**
 *	Called when the Mie amount edit field loses focus.
 */
/*afx_msg*/ void PageOptionsEnvironment::OnMieAmountEditKillFocus()
{
	BW_GUARD;

    if (filterChange_ == 0)
    {
        ++filterChange_;

        mieEdit_.SetValue( mieSlider_.getValue() );

        --filterChange_;        
    }
}


/**
 *	Called when the turbulance edit field loses focus.
 */
/*afx_msg*/ void PageOptionsEnvironment::OnTurbOffEditKillFocus()
{
	BW_GUARD;

    if (filterChange_ == 0)
    {
        ++filterChange_;

        turbOffsEdit_.SetValue( turbOffsSlider_.getValue() );

        --filterChange_;        
    }
}


/**
 *	Called when the turbulance factor edit field loses focus.
 */
/*afx_msg*/ void PageOptionsEnvironment::OnTurbFactEditKillFocus()
{
	BW_GUARD;

    if (filterChange_ == 0)
    {
        ++filterChange_;

        turbFactorEdit_.SetValue( turbFactorSlider_.getValue() );

        --filterChange_;        
    }
}


/**
 *	Called when the vertex effect edit field loses focus.
 */
/*afx_msg*/ void PageOptionsEnvironment::OnVertEffEditKillFocus()
{
	BW_GUARD;

    if (filterChange_ == 0)
    {
        ++filterChange_;

        vertexHeightEffectEdit_.SetValue( vertexHeightEffectSlider_.getValue() );

        --filterChange_;        
    }
}


/**
 *	Called when the sun effect edit field loses focus.
 */
/*afx_msg*/ void PageOptionsEnvironment::OnSunEffEditKillFocus()
{
	BW_GUARD;

    if (filterChange_ == 0)
    {
        ++filterChange_;

        sunHeightEffectEdit_.SetValue( sunHeightEffectSlider_.getValue() );

        --filterChange_;        
    }
}


/**
 *	Called when the power edit field loses focus.
 */
/*afx_msg*/ void PageOptionsEnvironment::OnPowerEditKillFocus()
{
	BW_GUARD;

    if (filterChange_ == 0)
    {
        ++filterChange_;

        powerEdit_.SetValue( powerSlider_.getValue() );

        --filterChange_;        
    }
}


void PageOptionsEnvironment::onModeChanged()
{
	BW_GUARD;

    if (colourTimeline_ != NULL)
    {
        colourTimeline_->DestroyWindow();
		bw_safe_delete(colourTimeline_);
    }

    if (colourPicker_ != NULL)
    {
        colourPicker_->DestroyWindow();
		bw_safe_delete(colourPicker_);
    }

    TimeOfDay &tod = getTimeOfDay();
    
    LinearAnimation<Vector3> const *animation = NULL;
    switch (mode_)
    {
    case MODE_SUN:
        animation = &tod.sunAnimation();
        break;
    case MODE_AMB:
        animation = &tod.ambientAnimation();
        break;
    default:
        ASSERT(0);
    }

    if (!animation->empty())
    {
        controls::ColorScheduleItems schedule;
        for
        (
            LinearAnimation<Vector3>::const_iterator it = animation->begin();
            it != animation->end();
            ++it
        )
        {
            Vector3 const &clr = it->second;
            controls::ColorScheduleItem item;
            item.normalisedTime_ = it->first/24.0f;
            item.color_ = 
                Vector4(clr.x/255.0f, clr.y/255.0f, clr.z/255.0f, 1.0f);
            schedule.push_back(item);
        }
        CWnd *timelineFrame = GetDlgItem(IDC_COLORTIMELINE);
        CRect timelineRect;
        timelineFrame->GetWindowRect(&timelineRect);
        ScreenToClient(&timelineRect);
        colourTimeline_ = new controls::ColorTimeline();
        colourTimeline_->Create
        (
            WS_CHILD | WS_VISIBLE, 
            timelineRect, 
            this, 
            schedule,
            false,                          // no alpha
            controls::ColorTimeline::TS_HOURS_MINS,   // time in hrs.mins format
            true                            // wrap
        );
        colourTimeline_->totalScheduleTime(24.0f);
        colourTimeline_->Invalidate();
        colourTimeline_->showLineAtTime(tod.gameTime());
    
        CWnd *pickerFrame = GetDlgItem(IDC_COLORPICKER);
        CRect pickerRect;
        pickerFrame->GetWindowRect(&pickerRect);
        ScreenToClient(&pickerRect);
        colourPicker_ = new controls::ColorPicker();
        colourPicker_->Create(WS_CHILD | WS_VISIBLE, pickerRect, this, false);
        colourPicker_->Invalidate();

        ++filterChange_;

        addClrBtn_.EnableWindow(TRUE);
        delClrBtn_.EnableWindow(TRUE);
        rEdit_.EnableWindow(TRUE);         
        gEdit_.EnableWindow(TRUE);         
        bEdit_.EnableWindow(TRUE); 
        Vector4 colour = colourPicker_->getRGBA();
        rEdit_.SetValue(255.0f*colour.x);
        gEdit_.SetValue(255.0f*colour.y);
        bEdit_.SetValue(255.0f*colour.z);

        --filterChange_;
    }
    else
    {
        ++filterChange_;

        addClrBtn_.EnableWindow(FALSE);
        delClrBtn_.EnableWindow(FALSE);
        rEdit_.EnableWindow(FALSE); 
        rEdit_.SetWindowText(L"");
        gEdit_.EnableWindow(FALSE); 
        gEdit_.SetWindowText(L"");
        bEdit_.EnableWindow(FALSE); 
        bEdit_.SetWindowText(L"");

        --filterChange_;
    }
}


void PageOptionsEnvironment::rebuildAnimation()
{
	BW_GUARD;

    TimeOfDay &tod = getTimeOfDay();
    controls::ColorScheduleItems schedule = colourTimeline_->colourScheduleItems();
    std::sort(schedule.begin(), schedule.end());

    if (colourTimeline_ == NULL)
        return;

    switch (mode_)
    {
    case MODE_SUN:
        {
        tod.clearSunAnimations();
        for 
        (
            controls::ColorScheduleItems::const_iterator it = schedule.begin();
            it != schedule.end();
            ++it
        )
        {
            controls::ColorScheduleItem const &item = *it;
            Vector3 color = 
                Vector3
                (
                    item.color_.x*255.0f,
                    item.color_.y*255.0f,
                    item.color_.z*255.0f
                );
            tod.addSunAnimation
            (
                item.normalisedTime_*24.0f,
                color
            );
        }
        }
        break;
    case MODE_AMB:
        {
        tod.clearAmbientAnimations();
        for 
        (
            controls::ColorScheduleItems::const_iterator it = schedule.begin();
            it != schedule.end();
            ++it
        )
        {
            controls::ColorScheduleItem const &item = *it;
            Vector3 color = 
                Vector3
                (
                    item.color_.x*255.0f,
                    item.color_.y*255.0f,
                    item.color_.z*255.0f
                );
            tod.addAmbientAnimation
            (
                item.normalisedTime_*24.0f,
                color
            );
        }
        }
        break;
    }
    WorldManager::instance().environmentChanged();
}


void PageOptionsEnvironment::saveUndoState(BW::string const &description)
{
	BW_GUARD;

    // Add a new undo/redo state:
    UndoRedo::instance().add(new EnvironmentUndo());
    UndoRedo::instance().barrier(description, false);
}


void PageOptionsEnvironment::timelineChanged()
{
	BW_GUARD;

    rebuildAnimation();
    if (colourTimeline_ != NULL && colourPicker_ != NULL)
    {
        ++filterChange_;

        Vector4 colour = 
            colourTimeline_->getColorScheduleItemSelectedColor();
        colourPicker_->setRGBA(colour);
        rEdit_.SetValue(255.0f*colour.x);
        gEdit_.SetValue(255.0f*colour.y);
        bEdit_.SetValue(255.0f*colour.z);
        controls::ColorScheduleItem *item =
            colourTimeline_->getColorScheduleItemSelected();
        if (item != NULL)
        {
            float gameTime = 
                item->normalisedTime_*colourTimeline_->totalScheduleTime();
            TimeOfDay &tod = getTimeOfDay();
            tod.gameTime(gameTime);
            BW::string gameTimeStr = tod.getTimeOfDayAsString();
            timeOfDayEdit_.SetWindowText(bw_utf8tow(gameTimeStr).c_str());
            colourTimeline_->showLineAtTime(gameTime);
            timeOfDaySlider_.SetPos(BW_ROUND_TO_INT(gameTime*SLIDER_PREC));
        }

		MF_ASSERT( WorldEditorApp::instance().pythonAdapter() != NULL &&
			"PageOptionsEnvironment::timelineChanged: PythonAdapter is NULL" );
		WorldEditorApp::instance().pythonAdapter()->onSliderAdjust(
            "slrProjectCurrentTime", 
            timeOfDaySlider_.GetPos(), 
            timeOfDaySlider_.GetRangeMin(), 
            timeOfDaySlider_.GetRangeMax() );

        Options::setOptionInt( 
            "graphics/timeofday", 
            int( timeOfDaySlider_.GetPos() * WorldManager::TIME_OF_DAY_MULTIPLIER / SLIDER_PREC ) );

        --filterChange_;        
    }
}


void PageOptionsEnvironment::pickerChanged()
{
	BW_GUARD;

    if (colourTimeline_ != NULL && colourPicker_ != NULL)
    {
        ++filterChange_;

        Vector4 colour = colourPicker_->getRGBA();
        colourTimeline_->setColorScheduleItemSelectedColor(colour);
        rEdit_.SetValue(255.0f*colour.x);
        gEdit_.SetValue(255.0f*colour.y);
        bEdit_.SetValue(255.0f*colour.z);


        if (colourTimeline_->itemSelected())
        {
            rebuildAnimation();
        }

        --filterChange_;
    }  
}


void PageOptionsEnvironment::rebuildSkydomeList()
{
	BW_GUARD;

    EnviroMinder &enviroMinder = getEnviroMinder();
	size_t partition = enviroMinder.skyDomesPartition();
    skyDomesList_.ResetContent();
    BW::vector<Moo::VisualPtr> &skyDomes = enviroMinder.skyDomes();
    for( size_t i=0; i < skyDomes.size(); i++ )
    {
        Moo::VisualPtr skyDome = skyDomes[i];
        BW::string file = BWResource::getFilename(skyDome->resourceID()).to_string();
        skyDomesList_.AddString(bw_utf8tow(file).c_str());
    }

	if (partition < skyDomes.size())
	{		
		skyDomesList_.InsertString( static_cast<int>(partition), 
			Localise(L"WORLDEDITOR/GUI/PAGE_OPTIONS_ENVIRONMENT/WEATHER_PARTITION") );
	}
	else
		skyDomesList_.AddString( Localise(L"WORLDEDITOR/GUI/PAGE_OPTIONS_ENVIRONMENT/WEATHER_PARTITION") );
}


void PageOptionsEnvironment::OnBnClickedSunColorButton()
{
	TimeOfDay &tod = getTimeOfDay();
	Vector3 c(tod.sunColor());
	CColorDialog colorDlg( RGB( c.x, c.y, c.z ), CC_FULLOPEN );
	if( colorDlg.DoModal() == IDOK )
	{
		COLORREF col = colorDlg.GetColor();
		float r = (float)(col & 0xff);
		float g = (float)((col / 256) & 0xff);
		float b = (float)((col / 65536) & 0xff);
		sunColorREdit_.SetValue( r );
		sunColorGEdit_.SetValue( g );
		sunColorBEdit_.SetValue( b );

		tod.sunColor(Vector3(r,g,b));
	}
	Invalidate(TRUE);


}

void PageOptionsEnvironment::OnBnClickedMoonColorButton()
{
	TimeOfDay &tod = getTimeOfDay();
	Vector3 c(tod.moonColor());
	CColorDialog colorDlg( RGB( c.x, c.y, c.z ), CC_FULLOPEN );
	if( colorDlg.DoModal() == IDOK )
	{
		COLORREF col = colorDlg.GetColor();
		float r = (float)(col & 0xff);
		float g = (float)((col / 256) & 0xff);
		float b = (float)((col / 65536) & 0xff);
		moonColorREdit_.SetValue( r );
		moonColorGEdit_.SetValue( g );
		moonColorBEdit_.SetValue( b );
		tod.moonColor(Vector3(r,g,b));
	}
	Invalidate(TRUE);

}

void PageOptionsEnvironment::OnEnChangeSuncolorEdit()
{
	TimeOfDay &tod = getTimeOfDay();
	Vector3 col(sunColorREdit_.GetValue(),
		sunColorGEdit_.GetValue(),
		sunColorBEdit_.GetValue());

	tod.sunColor(col);
}


void PageOptionsEnvironment::OnEnChangeMooncolorEdit()
{
	TimeOfDay &tod = getTimeOfDay();
	tod.moonColor(Vector3(	moonColorREdit_.GetValue(),
		moonColorGEdit_.GetValue(),
		moonColorBEdit_.GetValue()));
}


BW_END_NAMESPACE

