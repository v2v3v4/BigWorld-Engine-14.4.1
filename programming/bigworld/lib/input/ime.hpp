#ifndef BW_IME__HPP
#define BW_IME__HPP

#include "cstdmf/cstdmf_windows.hpp"

#include "cstdmf/init_singleton.hpp"
#include "pyscript/pyobject_plus.hpp"
#include "pyscript/script.hpp"

#include "ime_event.hpp"


BW_BEGIN_NAMESPACE

class InputHandler;

class IME : public Singleton<IME>
{
public:
	enum Language
	{
		LANGUAGE_NON_IME,
		LANGUAGE_CHS,
		LANGUAGE_CHT,
		LANGUAGE_JAPANESE,
		LANGUAGE_KOREAN
	};
	PY_BEGIN_ENUM_MAP( Language, LANGUAGE_ )
		PY_ENUM_VALUE( LANGUAGE_NON_IME )
		PY_ENUM_VALUE( LANGUAGE_CHS )
		PY_ENUM_VALUE( LANGUAGE_CHT )
		PY_ENUM_VALUE( LANGUAGE_JAPANESE )
		PY_ENUM_VALUE( LANGUAGE_KOREAN )
	PY_END_ENUM_MAP()

	enum State
	{
		STATE_OFF,			// No IME has been enabled
		STATE_ON,			// IME has been enabled
		STATE_ENGLISH		// IME has been enabled, but it is in English mode.
	};
	PY_BEGIN_ENUM_MAP( State, STATE_ )
		PY_ENUM_VALUE( STATE_OFF )
		PY_ENUM_VALUE( STATE_ON )
		PY_ENUM_VALUE( STATE_ENGLISH )
	PY_END_ENUM_MAP()

	typedef BW::vector<BW::wstring> WStringArray;	
	typedef BW::vector<uint8>	AttrArray;

	IME();
	~IME();

	bool initIME( HWND hWnd );
	void update();
	void processEvents( InputHandler & handler );
	bool handleWindowsMessage( HWND hWnd, UINT msg, WPARAM& wParam, LPARAM& lParam, LRESULT& result );
	void onInputLangChange();

	void enabled( bool enable, bool finalise=true );
	bool enabled();	

	void allowEnable( bool allow );

	bool currentlyVisible() const
	{
		return !composition_.empty() || 
				candidatesVisible_ || 
				readingVisible_;
	}

	Language language() { return language_; }	
	State state() { return state_; }	
	BW::wstring composition() { return composition_; }	
	const AttrArray& compositionAttr() { return compositionAttr_; }
	AttrArray& compositionAttrNonConst() { return compositionAttr_; }
	int compositionCursorPosition() { return compositionCursorPosition_; }
	const BW::wstring& reading() { return reading_; }
	bool readingVisible() { return readingVisible_; }
	bool readingVertical() { return readingVertical_; }
	const WStringArray& candidates() { return candidates_; }	
	WStringArray& candidatesNonConst() { return candidates_; }
	bool candidatesVisible() { return candidatesVisible_; }	
	bool candidatesVertical() { return candidatesVertical_; }	
	int selectedCandidate() { return selectedCandidate_; }

	HWND hWnd() const { return hwnd_; }

private:

	void pullCurrentState();

	static void CALLBACK imeCharHandler( WCHAR ch );

private:
	HWND hwnd_;

	bool allowEnable_;
	bool enabled_;

	IMEEvent event_;

	State state_;
	Language language_;

	BW::wstring composition_;
	AttrArray compositionAttr_;
	int compositionCursorPosition_;

	bool readingVisible_, readingVertical_;
	BW::wstring reading_;

	bool candidatesVisible_, candidatesVertical_;
	WStringArray candidates_;
	int selectedCandidate_;
};


PY_ENUM_CONVERTERS_DECLARE( IME::Language )
PY_ENUM_CONVERTERS_DECLARE( IME::State )

BW_END_NAMESPACE

#endif // BW_IME__HPP
