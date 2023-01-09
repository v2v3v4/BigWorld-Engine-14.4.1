#pragma once

class CWnd;

BW_BEGIN_NAMESPACE

//////////////////////////////////////////////////////////////////////////
class StatisticsControl
{
	typedef size_t (*GetStatisticsFunc)();

	static UINT s_ctrlID;
	int m_x;
	int m_y;
	wchar_t* m_caption;
	int m_minValue;
	int m_maxValue;
	CWnd* mp_parent;
	GetStatisticsFunc m_getFunc;
	bool mb_reverce;

	CStatic m_captionStatic;
	CStatic m_curValueStatic;
	CStatic m_minValueStatic;
	CStatic m_maxValueStatic;
	CProgressCtrl m_progressCtrl;
public:
	//Params: CWnd*	parent window,
	//		  LPCTSTR caption of control. Will be drawn as static text
	//		  int minV mimimal approved value for statistics function
	//		  int maxV maximal approved value for statistics function
	//		  int x, int y coordinates of the control in the parent window
	//		  GetStatisticsFunc func - statistics function of type size_t f(void)
	StatisticsControl(CWnd* p_parent, LPCTSTR caption, int minV, int maxV, int x, int y, GetStatisticsFunc func, bool reverce = false);
	~StatisticsControl();
	void Init(); // inits control and sets its sizes
	void Update(); // updates control by calling statistics function
	void onSize( int cx, int cy );
};

BW_END_NAMESPACE
