#include "pch.hpp"
#include "worldeditor/gui/pages/page_decals.hpp"
#include "cstdmf/debug_filter.hpp"
#include "moo/deferred_decals_manager.hpp"
#include "moo/renderer.hpp"
#include "worldeditor/gui/controls/statistics_control.hpp"
#include "appmgr/options.hpp"
#include "common/user_messages.hpp"
#include "common/utilities.hpp"
#include <afxcmn.h>


DECLARE_DEBUG_COMPONENT2("WorldEditor", 0)


BW_BEGIN_NAMESPACE


//////////////////////////////////////////////////////////////////////////


BEGIN_MESSAGE_MAP(PageDecals, CFormView)

    ON_MESSAGE  (WM_UPDATE_CONTROLS    , OnUpdateControls  )
    ON_WM_CTLCOLOR()
    ON_WM_CREATE()
    ON_WM_HSCROLL()
    ON_WM_SIZE()
    ON_EN_CHANGE(IDC_OPTIONS_DECAL_FADE_START_EDIT, &PageDecals::OnEnChangeOptionsDecalFadeStartEdit)
    ON_EN_CHANGE(IDC_OPTIONS_DECAL_FADE_END_EDIT, &PageDecals::OnEnChangeOptionsDecalFadeEndEdit)
END_MESSAGE_MAP()


LRESULT PageDecals::OnUpdateControls( WPARAM /*wParam*/, LPARAM /*lParam*/ )
{
    static bool s_initialized = false;
    if(!s_initialized)
    {
        InitPage();
        s_initialized = true;
    }

    m_statistics_control->Update();

    if(Renderer::instance().pipelineType() == IRendererPipeline::TYPE_DEFERRED_SHADING)
    {
        Vector2 v = DecalsManager::instance().getFadeRange();
        if(decalsFadeStartEdit_.GetIntegerValue() != (int)(v.x))
            decalsFadeStartEdit_.SetIntegerValue((int)v.x);
        if(decalsFadeEndEdit_.GetIntegerValue() != (int)(v.y))
            decalsFadeEndEdit_.SetIntegerValue( (int)v.y );
    }

    return 0;
}

PageDecals::~PageDecals()
{
    DebugFilter::instance().deleteMessageCallback( &s_debugMessageCallback_ );
    DebugFilter::instance().deleteCriticalCallback( &s_criticalMessageCallback_ );

    delete m_statistics_control;
    mp_instance = NULL;
}

const BW::wstring PageDecals::contentID = L"PageDecals";


int PageDecals::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (__super::OnCreate(lpCreateStruct) == -1)
        return -1;

    m_statistics_control = new StatisticsControl(this, _T("Decal count"), 0, Options::getOptionInt( "statistics/decalCount", 200 ), 10, 10, &DecalsManager::numVisibleDecals);

    return 0;
}

void PageDecals::OnSize(UINT nType, int cx, int cy)
{
    if (ready_ == false)
    {
        return;
    }
    __super::OnSize(nType, cx, cy);

    Utilities::stretchToRight( this, msgList_, cx, 5 );
    Utilities::stretchToRight(this, decalsFadeStartSlider_, cx, 65);
    Utilities::moveToRight(this, decalsFadeStartEdit_, cx, 5);
    Utilities::stretchToRight(this, decalsFadeEndSlider_, cx, 65);
    Utilities::moveToRight(this, decalsFadeEndEdit_, cx, 5);
    m_statistics_control->onSize( cx, cy );

    CRect r;
    msgList_.GetWindowRect(r);
    msgList_.SetColumnWidth( MSG_COL, r.Width() - 10 );
    msgList_.RedrawWindow();
    decalsFadeEndEdit_.RedrawWindow();
    decalsFadeStartEdit_.RedrawWindow();
    decalsFadeEndSlider_.RedrawWindow();
    decalsFadeStartSlider_.RedrawWindow();
}

void PageDecals::DoDataExchange( CDataExchange *pDX )
{
    if (ready_ == false)
    {
        m_statistics_control->Init();
    }

    DDX_Control(pDX, IDC_OPTIONS_DECAL_FADE_START_SLIDER, decalsFadeStartSlider_);
    DDX_Control(pDX, IDC_OPTIONS_DECAL_FADE_START_EDIT, decalsFadeStartEdit_);

    DDX_Control(pDX, IDC_OPTIONS_DECAL_FADE_END_SLIDER, decalsFadeEndSlider_);
    DDX_Control(pDX, IDC_OPTIONS_DECAL_FADE_END_EDIT, decalsFadeEndEdit_);
    DDX_Control( pDX, IDC_OPTIONS_DECAL_MESSAGES_LIST, msgList_ );
    msgList_.SetExtendedStyle( LVS_EX_FULLROWSELECT );
    msgList_.InsertColumn( MSG_COL, Localise(L"COMMON/PAGE_MESSAGES/MESSAGE"), LVCFMT_LEFT, 128 );

    ready_ = true;
}

PageDecals::PageDecals()
    : CFormView(IDD)
    , m_statistics_control(NULL)
    , ready_( false )
{
    decalsFadeStartEdit_.SetNumericType(controls::EditNumeric::ENT_INTEGER);
    decalsFadeEndEdit_.SetNumericType(controls::EditNumeric::ENT_INTEGER);

    DebugFilter::instance().addMessageCallback( &s_debugMessageCallback_ );
    DebugFilter::instance().addCriticalCallback( &s_criticalMessageCallback_ );

    mp_instance = this;
}

HBRUSH PageDecals::OnCtlColor( CDC* pDC, CWnd* pWnd, UINT nCtlColor )
{
    HBRUSH brush = CFormView::OnCtlColor( pDC, pWnd, nCtlColor );

    decalsFadeStartEdit_.SetBoundsColour( pDC, pWnd,
        decalsFadeStartEdit_.GetMinimum(), decalsFadeStartEdit_.GetMaximum() );
    decalsFadeEndEdit_.SetBoundsColour( pDC, pWnd,
        decalsFadeEndEdit_.GetMinimum(), decalsFadeEndEdit_.GetMaximum() );

    return brush;
}

void PageDecals::InitPage()
{
    INIT_AUTO_TOOLTIP();

    Vector2 fadeRange(0, 0);
    if (Renderer::instance().pipelineType() == IRendererPipeline::TYPE_DEFERRED_SHADING)
    {
        fadeRange = DecalsManager::instance().getFadeRange();
    }

    decalsFadeStartSlider_.SetRangeMin(50);
    decalsFadeStartSlider_.SetRangeMax(500);
    decalsFadeStartSlider_.SetPageSize(10);
    decalsFadeStartSlider_.SetPos((int)fadeRange.x);
    decalsFadeStartEdit_.SetNumericType( controls::EditNumeric::ENT_INTEGER );
    decalsFadeStartEdit_.SetAllowNegative( false );
    decalsFadeStartEdit_.SetMinimum( 50 );
    decalsFadeStartEdit_.SetMaximum( 500 );
    decalsFadeEndSlider_.SetRangeMin(60);
    decalsFadeEndSlider_.SetRangeMax(550);
    decalsFadeEndSlider_.SetPageSize(10);
    decalsFadeEndSlider_.SetPos((int)fadeRange.y);
    decalsFadeEndEdit_.SetNumericType( controls::EditNumeric::ENT_INTEGER );
    decalsFadeEndEdit_.SetAllowNegative( false );
    decalsFadeEndEdit_.SetMinimum( 60 );
    decalsFadeEndEdit_.SetMaximum( 550 );

    updateSliderEdits();
}


void PageDecals::updateSliderEdits()
{
    BW_GUARD;

    int pos = decalsFadeStartSlider_.GetPos();
    if(pos != decalsFadeStartEdit_.GetIntegerValue())
        decalsFadeStartEdit_.SetIntegerValue( pos );
    pos = decalsFadeEndSlider_.GetPos();
    if(pos != decalsFadeEndEdit_.GetIntegerValue())
        decalsFadeEndEdit_.SetIntegerValue( pos );
}

void PageDecals::OnHScroll( UINT nSBCode, UINT nPos, CScrollBar* pScrollBar )
{
    BW_GUARD;

    updateSliderEdits();
}

void PageDecals::OnEnChangeOptionsDecalFadeStartEdit()
{
    if (Renderer::instance().pipelineType() == IRendererPipeline::TYPE_DEFERRED_SHADING)
    {
        Vector2 v = DecalsManager::instance().getFadeRange();
        int fadeStart = decalsFadeStartEdit_.GetIntegerValue();
        if(fadeStart != decalsFadeStartSlider_.GetPos())
            decalsFadeStartSlider_.SetPos( fadeStart );

        DecalsManager::instance().setFadeRange((float)fadeStart, (float)v.y);
    }
}

void PageDecals::OnEnChangeOptionsDecalFadeEndEdit()
{
    if (Renderer::instance().pipelineType() == IRendererPipeline::TYPE_DEFERRED_SHADING)
    {
        Vector2 v = DecalsManager::instance().getFadeRange();
        int fadeEnd = decalsFadeEndEdit_.GetIntegerValue();
        if(fadeEnd != decalsFadeEndSlider_.GetPos())
            decalsFadeEndSlider_.SetPos( fadeEnd );

        DecalsManager::instance().setFadeRange((float)v.x, (float)fadeEnd);
    }
}

void PageDecals::addMessage( const char* message )
{
    if(message)
    {
        BW::wstring buf;
        bw_utf8tow(message, buf);
        int idx = msgList_.GetItemCount();
        msgList_.InsertItem(idx, L"");
        msgList_.SetItemText(idx, MSG_COL, buf.c_str());
        msgList_.UpdateWindow();
    }
}

PageDecals* PageDecals::pInstance()
{
    return mp_instance;
}

DecalsDebugMessageCallback PageDecals::s_debugMessageCallback_;

DecalsCriticalMessageCallback PageDecals::s_criticalMessageCallback_;

PageDecals* PageDecals::mp_instance = NULL;

//////////////////////////////////////////////////////////////////////////

bool DecalsDebugMessageCallback::handleMessage(
       DebugMessagePriority /*messagePriority*/,
       const char * /*pCategory*/,
       DebugMessageSource /*messageSource*/, const LogMetaData & /*metaData*/,
       const char * pFormat, va_list argPtr )
{
    static char buf[8192];
    _vsnprintf( buf, ARRAY_SIZE(buf), pFormat, argPtr );
    buf[sizeof(buf)-1]=0;

    if (buf[ strlen(buf) - 1 ] == '\n')
    {
        buf[ strlen(buf) - 1 ] = '\0';
    }

    if (strstr(buf, "[DECAL]") == buf && PageDecals::pInstance())
    {
        PageDecals::pInstance()->addMessage(buf);
    }

    return false;
}

void DecalsCriticalMessageCallback::handleCritical( const char * msg )
{
    static char buf[8192];
    sprintf_s( buf, ARRAY_SIZE(buf), "%s", msg );
    buf[sizeof(buf)-1]=0;
    size_t length = strlen(buf);
    if (buf[ length - 1 ] == '\n')
        buf[ length - 1 ] = '\0';

    if(strstr(buf, "[DECAL]") == buf && PageDecals::pInstance())
        PageDecals::pInstance()->addMessage(buf);
}

BW_END_NAMESPACE
