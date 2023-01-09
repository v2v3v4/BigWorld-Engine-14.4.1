#ifndef SINGLETON_MANAGER_HPP
#define SINGLETON_MANAGER_HPP

#ifdef ENABLE_SINGLETON_MANAGER

#include "cstdmf/bw_namespace.hpp"
#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_unordered_map.hpp"
#include "cstdmf/stringmap.hpp"
#include <typeinfo>
#include <memory>

BW_BEGIN_NAMESPACE

#define SINGLETON_MANAGER_WRAPPER( Type )\
	SingletonManager::InstanceFunc pFunc;\
	if (SingletonManager::instance().isRegisteredFunc< Type >( pFunc, &Type::instance ) == false)\
	{\
		return SingletonManager::instance().executeFunc( pFunc, &Type::instance, &Type::instance );\
	}

#define SINGLETON_MANAGER_WRAPPER_FUNC( Type, Func )\
	SingletonManager::InstanceFunc pFunc;\
	if (SingletonManager::instance().isRegisteredFunc< Type >( pFunc, Func ) == false)\
	{\
		return SingletonManager::instance().executeFunc( pFunc, Func, Func );\
	}

#define REGISTER_SINGLETON( Type )\
	SingletonManager::instance().registerInstanceFunc< Type >( &Type::instance );

#define REGISTER_SINGLETON_FUNC( Type, Func )\
	SingletonManager::instance().registerInstanceFunc< Type >( Func );

class SingletonManager
{
public:
	typedef int & (*InstanceFunc)();

	SingletonManager()
	{
		s_pInstance.reset( this );
	}

	template< class T, typename F >
	void registerInstanceFunc( F func )
	{
		s_Singletons.insert(
			std::make_pair(
				typeid( T ).name(),
				reinterpret_cast< InstanceFunc >( func ) ) );
	}

	template< class T, typename U >
	bool isRegisteredFunc( InstanceFunc & pFunc, U currFunc )
	{
		CollectionType::iterator instanceFuncIt =
			s_Singletons.find( typeid( T ).name() );
		if (instanceFuncIt == s_Singletons.end())
		{
			return true;
		}
		pFunc = instanceFuncIt->second;
		return reinterpret_cast< U >( pFunc ) == currFunc;
	}
 
	template< typename ReturnType, typename U >
	ReturnType executeFunc(
		InstanceFunc foundFunc, U currFunc, ReturnType (*currentFunc2)() )
	{
		U actualFunc = 
			reinterpret_cast< U >( foundFunc );
		return actualFunc();
	}


	CSTDMF_DLL static SingletonManager & instance() { return *s_pInstance; }

private:
	typedef StringRefUnorderedMap< InstanceFunc > CollectionType;
	CollectionType s_Singletons;
	static std::auto_ptr< SingletonManager > s_pInstance;
};

BW_END_NAMESPACE

#else

#define SINGLETON_MANAGER_WRAPPER( Type )
#define SINGLETON_MANAGER_WRAPPER_FUNC( Type, Func )
#define REGISTER_SINGLETON( Type )
#define REGISTER_SINGLETON_FUNC( Type, Func )

#endif //ENABLE_SINGLETON_MANAGER

#endif //SINGLETON_MANAGER_HPP

