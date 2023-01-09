/**
 *	FILTER_SPEC: filters text according to its include/exclude rules
 */

#include "pch.hpp"
#include <algorithm>
#include "cstdmf/bw_string.hpp"
#include "cstdmf/guard.hpp"
#include "filter_spec.hpp"
#include "common/string_utils.hpp"
#include "cstdmf/string_utils.hpp"

BW_BEGIN_NAMESPACE

/**
 *	Constructor.  Parses the include and exclude strings into vectors.
 */
FilterSpec::FilterSpec( const BW::wstring& name, bool active,
					   const BW::wstring& include, const BW::wstring& exclude,
					   const BW::wstring& group ) :
	name_( name ),
	active_( active ),
	enabled_( true ),
	group_( group )
{
	BW_GUARD;

	BW::wstring includeL = include;
	std::replace( includeL.begin(), includeL.end(), L'/', L'\\' );
	bw_tokenise( includeL, L",;", includes_ );

	BW::wstring excludeL = exclude;
	std::replace( excludeL.begin(), excludeL.end(), L'/', L'\\' );
	bw_tokenise( excludeL, L",;", excludes_ );
}


/**
 *	Destructor
 */
FilterSpec::~FilterSpec()
{
	BW_GUARD;
}


/**
 *	This method tests the input string agains the filter, and returns true if
 *	there's a match.
 *
 *	@param str	String to match.
 *	@return		True if the string matches the filter, false otherwise.
 */
bool FilterSpec::filter( const BW::wstring& str )
{
	BW_GUARD;

	if ( !active_ || !enabled_ )
		return true;

	// pass filter test if it's in the includes and not in the excludes.
	return StringUtils::matchSpecT( str, includes_ ) &&
		( excludes_.empty() || !StringUtils::matchSpecT( str, excludes_ ) );
}


/**
 *	This method enables or disables this filter.
 *
 *	@param enable	True to enable this filter, false to disable it.
 */
void FilterSpec::enable( bool enable )
{
	BW_GUARD;

	enabled_ = enable;
}

BW_END_NAMESPACE

