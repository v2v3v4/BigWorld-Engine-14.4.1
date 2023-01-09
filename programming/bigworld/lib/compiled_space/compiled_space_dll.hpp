#ifndef COMPILED_SPACE_DLL_HPP
#define COMPILED_SPACE_DLL_HPP

//-----------------------------------------------------------------------------
// API Exposure

#ifndef COMPILED_SPACE_API

#if defined( COMPILED_SPACE_EXPORT )
#	define COMPILED_SPACE_API __declspec(dllexport)
#elif defined( COMPILED_SPACE_IMPORT )
#	define COMPILED_SPACE_API __declspec(dllimport)
#else
#	define COMPILED_SPACE_API
#endif

#endif // COMPILED_SPACE_API

#endif // COMPILED_SPACE_DLL_HPP
