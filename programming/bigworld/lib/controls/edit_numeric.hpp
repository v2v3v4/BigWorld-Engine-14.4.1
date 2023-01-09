#ifndef CONTROLS_EDIT_NUMERIC_HPP
#define CONTROLS_EDIT_NUMERIC_HPP

#include "cstdmf/stdmf.hpp"
#include "controls/defs.hpp"
#include "controls/fwd.hpp"

BW_BEGIN_NAMESPACE

/////////////////////////////////////////////////////////////////////////////
// EditNumeric

namespace controls
{
	class CONTROLS_DLL EditNumeric : public CEdit
	{
	public:
		enum EditNumericType
		{
			ENT_FLOAT = 1,
			ENT_INTEGER = 2
		};

		EditNumeric();
		virtual ~EditNumeric();
		
		void		initInt( int minVal, int maxVal, int val );
		void		initFloat( double minVal, double maxVal, double val );

		void		SetDisplayThousandsSeperator(BOOL displaySeperator);
		void		SetNumericType(EditNumericType type);
		
		void		SetMinimum(double minimum, bool includeMinimum = true);
		void		SetMaximum(double maximum, bool includeMaximum = true);

		double		GetMinimum();
		double		GetMaximum();

		bool		isRanged();
		
		void		SetAllowNegative(bool option) { allowNegative_ = option; }
		void		SetAllowEmpty(bool option) { allowEmpty_ = option; }
		
		void		SetNumDecimals( int num );

		void		setSilent( bool value ) { silent_ = value; }
		bool		silent() const	{ return silent_; }
		
		void		touched( bool val )	{ touched_ = val; }
		bool		touched() const	{ return touched_; }

		// Operations
		float		GetValue() const			{ return float(value_); }
		double		GetDoubleValue() const		{ return value_; }
		int			GetIntegerValue() const		{ return int(value_); }
		uint32		GetUIntValue() const		{ return uint32(value_); }
		void		SetValue(float value)		{ SetDoubleValue( double(value) ); }
		void		SetDoubleValue(double value); // This is the prefered method Set*Value method
		void		SetIntegerValue(int value)	{ SetDoubleValue( double(value) ); }
		void		SetUIntValue(uint32 value)	{ SetDoubleValue( double(value) ); }

		double		GetRoundedNumber(double value) const;
		
		void		Clear();
		bool		isEmpty() { return isEmpty_; }
		
		CString	GetStringForm(BOOL useFormatting = true);
		
		// Format external value to the configuration of this EditNumeric.
		void GetStringFormOfValue( double value, CString	& result, BOOL useFormatting = true );

		void		SetNumericText(BOOL InsertFormatting);
		
		void		SetText(const CString & text);

		template<class rangeType>
		bool SetBoundsColour(
			CDC* pDC, CWnd* pWnd, rangeType minValue, rangeType maxValue);
		
		void commitOnFocusLoss( bool state ) { commitOnFocusLoss_ = state; }
		
		bool needsUpdate()	{ return dirty_; }
		void updateDone()	{ dirty_ = false; }
		bool doUpdate()		{ bool temp = dirty_; dirty_ = false; return temp; }
		bool		isValidValue();
		CString GetFormattedString( const double& value, BOOL insertFormatting ); 
		
	private:
		void doCommit( bool focus = false );

		bool				commitOnFocusLoss_;
		
		bool				dirty_;
		
		BOOL				decimalPointPresent_;
		int					decimalPointIndex_;
		BOOL				displayThousandsSeperator_;
		bool				previousTextIsCurrent_;
		double				value_;
		double				oldValue_;
		EditNumericType		type_;
		CString				previousText_;
		
		double				minimum_;
		bool				includeMinimum_;
		double				maximum_;
		bool				includeMaximum_;
		
		bool				allowNegative_;
		bool				allowEmpty_;
		bool				isEmpty_;
		
		int					numDecimals_;

		bool				silent_;

		bool				touched_;
		
		void		SetNumericValue( CString * pText = NULL );
		BOOL		DoesCharacterPass(UINT nChar, int CharacterPosition);
		double		clamp( double value ) const;
		
		// Generated message map functions
	protected:
		//{{AFX_MSG(EditNumeric)
		afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
		afx_msg BOOL OnChange();
		afx_msg BOOL OnSetfocus();
		afx_msg BOOL OnKillfocus();
		//}}AFX_MSG
		afx_msg LRESULT OnPaste(WPARAM Wparam, LPARAM LParam);
		afx_msg UINT OnGetDlgCode();

		DECLARE_MESSAGE_MAP()
	};
}

BW_END_NAMESPACE

#endif