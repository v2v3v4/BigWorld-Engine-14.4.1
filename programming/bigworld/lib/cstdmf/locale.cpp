#include "pch.hpp"

#include "locale.hpp"

BW_BEGIN_NAMESPACE

namespace // anonymous
{

std::locale s_standardC( "C" );

} // end namespace (anonymous)

namespace Locale
{

/**
 *	Return the standard "C" locale.
 */
const std::locale & standardC()
{
	return s_standardC;
}

} // end namespace Locale

BW_END_NAMESPACE

// locale.cpp

