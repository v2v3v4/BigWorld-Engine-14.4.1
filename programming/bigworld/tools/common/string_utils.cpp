/**
 *	StringUtils: String utility methods.
 */

#include "pch.hpp"
#include "cstdmf/cstdmf_windows.hpp"
#include "Shlwapi.h"
#include <algorithm>
#include "cstdmf/bw_vector.hpp"

#include "string_utils.hpp"

#include "cstdmf/bw_string.hpp"
#include "cstdmf/debug.hpp"
#include "cstdmf/guard.hpp"

#include "resmgr/string_provider.hpp"

DECLARE_DEBUG_COMPONENT( 0 );

BW_BEGIN_NAMESPACE


//------------------------------------------------------------------------------
bool StringUtils::PathMatchSpecT( const char * pszFileParam,
	const char * pszSpec )
{
	BW_GUARD;

	return PathMatchSpecA( pszFileParam, pszSpec ) == TRUE;
}

//------------------------------------------------------------------------------
bool StringUtils::PathMatchSpecT( const wchar_t * pszFileParam,
	const wchar_t * pszSpec )
{
	BW_GUARD;

	return PathMatchSpecW( pszFileParam, pszSpec ) == TRUE;
}


//------------------------------------------------------------------------------
void StringUtils::incrementT( BW::string & str,
	StringUtils::IncrementStyle incrementStyle )
{
	BW_GUARD;

	//
	// For IS_EXPLORER the incrementation goes like:
	//
	//      original string
	//      Copy of original string
	//      Copy (2) of original string
	//      Copy (3) of original string
	//      ...
	//
	if (incrementStyle == IS_EXPLORER)
	{
		// Handle the degenerate case:
		if (str.empty())
			return;

		// See if the string starts with "Copy of ".  If it does then the result
		// will be "Copy (2) of" remainder.
		BW::string prefix = LocaliseUTF8(L"COMMON/STRING_UTILS/COPY_OF" );

		if (str.substr(0, prefix.size()) == prefix)
		{
			BW::string remainder = str.substr(prefix.size(), BW::string::npos);
			str = LocaliseUTF8(L"COMMON/STRING_UTILS/COPY_OF_N", 2, remainder );
			return;
		}

		// See if the string starts with "Copy (n) of " where n is a number.
		// If it does then the result will be "Copy (n + 1) of " remainder.
		prefix = LocaliseUTF8(L"COMMON/STRING_UTILS/COPY");
		if (str.substr(0, prefix.size()) == prefix)
		{
			size_t       pos = 6;
			unsigned int num = 0;
			while (pos < str.length() && ::isdigit(str[pos]))
			{
				num *= 10;
				num += str[pos] - '0';
				++pos;
			}
			++num;
			if (pos < str.length())
			{
				BW::string suffix = LocaliseUTF8( L"COMMON/STRING_UTILS/OF" );
				if (str.substr(pos, suffix.size()) == suffix)
				{
					BW::string remainder = str.substr(pos + suffix.size(),
						BW::string::npos);
					str = LocaliseUTF8( L"COMMON/STRING_UTILS/COPY_OF_N",
						num, remainder );
					return;
				}
			}
		}

		// The result must be "Copy of " str.
		str = LocaliseUTF8(L"COMMON/STRING_UTILS/COPY_OF") + ' ' + str;
		return;
	}
	//
	// For IS_END the incrementation goes like:
	//
	//      original string
	//      original string 2
	//      original string 3
	//      original string 4
	//      ...
	//
	// or, if the orignal string is "original string(0)":
	//
	//      original string(0)
	//      original string(1)
	//      original string(2)
	//      ...
	//
	else if (incrementStyle == IS_END)
	{
		if (str.empty())
			return;

		char lastChar    = str[str.length() - 1];
		bool hasLastChar = ::isdigit(lastChar) == 0;

		// Find the position of the ending number and where it begins:
		int pos = (int)str.length() - 1;
		if (hasLastChar)
			--pos;
		unsigned int count = 0;
		unsigned int power = 1;
		bool hasDigits = false;

		for (;pos >= 0 && ::isdigit(str[pos]) != 0; --pos)
		{
			count += power*(str[pos] - '0');
			power *= 10;
			hasDigits = true;
		}

		// Case where there was no number:
		if (!hasDigits)
		{
			count = 1;
			++pos;
			hasLastChar = false;
		}

		// Increment the count:
		++count;

		// Construct the result:
		BW::stringstream stream;
		BW::string prefix = str.substr(0, pos + 1);
		stream << prefix.c_str();
		if (!hasDigits)
			stream << ' ';
		stream << count;
		if (hasLastChar)
			stream << lastChar;
		str = stream.str();
		return;
	}
	else
	{
		return;
	}
}

//------------------------------------------------------------------------------
void StringUtils::incrementT( BW::wstring & str,
	StringUtils::IncrementStyle incrementStyle )
{
	BW_GUARD;

	//
	// For IS_EXPLORER the incrementation goes like:
	//
	//      original string
	//      Copy of original string
	//      Copy (2) of original string
	//      Copy (3) of original string
	//      ...
	//
	if (incrementStyle == IS_EXPLORER)
	{
		// Handle the degenerate case:
		if (str.empty())
			return;

		// See if the string starts with "Copy of ".  If it does then the result
		// will be "Copy (2) of" remainder.
		BW::wstring prefix = Localise( L"COMMON/STRING_UTILS/COPY_OF" );

		if (str.substr(0, prefix.size()) == prefix)
		{
			BW::wstring remainder = str.substr( prefix.size(),
				BW::wstring::npos);
			str = Localise( L"COMMON/STRING_UTILS/COPY_OF_N", 2, remainder );
			return;
		}

		// See if the string starts with "Copy (n) of " where n is a number.
		// If it does then the result will be "Copy (n + 1) of " remainder.
		prefix = Localise( L"COMMON/STRING_UTILS/COPY" );

		if (str.substr(0, prefix.size()) == prefix)
		{
			size_t       pos = 6;
			unsigned int num = 0;

			while (pos < str.length() && ::isdigit( str[pos] ))
			{
				num *= 10;
				num += str[pos] - '0';
				++pos;
			}
			++num;

			if (pos < str.length())
			{
				BW::wstring suffix = Localise( L"COMMON/STRING_UTILS/OF" );

				if (str.substr(pos, suffix.size()) == suffix)
				{
					BW::wstring remainder = str.substr( pos + suffix.size(),
						BW::wstring::npos );
					str = Localise( L"COMMON/STRING_UTILS/COPY_OF_N",
						num, remainder );
					return;
				}
			}
		}

		// The result must be "Copy of " str.
		str = Localise(L"COMMON/STRING_UTILS/COPY_OF") + L' ' + str;
		return;
	}
	//
	// For IS_END the incrementation goes like:
	//
	//      original string
	//      original string 2
	//      original string 3
	//      original string 4
	//      ...
	//
	// or, if the orignal string is "original string(0)":
	//
	//      original string(0)
	//      original string(1)
	//      original string(2)
	//      ...
	//
	else if (incrementStyle == IS_END)
	{
		if (str.empty())
			return;

		wchar_t lastChar    = str[str.length() - 1];
		bool hasLastChar = ::isdigit(lastChar) == 0;

		// Find the position of the ending number and where it begins:
		int pos = (int)str.length() - 1;
		if (hasLastChar)
			--pos;
		unsigned int count = 0;
		unsigned int power = 1;
		bool hasDigits = false;
		for (;pos >= 0 && ::isdigit(str[pos]) != 0; --pos)
		{
			count += power*(str[pos] - '0');
			power *= 10;
			hasDigits = true;
		}

		// Case where there was no number:
		if (!hasDigits)
		{
			count = 1;
			++pos;
			hasLastChar = false;
		}

		// Increment the count:
		++count;

		// Construct the result:
		BW::wstringstream stream;
		BW::wstring prefix = str.substr(0, pos + 1);
		stream << prefix.c_str();
		if (!hasDigits)
			stream << ' ';
		stream << count;
		if (hasLastChar)
			stream << lastChar;
		str = stream.str();
		return;
	}
	else
	{
		return;
	}
}

//------------------------------------------------------------------------------
bool StringUtils::makeValidFilenameT( BW::string & str, char replaceChar,
	bool allowSpaces )
{
	BW_GUARD;

	static char const *NOT_ALLOWED         = "/<>?\\|*:";
	static char const *NOT_ALLOWED_NOSPACE = "/<>?\\|*: ";

	bool changed = false; // Were any changes made?

	// Remove leading whitespace:
	while (!str.empty() && ::isspace(str[0]))
	{
		changed = true;
		str.erase(str.begin() + 0);
	}

	// Remove trailing whitespace:
	while (!str.empty() && ::isspace(str[str.length() - 1]))
	{
		changed = true;
		str.erase(str.begin() + str.length() - 1);
	}

	// Handle the degenerate case:
	if (str.empty())
	{
		str = replaceChar;
		return false;
	}
	else
	{
		// Look for and replace characters that are not allowed:
		BW::string::size_type pos = BW::string::npos;
		do
		{
			if (allowSpaces)
				pos = str.find_first_of(NOT_ALLOWED);
			else
				pos = str.find_first_of(NOT_ALLOWED_NOSPACE);
			if (pos != BW::string::npos)
			{
				str[pos] = replaceChar;
				changed = true;
			}
		}
		while (pos != BW::string::npos);
		return !changed;
	}
}


//------------------------------------------------------------------------------
bool StringUtils::makeValidFilenameT( BW::wstring & str, wchar_t replaceChar,
	bool allowSpaces )
{
	BW_GUARD;

	static wchar_t const *NOT_ALLOWED         = L"/<>?\\|*:";
	static wchar_t const *NOT_ALLOWED_NOSPACE = L"/<>?\\|*: ";

	bool changed = false; // Were any changes made?

	// Remove leading whitespace:
	while (!str.empty() && ::isspace(str[0]))
	{
		changed = true;
		str.erase(str.begin() + 0);
	}

	// Remove trailing whitespace:
	while (!str.empty() && ::isspace(str[str.length() - 1]))
	{
		changed = true;
		str.erase(str.begin() + str.length() - 1);
	}

	// Handle the degenerate case:
	if (str.empty())
	{
		str = replaceChar;
		return false;
	}
	else
	{
		// Look for and replace characters that are not allowed:
		BW::string::size_type pos = BW::wstring::npos;
		do
		{
			if (allowSpaces)
				pos = str.find_first_of(NOT_ALLOWED);
			else
				pos = str.find_first_of(NOT_ALLOWED_NOSPACE);
			if (pos != BW::wstring::npos)
			{
				str[pos] = replaceChar;
				changed = true;
			}
		}
		while (pos != BW::wstring::npos);
		return !changed;
	}
}

BW_END_NAMESPACE
// string_utils.cpp
