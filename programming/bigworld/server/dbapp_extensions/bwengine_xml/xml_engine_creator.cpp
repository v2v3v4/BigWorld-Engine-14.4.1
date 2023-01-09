#include "cstdmf/bw_namespace.hpp"

#include "db_storage/db_engine_creator.hpp"

#include "db_storage_xml/xml_database.hpp"


BW_BEGIN_NAMESPACE

/**
 *	XML database engine creator.
 */
class XMLEngineCreator : public DatabaseEngineCreator
{
public:
	/**
	 *	Constructor.
	 */
	XMLEngineCreator() :
		DatabaseEngineCreator( "xml" )
	{ }


	/*
	 *	Override from DatabaseEngineCreator.
	 */
	IDatabase * createImpl( DatabaseEngineData & dbEngineData ) const
	{
		IDatabase * pDatabase = new XMLDatabase();

		if (pDatabase && dbEngineData.isProduction())
		{
			ERROR_MSG(
				"The XML database is suitable for demonstrations and "
				"evaluations only.\n"
				"Please use the MySQL database for serious development and "
				"production systems.\n"
				"See the Server Operations Guide for instructions on how to "
				"switch to the MySQL database.\n" );
		}

		return pDatabase;
	}
};

namespace // (anonymous)
{

XMLEngineCreator staticInitialiser;

} // end namespace (anonymous)

BW_END_NAMESPACE

// xml_engine_creator.cpp
