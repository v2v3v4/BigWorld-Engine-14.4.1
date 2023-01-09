#include "pch.hpp"
#include "worldeditor/gui/pages/page_options_hdr_lighting.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "worldeditor/framework/world_editor_app.hpp"
#include "worldeditor/scripting/we_python_adapter.hpp"
#include "controls/user_messages.hpp"
#include "appmgr/app.hpp"
#include "appmgr/options.hpp"
#include "gizmo/undoredo.hpp"
#include "moo/renderer.hpp"
#include "resmgr/xml_section.hpp"
#include "resmgr/auto_config.hpp"
#include "romp/enviro_minder.hpp"
#include "romp/sky_gradient_dome.hpp"
#include "romp/time_of_day.hpp"
#include "romp/weather.hpp"
#include "romp/hdr_support.hpp"
#include "romp/ssao_support.hpp"
#include "romp/hdr_settings.hpp"
#include "moo/visual_manager.hpp"
#include "terrain/terrain_settings.hpp"
#include "common/format.hpp"
#include "common/string_utils.hpp"
#include "common/user_messages.hpp"
#include <afxpriv.h>


DECLARE_DEBUG_COMPONENT2("WorldEditor", 0)

BW_BEGIN_NAMESPACE

BEGIN_MESSAGE_MAP(PageOptionsHDRLighting, CFormView)
    ON_WM_HSCROLL()
    ON_WM_CTLCOLOR()
    ON_WM_PAINT()

    //-- SSAO settings
    ON_BN_CLICKED( IDC_PAGE_ENV_SSAO_ENABLE                         , OnSSAOEnableBnClicked )
    ON_BN_CLICKED( IDC_PAGE_ENV_SSAO_SHOW_BUFFER                    , OnSSAOShowBufferBnClicked )

    ON_EN_CHANGE( IDC_PAGE_ENV_SSAO_FADE_EDIT                       , OnSSAOFadeEdit )
    ON_EN_CHANGE( IDC_PAGE_ENV_SSAO_AMPLIFY_EDIT                    , OnSSAOAmplifyEdit )
    ON_EN_CHANGE( IDC_PAGE_ENV_SSAO_RADIUS_EDIT                     , OnSSAORadiusEdit )

    ON_EN_KILLFOCUS( IDC_PAGE_ENV_SSAO_FADE_EDIT                    , OnKillFocusSSAOFade )
    ON_EN_KILLFOCUS( IDC_PAGE_ENV_SSAO_AMPLIFY_EDIT                 , OnKillFocusSSAOAmplify )
    ON_EN_KILLFOCUS( IDC_PAGE_ENV_SSAO_RADIUS_EDIT                  , OnKillFocusSSAORadius )

    //-- SSAO: infuence factors.
    ON_EN_CHANGE( IDC_SSAO_TERRAIN_AMBIENT_FACTOR_EDIT              , OnSSAOFactorTerrainAmbientEdit )
    ON_EN_CHANGE( IDC_SSAO_TERRAIN_LIGHTING_FACTOR_EDIT             , OnSSAOFactorTerrainLightingEdit )
    ON_EN_CHANGE( IDC_SSAO_SPEEDTREE_AMBIENT_FACTOR_EDIT            , OnSSAOFactorSpeedTreeAmbientEdit )
    ON_EN_CHANGE( IDC_SSAO_SPEEDTREE_LIGHTING_FACTOR_EDIT           , OnSSAOFactorSpeedTreeLightingEdit )
    ON_EN_CHANGE( IDC_SSAO_OBJECTS_AMBIENT_FACTOR_EDIT              , OnSSAOFactorObjectsAmbientEdit )
    ON_EN_CHANGE( IDC_SSAO_OBJECTS_LIGHTING_FACTOR_EDIT             , OnSSAOFactorObjectsLightingEdit )

    ON_EN_KILLFOCUS( IDC_SSAO_TERRAIN_AMBIENT_FACTOR_EDIT           , OnKillFocusSSAOFactorTerrainAmbient )
    ON_EN_KILLFOCUS( IDC_SSAO_TERRAIN_LIGHTING_FACTOR_EDIT          , OnKillFocusSSAOFactorTerrainLighting )
    ON_EN_KILLFOCUS( IDC_SSAO_SPEEDTREE_AMBIENT_FACTOR_EDIT         , OnKillFocusSSAOFactorSpeedTreeAmbient )
    ON_EN_KILLFOCUS( IDC_SSAO_SPEEDTREE_LIGHTING_FACTOR_EDIT        , OnKillFocusSSAOFactorSpeedTreeLighting )
    ON_EN_KILLFOCUS( IDC_SSAO_OBJECTS_AMBIENT_FACTOR_EDIT           , OnKillFocusSSAOFactorObjectsAmbient )
    ON_EN_KILLFOCUS( IDC_SSAO_OBJECTS_LIGHTING_FACTOR_EDIT          , OnKillFocusSSAOFactorObjectsLighting )

    //-- HDR: common settings
    ON_BN_CLICKED( IDC_PAGE_ENV_HDR_ENABLE                          , OnHDREnableBnClicked )
    ON_BN_CLICKED( IDC_PAGE_ENV_USE_LINEAR_SPACE_LIGHTING           , OnHDREnableLinearSpaceLightingBnClicked )
    ON_EN_CHANGE( IDC_PAGE_ENV_ADAPT_SPEED_EDIT                     , OnHDRAdaptationSpeedEdit )
    ON_EN_CHANGE( IDC_PAGE_ENV_HDR_AVG_LUM_UPDATE_INTERVAL_EDIT     , OnHDRAvgLumUpdateIntervalEdit )

    ON_EN_KILLFOCUS( IDC_PAGE_ENV_ADAPT_SPEED_EDIT                  , OnKillFocusHDRAdaptSpeed )
    ON_EN_KILLFOCUS( IDC_PAGE_ENV_HDR_AVG_LUM_UPDATE_INTERVAL_EDIT  , OnKillFocusHDRAvgLumUpdateInterval )

    //-- HDR: bloom settings.
    ON_BN_CLICKED( IDC_PAGE_ENV_HDR_BLOOM_ENABLE                    , OnHDRBloomEnableBnClicked )
    ON_EN_CHANGE( IDC_PAGE_ENV_BLOOM_BRIGHT_THRESHOLD_EDIT          , OnHDRBloomBrightThresholdEdit )
    ON_EN_CHANGE( IDC_PAGE_ENV_BLOOM_BRIGHT_OFFSET_EDIT             , OnHDRBloomBrightOffsetEdit )
    ON_EN_CHANGE( IDC_PAGE_ENV_BLOOM_AMOUNT_EDIT                    , OnHDRBloomAmountEdit )

    ON_EN_KILLFOCUS( IDC_PAGE_ENV_BLOOM_BRIGHT_THRESHOLD_EDIT       , OnKillFocusHDRBloomBrightThreshold )
    ON_EN_KILLFOCUS( IDC_PAGE_ENV_BLOOM_BRIGHT_OFFSET_EDIT          , OnKillFocusHDRBloomBrightOffset )
    ON_EN_KILLFOCUS( IDC_PAGE_ENV_BLOOM_AMOUNT_EDIT                 , OnKillFocusHDRBloomAmount )

    //-- HDR: tone mapping settings.
    ON_EN_CHANGE( IDC_PAGE_ENV_EYE_LIGHT_LIMIT_EDIT                 , OnHDRToneMappingEyeLightLimitEdit )
    ON_EN_CHANGE( IDC_PAGE_ENV_EYE_DARK_LIMIT_EDIT                  , OnHDRToneMappingEyeDarkLimitEdit )
    ON_EN_CHANGE( IDC_PAGE_ENV_MIDDLE_GRAY_EDIT                     , OnHDRToneMappingMiddleGrayEdit )
    ON_EN_CHANGE( IDC_PAGE_ENV_WHITE_POINT_EDIT                     , OnHDRToneMappingWhitePointEdit )

    ON_EN_KILLFOCUS( IDC_PAGE_ENV_EYE_LIGHT_LIMIT_EDIT              , OnKillFocusHDRToneMappingEyeLightLimit )
    ON_EN_KILLFOCUS( IDC_PAGE_ENV_EYE_DARK_LIMIT_EDIT               , OnKillFocusHDRToneMappingEyeDarkLimit )
    ON_EN_KILLFOCUS( IDC_PAGE_ENV_MIDDLE_GRAY_EDIT                  , OnKillFocusHDRToneMappingMiddleGray )
    ON_EN_KILLFOCUS( IDC_PAGE_ENV_WHITE_POINT_EDIT                  , OnKillFocusHDRToneMappingWhitePoint )

    //-- HDR: environment settings.
    ON_EN_CHANGE( IDC_PAGE_ENV_ENV_SKY_LUM_MUL_EDIT                 , OnHDREnvSkyLumMultiplierEdit )
    ON_EN_CHANGE( IDC_PAGE_ENV_ENV_SUN_LUM_MUL_EDIT                 , OnHDREnvSunLumMultiplierEdit )
    ON_EN_CHANGE( IDC_PAGE_ENV_ENV_AMBIENT_LUM_MUL_EDIT             , OnHDREnvAmbientLumMultiplierEdit )
    ON_EN_CHANGE( IDC_PAGE_ENV_FOG_LUM_MUL_EDIT                     , OnHDREnvFogLumMultiplierEdit )

    ON_EN_KILLFOCUS( IDC_PAGE_ENV_ENV_SKY_LUM_MUL_EDIT              , OnKillFocusHDREnvSkyLumMultiplier )
    ON_EN_KILLFOCUS( IDC_PAGE_ENV_ENV_SUN_LUM_MUL_EDIT              , OnKillFocusHDREnvSunLumMultiplier )
    ON_EN_KILLFOCUS( IDC_PAGE_ENV_ENV_AMBIENT_LUM_MUL_EDIT          , OnKillFocusHDREnvAmbientLumMultiplier )
    ON_EN_KILLFOCUS( IDC_PAGE_ENV_FOG_LUM_MUL_EDIT                  , OnKillFocusHDREnvFogLumMultiplier )

    ON_MESSAGE( WM_UPDATE_CONTROLS                                  , OnUpdateControls  )
    ON_MESSAGE( WM_NEW_SPACE                                        , OnNewSpace        )

    ON_NOTIFY_EX_RANGE( TTN_NEEDTEXTW, 0, 0xFFFF                    , OnToolTipText)
    ON_NOTIFY_EX_RANGE( TTN_NEEDTEXTA, 0, 0xFFFF                    , OnToolTipText)
END_MESSAGE_MAP()


const BW::wstring PageOptionsHDRLighting::contentID = L"PageOptionsHDRLighting";


/*static*/ PageOptionsHDRLighting *PageOptionsHDRLighting::s_instance_ = NULL;


namespace
{   
    const float INTERVAL_BETWEEN_FREQUENT_UNDOS = 300; // interval in milliseconds between adjacent Undo/Redo operations

    // This class is for environment undo/redo operations.
    class HDRLightingUndo : public UndoRedo::Operation
    {
    public:
        HDRLightingUndo();

        /*virtual*/ void undo();

        /*virtual*/ bool iseq(UndoRedo::Operation const &other) const;

    private:
        XMLSectionPtr       ds_;
    };

    HDRLightingUndo::HDRLightingUndo():
        UndoRedo::Operation((size_t)(typeid(HDRLightingUndo).name())),
        ds_(new XMLSection("environmentUndoRedo"))
    {
        BW_GUARD;

        PageOptionsHDRLighting *poe = PageOptionsHDRLighting::instance();

        EnviroMinder &em = PageOptionsHDRLighting::getEnviroMinder();
        em.save(ds_, false); // save everything internally
    }


    /*virtual*/ void HDRLightingUndo::undo()
    {
        BW_GUARD;

        // Save the current state to the undo/redo stack:
        UndoRedo::instance().add(new HDRLightingUndo());

        // Do the actual undo:
        EnviroMinder &em = PageOptionsHDRLighting::getEnviroMinder();
        em.load(ds_, false); // load everything internally

        //-- force update HDR rendering support.
        HDRSettings hdrCfg = em.hdrSettings();
        em.hdrSettings(hdrCfg);

        //-- force update SSAO rendering support.
        SSAOSettings ssaoCfg = em.ssaoSettings();
        em.ssaoSettings(ssaoCfg);

        // Update the PageOptionsHDRLighting page:
        PageOptionsHDRLighting *poe = PageOptionsHDRLighting::instance();
        if (poe != NULL)
            poe->reinitialise();
    }

    /*virtual*/ 
    bool 
    HDRLightingUndo::iseq
    (
        UndoRedo::Operation         const &other
    ) const
    {
        return false;
    }
}

struct PageOptionsHDRLighting::Impl
{
    struct EditSlider
    {
        EditSlider( controls::EditNumeric& edit, LimitSlider& slider )
            : edit_( edit )
            , slider_( slider )
        {}

        controls::EditNumeric& edit_;
        LimitSlider& slider_;
    };

    Impl( PageOptionsHDRLighting* parent )
        : parent_( parent )
    {
        addPairedControl(
            parent->ssaoFadeEdit_, parent->ssaoFadeSlider_ );
        addPairedControl(
            parent->ssaoAmplifyEdit_, parent->ssaoAmplifySlider_ );
        addPairedControl(
            parent->ssaoRadiusEdit_, parent->ssaoRadiusSlider_ );
        addPairedControl(
            parent->ssaoFactorSpeedTreeAmbientEdit_,
            parent->ssaoFactorSpeedTreeAmbientSlider_ );
        addPairedControl(
            parent->ssaoFactorSpeedTreeLightingEdit_,
            parent->ssaoFactorSpeedTreeLightingSlider_ );
        addPairedControl(
            parent->ssaoFactorTerrainAmbientEdit_,
            parent->ssaoFactorTerrainAmbientSlider_ );
        addPairedControl(
            parent->ssaoFactorTerrainLightingEdit_,
            parent->ssaoFactorTerrainLightingSlider_ );
        addPairedControl(
            parent->ssaoFactorObjectsAmbientEdit_,
            parent->ssaoFactorObjectsAmbientSlider_ );
        addPairedControl(
            parent->ssaoFactorObjectsLightingEdit_,
            parent->ssaoFactorObjectsLightingSlider_ );
        addPairedControl(
            parent->hdrAdaptSpeedEdit_, parent->hdrAdaptSpeedSlider_ );
        addPairedControl(
            parent->hdrAvgLumUpdateIntervalEdit_,
            parent->hdrAvgLumUpdateIntervalSlider_ );
        addPairedControl(
            parent->hdrBloomBrightThresholdEdit_,
            parent->hdrBloomBrightThresholdSlider_ );
        addPairedControl(
            parent->hdrBloomBrightOffsetEdit_,
            parent->hdrBloomBrightOffsetSlider_ );
        addPairedControl(
            parent->hdrBloomAmountEdit_, parent->hdrBloomAmountSlider_ );
        addPairedControl(
            parent->hdrToneMappingEyeDarkLimitEdit_,
            parent->hdrToneMappingEyeDarkLimitSlider_ );
        addPairedControl(
            parent->hdrToneMappingEyeLightLimitEdit_,
            parent->hdrToneMappingEyeLightLimitSlider_ );
        addPairedControl(
            parent->hdrToneMappingMiddleGrayEdit_,
            parent->hdrToneMappingMiddleGraySlider_ );
        addPairedControl(
            parent->hdrToneMappingWhitePointEdit_,
            parent->hdrToneMappingWhitePointSlider_ );
        addPairedControl(
            parent->hdrEnvSkyLumMultiplierEdit_,
            parent->hdrEnvSkyLumMultiplierSlider_ );
        addPairedControl(
            parent->hdrEnvSunLumMultiplierEdit_,
            parent->hdrEnvSunLumMultiplierSlider_ );
        addPairedControl(
            parent->hdrEnvAmbientLumMultiplierEdit_,
            parent->hdrEnvAmbientLumMultiplierSlider_ );
        addPairedControl(
            parent->hdrEnvFogLumMultiplierEdit_,
            parent->hdrEnvFogLumMultiplierSlider_ );
    }

    void addPairedControl( controls::EditNumeric& edit, LimitSlider& slider )
    {
        pairedControls_.push_back( EditSlider( edit, slider ) );
    }

    void updateControlColours( CDC* pDC, CWnd* pWnd )
    {
        for (EditSlider& editSlider: pairedControls_)
        {
            editSlider.edit_.SetBoundsColour( pDC, pWnd,
                editSlider.slider_.getMinRangeLimit(),
                editSlider.slider_.getMaxRangeLimit() );
        }
    }

    PageOptionsHDRLighting* parent_;
    BW::vector<EditSlider> pairedControls_;
};

bool PageOptionsHDRLighting::wasSlider( CWnd const & testScrollBar,
    CWnd const & scrollBar )
{
    BW_GUARD;

    return 
        testScrollBar.GetSafeHwnd() == scrollBar.GetSafeHwnd()
        &&
        ::IsWindow(scrollBar.GetSafeHwnd()) != FALSE;
}


PageOptionsHDRLighting::PageOptionsHDRLighting() :
    CFormView(IDD),
    initialised_(false),
    filterChange_(0),
    sliding_(false),
    nextSaveUndoTime_(-1),
    nextEnvironmentUndoOp_(NULL)
{
    s_instance_ = this;
    pImpl = new Impl( this );
}


/*virtual*/ PageOptionsHDRLighting::~PageOptionsHDRLighting()
{
    BW_GUARD;

    s_instance_ = NULL;
    bw_safe_delete( pImpl );
}


/*static*/ PageOptionsHDRLighting *PageOptionsHDRLighting::instance()
{
    return s_instance_;
}


void PageOptionsHDRLighting::reinitialise()
{
    BW_GUARD;

    ++filterChange_;

    CDataExchange ddx(this, FALSE);
    DoDataExchange(&ddx);

    EnviroMinder        &enviroMinder   = getEnviroMinder();
    const HDRSettings   &hdrCfg         = enviroMinder.hdrSettings();
    const SSAOSettings  &ssaoCfg        = enviroMinder.ssaoSettings();

    //SSAO settings
    {
        ssaoEnableBtn_.SetCheck(ssaoCfg.m_enabled ? BST_CHECKED : BST_UNCHECKED);
        
        if(SSAOSupport* pSSAO = Renderer::instance().pipeline()->ssaoSupport())
            ssaoShowBufferBtn_.SetCheck(pSSAO->showBuffer() ? BST_CHECKED : BST_UNCHECKED);
        else
            ssaoShowBufferBtn_.SetCheck(BST_UNCHECKED);

        //--
#undef  BW_SSAO_INIT_PROPERTY
#define BW_SSAO_INIT_PROPERTY(controlName, propertyName, a, b)      \
        controlName##Slider_.setDigits( 2 );                        \
        controlName##Slider_.setRangeLimit( a, b );                 \
        controlName##Slider_.setRange( a, b );                      \
        controlName##Slider_.setValue( ssaoCfg.m_##propertyName );  \
        controlName##Edit_.SetNumDecimals( 2 );                     \
        controlName##Edit_.SetValue( ssaoCfg.m_##propertyName );

        BW_SSAO_INIT_PROPERTY(ssaoFade,     fade,       1.0f,  10.0f)
        BW_SSAO_INIT_PROPERTY(ssaoAmplify,  amplify,    0.1f,   2.0f)
        BW_SSAO_INIT_PROPERTY(ssaoRadius,   radius,     0.005f, 0.1f)

        BW_SSAO_INIT_PROPERTY(ssaoFactorSpeedTreeAmbient,   influences.m_speedtree.x,   0.0f, 1.0f)
        BW_SSAO_INIT_PROPERTY(ssaoFactorSpeedTreeLighting,  influences.m_speedtree.y,   0.0f, 1.0f)
        BW_SSAO_INIT_PROPERTY(ssaoFactorTerrainAmbient,     influences.m_terrain.x,     0.0f, 1.0f)
        BW_SSAO_INIT_PROPERTY(ssaoFactorTerrainLighting,    influences.m_terrain.y,     0.0f, 1.0f)
        BW_SSAO_INIT_PROPERTY(ssaoFactorObjectsAmbient,     influences.m_objects.x,     0.0f, 1.0f)
        BW_SSAO_INIT_PROPERTY(ssaoFactorObjectsLighting,    influences.m_objects.y,     0.0f, 1.0f)
#undef BW_SSAO_INIT_PROPERTY

    }
    //-- HDR settings
    {
        //--
#undef  BW_HDR_INIT_PROPERTY
#define BW_HDR_INIT_PROPERTY(controlName, propertyName, a, b)       \
        controlName##Slider_.setDigits( 2 );                        \
        controlName##Slider_.setRangeLimit( a, b );                 \
        controlName##Slider_.setRange( a, b );                      \
        controlName##Slider_.setValue( hdrCfg.m_##propertyName );   \
        controlName##Edit_.SetNumDecimals( 2 );                     \
        controlName##Edit_.SetValue( hdrCfg.m_##propertyName );

        //-- common.
        {
            hdrEnableBtn_.SetCheck(hdrCfg.m_enabled ? BST_CHECKED : BST_UNCHECKED);
            hdrUseLinearSpaceLightingBtn_.SetCheck(hdrCfg.m_gammaCorrection.m_enabled ? BST_CHECKED : BST_UNCHECKED);

            BW_HDR_INIT_PROPERTY(hdrAdaptSpeed,                 adaptationSpeed,            1.0f, 200.0f)
            BW_HDR_INIT_PROPERTY(hdrAvgLumUpdateInterval,       avgLumUpdateInterval,       0.1f, 2.0f)
        }

        //-- bloom.
        {
            //-- enable/disable.
            hdrBloomEnableBtn_.SetCheck(hdrCfg.m_bloom.m_enabled ? BST_CHECKED : BST_UNCHECKED);

            BW_HDR_INIT_PROPERTY(hdrBloomBrightThreshold,       bloom.m_brightThreshold,    0.05f, 5.0f)
            BW_HDR_INIT_PROPERTY(hdrBloomBrightOffset,          bloom.m_brightOffset,       0.05f, 5.0f)
            BW_HDR_INIT_PROPERTY(hdrBloomAmount,                bloom.m_factor,             0.0f, 5.0f)
        }

        //-- tone mapping.
        {
            BW_HDR_INIT_PROPERTY(hdrToneMappingEyeDarkLimit,    tonemapping.m_eyeDarkLimit,     0.0f,   1.0f)
            BW_HDR_INIT_PROPERTY(hdrToneMappingEyeLightLimit,   tonemapping.m_eyeLightLimit,    0.0f,   2.5f)
            BW_HDR_INIT_PROPERTY(hdrToneMappingMiddleGray,      tonemapping.m_middleGray,       0.05f,  1.0f)
            BW_HDR_INIT_PROPERTY(hdrToneMappingWhitePoint,      tonemapping.m_whitePoint,       0.25f, 10.0f)
        }

        //-- environment.
        {
            BW_HDR_INIT_PROPERTY(hdrEnvSkyLumMultiplier,        environment.m_skyLumMultiplier,     0.25f, 30.0f)
            BW_HDR_INIT_PROPERTY(hdrEnvSunLumMultiplier,        environment.m_sunLumMultiplier,     0.25f, 30.0f)
            BW_HDR_INIT_PROPERTY(hdrEnvAmbientLumMultiplier,    environment.m_ambientLumMultiplier, 0.25f, 10.0f)
            BW_HDR_INIT_PROPERTY(hdrEnvFogLumMultiplier,        environment.m_fogLumMultiplier,     0.25f, 10.0f)
        }

#undef BW_HDR_INIT_PROPERTY

    }

    --filterChange_;
}


/*static*/ EnviroMinder &PageOptionsHDRLighting::getEnviroMinder()
{
    BW_GUARD;

    return WorldManager::instance().enviroMinder();
}



void PageOptionsHDRLighting::InitPage()
{
    BW_GUARD;

    ++filterChange_;

    reinitialise();

    if (!initialised_)
        INIT_AUTO_TOOLTIP();

    --filterChange_;

    initialised_ = true;
}


/*afx_msg*/ 
LRESULT 
PageOptionsHDRLighting::OnUpdateControls
(
    WPARAM      /*wParam*/, 
    LPARAM      /*lParam*/
)
{
    BW_GUARD;

    if (!initialised_)
        InitPage();

    periodicSaveUndoState();

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
PageOptionsHDRLighting::OnNewSpace
(
    WPARAM      /*wParam*/, 
    LPARAM      /*lParam*/
)
{
    BW_GUARD;

    InitPage(); // reinitialise due to the new space
    return 0;
}



/*virtual*/ void PageOptionsHDRLighting::DoDataExchange(CDataExchange *dx)
{
    CFormView::DoDataExchange(dx);

    //-- SSAO settings
    DDX_Control(dx, IDC_PAGE_ENV_SSAO_ENABLE                            , ssaoEnableBtn_           );
    DDX_Control(dx, IDC_PAGE_ENV_SSAO_SHOW_BUFFER                       , ssaoShowBufferBtn_       );
    DDX_Control(dx, IDC_PAGE_ENV_SSAO_FADE_EDIT                         , ssaoFadeEdit_            );
    DDX_Control(dx, IDC_PAGE_ENV_SSAO_FADE_SLIDER                       , ssaoFadeSlider_          );
    DDX_Control(dx, IDC_PAGE_ENV_SSAO_AMPLIFY_EDIT                      , ssaoAmplifyEdit_         );
    DDX_Control(dx, IDC_PAGE_ENV_SSAO_AMPLIFY_SLIDER                    , ssaoAmplifySlider_       );
    DDX_Control(dx, IDC_PAGE_ENV_SSAO_RADIUS_EDIT                       , ssaoRadiusEdit_          );
    DDX_Control(dx, IDC_PAGE_ENV_SSAO_RADIUS_SLIDER                     , ssaoRadiusSlider_        );

    //-- SSAO: infuence factors.
    DDX_Control(dx, IDC_SSAO_SPEEDTREE_AMBIENT_FACTOR_EDIT              , ssaoFactorSpeedTreeAmbientEdit_       );
    DDX_Control(dx, IDC_SSAO_SPEEDTREE_AMBIENT_FACTOR_SLIDER            , ssaoFactorSpeedTreeAmbientSlider_     );
    DDX_Control(dx, IDC_SSAO_SPEEDTREE_LIGHTING_FACTOR_EDIT             , ssaoFactorSpeedTreeLightingEdit_      );
    DDX_Control(dx, IDC_SSAO_SPEEDTREE_LIGHTING_FACTOR_SLIDER           , ssaoFactorSpeedTreeLightingSlider_    );
    DDX_Control(dx, IDC_SSAO_TERRAIN_AMBIENT_FACTOR_EDIT                , ssaoFactorTerrainAmbientEdit_         );
    DDX_Control(dx, IDC_SSAO_TERRAIN_AMBIENT_FACTOR_SLIDER              , ssaoFactorTerrainAmbientSlider_       );
    DDX_Control(dx, IDC_SSAO_TERRAIN_LIGHTING_FACTOR_EDIT               , ssaoFactorTerrainLightingEdit_        );
    DDX_Control(dx, IDC_SSAO_TERRAIN_LIGHTING_FACTOR_SLIDER             , ssaoFactorTerrainLightingSlider_      );
    DDX_Control(dx, IDC_SSAO_OBJECTS_AMBIENT_FACTOR_EDIT                , ssaoFactorObjectsAmbientEdit_         );
    DDX_Control(dx, IDC_SSAO_OBJECTS_AMBIENT_FACTOR_SLIDER              , ssaoFactorObjectsAmbientSlider_       );
    DDX_Control(dx, IDC_SSAO_OBJECTS_LIGHTING_FACTOR_EDIT               , ssaoFactorObjectsLightingEdit_        );
    DDX_Control(dx, IDC_SSAO_OBJECTS_LIGHTING_FACTOR_SLIDER             , ssaoFactorObjectsLightingSlider_      );

    //-- HDR: common.
    DDX_Control(dx, IDC_PAGE_ENV_HDR_ENABLE                             , hdrEnableBtn_            );
    DDX_Control(dx, IDC_PAGE_ENV_USE_LINEAR_SPACE_LIGHTING              , hdrUseLinearSpaceLightingBtn_     );
    DDX_Control(dx, IDC_PAGE_ENV_ADAPT_SPEED_EDIT                       , hdrAdaptSpeedEdit_       );
    DDX_Control(dx, IDC_PAGE_ENV_ADAPT_SPEED_SLIDER                     , hdrAdaptSpeedSlider_     );
    DDX_Control(dx, IDC_PAGE_ENV_HDR_AVG_LUM_UPDATE_INTERVAL_EDIT       , hdrAvgLumUpdateIntervalEdit_     );
    DDX_Control(dx, IDC_PAGE_ENV_HDR_AVG_LUM_UPDATE_INTERVAL_SLIDER     , hdrAvgLumUpdateIntervalSlider_       );

    //-- HDR: bloom.
    DDX_Control(dx, IDC_PAGE_ENV_HDR_BLOOM_ENABLE                       , hdrBloomEnableBtn_        );
    DDX_Control(dx, IDC_PAGE_ENV_BLOOM_BRIGHT_THRESHOLD_EDIT            , hdrBloomBrightThresholdEdit_  );
    DDX_Control(dx, IDC_PAGE_ENV_BLOOM_BRIGHT_THRESHOLD_SLIDER          , hdrBloomBrightThresholdSlider_  );
    DDX_Control(dx, IDC_PAGE_ENV_BLOOM_BRIGHT_OFFSET_EDIT               , hdrBloomBrightOffsetEdit_  );
    DDX_Control(dx, IDC_PAGE_ENV_BLOOM_BRIGHT_OFFSET_SLIDER             , hdrBloomBrightOffsetSlider_  );
    DDX_Control(dx, IDC_PAGE_ENV_BLOOM_AMOUNT_EDIT                      , hdrBloomAmountEdit_  );
    DDX_Control(dx, IDC_PAGE_ENV_BLOOM_AMOUNT_SLIDER                    , hdrBloomAmountSlider_  );

    //-- HDR: tone mapping.
    DDX_Control(dx, IDC_PAGE_ENV_EYE_LIGHT_LIMIT_EDIT                   , hdrToneMappingEyeLightLimitEdit_  );
    DDX_Control(dx, IDC_PAGE_ENV_EYE_LIGHT_LIMIT_SLIDER                 , hdrToneMappingEyeLightLimitSlider_  );
    DDX_Control(dx, IDC_PAGE_ENV_EYE_DARK_LIMIT_EDIT                    , hdrToneMappingEyeDarkLimitEdit_  );
    DDX_Control(dx, IDC_PAGE_ENV_EYE_DARK_LIMIT_SLIDER                  , hdrToneMappingEyeDarkLimitSlider_  );
    DDX_Control(dx, IDC_PAGE_ENV_MIDDLE_GRAY_EDIT                       , hdrToneMappingMiddleGrayEdit_  );
    DDX_Control(dx, IDC_PAGE_ENV_MIDDLE_GRAY_SLIDER                     , hdrToneMappingMiddleGraySlider_  );
    DDX_Control(dx, IDC_PAGE_ENV_WHITE_POINT_EDIT                       , hdrToneMappingWhitePointEdit_  );
    DDX_Control(dx, IDC_PAGE_ENV_WHITE_POINT_SLIDER                     , hdrToneMappingWhitePointSlider_  );

    //-- HDR: environment.
    DDX_Control(dx, IDC_PAGE_ENV_ENV_SKY_LUM_MUL_EDIT                   , hdrEnvSkyLumMultiplierEdit_  );
    DDX_Control(dx, IDC_PAGE_ENV_ENV_SKY_LUM_MUL_SLIDER                 , hdrEnvSkyLumMultiplierSlider_  );
    DDX_Control(dx, IDC_PAGE_ENV_ENV_SUN_LUM_MUL_EDIT                   , hdrEnvSunLumMultiplierEdit_  );
    DDX_Control(dx, IDC_PAGE_ENV_ENV_SUN_LUM_MUL_SLIDER                 , hdrEnvSunLumMultiplierSlider_  );
    DDX_Control(dx, IDC_PAGE_ENV_ENV_AMBIENT_LUM_MUL_EDIT               , hdrEnvAmbientLumMultiplierEdit_  );
    DDX_Control(dx, IDC_PAGE_ENV_ENV_AMBIENT_LUM_MUL_SLIDER             , hdrEnvAmbientLumMultiplierSlider_  );
    DDX_Control(dx, IDC_PAGE_ENV_FOG_LUM_MUL_EDIT                       , hdrEnvFogLumMultiplierEdit_  );
    DDX_Control(dx, IDC_PAGE_ENV_FOG_LUM_MUL_SLIDER                     , hdrEnvFogLumMultiplierSlider_  );
}

/*afx_msg*/ 
void 
PageOptionsHDRLighting::OnHScroll
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

    if (wasSlider(*wnd, ssaoAmplifySlider_))
    {
        OnSSAOAmplifySlider(sms);
    }
    else if (wasSlider(*wnd, ssaoFadeSlider_))
    {
        OnSSAOFadeSlider(sms);
    }
    else if (wasSlider(*wnd, ssaoRadiusSlider_))
    {
        OnSSAORadiusSlider(sms);
    }
    else if (wasSlider(*wnd, ssaoFactorSpeedTreeAmbientSlider_))
    {
        OnSSAOFactorSpeedTreeAmbientSlider(sms);
    }
    else if (wasSlider(*wnd, ssaoFactorSpeedTreeLightingSlider_))
    {
        OnSSAOFactorSpeedTreeLightingSlider(sms);
    }
    else if (wasSlider(*wnd, ssaoFactorTerrainAmbientSlider_))
    {
        OnSSAOFactorTerrainAmbientSlider(sms);
    }
    else if (wasSlider(*wnd, ssaoFactorTerrainLightingSlider_))
    {
        OnSSAOFactorTerrainLightingSlider(sms);
    }
    else if (wasSlider(*wnd, ssaoFactorObjectsAmbientSlider_))
    {
        OnSSAOFactorObjectsAmbientSlider(sms);
    }
    else if (wasSlider(*wnd, ssaoFactorObjectsLightingSlider_))
    {
        OnSSAOFactorObjectsLightingSlider(sms);
    }
    else if (wasSlider(*wnd, hdrAdaptSpeedSlider_))
    {
        OnHDRAdaptationSpeedSlider(sms);
    }
    else if (wasSlider(*wnd, hdrAvgLumUpdateIntervalSlider_))
    {
        OnHDRAvgLumUpdateIntervalSlider(sms);
    }
    else if (wasSlider(*wnd, hdrBloomBrightThresholdSlider_))
    {
        OnHDRBloomBrightThresholdSlider(sms);
    }
    else if (wasSlider(*wnd, hdrBloomBrightOffsetSlider_))
    {
        OnHDRBloomBrightOffsetSlider(sms);
    }
    else if (wasSlider(*wnd, hdrBloomAmountSlider_))
    {
        OnHDRBloomAmountSlider(sms);
    }
    else if (wasSlider(*wnd, hdrToneMappingEyeDarkLimitSlider_))
    {
        OnHDRToneMappingEyeDarkLimitSlider(sms);
    }
    else if (wasSlider(*wnd, hdrToneMappingEyeLightLimitSlider_))
    {
        OnHDRToneMappingEyeLightLimitSlider(sms);
    }
    else if (wasSlider(*wnd, hdrToneMappingMiddleGraySlider_))
    {
        OnHDRToneMappingMiddleGraySlider(sms);
    }
    else if (wasSlider(*wnd, hdrToneMappingWhitePointSlider_))
    {
        OnHDRToneMappingWhitePointSlider(sms);
    }
    else if (wasSlider(*wnd, hdrEnvSkyLumMultiplierSlider_))
    {
        OnHDREnvSkyLumMultiplierSlider(sms);
    }
    else if (wasSlider(*wnd, hdrEnvSunLumMultiplierSlider_))
    {
        OnHDREnvSunLumMultiplierSlider(sms);
    }
    else if (wasSlider(*wnd, hdrEnvAmbientLumMultiplierSlider_))
    {
        OnHDREnvAmbientLumMultiplierSlider(sms);
    }
    else if (wasSlider(*wnd, hdrEnvFogLumMultiplierSlider_))
    {
        OnHDREnvFogLumMultiplierSlider(sms);
    }
}

//--------------------------------------------------------------------------------------------------
#define BW_GENERATE_SSAO_ACCESSOR_DEFINITION(propertyName, controlName, funcName, message)                                          \
    void PageOptionsHDRLighting::funcName##Edit()                                                                                   \
    {                                                                                                                               \
        BW_GUARD;                                                                                                                   \
        SSAOSettings ssaoCfg = getEnviroMinder().ssaoSettings();                                                                    \
        onSSAOEditGeneric(ssaoCfg, ssaoCfg.m_##propertyName, controlName##Edit_, controlName##Slider_, "Change SSAO" ## message);   \
    }                                                                                                                               \
                                                                                                                                    \
    void PageOptionsHDRLighting::funcName##Slider(SliderMovementState)                                                              \
    {                                                                                                                               \
        BW_GUARD;                                                                                                                   \
        SSAOSettings ssaoCfg = getEnviroMinder().ssaoSettings();                                                                    \
        onSSAOSliderGeneric(ssaoCfg, ssaoCfg.m_##propertyName, controlName##Edit_, controlName##Slider_, "Change SSAO" ## message); \
    }

BW_GENERATE_SSAO_ACCESSOR_DEFINITION(fade,                          ssaoFade,                           OnSSAOFade,                         "fade.")
BW_GENERATE_SSAO_ACCESSOR_DEFINITION(amplify,                       ssaoAmplify,                        OnSSAOAmplify,                      "amplify.")
BW_GENERATE_SSAO_ACCESSOR_DEFINITION(radius,                        ssaoRadius,                         OnSSAORadius,                       "radius.")

BW_GENERATE_SSAO_ACCESSOR_DEFINITION(influences.m_speedtree.x,      ssaoFactorSpeedTreeAmbient,         OnSSAOFactorSpeedTreeAmbient,       "speedtree ambient factor.")
BW_GENERATE_SSAO_ACCESSOR_DEFINITION(influences.m_speedtree.y,      ssaoFactorSpeedTreeLighting,        OnSSAOFactorSpeedTreeLighting,      "speedtree lighting factor.")
BW_GENERATE_SSAO_ACCESSOR_DEFINITION(influences.m_terrain.x,        ssaoFactorTerrainAmbient,           OnSSAOFactorTerrainAmbient,         "terrain ambient factor.")
BW_GENERATE_SSAO_ACCESSOR_DEFINITION(influences.m_terrain.y,        ssaoFactorTerrainLighting,          OnSSAOFactorTerrainLighting,        "terrain lighting factor.")
BW_GENERATE_SSAO_ACCESSOR_DEFINITION(influences.m_objects.x,        ssaoFactorObjectsAmbient,           OnSSAOFactorObjectsAmbient,         "objects ambient factor.")
BW_GENERATE_SSAO_ACCESSOR_DEFINITION(influences.m_objects.y,        ssaoFactorObjectsLighting,          OnSSAOFactorObjectsLighting,        "objects lighting factor.")

//--------------------------------------------------------------------------------------------------
/*afx_msg*/ void PageOptionsHDRLighting::OnSSAOEnableBnClicked()
{
    BW_GUARD;

    if (filterChange_ == 0)
    {
        ++filterChange_;

        SSAOSettings ssaoCfg = getEnviroMinder().ssaoSettings();

        saveUndoState("Enable/disable SSAO.");
        ssaoCfg.m_enabled = !ssaoCfg.m_enabled;
        getEnviroMinder().ssaoSettings(ssaoCfg);
        WorldManager::instance().environmentChanged();

        --filterChange_;        
    }
}

//--------------------------------------------------------------------------------------------------
/*afx_msg*/ void PageOptionsHDRLighting::OnSSAOShowBufferBnClicked()
{
    if(SSAOSupport* pSSAO = Renderer::instance().pipeline()->ssaoSupport())
        pSSAO->showBuffer(!pSSAO->showBuffer());
}

#undef BW_GENERATE_SSAO_ACCESSORS

//--------------------------------------------------------------------------------------------------
#undef  BW_GENERATE_HDR_ACCESSOR_DEFINITION
#define BW_GENERATE_HDR_ACCESSOR_DEFINITION(propertyName, controlName, funcName, message)                                                   \
    void PageOptionsHDRLighting::funcName##Edit()                                                                                           \
    {                                                                                                                                       \
        BW_GUARD;                                                                                                                           \
        HDRSettings hdrCfg = getEnviroMinder().hdrSettings();                                                                               \
        onHDREditGeneric(hdrCfg, hdrCfg.m_##propertyName, controlName##Edit_, controlName##Slider_, "Change HDR Rendering" ## message);     \
    }                                                                                                                                       \
                                                                                                                                            \
    void PageOptionsHDRLighting::funcName##Slider(SliderMovementState)                                                                      \
    {                                                                                                                                       \
        BW_GUARD;                                                                                                                           \
        HDRSettings hdrCfg = getEnviroMinder().hdrSettings();                                                                               \
        onHDRSliderGeneric(hdrCfg, hdrCfg.m_##propertyName, controlName##Edit_, controlName##Slider_, "Change HDR Rendering" ## message);   \
    }

BW_GENERATE_HDR_ACCESSOR_DEFINITION(adaptationSpeed,                    hdrAdaptSpeed,                  OnHDRAdaptationSpeed,           "adaptation speed.");
BW_GENERATE_HDR_ACCESSOR_DEFINITION(avgLumUpdateInterval,               hdrAvgLumUpdateInterval,        OnHDRAvgLumUpdateInterval,      "average luminance update interval.");

BW_GENERATE_HDR_ACCESSOR_DEFINITION(bloom.m_brightThreshold,            hdrBloomBrightThreshold,        OnHDRBloomBrightThreshold,      "bloom bright threshold.");
BW_GENERATE_HDR_ACCESSOR_DEFINITION(bloom.m_brightOffset,               hdrBloomBrightOffset,           OnHDRBloomBrightOffset,         "bloom bright offset.");
BW_GENERATE_HDR_ACCESSOR_DEFINITION(bloom.m_factor,                     hdrBloomAmount,                 OnHDRBloomAmount,               "bloom amount.");

BW_GENERATE_HDR_ACCESSOR_DEFINITION(tonemapping.m_eyeDarkLimit,         hdrToneMappingEyeDarkLimit,     OnHDRToneMappingEyeDarkLimit,   "tone mapping eye dark limit.");
BW_GENERATE_HDR_ACCESSOR_DEFINITION(tonemapping.m_eyeLightLimit,        hdrToneMappingEyeLightLimit,    OnHDRToneMappingEyeLightLimit,  "tone mapping eye light limit.");
BW_GENERATE_HDR_ACCESSOR_DEFINITION(tonemapping.m_middleGray,           hdrToneMappingMiddleGray,       OnHDRToneMappingMiddleGray,     "tone mapping middle gray point.");
BW_GENERATE_HDR_ACCESSOR_DEFINITION(tonemapping.m_whitePoint,           hdrToneMappingWhitePoint,       OnHDRToneMappingWhitePoint,     "tone mapping white point.");

BW_GENERATE_HDR_ACCESSOR_DEFINITION(environment.m_skyLumMultiplier,     hdrEnvSkyLumMultiplier,         OnHDREnvSkyLumMultiplier,       "environment sky luminance multiplier.");
BW_GENERATE_HDR_ACCESSOR_DEFINITION(environment.m_sunLumMultiplier,     hdrEnvSunLumMultiplier,         OnHDREnvSunLumMultiplier,       "environment sun luminance multiplier.");
BW_GENERATE_HDR_ACCESSOR_DEFINITION(environment.m_ambientLumMultiplier, hdrEnvAmbientLumMultiplier,     OnHDREnvAmbientLumMultiplier,   "environment ambient luminance multiplier.");
BW_GENERATE_HDR_ACCESSOR_DEFINITION(environment.m_fogLumMultiplier,     hdrEnvFogLumMultiplier,         OnHDREnvFogLumMultiplier,       "environment fog luminance multiplier.");

#undef BW_GENERATE_HDR_ACCESSORS

#undef GENERATE_EDIT_KILLFOCUS_DEFINITION
#define GENERATE_EDIT_KILLFOCUS_DEFINITION( name, variable )                \
void PageOptionsHDRLighting::OnKillFocus##name()                            \
{                                                                           \
    if (filterChange_ == 0)                                                 \
    {                                                                       \
        ++filterChange_;                                                    \
        variable##Edit_.SetValue( variable##Slider_.getValue() );           \
        --filterChange_;                                                    \
    }                                                                       \
}

    GENERATE_EDIT_KILLFOCUS_DEFINITION( HDRAdaptSpeed, hdrAdaptSpeed )
    GENERATE_EDIT_KILLFOCUS_DEFINITION( HDRAvgLumUpdateInterval, hdrAvgLumUpdateInterval )
    GENERATE_EDIT_KILLFOCUS_DEFINITION( HDRBloomBrightThreshold, hdrBloomBrightThreshold )
    GENERATE_EDIT_KILLFOCUS_DEFINITION( HDRBloomBrightOffset, hdrBloomBrightOffset )
    GENERATE_EDIT_KILLFOCUS_DEFINITION( HDRBloomAmount, hdrBloomAmount )
    GENERATE_EDIT_KILLFOCUS_DEFINITION( HDRToneMappingEyeDarkLimit, hdrToneMappingEyeDarkLimit )
    GENERATE_EDIT_KILLFOCUS_DEFINITION( HDRToneMappingEyeLightLimit, hdrToneMappingEyeLightLimit )
    GENERATE_EDIT_KILLFOCUS_DEFINITION( HDRToneMappingMiddleGray, hdrToneMappingMiddleGray )
    GENERATE_EDIT_KILLFOCUS_DEFINITION( HDRToneMappingWhitePoint, hdrToneMappingWhitePoint )
    GENERATE_EDIT_KILLFOCUS_DEFINITION( HDREnvSkyLumMultiplier, hdrEnvSkyLumMultiplier )
    GENERATE_EDIT_KILLFOCUS_DEFINITION( HDREnvSunLumMultiplier, hdrEnvSunLumMultiplier )
    GENERATE_EDIT_KILLFOCUS_DEFINITION( HDREnvAmbientLumMultiplier, hdrEnvAmbientLumMultiplier )
    GENERATE_EDIT_KILLFOCUS_DEFINITION( HDREnvFogLumMultiplier, hdrEnvFogLumMultiplier )
    GENERATE_EDIT_KILLFOCUS_DEFINITION( SSAOFade, ssaoFade )
    GENERATE_EDIT_KILLFOCUS_DEFINITION( SSAOAmplify, ssaoAmplify )
    GENERATE_EDIT_KILLFOCUS_DEFINITION( SSAORadius, ssaoRadius )
    GENERATE_EDIT_KILLFOCUS_DEFINITION( SSAOFactorSpeedTreeAmbient, ssaoFactorSpeedTreeAmbient )
    GENERATE_EDIT_KILLFOCUS_DEFINITION( SSAOFactorSpeedTreeLighting, ssaoFactorSpeedTreeLighting )
    GENERATE_EDIT_KILLFOCUS_DEFINITION( SSAOFactorTerrainAmbient, ssaoFactorTerrainAmbient )
    GENERATE_EDIT_KILLFOCUS_DEFINITION( SSAOFactorTerrainLighting, ssaoFactorTerrainLighting )
    GENERATE_EDIT_KILLFOCUS_DEFINITION( SSAOFactorObjectsAmbient, ssaoFactorObjectsAmbient )
    GENERATE_EDIT_KILLFOCUS_DEFINITION( SSAOFactorObjectsLighting, ssaoFactorObjectsLighting )

#undef GENERATE_EDIT_KILLFOCUS_DEFINITION


//--------------------------------------------------------------------------------------------------
/*afx_msg*/ void PageOptionsHDRLighting::OnHDRBloomEnableBnClicked()
{
    if (filterChange_ == 0)
    {
        ++filterChange_;

        HDRSettings hdrCfg = getEnviroMinder().hdrSettings();

        saveUndoState("Enable/disable HDR Rendering bloom effect.");
        hdrCfg.m_bloom.m_enabled = !hdrCfg.m_bloom.m_enabled;
        getEnviroMinder().hdrSettings(hdrCfg);
        WorldManager::instance().environmentChanged();

        --filterChange_;        
    }
}

//--------------------------------------------------------------------------------------------------
/*afx_msg*/ void PageOptionsHDRLighting::OnHDREnableLinearSpaceLightingBnClicked()
{
    if (filterChange_ == 0)
    {
        ++filterChange_;

        HDRSettings hdrCfg = getEnviroMinder().hdrSettings();

        saveUndoState("Enable/disable linear space lighting.");
        hdrCfg.m_gammaCorrection.m_enabled = !hdrCfg.m_gammaCorrection.m_enabled;
        getEnviroMinder().hdrSettings(hdrCfg);
        WorldManager::instance().environmentChanged();

        --filterChange_;        
    }
}

//--------------------------------------------------------------------------------------------------
/*afx_msg*/ void PageOptionsHDRLighting::OnHDREnableBnClicked()
{
    BW_GUARD;

    if (filterChange_ == 0)
    {
        ++filterChange_;

        HDRSettings hdrCfg = getEnviroMinder().hdrSettings();

        saveUndoState("Enable/disable HDR Rendering.");
        hdrCfg.m_enabled = !hdrCfg.m_enabled;
        getEnviroMinder().hdrSettings(hdrCfg);
        WorldManager::instance().environmentChanged();

        --filterChange_;        
    }
}

BOOL PageOptionsHDRLighting::OnToolTipText(UINT, NMHDR* pNMHDR, LRESULT *result)
{
    BW_GUARD;

    // Allow top level routing frame to handle the message
    if (GetRoutingFrame() != NULL)
        return FALSE;

    TOOLTIPTEXTW *pTTTW = (TOOLTIPTEXTW*)pNMHDR;
    CString cstTipText;
    CString cstStatusText;

    UINT_PTR nID = (UINT_PTR)pNMHDR->idFrom;
    if ( pNMHDR->code == TTN_NEEDTEXTW && (pTTTW->uFlags & TTF_IDISHWND) )
    {
        // idFrom is actually the HWND of the tool
        nID = ((UINT_PTR)::GetDlgCtrlID( (HWND)nID ));
    }

    if (nID != 0) // will be zero on a separator
    {
        MF_ASSERT( nID < std::numeric_limits<UINT>::max() );
        cstTipText.LoadString( (UINT)nID );
        cstStatusText.LoadString( (UINT)nID );
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

void PageOptionsHDRLighting::OnPaint()
{
    BW_GUARD;
    CFormView::OnPaint();

    static CPen s_penHDR(PS_SOLID, 2, RGB(255, 128, 128));
    static CPen s_penSSAO(PS_SOLID, 2, RGB(128, 255, 128));

    CClientDC dc(this);
    CRect rect;
    
    GetDlgItem(IDC_GROUPBOX_HDR)->GetWindowRect(rect);
    ScreenToClient(rect);
    rect.InflateRect(2, 2, 2, 2);
    CPen *pPrevPen = (CPen*)dc.SelectObject(&s_penHDR);
    CBrush *pPrevPrush = (CBrush*)dc.SelectStockObject(NULL_BRUSH);
    dc.RoundRect(rect, CPoint(2, 2));

    GetDlgItem(IDC_GROUPBOX_SSAO)->GetWindowRect(rect);
    ScreenToClient(rect);
    rect.InflateRect(2, 2, 2, 2);
    dc.SelectObject(&s_penSSAO);
    dc.RoundRect(rect, CPoint(2, 2));

    dc.SelectObject(pPrevPen);
    dc.SelectObject(pPrevPrush);

}

HBRUSH PageOptionsHDRLighting::OnCtlColor( CDC* pDC, CWnd* pWnd, UINT nCtlColor )
{
    HBRUSH brush = CFormView::OnCtlColor( pDC, pWnd, nCtlColor );
    pImpl->updateControlColours( pDC, pWnd );
    return brush;
}

// Add a new undo/redo state:
void PageOptionsHDRLighting::saveUndoState(BW::string const &description)
{
    BW_GUARD;

    // defer undo towards later moment of time, until large interval between undos
    nextSaveUndoTime_ = timestamp() + uint64(INTERVAL_BETWEEN_FREQUENT_UNDOS * (double)stampsPerSecond() / 1000.0);
    nextSaveUndoMsg_ = description;

    if (nextEnvironmentUndoOp_ == NULL)
        nextEnvironmentUndoOp_ = new HDRLightingUndo();
}

void PageOptionsHDRLighting::periodicSaveUndoState()
{
    if (nextSaveUndoTime_ == -1
        || nextSaveUndoTime_ > timestamp())
        return;

    if (nextEnvironmentUndoOp_ != NULL)
    {
        UndoRedo::instance().add(nextEnvironmentUndoOp_);
        UndoRedo::instance().barrier(nextSaveUndoMsg_, false);
        nextEnvironmentUndoOp_ = NULL;
    }

    nextSaveUndoTime_ = -1;
}

BW_END_NAMESPACE