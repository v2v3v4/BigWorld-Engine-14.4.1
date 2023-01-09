#ifndef _SANITISE_HELPER_HPP_
#define _SANITISE_HELPER_HPP_

#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_string_ref.hpp"
#include "cstdmf/bw_namespace.hpp"

BW_BEGIN_NAMESPACE

class SanitiseHelper
{
public:
	// Sanitising token
	static const BW::string SANITISING_TOKEN;	// = ".."
	static const BW::string SPACE_TOKEN;	// = " "

	static BW::string substringReplace
		(
		 	const BW::StringRef& val,
		 	const BW::string& oldSubstr = SPACE_TOKEN,
			const BW::string& newSubstr = SANITISING_TOKEN
		);
};

BW_END_NAMESPACE

#endif // _SANITISE_HELPER_HPP_
