#ifndef LINUX_MACHINE_GUARD_HPP
#define LINUX_MACHINE_GUARD_HPP

#include <signal.h>
#include <unistd.h>
#include <syslog.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "common_machine_guard.hpp"


BW_BEGIN_NAMESPACE

void checkSocketBufferSizes();

BW_END_NAMESPACE

#endif
