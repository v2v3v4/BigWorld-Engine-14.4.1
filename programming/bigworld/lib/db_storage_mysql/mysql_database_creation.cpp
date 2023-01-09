#include "mysql_database_creation.hpp"

#include "mysql_database.hpp"


BW_BEGIN_NAMESPACE

IDatabase * createMySqlDatabase(
		Mercury::NetworkInterface & interface,
		Mercury::EventDispatcher & dispatcher )
{
	try
	{
		MySqlDatabase * pDatabase = new MySqlDatabase( interface, dispatcher );

		return pDatabase;
	}
	catch (std::exception & e)
	{
		return NULL;
	}
}

BW_END_NAMESPACE

// mysql_database_creation.cpp
