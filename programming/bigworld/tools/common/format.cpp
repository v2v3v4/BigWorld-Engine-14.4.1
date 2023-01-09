#include "pch.hpp"
#include "format.hpp"



BW_BEGIN_NAMESPACE

#ifdef _MFC_VER

BW::string loadString(UINT resid)
{
	BW_GUARD;

    CStringA s;
    s.LoadString(resid);
    return s.GetBuffer();
}

BW::wstring wloadString(UINT resid)
{
	BW_GUARD;

    CStringW s;
    s.LoadString(resid);
    return s.GetBuffer();
}

#endif // _MFC_VER

bool
details::sformatNextObj
(
    BW::string             &result,
    BW::string             const &str,
    size_t             &pos,
    size_t             &idx
)
{
	BW_GUARD;

    while (pos < str.length())
    {
        if (str[pos] == '{')
        {
            // Is it a number?
            if (pos + 1 < str.length() && ::isdigit(str[pos + 1]))
            {
                ++pos;
                idx = 0;
                while (pos < str.length() && ::isdigit(str[pos]))
                {
                    idx = idx*10 + (str[pos] -'0');
                    ++pos;
                }
                if (pos < str.length() && str[pos] == '}')
                    ++pos;
                return true; // there is more to do
            }
        }
        // double "{", print a single "{":
        if 
        (
            str[pos] =='{'
            &&
            pos + 1 < str.length() 
            && 
            str[pos + 1] == '{'
        )
        {
            result += '{';
            pos += 2;
        }
        // double "}", print a single "}":
        else if 
        (
            str[pos] == '}'
            &&
            pos + 1 < str.length() 
            && 
            str[pos + 1] == '}'
        )
        {
            result += '}';
            pos += 2;
        }
        // Normal character:
        else
        {
            result += str[pos];
            ++pos;
        }
    }
    return false; // we reached the end of the format string
}

bool
details::wformatNextObj
(
    BW::wstring            &result,
    BW::wstring            const &str,
    size_t             &pos,
    size_t             &idx
)
{
	BW_GUARD;

    while (pos < str.length())
    {
        if (str[pos] == L'{')
        {
            // Is it a number?
            if (pos + 1 < str.length() && ::iswdigit(str[pos + 1]))
            {
                ++pos;
                idx = 0;
                while (pos < str.length() && ::iswdigit(str[pos]))
                {
                    idx = idx*10 + (str[pos] - L'0');
                    ++pos;
                }
                if (pos < str.length() && str[pos] == L'}')
                    ++pos;
                return true; // there is more to do
            }
        }
        // double "{", print a single "{":
        if 
        (
            str[pos] == L'{'
            &&
            pos + 1 < str.length() 
            && 
            str[pos + 1] == L'{'
        )
        {
            result += L'{';
            pos += 2;
        }
        // double "}", print a single "}":
        else if 
        (
            str[pos] == L'}'
            &&
            pos + 1 < str.length() 
            && 
            str[pos + 1] == L'}'
        )
        {
            result += L'}';
            pos += 2;
        }
        // Normal character:
        else
        {
            result += str[pos];
            ++pos;
        }
    }
    return false; // we reached the end of the format string
}

BW_END_NAMESPACE
