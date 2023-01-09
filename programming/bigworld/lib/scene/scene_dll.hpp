#ifndef SCENE_DLL_HPP
#define SCENE_DLL_HPP

//-----------------------------------------------------------------------------
// API Exposure

#ifndef SCENE_API

#if defined( SCENE_EXPORT )
#	define SCENE_API __declspec(dllexport)
#elif defined( SCENE_IMPORT )
#	define SCENE_API __declspec(dllimport)
#else
#	define SCENE_API
#endif

#endif // SCENE_API

#endif // SCENE_DLL_HPP
