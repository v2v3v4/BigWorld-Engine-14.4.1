#ifndef PS3_COMPATIBILITY_HPP
#define PS3_COMPATIBILITY_HPP

#include <netex/errno.h>
#include <sys/time.h>
#define unix
#undef EAGAIN
#define EAGAIN SYS_NET_EAGAIN
#define ECONNREFUSED SYS_NET_ECONNREFUSED
#define EHOSTUNREACH SYS_NET_EHOSTUNREACH
#define ENOBUFS SYS_NET_ENOBUFS
#define select socketselect
#undef errno
#define errno sys_net_errno

#endif // PS3_COMPATIBILITY_HPP
