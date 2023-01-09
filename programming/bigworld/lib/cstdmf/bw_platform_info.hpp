#pragma once
#ifndef BW_PLATFORM_INFO_HPP
#define BW_PLATFORM_INFO_HPP

#include "bw_namespace.hpp"
#include "bw_string.hpp"


BW_BEGIN_NAMESPACE

class PlatformInfo
{
public:
	CSTDMF_DLL static const BW::string & buildStr();
	CSTDMF_DLL static const BW::string & str();

private:
	static bool parseRedHatRelease( BW::string & shortPlatformName );
};

BW_END_NAMESPACE

#endif // BW_PLATFORM_INFO_HPP
