#include "pch.hpp"
#include "input.hpp"
#include "ime.hpp"
#include "ImeUI.h"

BW_BEGIN_NAMESPACE

#define LANG_CHT	MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL)
#define LANG_CHS	MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED)

namespace 
{
	IME::State immGetIMEState()
	{
		switch( ImeUi_GetState() )
		{
		case IMEUI_STATE_ENGLISH:	return IME::STATE_ENGLISH;
		case IMEUI_STATE_ON:		return IME::STATE_ON;
		case IMEUI_STATE_OFF:		return IME::STATE_OFF;
		default:					return IME::STATE_OFF;	
		}
	}

	IME::Language immGetIMELanguage()
	{
		switch ( ImeUi_GetPrimaryLanguage() )
		{
		case LANG_CHINESE:
			switch ( SUBLANGID(ImeUi_GetLanguage()) )
			{
			case SUBLANG_CHINESE_SIMPLIFIED:	return IME::LANGUAGE_CHS;
			case SUBLANG_CHINESE_TRADITIONAL:	return IME::LANGUAGE_CHT;
			default:							return IME::LANGUAGE_NON_IME;
			}
		case LANG_CHS:		return IME::LANGUAGE_CHS;
		case LANG_CHT:		return IME::LANGUAGE_CHT;
		case LANG_JAPANESE: return IME::LANGUAGE_JAPANESE;
		case LANG_KOREAN:	return IME::LANGUAGE_KOREAN;
		default:			return IME::LANGUAGE_NON_IME;
		}
	}
}

BW_SINGLETON_STORAGE( IME )

PY_ENUM_MAP( IME::Language );
PY_ENUM_CONVERTERS_CONTIGUOUS( IME::Language );

PY_ENUM_MAP( IME::State );
PY_ENUM_CONVERTERS_CONTIGUOUS( IME::State );

IME::IME() 
	:	hwnd_( NULL ),
		allowEnable_( true),
		enabled_( false ),
		state_( STATE_OFF ),
		language_( LANGUAGE_NON_IME ),
		compositionCursorPosition_( 0 ),
		selectedCandidate_( 0 ),
		readingVisible_( false ),
		readingVertical_( false ),
		candidatesVisible_( false ),
		candidatesVertical_( false )

{
	BW_GUARD;
}

IME::~IME()
{
	BW_GUARD;
	ImeUi_Uninitialize();
}


bool IME::initIME( HWND hWnd )
{
	BW_GUARD;
	ImeUiCallback_Malloc = bw_malloc;
	ImeUiCallback_Free = bw_free;
	ImeUiCallback_OnChar = imeCharHandler;
	if (!ImeUi_Initialize( hWnd, false ))
	{
		return false;
	}

	hwnd_ = hWnd;
	pullCurrentState();
	return true;
}

bool IME::handleWindowsMessage( HWND hWnd, UINT msg, WPARAM& wParam, 
								LPARAM& lParam, LRESULT& result )
{
	BW_GUARD;
	bool handled = false;
	LRESULT imeResult = ImeUi_ProcessMessage( hWnd, msg, wParam, lParam, &handled );
	if (handled)
	{
		result = imeResult;
	}
	return handled;
}

void IME::onInputLangChange()
{
	if (hwnd_)
	{
		// Fix for making sure the language bar updates itself. If there's
		// any pause directly after language change it seems to screw up the refresh.
		HWND hwndImeDef = ImmGetDefaultIMEWnd( hwnd_ );
		if ( hwndImeDef )
		{
			SendMessageA(hwndImeDef, WM_IME_CONTROL, IMC_CLOSESTATUSWINDOW, 0);
			SendMessageA(hwndImeDef, WM_IME_CONTROL, IMC_OPENSTATUSWINDOW, 0);
		}
	}
}

/*static*/ void IME::imeCharHandler( WCHAR ch )
{
	BW_GUARD;
	wchar_t chs[2] = { ch, 0 };
	InputDevices::pushIMECharEvent( chs );

}

void IME::pullCurrentState()
{
	BW_GUARD;
	language_ = immGetIMELanguage();
	state_ = immGetIMEState();
	composition_ = ImeUi_GetCompositionString();
	compositionCursorPosition_ = (int)ImeUi_GetImeCursorChars();
	readingVisible_ = ImeUi_IsShowReadingWindow();
	readingVertical_ = !ImeUi_IsHorizontalReading();
	candidatesVisible_ = ImeUi_IsShowCandListWindow();
	selectedCandidate_ = (int)ImeUi_GetCandidateSelection();

}

void IME::update()
{
	BW_GUARD;
	event_ = IMEEvent();

	//
	// Go through and see what has changed since last time...

	language_ = immGetIMELanguage();

	// IME state
	if ( immGetIMEState() != state_ )
	{
		state_ = immGetIMEState();
		event_.stateChanged( true );
	}

	// reading string
	if ( ImeUi_GetReadingString() != reading_ )
	{
		reading_ = ImeUi_GetReadingString();
		event_.readingChanged( true );
	}

	// composition string
	if ( ImeUi_GetCompositionString() != composition_ )
	{
		composition_ = ImeUi_GetCompositionString();
		event_.compositionChanged( true );
	}

	// composition string attributes
	bool compAttrChanged = 
		( compositionAttr_.size() != composition_.size() );

	compositionAttr_.resize( composition_.length() );

	for( size_t i = 0; i < composition_.length(); i++ )
	{
		uint8 newAttr = uint8( ImeUi_GetCompStringAttr()[i] );

		if (newAttr != compositionAttr_[i])
		{
			compositionAttr_[i] = newAttr;
			compAttrChanged = true;
		}
	}

	if ( compAttrChanged )
	{
		event_.compositionChanged( true );
	}


	// position of the composition string cursor
	int compositionCursorPos = (int)ImeUi_GetImeCursorChars();
	if ( compositionCursorPos != compositionCursorPosition_ )
	{
		compositionCursorPosition_ = compositionCursorPos;
		event_.compositionCursorPositionChanged( true );
	}

	// visibility and orientation of various IME components
	if ( ImeUi_IsShowReadingWindow() != readingVisible_ )
	{
		readingVisible_ = ImeUi_IsShowReadingWindow();
		event_.readingVisibilityChanged( true );
	}

	if ( !ImeUi_IsHorizontalReading() != readingVertical_ )
	{
		readingVertical_ = !ImeUi_IsHorizontalReading();
	}

	if ( ImeUi_IsShowCandListWindow() != candidatesVisible_ )
	{
		candidatesVisible_ = ImeUi_IsShowCandListWindow();
		event_.candidatesVisibilityChanged( true );
	}

	if ( ImeUi_IsVerticalCand() != candidatesVertical_ )
	{
		candidatesVertical_ = ImeUi_IsVerticalCand();
	}

	if ( ImeUi_GetCandidateSelection() != selectedCandidate_ )
	{
		selectedCandidate_ = (int)ImeUi_GetCandidateSelection();
		event_.selectedCandidateChanged( true );
	}


	// current list of candidate strings
	WStringArray currentCandidates;
	for ( DWORD i = 0; i < MAX_CANDLIST; i++ )
	{
		if( *ImeUi_GetCandidate( i ) == L'\0' )
			break;

		currentCandidates.push_back(ImeUi_GetCandidate( i ) );
		if ( i < candidates_.size() && currentCandidates[i] != candidates_[i] )
		{
			event_.candidatesChanged( true );
		}
	}

	if ( currentCandidates.size() != candidates_.size() )
	{
		event_.candidatesChanged( true );
	}

	if ( event_.candidatesChanged() )
	{
		candidates_ = currentCandidates;
	}
}

void IME::processEvents( InputHandler & handler )
{
	BW_GUARD;
	//
	// Post off pending events
	if ( event_.dirty() )
	{
		handler.handleIMEEvent( event_ );
	}
}

void IME::allowEnable( bool allow )
{
	BW_GUARD;
	allowEnable_ = allow;
	enabled( enabled_, false );
}

void IME::enabled( bool enable, bool finalise )
{
	BW_GUARD;
	if (finalise)
	{
		ImeUi_FinalizeString( false );
	}

	enabled_ = enable;	
	ImeUi_EnableIme( enabled_ && allowEnable_ );	
}

bool IME::enabled()
{
	BW_GUARD;
	return enabled_;
}

BW_END_NAMESPACE

// ime.cpp

