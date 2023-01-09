#ifndef CSTDMF_INIT_HPP
#define CSTDMF_INIT_HPP

#include "bw_namespace.hpp"
#include "singleton.hpp"
#include "debug_filter.hpp"
#include "dogwatch.hpp"
#include "bgtask_manager.hpp"

BW_BEGIN_NAMESPACE

class CStdMf : public Singleton<CStdMf>
{
public:
						CStdMf();
	virtual				~CStdMf();

	DebugFilter&		debugFilter();
	DogWatchManager&	dogWatchManager();
	BgTaskManager&		bgTaskManager();

	CSTDMF_DLL static bool	checkUnattended();

};


inline DebugFilter& CStdMf::debugFilter()
{
	return DebugFilter::instance();
}


inline DogWatchManager& CStdMf::dogWatchManager()
{
	return DogWatchManager::instance();
}


inline BgTaskManager& CStdMf::bgTaskManager()
{
	return BgTaskManager::instance();
}

BW_END_NAMESPACE

#endif // CSTDMF_INIT_HPP
