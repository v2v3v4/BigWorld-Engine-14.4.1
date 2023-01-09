#ifndef LIBCONNECTION_PCH_HPP
#define LIBCONNECTION_PCH_HPP

#if PCH_SUPPORT
#include "cstdmf/cstdmf_lib.hpp"
#include "network/network_lib.hpp"
#endif // PCH_SUPPORT

// Needs to be included before Winsock2.h
#include "cstdmf/cstdmf_windows.hpp"

#endif // LIBCONNECTION_PCH_HPP
