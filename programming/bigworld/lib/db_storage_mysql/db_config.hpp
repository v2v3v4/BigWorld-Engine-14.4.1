#ifndef DATABASE_CONFIG_HPP
#define DATABASE_CONFIG_HPP

#include "connection_info.hpp"

#include "cstdmf/stdmf.hpp"


BW_BEGIN_NAMESPACE

namespace DBConfig
{

const ConnectionInfo & connectionInfo();

}	// namespace DBConfig

BW_END_NAMESPACE

#endif // DATABASE_CONFIG_HPP
