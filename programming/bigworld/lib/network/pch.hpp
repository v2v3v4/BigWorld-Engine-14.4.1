#ifndef NETWORK_PCH_HPP
#define NETWORK_PCH_HPP

#if PCH_SUPPORT

#include "cstdmf/cstdmf_lib.hpp"

#if defined( USE_OPENSSL )
namespace BWOpenSSL
{
#include "openssl/bio.h"
#include "openssl/bn.h"
#include "openssl/crypto.h"
#include "openssl/evp.h"
#include "openssl/ec.h"
#include "openssl/ecdsa.h"
#include "openssl/engine.h"
#include "openssl/err.h"
#include "openssl/pem.h"
#include "openssl/rand.h"
#include "openssl/rsa.h"
#include "openssl/sha.h"
} // namespace BWOpenSSL
#endif // defined( USE_OPENSSL )

#endif // PCH_SUPPORT

#ifdef _WIN32
#include "cstdmf/cstdmf_windows.hpp"
#include <WinSock2.h>
#endif // _WIN32

#endif // NETWORK_PCH_HPP
