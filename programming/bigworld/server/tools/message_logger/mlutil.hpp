#ifndef ML_UTIL_HPP
#define ML_UTIL_HPP

#include "cstdmf/bw_string.hpp"


BW_BEGIN_NAMESPACE

namespace MLUtil
{

bool determinePathToConfig( BW::string & configFile );

bool isPathAccessible( const char *path );

bool softMkDir( const char *path );
};

BW_END_NAMESPACE

#endif // ML_UTIL_HPP
