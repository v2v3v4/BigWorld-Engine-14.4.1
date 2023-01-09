#ifndef SPACECONVERTER_DLL

#if defined( SPACECONVERTER_EXPORT )
#	define SPACECONVERTER_DLL __declspec(dllexport)
#elif defined( SPACECONVERTER_IMPORT )
#	define SPACECONVERTER_DLL __declspec(dllimport)
#else
#	define SPACECONVERTER_DLL
#endif

#endif
