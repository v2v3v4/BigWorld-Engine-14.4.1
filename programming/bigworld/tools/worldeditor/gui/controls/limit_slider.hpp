#ifndef LIMIT_SLIDER_HPP
#define LIMIT_SLIDER_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "worldeditor/resource.h"
#include "controls/auto_tooltip.hpp"
#include "controls/range_slider_ctrl.hpp"
#include "controls/edit_numeric.hpp"
#include <afxwin.h>
#include <afxcmn.h>

BW_BEGIN_NAMESPACE

class LimitSliderDlg : public CDialog
{
	DECLARE_DYNAMIC(LimitSliderDlg)

public:
	LimitSliderDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~LimitSliderDlg();

// Dialog Data
	enum { IDD = IDD_LIMITSLIDER };

	static void show( float minLimit, float maxLimit, float min, float max,
		unsigned int digits, LimitSlider* limitSlider );

protected:
	DECLARE_AUTO_TOOLTIP(LimitSliderDlg,CDialog);
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	CString formatValue( float value );
	CRect getBestRect( CSize size, CPoint p );
	void internalShow( float minLimit, float maxLimit, float min, float max,
		unsigned int digits, LimitSlider* limitSlider );

	DECLARE_MESSAGE_MAP()
	afx_msg void OnActivate( UINT, CWnd*, BOOL );
	afx_msg void OnEnKillFocusMinEdit();
	afx_msg void OnEnKillFocusMaxEdit();
	afx_msg void OnEnChangeMinEdit();
	afx_msg void OnEnChangeMaxEdit();
	afx_msg HBRUSH OnCtlColor( CDC* pDC, CWnd* pWnd, UINT nCtlColor );
	LRESULT OnRangeSliderChanged( WPARAM , LPARAM );
	LRESULT OnRangeSliderTrack( WPARAM , LPARAM );
public:
	controls::EditNumeric mMin;
	controls::EditNumeric mMax;
	controls::EditNumeric mMinLimit;
	controls::EditNumeric mMaxLimit;
    controls::RangeSliderCtrl mSlider;

	bool mChanging;
	unsigned int mDigits;
	LimitSlider* mLimitSlider;
};

// LimitSlider

class LimitSlider : public CSliderCtrl
{
	DECLARE_DYNAMIC(LimitSlider)

public:
	LimitSlider();
	virtual ~LimitSlider();
	float getValue() const;
	void setValue( float value );
	void setDigits( unsigned int digits );
	void setRange( float min, float max );
	void setRangeLimit( float min, float max );
	void setPageSteps( unsigned int value );
	float getMinRange() const;
	float getMaxRange() const;
	float getMinRangeLimit() const;
	float getMaxRangeLimit() const;
	unsigned int getPageSteps() const;
	void beginEdit();
protected:
	void recalcPageSize();
	virtual void PreSubclassWindow();
	unsigned int mDigits;
	float mMin;
	float mMax;
	float mMinLimit;
	float mMaxLimit;
	unsigned int mPageSteps;
	DECLARE_MESSAGE_MAP()
	afx_msg void OnRButtonDown( UINT nFlags, CPoint point );
};

BW_END_NAMESPACE

#endif // LIMIT_SLIDER_HPP
