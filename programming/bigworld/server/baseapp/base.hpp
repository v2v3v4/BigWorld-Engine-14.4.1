#ifndef BASE_HPP
#define BASE_HPP

#include "Python.h"

#include "baseapp_config.hpp"
#include "baseapp_int_interface.hpp"
#include "mailbox.hpp"
#include "entity_type.hpp"
#include "write_to_db_reply.hpp"

#include "common/py_timer.hpp"

#include "cstdmf/time_queue.hpp"

#include "entitydef/entity_description.hpp"
#include "delegate_interface/entity_delegate.hpp"

#include "network/udp_channel.hpp"

#include "pyscript/pyobject_plus.hpp"
#include "pyscript/script.hpp"

#include "server/auto_backup_and_archive.hpp"
#include "server/entity_profiler.hpp"
#include "server/writedb.hpp"

#include <memory>


BW_BEGIN_NAMESPACE

class Base;
typedef SmartPointer<PyObject> PyObjectPtr;
typedef SmartPointer<ServerEntityMailBox> ServerEntityMailBoxPtr;


/*~ class BigWorld.Base
 *	@components{ base }
 *	The class Base represents an entity residing on a base application. Base
 *	entities can be created via the BigWorld.createBase function (and functions
 *	with createBase prefix). An base entity instance can also be created from a
 *	remote cellApp BigWorld.createEntityOnBaseApp function call.
 *
 *	A base entity can be linked to an entity within one of the active cells,
 *	and can be used to create an associated entity on an appropriate cell. The
 *	functionality of this class allows you to create and destroy the entity on
 *	existing cells, register a timer callback function to be called on the base
 *	entity, access contact information for this object,	and also access a
 *	CellEntityMailBox through which the base entity can	communicate with its cell
 *	entity (the associated cell entity can move to different cells as a result
 *	of movement of the cell entity, or load balancing).
 */

/**
 *	Instances of this class are used to represent a generic base.
 */
class Base: public PyObjectPlus
{
	Py_Header( BW_NAMESPACE Base, PyObjectPlus )

public:
	Base( EntityID id, DatabaseID dbID, EntityTypePtr pType );
	~Base();
	bool init( PyObject * pDict, PyObject * pCellArgs,
		const BW::string * pBillingData );

	bool init( const BW::string & templateID );

	bool initDelegate( const BW::string & templateID );

	PyObject * dictFromStream( BinaryIStream & data ) const;

	EntityID id() const								{ return id_; }
	EntityMailBoxRef baseEntityMailBoxRef() const;

	Mercury::UDPChannel & channel() { return *pChannel_; }

	const Mercury::Address & cellAddr() const	{ return pChannel_->addr(); }

	void databaseID( DatabaseID );
	DatabaseID databaseID()	const					{ return databaseID_; }

	// Has a valid DatabaseID or about to get one.
	bool hasWrittenToDB() const			 { return (databaseID_ != 0); }

	// Has received a valid DatabaseID from DBApp.
	bool hasFullyWrittenToDB() const
	{
		return this->hasWrittenToDB() &&
			(databaseID_ != PENDING_DATABASE_ID);
	}

	CellEntityMailBox * pCellEntityMailBox() const	{ return pCellEntityMailBox_; }
	SpaceID spaceID() const							{ return spaceID_; }

	void destroy( bool deleteFromDB, bool writeToDB, bool logOffFromDB = true );
	void discard( bool isOffload = false );
	bool writeToDB( WriteDBFlags flags, WriteToDBReplyHandler * pHandler = NULL,
			PyObjectPtr pCellData = NULL, DatabaseID explicitDatabaseID = 0 );
	bool writeToDB( WriteDBFlags flags, WriteToDBReplyStructPtr pReplyStruct,
			PyObjectPtr pCellData, DatabaseID explicitDatabaseID );
	bool requestCellDBData( WriteDBFlags flags, WriteToDBReplyStructPtr pReplyStruct );
	PyObjectPtr getDBCellData( BinaryIStream & data );


	void autoArchive();
	bool archive();
	void backupTo( const Mercury::Address & addr,
			Mercury::Bundle & bundle,
			bool isOffloading, Mercury::ReplyMessageHandler * pHandler );
	
	void writeBackupData( BinaryOStream & stream, bool isOffload );
	void offload( const Mercury::Address & dstAddr );
	void readBackupData( BinaryIStream & stream );

	bool hasCellEntity() const;
	bool shouldSendToCell() const;

	bool isCreateCellPending() const	{ return isCreateCellPending_; }
	bool isGetCellPending() const		{ return isGetCellPending_; }
	bool isDestroyCellPending() const 	{ return isDestroyCellPending_; }

	bool hasBeenBackedUp() const		{ return hasBeenBackedUp_; }

	void reloadScript();
	bool migrate();
	void migratedAll();

	// virtual methods
	bool isProxy() const							{ return isProxy_; }

	bool isDestroyed() const						{ return isDestroyed_; }

	bool isServiceFragment() const
	{
		return pType_->description().isService();
	}

	// tell the base that its cell entity is in a new cell
	void setCurrentCell( SpaceID spaceID,
		const Mercury::Address & cellAppAddr,
		const Mercury::Address * pSrcAddr = NULL,
		bool shouldReset = false );

	void currentCell( const Mercury::Address & srcAddr,
				const Mercury::UnpackedMessageHeader & header,
				const BaseAppIntInterface::currentCellArgs & args );

	void teleportOther( const Mercury::Address & srcAddr,
				const Mercury::UnpackedMessageHeader & header,
				const BaseAppIntInterface::teleportOtherArgs & args );

	void emergencySetCurrentCell( const Mercury::Address & srcAddr,
		const Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data );

	void backupCellEntity( BinaryIStream & data );

	void writeToDB( BinaryIStream & data );

	void cellEntityLost( const Mercury::Address & srcAddr,
		   const Mercury::UnpackedMessageHeader & header,
		   BinaryIStream & data );

	void callBaseMethod( const Mercury::Address & srcAddr,
		   const Mercury::UnpackedMessageHeader & header,
		   BinaryIStream & data );

	void callCellMethod( const Mercury::Address & srcAddr,
		   const Mercury::UnpackedMessageHeader & header,
		   BinaryIStream & data );

	void getCellAddr( const Mercury::Address & srcAddr,
		   const Mercury::UnpackedMessageHeader & header,
		   BinaryIStream & data );


	void startKeepAlive( const Mercury::Address & srcAdr,
			const Mercury::UnpackedMessageHeader & header,
			const BaseAppIntInterface::startKeepAliveArgs & args );

	// Profiling
	const EntityProfiler & profiler() const { return profiler_; }
	EntityProfiler & profiler() { return profiler_; }

	// Script related methods
	ScriptObject pyGetAttribute( const ScriptString & attrObj );
	bool pySetAttribute( const ScriptString & attrObj,
		const ScriptObject & value );

	bool teleport( const EntityMailBoxRef & nearbyBaseMB );
	PY_AUTO_METHOD_DECLARE( RETOK, teleport,
			ARG( EntityMailBoxRef, END ) );

	bool createCellEntity( const ServerEntityMailBoxPtr & pNearbyMB );
	PY_AUTO_METHOD_DECLARE( RETOK, createCellEntity,
		OPTARG( ServerEntityMailBoxPtr, NULL, END ) )

	bool createInDefaultSpace();
	PY_AUTO_METHOD_DECLARE( RETOK, createInDefaultSpace, END )

	PY_KEYWORD_METHOD_DECLARE( py_createInNewSpace )

	bool createInSpace( SpaceID spaceID, const char * pyErrorPrefix );

	void cellCreationResult( bool success );

	bool sendCreateCellEntity(
			Mercury::ReplyMessageHandler * pHandler,
			EntityID nearbyID,
			const Mercury::Address & cellAddr,
			const char * errorPrefix );

	bool restoreTo( SpaceID spaceID, const Mercury::Address & cellAppAddr );

	Mercury::Bundle & cellBundle()	{ return pChannel_->bundle(); }
	void sendToCell();

	PY_KEYWORD_METHOD_DECLARE( py_destroy )
	PY_METHOD_DECLARE( py_destroyCellEntity )
	PY_KEYWORD_METHOD_DECLARE( py_writeToDB )
	PY_METHOD_DECLARE( py_addTimer )
	PY_METHOD_DECLARE( py_delTimer )

	PY_METHOD_DECLARE( py_registerGlobally )
	PY_METHOD_DECLARE( py_deregisterGlobally )

	PY_PICKLING_METHOD_DECLARE( MailBox )
	
	PY_AUTO_METHOD_DECLARE( RETOWN, getComponent, ARG( BW::string, END ) );
	PyObject * getComponent( const BW::string & name );

	float artificialMinLoad() const
		{ return profiler_.artificialMinLoad(); }
	void artificialMinLoad( float v )
		{ profiler_.artificialMinLoad( v ); }
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, artificialMinLoad, artificialMinLoad );


	PY_RO_ATTRIBUTE_DECLARE( (int)id_, id )
	PY_RO_ATTRIBUTE_DECLARE( isDestroyed_, isDestroyed )

	PY_RO_ATTRIBUTE_DECLARE( (int)pType_->description().index(), baseType )
	PY_RO_ATTRIBUTE_DECLARE( pType_->name(), className );

	PyObject * pyGet_cell();
	PY_RO_ATTRIBUTE_SET( cell )

	PY_RO_ATTRIBUTE_DECLARE( this->hasCellEntity() || this->isGetCellPending(),
			hasCell )

	PyObject * pyGet_cellData();
	int pySet_cellData( PyObject * value );

	PyObject * pyGet_databaseID();
	PY_RO_ATTRIBUTE_SET( databaseID )

	AutoBackupAndArchive::Policy shouldAutoBackup() const 
		{ return shouldAutoBackup_; }

	void shouldAutoBackup( AutoBackupAndArchive::Policy value )
	{
		if (BaseAppConfig::shouldLogBackups())
		{
			DEBUG_MSG( "Base::shouldAutoBackup: "
						"Base %d backup policy has changed, from %s to %s.\n",
					this->id(),
					AutoBackupAndArchive::policyAsString(
							this->shouldAutoBackup() ),
					AutoBackupAndArchive::policyAsString( value ));
		}

		shouldAutoBackup_ = value;
	}

	PY_RW_ATTRIBUTE_DECLARE( shouldAutoBackup_, shouldAutoBackup )
	PY_RW_ATTRIBUTE_DECLARE( shouldAutoArchive_, shouldAutoArchive )

	EntityTypePtr pType() const								{ return pType_; }

#if ENABLE_WATCHERS

	static WatcherPtr pWatcher();
	unsigned long getBackupSize () const;
	uint32 backupSize() const {return backupSize_;}
	uint32 archiveSize() const {return archiveSize_;}
protected:
	uint32 backupSize_;
	uint32 archiveSize_;
#endif

protected:
	void onDestroy( bool isOffload );
	void createCellData( BinaryIStream & data );

	void keepAliveTimeout();

	bool callOnPreArchiveCallback();
	
	bool addAttributesToStream( BinaryOStream & stream, int dataDomains );

	void sendExposedForReplayClientPropertiesToCell();

private:
	bool addToStream( WriteDBFlags flags, BinaryOStream & stream,
			PyObjectPtr pCellData );

	void writeBackupDataInternal( BinaryOStream & stream, bool isOffload );
	void readBackupDataInternal( BinaryIStream & stream );

	void backUpTimers( BinaryOStream & stream );
	void backUpAttributes( BinaryOStream & stream );
	void backUpNonDefAttributes( BinaryOStream & stream );
	void backUpCellData( BinaryOStream & stream );

	void restoreTimers( BinaryIStream & stream );
	void restoreAttributes( BinaryIStream & stream );
	void restoreNonDefAttributes( BinaryIStream & stream );
	void restoreCellData( BinaryIStream & stream );

	std::auto_ptr< Mercury::ReplyMessageHandler >
		prepareForCellCreate( const char * errorPrefix );
	bool addCellCreationData( Mercury::Bundle & bundle,
		const char * errorPrefix );
	bool checkAssociatedCellEntity( bool havingEntityGood,
		const char * errorPrefix = NULL );

	bool createCellEntityFromSpaceID( const char * errorPrefix );

	bool sendCreateCellEntityViaBase(
			Mercury::ReplyMessageHandler * pHandler,
			EntityID nearbyID,
			const Mercury::Address & baseAddr,
			const char * errorPrefix );

	bool sendCreateCellEntityToMailBox(
			Mercury::ReplyMessageHandler * pHandler,
			const ServerEntityMailBox & mailBox,
			const char * errorPrefix );

	bool assignAttribute( const ScriptString & propertyName, 
		const ScriptObject & value,
		DataDescription * pDescription );

	bool cacheExposedForReplayClientProperties();

protected:
	Mercury::UDPChannel * pChannel_;

	EntityID			id_;
	DatabaseID			databaseID_;
	EntityTypePtr		pType_;
	PyObjectPtr			pCellData_;

	CellEntityMailBox *	pCellEntityMailBox_;
	SpaceID				spaceID_;

	bool				isProxy_;
	bool				isDestroyed_;

	bool				inDestroy_;

	/// True if a 'create' request has been sent to the CellApp but no reply has
	/// yet been received.
	bool				isCreateCellPending_;

	/// True if a 'create' request has been sent but setCurrentCell has yet to
	/// be called.
	bool				isGetCellPending_;

	/// True if a 'destroy' request has been sent but cellEntityLost has yet to
	/// be called.
	bool				isDestroyCellPending_;

	bool				hasBeenBackedUp_;

	BW::string		cellBackupData_;

	friend class EntityType;

	friend class KeepAliveTimerHandler;

	TimerHandle				keepAliveTimerHandle_;
	uint64					nextKeepAliveStop_;

	PyTimer pyTimer_;

	AutoBackupAndArchive::Policy shouldAutoBackup_;
	AutoBackupAndArchive::Policy shouldAutoArchive_;

	IEntityDelegatePtr	pEntityDelegate_;
	BW::string			templateID_;
private:

	ScriptDict	exposedForReplayClientProperties_;

	// Profiling
	EntityProfiler profiler_;
};

typedef SmartPointer< Base > BasePtr;
typedef Base	            BaseOrEntity;

BW_END_NAMESPACE

#endif // BASE_HPP
