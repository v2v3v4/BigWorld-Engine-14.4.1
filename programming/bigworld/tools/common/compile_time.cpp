#include "pch.hpp"
#include "compile_time.hpp"

BW_BEGIN_NAMESPACE

/// This string stores the time and date that the executable was compiled.
const char * aboutCompileTimeString = __DATE__ " at " __TIME__;
BW_END_NAMESPACE

