#ifndef BW_STRING_HPP
#define BW_STRING_HPP

#include "cstdmf/cstdmf_dll.hpp"
#include "cstdmf/stl_fixed_sized_allocator.hpp"

#include <sstream>
#include <string>

namespace BW
{

typedef StlAllocator< char > char_allocator;
typedef StlAllocator< wchar_t > wchar_allocator;

} // namespace BW

// MSVC 2012 can't export basic_string< char, ...> or basic_string<wchar_t,...>
// from a DLL if the main application also instantiates such a template, as
// they will share a common base class std::_String_val< _Simple_types< char > >
// and std::_String_val< _Simple_types< wchar > > respectively.
#if !defined(_MSC_VER) || (_MSC_VER < 1700)
// Explicit instantiation of our std::basic_string-related specialisations in
// the std namespace, as required by 14.7.3 [temp.expl.spec] paragraph 2 of
// C++03. N3867 proposes to remove this namespace restriction.
namespace std
{

CSTDMF_EXPORTED_TEMPLATE_CLASS basic_string< char, char_traits< char >,
	BW::char_allocator >;
CSTDMF_EXPORTED_TEMPLATE_CLASS basic_istringstream< char, char_traits< char >,
	BW::char_allocator >;
CSTDMF_EXPORTED_TEMPLATE_CLASS basic_ostringstream< char, char_traits< char >,
	BW::char_allocator >;
CSTDMF_EXPORTED_TEMPLATE_CLASS basic_stringstream< char, char_traits< char >,
	BW::char_allocator >;

CSTDMF_EXPORTED_TEMPLATE_CLASS basic_string< wchar_t, char_traits< wchar_t >,
	BW::wchar_allocator >;
CSTDMF_EXPORTED_TEMPLATE_CLASS basic_istringstream< wchar_t, char_traits< wchar_t >,
	BW::wchar_allocator >;
CSTDMF_EXPORTED_TEMPLATE_CLASS basic_ostringstream< wchar_t, char_traits< wchar_t >,
	BW::wchar_allocator >;
CSTDMF_EXPORTED_TEMPLATE_CLASS basic_stringstream< wchar_t, char_traits< wchar_t >,
	BW::wchar_allocator >;

} // namespace std
#endif // !defined(_MSC_VER) || (_MSC_VER < 1700)

// Pull our std::basic_string* specialisations into the BW namespace.
namespace BW
{

typedef std::basic_string< char, std::char_traits< char >,
	char_allocator > string;
typedef std::basic_istringstream< char, std::char_traits< char >,
	char_allocator > istringstream;
typedef std::basic_ostringstream< char, std::char_traits< char >,
	char_allocator > ostringstream;
typedef std::basic_stringstream< char, std::char_traits< char >,
	char_allocator > stringstream;

typedef std::basic_string< wchar_t, std::char_traits< wchar_t >,
	wchar_allocator > wstring;
typedef std::basic_istringstream< wchar_t, std::char_traits< wchar_t >,
	wchar_allocator > wistringstream;
typedef std::basic_ostringstream< wchar_t, std::char_traits< wchar_t >,
	wchar_allocator > wostringstream;
typedef std::basic_stringstream< wchar_t, std::char_traits< wchar_t >,
	wchar_allocator > wstringstream;

} // namespace BW

#if defined( __GNUC__ ) || (_MSC_VER >= 1600) || defined( __clang__ )
// Modern compilers require template specialisations of str::tr1::hash
// for BW::string and BW::wstring

#include "bw_hash.hpp"

#include <cstddef>

BW_HASH_NAMESPACE_BEGIN

template<>
struct hash<BW::string> : public std::unary_function<BW::string, std::size_t>
{
	std::size_t operator()(const BW::string& s) const
	{
		return BW::hash_string( s.data(), s.size() );
	}
};

template<>
struct hash<BW::wstring> : public std::unary_function<BW::wstring, std::size_t>
{
	std::size_t operator()(const BW::wstring& s) const
	{
		return BW::hash_string( s.data(), s.size() * sizeof(wchar_t) );
	}
};

BW_HASH_NAMESPACE_END

#endif // __GNUC__ || (_MSC_VER >= 1600) || __clang__

#endif // BW_STRING_HPP
