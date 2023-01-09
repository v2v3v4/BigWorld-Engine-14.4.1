#ifndef IDATABASE_HPP
#define IDATABASE_HPP

#include "entity_key.hpp"

#include "connection/log_on_status.hpp"

#include "network/basictypes.hpp"

#include "server/backup_hash.hpp"
#include "server/common.hpp"
#include "server/look_up_entities.hpp"

#include <limits>
#include "cstdmf/bw_set.hpp"
#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_vector.hpp"


BW_BEGIN_NAMESPACE

class BillingSystem;
class EntityDescriptionMap;
class EntityDefs;
class IEntityAutoLoader;

namespace Mercury
{
class EventDispatcher;
}

typedef BW::vector<EntityTypeID> TypeIDVec;
typedef BW::vector< Mercury::Address > SecondaryDBAddrs;
typedef BW::set< SpaceID > SpaceIDSet;

/**
 *	This class defines an interface to the database. This allows us to support
 *	different database types (such as XML or MySQL).
 *
 *	Many functions in this interface are asynchronous i.e. return results
 *	through callbacks. The implementation has the option to call the
 *	callback prior to returning from the function, essentially making it
 *	a synchronous operation. Asynchronous implementations should take a copy
 *	the function parameters as the caller is not required to keep them alive
 *	after the function returns.
 */
class IDatabase
{
public:
	virtual ~IDatabase() {}

	/**
	 *	This method starts up the database.
	 *
	 *	@param entityDefs 	The entity definitions.
	 *	@param dispatcher 	The application dispatcher.
	 *	@param numRetries	The number of times to retry acquiring the 
	 *						application lock, if that applies for the subclass.
	 */
	virtual bool startup( const EntityDefs& entityDefs,
			Mercury::EventDispatcher & dispatcher, int numRetries ) = 0;

	/**
	 *	This method clears any game server state, and should be called once per
	 *	server-run.
	 *
	 *	@param return true on success, false otherwise.
	 */
	virtual bool resetGameServerState() { return true; }

	virtual bool shutDown() = 0;

	virtual BillingSystem * createBillingSystem() = 0;

	/**
	 *	This method returns whether this database type supports multiple DBApps.
	 *	If this returns false, then extra DBApps started past the first DBApp
	 *	will fail to initialise.
	 *
	 *	@return true if multiple DBApps are supported, false otherwise.
	 */
	virtual bool supportsMultipleDBApps() const { return true; }

	/**
	 *	This is the callback interface used by getEntity().
	 */
	class IGetEntityHandler
	{
	public:
		/**
		 *	This method is called when getEntity() completes.
		 *
		 *	@param	isOK	True if sucessful. False if the entity cannot
		 *	be found with ekey.
		 */
		virtual void onGetEntityComplete( bool isOK,
				const EntityDBKey & entityKey,
				const EntityMailBoxRef * pBaseEntityLocation ) = 0;
	};


	/**
 	 *	This method retrieves an entity's data from the database.
	 *
	 *	@param	handler	This handler will be asked to provide the parameters
	 *	for this operation, and it will be notified when the operation
	 *	completes.
	 */
	virtual void getEntity( const EntityDBKey & entityKey,
			BinaryOStream * pStream,
			bool shouldGetBaseEntityLocation,
			IGetEntityHandler & handler ) = 0;


	/**
	 *	This is the callback interface used by getDatabaseIDFromName.
	 */
	class IGetDbIDHandler
	{
	public:
		virtual void onGetDbIDComplete( bool isOK,
			const EntityDBKey & entityKey ) = 0;
	};


	/**
	 *	This method converts a database identifier into a DBID.
	 *
	 *	The onGetDbIDComplete callback will be invoked on completion.
	 *
	 *	@param entityKey  The entity key containing the identifier and
	 *	                  entity TypeID to convert into a DBID.
	 */
	virtual void getDatabaseIDFromName( const EntityDBKey & entityKey,
			IGetDbIDHandler & handler ) = 0;


	/**
	 *	This is the callback interface used by lookUpEntities().
	 */
	class ILookUpEntitiesHandler
	{
	public:
		/**
		 *	Called when the results are started.
		 *
		 *	@param numEntities 	The number of entities that matched the request
		 *						query.
		 */
		virtual void onLookUpEntitiesStart( size_t numEntities ) = 0;

		/**
		 *	Called for each entity that has been successfully returned.
		 *
		 *	@param databaseID 	The database ID of the entity.
		 *	@param baseEntityLocation 
		 *						The location of the base entity for this
		 *						entity, if it has been checked out. 
		 *						If the entity is not checked out, the
		 *						ip and address fields will be set to 0 and
		 *						Mercury::Address::NONE respectively.
		 */
		virtual void onLookedUpEntity( DatabaseID databaseID,
				const EntityMailBoxRef & baseEntityLocation ) = 0;

		/**
		 *	Called when the results are finished.
		 *
		 *	@param hasError 	True if there was an error while processing 
		 *						the request.
		 */
		virtual void onLookUpEntitiesEnd( bool hasError = false ) = 0;
	};


	/**
	 *	This method looks up entities and their checked out locations based on
	 *	matches on all the given property queries.
	 *
	 *	@param entityTypeID 	The entity type ID to search for.
	 *	@param queries			The property queries to match.
	 *	@param handler			The handler that will be called back with the
	 *							results.
	 */
	virtual void lookUpEntities( EntityTypeID entityTypeID,
			const LookUpEntitiesCriteria & criteria,
			ILookUpEntitiesHandler & handler ) = 0;


	/**
	 *	This is the callback interface used by putEntity().
	 */
	class IPutEntityHandler
	{
	public:
		/**
		 *	This method is called when putEntity() completes.
		 *
		 *	@param	isOK	Is true if entity was written to the database
		 *	Is false if ekey.dbID != 0 but didn't identify an existing entity.
		 *	@param	dbID	Is the database ID of the entity. Particularly
		 *	useful if ekey.dbID == 0 originally.
		 */
		virtual void onPutEntityComplete( bool isOK, DatabaseID dbID ) = 0;
	};

	/**
	 *	This method writes an entity's data into the database.
	 *
	 *	@param entityKey	This contains the key to find the entity.
	 *	@param pStream	If provided, this data is written.
	 *	@param pBaseMailbox If provided, the location of the entity is stored
	 *			in the database.
	 *	@param removeBaseMailbox If true, the location of the entity is removed
	 *		from the database.
	 *	@param putExplicitID If true, the entityKey.dbID will contain the dbID
	 *		as the key for the inserted row.
	 *	@param handler	This handler will be notified when the operation
	 *	completes.
	 */
	virtual void putEntity( const EntityKey & entityKey,
			EntityID entityID,
			BinaryIStream * pStream,
			const EntityMailBoxRef * pBaseMailbox,
			bool removeBaseMailbox,
			bool putExplicitID,
			UpdateAutoLoad updateAutoLoad,
			IPutEntityHandler & handler ) = 0;

	/**
	 *	This is the callback interface used by delEntity().
	 */
	class IDelEntityHandler
	{
	public:
		virtual ~IDelEntityHandler() {}
		/**
		 *	This method is called when delEntity() completes.
		 *
		 *	@param	isOK	Is true if entity was deleted from the database.
		 *	Is false if the entity doesn't exist.
		 */
		virtual void onDelEntityComplete( bool isOK ) = 0;
	};

	/**
	 *	This method deletes an entity from the database, if it exists.
	 *
	 *	@param	ekey	The key for indentifying the entity. If ekey.dbID != 0
	 *	then it will be used to identify the entity. Otherwise ekey.name will
	 *	be used.
	 *	@param	handler	This handler will be notified when the operation
	 *	completes.
	 */
	virtual void delEntity( const EntityDBKey & ekey,
			EntityID entityID,
			IDelEntityHandler& handler ) = 0;

	//-------------------------------------------------------------------

	virtual void setGameTime( GameTime time ) {};

	/**
	 * 	Check whether there is any unrecoverable error case.
	 */
	virtual bool hasUnrecoverableError() const { return false; }

	/**
	 * 	Callback interface for the getBaseAppMgrInitData() method.
	 */
	class IGetBaseAppMgrInitDataHandler
	{
	public:
		/**
		 * 	This method is called when getBaseAppMgrInitData() completes.
		 *
		 * 	@param	gameTime	The game time as stored in the database.
		 */
		virtual void onGetBaseAppMgrInitDataComplete( GameTime gameTime ) = 0;
	};

	/**
	 * 	Gets the initialisation data required for BaseAppMgr.
	 */
	virtual void getBaseAppMgrInitData(
			IGetBaseAppMgrInitDataHandler& handler ) = 0;

	//-------------------------------------------------------------------

	/**
	 *	This is the callback interface used by executeRawCommand().
	 */
	class IExecuteRawCommandHandler
	{
	public:
		/**
		 *	This method is called by executeRawCommand() to get the stream
		 *	in which to store the results of the operation. This method
		 *	may be called from another thread.
		 *
		 *	This function may be called multiple times and the implementation
		 *	should return the stream instance each time.
		 */
		virtual BinaryOStream& response() = 0;

		/**
		 *	This method is called when executeRawCommand() completes.
		 */
		virtual void onExecuteRawCommandComplete() = 0;
	};
	virtual void executeRawCommand( const BW::string & command,
		IExecuteRawCommandHandler& handler ) = 0;

	virtual void putIDs( int count, const EntityID * ids ) = 0;

	/**
	 *	This is the callback interface used by getIDs().
	 */
	class IGetIDsHandler
	{
	public:
		/**
		 *	This method is called by getIDs() to get the stream	in which to
		 *	store the IDs. This method may be called from another thread.
		 *
		 *	This function may be called multiple times and the implementation
		 *	should return the stream instance each time.
		 */
		virtual BinaryOStream& idStrm() = 0;

		/**
		 *	This method resets whatever there is in the stream obtained using
		 * 	idStrm(), reverting any changes to the stream so far.
		 */
		virtual void resetStrm() = 0;

		/**
		 *	This method is called when getIDs() completes.
		 */
		virtual void onGetIDsComplete() = 0;
	};
	virtual void getIDs( int count, IGetIDsHandler& handler ) = 0;

	virtual void writeSpaceData( BinaryIStream& spaceData ) = 0;

	/**
	 * 	This method adds the space data into the stream, in a format compatible
	 * 	with the BaseAppMgrInterface::prepareForRestoreFromDB message.
	 */
	virtual bool getSpacesData( BinaryOStream& strm ) = 0;

	/**
	 * 	This method tells the recoverer about all entities that were active
	 * 	during controlled shutdown. The recoverer is responsible for loading
	 * 	the entities back into the system.
	 */
	virtual void autoLoadEntities( IEntityAutoLoader & autoLoader ) = 0;

	/**
	 *	This method converts all the entity mailboxes in the
	 * 	database from srcAddr to destAddrs using the following algorithm:
	 * 		IF mailbox.addr == srcAddr
	 *			mailbox.addr = destAddrs[ mailbox.id % destAddrs.size() ]
	 * 	NOTE: The salt of the addr must not be modified.
	 */
	virtual void remapEntityMailboxes( const Mercury::Address& srcAddr,
			const BackupHash & destAddrs ) = 0;

	//-------------------------------------------------------------------
	// Secondary databases
	//-------------------------------------------------------------------

	/**
	 *	This method returns whether or not the database implements secondary
	 *	databases that should be consolidated.
	 */
	virtual bool shouldConsolidate() const = 0;

	/**
	 *	This method sets whether or not the database should allow or
	 *	disallow consolidation.
	 */
	virtual void shouldConsolidate( bool shouldConsolidate ) = 0;

	/**
	 *	A secondary database entry. Stores information about the secondary
	 * 	database so that we can access the secondary database from DBApp.
	 */
	class SecondaryDBEntry
	{
	public:
		// The address of the BaseApp.
		Mercury::Address 	addr;
		// The location of the secondary database on BaseApp machine.
		BW::string			location;

		SecondaryDBEntry( uint32 ip, int16 port, const BW::string& loc ) :
			addr( ip, port ), location( loc )
		{}
		SecondaryDBEntry() {}
	};
	typedef BW::vector< SecondaryDBEntry > SecondaryDBEntries;

	/**
	 *	Adds a secondary database entry.
	 *
	 * 	@param	The entry to add.
	 */
	virtual void addSecondaryDB( const SecondaryDBEntry& entry ) = 0;

	/**
	 *	This is the completion callback interface for updateSecondaryDBs().
	 */
	class IUpdateSecondaryDBshandler
	{
	public:
		/**
		 *	This callback is called when updateSecondaryDBs() completes.
		 *
		 * 	@param	The secondary database entries that were removed by the
		 * 	update operation.
		 */
		virtual void onUpdateSecondaryDBsComplete(
				const SecondaryDBEntries& removedEntries ) = 0;
	};

	/**
	 *	Updates secondary database entries. Only entries with an address in
	 *	the provided list are kept.
	 *
	 * 	@param	The entry addresses to keep.
	 */
	virtual void updateSecondaryDBs( const SecondaryDBAddrs& addrs,
			IUpdateSecondaryDBshandler& handler ) = 0;

	/**
	 *	This is the completion callback interface for getSecondaryDatabases().
	 */
	class IGetSecondaryDBsHandler
	{
	public:
		/**
		 *	This callback is called when getSecondaryDatabases() completes.
		 *
		 * 	@param	The secondary database entries in the database.
		 */
		virtual void onGetSecondaryDBsComplete(
				const SecondaryDBEntries& entries ) = 0;
	};
	/**
	 *	Gets all the secondary database entries in the database.
	 */
	virtual void getSecondaryDBs( IGetSecondaryDBsHandler& handler ) = 0;

	/**
	 * 	Gets the number of secondary database entries in the database.
	 * 	This is a blocking function.
	 */
	virtual uint32 numSecondaryDBs() = 0;

	/**
	 *	Clears all secondary database entries. This function blocks until
	 * 	completion.
	 *
	 * 	@return	The number of secondary database entries cleared. Returns -1
	 * 			on error.
	 */
	virtual int clearSecondaryDBs() = 0;

	//-------------------------------------------------------------------
	// Lock/Unlock database
	//-------------------------------------------------------------------
	/**
	 * 	Locks the database so that this process has exclusive access to the
	 * 	database. Database is automatically locked by the startup() method,
	 * 	so this method and the unlockDB() method is mainly intended for
	 * 	unlocking the database temporarily to allow other processes to
	 * 	access the database.
	 */
	virtual bool lockDB() = 0;
	/**
	 * 	Unlocks the database so that another BigWorld process can use it.
	 */
	virtual bool unlockDB() = 0;
};

BW_END_NAMESPACE

#endif // IDATABASE_HPP

