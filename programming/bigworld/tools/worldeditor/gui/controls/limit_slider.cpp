#include "pch.hpp"
#include "worldeditor/resource.h"
#include "worldeditor/gui/controls/limit_slider.hpp"
#include "controls/user_messages.hpp"
#include "controls/utils.hpp"

BW_BEGIN_NAMESPACE

// LimitSliderDlg dialog

IMPLEMENT_DYNAMIC(LimitSliderDlg, CDialog)
LimitSliderDlg::LimitSliderDlg(CWnd* pParent /*=NULL*/) :
	CDialog(LimitSliderDlg::IDD, pParent), mChanging( false ), mLimitSlider( 0 )
{
}

LimitSliderDlg::~LimitSliderDlg()
{
}

BOOL LimitSliderDlg::OnInitDialog()
{
	BW_GUARD;

	BOOL ret = CDialog::OnInitDialog();
	INIT_AUTO_TOOLTIP();
	return ret;
}

void LimitSliderDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_MINEDIT, mMin);
	DDX_Control(pDX, IDC_MAXEDIT, mMax);
//	DDX_Control(pDX, IDC_MINLIMIT, mMinLimit);
//	DDX_Control(pDX, IDC_MAXLIMIT, mMaxLimit);
	DDX_Control(pDX, IDC_SLIDER, mSlider);
}

BEGIN_MESSAGE_MAP(LimitSliderDlg, CDialog)
	ON_WM_ACTIVATE()
	ON_WM_CTLCOLOR()
	ON_MESSAGE( WM_RANGESLIDER_CHANGED, OnRangeSliderChanged )
	ON_MESSAGE( WM_RANGESLIDER_TRACK, OnRangeSliderTrack )
	ON_EN_KILLFOCUS(IDC_MINEDIT, OnEnKillFocusMinEdit)
	ON_EN_KILLFOCUS(IDC_MAXEDIT, OnEnKillFocusMaxEdit)
	ON_EN_CHANGE(IDC_MINEDIT, OnEnChangeMinEdit)
	ON_EN_CHANGE(IDC_MAXEDIT, OnEnChangeMaxEdit)
END_MESSAGE_MAP()

CString LimitSliderDlg::formatValue( float value )
{
	BW_GUARD;

	CString result, format;
	format.Format( L"%%.%df", mDigits );
	result.Format( format, value );
	return result;
}

CRect LimitSliderDlg::getBestRect( CSize size, CPoint p )
{
	BW_GUARD;

	CPoint pos = controls::validateDlgPos(
		size, p, controls::LEFT, controls::TOP );

	return CRect( pos, pos + size );
}

void LimitSliderDlg::internalShow( float minLimit, float maxLimit, float min, float max,
	unsigned int digits, LimitSlider* limitSlider )
{
	BW_GUARD;

	mDigits = digits;
	mLimitSlider = limitSlider;

	// If digits == 0, assume integer type
	if (digits == 0)
	{
		mMin.SetNumericType( controls::EditNumeric::ENT_INTEGER );
		mMax.SetNumericType( controls::EditNumeric::ENT_INTEGER );
	}
	else
	{
		mMin.SetNumericType( controls::EditNumeric::ENT_FLOAT );
		mMax.SetNumericType( controls::EditNumeric::ENT_FLOAT );
	}

	mMin.SetNumDecimals( digits );
	mMax.SetNumDecimals( digits );
	mMin.SetValue( min );
	mMax.SetValue( max );

	UpdateData( FALSE );

//	mMinLimit.SetWindowText( formatValue( minLimit ) );
//	mMaxLimit.SetWindowText( formatValue( maxLimit ) );

	mSlider.setRange( minLimit, maxLimit, (int)mDigits );
	mSlider.setThumbValues( min, max );

	CPoint cursor;
	GetCursorPos( &cursor );

	CRect client;
	GetWindowRect( &client );

	CRect rc( getBestRect( client.Size(), cursor ) );
	MoveWindow( &rc);
	ShowWindow( SW_SHOW );
	SetActiveWindow();
	UpdateWindow();
}

void LimitSliderDlg::show( float minLimit, float maxLimit, float min, float max,
	unsigned int digits, LimitSlider* limitSlider )
{
	BW_GUARD;

	static LimitSliderDlg* limitSliderDlg = NULL;
	if( !limitSliderDlg )
	{
		limitSliderDlg = new LimitSliderDlg;
		limitSliderDlg->Create( IDD, AfxGetMainWnd() );
	}

	limitSliderDlg->internalShow( minLimit, maxLimit, min, max, digits, limitSlider );
}

// LimitSliderDlg message handlers
void LimitSliderDlg::OnActivate( UINT nState, CWnd*, BOOL )
{
	BW_GUARD;

	if( nState == WA_INACTIVE )
	{
		ShowWindow( SW_HIDE );
		UpdateWindow();
	}
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
afx_msg HBRUSH LimitSliderDlg::OnCtlColor( CDC* pDC, CWnd* pWnd, UINT nCtlColor )
{
	BW_GUARD;

	HBRUSH brush = CDialog::OnCtlColor( pDC, pWnd, nCtlColor );

	if (mLimitSlider)
	{
		mMin.SetBoundsColour( pDC, pWnd,
			mLimitSlider->getMinRangeLimit(), mLimitSlider->getMaxRangeLimit() );
		mMax.SetBoundsColour( pDC, pWnd,
			mLimitSlider->getMinRangeLimit(), mLimitSlider->getMaxRangeLimit() );
	}

	return brush;
}

LRESULT LimitSliderDlg::OnRangeSliderChanged(WPARAM wParam, LPARAM lParam)
{
	BW_GUARD;

	if( mChanging )
		return 0;
	mChanging = true;
	float min, max;
	mSlider.getThumbValues( min, max );
	mMin.SetValue( min );
	mMax.SetValue( max );
	UpdateData( FALSE );
	if (mLimitSlider != NULL)
	{
		mLimitSlider->setRange( min, max );
	}
	mChanging = false;

	return 1;
}

LRESULT LimitSliderDlg::OnRangeSliderTrack( WPARAM wParam, LPARAM lParam )
{
	BW_GUARD;

	if (mChanging)
	{
		return 0;
	}
	mChanging = true;
	bool needUpdate = false;
	float min, max, minLimit, maxLimit;
	mSlider.getRange( minLimit, maxLimit );
	mSlider.getThumbValues( min, max );
	if (min < minLimit)
	{
		min = minLimit;
		needUpdate = true;
	}
	else if (min > maxLimit)
	{
		min = maxLimit;
		needUpdate = true;
	}

	if (max < minLimit)
	{
		max = minLimit;
		needUpdate = true;
	}
	else if (max > maxLimit)
	{
		max = maxLimit;
		needUpdate = true;
	}

	mMin.SetValue( min );
	mMax.SetValue( max );
	if (needUpdate)
	{
		mSlider.setThumbValues( min, max );
	}
	UpdateData( FALSE );
	if (mLimitSlider != NULL)
	{
		mLimitSlider->setRange( min, max );
	}
	mChanging = false;

	return 1;

}


void LimitSliderDlg::OnEnKillFocusMinEdit()
{
	BW_GUARD;

	if( mChanging )
		return;
	float minLimit, maxLimit, min, max;
	mSlider.getRange( minLimit, maxLimit );
	mSlider.getThumbValues( min, max );

	UpdateData( TRUE );
	float f = mMin.GetValue();

	if( f < minLimit )
	{
		mMin.SetValue( minLimit );
		UpdateData( FALSE );
	}
	else if( f > max )
	{
		mMin.SetValue( max );
		UpdateData( FALSE );
	}
	mChanging = true;
	mSlider.setThumbValues( f, max );
	mChanging = false;
}

void LimitSliderDlg::OnEnKillFocusMaxEdit()
{
	BW_GUARD;

	if( mChanging )
		return;
	float minLimit, maxLimit, min, max;
	mSlider.getRange( minLimit, maxLimit );
	mSlider.getThumbValues( min, max );

	UpdateData( TRUE );
	float f = mMax.GetValue();

	if( f > maxLimit )
	{
		mMax.SetValue( maxLimit );
		UpdateData( FALSE );
	}
	else if( f < min )
	{
		mMax.SetValue( min );
		UpdateData( FALSE );
	}
	mChanging = true;
	mSlider.setThumbValues( min, f );
	mChanging = false;
}

void LimitSliderDlg::OnEnChangeMinEdit()
{
	BW_GUARD;

	if( mChanging )
		return;
	float minLimit, maxLimit, min, max;
	mSlider.getRange( minLimit, maxLimit );
	mSlider.getThumbValues( min, max );

	UpdateData( TRUE );
	float f = mMin.GetValue();

	if( f >= minLimit && f <= max )
	{
		mChanging = true;
		mSlider.setThumbValues( f, max );
		if (mLimitSlider != NULL)
		{
			mLimitSlider->setRange( f, max );
		}
		mChanging = false;
	}
}

void LimitSliderDlg::OnEnChangeMaxEdit()
{
	BW_GUARD;

	if( mChanging )
		return;
	float minLimit, maxLimit, min, max;
	mSlider.getRange( minLimit, maxLimit );
	mSlider.getThumbValues( min, max );

	UpdateData( TRUE );
	float f = mMax.GetValue();

	if( f <= maxLimit && f >= min )
	{
		mChanging = true;
		mSlider.setThumbValues( min, f );
		if (mLimitSlider != NULL)
		{
			mLimitSlider->setRange( min, f );
		}
		mChanging = false;
	}
}

// LimitSlider

IMPLEMENT_DYNAMIC(LimitSlider, CSliderCtrl)
LimitSlider::LimitSlider()
	: mDigits( 0 )
	, mMin( 0 )
	, mMax( 100 )
	, mMinLimit( 0 )
	, mMaxLimit( 1000 )
	, mPageSteps( 10 )
{
}

LimitSlider::~LimitSlider()
{}

float LimitSlider::getValue() const
{
	int pos = GetPos();
	float fpos = float( ( float( pos ) ) / powf( 10.f, (float)mDigits ) );
	if( fpos <= mMin )
		fpos = mMin;
	if( fpos >= mMax )
		fpos = mMax;
	return fpos;
}

void LimitSlider::setValue( float value )
{
	BW_GUARD;

	if( value > mMaxLimit )
		value = mMaxLimit;
	if( value < mMinLimit )
		value = mMinLimit;
	if( value > mMax )
		setRange( mMin, value );
	if( value < mMin )
		setRange( value, mMax );
	SetPos( BW_ROUND_TO_INT( value * powf( 10.f, (float)mDigits ) ) );
	GetParent()->PostMessage( WM_HSCROLL, SB_THUMBPOSITION, NULL );
}

void LimitSlider::setDigits( unsigned int digits )
{
	BW_GUARD;

	if( mDigits != digits )
	{
		mDigits = digits;
		int min = BW_ROUND_TO_INT( mMin * powf( 10.f, (float)mDigits ) );
		int max = BW_ROUND_TO_INT( mMax * powf( 10.f, (float)mDigits ) );
		SetRange( min, max );
		recalcPageSize();
	}
}

void LimitSlider::setRange( float min, float max )
{
	BW_GUARD;

	ASSERT( min >= mMinLimit );
	ASSERT( max <= mMaxLimit );
	if( min != mMin || max != mMax )
	{
		float value = getValue();
		mMin = min;
		mMax = max;
		int sliderMin = BW_ROUND_TO_INT( mMin * powf( 10.f, (float)mDigits ) );
		int sliderMax = BW_ROUND_TO_INT( mMax * powf( 10.f, (float)mDigits ) );
		SetRange( sliderMin, sliderMax, TRUE );
		if( value < min )
			value = min;
		if( value > max )
			value = max;
		setValue( value );
		recalcPageSize();
	}
}

void LimitSlider::setRangeLimit( float min, float max )
{
	BW_GUARD;

	mMinLimit = min;
	mMaxLimit = max;
	if( mMin < min || mMax < max )
	{
		setRange(	mMin < min	?	min	:	mMin,
			mMax > max	?	max	:	mMax );
	}
}

void LimitSlider::setPageSteps( unsigned int value )
{
	BW_GUARD;
	mPageSteps = value;
	recalcPageSize();
}

float LimitSlider::getMinRange() const
{
	return mMin;
}

float LimitSlider::getMaxRange() const
{
	return mMax;
}

float LimitSlider::getMinRangeLimit() const
{
	return mMinLimit;
}

float LimitSlider::getMaxRangeLimit() const
{
	return mMaxLimit;
}

unsigned int LimitSlider::getPageSteps() const
{
	return mPageSteps;
}

void LimitSlider::beginEdit()
{
	BW_GUARD;

	LimitSliderDlg::show( mMinLimit, mMaxLimit, mMin, mMax, mDigits, this );
}

void LimitSlider::recalcPageSize()
{
	float pageSize = ( mMax - mMin ) / mPageSteps * powf( 10.f, (float)mDigits );
	SetPageSize( BW_ROUND_TO_INT( pageSize ) );
}

BEGIN_MESSAGE_MAP(LimitSlider, CSliderCtrl)
	ON_WM_RBUTTONDOWN()
END_MESSAGE_MAP()

// LimitSlider message handlers
void LimitSlider::OnRButtonDown( UINT nFlags, CPoint point )
{
	BW_GUARD;

	LimitSliderDlg::show( mMinLimit, mMaxLimit, mMin, mMax, mDigits, this );
}

void LimitSlider::PreSubclassWindow()
{
	CSliderCtrl::PreSubclassWindow();
	recalcPageSize();
}

BW_END_NAMESPACE

