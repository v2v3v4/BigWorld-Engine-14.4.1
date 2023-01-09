#ifndef MYSQL_DATABASE_CREATION_HPP
#define MYSQL_DATABASE_CREATION_HPP

#include "cstdmf/bw_namespace.hpp"


BW_BEGIN_NAMESPACE

class IDatabase;

namespace Mercury
{
class EventDispatcher;
class NetworkInterface;
}


IDatabase * createMySqlDatabase(
		Mercury::NetworkInterface & interface,
		Mercury::EventDispatcher & dispatcher );

BW_END_NAMESPACE

#endif // MYSQL_DATABASE_CREATION_HPP
