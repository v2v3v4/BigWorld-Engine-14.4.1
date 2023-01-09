#ifndef INIT_SINGLETON_HPP
#define INIT_SINGLETON_HPP

#include "singleton.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class is used to implement singletons that are created through a init
 *	method and destroyed through a fini method. These methods are reference
 *	counted, allowing a class to init/fini other classes it depends on. Also,
 *	a virtual doInit method is called in the derived class if refcount is 0 in
 *	the init method, and a virtual doFini method is called in the derived class
 *	when refcount reaches 0 in the fini method.
 *
 *	If creating a InitSingleton-derived class MyApp:
 *
 *	class MyApp : public InitSingleton< MyApp >
 *	{
 *	};
 *
 *	In cpp file,
 *	BW_INIT_SINGLETON_STORAGE( MyApp )
 *
 *	To use:
 *	MyApp::init(); // This will create the singleton if the refcount is 0.
 *	...
 *	MyApp * pApp = MyApp::pInstance(); // pointer access
 *	MyApp & app = MyApp::instance(); // reference access
 *	...
 *	MyApp::fini(); // This will destroy the singleton when the refcount is 0.
 *
 */
template <class T>
class InitSingleton : public Singleton<T>
{
public:

	/**
	 *	This method creates the singleton object and calls the virtual doInit
	 *	method in the derived class if the reference count is 0.
	 *
	 *	@return		True if it was successful, false otherwise.
	 */
	static bool init()
	{
		MF_ASSERT_DEV( !s_finalised_ );

		bool result = true;

		if ( s_initedCount_ == 0 )
		{
			// The Singleton base class will init the instance in the constructor, which
			// will hold the object, so no need to store it anywhere, "managerInstance"
			// is just a local pointer.
			InitSingleton<T>* baseInstance = new T();

			// Do init
			result = baseInstance->doInit();
		}

		++s_initedCount_;
		return result;
	}


	/**
	 *	This method calls the virtual doFini method in the derived class and
	 *	deletes the singleton object if the reference count reaches 0.
	 *
	 *	@return		True if it was succesful, false otherwise.
	 */
	static bool fini()
	{
		MF_ASSERT_DEV( !s_finalised_ );

		IF_NOT_MF_ASSERT_DEV( s_initedCount_ > 0 )
		{
			return true;
		}

		--s_initedCount_;

		bool result = true;

		if ( s_initedCount_ == 0 )
		{
			// Do fini
			InitSingleton<T>* baseInstance = Singleton<T>::pInstance();

			result = baseInstance->doFini();

			// We don't have a pointer to it, delete using the pInstance directly.
			delete Singleton<T>::pInstance();

			s_finalised_ = true;
		}

		return result;
	}


	/**
	 *	This method allows checking if the class has been finalised, to avoid
	 *	trying to access it after finalisation.
	 *
	 *	@return		True if the fini has been called until the refcount is 0.
	 */
	static bool finalised() { return s_finalised_; }

protected:
	static bool s_finalised_;
	static int s_initedCount_;

	virtual ~InitSingleton() {}
	virtual bool doInit() { return true; }
	virtual bool doFini() { return true; }
};


/**
 *	This macro should appear in the cpp of the derived singleton class.
 */
#define BW_INIT_SINGLETON_STORAGE( TYPE )					\
BW_SINGLETON_STORAGE( TYPE )								\
template <>													\
bool InitSingleton< TYPE >::s_finalised_ = false;			\
template <>													\
int InitSingleton< TYPE >::s_initedCount_ = 0;

BW_END_NAMESPACE

#endif // INIT_SINGLETON_HPP
