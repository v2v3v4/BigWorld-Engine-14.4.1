#include "pch.hpp"
#include "singleton_manager.hpp"

#ifdef ENABLE_SINGLETON_MANAGER
BW_BEGIN_NAMESPACE

std::auto_ptr< SingletonManager > SingletonManager::s_pInstance( new SingletonManager() );

BW_END_NAMESPACE

#endif

