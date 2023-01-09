#ifndef PAGE_DECALS_HPP
#define PAGE_DECALS_HPP

#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "worldeditor/resource.h"
#include "guitabs/guitabs_content.hpp"
#include "controls/auto_tooltip.hpp"
#include "controls/slider.hpp"
#include "controls/edit_numeric.hpp"
#include "afxwin.h"

BW_BEGIN_NAMESPACE

class StatisticsControl;

struct DecalsDebugMessageCallback : public DebugMessageCallback
{
	virtual bool handleMessage(
		DebugMessagePriority messagePriority, const char * pCategory,
		DebugMessageSource messageSource, const LogMetaData & metaData,
		const char * pFormat, va_list argPtr  );
};

struct DecalsCriticalMessageCallback : public CriticalMessageCallback
{
	virtual void handleCritical( const char * msg );
};


class PageDecals : public CFormView, public GUITABS::Content
{
    IMPLEMENT_BASIC_CONTENT
    (
        Localise(L"WORLDEDITOR/GUI/PAGE_DECALS/SHORT_NAME"),				// short name of page
        Localise(L"WORLDEDITOR/GUI/PAGE_DECALS/LONG_NAME"),					// long name of page
        290,                        // default width
        500,                        // default height
        NULL                        // icon
    )

public:
    enum { IDD = IDD_PAGE_DECALS };

    PageDecals();
    ~PageDecals();
protected:
	/*virtual*/ void DoDataExchange(CDataExchange *dx);

    DECLARE_MESSAGE_MAP()

    DECLARE_AUTO_TOOLTIP(PageDecals, CFormView)
public:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnEnChangeOptionsDecalFadeStartEdit();
	afx_msg void OnEnChangeOptionsDecalFadeEndEdit();
	afx_msg HBRUSH OnCtlColor( CDC* pDC, CWnd* pWnd, UINT nCtlColor );
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg LRESULT OnUpdateControls(WPARAM /*wParam*/, LPARAM /*lParam*/);

	void addMessage(const char* message);
	static PageDecals* pInstance();
private:
	inline EnviroMinder& enviroMinder()	{ return WorldManager::instance().enviroMinder(); }
	inline TimeOfDay& timeOfDay() {	return *WorldManager::instance().timeOfDay(); }
	void updateSliderEdits();
	void InitPage();

	StatisticsControl* m_statistics_control;
	controls::Slider decalsFadeStartSlider_;
	controls::EditNumeric decalsFadeStartEdit_;
	controls::Slider decalsFadeEndSlider_;
	controls::EditNumeric decalsFadeEndEdit_;
	CListCtrl msgList_;
	bool ready_;

	BW::list<BW::string> s_messages;

	// handle the debug messages
	static DecalsDebugMessageCallback s_debugMessageCallback_;
	static DecalsCriticalMessageCallback s_criticalMessageCallback_;

	static PageDecals* mp_instance;
	static const int MSG_COL = 0;
};

IMPLEMENT_BASIC_CONTENT_FACTORY(PageDecals)

BW_END_NAMESPACE

#endif
