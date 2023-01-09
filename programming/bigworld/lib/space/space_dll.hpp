#ifndef SPACE_DLL_HPP
#define SPACE_DLL_HPP

//-----------------------------------------------------------------------------
// API Exposure

#ifndef SPACE_API

#if defined( SPACE_EXPORT )
#	define SPACE_API __declspec(dllexport)
#elif defined( SPACE_IMPORT )
#	define SPACE_API __declspec(dllimport)
#else
#	define SPACE_API
#endif

#endif // SPACE_API

#endif // SPACE_DLL_HPP
