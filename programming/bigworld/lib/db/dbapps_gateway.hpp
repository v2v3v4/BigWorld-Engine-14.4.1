#ifndef DBAPPS_GATEWAY_HPP
#define DBAPPS_GATEWAY_HPP

#include "dbapp_gateway.hpp"

#include "cstdmf/watcher.hpp"

#include "db/db_hash_schemes.hpp"

#include "network/basictypes.hpp"


BW_BEGIN_NAMESPACE

class BinaryIStream;
class BinaryOStream;

/**
 *	This class serves as a gateway class for remote processes to access DBApp
 *	functionality.
 */
class DBAppsGateway
{
public:
	typedef DBHashSchemes::DBAppIDBuckets< DBAppGateway >::HashScheme 
		HashScheme;


	class IUpdateVisitor
	{
	public:
		
		/** Destructor. */
		virtual ~IUpdateVisitor() {}


		/**
		 *	This method is called when a new DBApp has been added in the update.
		 *
		 *	@param gateway 		The gateway class.
		 *	@param descriptor 	The descriptor for the DBApp that was added.
		 *	@param isAlpha 		True if this DBApp is the new alpha DBApp,
		 *						false otherwise.
		 */
		virtual void onDBAppAdded( DBAppsGateway & gateway,
				const DBAppGateway & descriptor,
				bool isAlpha )
		{}


		/**
		 *	This method is called when a DBApp has been removed in the update.
		 *
		 *	@param gateway 		The gateway class.
		 *	@param descriptor 	The descriptor for the DBApp that was removed.
		 *	@param wasAlpha 	True if this DBApp was the former alpha
		 *						DBApp, false otherwise.
		 */
		virtual void onDBAppRemoved( DBAppsGateway & gateway,
				const DBAppGateway & descriptor,
				bool wasAlpha )
		{}

	};

	DBAppsGateway();

	void addDBApp( const DBAppGateway & descriptor );
	bool removeDBApp( DBAppID appID );

	bool updateFromStream( BinaryIStream & data,
			IUpdateVisitor * pVisitor = NULL );

	/** This method returns true if there are no DBApps available. */
	bool empty() const { return hashScheme_.empty(); }

	/** This method returns the number of DBApps in the collection. */
	size_t size() const { return hashScheme_.size(); }

	const DBAppGateway & getDBApp( DatabaseID dbID = 0 ) const;

	/** Convenience accessor for operator[] to call getDBApp(). */
	const DBAppGateway & operator[]( DatabaseID dbID ) const
	{
		return this->getDBApp( dbID );
	}

	const DBAppGateway & alpha() const;

	static WatcherPtr pWatcher();

private:

	HashScheme 	hashScheme_;

	friend BinaryOStream & operator<<( BinaryOStream & os,
		const DBAppsGateway & gateway );
	friend BinaryIStream & operator>>( BinaryIStream & is,
		DBAppsGateway & gateway );
};


BinaryOStream & operator<<( BinaryOStream & os, const DBAppsGateway & gateway );
BinaryIStream & operator>>( BinaryIStream & is, DBAppsGateway & gateway );

BW_END_NAMESPACE


#endif  // DBAPPS_GATEWAY_HPP
