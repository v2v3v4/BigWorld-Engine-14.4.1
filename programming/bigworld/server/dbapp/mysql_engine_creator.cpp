#include "db_engine_creator.hpp"

#include "cstdmf/bw_namespace.hpp"

#include "db_storage_mysql/mysql_database_creation.hpp"


BW_BEGIN_NAMESPACE

/**
 *	MySql database engine creator.
 */
class MySqlEngineCreator : public DatabaseEngineCreator
{
public:
	/**
	 *	Constructor.
	 */
	MySqlEngineCreator() :
		DatabaseEngineCreator( "mysql" )
	{ }


	/*
	 *	Override from DatabaseEngineCreator
	 */
	IDatabase * createImpl( Mercury::NetworkInterface & interface,
							Mercury::EventDispatcher & dispatcher ) const
	{
		return createMySqlDatabase( interface, dispatcher );
	}
};

namespace // (anonymous)
{

MySqlEngineCreator staticInitialiser;

} // end namespace (anonymous)

BW_END_NAMESPACE

// mysql_engine_creator.cpp
