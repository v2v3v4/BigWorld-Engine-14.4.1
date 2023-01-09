#ifndef CSTDMF_DLL

#if defined(_MSC_VER) && defined( CSTDMF_EXPORT )
#	define CSTDMF_DLL __declspec(dllexport)
#elif defined(_MSC_VER) && defined( CSTDMF_IMPORT )
#	define CSTDMF_DLL __declspec(dllimport)
#else
#	define CSTDMF_DLL
#endif

// When building with CSTDMF_IMPORT, these are explicit instantiation
// declarations; part of C++11 (14.7.2 [temp.explicit]) but
// both gcc (since 3.2.3) and Visual Studio (since Visual C++ 5.0) support it.
#if defined( CSTDMF_EXPORT )
#	define CSTDMF_EXPORTED_TEMPLATE_CLASS template class CSTDMF_DLL
#elif defined( CSTDMF_IMPORT )
#if defined(_MSC_VER) && (_MSC_VER < 1700)
// Visual Studio before VS2012 produces this warning when you use
// "extern template class":
// nonstandard extension used : 'extern' before template explicit instantiation
#pragma warning(disable : 4231)
#endif // defined(_MSC_VER) && (_MSC_VER < 1700)
#	define CSTDMF_EXPORTED_TEMPLATE_CLASS extern template class CSTDMF_DLL
#else
#	define CSTDMF_EXPORTED_TEMPLATE_CLASS template class CSTDMF_DLL
#endif

#endif
