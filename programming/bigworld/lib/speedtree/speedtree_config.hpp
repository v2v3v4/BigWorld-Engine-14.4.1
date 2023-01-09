#ifndef SPEEDTREE_CONFIG_HPP
#define SPEEDTREE_CONFIG_HPP

#include "cstdmf/timestamp.hpp"

#include "cstdmf/bw_string.hpp"

#include "speedtree_config_lite.hpp"

// set ENABLE_PROFILE_OUTPUT to 1 to 
// enable output of profiling messages
#define ENABLE_PROFILE_OUTPUT 0

#if ENABLE_PROFILE_OUTPUT
	#define START_TIMER(timer)                    \
		uint64 timer##timer = timestamp()
	#define STOP_TIMER(timer, message)            \
		timer##timer = timestamp() - timer##timer;\
		BW::stringstreamstream##timer;          \
		stream##timer << NiceTime(timer##timer);  \
		INFO_MSG(	                              \
			"%s took %s (%s)\n", #timer,          \
			stream##timer.str().c_str(), message)
#else
	#define START_TIMER(timer)
	#define STOP_TIMER(timer, message)
#endif

#if SPEEDTREE_SUPPORT
BW_BEGIN_NAMESPACE

namespace speedtree {

// Error callback
typedef void(*ErrorCallbackFunc)(const BW::string &, const BW::string &);

void setErrorCallback(ErrorCallbackFunc callback);
void speedtreeError(const char * treeFilename, const char * errorMsg);

} // namespace speedtree

BW_END_NAMESPACE
#endif // SPEEDTREE_SUPPORT

#endif // SPEEDTREE_CONFIG_HPP
