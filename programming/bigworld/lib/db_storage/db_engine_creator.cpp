#include "db_engine_creator.hpp"

#include "cstdmf/debug.hpp"

#include "cstdmf/bw_vector.hpp"

DECLARE_DEBUG_COMPONENT2( "DBEngine", 0 )

BW_BEGIN_NAMESPACE

namespace // (anonymous)
{

typedef BW::vector< DatabaseEngineCreator * > DatabaseEngines;

// This global is used by the IntrusiveObject of the DatabaseEngineCreator
DatabaseEngines * g_pDatabaseEnginesCollection = NULL;

} // end namespace (anonymous)


// --------------------------------------------------------------------------
// Section: DatabaseEngineData
// --------------------------------------------------------------------------

DatabaseEngineData::DatabaseEngineData(
	Mercury::NetworkInterface & interface,
		Mercury::EventDispatcher & dispatcher,
		bool isProduction ) :
	pInterface_( &interface ),
	pDispatcher_( &dispatcher ), 
	isProduction_( isProduction )
{ }


// --------------------------------------------------------------------------
// Section: DatabaseEngineCreator
// --------------------------------------------------------------------------

/**
 *	Constructor.
 */
DatabaseEngineCreator::DatabaseEngineCreator( const BW::string & typeName ) :
	IntrusiveObject< DatabaseEngineCreator >( g_pDatabaseEnginesCollection ),
	typeName_( typeName )
{ }


/**
 *	This method calls through to the database specific creator method
 *	in order to create the database engine if the type name requested
 *	matches the current implementation.
 *
 *	@param type  The type name of the DB engine to create (eg: mysql).
 *	@param interface  The network interface for the engine to use (if required).
 *	@param dispatcher  The event dispatcher to attach to (if required).
 *
 *	@returns A pointer to an IDatabase interface for the engine created on
 *	         success, NULL on error.
 */
IDatabase * DatabaseEngineCreator::create( const BW::string type,
							const DatabaseEngineData & dbEngineData ) const
{
	IDatabase * pDatabase = NULL;

	if (type.empty() || type == typeName_)
	{
		pDatabase = this->createImpl(
						const_cast< DatabaseEngineData & >( dbEngineData ) );
	}

	return pDatabase;
}


/**
 *	This method creates an IDatabase instance.
 *
 *	@param type  The type name of the DB engine to create (eg: mysql).
 *	@param dbEngineData  Database engine data structure containing the
 *						dispatcher and network interface to use when
 *						creating the database implementation.
 *
 *	@returns A pointer to an IDatabase interface for the engine created on
 *	         success, NULL on error.
 */
/* static */ IDatabase * DatabaseEngineCreator::createInstance(
	const BW::string type,
	const DatabaseEngineData & dbEngineData )
{
	if (g_pDatabaseEnginesCollection == NULL)
	{
		WARNING_MSG( "DatabaseEngineCreator::createInstance: "
			"No database engines registered.\n" );
		return NULL;
	}

	DatabaseEngines::iterator iter = g_pDatabaseEnginesCollection->begin();
	IDatabase * pDatabase = NULL;
	while (iter != g_pDatabaseEnginesCollection->end())
	{
		pDatabase = (*iter)->create( type, dbEngineData );

		if (pDatabase)
		{
			INFO_MSG( "DatabaseEngineCreator::createInstance: "
				"Created database interface: %s\n", type.c_str() ); 
			return pDatabase;
		}

		++iter;
	}

	return NULL;
}

BW_END_NAMESPACE

// db_engine_creator.cpp
