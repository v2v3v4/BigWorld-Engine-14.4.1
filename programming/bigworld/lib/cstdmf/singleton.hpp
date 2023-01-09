#ifndef SINGLETON_HPP
#define SINGLETON_HPP

#include "debug.hpp"
#include "singleton_manager.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class is used to implement singletons. Generally singletons should be
 *	avoided. If they _need_ to be used, try to use this as the base class.
 *
 *	If creating a singleton class MyApp:
 *
 *	class MyApp : public Singleton< MyApp >
 *	{
 *	};
 *
 *	In cpp file,
 *	BW_SINGLETON_STORAGE( MyApp )
 *
 *	To use:
 *	MyApp app; // This will be registered as the singleton
 *
 *	...
 *	MyApp * pApp = MyApp::pInstance();
 *	MyApp & app = MyApp::instance();
 */
template <class T>
class Singleton
{
protected:
	static T * s_pInstance;

public:
	Singleton()
	{
		MF_ASSERT( NULL == s_pInstance );
		s_pInstance = static_cast< T * >( this );
		REGISTER_SINGLETON_FUNC( T, &T::pInstance );
	}

	~Singleton()
	{
		MF_ASSERT( this == s_pInstance );
		s_pInstance = NULL;
	}

	static T & instance()
	{
		T * instanceP = pInstance();
		MF_ASSERT( instanceP );
		return *instanceP;
	}

	static T * pInstance()
	{
		SINGLETON_MANAGER_WRAPPER_FUNC( T, &T::pInstance )
		return s_pInstance;
	}
};

/**
 *	This should appear in the cpp of the derived singleton class.
 */
#if defined (__INTEL_COMPILER)
#define BW_SINGLETON_STORAGE( TYPE )						\
TYPE * BW_NAMESPACE Singleton< TYPE >::s_pInstance = NULL;
#else
#define BW_SINGLETON_STORAGE( TYPE )						\
template <>													\
TYPE * BW_NAMESPACE Singleton< TYPE >::s_pInstance = NULL;
#endif

BW_END_NAMESPACE

#endif // SINGLETON_HPP
