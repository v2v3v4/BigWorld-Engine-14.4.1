#include "script/first_include.hpp"

#ifdef _WIN32
#pragma warning (disable: 4355)
#endif

#include "pyscript/stl_to_py.hpp"

#include "base.hpp"

#include "create_cell_entity_handler.hpp"
#include "global_bases.hpp"
#include "baseapp.hpp"
#include "baseapp_config.hpp"
#include "mailbox.hpp"
#include "proxy.hpp"
#include "py_cell_data.hpp"
#include "py_cell_spatial_data.hpp"
#include "sqlite_database.hpp"

#include "cstdmf/debug.hpp"
#include "cstdmf/memory_stream.hpp"

#include "cellapp/cellapp_interface.hpp"
#include "cellappmgr/cellappmgr_interface.hpp"
#include "db/dbapp_interface.hpp"

#include "entitydef/entity_description_map.hpp"
#include "entitydef/script_data_sink.hpp"
#include "entitydef/entity_delegate_helpers.hpp"
#include "entitydef/property_change.hpp"

#include "connection/replay_data.hpp"

#include "network/channel_owner.hpp"
#include "network/channel_sender.hpp"
#include "network/compression_stream.hpp"
#include "network/forwarding_reply_handler.hpp"
#include "network/nub_exception.hpp"
#include "network/udp_bundle.hpp"
#include "network/udp_channel.hpp"

#include "pyscript/script.hpp"
#include "pyscript/py_data_section.hpp"
#include "pyscript/pyobject_base.hpp"

#include "resmgr/bwresource.hpp"

#include "server/bwconfig.hpp"
#include "server/stream_helper.hpp"
#include "server/writedb.hpp"

#include "entitydef_script/py_component.hpp"
#include "entitydef_script/delegate_entity_method.hpp"
#include "delegate_interface/game_delegate.hpp"

#include <algorithm>

DECLARE_DEBUG_COMPONENT( 0 );


BW_BEGIN_NAMESPACE

PY_BASETYPEOBJECT( Base )

PY_BEGIN_METHODS( Base )
	/*~ function Base.destroy
	 *	@components{ base }
	 *	This method destroys the base part of this entity. It can only be called
	 *	when there is no cell entity. To destroy the cell entity, call
	 *	Base.destroyCellEntity.
	 *
	 *	It may be convenient to call self.destroy in the onLoseCell callback.
	 *	This causes the base entity to be destroyed whenever the cell entity is
	 *	destroyed.
	 *
	 *	This method has two keyword argument:
	 *	@param deleteFromDB If True, the entry in the database associated with
	 *		this entity is deleted. This defaults to False.
	 *	@param writeToDB If True, the properties associated with this entity are
	 *		written to the database. This only occurs if this entity was read
	 *		from the database or has previously been written with
	 *		Base.writeToDB. This defaults to True but is ignored if deleteFromDB
	 *		is True. 
	 */
	PY_METHOD( destroy )

	/*~ function Base.teleport
	 *	@components{ base }
	 *	<i>teleport</i> will teleport this entity's cell instance to the space
	 *	that the argument entity mailbox is in.
	 *
	 *	On arrival in the new space, Entity.onTeleportSuccess is called. This
	 *	can be used to move the entity to the appropriate position within that
	 *	space.
	 *
	 *	@param baseEntityMB The base mailbox of the entity that this entity
	 *	should be moved to. The cell entity associated with this argument will
	 *	be passed into Entity.onTeleportSuccess on success.
	 */
	PY_METHOD( teleport )

	/*~ function Base.createCellEntity
	 *	@components{ base }
	 *	<i>createCellEntity</i> makes a request to create an associated entity
	 *	within a cell.
	 *
	 *	The information used to create the cell entity is stored in the
	 *	cellData property of this entity. This property is a dictionary
	 *	corresponding to the values in the entity's .def file plus a
	 *	"position" and "direction" entries for the entity's position
	 *	and (roll, pitch, yaw) direction as well as optional "spaceID"
	 *	and "templateID" entries. In case if "templateID" entry is present
	 *	the non-spatial properties are not transmitted, instead the "templateID"
	 *	is used on CellApp to populate the cell entity's properties 
	 *	from a local storage.
	 *
	 *	If nearbyMB is not passed in, the "spaceID" entry in cellData is
	 *	used to indicate which space to create the cell entity in.
	 *
	 *	@param nearbyMB an optional mailbox argument which is used to indicate
	 *	which space to create the cell entity in. Ideally, the two entities
	 *	are near so that it is likely that the correct cell will be found
	 *	immediately. Either the base or the cell mailbox of the nearby entity
	 *	can be used; when using base or cell-via-base mailboxes, the entity
	 *	reference will be passed to the __init__() method of the cell entity so
	 *	the nearby entity's position can be used to set the new cell entity's
	 *	position.
	 */
	PY_METHOD( createCellEntity )

	/*~ function Base.createInDefaultSpace
	 *	@components{ base }
	 *	<i>createInDefaultSpace</i> makes a request to create an associated
	 *	entity within the default space.
	 *
	 *	The information used to create the cell entity are stored
	 *	in the cellData property of this entity. This property is a dictionary
	 *	corresponding to the values in the entity's .def file plus a "position",
	 *	"position" and "direction" entries for the entity's position
	 *	and (roll, pitch, yaw) direction as well as optional "spaceID"
	 *	and "templateID" entries. In case if "templateID" entry is present
	 *	the non-spatial properties are not transmitted, instead the "templateID"
	 *	is used on CellApp to populate the cell entity's properties 
	 *	from a local storage.
	 *
	 *	This method is only available if useDefaultSpace is set to true in the
	 *	server's configuration file.
	 */
	PY_METHOD( createInDefaultSpace )

	/*~ function Base.createInNewSpace
	 *	@components{ base }
	 *	<i>createInNewSpace</i> creates an associated entity on a cell in a new
	 *	space. It makes its request to create the new entity via the cell
	 *	manager.
	 *
	 *	The information used to create the cell entity are stored
	 *	in the cellData property of this entity. This property is a dictionary
	 *	corresponding to the values in the entity's	.def file plus a
	 *	"position" and "direction" entries for the entity's position
	 *	and (roll, pitch, yaw) direction as well as optional "spaceID"
	 *	and "templateID" entries. In case if "templateID" entry is present
	 *	the non-spatial properties are not transmitted, instead the "templateID"
	 *	is used on CellApp to populate the cell entity's properties 
	 *	from a local storage.
	 *
	 *	@param shouldPreferThisMachine This optional argument will tell the
	 *	CellAppMgr that the new space has a preference for the physical
	 *	machine on which this BaseApp is running. Load balancing will then give
	 *	preference to CellApps running on this machine when choosing where to
	 *	create a new cell partition.
	 */
	PY_METHOD( createInNewSpace )
	/*~ function Base.destroyCellEntity
	 *	@components{ base }
	 *	<i>destroyCellEntity</i> send a request to destroy the associated cell
	 *	entity. If no cell entity is currently associated then this method
	 *	raises a ValueError.
	 */
	PY_METHOD( destroyCellEntity )
	/*~ function Base.addTimer
	 *	@components{ base }
	 *	The function addTimer registers the timer function <i>onTimer</i> (a
	 *	member function) to be called
	 *	after "initialOffset" seconds. Optionally it can be repeated every
	 *	"repeatOffset" seconds, and have optional user data (an integer that is
	 *	simply passed to the timer function) "userArg".
	 *
	 *	The <i>onTimer</i> function
	 *	must be defined in the base entity's class and accept 2 arguments; the
	 *	internal id of the timer (useful to remove the timer via the "delTimer"
	 *	method), and the optional user supplied integer "userArg".
	 *
	 *	Example:
	 *	@{
	 *	<pre># this provides an example of the usage of addTimer
	 *	import BigWorld
	 *	&nbsp;
	 *	class MyBaseEntity( BigWorld.Base ):
	 *	&nbsp;
	 *	    def __init__( self ):
	 *	        BigWorld.Base.__init__( self )
	 *	&nbsp;
	 *	        # add a timer that is called after 5 seconds, and then every
	 *	        # second with custom user data of 9
	 *	        self.addTimer( 5, 1, 9 )
	 *	&nbsp;
	 *	        # add a timer that is called once after a second, with custom
	 *	        # user data defaulting to 0
	 *	        self.addTimer( 1 )
	 *	&nbsp;
	 *	    # timers added on a Base have "onTimer" called as their timer function
	 *	    def onTimer( self, id, userArg ):
	 *	        print "MyBaseEntity.onTimer called: id %i, userArg: %i" % ( id, userArg )
	 *	        # if this is a repeating timer then call:
	 *	        #     self.delTimer( id )
	 *	        # to remove the timer when it is no longer needed</pre>
	 *	@}
	 *	@param initialOffset initialOffset is a float and specifies the
	 *	interval (in seconds) between registration of the timer, and the time of
	 *	first call.
	 *	@param repeatOffset=0 repeatOffset is an optional float and
	 *	specifies the interval (in seconds) between repeated executions after
	 *	the first execution. The delTimer method must be used to remove the
	 *	timer or this will repeat continuously. Values less than 0 are ignored.
	 *	@param userArg=0 userArg is an optional integer which specifies an
	 *	optional user supplied value to be passed to the "onTimer" method of
	 *	the base entity.
	 *	@return Integer. This function returns the internal id of the timer.
	 *	This can be used with delTimer method to remove the timer.
	 */
	PY_METHOD( addTimer )
	/*~ function Base.delTimer
	 *	@components{ base }
	 *	The function delTimer removes a registered timer function so that it no
	 *	longer executes. Timers that execute only once are automatically removed
	 *	and do not have to be removed using delTimer. If delTimer is called with
	 *	an id that is no longer valid (already removed for example) then an
	 *	error will be logged to the standard log file.
	 *
	 *	See Base.addTimer for an example of timer usage.
	 *	@param id id is an integer that specifies the internal id of the timer
	 *	to be removed.
	 */
	PY_METHOD( delTimer )

	/*~ function Base.registerGlobally
	 *	@components{ base }
	 *	This method registers this base as a global base under a given name.
	 *	Bases may be registered multiple times under different names, but only
	 *	one base may be registered under a specific name.
	 *	@param key The global key by which this base is to be known. This key
	 *		can be any Python type that can be pickled and hashed. Typically
	 *		this would be a string or a tuple of basic types.
	 *
	 *	@param callback A callback that is called when registration completes.
	 *	The callback is called with True (succeeded) or False (failed).
	 *	Failure is usually due to an existing global base with the given name.
	 *	The callback argument is not optional.
	 */
	PY_METHOD( registerGlobally )
	/*~ function Base.deregisterGlobally
	 *	@components{ base }
	 *	This method deregisters this base from the set of global bases.
	 *	Only the given name of the base is deregistered, other names are kept.
	 *	@param name the global name (a string) by which this base is currently
	 *	known.
	 */
	PY_METHOD( deregisterGlobally )

	/*~ function Base.writeToDB
	 *	@components{ base }
	 *	This function saves this entity to the database, so that it can be
	 *	loaded again later if desired.
	 *
	 *	Entities can also be marked as auto-loaded, so that subsequent server
	 *	start-ups will recreate this entity automatically. This auto-load data
	 *	can be cleared from the database using the clear_auto_load tool. See
	 *	the Server Operations Guide -> MySQL Support -> The clear_auto_load
	 *	tool for more details.
	 *
	 *	@param callback This optional argument is a callable object that will be
	 *	called when the response from the database is received. It takes
	 *	two arguments. The first is a boolean indicating success or failure
	 *	and the second is the base entity.
	 *	@param shouldAutoLoad This optional argument specifies whether this
	 *	entity should be loaded from the database when the server starts up.
	 *	@param shouldWriteToPrimary This optional argument specifies whether
	 *	this write should be sent to the primary database instead of the
	 *	secondary database (defaults to False), if secondary database was
	 *	disabled explicitly or baseApp/archivePeriod was set to 0 (implicitly
	 *	disable secondary database), then this parameter will be ignored and
	 *	the enity data are always written to primary database.
	 *	@param explicitDatabaseID This optional argument allows specifying the 
	 *	database ID for the entity. Implies shouldWriteToPrimary = True.
	 *	This parameter is mandatory if the entity has not been written to 
     *  DBApp yet and its type has the ExplicitDatabaseID property set to true.
	 *	This parameter is not allowed if the entity type has the 
	 *	ExplicitDatabaseID property unset, or set to false.
	 */
	PY_METHOD( writeToDB )

	/*~ function Base.getComponent
	 *	@components{ base }
	 *	This method gets a component on an entity
	 */
	PY_METHOD( getComponent )

	PY_PICKLING_METHOD()
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( Base )
	/*~ attribute Base.id
	 *	@components{ base }
	 *	id is the entity's object id. This id is an integer, and
	 *	is common among associated entities across the base, cell and client.
	 *	This attribute is read-only.
	 *	@type Read-only int32
	 */
	PY_ATTRIBUTE( id )
	/*~ attribute Base.databaseID
	 *	@components{ base }
	 *	databaseID is the entity's persistent ID. This id is an INT64 > 0,
	 *	or 0 if the entity is not persistent.
	 *	@type Read-only int64
	 */
	PY_ATTRIBUTE( databaseID )
	/*~ attribute Base.baseType
	 *	@components{ base }
	 *	baseType is the index of the entity's type stored in the entity
	 *	description map (this number is dependant upon the order in
	 *	which the entity definitions ".def" files are read from the
	 *	scripts/entity_defs folder). It is a read-only integer.
	 *	@type Read-only int32
	 */
	PY_ATTRIBUTE( baseType )
	/*~ attribute Base.className
	 *	@components{ base }
	 *
	 *	The class name of the Entity.
	 *	@type Read-only string
	 */
	PY_ATTRIBUTE( className )
	/*~ attribute Base.cell
	 *	@components{ base }
	 *	cell is the CellEntityMailBox for the associated cell
	 *	entity. This attribute is read-only, and raises an AttributeError on
	 *	read if no cell is associated with this base entity.
	 *
	 *	Note that there can be a period of time when the entity has a cell
	 *	mailbox but it does not yet point to the correct CellApp. This is in
	 *	the time period where Base.createCellEntity() has been called but
	 *	the Base.onGetCell() callback has yet to be called. In this state, the
	 *	mailbox can stil be used to send from the base entity directly but
	 *	cannot be used, for example, as an argument to
	 *	BigWorld.createCellEntity().
	 *
	 *	@type Read-only CellEntityMailBox
	 */
	PY_ATTRIBUTE( cell )

	/*~ attribute Base.hasCell
	 *	@components{ base }
	 *  Bool indicating whether or not this Base entity has an associated cell
	 *  entity.	This is the same as calling hasattr( base, "cell" ).
	 *
	 *	This property can be used to decide whether Base.destroyCellEntity()
	 *	should be called instead of Base.destroy().
	 *
	 *	Note that this property is also True when this Base entity is in the
	 *	process of getting a cell entity. For example, when
	 *	Base.createCellEntity has been called but the Base.onGetCell() callback
	 *	has yet to be called.
	 *
	 *	@type Read-only bool
	 */
	PY_ATTRIBUTE( hasCell )

	/*~ attribute Base.cellData
	 *	@components{ base }
	 *	This is a dictionary attribute. Whenever a base entity does
	 *	not have its cell entity part created, the cell entity's properties are
	 *	stored here.
	 *
	 *	If the cell entity is created, these values are used and the cellData
	 *	property is deleted. Besides the cell entity properties specified in the
	 *	entity's type's definition, it also contains position, direction and
	 *	spaceID.
	 *
	 *	@type PyCellData
	 */
	PY_ATTRIBUTE( cellData )

	/*~ attribute Base.isDestroyed
	 *	@components{ base }
	 *	This attribute will be True if the Base Entity has been destroyed.
	 *	@type Boolean
	 */
	PY_ATTRIBUTE( isDestroyed )

	/*~ attribute Base.shouldAutoBackup
	 *	@components{base}
	 *	This attribute determines the policy for automatic backups. 
	 *	If set to True, automatic backups are enabled, if set to False,
	 *	automatic backups are disabled. 
	 *	If set to BigWorld.NEXT_ONLY, automatic backups are enabled for the
	 *	next scheduled time; after the next backup, this is attribute is set to
	 *	False.
	 *	@type True, False or BigWorld.NEXT_ONLY
	 */
	PY_ATTRIBUTE( shouldAutoBackup )

	/*~ attribute Base.shouldAutoArchive
	 *	@components{ base }
	 *	This attribute determines the policy for automatic archival. 
	 *	If set to True, automatic archival is enabled, if set to False,
	 *	automatic archival is disabled. 
	 *	If set to BigWorld.NEXT_ONLY, automatic archival is enabled for the
	 *	next scheduled time; after the next archival, this is attribute is set
	 *	to False.
	 *	@type True, False or BigWorld.NEXT_ONLY
	 */
	PY_ATTRIBUTE( shouldAutoArchive )

	/*~ attribute Base.artificialMinLoad
	 *	@components{ base }
	 *
	 *	This attribute allows this entity to report an amount of
	 *	artificial minimal load. Setting a value to greater than 0 guarantees
	 *	the reported load for the given entity will not be less than the
	 *	artificial minimal load.
	 *
	 *	The value should be in range of [0, +inf). Values less than 1e-6
	 *	will be changed to 0. The default value for this attribute is 0
	 *	which effectively disables artificial load.
	 *
	 *	The value should generally be small, say 0.01 or less.
	 *
	 *	This feature should be used with care. Setting this value too large,
	 *	for example, could adversely affect the load balancing.
	 */
	PY_ATTRIBUTE( artificialMinLoad )

PY_END_ATTRIBUTES()

// Callbacks
/*~ callback Base.onDestroy
 *	@components{ base }
 *	If present, this method is called on the base entity just prior to it being
 *	destroyed as a result of a call to Base.destroy(). This method has no
 *	arguments.
 */ 
/*~ callback Base.onOnload
 *	@components{ base }
 *	If present, this method is called on the base entity if it has been restored
 *	after the offloading of a Base application. This method has no arguments.
 */
/*~ callback Base.onRestore
 *	@components{ base }
 *	If present, this method is called on the base entity if it has been restored
 *	after the failure or (if Base.onOnload doesn't exist) after the offloading of
 *  a Base application. This method has no arguments.
 */
/*~ callback Base.onGetCell
 *	@components{ base }
 *	If present, this method is called on the base entity when it gets a cell
 *	entity after having none. This method has no arguments.
 */
/*~	callback Base.onLoseCell
 *	@components{ base }
 *	If present, this method is called on the base entity when the cell entity
 *	associated it is destroyed. This method has no arguments.
 */
/*~ callback Base.onCreateCellFailure
 *	@components{ base }
 *	If present, this method is called if the creation of cell entity failed.
 *	This method has no arguments.
 */
/*~ callback Base.onWindowOverflow
 *	@components{ base }
 *	If present, this method is called if this entity's channel is close to
 *	overflowing. This threshold is configured by the
 *	baseApp/sendWindowCallbackThreshold setting in bw.xml.
 */
/*~	callback Base.onWriteToDB
 *	@components{ base }
 *	If present, this method is called on the base entity when it is about to
 *	write its data to the database.
 *
 *	Note that calling writeToDB during this callback can lead to an infinite
 *	loop.
 *	@param cellData	cellData contains cell properties that are going to be
 *	stored in database. cellData is dictionary.
 */

/*~	callback Base.onPreArchive
 *	@components{ base }
 *	If present, this method is called before the entity is to be written
 * 	to the database due to the automatic archiving facility. This callback is
 * 	called before the Base.onWriteToDB callback and before the base entity
 * 	requests persistent data from its cell entity. Therefore, unlike
 * 	Base.onWriteToDB, cell properties are not provided. If this callback
 * 	returns False, the archiving operation is aborted. This callback should
 * 	return True to allow the operation to continue. If this callback is not
 * 	present, then the operation is allowed to continue.
 */

// NOTE: We no longer support the updater feature. Will remove in the future
// once we figure out what to do with reloadScript().
/*	callback Base.onMigrate
 *	@components{ base }
 *	If present, this method is called when this base has migrated its Python
 *  class due to the updater process.
 *	@param oldBase The previous instance of this base entity.
 */
/*	callback Base.onMigratedAll
 *	@components{ base }
 *	If present, this method is called when all bases on this Base application
 *	have migrated their Python class due to the updater process.
 */
/*~	callback Base.onTimer
 *	@components{ base }
 *	This method is called when a timer associated with this entity is triggered.
 *	A timer can be added with the Base.addTimer method.
 *	@param timerHandle	The id of the timer.
 *	@param userData	The integer user data passed into Base.addTimer.
 */


// -----------------------------------------------------------------------------
// Section: Base
// -----------------------------------------------------------------------------

namespace
{
uint32 g_numBasesNotDestructed = 0;

uint32 numEntitiesLeaked()
{
	return g_numBasesNotDestructed - BaseApp::instance().bases().size();
}

class WatcherInitialiser
{
public:
	WatcherInitialiser()
	{
		MF_WATCH( "stats/numEntitiesLeaked", numEntitiesLeaked, 
				"The number of entities which have been destroyed but are "
				"still referenced.");
	}
};

WatcherInitialiser g_watcherInitialiser;

class PyBaseComponent : public PyComponent
{
public:
	PyBaseComponent( const IEntityDelegatePtr & delegate,
					 EntityID entityID,
					 const BW::string & componentName,
					 const EntityDescription * pEntityDescription) :
		PyComponent( delegate, entityID, componentName ),
		pEntityDescription_( pEntityDescription )
	{
	}
private:
	virtual DataDescription * findProperty( const char * attribute,
											const char * component )
	{
		DataDescription * pDescription = 
			pEntityDescription_->findProperty( attribute, component );
		
		if (pDescription && !pDescription->isBaseData())
		{
			pDescription = NULL;
		}
		return pDescription;
	}
	
	const EntityDescription * pEntityDescription_;
};


/**
 *	This method gets the IEntityDelegateFactory* from the singleton
 */
static IEntityDelegateFactory * getEntityDelegateFactory()
{
	IGameDelegate * pGameDelegate = IGameDelegate::instance();

	if (!pGameDelegate)
	{
		return NULL;
	}

	//INFO_MSG( "getEntityDelegateFactory: Getting IEntityDelegateFactory\n" );

	return pGameDelegate->getEntityDelegateFactory();
}

} // anonymous namespace

/**
 *	The constructor for a Base
 */
Base::Base( EntityID id, DatabaseID dbID, EntityTypePtr pType ) :
	PyObjectPlus( (PyTypeObject *)(pType->pClass() ), true ),
#if ENABLE_WATCHERS
	backupSize_( 0 ),
	archiveSize_( 0 ),
#endif
	pChannel_(
		new Mercury::UDPChannel( BaseApp::instance().intInterface(),
			Mercury::Address::NONE,
			Mercury::UDPChannel::INTERNAL,
			DEFAULT_INACTIVITY_RESEND_DELAY,
			/* filter: */ NULL,
			Mercury::ChannelID( id ) ) ),
	id_( id ),
	databaseID_( dbID ),
	pType_( pType ),
	pCellData_( NULL ),
	pCellEntityMailBox_(
			new CellEntityMailBox( pType, pChannel_->addr(), id_ ) ),
	spaceID_( 0 ),
	isProxy_( false ),
	isDestroyed_( false ),
	inDestroy_( false ),
	isCreateCellPending_( false ),
	isGetCellPending_( false ),
	isDestroyCellPending_( false ),
	hasBeenBackedUp_( false ),
	cellBackupData_(),
	keepAliveTimerHandle_(),
	pyTimer_( this, id_ ),
	shouldAutoBackup_( AutoBackupAndArchive::YES ),
	shouldAutoArchive_( AutoBackupAndArchive::YES )
{
	INFO_MSG( "Base::Base(%u): %s\n", id_, pType_->name() );

	// Make sure that this class does not have virtual functions. If it does,
	// it screws up the memory layout when we pretend it is a PyInstanceObject.
	MF_ASSERT( (void *)this == (void *)((PyObject *)this) );

	BaseApp::instance().addBase( this );
	++g_numBasesNotDestructed;

	pChannel_->isLocalRegular( false );
	pChannel_->isRemoteRegular( false );

	// Because we rely on a regular flow of incoming packets from the client to
	// trigger sends to the cell, if the client dies there is a risk that the
	// channel from cell to here will overflow because we're not sending it any
	// ACKs.  Activate this option on the channel to prevent this.
	pChannel_->pushUnsentAcksThreshold( pChannel_->windowSize() / 16 );

	// Base channels must auto switch to the incoming address, because if
	// packets are lost before a long teleport (i.e. so old ghost will not hang
	// around), incoming packets (with the setCurrentCell message) will be
	// buffered and the address switch might never happen.
	pChannel_->shouldAutoSwitchToSrcAddr( true );
#if ENABLE_WATCHERS
	pType_->countNewInstance( *this );
#endif
}


/**
 *	This method initialises this base object.
 *
 *	@param pDict		The dictionary for the script object or NULL.
 *						The property values of this dictionary are assumed to be
 *						of correct types for this entity type.
 *	@param pCellData	A dictionary containing the values to create the cell
 *						entity with, or NULL.
 *	@param pLogOnData	A string containing a value provided by the billing
 *						system if this is a Proxy crated by player login, or
 *						NULL.
 *
 *	@return	True if successful, otherwise false.
 */
bool Base::init( PyObject * pDict, PyObject * pCellData,
	const BW::string * pLogOnData )
{
	MF_ASSERT( !PyErr_Occurred() );

	MF_ASSERT( pLogOnData == NULL || this->isProxy() );

	// creation of entity delegate
	if (!this->initDelegate( /*templateID*/ "" ))
	{
		ERROR_MSG( "Base::init(%d): Failed to initialise delegate\n", id_);
		return false;
	}
	
	if (pDict)
	{
		ScriptDict dict( pDict, ScriptDict::FROM_BORROWED_REFERENCE );
		ScriptDict::size_type pos = 0;

		// populate entity properties
		for (ScriptObject key, value; dict.next( pos, key, value ); )
		{
			ScriptString propertyName = key.str( ScriptErrorPrint() );

			const char * attr = propertyName.c_str();

			DataDescription* pDataDescription = 
				this->pType()->description().findCompoundProperty( attr );

			if (pDataDescription && 
				pDataDescription->isComponentised() &&
				pDataDescription->isBaseData())
			{
				continue; // skip base components properties
			}

			if (!this->assignAttribute( propertyName, value, pDataDescription ))
			{
				ERROR_MSG( "Base::init(%d): Failed to assign '%s'\n",
						 id_, attr );
				Script::printError();
			}
		}
		// populate components properties
		if (!populateDelegateWithDict( this->pType()->description(), 
				pEntityDelegate_.get(),	dict, EntityDescription::BASE_DATA ))
		{
			ERROR_MSG("Base::init(%d): Failed to populate delegate with data\n",
					 id_);
			return false;
		}
	}


	if (pCellData)
	{
		pCellData_ = pCellData;
	}

	if (ob_type->tp_init)
	{
		PyObject * pArgs = PyTuple_New( 0 );
		PyObject * pKeyArgs = PyDict_New();
		if (ob_type->tp_init( (PyObject*)this, pArgs, pKeyArgs ) == -1)
		{
			PyErr_Print();
		}

		Py_DECREF( pKeyArgs );
		Py_DECREF( pArgs );

		if (pLogOnData != NULL)
		{
			Script::call( PyObject_GetAttrString( this, "onLoggedOn" ),
				Py_BuildValue( "(s#)",
					pLogOnData->c_str(), pLogOnData->size() ),
				"onLoggedOn", /*okIfFunctionNull:*/true );
		}
	}
	
	if (pEntityDelegate_)
	{
		pEntityDelegate_->onEntityInitialised();
	}
	
	return true;
}

bool Base::init( const BW::string & templateID )
{
	// creation of entity delegate
	if (!this->initDelegate( templateID ))
	{
		ERROR_MSG( "Base::init(%d): "
			"Failed to create delegate from template '%s'\n",
			id_, templateID.c_str() );
		return false;
	}

	Position3D position = Position3D::zero();
	bool isOnGround = false;
	Direction3D direction = Direction3D( Vector3::zero() );

	pEntityDelegate_->getSpatialData( position, isOnGround, direction );

	pCellData_ = PyObjectPtr( new PyCellSpatialData( 
		position, direction, NULL_SPACE_ID, isOnGround, templateID ),
		PyObjectPtr::STEAL_REFERENCE );

	ScriptObject self = ScriptObject( this );
	self.callMethod( "__init__", ScriptErrorPrint( "Base::init" ),
		/* okIfNull */ true );

	pEntityDelegate_->onEntityInitialised();
	return true;
}

bool Base::initDelegate( const BW::string & templateID )
{
	templateID_ = templateID;
	IEntityDelegateFactory * const pDelegateFactory = getEntityDelegateFactory();

	if (!pDelegateFactory)
	{
		if (templateID.empty())
		{
			return true;
		}
		else
		{
			ERROR_MSG( "Base::initDelegate(%d): There is no delegate factory registered\n", id_ );
			return false;
		}
	}

	if (templateID.empty())
	{
		TRACE_MSG( "Base::init(%d): calling createEntityDelegate for '%s' entity",
			this->id(), 
			this->pType()->description().name().c_str() );

		pEntityDelegate_ = pDelegateFactory->createEntityDelegate(
			this->pType()->description(),
			this->id(),
			NULL_SPACE_ID );

		return true;
	}
	else
	{
		TRACE_MSG( "Base::init(%d): calling createBaseEntityDelegateFromTemplate "
			"for '%s' entity with templateID '%s'",
			this->id(),
			this->pType()->description().name().c_str(),
			templateID.c_str() );

		pEntityDelegate_ = pDelegateFactory->createEntityDelegateFromTemplate(
			this->pType()->description(),
			this->id(),
			NULL_SPACE_ID,
			templateID );

		return !!pEntityDelegate_;
	}
}

/**
 *	Destructor
 */
Base::~Base()
{
	INFO_MSG( "Base::~Base(%u): %s\n", id_, pType_->name() );

	// If this assertion triggers, we likely have a reference counting problem.
	MF_ASSERT( isDestroyed_ );

	pChannel_->condemn();
	pChannel_ = NULL;
	--g_numBasesNotDestructed;

#if ENABLE_WATCHERS
	pType_->forgetOldInstance( *this );
#endif
	keepAliveTimerHandle_.cancel();
}


/**
 *	This method contains all of the testing necessary to say whether or not
 *	we have a currently instantiated cell entity.
 */
bool Base::hasCellEntity() const
{
	return pChannel_->isEstablished() && !isGetCellPending_;
}


/**
 *  This method returns true if we should be sending to our cell entity.  The
 *  important difference between this and hasCellEntity() is that it returns
 *  false if we still have a cell entity but we have sent a destroy message to
 *  it.
 */
bool Base::shouldSendToCell() const
{
	return this->hasCellEntity() && !isDestroyCellPending_;
}


/**
 *	This method destroys this base entity.
 */
void Base::onDestroy( bool /* isOffload */ )
{
	// Take ourselves out of the app's base list
	BaseApp::instance().removeBase( this );

	Py_XDECREF( pCellEntityMailBox_ );
	pCellEntityMailBox_ = NULL;

	pyTimer_.cancelAll();
}


/**
 *	This method adds a cellData proprety to this entity. It should only have
 *	this when it does not have the cell entity.
 */
void Base::createCellData( BinaryIStream & data )
{
	if (data.remainingLength() == 0)
	{
		ERROR_MSG( "Base::createCellData: No cell data for %s (%u)\n",
				this->pType()->name(), id_ );
	}

	pCellData_ = PyObjectPtr( new PyCellData( this->pType(), data, false ),
			PyObjectPtr::STEAL_REFERENCE );
}


/**
 *	This method sets a new database id if we havent got one yet
 */
void Base::databaseID( DatabaseID dbID )
{
	databaseID_ = dbID;
}


/**
 *	This method creates a dictionary from the input stream.
 *
 *	@return A new reference to a dictionary.
 */
PyObject * Base::dictFromStream( BinaryIStream & data ) const
{
	ScriptDict pDict = ScriptDict::create();

	// TODO: What should we do if this fails?
	pType_->description().readStreamToDict( data,
			EntityDescription::BASE_DATA, pDict );

	return pDict.newRef();
}


/**
 *	This method is used to inform the base that the cell we send to has changed.
 */
void Base::setCurrentCell( SpaceID spaceID,
	const Mercury::Address & cellAppAddr,
	const Mercury::Address * pSrcAddr, bool shouldReset )
{
	// Make sure that we are still around after any script call.
	PyObjectPtr pThis = this;

	// If we're losing our cell entity, flush the channel.
	if (cellAppAddr == Mercury::Address::NONE)
	{
		if (pChannel_->isEstablished())
		{
			pChannel_->send();

			// We have to reset the channel here because we might get another
			// cell entity later on and that cell entity will expect the channel
			// to be in a reset state.  This will put the channel into the
			// 'wantsFirstPacket_' state, so even if a packet arrives from the
			// old app, it will be dropped.
			pChannel_->reset( Mercury::Address::NONE, false );
		}
	}

	// If we're getting a cell entity or offloading, just switch the address.
	else
	{
		// Usually we don't need to manually switch address here, since the
		// channel has the autoSwitchToSrcAddr flag enabled and it has already
		// been done by Mercury.  We still need to do this when called from
		// emergencySetCurrentCell() however.
		if (shouldReset)
		{
			pChannel_->reset( cellAppAddr );
		}
		else
		{
			pChannel_->setAddress( cellAppAddr );
		}
	}

	if (pCellEntityMailBox_ != NULL)
	{
		bool hadCell = (pCellEntityMailBox_->address().ip != 0);
		bool haveCell = (cellAppAddr.ip != 0);

		pCellEntityMailBox_->address( pChannel_->addr() );
		spaceID_ = spaceID;

		if (hadCell != haveCell)
		{
			isGetCellPending_ = false;

			if (haveCell)
			{
				pCellData_ = NULL;
			}
			else
			{
				cellBackupData_.clear();
			}

			// inform the proxy that the cell entity has gone
			// (even if a new one is requested by the script method below)
			if (this->isProxy() && hadCell)
			{
				((Proxy*)this)->cellEntityDestroyed( pSrcAddr );
			}

			if (haveCell)
			{
				isCreateCellPending_ = false;
				// There might still be stuff waiting for a valid IP address to
				// come along
				pChannel_->send();
			}

			// call the script method notifying it of this event
			char * methodName = (char*)(haveCell ? "onGetCell" : "onLoseCell");
			PyObject * pMethod = PyObject_GetAttrString( this, methodName );

			if (pMethod == NULL)
				PyErr_Clear();

			// inform the proxy that the cell entity is ready
			if (this->isProxy() && this->hasCellEntity())
			{
				((Proxy*)this)->cellEntityCreated();
			}

			if (pMethod)
			{
				Script::call( pMethod, PyTuple_New( 0 ), methodName );
			}
			
			// notify the delegate about this event
			if (pEntityDelegate_)
			{
				if (haveCell)
				{
					//TODO: maybe we should add the handling of this condition to the delegate, 
					//      but meanwhile it's not needed/handled.
					//
					//pEntityDelegate_->onGetCell();
				}
				else
				{
					pEntityDelegate_->onLoseCell();
				}
			}
		}
	}
}


/**
 *	This method is used to inform the base that the cell we send to has changed.
 */
void Base::currentCell( const Mercury::Address & srcAddr,
			const Mercury::UnpackedMessageHeader & header,
			const BaseAppIntInterface::currentCellArgs & args )
{
	this->setCurrentCell( args.newSpaceID, args.newCellAddr,
			&srcAddr );
}


/**
 *	This method handles a message that requests to teleport another entity to
 *	the space of this entity.
 */
void Base::teleportOther( const Mercury::Address & srcAddr,
				const Mercury::UnpackedMessageHeader & header,
				const BaseAppIntInterface::teleportOtherArgs & args )
{
	const EntityMailBoxRef & teleportingMB = args.cellMailBox;

	if (pCellEntityMailBox_ == NULL)
	{
		ERROR_MSG( "Base::teleportOther( %u ): "
					"pCellEntityMailBox_ is NULL while teleporting %u\n",
				id_, teleportingMB.id );
		return;
	}

	if (pCellEntityMailBox_->address().ip == 0)
	{
		ERROR_MSG( "Base::teleportOther( %u ): Cell mailbox has no address while "
					"teleporting %u. isGetCellPending_ = %d\n",
				id_, teleportingMB.id, this->isGetCellPending() );
		return;
	}

	Mercury::Channel & channel = BaseApp::getChannel( teleportingMB.addr );
	Mercury::Bundle & bundle = channel.bundle();

	CellAppInterface::teleportArgs & rTeleportArgs =
		CellAppInterface::teleportArgs::start( bundle, teleportingMB.id );

	rTeleportArgs.dstMailBoxRef = pCellEntityMailBox_->ref();

	channel.send();
}


/**
 *	This method is used to inform the base that the cell we send to has changed.
 *	It is called in emergency situations when a ghost thinks that the base may
 *	still think that it is the real. This is when the CellApp that the ghost
 *	thinks the real is on dies.  These messages should arrive just before the
 *	handleCellAppDeath message does, and are intended to make sure as many of
 *	the lost reals as possible have their cellAddr() set correctly before we do
 *	the big restoreTo() pass in BaseApp::handleCellAppDeath().
 */
void Base::emergencySetCurrentCell( const Mercury::Address & srcAddr,
	const Mercury::UnpackedMessageHeader & header,
	BinaryIStream & data )
{
	SpaceID newSpaceID;
	Mercury::Address newCellAddr;

	data >> newSpaceID >> newCellAddr;

	// If we already knew that the dead CellApp was dead, then this message has
	// arrived after the handleCellAppDeath message.  We've gone to some lengths
	// to prevent this from happening so we should be pretty worried about it.
	IF_NOT_MF_ASSERT_DEV (
		!BaseApp::instance().isRecentlyDeadCellApp( newCellAddr ) )
	{
		ERROR_MSG( "Base::emergencySetCurrentCell: "
			"Got late notification about entity %u on %s from %s\n",
			id_, newCellAddr.c_str(), srcAddr.c_str() );
	}

	// Otherwise just mark this cell entity as being on the soon-to-be-dead app
	// so that handleCellAppDeath() will restore it.
	else if (this->cellAddr() == srcAddr)
	{
		NOTICE_MSG( "Base::emergencySetCurrentCell: "
			"%u was on soon-to-be-dead app %s (not %s as we thought)\n",
			id_, newCellAddr.c_str(), srcAddr.c_str() );

		if (spaceID_ == newSpaceID)
		{
			// this means we missed the offload, so the channel should be
			// incremented
			Mercury::ChannelVersion newChannelVer = 
				Mercury::seqMask( pChannel_->version() + 1 );
			pChannel_->version( newChannelVer );
			this->setCurrentCell( newSpaceID, newCellAddr );
		}
		else
		{
			ERROR_MSG( "Base::emergencySetCurrentCell: "
				"Trying to change the entity %u's space from %u to %u\n",
				id_, spaceID_, newSpaceID );
		}
	}

	// ACK to the CellApp so that it can get closer to ACKing to the CellAppMgr.
	Mercury::ChannelSender sender( BaseApp::getChannel( srcAddr ) );
	sender.bundle().startReply( header.replyID );
}


/**
 *	This method handles a message from the cell entity asking to back up its
 *	data.
 */
void Base::backupCellEntity( BinaryIStream & data )
{
	MF_ASSERT_DEV( size_t( data.remainingLength() ) >=
		sizeof( bool ) +
		sizeof( StreamHelper::AddEntityData ) +
		sizeof( StreamHelper::CellEntityBackupFooter ) );

	bool cellBackupHasWitness;
	data >> cellBackupHasWitness;

	if (this->isProxy())
	{
		static_cast<Proxy *>( this )->cellBackupHasWitness(
				cellBackupHasWitness );
	}

	int size = data.remainingLength();
	cellBackupData_.assign( (char *)data.retrieve( size ), size );
}


/**
 *	This method handles a message from the cell entity asking to write ourself
 *	to the database. It takes the exact same input as backupCellEntity.
 */
void Base::writeToDB( BinaryIStream & data )
{
	this->writeToDB( WRITE_BASE_CELL_DATA, NULL, this->getDBCellData( data ) );
}


/**
 *	Back up a cell entity and extract the cell dict
 */
PyObjectPtr Base::getDBCellData( BinaryIStream & data )
{
	// This will stream off data into cellBackupData_ so we can stream it off 
	// again to create the backup
	this->backupCellEntity( data );

	int length = cellBackupData_.length();

	// Ignore the footer
	length -= sizeof( StreamHelper::CellEntityBackupFooter );

	// This is same as data minus the cellBackupHasWitness_ flag
	MemoryIStream stream( cellBackupData_.data(), length );

	StreamHelper::AddEntityData addEntityData;
	StreamHelper::removeEntity( stream, addEntityData );
	// Template streams don't carry any properties, and it doesn't make much
	// sense to persist a template name.
	// Nothing should have sent us an AddEntityData with a template anyway.
	MF_ASSERT( !addEntityData.hasTemplate() );

	ScriptDict pCellData = ScriptDict::create();

	// Written by RealEntity::writeBackupProperties
	if (!this->pType()->description().
		readStreamToDict( stream, EntityDescription::CELL_DATA,
						  pCellData ) )
	{
		ERROR_MSG( "Base::writeToDB: Could not destream cell dict\n" );
	}

	PyCellData::addSpatialData( pCellData, addEntityData.position, 
								addEntityData.direction, spaceID_ );

	// Finishing the stream as RealEntity::writeBackupProperties adds
	// more information than we are interested in here (volatileInfo,
	// controllers, topSpeed.. etc
	stream.finish();

	return pCellData;
}


/**
 *	This method handles a message from the cell entity informing us that it has
 *	been destroyed.
 */
void Base::cellEntityLost( const Mercury::Address & srcAddr,
		const Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data )
{
	this->createCellData( data );

	isDestroyCellPending_ = false;

	if (this->hasCellEntity())
	{
		this->setCurrentCell( 0, Mercury::Address::NONE, &srcAddr );
	}
	else
	{
		WARNING_MSG( "Base::cellEntityLost: "
				"%u did not think it had a cell entity.\n", id_ );
	}
}


/**
 *	This method handles a message telling us to run a Python method.
 */
void Base::callBaseMethod( const Mercury::Address & srcAddr,
		const Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data )
{
	MethodIndex index;
	data >> index;

	MethodDescription * pMethodDescription =
		pType_->description().base().internalMethod( index );

	if (pMethodDescription != NULL)
	{
		if (pMethodDescription->isComponentised())
		{
			MF_ASSERT(pEntityDelegate_);
			// TODO: processing of call results and error
			MF_ASSERT(header.replyID == Mercury::REPLY_ID_NONE);
			
			if (!pEntityDelegate_->handleMethodCall(*pMethodDescription, data))
			{
				ERROR_MSG( "Base::callBaseMethod: "
					"failed to call method %s on %s entity's delegate",
					pMethodDescription->name().c_str(),
					this->pType()->name());
			}
		}
		else // conventional call to entity method
		{
			if (header.replyID != Mercury::REPLY_ID_NONE)
			{
				pMethodDescription->callMethod( 
					ScriptObject( this, ScriptObject::FROM_BORROWED_REFERENCE ),
					data, 0, header.replyID, &srcAddr, 
					&BaseApp::instance().intInterface() );
			}
			else
			{
				pMethodDescription->callMethod( 
					ScriptObject( this, ScriptObject::FROM_BORROWED_REFERENCE ),
					data );
			}
		}
	}
	else
	{
		ERROR_MSG( "Base::callBaseMethod: "
			"Do not have method with index %d\n", index );

		if (header.replyID != Mercury::REPLY_ID_NONE)
		{
			MethodDescription::sendReturnValuesError(
					"BWInternalError", "Invalid method index",
					header.replyID, srcAddr,
					BaseApp::instance().intInterface() );
		}
	}
}


namespace
{

void sendTwoWayFailure( const char * errorType,
		const char * errorMsg, Mercury::ReplyID replyID,
		const Mercury::Address & addr )
{
	if (replyID != Mercury::REPLY_ID_NONE)
	{
		MethodDescription::sendReturnValuesError( errorType, errorMsg,
				replyID, addr, BaseApp::instance().intInterface() );
	}
}

}


/**
 *	This class is used to relay the response from the cell entity to the
 *	original source.
 */
class TwoWayMethodForwardingReplyHandler : public ForwardingReplyHandler
{
public:
	TwoWayMethodForwardingReplyHandler( const Mercury::Address & srcAddr,
			Mercury::ReplyID replyID ) :
		ForwardingReplyHandler( srcAddr, replyID,
				BaseApp::instance().intInterface() )
	{
	}

protected:
	virtual void appendErrorData( BinaryOStream & stream )
	{
		MethodDescription::addReturnValuesError(
				MethodDescription::createErrorObject( "BWMercuryError",
					"No response from cell entity" ),
				stream );
	}
};


/**
 *  This method handles a message from a CellViaBaseMailBox. It calls
 *  the target method on the cell entity.
 */
void Base::callCellMethod( const Mercury::Address & srcAddr,
		   const Mercury::UnpackedMessageHeader & header,
		   BinaryIStream & data )
{

	if (pCellEntityMailBox_ == NULL)
	{
		ERROR_MSG( "Base::callCellMethod(%u): "
					"Unable to locate cell entity mailbox.\n", id_ );
		data.finish();

		char msg[ 128 ];
		bw_snprintf( msg, sizeof( msg ),
				"Entity %d of type %s has no cell entity\n",
				id_, pType_->name() );

		sendTwoWayFailure( "BWNoSuchCellEntityError", msg,
				header.replyID, srcAddr );

		return;
	}

	MethodIndex methodIndex;
	data >> methodIndex;

	const MethodDescription * pDescription =
			this->pType()->description().cell().internalMethod( methodIndex );

	if (pDescription != NULL)
	{
		std::auto_ptr< Mercury::ReplyMessageHandler > pReplyHandler;

		if (header.replyID != Mercury::REPLY_ID_NONE)
		{
			pReplyHandler.reset( new TwoWayMethodForwardingReplyHandler(
					srcAddr, header.replyID ) );
		}

		BinaryOStream * pBOS = pCellEntityMailBox_->getStream( *pDescription,
				pReplyHandler );

	    if (pBOS == NULL)
		{
			PyErr_Clear();

			ERROR_MSG( "Base::callCellMethod(%u): "
						"Failed to get stream on cell method %s.\n",
						id_, pDescription->name().c_str() );
			data.finish();

			sendTwoWayFailure( "BWInternalError",
					"Failed to get stream from cell mailbox",
					header.replyID, srcAddr );
			return;
		}

		pBOS->transfer( data, data.remainingLength() );
		pCellEntityMailBox_->sendStream();
	}
	else
	{
		ERROR_MSG( "Base::callCellMethod(%u): "
					"Invalid method index (%d) on cell.\n", id_, methodIndex );

		sendTwoWayFailure( "BWInternalError", "Invalid method index",
				header.replyID, srcAddr );
	}
}


/**
 *	This method handles a request message asking for our cellapp address.
 */
void Base::getCellAddr( const Mercury::Address & srcAddr,
		const Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data )
{
	MF_ASSERT( header.replyID != Mercury::UnpackedMessageHeader::NO_REPLY );

	Mercury::Channel & channel = BaseApp::getChannel( srcAddr );
	Mercury::Bundle & bundle = channel.bundle();

	bundle.startReply( header.replyID );

	bundle << (pCellEntityMailBox_? pCellEntityMailBox_->address()
				: Mercury::Address::NONE);

	channel.send();
}


/**
 *	This method destroys this base.
 */
void Base::destroy( bool deleteFromDB, bool writeToDB, bool logOffFromDB )
{
	IF_NOT_MF_ASSERT_DEV( !isDestroyed_ )
	{
		return;
	}

	if (inDestroy_)
	{
		return;
	}

	inDestroy_ = true;

	Script::call( PyObject_GetAttrString( this, "onDestroy" ),
					PyTuple_New( 0 ), "onDestroy", true );

	// TRACE_MSG( "Base(%d)::destroy: deleteFromDB=%d, writeToDB=%d\n",
	//	id_, deleteFromDB, writeToDB );

	keepAliveTimerHandle_.cancel();

	// Inform our backup that we've been destroyed.
	const Mercury::Address backupAddr =
		BaseApp::instance().backupAddrFor( this->id() );

	if (backupAddr != Mercury::Address::NONE)
	{
		Mercury::ChannelSender sender( BaseApp::getChannel( backupAddr ) );
		BaseAppIntInterface::stopBaseEntityBackupArgs::start(
			sender.bundle() ).entityID = this->id();
	}

	if (this->hasWrittenToDB())
	{
		WriteDBFlags flags = 0;

		if (logOffFromDB)
		{
			flags |= WRITE_LOG_OFF;
		}
		if (deleteFromDB)
		{
			flags |= WRITE_DELETE_FROM_DB;
		}
		else if (writeToDB)
		{
			flags |= WRITE_BASE_CELL_DATA;
		}

		this->writeToDB( flags );
	}

	BaseApp::instance().pGlobalBases()->onBaseDestroyed( this );

	this->discard();

	inDestroy_ = false;
}


/**
 *	Remove references to this base, remove from BaseApp's bases_ and hand back
 *	our ID.
 *
 *	Called in Base::destroy(), also called during BaseApp shutdown.
 */
void Base::discard( bool isOffload )
{
	isDestroyed_ = true;

	pEntityDelegate_ = IEntityDelegatePtr();

	// This dodgy hack is because we cannot have virtual methods in instance
	// objects.
	if (this->isProxy())
	{
		static_cast<Proxy *>( this )->onDestroy( isOffload );
	}
	else
	{
		this->onDestroy( isOffload );
	}

	// Add back our id.
	{
		BaseApp * pBaseApp = BaseApp::pInstance();

		if (pBaseApp != NULL)
		{
			pBaseApp->putUsedID( this->id() );
		}
		else
		{
			WARNING_MSG( "Base::destroy: BaseApp is NULL\n" );
		}
	}

	PyObject * pDict = PyObject_GetAttrString( this, "__dict__" );

	if (pDict)
	{
		PyDict_Clear( pDict );
		Py_DECREF( pDict);
	}
	else
	{
		ERROR_MSG( "Base::discard: No __dict__\n" );
		PyErr_Clear();
	}

	Py_DECREF( this );
}


/**
 *	This class handles the response from a writeToDBRequest call to the cell.
 */
class CellDBDataReplyHandler : public Mercury::ReplyMessageHandler
{
public:
	CellDBDataReplyHandler( WriteDBFlags flags, BasePtr pBase,
			WriteToDBReplyStructPtr pReplyStruct):
		flags_( flags ),
		pBase_( pBase ),
		pReplyStruct_( pReplyStruct ) {}
	virtual ~CellDBDataReplyHandler() {}

private:
	void handleMessage( const Mercury::Address& srcAddr,
			Mercury::UnpackedMessageHeader& header,
			BinaryIStream& data, void * /*arg*/ )
	{
		PyObjectPtr pCellData = pBase_->getDBCellData( data );

		if (!pBase_->writeToDB( flags_, pReplyStruct_, pCellData, 
			/*explicitDatabaseID*/ 0 ))
		{
			pReplyStruct_->onWriteToDBComplete( false );
		}

		delete this;
	}

	void handleException( const Mercury::NubException & exception, void * )
	{
		ERROR_MSG( "CellDBDataReplyHandler::handleException( %s %d ): "
				"Failed to write to the database. "
				"Could not get cell data: %s\n", 
			pBase_->pType()->name(),
			pBase_->id(), Mercury::reasonToString( exception.reason() ) );

		if (!pBase_->isDestroyed() && !pBase_->hasCellEntity())
		{
			pBase_->writeToDB( flags_, pReplyStruct_, NULL, 
				/*explicitDatabaseID*/ 0 );
		}
		else
		{
			pReplyStruct_->onWriteToDBComplete( false );
		}

		delete this;
	}

	void handleShuttingDown( const Mercury::NubException & exception, void * )
	{
		WARNING_MSG( "CellDBDataReplyHandler::handleShuttingDown: "
				"Database write in progress for %u\n", pBase_->id() );

		delete this;
	}

	WriteDBFlags flags_;
	BasePtr pBase_;
	WriteToDBReplyStructPtr pReplyStruct_;
};


/**
 *	This class handles the response from a writeEntity call to the database
 *	when we are expecting to get our DBID.
 */
class WriteEntityReplyHandler : public Mercury::ShutdownSafeReplyMessageHandler
{
public:
	WriteEntityReplyHandler( BasePtr pBase,
			WriteToDBReplyStructPtr pReplyStruct ) :
		pBase_( pBase ),
		pReplyStruct_( pReplyStruct ) {}
		virtual ~WriteEntityReplyHandler() {}

private:
	void handleMessage( const Mercury::Address& /*srcAddr*/,
			Mercury::UnpackedMessageHeader& /*header*/,
			BinaryIStream& data, void * /*arg*/ )
	{
		bool success;
		data >> success;

		if (data.remainingLength() == sizeof(DatabaseID))
		{
			DatabaseID dbID;
			data >> dbID;

			if (success)
			{
				pBase_->databaseID( dbID );
			}
		}

		if (!success)
		{
			WARNING_MSG( "WriteEntityReplyHandler::handleMessage: "
							"Initial write failed for %d\n",
						pBase_->id() );

			// TODO: There is a slight bug here. There is a chance that there
			// are other writeEntity calls still outstanding. If any of these
			// return successfully and another writeEntity was sent in the
			// meantime, this entity could be written to the database twice.
			// (ie. have two DBIDs).
			this->onFailure();
		}

		pReplyStruct_->onWriteToDBComplete( success );

		delete this;
	}

	void handleException( const Mercury::NubException &, void * )
	{
		ERROR_MSG( "WriteEntityReplyHandler::handleException: "
					"Failed to write %u to database\n", pBase_->id() );

		this->onFailure();

		pReplyStruct_->onWriteToDBComplete( false );
		delete this;
	}

	void handleShuttingDown( const Mercury::NubException & exception, 
			void * arg )
	{
		WARNING_MSG( "WriteEntityReplyHandler::handleShuttingDown: "
				"Database write in progress for %u\n", pBase_->id() );

		this->handleException( exception, arg );
	}


	/**
	 *	This method resets temporary state of the Base if a DBApp writeEntity
	 *	request fails.
	 */
	void onFailure()
	{
		if (pBase_->databaseID() == PENDING_DATABASE_ID)
		{
			WARNING_MSG( "WriteEntityReplyHandler::onFailure: "
							"Resetting databaseID for entity %d\n",
						pBase_->id() );

			pBase_->databaseID( 0 );
		}
	}


	BasePtr pBase_;
	WriteToDBReplyStructPtr pReplyStruct_;
};


/**
 *	This class handles the response from a writeEntity call to the database
 *	when we are logging off the entity.
 */
class LogOffReplyHandler : public Mercury::ReplyMessageHandler
{
public:
	LogOffReplyHandler( EntityID entityID, MemoryOStream * pPendingStream ) :
		entityID_( entityID ),
		pPendingStream_( pPendingStream ) {}
	virtual ~LogOffReplyHandler()
	{
		delete pPendingStream_;
	}

private:
	void handleMessage( const Mercury::Address& /*srcAddr*/,
			Mercury::UnpackedMessageHeader& /*header*/,
			BinaryIStream& data, void * /*arg*/ )
	{
		bool success;
		data >> success;

		if (!success)
		{
			ERROR_MSG( "LogOffReplyHandler::handleMessage: "
				"Failed to log out entity %d from DB.\n", entityID_ );
		}

		// TODO: Don't need this dbID in this LogOff reply case, we should
		//       be able to avoid sending it from DBApp.
		if (data.remainingLength() == sizeof(DatabaseID))
		{
			DatabaseID dbID;
			data >> dbID;
		}

		delete this;
	}


	void handleException( const Mercury::NubException &, void * )
	{
		INFO_MSG( "LogOffReplyHandler::handleException: "
					"DBApp failed while attempting to write Entity %d. "
						"Retrying...\n",
					entityID_ );

		Mercury::Bundle & bundle = BaseApp::instance().dbApp().bundle();
		bundle.startRequest( DBAppInterface::writeEntity, this );
		bundle.addBlob( pPendingStream_->data(), pPendingStream_->size() );

		BaseApp::instance().dbApp().send();
	}


	void handleShuttingDown( const Mercury::NubException &, void * )
	{
		WARNING_MSG( "LogOffReplyHandler::handleShuttingDown: "
				"Database log off in progress for %d\n", entityID_ );

		delete this;
	}

	EntityID entityID_;
	MemoryOStream * pPendingStream_;
};


/**
 *	This class handles a reply from a writeToDB request via Python script.
 */
class WriteToDBPyReplyHandler : public WriteToDBReplyHandler
{
public:
	WriteToDBPyReplyHandler( BasePtr pBase, PyObjectPtr pScriptHandler ):
		pBase_( pBase ),
		pScriptHandler_( pScriptHandler ) {};

	virtual void onWriteToDBComplete( bool succeeded )
	{
		Py_INCREF( pScriptHandler_.get() );
		Script::call( pScriptHandler_.get(),
				Py_BuildValue( "(OO)",
					succeeded ? Py_True : Py_False,
					pBase_.get() ) );
		delete this;
	}

private:
	BasePtr pBase_;
	PyObjectPtr pScriptHandler_;
};


/**
 *	This class handles the response from a backup call to another baseapp
 *	since we ensure that the backup is no older than the DB entry when the
 *	write operation is complete.
 */
class BackupEntityReplyHandler : public Mercury::ReplyMessageHandler
{
public:
	BackupEntityReplyHandler( WriteToDBReplyStructPtr pReplyStruct ) :
		pReplyStruct_( pReplyStruct ) {}
		virtual ~BackupEntityReplyHandler() {}

private:
	void handleMessage( const Mercury::Address& /*srcAddr*/,
						Mercury::UnpackedMessageHeader& /*header*/,
						BinaryIStream& /*data*/, void * /*arg*/ )
	{
		pReplyStruct_->onBackupComplete();
		delete this;
	}

	void handleException( const Mercury::NubException &, void * )
	{
		// If the backup failed, it implies that there is no backup, thus there
		// is no fault tollerance that can possibly be older than the DB copy.
		pReplyStruct_->onBackupComplete();
		delete this;
	}

	void handleShuttingDown( const Mercury::NubException &, void * )
	{
		// Just ignore. Causes small memory leak.
		delete this;
	}

	WriteToDBReplyStructPtr pReplyStruct_;
};


/**
 *	This method is called when this base entity should be automatically
 *	archived.
 */
void Base::autoArchive()
{
	if (shouldAutoArchive_ == AutoBackupAndArchive::NO)
	{
		return;
	}

	bool success = this->archive();

	if (success && (shouldAutoArchive_ == AutoBackupAndArchive::NEXT_ONLY))
	{
		shouldAutoArchive_ = AutoBackupAndArchive::NO;
	}
}


/**
 *	This method writes the entity to the database. This is called by the
 * 	regular archiving operation.
 */
bool Base::archive()
{
	AUTO_SCOPED_THIS_ENTITY_PROFILE;
	if (this->hasCellEntity() && this->isDestroyCellPending())
	{
		// A bit dodgy, but we skip this archive since, in the vast majority of
		// the cases, the base entity will be destroyed when the cell entity
		// is destroyed. So this entity will be written to the database soon
		// anyway.
		// DEBUG_MSG( "Base::archive: Archiving skipped for entity %lu due to "
		//		"pending cell entity destruction\n", this->id_ );
		return false;
	}

	if (!this->callOnPreArchiveCallback())
	{
		return false;	// Early exit requested by script.
	}

	if (this->isDestroyed())
	{
		return false;
	}

	return this->writeToDB( WRITE_BASE_CELL_DATA );
}

/**
 *	This method writes the entity to the database. If there is a cell part and
 *	no cell data is passed in, the cell data is requested.
 */
bool Base::writeToDB( WriteDBFlags flags, WriteToDBReplyHandler * pHandler /*=NULL*/,
		PyObjectPtr pCellData /* = NULL */,
		DatabaseID explicitDatabaseID /* = 0 */ )
{
	AUTO_SCOPED_THIS_ENTITY_PROFILE;
	WriteToDBReplyStructPtr pReplyStruct =
		new WriteToDBReplyStruct( pHandler );

	if (this->hasCellEntity() && !pCellData)
	{
		return this->requestCellDBData( flags, pReplyStruct );
	}

	return this->writeToDB( flags, pReplyStruct, pCellData, 
		explicitDatabaseID );
}


/**
 *	This method request the database information from the cell.
 */
bool Base::requestCellDBData( WriteDBFlags flags, WriteToDBReplyStructPtr pReplyStruct )
{
	Mercury::Bundle & bundle = this->cellBundle();
	bundle.startRequest( CellAppInterface::writeToDBRequest,
	   new CellDBDataReplyHandler( flags, this, pReplyStruct ) );
	bundle << id_;

	this->sendToCell();

	return true;
}


/**
 *	This method writes the entity to the database.
 */
bool Base::writeToDB( WriteDBFlags flags, WriteToDBReplyStructPtr pReplyStruct,
		PyObjectPtr pCellData, DatabaseID explicitDatabaseID )
{
	AUTO_SCOPED_THIS_ENTITY_PROFILE;
	// Nothing to do.
	if (flags == 0)
	{
		pReplyStruct->onWriteToDBComplete( false );
		return false;
	}

	if ((flags & WRITE_DELETE_FROM_DB) != 0)
	{
		flags |= WRITE_LOG_OFF;
		flags &= ~WRITE_BASE_CELL_DATA;
	}

	if ((flags & WRITE_LOG_OFF) && !this->hasWrittenToDB())
	{
		// we don't need to tell the database to log off things that are not in
		// the database.
		return false;
	}

	if (!this->pType()->canBeOnCell())
	{
		flags &= ~WRITE_CELL_DATA;
	}

	if (!pCellData)
	{
		if (flags & WRITE_CELL_DATA)
		{
			if (!pCellData_)
			{
				ERROR_MSG( "Base::writeToDB: "
					"pCellData_ for %u is NULL, aborting\n",
					id_ );

				return false;
			}

			pCellData = pCellData_;
		}
		else
		{
			pCellData = Py_None;
		}
	}


	if (flags & WRITE_BASE_DATA)
	{
		// perform a call back notifying the base entity that we are about to
		// write its data (in case it needs to adjust anything to present
		// the right data to persist)
		Script::call( PyObject_GetAttrString( this, "onWriteToDB" ),
				Py_BuildValue( "(O)", pCellData.get() ),
				"onWriteToDB", true );
	}

	// The call to backupBaseNow may end up offloading this base if we are
	// currently in the process of retiring. Hold onto a pointer so that we
	// still exist afterwards. 
	BasePtr pThis( this );

	bool shouldBackup = !inDestroy_;
	if (shouldBackup)
	{
		if (pReplyStruct->expectsReply())
		{
			BaseApp::instance().backupBaseNow( *this,
				new BackupEntityReplyHandler( pReplyStruct ) );
		}
		else
		{
			BaseApp::instance().backupBaseNow( *this );
		}
	}
	else
	{
		// If an entity has just been destroyed we don't want to force a
		// a backup now. This avoids re-adding the entity to the backup hash
		// after it has received the remove from backup request.
		if (pReplyStruct->expectsReply())
		{
			pReplyStruct->onBackupComplete();
		}
	}

	uint32 persistentSize;

	// TODO: Should support writing the the secondary database when writing to
	// primary database is pending.
	SqliteDatabase* pSecondaryDB = BaseApp::instance().pSqliteDB();
	bool shouldWriteToSecondary = (pSecondaryDB && 
		this->hasFullyWrittenToDB() &&
		!(flags & WRITE_AUTO_LOAD_MASK) &&
		!(flags & WRITE_DELETE_FROM_DB) &&
		!(flags & WRITE_TO_PRIMARY_DATABASE) &&
		!(flags & WRITE_EXPLICIT_DBID));

	if (shouldWriteToSecondary)
	{
		MemoryOStream stream;

		if (!this->addToStream( flags, stream, pCellData ))
		{
			return false;
		}

		GameTime gameTime = BaseApp::instance().time();

		pSecondaryDB->writeToDB( databaseID_, this->pType()->id(), gameTime,
				stream, pReplyStruct );

		if (flags & WRITE_EXPLICIT)
		{
			BaseApp::instance().commitSecondaryDB();
		}

		persistentSize = stream.size() + sizeof( databaseID_ ) +
				sizeof( this->pType()->id() ) + sizeof( gameTime ) +
				sizeof( id_ );
	}

	bool isLogOff =(flags & WRITE_LOG_OFF);

	if (!shouldWriteToSecondary || isLogOff)
	{
		BaseApp & baseApp = BaseApp::instance();

		DatabaseID dbID = (flags & WRITE_EXPLICIT_DBID) ? 
				explicitDatabaseID : databaseID_;

		if (dbID == PENDING_DATABASE_ID)
		{
			DEBUG_MSG( "Base::writeToDB: %s %d is still pending initial "
					"database write.\n",
				this->pType()->name(), id_ );
			// Set the dbID to 0, which will distribute this operation to
			// the Alpha DBApp.
			dbID = 0;
		}

		const DBAppGateway & dbApp = baseApp.dbAppGatewayFor( dbID );
		
		if (dbApp.address().isNone())
		{
			ERROR_MSG( "Base::writeToDB: No DBApp is available, "
				"data for %" PRI64 " has been lost\n", dbID );
			return false;
		}

		Mercury::Channel & channel = baseApp.getChannel( dbApp.address() );
		std::auto_ptr< Mercury::Bundle > pBundle( channel.newBundle() );

		BinaryOStream * pStream = pBundle.get();

		bool shouldSetToPending = false;

		// We expect a reply if we are getting a database id. That is, this is
		// the first time that we are being written to the database.
		if (pReplyStruct->expectsReply() ||
			(!isLogOff && !this->hasWrittenToDB()))
		{
			shouldSetToPending = (databaseID_ == 0);
			pBundle->startRequest( DBAppInterface::writeEntity,
				new WriteEntityReplyHandler( this, pReplyStruct ) );
		}
		else if (isLogOff && this->hasWrittenToDB())
		{
			// Owned by LogOffReplyHandler
			MemoryOStream * pMemoryStream = new MemoryOStream;
			pStream = pMemoryStream;
			pBundle->startRequest( DBAppInterface::writeEntity,
				new LogOffReplyHandler( this->id(), pMemoryStream ) );
		}
		else
		{
			pBundle->startMessage( DBAppInterface::writeEntity );
		}

		if (flags & WRITE_EXPLICIT_DBID)
		{
			MF_ASSERT_DEV( (flags & WRITE_EXPLICIT_DBID) && (databaseID_ == 0) );
			MF_ASSERT_DEV( (flags & WRITE_EXPLICIT_DBID) && (explicitDatabaseID != 0) );
		}

		*pStream << flags << this->pType()->id() << dbID << this->id();

		if (shouldSetToPending)
		{
			// This needs to be done after databaseID_ is streamed on.
			databaseID_ = PENDING_DATABASE_ID;
		}

		if (!this->addToStream( flags, *pStream, pCellData ))
		{
			return false;
		}

		if (flags & WRITE_BASE_CELL_DATA)
		{
			*pStream << BaseApp::instance().time();
		}

		// If the stream is not already on the bundle, place it on now.
		if (pStream != pBundle.get())
		{
			pBundle->addBlob(
				static_cast< MemoryOStream * >( pStream )->data(),
				static_cast< MemoryOStream * >( pStream )->size() );
		}

		persistentSize = pBundle->size();

		channel.send( pBundle.get() );
	}

#if ENABLE_WATCHERS
	pType_->updateArchiveSize( *this, persistentSize );

	archiveSize_ = persistentSize;
#endif

	return true;
}

/**
 *	This function calls the onPreArchive() callback if it exists.
 * 	Returns true if we should continue with the writeToDB() operation, or false
 * 	if we should abort.
 */
bool Base::callOnPreArchiveCallback()
{
	PyObjectPtr pFunction( PyObject_GetAttrString( this, "onPreArchive" ),
			PyObjectPtr::STEAL_REFERENCE );
	if (pFunction)
	{
		PyObjectPtr pResult( PyObject_CallObject( pFunction.get(), NULL ),
				PyObjectPtr::STEAL_REFERENCE );
		if (pResult.get() == Py_True)
		{
			return true;
		}
		else if (pResult.get() == Py_False)
		{
			return false;
		}
		else if (!pResult)
		{
			PyErr_Print();
			WARNING_MSG( "Base::callOnPreArchiveCallback: Continuing "
					"writeToDB() for entity %u despite error in "
					"onPreArchive() callback\n", this->id_ );
			return true;
		}
		else
		{
			WARNING_MSG( "Base::callOnPreArchiveCallback: onPreArchive() "
					"should return either True or False. Continuing with "
					"writeToDB() of entity %u despite invalid return value\n",
					this->id_ );
			return true;
		}
	}
	else
	{
		PyErr_Clear();
	}

	return true;
}

/**
 *	This method writes attributes of the entity related to the given dataDomains
 *	to stream.
 */
bool Base::addAttributesToStream( BinaryOStream & stream, int dataDomains )
{
	const EntityDescription & entityDesc = this->pType()->description();
	ScriptObject self( this, ScriptObject::FROM_BORROWED_REFERENCE );
	ScriptDict attrs = createDictWithAllProperties(entityDesc, 
		self, pEntityDelegate_.get(), dataDomains);
	
	if (!attrs)
	{
		return false;
	}
	return entityDesc.addDictionaryToStream( attrs, stream, dataDomains );
}

/**
 * 
 */
void Base::backupTo( const Mercury::Address & addr, 
		Mercury::Bundle & bundle,
		bool isOffloading, Mercury::ReplyMessageHandler * pHandler )
{
	if (pHandler)
	{
		bundle.startRequest( BaseAppIntInterface::backupBaseEntity,
							 pHandler );
	}
	else
	{
		bundle.startMessage( BaseAppIntInterface::backupBaseEntity );
	}

	bundle << isOffloading;
	this->writeBackupData( bundle, /*isOnload*/ isOffloading );

	if (isOffloading)
	{
		this->offload( addr );
	}

	hasBeenBackedUp_ = true;
}

/**
 *	This method writes the information needed to back up this base entity to the
 *	input stream.
 */
void Base::writeBackupData( BinaryOStream & stream, bool isOffload )
{
	AUTO_SCOPED_THIS_ENTITY_PROFILE;
	MF_ASSERT( !isDestroyed_ );

	int initialStreamLength = stream.size();

	// Read from BaseApp::createBaseFromStream
	stream << id_ << pType_->id() << templateID_ << databaseID_;

	// Scoped as stream is written to in CompressionOStream destructor.
	{
		CompressionOStream compressedStream( stream,
				pType_->description().internalNetworkCompressionType() );
		this->writeBackupDataInternal( compressedStream, isOffload );
	}

	int backupSize = stream.size() - initialStreamLength;

	if (backupSize > BaseAppConfig::backupSizeWarningLevel())
	{
		WARNING_MSG( "Base::writeBackupData: sizeWarningLevel for base entity "
						"exceeded (%s %u of size %d)\n",
					pType_->description().name().c_str(), id_, backupSize );
	}

#if ENABLE_WATCHERS
	pType_->updateBackupSize( *this, backupSize );
	backupSize_ = backupSize;
#endif
}


/**
 *	This method is called by writeBackupData and is passed a stream that handles
 *	compression appropriately.
 */
void Base::writeBackupDataInternal( BinaryOStream & stream, bool isOffload )
{
	// The following is read by Base::restore.
	stream << this->cellAddr();

	stream << isOffload;
	if (isOffload)
	{
		pChannel_->addToStream( stream );
	}

	stream  << isCreateCellPending_ << isGetCellPending_ << 
		isDestroyCellPending_ << spaceID_ << 
		shouldAutoBackup_ << shouldAutoArchive_ << 
		cellBackupData_;

	// This needs to be done because class Base cannot have virtual methods.
	if (this->isProxy())
	{
		// This needs to be added here because the client address must be the
		// next thing on the stream.
		static_cast< Proxy * >( this )->writeBackupData( stream );
	}

	this->backUpTimers( stream );
	this->backUpAttributes( stream );
	this->backUpCellData( stream );
}


/**
 *	This method offloads the base entity to the BaseApp at the destination
 *	address.
 *
 *	@param dstAddr 	The destination BaseApp (internal address).
 */
void Base::offload( const Mercury::Address & dstAddr )
{
	BaseApp & baseApp = BaseApp::instance();
	baseApp.makeLocalBackup( *this );
	baseApp.addForwardingMapping( id_, dstAddr );

	if (this->cellAddr() != Mercury::Address::NONE)
	{
		Mercury::Channel & channel = 
			baseApp.intInterface().findOrCreateChannel( this->cellAddr() );
		Mercury::Bundle & bundle = channel.bundle();
		CellAppInterface::onBaseOffloadedArgs & rOnBaseOffloadedArgs =
			CellAppInterface::onBaseOffloadedArgs::start( bundle, this->id() );
		rOnBaseOffloadedArgs.newBaseAddr = dstAddr;

		DEBUG_MSG( "Base( %s %u )::offload: Now on baseapp %s\n",
				   pType_->name(), id_, dstAddr.c_str() );

		channel.send();
	}

	// Stop the Proxy trying to disable its Witness when it offloads
	// its client.
	pChannel_->reset( Mercury::Address::NONE, false );

	if (this->isProxy())
	{
		static_cast< Proxy * >( this )->offload( dstAddr );
	}

	this->discard( /* isOffload */ true );
}


/**
 *	This method writes this entity's timers to a backup stream.
 */
void Base::backUpTimers( BinaryOStream & stream )
{
	pyTimer_.backUpTimers( stream );
}


/**
 *	This method writes this entity's Python attributes to a backup stream.
 */
void Base::backUpAttributes( BinaryOStream & stream )
{
	if (!this->addAttributesToStream( stream, EntityDescription::BASE_DATA ))
	{
		CRITICAL_MSG( "Base::backUpAttributes(%u): Failed writing %s to stream\n",
			id_,
			this->pType()->description().name().c_str() );
	}

	this->backUpNonDefAttributes( stream );
}


/**
 *	This method writes attributes not in the def file to a backup stream.
 */
void Base::backUpNonDefAttributes( BinaryOStream & stream )
{
	if (BaseAppConfig::backUpUndefinedProperties())
	{
		// Stream on the dictionary members that are not in the def
		PyObject * pDict = PyObject_GetAttrString( this, "__dict__" );
		if (pDict)
		{
			const EntityDescription & entityDesc = this->pType()->description();

			PyObject * pKey;
			PyObject * pValue;
			Py_ssize_t pos = 0;

			while (PyDict_Next( pDict, &pos, &pKey, &pValue ))
			{
				char * key = PyString_AsString( pKey );
				DataDescription * pDD = entityDesc.findProperty( key );

				if (((pDD == NULL) || !pDD->isBaseData()) &&
					!entityDesc.isTempProperty( key ))
				{
					stream << key;
					BW::string value = BaseApp::instance().pickle( 
						ScriptObject( pValue,
								ScriptObject::FROM_BORROWED_REFERENCE ) );
					stream << value;

					if (value.empty())
					{
						ERROR_MSG( "Base::backup: Failed to pickle member "
								"'%s' of entity %u\n", key, id_ );
					}
				}
			}

			Py_DECREF( pDict );
		}
		else
		{
			PyErr_Print();
		}
	}

	// Terminated with an empty string.
	stream << "";
}


/**
 *	This method writes cellData to a backup stream.
 */
void Base::backUpCellData( BinaryOStream & stream )
{
	// cellData is not like other attributes as it is not in the __dict__
	// member.
	if (pCellData_)
	{
		stream << uint8(1);
		PyCellData::addToStream( stream, pType_, pCellData_.get(),
				/*addPosAndDir:*/true, /*addPersistentOnly:*/false );
	}
	else
	{
		stream << uint8(0);
	}
}


/**
 *	This method restores this entity from the input backup information.
 */
void Base::readBackupData( BinaryIStream & stream )
{
	CompressionIStream compressedStream( stream );
	this->readBackupDataInternal( compressedStream );
}


/**
 *	This method is called by readBackupData with a stream that handles
 *	compression.
 */
void Base::readBackupDataInternal( BinaryIStream & stream )
{
	Mercury::Address cellAddr;
	stream >> cellAddr;
	pChannel_->setAddress( cellAddr );

	bool hasChannel;
	stream >> hasChannel;
	if (hasChannel)
	{
		pChannel_->initFromStream( stream, cellAddr );
	}

	if (pCellEntityMailBox_)
		pCellEntityMailBox_->address( this->cellAddr() );

	stream >> isCreateCellPending_ >> isGetCellPending_ >> 
		isDestroyCellPending_ >> spaceID_ >> 
		shouldAutoBackup_ >> shouldAutoArchive_ >> 
		cellBackupData_;

	Mercury::Address clientAddr;
	if (this->isProxy())
	{
		clientAddr = static_cast< Proxy * >( this )->readBackupData(
			stream, hasChannel );
	}

	this->restoreTimers( stream );
	this->restoreAttributes( stream );
	this->restoreCellData( stream ); // Must be last. Consumes rest of data.

	// Holding onto ourselves to ensure that the entity isn't destroyed in
	// onRestore() 
	BasePtr pThis( this );

	MF_ASSERT( !PyErr_Occurred() );

	PyObject * pFunc = NULL;
	if (hasChannel)
	{
		pFunc = PyObject_GetAttrString( this, "onOnload" );
	}

	if (!pFunc)
	{
		PyErr_Clear();
		pFunc = PyObject_GetAttrString( this, "onRestore" );
		if (!pFunc)
		{
			PyErr_Clear();
		}
	}

	if (pFunc)
	{
		Script::call( pFunc, PyTuple_New( 0 ), "Base::readBackupDataInternal", true );
	}

	if (!this->isDestroyed() && this->isProxy())
	{
		static_cast< Proxy * >( this )->onRestored( hasChannel, clientAddr );
	}
}


/**
 *	This method restores this entity's timers from a backup stream.
 */
void Base::restoreTimers( BinaryIStream & stream )
{
	pyTimer_.restoreTimers( stream );
}


/**
 *	This method restores this entity's Python attributes from a backup stream.
 */
void Base::restoreAttributes( BinaryIStream & stream )
{
	class Visitor : public IDataDescriptionVisitor
	{
		BinaryIStream & stream_;
		PyObject * pBase_;
		IEntityDelegate * pDelegate_;

	public:
		Visitor( BinaryIStream & stream, 
				 PyObject * pBase,
				 IEntityDelegate * pDelegate) :
			stream_( stream ),
			pBase_( pBase ),
			pDelegate_(pDelegate)
		{}

		bool visit( const DataDescription & dataDesc )
		{
			if (dataDesc.isComponentised())
			{
				MF_ASSERT( pDelegate_ );
				if (!pDelegate_ ||
					!pDelegate_->updateProperty( stream_, 
												dataDesc, 
												/*isPersistentOnly*/false))
				{
					ERROR_MSG("Base::restoreAttributes failed to update"
							  " component attribute '%s.%s'",
							 dataDesc.componentName().c_str(),
							 dataDesc.name().c_str());
				}
			}
			else
			{
				ScriptDataSink sink;
				dataDesc.createFromStream( stream_, sink,
					/* isPersistentOnly */ false );

				ScriptObject pObj = sink.finalise();

				if (pObj)
				{
					if (PyObject_SetAttrString( pBase_,
							const_cast< char * >( dataDesc.name().c_str() ),
							pObj.get() ) == -1)
					{
						PyErr_Print();
					}
				}
			}
			return true;
		}
	};

	// Save the database ID so it doesn't complain when we set identifiers
	DatabaseID dbID = databaseID_;
	databaseID_ = 0;

	Visitor visitor( stream, this, pEntityDelegate_.get() );

	this->pType()->description().visit(EntityDescription::BASE_DATA, visitor );

	databaseID_ = dbID;

	this->restoreNonDefAttributes( stream );
}


/**
 *	This method restores non def files attributes from a backup stream.
 */
void Base::restoreNonDefAttributes( BinaryIStream & stream )
{
	// Stream off the attributes not in the def file.
	BW::string attrName;
	stream >> attrName;

	while (!attrName.empty())
	{
		BW::string pickleData;
		stream >> pickleData;

		ScriptObject pObj = BaseApp::instance().unpickle( pickleData );
		if (pObj)
		{
			if (PyObject_SetAttrString( this,
				const_cast< char * >( attrName.c_str() ), pObj.get() ) == -1)
			{
				PyErr_Print();
			}
		}
		else
		{
			ERROR_MSG( "Base::restore: Could not unpickle %s\n",
					attrName.c_str() );
		}

		stream >> attrName;
	}
}


/**
 *	This method restores cellData from a backup stream.
 */
void Base::restoreCellData( BinaryIStream & stream )
{
	uint8 hasCellData;
	stream >> hasCellData;
	if (hasCellData)
	{
		pCellData_ = new PyCellData( pType_, stream, false );
	}
	else
	{
		pCellData_ = NULL;
	}
}


/**
 *	This method gets a mailbox ref for this base.
 */
EntityMailBoxRef Base::baseEntityMailBoxRef() const
{
	EntityMailBoxRef embr;
	embr.init( id_,
		BaseApp::instance().intInterface().address(),
		(this->isServiceFragment() ? 
			EntityMailBoxRef::SERVICE : 
			EntityMailBoxRef::BASE),
		pType_->id() );
	return embr;
}


/**
 *	This method reloads the script for this base.
 */
void Base::reloadScript()
{
	AUTO_SCOPED_THIS_ENTITY_PROFILE;
	pType_ = EntityType::getType( pType_->id() );
	MF_ASSERT( pType_ );

	if (!pType_->pClass())
	{
		MF_ASSERT( BaseApp::instance().isServiceApp() &&
				this->isServiceFragment() );
		return;
	}

	if (PyObject_SetAttrString( this, "__class__", pType_->pClass() ) == -1)
	{
		ERROR_MSG( "Base::reloadScript: Setting __class__ failed\n" );
		PyErr_Print();
	}
}


/**
 *	This method migrates the script for this base to the next version.
 *	It is very similar to the reload method.
 */
bool Base::migrate()
{
	AUTO_SCOPED_THIS_ENTITY_PROFILE;
	// if we're already gone then don't bother migrating us
	if (isDestroyed_) return true;

	EntityTypePtr pOldType = pType_, pNewType;
	pNewType = pType_ = EntityType::getType( pOldType->name() );

	// see if this entity type has been destroyed
	if (!pType_ ||
			(pType_->id() != pOldType->id()) ||
			!pType_->canBeOnBase())
	{
		WARNING_MSG( "Base::migrate: "
			"Entity id %u type %s has been made obsolete "
			"(absent or type id mismatch %d => %d)\n",
			id_, pOldType->name(), pOldType->id(), pNewType?pNewType->id():-1 );
		pType_ = pOldType;	// in case we call e.g. onClientDead...
		return false;
	}

	if (!pType_->pClass())
	{
		MF_ASSERT( this->isServiceFragment() );
		return false;
	}

	// see if we can't smoothly set in the script type
	PyObject * pNewBase = this;
	if (PyObject_SetAttrString( this, "__class__", pType_->pClass() ) == -1)
	{
		// set the class by some other means ... hmmm,
		// I think I need to ask Murf about this one
		// possibly create a new base and replace our entry with it...
		// but for now just delete the base (!)
		WARNING_MSG( "Base::migrate: "
			"Entity id %u type %s has been made obsolete "
			"(could not change __class__ to new class)\n",
			id_, pType_->name() );
		PyErr_Print();
		pType_ = pOldType;
		return false;
	}

	// Optimise for case where we are only reloading scripts.
	if (pOldType != pType_)
	{
		// deal with the cell data
		if (PyCellData::Check( pCellData_.get() ))
			((PyCellData*)pCellData_.get())->migrate( pType_ );
	}

	// call the migrate method if it has one
	Script::call( PyObject_GetAttrString( pNewBase, "onMigrate" ),
		Py_BuildValue("(O)",(PyObject*)this), "onMigrate", true/*okIfFnNull*/ );

	// and we are done! sweet!
	return true;
}

/**
 *	This method is called when all bases have been migrated
 */
void Base::migratedAll()
{
	if (isDestroyed_) return;

	// call the onMigratedAll method if it has one
	Script::call( PyObject_GetAttrString( this, "onMigratedAll" ),
		PyTuple_New(0), "onMigratedAll", true/*okIfFnNull*/ );
}


#if ENABLE_WATCHERS

BW_END_NAMESPACE

#include "pyscript/pywatcher.hpp"

BW_BEGIN_NAMESPACE

/**
 * 	This method returns the generic watcher for Bases.
 */
WatcherPtr Base::pWatcher()
{
	static DirectoryWatcherPtr pWatcher = NULL;

	if (pWatcher == NULL)
	{
		pWatcher = new DirectoryWatcher();
		Base	*pNull = NULL;

		pWatcher->addChild( "type",
							new SmartPointerDereferenceWatcher(
								makeWatcher( &EntityType::name ) ),
							&pNull->pType_);

		pWatcher->addChild( "backupSize", makeWatcher( &Base::getBackupSize ) );

		pWatcher->addChild( "archiveSize",
			makeWatcher( &Base::archiveSize_ ) );

		pWatcher->addChild( "properties", new SimplePythonWatcher() );

		pWatcher->addChild( "profile",
							EntityProfiler::pWatcher(),
							&pNull->profiler_ );
	}

	return pWatcher;
}

unsigned long Base::getBackupSize() const
{
	if (backupSize_ == 0)
	{
		MemoryOStream measuringStream( 300 );
		const_cast<Base *>( this )->writeBackupData( measuringStream, false );
	}
	return backupSize_;
}

#endif // ENABLE_WATCHERS

// -----------------------------------------------------------------------------
// Section: Base script related methods
// -----------------------------------------------------------------------------

/**
 *	This method implements the 'get' method for the cell attribute.
 */
PyObject * Base::pyGet_cell()
{
	if (!MainThreadTracker::isCurrentThreadMain())
	{
		PyErr_Format( PyExc_AttributeError,
				"Base.cell is not available in background threads" );
		return NULL;
	}

	if (!pCellEntityMailBox_)
	{
		MF_ASSERT( isDestroyed_ );

		PyErr_Format( PyExc_AttributeError, "Entity %d is destroyed", id_ );
		return NULL;
	}
	else if ((pCellEntityMailBox_->address().ip == 0) &&
			 !this->isGetCellPending())
	{
		PyErr_SetString( PyExc_AttributeError, "Cell not created yet" );
		return NULL;
	}
	else
		return Script::getData( pCellEntityMailBox_ );
}


/**
 *	This method implements the 'get' method for the cellData attribute.
 */
PyObject * Base::pyGet_cellData()
{
	if (!pCellData_)
	{
		PyErr_SetString( PyExc_AttributeError, "No cellData attribute" );
		return NULL;
	}

	if (PyCellData::Check( pCellData_.get() ))
	{
		pCellData_ = static_cast<PyCellData *>( pCellData_.get() )->
							createPyDictOnDemand();
	}

	PyObject * pResult = pCellData_.get();
	Py_INCREF( pResult );

	return pResult;
}


/**
 *	This method implements the 'set' method for the cellData attribute.
 */
int Base::pySet_cellData( PyObject * value )
{
	if (PyDict_Check( value ) || PyCellData::Check( value ))
	{
		pCellData_ = value;
		return 0;
	}
	else
	{
		PyErr_SetString( PyExc_TypeError,
				"cellData must be set to a dictionary" );
		return -1;
	}
}


/**
 *	This method returns the database ID of the entity. If this entity does not
 *	have a database entry, 0 is returned.
 */
PyObject * Base::pyGet_databaseID()
{
	return Script::getData( (databaseID_ != PENDING_DATABASE_ID) ?
								databaseID_ : 0 );
}


/**
 *	This method is responsible for getting script attributes associated with
 *	this object.
 */
ScriptObject Base::pyGetAttribute( const ScriptString & attrObj )
{
	const char * attr = attrObj.c_str();
	// This macro checks through our methods and also the attributes and methods
	// of ancestors.

	if (isDestroyed_ &&
		strcmp( attr, "isDestroyed" ) != 0 &&
		strcmp( attr, "id" ) != 0 &&
		attr[0] != '_')
	{
		WARNING_MSG( "Base.%s should not be accessed since "
			"%s entity id %d is destroyed\n", attr, pType_->name(), id_ );
		// ... but continue for now
	}

	if (pEntityDelegate_)
	{
		// look up for delegate methods
		const MethodDescription * const pMethod =
			pType_->description().base().find( attr );

		if (pMethod && pMethod->isComponentised())
		{
			return ScriptObject( 
				new DelegateEntityMethod( pEntityDelegate_, pMethod, id_ ),
				ScriptObject::FROM_NEW_REFERENCE );
		}
	}
	
	return PyObjectPlus::pyGetAttribute( attrObj );
}


/**
 *	This method is responsible for setting script attributes associated with
 *	this object.
 */
bool Base::pySetAttribute( const ScriptString & attrObj,
	const ScriptObject & value )
{
	const char * attr = attrObj.c_str();

	DataDescription * pDescription =
		this->pType()->description().findProperty( attr );

	if ((pDescription != NULL) &&
			pDescription->isBaseData() &&
			pDescription->isIdentifier() &&
			this->hasWrittenToDB())
	{
		PyErr_Format( PyExc_ValueError,
				"Cannot set identifier property %s.%s "
				"for entity %d already in the database",
			this->pType()->name(),
			attr,
			id_ );
		return false;
	}

	return this->assignAttribute( attrObj, value, pDescription );
}


/**
 *	Assigns an attribute, canonicalising the data type if it is a property.
 *
 *	If the value is of incorrect type, a TypeError is raised.
 *
 *	@param attr 			The name of the property to assign.
 *	@param value			The value of the property to assign.
 *	@param pDataDescription	The data description for the name. If NULL, 
 *							the potential property is looked up from the
 *							attribute. If no such property exists, a normal
 *							attribute assignment will be performed.
 *
 *	@return 0 on success, otherwise -1 with the Python error state raised.
 */
bool Base::assignAttribute( const ScriptString & attrObj, 
		const ScriptObject & value, DataDescription * pDataDescription )
{
	if ((pDataDescription == NULL) || !pDataDescription->isBaseData())
	{
		const char * attr = attrObj.c_str();
		if (BaseAppConfig::warnOnNoDef() &&
				!this->pType()->description().isTempProperty( attr ) &&
				!isAdditionalProperty<Base>( attr ))
		{
			WARNING_MSG( "Base::pySetAttribute: "
					"%s.%s is not a %sdef file property or temp property\n",
				this->pType()->name(),
				attr,
				pDataDescription ? "BASE " : "" );
		}

		if (strcmp( attr, "shouldAutoBackup" ) == 0 &&
				BaseAppConfig::shouldLogBackups())
		{
			DEBUG_MSG( "Base::pySetAttribute: "
						"Base %d backup policy has changed, from %s to %s.\n",
					this->id(),
					AutoBackupAndArchive::policyAsString(
							this->shouldAutoBackup() ),
					AutoBackupAndArchive::policyAsString(
							static_cast< AutoBackupAndArchive::Policy >(
								static_cast< ScriptInt >( value ).asLong() )));
		}

		return this->PyObjectPlus::pySetAttribute( attrObj, value );
	}

	ScriptObject pRealNewValue( value );
		
	pRealNewValue = pDataDescription->dataType()->attach( 
		pRealNewValue, NULL, 0 );

	if (!pRealNewValue)
	{
		PyErr_Format( PyExc_TypeError,
				"%s.%s is a BASE property and must be set to the "
				"correct type - %s",
			this->pType()->name(), 
			pDataDescription->name().c_str(),
			pDataDescription->dataType()->typeName().c_str() );
		return false;
	}
	return this->PyObjectPlus::pySetAttribute( attrObj, pRealNewValue );
}


/**
 *	This method cache and send any exposed for replay base properties
 *	to cell for recording.
 *	Note: When Proxy::addCreateBasePlayerToChannelBundle() is called
 *		this method should be called together at the same time.
 */
void Base::sendExposedForReplayClientPropertiesToCell()
{
	if (!this->cacheExposedForReplayClientProperties() ||
		!this->shouldSendToCell())
	{
		return;
	}

	const EntityDescription & edesc = pType_->description();
	Mercury::Bundle & bundle = this->cellBundle();
	MemoryOStream data;

	if (!edesc.addDictionaryToStream( exposedForReplayClientProperties_, data,
			EntityDescription::BASE_DATA ))
	{
		ERROR_MSG( "Base::sendExposedForReplayClientPropertiesToCell: "
				"Add exposed base properties failed\n" );
		return;
	}

	bundle.startMessage( CellAppInterface::recordClientProperties );
	bundle << this->id();
	bundle.transfer( data, data.remainingLength() );
	this->sendToCell();
}

/**
 *	This method cache the current exposed for replay base properties
 *	which will be used for sending to cell as reply property updates
 *	or as part of cell entity creation initial data.
 */
bool Base::cacheExposedForReplayClientProperties()
{
	const EntityDescription & entityDesc = pType_->description();

	const uint propertyCount = entityDesc.clientServerPropertyCount();

	ScriptObject base( this, ScriptObject::FROM_BORROWED_REFERENCE );

	exposedForReplayClientProperties_ = ScriptDict::create();

	for (uint i = 0; i < propertyCount; ++i)
	{
		const DataDescription * pDataDesc =
			entityDesc.clientServerProperty( i );

		if (pDataDesc->isBaseData() && pDataDesc->isExposedForReplay())
		{
			ScriptObject value = pDataDesc->getAttrFrom( base );
			if (!pDataDesc->insertItemInto( exposedForReplayClientProperties_,
					value ))
			{
				ERROR_MSG( "Base::cacheExposedForReplayClientProperties: "
						"Could not insert to dict,"
						" entity description %s, property: %s\n",
						entityDesc.name().c_str(), pDataDesc->name().c_str() );

				Script::printError();
				return false;
			}
		}
	}
	return exposedForReplayClientProperties_.size() != 0;
}


/**
 *	This method allow script to get a component
 */
PyObject * Base::getComponent( const BW::string & name )
{
	if (!pType_->description().hasComponent( name ))
	{
		PyErr_Format( PyExc_ValueError,
			"Unable to find component %s", name.c_str() );
		return NULL;
	}
	return new PyBaseComponent( pEntityDelegate_, id_, name, 
		&pType_->description() );
}

/**
 *
 */
bool Base::teleport( const EntityMailBoxRef & nearbyBaseMB )
{
	if (nearbyBaseMB.id == 0)
	{
		PyErr_SetString( PyExc_ValueError,
			"Mailbox not yet initialised. This may have come from a base "
				"entity that has not yet had Base.onGetCell called." );
		return false;
	}

	if (!this->hasCellEntity())
	{
		if (this->isGetCellPending())
		{
			PyErr_Format( PyExc_ValueError,
					"Entity %u has a pending cell creation", id_ );
		}
		else
		{
			PyErr_Format( PyExc_ValueError,
					"Entity %u does not have a cell part", id_ );
		}
		return false;
	}

	EntityMailBoxRef::Component component = nearbyBaseMB.component();

	if ((component != EntityMailBoxRef::BASE) &&
		(component != EntityMailBoxRef::CELL_VIA_BASE))
	{
		PyErr_Format( PyExc_ValueError,
					"Expected BASE mailbox but got %s mailbox\n",
					nearbyBaseMB.componentName() );
		return false;
	}

	Mercury::Channel & channel = BaseApp::getChannel( nearbyBaseMB.addr );
	Mercury::Bundle & bundle = channel.bundle();

	BaseAppIntInterface::setClientArgs::start( bundle ).id = nearbyBaseMB.id;

	BaseAppIntInterface::teleportOtherArgs::start( bundle ).cellMailBox =
		pCellEntityMailBox_->ref();

	channel.send();

	return true;
}


/**
 *	This method prepares this entity for creating an associated cell entity.
 *
 *	@return If successful, a new ReplyMessageHandler is returned to handle the
 *		reply message, otherwise NULL is returned and the Python error state is
 *		set.
 */
std::auto_ptr< Mercury::ReplyMessageHandler >
	Base::prepareForCellCreate( const char * errorPrefix )
{
	if (!this->checkAssociatedCellEntity( false, errorPrefix ))
	{
		return std::auto_ptr< Mercury::ReplyMessageHandler >( NULL );
	}

	if (!this->pType()->canBeOnCell())
	{
		PyErr_Format( PyExc_AttributeError, "%sBase %d has no cell script.",
				errorPrefix, int(id_) );
		return std::auto_ptr< Mercury::ReplyMessageHandler >( NULL );
	}

	Mercury::ReplyMessageHandler * pHandler =
		new CreateCellEntityHandler( this );

	isCreateCellPending_ = true;
	isGetCellPending_ = true;

	return std::auto_ptr< Mercury::ReplyMessageHandler >( pHandler );
}


/**
 *	This method implements the base's script method to create an associated
 *	entity on the cell based on the spaceID property in cellData.
 */
bool Base::createCellEntityFromSpaceID( const char * errorPrefix )
{
	if (!pCellData_)
	{
		PyErr_Format( PyExc_AttributeError,
			"%sBase %d does not have a cellData property", errorPrefix, id_ );
		return false;
	}

	SpaceID spaceID = PyCellData::getSpaceID( pCellData_ );

	if (spaceID == NULL_SPACE_ID)
	{
		PyErr_Format( PyExc_ValueError,
				"%sEntity %d does not have a valid spaceID in its cellData",
				errorPrefix, id_ );
		return false;
	}

	return this->createInSpace( spaceID, errorPrefix );
}


/**
 *	This method implements the base's script method to create an associated
 *	entity on the cell.
 */
bool Base::createCellEntity( const ServerEntityMailBoxPtr & pNearbyMB )
{
	static const char* errorPrefix = "Base.createCellEntity: ";

	if (!this->checkAssociatedCellEntity( /* havingEntityGood */ false,
		errorPrefix ))
	{
		return false;
	}

	if (!pNearbyMB)
	{
		return this->createCellEntityFromSpaceID( errorPrefix );
	}

	if (pNearbyMB->address().ip == 0)
	{
		PyErr_Format( PyExc_ValueError,
			"%sCell mailbox not yet initialised. This may have come from a "
				"base entity that has not yet had Base.onGetCell called.",
			errorPrefix );
		return false;
	}

	std::auto_ptr< Mercury::ReplyMessageHandler > pHandler(
		this->prepareForCellCreate( errorPrefix ) );

	if (!pHandler.get())
	{
		DEBUG_MSG( "Base::createCellEntity: "
					"Failed prepareForCellCreate for %u.\n", id_ );
		return false;
	}

	bool result = this->sendCreateCellEntityToMailBox( pHandler.get(),
		*pNearbyMB, errorPrefix );

	pHandler.release(); // Handler deletes itself on callback.

	return result;
}

bool Base::sendCreateCellEntity( Mercury::ReplyMessageHandler * pHandler,
		EntityID nearbyID,
		const Mercury::Address & cellAddr,
		const char * errorPrefix )
{
	// We don't use the channel's own bundle here because the streaming might
	// fail and the message might need to be aborted halfway through.
	Mercury::UDPBundle bundle;

	bundle.startRequest( CellAppInterface::createEntityNearEntity, pHandler );

	bundle << nearbyID;

	// stream on the entity channel version
	bundle << this->channel().version();

	bundle << false; /* isRestore */

	// See if we can add the necessary data to the bundle
	if (!this->addCellCreationData( bundle, errorPrefix ))
	{
		DEBUG_MSG( "Base::sendCreateCellEntity: "
				"Failed addCellCreationData for %u.\n", id_ );

		isCreateCellPending_ = false;
		isGetCellPending_ = false;

		return false;
	}

	BaseApp::getChannel( cellAddr ).send( &bundle );

	return true;
}


/**
 *
 */
bool Base::sendCreateCellEntityViaBase(
							Mercury::ReplyMessageHandler * pHandler,
							EntityID nearbyID,
							const Mercury::Address & baseAddr,
							const char * errorPrefix )
{
	Mercury::Channel & channel = BaseApp::getChannel( baseAddr );
	Mercury::Bundle & bundle = channel.bundle();

	BaseAppIntInterface::setClientArgs::start( bundle ).id = nearbyID;

	bundle.startRequest( BaseAppIntInterface::getCellAddr,
			new CreateCellEntityViaBaseHandler( this, pHandler, nearbyID ) );

	channel.send();

	return true;
}


/**
 *	This method sends a request to create a cell entity to the supplied
 *	ServerEntityMailBox.
 *	If the ServerEntityMailBox is a Base or CellViaBase mailbox, the target
 *	Base will be asked for its Cell mailbox first
 */
bool Base::sendCreateCellEntityToMailBox(
							Mercury::ReplyMessageHandler * pHandler,
							const ServerEntityMailBox & mailBox,
							const char * errorPrefix )
{
	EntityMailBoxRef::Component component = mailBox.component();

	if ((component == EntityMailBoxRef::CELL) ||
			(component == EntityMailBoxRef::BASE_VIA_CELL))
	{
		if (!this->sendCreateCellEntity( pHandler, mailBox.id(),
				mailBox.address(), errorPrefix ))
		{
			return false;
		}
	}
	else if ((component == EntityMailBoxRef::BASE) ||
		(component == EntityMailBoxRef::CELL_VIA_BASE))
	{
		if (!this->sendCreateCellEntityViaBase( pHandler, mailBox.id(),
				mailBox.address(), errorPrefix ))
		{
			return false;
		}
	}
	else
	{
		PyErr_Format( PyExc_TypeError, "%sCannot use a %s mailbox",
			errorPrefix, mailBox.componentName() );
		return false;
	}

	return true;
}


/**
 *	This method implements the base's script method to create an associated
 *	entity in the default space.
 */
bool Base::createInDefaultSpace()
{
	const char * errorPrefix = "Base.createInDefaultSpace: ";
	if (!BaseAppConfig::useDefaultSpace())
	{
		PyErr_Format( PyExc_NotImplementedError,
			"%suseDefaultSpace is set to false", errorPrefix );
		return false;
	}

	// The default space has spaceID 1.
	return this->createInSpace( 1, errorPrefix );
}


/**
 *	This method creates the cell entity associated with this entity into the
 *	input space.
 */
bool Base::createInSpace( SpaceID spaceID, const char * pyErrorPrefix )
{
	BaseApp & app = BaseApp::instance();

	// TODO:
	// As an optimisation, try to find a cell entity mailbox for an existing
	// base entity that is in the same space.
	//
	// This is currently not implemented as there is a potential race-condition.
	// The entity may currently be in the same space but may be in a different
	// space by the time the createCellEntity message arrives.

	std::auto_ptr< Mercury::ReplyMessageHandler > pHandler(
		this->prepareForCellCreate( pyErrorPrefix ) );

	if (!pHandler.get())
	{
		return false;
	}

	Mercury::Channel & channel = BaseApp::getChannel( app.cellAppMgrAddr() );
	// We don't use the channel's own bundle here because the streaming might
	// fail and the message might need to be aborted halfway through.
	std::auto_ptr< Mercury::Bundle > pBundle( channel.newBundle() );
	pBundle->startRequest( CellAppMgrInterface::createEntity, pHandler.get() );
	*pBundle << spaceID;

	// stream on the entity channel version
	*pBundle << this->channel().version();

	*pBundle << false; /* isRestore */

	// See if we can add the necessary data to the bundle
	if (!this->addCellCreationData( *pBundle, pyErrorPrefix ))
	{
		isCreateCellPending_ = false;
		isGetCellPending_ = false;

		return false;
	}

	channel.send( pBundle.get() );
	pHandler.release(); // Handler deletes itself on callback.

	return true;
}


/**
 *	This method implements the base's script method to create an associated
 *	entity on a cell in a new space.
 */
PyObject * Base::py_createInNewSpace( PyObject * args, PyObject * kwargs )
{
	const char * errorPrefix = "Base.createEntityInNewSpace: ";

	PyObject * pPreferThisMachine = NULL;

	static char * keywords[] = 
	{
		const_cast< char * >( "shouldPreferThisMachine" ),
		NULL
	};

	if (!PyArg_ParseTupleAndKeywords( args, kwargs,
		"|O:Base.createEntityInNewSpace", keywords, &pPreferThisMachine ))
	{
		return NULL;
	}

	std::auto_ptr< Mercury::ReplyMessageHandler > pHandler(
		this->prepareForCellCreate( errorPrefix ) );

	if (!pHandler.get())
	{
		return NULL;
	}

	bool shouldPreferThisMachine = false;

	if (pPreferThisMachine)
	{
		shouldPreferThisMachine = PyObject_IsTrue( pPreferThisMachine );
	}

	Mercury::Channel & channel = 
		BaseApp::getChannel( BaseApp::instance().cellAppMgrAddr() );

	// We don't use the channel's own bundle here because the streaming might
	// fail and the message might need to be aborted halfway through.
	std::auto_ptr< Mercury::Bundle > pBundle( channel.newBundle() );

	// Start a request to the Cell App Manager.
	pBundle->startRequest( CellAppMgrInterface::createEntityInNewSpace,
			pHandler.get() );

	*pBundle << shouldPreferThisMachine;

	*pBundle << this->channel().version();

	*pBundle << false; /* isRestore */

	// See if we can add the necessary data to the bundle
	if (!this->addCellCreationData( *pBundle, errorPrefix ))
	{
		isCreateCellPending_ = false;
		isGetCellPending_ = false;

		return NULL;
	}

	// Send it to the Cell App Manager.
	channel.send( pBundle.get() );
	pHandler.release(); // Now owned by Mercury.

	Py_RETURN_NONE;
}


/**
 *	This method adds creation data to a bundle for cell representation
 *	of this base entity. It sets a Python error and returns false on failure.
 */
bool Base::addCellCreationData( Mercury::Bundle & bundle,
	const char * errorPrefix )
{
	const EntityDescription & desc = this->pType()->description();
	if (!desc.canBeOnCell())
	{
		PyErr_Format( PyExc_AttributeError, "%sBase %d is has no cell script.",
				errorPrefix, id_ );
		return false;
	}

	if (!pCellData_)
	{
		PyErr_Format( PyExc_AttributeError, "%sBase has no cellData property",
			errorPrefix );
		ERROR_MSG( "Base::addCellCreationData: %u has no cellData.\n", id_ );
		return false;
	}

	const BW::string templateID = PyCellData::getTemplateID( pCellData_ );

	StreamHelper::AddEntityData entityData( this->id(),
		PyCellData::getPos( pCellData_ ),
		PyCellData::getOnGround( pCellData_ ),
		desc.index(),
		PyCellData::getDir( pCellData_ ),
		!templateID.empty(), // hasTemplate
		BaseApp::instance().intInterface().address() ); // baseAddr

	StreamHelper::addEntity( bundle, entityData );

	// TODO: Make CellAppMgrInterface::createEntityInNewSpace accept
	// persistent data only since initial bundle from DBApp only contains
	// persistent data. The following call will destream the original DBApp
	// data and restream it with default values for non-persistent data.
	// The flag for the type of data in the stream could be included in
	// AddEntityData's flags, since that's how isTemplate works now.

	// Ready by:
	//     CellApp's Entity::readRealDataInEntityFromStreamForInitOrRestore.

	if (!templateID.empty())
	{
		bundle << templateID;
	}
	else
	{
		if (!PyCellData::addToStream( bundle, this->pType(),
			pCellData_.get(), /* addPosAndDir */ false,
			/* addPersistentOnly */ false ))
		{
			PyErr_Format( PyExc_ValueError,
				"%sFailed to create stream from cellData",
				errorPrefix );
			ERROR_MSG( "Base::addCellCreationData: %u failed to "
				"stream cellData.\n", id_ );
			return false;
		}
	}

	// TOKEN_ADD( bundle, "DB End" );

	// Add the entity stuff
	StreamHelper::addRealEntity( bundle );
	// For now we never create the witness here, instead we wait for
	// it to tell us its current cell. Not sure if that is altogether
	// good or not. Probably not :)
	bundle << '-';	// no Witness

	// Append any exposed for replay BASE_AND_CLIENT properties
	MemoryOStream data;
	if (exposedForReplayClientProperties_ &&
			exposedForReplayClientProperties_.size() > 0 &&
			!desc.addDictionaryToStream(
				exposedForReplayClientProperties_, data,
				EntityDescription::BASE_DATA ))
	{
		PyErr_Format( PyExc_ValueError,
			"%sFailed to stream exposed base properties",
			errorPrefix );

		ERROR_MSG( "Base::addCellCreationData( %u ): "
				"Failed to stream exposed base properties.\n",
			id_ );
		return false;
	}

	bundle.writePackedInt( data.remainingLength() );
	bundle.transfer( data, data.remainingLength() );

	return true;
}


/**
 *	This method checks whether or not this base has an associated cell entity,
 *	setting a Python error and returning false if it does not.
 */
bool Base::checkAssociatedCellEntity( bool havingEntityGood,
	const char * errorPrefix )
{
	if (isDestroyed_)
	{
		PyErr_Format( PyExc_ValueError, "%sBase %d is destroyed",
			errorPrefix, id_ );
		return false;
	}
	else if (isCreateCellPending_)
	{
		PyErr_Format( PyExc_ValueError, "%sBase %d is in the process of "
			"creating an associated cell entity.", errorPrefix, id_ );
		return false;
	}
	else if (isGetCellPending_)
	{
		PyErr_Format( PyExc_ValueError, "%sBase %d is waiting for contact from "
			"its created associated cell entity.\n", errorPrefix, id_ );
		return false;
	}
	else if (!havingEntityGood && this->hasCellEntity())
	{
		PyErr_Format( PyExc_ValueError, "%sBase %d already "
			"has an associated cell entity.", errorPrefix, id_ );
		return false;
	}
	else if (havingEntityGood && !this->hasCellEntity())
	{
		PyErr_Format( PyExc_ValueError, "%sBase %d does not "
			"have an associated cell entity.", errorPrefix, id_ );
		return false;
	}

	return true;
}


/**
 *	This method is called when we find out whether or not creating an
 *	entity on the cell succeeded.
 */
void Base::cellCreationResult( bool success )
{
	if (isDestroyed_)
	{
		return;
	}

	// isCreateCellPending_ is also clear before onGetCell is called
	if (!isCreateCellPending_)
	{
		// This may occur in a very rare situation. If the cell was created via
		// the CellAppMgr, it's possible (although unlikely) for the CellApp to
		// have responded with success but the CellAppMgr responds with failure.
		// This occurrs when the CellApp crashes while a 
		// CreateEntityReplyHandler is outstanding but after setCurrentCell.

		if (!success)
		{
			ERROR_MSG( "Base::cellCreationResult: Ignoring failure after "
					   "setCurrentCell\n" );
		}
		return;
	}

	isCreateCellPending_ = false;

	if (!success)
	{
		isGetCellPending_ = false;
		Script::call( PyObject_GetAttrString( this, "onCreateCellFailure" ),
				PyTuple_New( 0 ), "onCreateCellFailure", true );
		DEBUG_MSG( "Base::cellCreationResult: Failed for %u\n", id_ );
	}
}


/**
 *	This method sends data on this base entity's bundle to its cell entity.
 */
void Base::sendToCell()
{
	if (this->hasCellEntity())
	{
		pChannel_->send();
	}
	else
	{
		ERROR_MSG( "Base::sendToCell: %u has no cell. bundle.size() = %d\n",
			   id_, pChannel_->bundle().size() );
	}
}


/**
 *	This method is called to restore the cell entity after its cell application
 *	has died unexpectedly.
 *
 *	@return True if restoring is successful. If the cell entity has not been
 *		backed up, false is returned
 */
bool Base::restoreTo( SpaceID spaceID, const Mercury::Address & cellAppAddr )
{
	// TODO: We should probably pass in a bundle and just add the message to
	// it. It is likely to restore a number of entities to the same cell
	// application so we should use that same bundle.

	const size_t footerSize = sizeof( StreamHelper::CellEntityBackupFooter );

	if (cellBackupData_.size() < footerSize)
	{
		WARNING_MSG( "Base::restoreTo: Entity %u has not been backed up\n",
			id_ );

		return false;
	}

	// Need to store this because it will be blown away by setCurrentCell() and
	// we need to pass it into RestoreToReplyHandler().
	const Mercury::Address deadAddr = this->cellAddr();

	const Mercury::InterfaceElement * pToCall;
	Mercury::Address addressToCall;

	// To avoid issue with brown-out cellApps, increase channel version further.
	// This is particularly important for the case of teleport to same cellApp 
	// (different space)
	Mercury::ChannelVersion newChannelVer = 
		Mercury::seqMask( pChannel_->version() + 2 );
	pChannel_->version( newChannelVer );

	if (cellAppAddr == Mercury::Address::NONE)
	{
		INFO_MSG( "Base::restoreTo: "
					"Restoring %u via CellAppMgr. Was on space %u on %s\n",
				id_, spaceID_, deadAddr.c_str() );
		addressToCall = BaseApp::instance().cellAppMgrAddr();
		pChannel_->reset( Mercury::Address::NONE, false );
		pToCall = &CellAppMgrInterface::createEntity;
	}
	else
	{
		INFO_MSG( "Base::restoreTo: Restoring %u to %s. Was on space %u on %s\n",
				  id_, cellAppAddr.c_str(), spaceID_, deadAddr.c_str() );
		// Set the current cell now, so that if the cellapp we're trying to restore
		// to dies, we'll figure out that this entity needs to be restored somewhere
		// else.  We reset the channel because we know we're going to be talking to
		// a new cell entity.
		addressToCall = cellAppAddr;
		this->setCurrentCell( spaceID, cellAppAddr,
			/* pSrcAddr: */ NULL, /* shouldReset: */ true );
		pToCall = &CellAppInterface::createEntity;
	}

	MemoryOStream exposedForReplayClientData;
	if (exposedForReplayClientProperties_ &&
			exposedForReplayClientProperties_.size() > 0 &&
			!pType_->description().addDictionaryToStream(
				exposedForReplayClientProperties_, exposedForReplayClientData,
				EntityDescription::BASE_DATA ))
	{
		ERROR_MSG( "Base::restoreTo( %u ): "
				"Failed to stream exposed base properties for restore.\n",
			id_ );
		return false;
	}

	// Send the restoreEntity message to the cellapp
	Mercury::Channel & channel = BaseApp::getChannel( addressToCall );
	Mercury::Bundle & bundle = channel.bundle();

	// We should wait at least as long as we wait for entity creation, as the
	// CellApps are likely to be pretty overloaded at the moment with trying to
	// restore all the dead entities.
	bundle.startMessage( *pToCall );

	bundle << spaceID;

	// We increment the channel version in Channel::reset() instead of setting
	// it back to 0 because we need it to be strictly increasing to guard
	// against browned-out apps coming back from the dead and sending with their
	// channel version, which might be higher than ours if we reset it to 0.
	// Since we're not resetting it, we need to send it to the Cell entity so it
	// can sync up with us.
	bundle << pChannel_->version();

	bundle << true /*isRestore*/;

	/*
	// It would be possible to add this here instead of being put on by
	// Entity::backup
	bundle << id_;
	bundle << this->pType()->id();
	*/
	MF_ASSERT( cellBackupData_.length() >= size_t( footerSize ) );
	bundle.addBlob( cellBackupData_.data(),
			cellBackupData_.length() - footerSize );

	// Append any exposed-for-replay BASE_AND_CLIENT properties.
	bundle.writePackedInt( exposedForReplayClientData.remainingLength() );
	bundle.transfer( exposedForReplayClientData,
		exposedForReplayClientData.remainingLength() );

	if (this->isProxy())
	{
		Proxy * pProxy = static_cast< Proxy * >( this );
		pProxy->proxyRestoreTo();
	}

	channel.send();

	return true;
}


/**
 *	This method adds base and cell data to a stream.
 */
bool Base::addToStream( WriteDBFlags flags, BinaryOStream & stream,
		PyObjectPtr pCellData )
{
	if (flags & WRITE_BASE_DATA)
	{
		bool isOK = this->addAttributesToStream(
				stream,
				EntityDescription::BASE_DATA |
				EntityDescription::ONLY_PERSISTENT_DATA );

		if (!isOK)
		{
			ERROR_MSG( "Base::addToStream(%u): "
					"Writing base attribute data failed.\n",
				id_ );
			return false;
		}
	}

	if (flags & WRITE_CELL_DATA)
	{
		bool isOK = PyCellData::addToStream( stream, this->pType(),
						pCellData.get(), true, true );

		if (!isOK)
		{
			ERROR_MSG( "Base::addToStream: Writing cell data failed for %u.\n",
				   id_ );
			return false;
		}
	}

	return true;
}


/**
 *	This method destroys this object when the script says so.
 */
PyObject * Base::py_destroy( PyObject * args, PyObject * kwargs )
{
	if (PyTuple_Size( args ) != 0)
	{
		PyErr_SetString( PyExc_TypeError, "Only expecting keyword arguments" );
		return NULL;
	}

	if (this->hasCellEntity() || this->isGetCellPending())
	{
		PyErr_SetString( PyExc_ValueError,
				"Still has cell entity. Use Base.destroyCellEntity" );

		return NULL;
	}

	if (isDestroyed_)
	{
		PyErr_SetString( PyExc_ValueError, "Base entity already destroyed" );
		return NULL;
	}

	static char * keywords[] = 
	{
		const_cast< char *> ( "deleteFromDB" ),
		const_cast< char *> ( "writeToDB" ),
		NULL
	};

	PyObject * pDeleteFromDB = NULL;
	PyObject * pWriteToDB = NULL;

	if (!PyArg_ParseTupleAndKeywords( args, kwargs, "|OO", keywords,
			&pDeleteFromDB, &pWriteToDB ))
	{
		return NULL;
	}

	bool deleteFromDB = (pDeleteFromDB != NULL) ? 
		PyObject_IsTrue( pDeleteFromDB ) : false;

	bool writeToDB = (pWriteToDB != NULL) ? 
		PyObject_IsTrue( pWriteToDB ) : this->hasWrittenToDB();

	if (pWriteToDB && !writeToDB && BaseApp::instance().pSqliteDB())
	{
		// Writes lost due to flip-floping
		SECONDARYDB_WARNING_MSG( "Base::py_destroy: %s %d destroyed with "
			"writeToDB=False. All writes to the secondary "
			"database will be lost.\n",	pType_->name(), id_ );
	}

	this->destroy( deleteFromDB, writeToDB );

	Py_RETURN_NONE;
}


/**
 *	This method tells the cell entity to destroy itself. This should, in turn,
 *	destroy this base.
 */
PyObject * Base::py_destroyCellEntity( PyObject * args )
{
	if (!this->isGetCellPending() &&
		!this->checkAssociatedCellEntity( true, "Base.destroyCellEntity: " ))
	{
		return NULL;
	}

	if (PyTuple_Size( args ) > 0)
	{
		PyErr_SetString( PyExc_TypeError, "Base.destroyCellEntity() accepts no arguments" );
		return NULL;
	}

	// Don't let people do this twice.  It's not really fatal but causes resend
	// error spam because the cellapp drops the second packet because the
	// channel is gone.
	if (!isDestroyCellPending_)
	{
		Mercury::Bundle & bundle = this->cellBundle();
		CellAppInterface::destroyEntityArgs & rDestroyEntityArgs =
			CellAppInterface::destroyEntityArgs::start( bundle, this->id() );

		rDestroyEntityArgs.flags = 0; // Currently not used

		if (this->shouldSendToCell())
		{
			this->sendToCell();
		}

		isDestroyCellPending_ = true;
	}

	Py_RETURN_NONE;
}


/**
 *	This method writes an entity into the database.
 */
PyObject * Base::py_writeToDB( PyObject * args, PyObject * kwargs )
{
	if (pType_->isService())
	{
		PyErr_SetString( PyExc_TypeError,
				"Services cannot be written to the database" );
		return NULL;
	}

	if (!pType_->description().isPersistent())
	{
		PyErr_Format( PyExc_TypeError,
			"Entities of type %s cannot be persisted."
				"See <Persistent> in %s.def.",
			pType_->name(), pType_->name() );

		return NULL;
	}

	PyObject * pHandler = NULL;
	PyObject * pShouldAutoLoad = NULL;
	PyObject * pShouldWriteToPrimary = NULL;
	DatabaseID explicitDatabaseID = 0;

	static char * keywords[] = 
	{
		const_cast< char * >( "callback" ),
		const_cast< char * >( "shouldAutoLoad" ),
		const_cast< char * >( "shouldWriteToPrimary" ),
		const_cast< char * >( "explicitDatabaseID" ),
		NULL
	};

	if (!PyArg_ParseTupleAndKeywords( args, kwargs, "|OOOl", keywords,
			&pHandler, &pShouldAutoLoad, &pShouldWriteToPrimary,
			&explicitDatabaseID ))
	{
		return NULL;
	}

	if (pHandler == Py_None)
	{
		pHandler = NULL;
	}

	if (pHandler && !PyCallable_Check( pHandler ))
	{
		PyErr_SetString( PyExc_TypeError, 
				"Callback argument must be callable" );
		return NULL;
	}

	if (!pType_->description().forceExplicitDBID() && explicitDatabaseID != 0)
	{
		PyErr_SetString( PyExc_TypeError, 
				"Entity of this type does not allow explicitDatabaseID" );
		return NULL;
	}

	bool isFirstWrite = (databaseID_ == 0);
	if (isFirstWrite)
	{
		if (pType_->description().forceExplicitDBID() && 
				explicitDatabaseID == 0)
		{	
			PyErr_SetString( PyExc_TypeError, 
				"Entity of this type requires an "
				"explicitDatabaseID for the first write." );
			return NULL;
		}
	}
	else if (explicitDatabaseID != 0)
	{
	 	if (explicitDatabaseID != databaseID_)
		{
			PyErr_SetString( PyExc_TypeError, 
				"Entity already checked in, "
				"cannot accept explicitDatabaseID" );
			return NULL;
		}

		// this is an update
		explicitDatabaseID = 0;
	}

	WriteDBFlags flags = WRITE_BASE_CELL_DATA | WRITE_EXPLICIT;

	if (pShouldAutoLoad)
	{
		flags |= (PyObject_IsTrue( pShouldAutoLoad ) ? 
			WRITE_AUTO_LOAD_YES : WRITE_AUTO_LOAD_NO);
	}

	if (pShouldWriteToPrimary &&
			PyObject_IsTrue( pShouldWriteToPrimary ))
	{
		flags |= WRITE_TO_PRIMARY_DATABASE;
	}

	if (explicitDatabaseID)
	{
		flags |= WRITE_EXPLICIT_DBID;
	}

	WriteToDBPyReplyHandler * pPyHandler = (pHandler) ?
		new WriteToDBPyReplyHandler( this, pHandler ) : NULL;

	if (!this->writeToDB( flags, pPyHandler, NULL, explicitDatabaseID ))
	{
		PyErr_SetString( PyExc_ValueError, "Failed to write data" );
		return NULL;
	}

	Py_RETURN_NONE;
}


/**
 *	This method is exposed to scripting. It is used by a base to register a
 *	timer function to be called with a given period.
 */
PyObject * Base::py_addTimer( PyObject * args )
{
	if (isDestroyed_)
	{
		PyErr_Format( PyExc_TypeError, "Base.addTimer: "
			"Entity %d has already been destroyed", id_ );
		return NULL;
	}

	return pyTimer_.addTimer( args );
}


/**
 *	This method is exposed to scripting. It is used by a base to remove a timer
 *	from the time queue.
 */
PyObject * Base::py_delTimer( PyObject * args )
{
	return pyTimer_.delTimer( args );
}


/**
 *	This method is exposed to scripting. It attempts to register this base
 *	globally with the input label.
 */
PyObject * Base::py_registerGlobally( PyObject * args )
{
	if (this->isServiceFragment())
	{
		PyErr_SetString( PyExc_TypeError,
				"Services cannot be registered globally" );
		return NULL;
	}

	PyObject * pKey;
	PyObject * pCallback;

	if (!PyArg_ParseTuple( args, "OO", &pKey, &pCallback ))
	{
		return NULL;
	}

	if (!PyCallable_Check( pCallback ))
	{
		PyErr_SetString( PyExc_TypeError,
			"The second argument must be callable" );
		return NULL;
	}

	PyObject * pStringObj = PyObject_Str( pKey );
	if (pStringObj)
	{
		const char * pString = PyString_AsString( pStringObj );
		if (pString == NULL)
		{
			NOTICE_MSG( "Base::py_registerGlobally: Unable to convert key to "
				"a valid string\n" );
			PyErr_Clear();
		}
		else
		{
			INFO_MSG( "Base::py_registerGlobally: Registering global base %d "
				"with key '%s'\n", id_, pString );
		}

		Py_DECREF( pStringObj );
	}
	else
	{
		NOTICE_MSG( "Base::py_registerGlobally: Unable to convert key to "
			"string object\n" );
		PyErr_Clear();
	}

	if (!BaseApp::instance().pGlobalBases()->registerRequest( this, pKey, 
		pCallback ))
	{
		return NULL;
	}

	Py_RETURN_NONE;
}


/**
 *	This method is exposed to scripting. It attempts to deregister this base
 *	globally.
 */
PyObject * Base::py_deregisterGlobally( PyObject * args )
{
	PyObject * pKey;

	if (!PyArg_ParseTuple( args, "O", &pKey ))
	{
		return NULL;
	}

	PyObject * pStringObj = PyObject_Str( pKey );
	if (pStringObj)
	{
		const char * pString = PyString_AsString( pStringObj );
		if (pString == NULL)
		{
			PyErr_Clear();
		}
		else
		{
			INFO_MSG( "Base::py_deregisterGlobally: De-registering global base %d "
				"with key '%s'\n", id_, pString );
		}

		Py_DECREF( pStringObj );
	}
	else
	{
		PyErr_Clear();
	}


	PyObject * pResult =
		BaseApp::instance().pGlobalBases()->deregister( this, pKey ) ?
			Py_True : Py_False;

	Py_INCREF( pResult );

	return pResult;
}


/**
 *	This method reduces the base into a pickled form. In fact it makes
 *	it appear exactly like a mailbox.
 */
PyObject * Base::pyPickleReduce()
{
	EntityMailBoxRef embr;
	PyEntityMailBox::reduceToRef( this, &embr );

	PyObject * pConsArgs = PyTuple_New( 1 );
	PyTuple_SET_ITEM( pConsArgs, 0,
		PyString_FromStringAndSize( (char*)&embr, sizeof(embr) ) );

	return pConsArgs;
}

/**
 *	This static method constructs a new object from a pickled base.
 *	Except since we don't refer to it we don't bother implementing it.
 */
/*
PyObject * Base::pyPickleConstruct( PyObject * args )
{
}
*/


// ----------------------------------------------------------------------------
// Section: Keepalive management
// ----------------------------------------------------------------------------

class KeepAliveTimerHandler: public TimerHandler
{
public:
	KeepAliveTimerHandler( Base * pBase ):
			pBase_( pBase )
	{}

	~KeepAliveTimerHandler()
	{}


private: // virtual TimerHandler methods
	virtual void handleTimeout( TimerHandle handle, void * pUser )
	{
		pBase_->keepAliveTimeout();
	}

	virtual void onRelease( TimerHandle handle, void * pUser )
	{
		delete this;
	}

	BasePtr pBase_;
};


/**
 *	This method is called when a user of this base (for example, the web
 *	integration module) wants to keep this base around and not have it
 *	destroyed before it is finished with it.
 *
 *	When a keepalive is registered, a timer is started with the interval
 *	specified in the keepalive request message. If no keepalive is active on
 *	this base, then a script callback is called onKeepAliveStart().
 *
 *	Subsequent keep alive requests cause the keepalive interval to be reset to
 *	the new interval.
 *
 *	The actual decision to destroy or not is up to the script callback
 *	onKeepAliveStop().
 *
 */
void Base::startKeepAlive(
		const Mercury::Address & srcAddr,
		const Mercury::UnpackedMessageHeader & header,
		const BaseAppIntInterface::startKeepAliveArgs & args )
{
	int hertz = BaseAppConfig::updateHertz();
	GameTime intervalTicks = GameTime( BaseApp::instance().time() +
			args.interval * hertz );

	uint64 now = timestamp();

	uint64 newKeepAliveStop = now + args.interval * stampsPerSecond();

	if (keepAliveTimerHandle_.isSet())
	{
		// check that the new interval is not contained within the current
		// keepalive interval
		if (nextKeepAliveStop_ < newKeepAliveStop)
		{
			// update the interval by cancelling the current timer
			keepAliveTimerHandle_.cancel();
		}
		else
		{
			// new interval is less than our current one
			return;
		}
	}
	else
	{
		// call the start callback, Script::call handles exceptions by printing
		// them
		char contextBuf[256];
		bw_snprintf( contextBuf, sizeof( contextBuf ),
			"Base(%d).onKeepAliveStart:", id_ );
		Script::call( PyObject_GetAttrString( this, "onKeepAliveStart" ),
				PyTuple_New( 0 ), contextBuf, 
				/*okIfFunctionNull:*/ true );
	}

	nextKeepAliveStop_ = newKeepAliveStop;

	// add ourselves to the timer queue
	keepAliveTimerHandle_ = BaseApp::instance().timeQueue().add( intervalTicks, 0,
		new KeepAliveTimerHandler( this ), NULL, "KeepAliveTimer" );

}

/**
 *	This method is called when the current keepalive timer expires. It calls
 *	back on onKeepAliveStop().
 */
void Base::keepAliveTimeout()
{
	MF_ASSERT( keepAliveTimerHandle_.isSet() );
	keepAliveTimerHandle_.clearWithoutCancel();

	if (isDestroyed_)
	{
		return;
	}

	// call the callback
	char contextBuf[256];
	bw_snprintf( contextBuf, sizeof( contextBuf ),
		"Base(%d).onKeepAliveStop:", id_ );
	Script::call( PyObject_GetAttrString( this, "onKeepAliveStop" ),
			PyTuple_New( 0 ), contextBuf, 
			/* okIfFunctionNull: */ true );

}

BW_END_NAMESPACE

// base.cpp
