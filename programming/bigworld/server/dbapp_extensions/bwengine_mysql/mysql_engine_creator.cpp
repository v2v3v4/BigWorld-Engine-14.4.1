#include "cstdmf/bw_namespace.hpp"

#include "db_storage/db_engine_creator.hpp"

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
	IDatabase * createImpl( DatabaseEngineData & dbEngineData ) const
	{
		return createMySqlDatabase( dbEngineData.interface(),
									dbEngineData.dispatcher() );
	}
};

namespace // (anonymous)
{

MySqlEngineCreator staticInitialiser;

} // end namespace (anonymous)

BW_END_NAMESPACE

// mysql_engine_creator.cpp
