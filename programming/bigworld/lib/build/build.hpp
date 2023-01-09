/**
 * Build revision information, automatically updated by
 * make on every build.
 */

#ifndef BUILD_HPP
#define BUILD_HPP

#include "cstdmf/bw_namespace.hpp"


BW_BEGIN_NAMESPACE

class Build
{
public:
	typedef int Revision;
	static const Revision UNKNOWN_REVISION = -1;
	static Revision revision();
};

BW_END_NAMESPACE

#endif // BUILD_HPP
