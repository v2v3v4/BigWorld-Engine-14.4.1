#ifndef REVIVER_COMMON_HPP
#define REVIVER_COMMON_HPP

// This file contains information used by both the Reviver process and by
// Reviver subjects.

#include "cstdmf/stdmf.hpp"


BW_BEGIN_NAMESPACE

typedef uint8 ReviverPriority;
const ReviverPriority REVIVER_PING_NO  = 0;
const ReviverPriority REVIVER_PING_YES = 1;
const float REVIVER_DEFAULT_SUBJECT_TIMEOUT = 0.2f;
const float REVIVER_DEFAULT_PING_PERIOD = 0.1f;

BW_END_NAMESPACE

#endif // REVIVER_COMMON_HPP
