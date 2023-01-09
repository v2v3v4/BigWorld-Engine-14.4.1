#include "pch.hpp"
#include "bw_string_ref.hpp"

namespace BW
{

template <>
const char StringRef::s_nullTerminator_ = '\0';

template <>
const wchar_t WStringRef::s_nullTerminator_ = L'\0';

template <>
const StringRef::size_type StringRef::npos = ~StringRef::size_type(0);

template <>
const WStringRef::size_type WStringRef::npos = ~WStringRef::size_type(0);

} // namespace BW

