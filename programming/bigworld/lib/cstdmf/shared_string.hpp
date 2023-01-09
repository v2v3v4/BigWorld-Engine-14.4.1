#ifndef SHARED_STRING_HPP
#define SHARED_STRING_HPP

#include "bw_namespace.hpp"
#include "concurrency.hpp"
#include "cstdmf_dll.hpp"
#include "stringmap.hpp"

BW_BEGIN_NAMESPACE

class SharedString
{
public:
	typedef StringHashMap< int > StringMapType;

	CSTDMF_DLL SharedString( const SharedString & other );
	CSTDMF_DLL SharedString( const BW::string & string = s_EmptyString );
	CSTDMF_DLL ~SharedString();

	CSTDMF_DLL const BW::string & getString() const;
	CSTDMF_DLL const SharedString & operator = ( const SharedString & other );
	CSTDMF_DLL const SharedString & operator = ( const BW::string & other );

private:
	const BW::string * string_;
	static StringMapType s_SharedStrings;
	static SimpleMutex s_Mutex;
	static size_t s_SavedMemory;
	CSTDMF_DLL static const BW::string s_EmptyString;

	void unassign();
	void assign( const BW::string & string );
};

BW_END_NAMESPACE

#endif
