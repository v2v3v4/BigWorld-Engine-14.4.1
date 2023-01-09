#ifndef BWENTITY_API_HPP
#define BWENTITY_API_HPP

#if defined(_MSC_VER) && defined(BWENTITY_DLL_EXPORT)
#   define BWENTITY_API   			__declspec(dllexport)
#elif defined(_MSC_VER) && defined(BWENTITY_DLL_IMPORT)
#   define BWENTITY_API   			__declspec(dllimport)
#else
#	define BWENTITY_API
#endif  // BWENTITY_DLL_

// When building with BWENTITY_DLL_IMPORT, these are explicit instantiation
// declarations; part of C++11 (14.7.2 [temp.explicit]) but
// both gcc (since 3.2.3) and Visual Studio (since Visual C++ 5.0) support it.
#if defined(BWENTITY_DLL_EXPORT)
#	define BWENTITY_EXPORTED_TEMPLATE_CLASS template class BWENTITY_API
#	define BWENTITY_EXPORTED_TEMPLATE_STRUCT template struct BWENTITY_API
#elif defined(BWENTITY_DLL_IMPORT)
#if defined(_MSC_VER) && (_MSC_VER < 1700)
// Visual Studio before VS2012 produces this warning when you use
// "extern template class":
// nonstandard extension used : 'extern' before template explicit instantiation
#pragma warning(disable : 4231)
#endif // defined(_MSC_VER) && (_MSC_VER < 1700)
#	define BWENTITY_EXPORTED_TEMPLATE_CLASS extern template class BWENTITY_API
#	define BWENTITY_EXPORTED_TEMPLATE_STRUCT extern template struct BWENTITY_API
#else
#	define BWENTITY_EXPORTED_TEMPLATE_CLASS template class BWENTITY_API
#	define BWENTITY_EXPORTED_TEMPLATE_STRUCT template struct BWENTITY_API
#endif

#endif /* BWENTITY_API_HPP */
