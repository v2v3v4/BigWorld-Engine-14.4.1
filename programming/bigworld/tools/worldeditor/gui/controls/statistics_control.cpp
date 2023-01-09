#include "pch.hpp"
#include "statistics_control.hpp"
#include "tools/common/utilities.hpp"

BW_BEGIN_NAMESPACE

UINT StatisticsControl::s_ctrlID = 22000;

StatisticsControl::StatisticsControl(CWnd* p_parent, LPCTSTR caption, int minV, int maxV, int x, int y, GetStatisticsFunc func, bool reverce):
mp_parent(p_parent),
m_minValue(minV),
m_maxValue(maxV),
m_x(x),
m_y(y),
m_getFunc(func),
mb_reverce(reverce)
{
	MF_ASSERT(caption && p_parent)
	m_caption = bw_wcsdup(caption);

}

#define TEXT_HEIGHT 15

void StatisticsControl::Init()
{
	RECT rect;
	mp_parent->GetWindowRect(&rect);
	CFont* pFont = mp_parent->GetFont();
	m_captionStatic.Create(	m_caption, 
		WS_CHILD | WS_VISIBLE, 
		CRect(	m_x, 															
		m_y, 
		m_x + 120, 
		m_y + TEXT_HEIGHT), 
		mp_parent, s_ctrlID++);
	m_captionStatic.SetFont(pFont);

	TCHAR buf[20];

	_stprintf_s(buf, 20, _T("%") _T(PRIzu), m_getFunc());
	m_curValueStatic.Create(buf, 
		WS_CHILD | WS_VISIBLE, 
		CRect(	m_x + 122, 															
		m_y, 
		m_x + rect.right - rect.left, 
		m_y + TEXT_HEIGHT), 
		mp_parent, s_ctrlID++);
	m_curValueStatic.SetFont(pFont);

	_stprintf_s(buf, 20, _T("%d"), m_minValue);
	m_minValueStatic.Create(buf, 
		WS_CHILD | WS_VISIBLE, 
		CRect(	m_x, 
		m_y + 25, 
		m_x + 30, 
		m_y + 25 + TEXT_HEIGHT), 
		mp_parent, s_ctrlID++);
	m_minValueStatic.SetFont(pFont);

	_stprintf_s(buf, 20, _T("%d"), m_maxValue);
	m_maxValueStatic.Create(buf, 
		WS_CHILD | WS_VISIBLE, 
		CRect(	m_x + rect.right - rect.left - 57, 
		m_y + 25, 
		m_x + rect.right - rect.left - 15, 
		m_y + 25 + TEXT_HEIGHT), 
		mp_parent, s_ctrlID++);
	m_maxValueStatic.SetFont(pFont);

	m_progressCtrl.Create(	WS_CHILD | WS_VISIBLE | PBS_SMOOTH,  
		CRect(	m_x + 32, 
		m_y + 25, 
		m_x + rect.right - rect.left - 59, 
		m_y + 25 + TEXT_HEIGHT), 
		mp_parent, s_ctrlID++);

	SetWindowTheme (m_progressCtrl.GetSafeHwnd(), TEXT (" "), TEXT (" "));
	m_progressCtrl.SetRange(0, 100);
	m_progressCtrl.SetPos(0);
	m_progressCtrl.SetStep((m_maxValue-m_minValue)/100);
}
#undef TEXT_HEIGHT

void StatisticsControl::Update()
{
	size_t val = m_getFunc();
	float value = (float)(val - m_minValue) / (float)(m_maxValue - m_minValue);
	m_progressCtrl.SetPos((int)(value * 100.0f));

	TCHAR buf[20];
	_stprintf_s(buf, 20, _T("%") _T(PRIzu), val);
	m_curValueStatic.SetWindowText(buf);

	if(mb_reverce)
		value = 1.0f - value;

	//works only with disabled visual styles
	COLORREF clr = ((value > 1.0f || value < 0.0f) ? RGB( 255, 0, 0 ) : RGB( min( 220, (int)(220.0f * value)), 220, 0 ));
	m_progressCtrl.SetBarColor( clr );	
	m_progressCtrl.SetBkColor( RGB(100,100,100) );	

}


void StatisticsControl::onSize( int cx, int cy )
{
	RECT rect;
	mp_parent->GetWindowRect(&rect);

	Utilities::stretchToRight( mp_parent, m_curValueStatic, cx, 0 );
	Utilities::moveToRight( mp_parent, m_maxValueStatic, cx, 15 );
	Utilities::stretchToRight( mp_parent, m_progressCtrl, cx, 59 );

	m_curValueStatic.RedrawWindow();
	m_maxValueStatic.RedrawWindow();
	m_progressCtrl.RedrawWindow();
}


StatisticsControl::~StatisticsControl()
{
	bw_free( m_caption );
}

BW_END_NAMESPACE
