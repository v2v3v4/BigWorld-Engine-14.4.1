#ifndef BWVERSION_HPP
#define BWVERSION_HPP

#define BW_VERSION_MAJOR 14
#define BW_VERSION_MINOR 4
#define BW_VERSION_PATCH 1

#define BW_COPYRIGHT_NOTICE "(c) 1999 - 2014, BigWorld Pty. Ltd. All Rights Reserved"

#define STRINGIFY(X) #X
#define TOSTRING(X) STRINGIFY(X)

#define BW_VERSION_MSVC_RC BW_VERSION_MAJOR, BW_VERSION_MINOR, BW_VERSION_PATCH
#define BW_VERSION_MSVC_RC_STRING TOSTRING( BW_VERSION_MAJOR.BW_VERSION_MINOR.BW_VERSION_PATCH )

#ifndef RC_INVOKED


#include "stdmf.hpp"
#include "bw_string.hpp"

BW_BEGIN_NAMESPACE

namespace BWVersion
{
	CSTDMF_DLL uint16 majorNumber();
	CSTDMF_DLL uint16 minorNumber();
	CSTDMF_DLL uint16 patchNumber();
	
	CSTDMF_DLL void majorNumber( uint16 number );
	CSTDMF_DLL void minorNumber( uint16 number );
	CSTDMF_DLL void patchNumber( uint16 number );

	CSTDMF_DLL const BW::string & versionString();
}

BW_END_NAMESPACE

#endif // RC_INVOKED

#endif // BWVERSION_HPP
