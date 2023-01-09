#ifndef DB_ENGINE_CREATOR
#define DB_ENGINE_CREATOR

#include "db_storage/idatabase.hpp"

#include "network/event_dispatcher.hpp"
#include "network/network_interface.hpp"

#include "cstdmf/intrusive_object.hpp"

#include "cstdmf/bw_string.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class is a holder to pass data from the DBApp through to the
 *	database engine implementation without requiring each intermediary
 *	method from having to have access to the data.
 */
class DatabaseEngineData
{
public:
	DatabaseEngineData( Mercury::NetworkInterface & interface_,
		Mercury::EventDispatcher & dispatcher,
		bool isProduction );

	Mercury::NetworkInterface & interface()
		{ return *pInterface_; }

	Mercury::EventDispatcher & dispatcher()
		{ return *pDispatcher_; }

	bool isProduction()
		{ return isProduction_; }

private:
	Mercury::NetworkInterface * pInterface_;
	Mercury::EventDispatcher * pDispatcher_;
	bool isProduction_;
};


/**
 *	This class provides a Factory interface so the DBApp can create a database
 *	engine interface on demand. This allows for link time registration of
 *	database types.
 *
 *	Each database type implements this interface to allow the creation 
 */
class DatabaseEngineCreator : public IntrusiveObject< DatabaseEngineCreator >
{
public:
	DatabaseEngineCreator( const BW::string & typeName );

	static IDatabase * createInstance( const BW::string type,
									const DatabaseEngineData & dbEngineData );

protected:
	/**
	 *	This virtual method allows the engine specific creator to return its
	 *	Database engine implementation.
	 *	
	 *	@param dbEngineData  Database engine data structure containing the
	 *						dispatcher and network interface to use when
	 *						creating the database implementation.
	 *
	 *	@returns A pointer to an IDatabase interface for the engine created on
	 *	         success, NULL on error.
	 *
	 */
	virtual IDatabase * createImpl( DatabaseEngineData & dbEngineData ) const = 0;

private:
	DatabaseEngineCreator();

	IDatabase * create( const BW::string type,
							const DatabaseEngineData & dbEngineData ) const;

	BW::string typeName_;
};

BW_END_NAMESPACE

#endif // DB_ENGINE_CREATOR
