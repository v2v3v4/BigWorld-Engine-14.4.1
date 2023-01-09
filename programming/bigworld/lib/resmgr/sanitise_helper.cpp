#include "pch.hpp"
#include "sanitise_helper.hpp"

#include "cstdmf/bw_namespace.hpp"

BW_BEGIN_NAMESPACE

/**
 *	The default tokens used to sanitise data sections.
 */
const BW::string SanitiseHelper::SANITISING_TOKEN = "..";
const BW::string SanitiseHelper::SPACE_TOKEN = " ";


/**
 *	Static helper method for replacing tokens in a string.
 *
 * @param val		The string being processed
 * @param oldSubstr	The substring being searched for
 * @param newSubstr	The replacing substring
 * @return			The string after processing
 */
/*static*/ BW::string SanitiseHelper::substringReplace
	(
		const BW::StringRef& val,
		const BW::string& oldSubstr /* = SPACE_TOKEN */,
		const BW::string& newSubstr /* = SANITISING_TOKEN */
	)
{
	// Check if we can early out
	size_t valSize = val.size();
	size_t oldSubstrSize = oldSubstr.size();
	if ( valSize < oldSubstrSize )
		return val.to_string();

	BW::string result( val.data(), val.size() );

	for
		(
			BW::string::size_type pos = result.find(oldSubstr);
			pos != BW::string::npos;
			pos = result.find(oldSubstr)
		)
	{
		result.replace(pos, oldSubstrSize, newSubstr);
	}

	return result;
}

BW_END_NAMESPACE

// sanitise_helper.cpp
