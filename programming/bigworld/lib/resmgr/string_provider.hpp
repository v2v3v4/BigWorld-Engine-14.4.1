#ifndef STRING_PROVIDER_HPP__
#define STRING_PROVIDER_HPP__

#include "datasection.hpp"
#include "cstdmf/smartpointer.hpp"
#include "cstdmf/string_utils.hpp"
#include "cstdmf/bw_namespace.hpp"
#include "cstdmf/bw_string.hpp"
#include <cstring>
#include <stdio.h>
#include <sstream>
#include "cstdmf/bw_vector.hpp"
#include "cstdmf/bw_set.hpp"
#include "cstdmf/bw_map.hpp"
#include "cstdmf/bw_stack_container.hpp"


BW_BEGIN_NAMESPACE

namespace
{
	static const int MAX_LOCALISED_STRING_LENGTH = 10240;
}

class Formatter
{
	wchar_t str_[ 1024 ];

public:
	Formatter()
	{
		wmemset( str_, L'\0', 1024 );
	}

	Formatter( const BW::wstring& str )
	{
		MF_ASSERT( str.size() <= ARRAY_SIZE( str_ ) );
		wcsncpy( str_, str.c_str(), ARRAY_SIZE( str_ ) );
	}

	Formatter( const wchar_t* str )
	{
		MF_ASSERT( wcslen( str ) <= ARRAY_SIZE( str_ ) );
		wcsncpy( str_, str, ARRAY_SIZE( str_ ) );
	}

	Formatter( const BW::string& str )	
	{
		MF_ASSERT( str.size() <= ARRAY_SIZE( str_ ) );
		BW::WStackString< ARRAY_SIZE( str_ ) > stackString;
		bw_utf8tow( str, stackString.container() );
		wcsncpy( str_, stackString->c_str(), ARRAY_SIZE( str_ ) );
	}
	Formatter( const char* str )
	{
		MF_ASSERT( strlen( str ) <= ARRAY_SIZE( str_ ) );
		BW::StackString< ARRAY_SIZE( str_ ) > stackString;
		stackString->append( str );
		BW::WStackString< ARRAY_SIZE( str_ ) > wStackString;
		// assume utf8, if not, it *should* be in ASCII, which is utf8-safe.
		bw_utf8tow( stackString.container(), wStackString.container() );
		wcsncpy( str_, wStackString->c_str(), ARRAY_SIZE( str_ ) );
	}
	Formatter( float f, const wchar_t* format = L"%g" )
	{
		bw_snwprintf( str_, ARRAY_SIZE( str_ ), format, f );
		str_[ ARRAY_SIZE( str_ ) - 1 ] = 0;
	}
	Formatter( double d, const wchar_t* format = L"%g" )
	{
		bw_snwprintf( str_, ARRAY_SIZE( str_ ), format, d );
		str_[ ARRAY_SIZE( str_ ) - 1 ] = 0;
	}
	Formatter( int i, const wchar_t* format = L"%d" )
	{
		bw_snwprintf( str_, ARRAY_SIZE( str_ ), format, i );
		str_[ ARRAY_SIZE( str_ ) - 1 ] = 0;
	}
	Formatter( unsigned int ui, const wchar_t* format = L"%u" )
	{
		bw_snwprintf( str_, ARRAY_SIZE( str_ ), format, ui );
		str_[ ARRAY_SIZE( str_ ) - 1 ] = 0;
	}
	Formatter( unsigned long ul, const wchar_t* format = L"%u" )
	{
		bw_snwprintf( str_, ARRAY_SIZE( str_ ), format, ul );
		str_[ ARRAY_SIZE( str_ ) - 1 ] = 0;
	}
	Formatter( char ch, const wchar_t* format = L"%c" )
	{
		bw_snwprintf( str_, ARRAY_SIZE( str_ ), format, ch );
		str_[ ARRAY_SIZE( str_ ) - 1 ] = 0;
	}
	Formatter( unsigned char ch, const wchar_t* format = L"%c" )
	{
		bw_snwprintf( str_, ARRAY_SIZE( str_ ), format, ch );
		str_[ ARRAY_SIZE( str_ ) - 1 ] = 0;
	}
	Formatter( void* p, const wchar_t* format = L"%p" )
	{
		bw_snwprintf( str_, ARRAY_SIZE( str_ ), format, p );
		str_[ ARRAY_SIZE( str_ ) - 1 ] = 0;
	}

#if defined(_WIN64)
	Formatter( size_t ui, const wchar_t* format = L"%u" )
	{
		bw_snwprintf( str_, ARRAY_SIZE( str_ ), format, (unsigned long)ui );
		str_[ ARRAY_SIZE( str_ ) - 1 ] = 0;
	}
#endif
	const wchar_t * str() const {	return str_;	}
};


// for index out of the range, set it to empty string
inline void formatString( const wchar_t* format, BW::wstring & o_Result, const Formatter& f1 = Formatter(), const Formatter& f2 = Formatter(),
						 const Formatter& f3 = Formatter(), const Formatter& f4 = Formatter(),
						 const Formatter& f5 = Formatter(), const Formatter& f6 = Formatter(),
						 const Formatter& f7 = Formatter(), const Formatter& f8 = Formatter() )
{
	const wchar_t * strs[ 8 ];
	strs[ 0 ] = f1.str();	strs[ 1 ] = f2.str();	strs[ 2 ] = f3.str();	strs[ 3 ] = f4.str();
	strs[ 4 ] = f5.str();	strs[ 5 ] = f6.str();	strs[ 6 ] = f7.str();	strs[ 7 ] = f8.str();
	while( const wchar_t* escape = std::wcschr( format, L'%' ) )
	{
		o_Result.append( format, escape - format );
		format = escape;
		if( format[1] == 0 )
		{
			++format;
			break;
		}
		if( format[1] == L'%' )
		{
			o_Result += L'%';
			++format;
		}
		else if( format[1] >= L'0' && format[1] <= L'7' )
		{
			o_Result += strs[ format[1] - L'0' ];
			++format;
		}
		/*
		else
			0;// wrong escape ==> empty strings
		*/
		++format;
	}
	o_Result += format;
}

#define LANGUAGE_NAME_TAG ( L"LanguageName" )
#define ENGLISH_LANGUAGE_NAME_TAG ( L"EnglishLanguageName" )
#define DEFAULT_LANGUAGE_NAME ( L"en" )
#define DEFAULT_COUNTRY_NAME ( L"us" )

class StringID
{
public:
	StringID() : key_(0) {}
	//explicit StringID( const char* str );
	explicit StringID( const wchar_t* str );
	unsigned int key() const	{	return key_;	}
	const BW::wstring& str() const	{	return str_;	}
	bool operator<( const StringID& that ) const;// should be a free function, but ...
private:
	BW::wstring str_;
	unsigned int key_;
};

class Language : public SafeReferenceCount
{
public:
	virtual void load( DataSectionPtr language, const BW::wstring& root = L"" ) = 0;
	virtual const wchar_t* getLanguageName() const = 0;
	virtual const wchar_t* getLanguageEnglishName() const = 0;
	virtual const BW::wstring& getIsoLangName() const = 0;
	virtual const BW::wstring& getIsoCountryName() const = 0;
	virtual const wchar_t* getString( const StringID& id ) const = 0;
	static std::pair<BW::wstring, BW::wstring> splitIsoLangCountryName( const BW::wstring& isoLangCountryName );
	static const int ISO_NAME_LENGTH = 2;

	inline BW::string getIsoLangNameUTF8() const
	{
		BW::string langName;
		bw_wtoutf8( getIsoLangName(), langName );
		return langName;
	}
	inline BW::string getIsoCountryNameUTF8() const
	{
		BW::string countryName;
		bw_wtoutf8( getIsoCountryName(), countryName );
		return countryName;
	}
};

typedef SmartPointer<Language> LanguagePtr;

struct LanguageNotifier
{
	LanguageNotifier();
	virtual ~LanguageNotifier();
	virtual void changed() = 0;
};

/**
	Localised String Provider, after setting to the appropriate language/country
	You can call localised string provider with an id to get back a string
 */

class StringProvider
{
	BW::set<LanguageNotifier*> notifiers_;
	BW::vector<LanguagePtr> languages_;
	LanguagePtr currentLanguage_;
	LanguagePtr currentMainLanguage_;
	LanguagePtr defaultLanguage_;

	const wchar_t* str( const StringID& id ) const;
	void formatString( const StringID& formatID, BW::wstring & o_OutputString, const Formatter& f1 = Formatter(), const Formatter& f2 = Formatter(),
						 const Formatter& f3 = Formatter(), const Formatter& f4 = Formatter(),
						 const Formatter& f5 = Formatter(), const Formatter& f6 = Formatter(),
						 const Formatter& f7 = Formatter(), const Formatter& f8 = Formatter() )
	{
		::BW_NAMESPACE formatString( str( formatID ), o_OutputString, f1, f2, f3, f4, f5, f6, f7, f8 );
	}
public:
	enum DefResult
	{
		RETURN_NULL_IF_NOT_EXISTING,
		RETURN_PARAM_IF_NOT_EXISTING
	};


	void load( DataSectionPtr file );
	unsigned int languageNum() const;
	LanguagePtr getLanguage( unsigned int index ) const;

	void setLanguage();
	void setLanguage( unsigned int language );
	void setLanguages( const BW::wstring& langName, const BW::wstring& countryName );

	const wchar_t* str( const wchar_t* id, DefResult def = RETURN_PARAM_IF_NOT_EXISTING ) const;

	void formatString( const wchar_t* formatID, BW::wstring & o_OutputString, const Formatter& f1 = Formatter(), const Formatter& f2 = Formatter(),
						 const Formatter& f3 = Formatter(), const Formatter& f4 = Formatter(),
						 const Formatter& f5 = Formatter(), const Formatter& f6 = Formatter(),
						 const Formatter& f7 = Formatter(), const Formatter& f8 = Formatter() )
	{
		::BW_NAMESPACE formatString( str( formatID ), o_OutputString, f1, f2, f3, f4, f5, f6, f7, f8 );
	}


	LanguagePtr currentLanguage() const;

	static StringProvider& instance();

	void registerNotifier( LanguageNotifier* notifier );
	void unregisterNotifier( LanguageNotifier* notifier );
	void notify();
};

inline void formatLocalisedString( const wchar_t* format, BW::wstring & o_OutputString, const Formatter& f1, const Formatter& f2 = Formatter(),
						 const Formatter& f3 = Formatter(), const Formatter& f4 = Formatter(),
						 const Formatter& f5 = Formatter(), const Formatter& f6 = Formatter(),
						 const Formatter& f7 = Formatter(), const Formatter& f8 = Formatter() )
{
	StringProvider::instance().formatString( format, o_OutputString, f1, f2, f3, f4, f5, f6, f7, f8 );
}

inline const wchar_t* Localise( const wchar_t* key, StringProvider::DefResult def = StringProvider::RETURN_PARAM_IF_NOT_EXISTING )
{
	return StringProvider::instance().str( key, def );
}

//-----------------------------------------------------------------------------
inline const wchar_t* Localise( const wchar_t* format, const Formatter& f1, const Formatter& f2 = Formatter(),
						 const Formatter& f3 = Formatter(), const Formatter& f4 = Formatter(),
						 const Formatter& f5 = Formatter(), const Formatter& f6 = Formatter(),
						 const Formatter& f7 = Formatter(), const Formatter& f8 = Formatter() )
{
	WStackString< MAX_LOCALISED_STRING_LENGTH > stackString;
	formatLocalisedString( format, stackString.container(), f1, f2, f3, f4, f5, f6, f7, f8 );

#ifdef WIN32
	__declspec(thread) static wchar_t sz[ MAX_LOCALISED_STRING_LENGTH + 1 ];
#else//WIN32
	static wchar_t sz[ MAX_LOCALISED_STRING_LENGTH + 1 ];
#endif//WIN32
	std::wcsncpy( sz, stackString->c_str(), MAX_LOCALISED_STRING_LENGTH );
	sz[ MAX_LOCALISED_STRING_LENGTH ] = 0;
	return sz;
}


inline void LocaliseUTF8( BW::string & o_OutputString, const wchar_t* format, const Formatter& f1, const Formatter& f2 = Formatter(),
					const Formatter& f3 = Formatter(), const Formatter& f4 = Formatter(),
					const Formatter& f5 = Formatter(), const Formatter& f6 = Formatter(),
					const Formatter& f7 = Formatter(), const Formatter& f8 = Formatter() ) 
{
	WStackString< MAX_LOCALISED_STRING_LENGTH > stackString;
	formatLocalisedString( format, stackString.container(), f1, f2, f3, f4, f5, f6, f7, f8 );
	bw_wtoutf8( stackString.container(), o_OutputString );
}


void LocaliseUTF8(
	BW::wstring & o_OutputString, const wchar_t* format,
	const Formatter& f1, const Formatter& f2 = Formatter(),
	const Formatter& f3 = Formatter(), const Formatter& f4 = Formatter(),
	const Formatter& f5 = Formatter(), const Formatter& f6 = Formatter(),
	const Formatter& f7 = Formatter(), const Formatter& f8 = Formatter() );

inline BW::string LocaliseUTF8( const wchar_t* format, const Formatter& f1, const Formatter& f2 = Formatter(),
								const Formatter& f3 = Formatter(), const Formatter& f4 = Formatter(),
								const Formatter& f5 = Formatter(), const Formatter& f6 = Formatter(),
								const Formatter& f7 = Formatter(), const Formatter& f8 = Formatter() )
{
	StackString< MAX_LOCALISED_STRING_LENGTH > localisedString;
	WStackString< MAX_LOCALISED_STRING_LENGTH > wlocalisedString;
	formatLocalisedString( format, wlocalisedString.container(), f1, f2, f3, f4, f5, f6, f7, f8 );
	bw_wtoutf8( wlocalisedString.container(), localisedString.container() );
	return localisedString.container();
}

#ifdef WIN32
inline BW::string LocaliseACP( const wchar_t* format, const Formatter& f1, const Formatter& f2 = Formatter(),
								const Formatter& f3 = Formatter(), const Formatter& f4 = Formatter(),
								const Formatter& f5 = Formatter(), const Formatter& f6 = Formatter(),
								const Formatter& f7 = Formatter(), const Formatter& f8 = Formatter() )
{
	StackString< MAX_LOCALISED_STRING_LENGTH > localisedString;
	WStackString< MAX_LOCALISED_STRING_LENGTH > wlocalisedString;
	formatLocalisedString( format, wlocalisedString.container(), f1, f2, f3, f4, f5, f6, f7, f8 );
	bw_wtoacp( wlocalisedString.container(), localisedString.container() );
	return localisedString.container();
}
#endif

inline void LocaliseUTF8( BW::string & o_OutputString, const wchar_t * key, StringProvider::DefResult def = StringProvider::RETURN_PARAM_IF_NOT_EXISTING )
{
	bw_wtoutf8( StringProvider::instance().str( key, def ), o_OutputString );
}


//-----------------------------------------------------------------------------
void LocaliseUTF8(
	BW::wstring & o_OutputString, const wchar_t * key,
	StringProvider::DefResult def = StringProvider::RETURN_PARAM_IF_NOT_EXISTING );


//-----------------------------------------------------------------------------
// from WIDE key to UTF8/ACP
inline BW::string LocaliseUTF8( const wchar_t * key, StringProvider::DefResult def = StringProvider::RETURN_PARAM_IF_NOT_EXISTING )
{
	BW::string localisedString;
	bw_wtoutf8( StringProvider::instance().str( key, def ), localisedString );
	return localisedString;
}

#ifdef WIN32
inline BW::string LocaliseACP( const wchar_t * key, StringProvider::DefResult def = StringProvider::RETURN_PARAM_IF_NOT_EXISTING )
{
	BW::string localisedString;
	bw_wtoacp( StringProvider::instance().str( key, def ), localisedString );
	return localisedString;
}
#endif

//-----------------------------------------------------------------------------
// from NARROW key to UTF8/ACP
inline BW::string LocaliseUTF8( const char * key, StringProvider::DefResult def = StringProvider::RETURN_PARAM_IF_NOT_EXISTING )
{
	BW::wstring wkey;
	bw_ansitow( key, wkey );
	return LocaliseUTF8( wkey.c_str(), def );
}

#ifdef WIN32
inline BW::string LocaliseACP( const char * key, StringProvider::DefResult def = StringProvider::RETURN_PARAM_IF_NOT_EXISTING )
{
	BW::wstring wkey;
	bw_ansitow( key, wkey );
	return LocaliseACP( wkey.c_str(), def );
}
#endif

inline bool isLocaliseToken( const char* s )
{
	return s && s[0] == '`';
}

inline bool isLocaliseToken( const wchar_t* s )
{
	return s && s[0] == L'`';
}


//-----------------------------------------------------------------------------
// Localise caches for speed.
inline BW::string LocaliseStaticUTF8( const wchar_t * key )
{
	typedef BW::map< const wchar_t *, BW::string > LocaliseCache;
	static LocaliseCache s_cache;

	LocaliseCache::const_iterator it = s_cache.find( key );

	if (it == s_cache.end())
	{
		it = s_cache.insert(
				std::make_pair( key, LocaliseUTF8( key ) ) ).first;
	}

	return (*it).second;
}



#ifdef WIN32

BW_END_NAMESPACE

#include <commctrl.h>

BW_BEGIN_NAMESPACE

class WindowTextNotifier : LanguageNotifier
{
	BW::map<HWND, StringID> windows_;
	BW::map<HWND, BW::vector<StringID> > combos_;
	BW::map<std::pair<HMENU, UINT>, StringID> menus_;
	BW::map<HWND, WNDPROC> subClassMap_;
	HHOOK callWinRetHook_;
	HHOOK callWinHook_;
	WNDPROC comboWndProc_;
	WNDPROC toolTipProc_;
	wchar_t localisedText_[MAX_LOCALISED_STRING_LENGTH];
public:
	WindowTextNotifier();
	~WindowTextNotifier();

	void set( HWND hwnd, const wchar_t* id );

	void set( HMENU menu );

	void addComboString( HWND hwnd, const wchar_t* id );
	void deleteComboString( HWND hwnd, BW::vector<StringID>::size_type index );
	void insertComboString( HWND hwnd, BW::vector<StringID>::size_type index, const wchar_t* id );
	void resetContent( HWND hwnd );

	virtual void changed();

	static WindowTextNotifier& instance();
	static void fini();
	static LRESULT CALLBACK CallWndRetProc( int nCode, WPARAM wParam, LPARAM lParam );
	static LRESULT CALLBACK CallWndProc( int nCode, WPARAM wParam, LPARAM lParam );
	static LRESULT CALLBACK ComboProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
	static LRESULT CALLBACK ToolTipProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
	static LRESULT CALLBACK ToolTipParentProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
};

inline void localiseWindowText( HWND hwnd, const wchar_t* id )
{
	WindowTextNotifier::instance().set( hwnd, id );
}

inline void Localise( HWND hwnd, const wchar_t* id )
{
	localiseWindowText( hwnd, id );
}

BW::string localiseFileName( const BW::string& filename,
	LanguagePtr lang = StringProvider::instance().currentLanguage() );


#endif//WIN32

#define STATIC_LOCALISE_NAME( name, key )	static Name name( LocaliseUTF8( key ) )
#define STATIC_LOCALISE_STRING( name, key )	static BW::string name( LocaliseUTF8( key ) )
#define STATIC_LOCALISE_WSTRING( name, key )	static BW::wstring name( Localise( key ) )

BW_END_NAMESPACE

#endif//STRING_PROVIDER_HPP__
