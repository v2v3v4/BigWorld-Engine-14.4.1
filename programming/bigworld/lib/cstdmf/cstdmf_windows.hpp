#ifndef CSTDMF_WINDOWS_HPP
#define CSTDMF_WINDOWS_HPP

#if defined( _WIN32 ) && !defined( _XBOX ) && !defined( _XBOX360 )
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
#endif

#endif // CSTDMF_WINDOWS_HPP
