#include "pch.hpp"
#include "controls/edit_numeric.hpp"
#include "controls/user_messages.hpp"
#include "resmgr/string_provider.hpp"
#include <afxole.h>
#include <math.h>
#include <float.h>

BW_BEGIN_NAMESPACE
namespace controls
{

static const int numberDecimalsAllowed = 3;

static const int MAX_DECIMALS = 16;
static bool s_powInited = false;
static double s_powTen[ MAX_DECIMALS ];
static double s_powOneTenth[ MAX_DECIMALS ];


/////////////////////////////////////////////////////////////////////////////
// EditNumeric

EditNumeric::EditNumeric()
	: type_(ENT_FLOAT)
	, decimalPointPresent_(FALSE)
	, displayThousandsSeperator_(FALSE)
	, previousTextIsCurrent_( false )
	, value_(0.f)
	, minimum_(-FLT_MAX)
	, includeMinimum_(true)
	, maximum_(FLT_MAX)
	, includeMaximum_(true)
	, allowNegative_(true)
	, allowEmpty_(false)
	, isEmpty_(true)
	, previousText_("")
	, dirty_(false)
	, commitOnFocusLoss_(true)
	, numDecimals_(3)
	, silent_( true )
	, touched_( false )
{
	BW_GUARD;

	if (!s_powInited)
	{
		s_powInited = true;
		for (int i = 0; i < MAX_DECIMALS; ++i)
		{
			s_powTen[ i ] = pow( 10.0, i );
			s_powOneTenth[ i ] = pow( 0.1f, i );
		}
	}
}

EditNumeric::~EditNumeric()
{
}


void EditNumeric::initInt( int minVal, int maxVal, int val )
{
	BW_GUARD;

	SetNumericType( controls::EditNumeric::ENT_INTEGER );
	SetAllowNegative( minVal < 0 );
	SetMinimum( double( minVal ) );
	SetMaximum( double( maxVal ) );
	SetIntegerValue( val );
}


void EditNumeric::initFloat( double minVal, double maxVal, double val )
{
	BW_GUARD;

	SetNumericType( controls::EditNumeric::ENT_FLOAT );
	SetAllowNegative( minVal < 0 );
	SetMinimum( minVal );
	SetMaximum( maxVal );
	SetDoubleValue( val );
}


BEGIN_MESSAGE_MAP(EditNumeric, CEdit)
	//{{AFX_MSG_MAP(EditNumeric)
	ON_WM_CHAR()
	ON_CONTROL_REFLECT_EX(EN_CHANGE, OnChange)
	ON_CONTROL_REFLECT_EX(EN_SETFOCUS, OnSetfocus)
	ON_CONTROL_REFLECT_EX(EN_KILLFOCUS, OnKillfocus)
	ON_WM_GETDLGCODE()
	//}}AFX_MSG_MAP
   ON_MESSAGE(WM_PASTE, OnPaste)
END_MESSAGE_MAP()

//ON_CONTROL_REFLECT


/////////////////////////////////////////////////////////////////////////////
// EditNumeric message handlers

void EditNumeric::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	BW_GUARD;

	int start, end;
	GetSel(start, end);

	touched_ = true;

	if (nChar == 13) // Catch enter press...
	{
		OnKillfocus();  // commit the contents
		return;			// Ignore the enter
	}
	
	// if character suitable, continue
	if(DoesCharacterPass(nChar, start))
	{
		CEdit::OnChar(nChar, nRepCnt, nFlags);
	}
}


UINT EditNumeric::OnGetDlgCode()
{
	return CEdit::OnGetDlgCode() | DLGC_WANTALLKEYS;
}


BOOL EditNumeric::DoesCharacterPass(UINT nChar, int index)
{ 
	BW_GUARD;

	BOOL allowed = TRUE;

	// if the character is not allowed, return without doing anything
	// check isprint to ensure that unprinted characters are passed to cedit (ctrl-v,etc.)
	if(!iswdigit(nChar) && iswprint(nChar))
	{
		allowed = FALSE;
		switch(nChar)
		{
			case '.':
				{
					int start, end;
					GetSel(start, end);
					CString tempString;
					GetWindowText(tempString);
					int decimalPlace = tempString.Find(L".");

					allowed =  ((type_ == ENT_FLOAT) && ((decimalPlace == -1) || ((decimalPlace >= start) && (decimalPlace < end))));

					break;
				}
			case '-':      // allow '-' only as first character
				{
					if (allowNegative_ && (index == 0))
						allowed = TRUE;
					break;
				}
			default:
				break;
		} 
	}
    
	// do not allow more than numDecimals_ decimal places
	if (type_ == ENT_FLOAT)
	{
		int start, end;
		GetSel(start, end);
		bool multiSelected = start != end;
		CString tempString;
		GetWindowText(tempString);
		int decimalPlace = tempString.Find(L".");
		int numberDecimals = tempString.GetLength() - decimalPlace;
		if (!multiSelected &&
			(decimalPlace != -1) && (index > decimalPlace) && 
			(numberDecimals > numDecimals_) && 
			(nChar != 0x8)) // 0x8 == backspace
		{
			allowed = FALSE;
		}
	}

	return allowed;
}


BOOL EditNumeric::OnChange()
{
	BW_GUARD;

	// update the real object
	// advise only if there was a change
	CString windowText;
	if (previousTextIsCurrent_)
	{
		windowText = previousText_;
	}
	else
	{
		GetWindowText(windowText);
	}

	// update the current value
	SetNumericValue( &windowText );

	isEmpty_ = ((allowEmpty_) && windowText.IsEmpty());

	if (!previousTextIsCurrent_ && windowText != previousText_)
	{
		int start, end;
		GetSel(start, end);

		// tell our parent and ourselves of a change
		SendMessage(WM_EDITNUMERIC_CHANGE);
		GetParent()->SendMessage(WM_EDITNUMERIC_CHANGE);

		// the change might fail in the SendMessage
		// also, previousText_ may change, do not check against this
		CString finalText;
		GetWindowText(finalText);
		if (finalText == windowText)
		{
			SetSel(start, start);
			previousText_ = windowText;
		}
		else
		{
			windowText = finalText;
		}
	}

	// update whether the decimal has been used or not
	if (type_ == ENT_INTEGER)
		return FALSE;

	int decimalPlace = windowText.Find(L".");
	if (decimalPlace != -1)
	{
		decimalPointPresent_ = TRUE;
	}
	else
	{
		decimalPointPresent_ = FALSE;
	}

	return FALSE;
}

void EditNumeric::SetNumericText(BOOL useFormatting)
{
	BW_GUARD;

	SetText(GetStringForm(useFormatting));
}


CString EditNumeric::GetFormattedString( const double& value, BOOL insertFormatting )
{
	BW_GUARD;

	if (value == 0.0f)
	{
		CString finalString;
		if (type_ == ENT_INTEGER)
			finalString = "0";
		else
			finalString = "0.0";
	
		return finalString;
	}

	// Round to the appropriate number of decimal points
	double roundEpsilon = (value > 0.0f) ?
			5.0 / s_powTen[ numDecimals_ + 1 ] :
			-5.0 / s_powTen[ numDecimals_ + 1 ];
	double dValue = (
			BW_ROUND( double( value ) * s_powTen[ numDecimals_ ] ) + roundEpsilon
					) / ( s_powTen[ numDecimals_ ] );

	// take dValue from edit control's double dValue
	const int maxStringLength = 50;	// pretty big!
	wchar_t charString[maxStringLength];
	bw_snwprintf( charString, ARRAY_SIZE(charString), L"%.6f", dValue );
	CString initialString( charString );

	int	signNegative = 0;
	if (initialString.Find('-') != -1)
	{
		// must be at the beginning (ensured elsewhere)
		signNegative = 1;
		initialString.TrimLeft( L"-" );
	}

	int decimalPlace = initialString.Find('.');

	if (decimalPlace != -1)
	{
		// remove trailing '0'
		initialString.TrimRight( L"0" );

		// remove the decimal (put back later)
		initialString.Delete(decimalPlace);

		// keep at least one zero after the decimal
		if (initialString.GetLength() == decimalPlace)
			initialString.AppendChar('0');
	}

	int string_length = initialString.GetLength();

	// restrict number of decimals to numDecimals_ (float inaccuracies may occur soon after this )
	string_length = min(string_length, decimalPlace + numDecimals_);

	// put together the final rendered string
	CString finalString;

	if(signNegative)
	{
		finalString = L"-";
	}

	for(int i = 0; i < string_length; i++)
	{
		int wholenum_place = decimalPlace - i;
		if(insertFormatting && displayThousandsSeperator_ && i > 0 && wholenum_place > 2)
		{
			if((wholenum_place % 3) == 0)
			{
				finalString.AppendChar( L',' );
			}
		}
		if(decimalPlace == i)
		{
			if (type_ == ENT_INTEGER)
				break;

			finalString.AppendChar( L'.' );
		}

		finalString.AppendChar( initialString[i] );
	}

	return finalString;
}


CString EditNumeric::GetStringForm(BOOL useFormatting)
{
	BW_GUARD;

	// special case if the field is empty (and is allowed to be)
	if ((allowEmpty_) && (isEmpty_))
	{
		return L"";
	}
		
	return this->GetFormattedString( value_, useFormatting );
}


void EditNumeric::GetStringFormOfValue( double value, CString & result,
										BOOL useFormatting /*= true*/ )
{
	BW_GUARD;

	result = this->GetFormattedString( this->clamp( value ), useFormatting );
}


void EditNumeric::SetText(const CString & text)
{
	BW_GUARD;

	previousText_ = text;
	previousTextIsCurrent_ = true;
	SetWindowText( text );
	previousTextIsCurrent_ = false;
}

/**
 *  Checks if the value is beyond the passed range.  Sets the text colour to
 *	red if it is.
 *
 *	@param pDC	Contains a pointer to the display context for the child window.
 *	@param pWnd	Contains a pointer to the control asking for the color.
 *	@param editMinValue	The lower bound of the specified range.
 *	@param editMaxValue	The upper bound of the specified range.
 *	@return	True if this edit numeric was to be checked, false otherwise.
 */
template<class rangeType>
bool EditNumeric::SetBoundsColour(
		CDC* pDC, CWnd* pWnd, rangeType minValue, rangeType maxValue )
{
	bool checked = false;

	if (this->m_hWnd == pWnd->m_hWnd)
	{
		checked = true;

		CString text;
		this->GetWindowText( text );
		rangeType val = rangeType( _wtof( text ) );
		minValue -=
			( abs ( floor( minValue ) ) *
			std::numeric_limits<rangeType>::epsilon() );
		maxValue +=
			( abs ( floor( maxValue ) ) *
			std::numeric_limits<rangeType>::epsilon() );

		if (val > maxValue || val < minValue)
		{
			pDC->SetTextColor( 0x0000ff );
		}
	}

	return checked;
}

void ForceFloatAndDoubleTemplates()
{
	EditNumeric edit;
	edit.SetBoundsColour( nullptr, nullptr, 0.f, 0.f );
	edit.SetBoundsColour( nullptr, nullptr, 0.0, 0.0 );
}

void EditNumeric::SetNumericValue( CString * pText /*= NULL*/ )
{
	BW_GUARD;

	CString tempString;
	if (pText)
	{
		tempString = *pText;
	}
	else
	{
		GetWindowText(tempString);
	}

	// remove the commas
	tempString.Remove(',');

	double newValue = _wtof(tempString);

	// restrict the range of values
	if (newValue <= minimum_)
	{
		newValue = minimum_;
		if (!includeMinimum_)
			newValue += s_powOneTenth[ numDecimals_ ];
	}

	if (newValue >= maximum_)
	{
		newValue = maximum_;
		if (!includeMaximum_)
			newValue -= s_powOneTenth[ numDecimals_ ];
	}

	if ( GetStringForm( false ) != tempString )
		value_ = newValue;
}


double EditNumeric::clamp( double value ) const
{
	BW_GUARD;

	// restrict the range of values
	if (value <= minimum_)
	{
		value = minimum_;
		if (!includeMinimum_)
			value += s_powOneTenth[ numDecimals_ ];
	}

	if (value >= maximum_)
	{
		value = maximum_;
		if (!includeMaximum_)
			value -= s_powOneTenth[ numDecimals_ ];
	}

	return value;
}


void EditNumeric::SetDoubleValue( double value ) 
{
	BW_GUARD;

	isEmpty_ = false;
	touched_ = true;

	// round off the value correctly
	value_ = this->GetRoundedNumber( this->clamp( value ) );

	//Render with formating
	SetNumericText( TRUE );

	//Avoid a superfluous update
	oldValue_ = value_;
}

double EditNumeric::GetRoundedNumber( double value ) const
{
	BW_GUARD;

	// Round to the appropriate number of decimal points
	return BW_ROUND(double( value ) * s_powTen[ numDecimals_ ]) /
									s_powTen[ numDecimals_ ];
}

void EditNumeric::Clear() 
{
	BW_GUARD;

	if (!allowEmpty_) return;
	SetWindowText(L"");
	value_ = 0.f;
	isEmpty_ = true;
}


BOOL EditNumeric::OnSetfocus() 
{
	BW_GUARD;

	// set text without formatting, since going into user edit mode
	SetNumericText(FALSE);
	oldValue_ = value_;
	SetSel(0,-1);

	return FALSE; // Pass on this event
}

void EditNumeric::doCommit( bool focusLost /* = false */ )
{
	BW_GUARD;

	SetNumericValue();

	// Render
	SetNumericText(TRUE);

	// advise of change
	if (value_ != oldValue_)
	{
		if ((!focusLost) || (commitOnFocusLoss_))
		{
			dirty_ = true;
		}
		oldValue_ = value_;
		SendMessage(WM_EDITNUMERIC_FINAL_CHANGE);
		GetParent()->SendMessage(WM_EDITNUMERIC_FINAL_CHANGE);
	}
}

BOOL EditNumeric::OnKillfocus() 
{
	BW_GUARD;

	if (silent_ || isValidValue())
	{
		doCommit( true );

		return FALSE; // Pass on this event
	}
	else
	{
		this->SetFocus();
		return TRUE;
	}
}


bool EditNumeric::isValidValue()
{
	BW_GUARD;

	CString tempString;
	GetWindowText(tempString);

	// remove the commas
	tempString.Remove(',');

	double newValue = _wtof(tempString);

	// restrict the range of values
	if (newValue < minimum_)
	{
		CString warningTooSmall = Localise(
			L"CONTROLS/EDIT_NUMERIC/VALUE_TOO_SMALL",
			(LPCTSTR)this->GetFormattedString( minimum_, TRUE ) );

		BW::wstring warningTooSmallTitle = Localise( L"CONTROLS/EDIT_NUMERIC/VALUE_TOO_SMALL_TITLE" );
		
		this->silent_ = true;
		bool ret = MessageBox( warningTooSmall, warningTooSmallTitle.c_str(), 
				MB_OKCANCEL | MB_DEFBUTTON1 | MB_ICONWARNING ) == IDOK;

		this->silent_ = false;

		return ret;

	}
	else if (newValue > maximum_)
	{
		CString warningTooBig = Localise(
			L"CONTROLS/EDIT_NUMERIC/VALUE_TOO_BIG",
			(LPCTSTR)this->GetFormattedString( maximum_, TRUE ) );
		BW::wstring warningTooBigTitle = Localise( L"CONTROLS/EDIT_NUMERIC/VALUE_TOO_BIG_TITLE" );

		this->silent_ = true;
		bool ret = MessageBox( warningTooBig, warningTooBigTitle.c_str(),
			MB_OKCANCEL | MB_DEFBUTTON1 | MB_ICONWARNING ) == IDOK;
		this->silent_ = false;

		return ret;

	}

	return true;

}


void EditNumeric::SetDisplayThousandsSeperator(BOOL displaySeperator)
{
	BW_GUARD;

	displayThousandsSeperator_ = displaySeperator;

	// Render
	SetNumericText(TRUE);
}


void EditNumeric::SetNumericType(EditNumericType type)
{
	type_ = type;
}


void EditNumeric::SetMinimum(double minimum, bool includeMinimum)
{
	minimum_ = minimum;
	includeMinimum_ = includeMinimum;

	maximum_ = max(maximum_, minimum_);

	allowNegative_ = minimum_ < 0.f;
}

void EditNumeric::SetMaximum(double maximum, bool includeMaximum)
{
	maximum_ = maximum;
	includeMaximum_ = includeMaximum;

	minimum_ = min(maximum_, minimum_);
}

double EditNumeric::GetMinimum()
{
	return minimum_;
}

double EditNumeric::GetMaximum()
{
	return maximum_;
}

bool EditNumeric::isRanged()
{
	return ( minimum_ != -FLT_MAX ) || ( maximum_ != FLT_MAX );
}


void EditNumeric::SetNumDecimals( int num )
{
	if (numDecimals_ < 0)
	{
		numDecimals_ = 0;
		ERROR_MSG( "EditNumeric::SetNumDecimals: Called with a "
					"value smaller than zero (%d)\n", num );
	}
	else if (numDecimals_ >= MAX_DECIMALS)
	{
		numDecimals_ = MAX_DECIMALS - 1;
		ERROR_MSG( "EditNumeric::SetNumDecimals: Called with a value that "
				"exceeds the maximum of %d (%d)\n", MAX_DECIMALS - 1, num );
	}
	else
	{
		numDecimals_ = num;
	}
}

// this signature used for pasting function in order to get around
// bug mentioned in Article ID: Q195032 of the MS knowledge base 
LRESULT EditNumeric::OnPaste(WPARAM Wparam, LPARAM LParam) 
{
	BW_GUARD;

	UINT this_char;
	int start_sel, end_sel, i;
	BOOL does_clipstring_pass = TRUE, pre_was_decimal_used;
	CString buffer, workstr;
	COleDataObject	obj;

	if (obj.AttachClipboard()) 
	{
		if (obj.IsDataAvailable(CF_UNICODETEXT)) 
		{
			HGLOBAL hmem = obj.GetGlobalData(CF_UNICODETEXT);

			int size = (UINT) ::GlobalSize(hmem);
			wchar_t* src = (wchar_t*) ::GlobalLock(hmem);
			CString buffer;
			LPTSTR str = buffer.GetBufferSetLength( size / sizeof( wchar_t ) );
			memcpy( str, src, size );
			::GlobalUnlock(hmem);

			// pass characters one at a time to control, allowing the control
			// to decide whether to paste it or not (this could be funky for mixed
			// alpha and digit strings)
			buffer.ReleaseBuffer();
			buffer.FreeExtra();

			//  Check buffer for validity- if any character invalid, don't paste
			GetSel(start_sel, end_sel);

			// if decimal is within selection, allow a decimal to be pasted
			// save decimal use flag
			pre_was_decimal_used = decimalPointPresent_;
			if(pre_was_decimal_used == TRUE)
			{
				CString temp_str;
				GetWindowText(temp_str);
				for(i = start_sel; i < end_sel; i++)
				{
					if(temp_str[i] == '.')
						decimalPointPresent_ = FALSE;
				}
			}

			for(i = 0; i < buffer.GetLength(); i++)
			{
				this_char = buffer[i];
				if(!DoesCharacterPass(this_char, start_sel+i))
				{
					does_clipstring_pass = FALSE;
					break;
				}
			}

			// reset decimal use flag
			decimalPointPresent_ = pre_was_decimal_used;

			// continue default windows processing if string okay
			if(does_clipstring_pass)
			{
				Default();
			}
			else   // let user know that paste wasn't allowed
			{
				workstr = Localise(L"CONTROLS/EDIT_NUMERIC/CANNOT_PASTE_TEXT");
				BW::wstring title = Localise(L"CONTROLS/EDIT_NUMERIC/CANNOT_PASTE_TITLE");
				MessageBox( workstr, title.c_str(), MB_OK | MB_ICONINFORMATION);
			}
		}
	}

	return 0;  
}

} // namespace controls
BW_END_NAMESPACE

