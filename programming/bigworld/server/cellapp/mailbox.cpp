#include "script/first_include.hpp"

#include "mailbox.hpp"

#include "cstdmf/debug.hpp"

#include "cellapp.hpp"
#include "cellapp_config.hpp"
#include "cell.hpp"
#include "cell_app_channels.hpp"
#include "entity.hpp"
#include "entity_type.hpp"
#include "real_entity.hpp"

#include "baseapp/baseapp_int_interface.hpp"

#include "entitydef/remote_entity_method.hpp"

#include "network/bundle.hpp"
#include "network/channel_sender.hpp"

#include "server/backup_hash_chain.hpp"
#include "server/base_backup_switch_mailbox_visitor.hpp"
#include "server/client_method_calling_flags.hpp"
#include "server/migrate_mailbox_visitor.hpp"

DECLARE_DEBUG_COMPONENT( 0 )


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: CellViaBaseMailBox
// -----------------------------------------------------------------------------

/**
 *	This class is used to create a mailbox to a cell entity. Traffic for the
 *	entity is sent via the base entity instead of directly to the cell entity.
 *	This means that these mailboxes do not have the restrictions that normal
 *	cell entity mailboxes have.
 */
class CellViaBaseMailBox : public CommonBaseEntityMailBox
{
	Py_Header( CellViaBaseMailBox, CommonBaseEntityMailBox )

	public:
		CellViaBaseMailBox( EntityTypePtr pBaseType,
					const Mercury::Address & addr, EntityID id,
					PyTypeObject * pType = &s_type_ ):
			CommonBaseEntityMailBox( pBaseType, addr, id, pType )
		{}

		~CellViaBaseMailBox() { }

		virtual ScriptObject pyGetAttribute( const ScriptString & attrObj );
		virtual BinaryOStream * getStream( const MethodDescription & methodDesc,
			std::auto_ptr< Mercury::ReplyMessageHandler > pHandler );
		virtual EntityMailBoxRef::Component component() const;
		virtual const MethodDescription * findMethod( const char * attr ) const;
};

PY_SCRIPT_CONVERTERS_DECLARE( CellViaBaseMailBox )

PY_TYPEOBJECT( CellViaBaseMailBox )
PY_BEGIN_METHODS( CellViaBaseMailBox )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( CellViaBaseMailBox )
PY_END_ATTRIBUTES()

PY_SCRIPT_CONVERTERS( CellViaBaseMailBox )

ScriptObject CellViaBaseMailBox::pyGetAttribute( const ScriptString & attrObj )
{
	const char * attr = attrObj.c_str();

	if (!strcmp( attr, "base" ) && pLocalType_->canBeOnBase())
	{
		return ScriptObject( new BaseEntityMailBox( pLocalType_, addr_, id_ ),
			ScriptObject::FROM_NEW_REFERENCE );
	}

	return this->CommonBaseEntityMailBox::pyGetAttribute( attrObj );
}

BinaryOStream * CellViaBaseMailBox::getStream(
		const MethodDescription & methodDesc,
		std::auto_ptr< Mercury::ReplyMessageHandler > pHandler )
{
	Mercury::Bundle & bundle = this->bundle();

	// Not supporting return values
	if (pHandler.get())
	{
		PyErr_Format( PyExc_TypeError,
				"Cannot call two-way method '%s' from CellApp",
				methodDesc.name().c_str() );
		return NULL;
	}

	bundle.startMessage( BaseAppIntInterface::callCellMethod );
	bundle << methodDesc.internalIndex();

	return &bundle;
}

/**
 *  Say what type of component we are
 */
EntityMailBoxRef::Component CellViaBaseMailBox::component() const
{
	return EntityMailBoxRef::CELL_VIA_BASE;
}

/**
 *  This method overrides the virtual findMethod function. It returns the
 *  cell MethodDescription with the input name.
 */
const MethodDescription * CellViaBaseMailBox::findMethod(
	const char * attr ) const
{
	return pLocalType_->description().cell().find( attr );
}

// -----------------------------------------------------------------------------
// Section: BaseViaCellMailBox
// -----------------------------------------------------------------------------

/**
 *	This class is used to create a mailbox to a base entity. Traffic for the
 *	entity is sent via the cell entity instead of directly to the base entity.
 *	This means that these mailboxes have the restrictions that cell entity
 *	mailboxes have. That is, they cannot be stored and must be used immediately.
 */
class BaseViaCellMailBox : public CellEntityMailBox
{
	Py_Header( BaseViaCellMailBox, CellEntityMailBox )

	public:
		BaseViaCellMailBox( EntityTypePtr pBaseType,
					const Mercury::Address & addr, EntityID id,
					PyTypeObject * pType = &s_type_ ):
			CellEntityMailBox( pBaseType, addr, id, pType )
		{}

		~BaseViaCellMailBox() { }

		virtual ScriptObject pyGetAttribute( const ScriptString & attrObj );
		virtual BinaryOStream * getStream( const MethodDescription & methodDesc, 
			std::auto_ptr< Mercury::ReplyMessageHandler > pHandler );
		virtual EntityMailBoxRef::Component component() const;
		virtual const MethodDescription * findMethod( const char * attr ) const;
};

/*~	class NoModule.BaseViaCellMailBox
 *  @components{ cell }
 *
 *	BaseViaCellMailBox inherits from CellEntityMailBox.
 *
 *	A BaseViaCellMailBox is used to communicate with a Cell entity's associated
 *	Base entity with communication passing through the Cell entity. This kind
 *	of mailbox may be returned when referencing another Cell entity through a
 *  mailbox, and then requesting it's Base mailbox.
 *
 *	The BaseViaCellMailBox is used as an optimisation within BigWorld to avoid
 *	having to perform a Base entity lookup from the calling entity.
 *
 *	These mailboxes have the same restrictions as Cell mailboxes and should not
 *	be stored, they should be used immediately then discarded.
 */
PY_SCRIPT_CONVERTERS_DECLARE( BaseViaCellMailBox )

PY_TYPEOBJECT( BaseViaCellMailBox )
PY_BEGIN_METHODS( BaseViaCellMailBox )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( BaseViaCellMailBox )
PY_END_ATTRIBUTES()

PY_SCRIPT_CONVERTERS( BaseViaCellMailBox )

ScriptObject BaseViaCellMailBox::pyGetAttribute( const ScriptString & attrObj )
{
	const char * attr = attrObj.c_str();

	if (!strcmp( attr, "cell" ) && pLocalType_->canBeOnCell())
	{
		return ScriptObject( new CellEntityMailBox( pLocalType_, addr_, id_ ),
			ScriptObject::FROM_NEW_REFERENCE );
	}

	return this->CellEntityMailBox::pyGetAttribute( attrObj );
}

BinaryOStream * BaseViaCellMailBox::getStream(
		const MethodDescription & methodDesc,
		std::auto_ptr< Mercury::ReplyMessageHandler > pHandler )
{
	Mercury::Channel * pChannel = this->pChannel();
	if (!pChannel)
	{
		PyErr_Format( PyExc_TypeError,
				"Cannot get channel for %s",
				methodDesc.name().c_str() );
		return NULL;
	}
	Mercury::Bundle & bundle = pChannel->bundle();

	// Not supporting return values
	if (pHandler.get())
	{
		PyErr_Format( PyExc_TypeError,
				"Cannot call two-way method '%s' from CellApp",
				methodDesc.name().c_str() );
		return NULL;
	}

	bundle.startMessage( CellAppInterface::callBaseMethod );
	bundle << id_;
	bundle << methodDesc.internalIndex();

	return &bundle;
}

/**
 *  Say what type of component we are
 */
EntityMailBoxRef::Component BaseViaCellMailBox::component() const
{
	return EntityMailBoxRef::BASE_VIA_CELL;
}

/**
 *  This method overrides the virtual findMethod function. It returns the
 *  base MethodDescription with the input name.
 */
const MethodDescription * BaseViaCellMailBox::findMethod(
	const char * attr ) const
{
	return pLocalType_->description().base().find( attr );
}


// -----------------------------------------------------------------------------
// Section: ClientViaBaseMailBox
// -----------------------------------------------------------------------------
/**
 *	This class is used to create a mailbox to a client entity. Traffic for the
 *	entity is sent via the base entity.
 */
class ClientViaBaseMailBox : public CommonBaseEntityMailBox
{
	Py_Header( ClientViaBaseMailBox, CommonBaseEntityMailBox )

public:
	ClientViaBaseMailBox( EntityTypePtr pBaseType,
				const Mercury::Address & addr, EntityID id,
				RecordingOption recordingOption =
					RECORDING_OPTION_METHOD_DEFAULT,
				PyTypeObject * pType = &s_type_ ):
		CommonBaseEntityMailBox( pBaseType, addr, id, pType ),
		recordingOption_( recordingOption )
	{}

	virtual ~ClientViaBaseMailBox() { }

	virtual BinaryOStream * getStream( const MethodDescription & methodDesc,
			std::auto_ptr< Mercury::ReplyMessageHandler > pHandler );
	virtual EntityMailBoxRef::Component component() const;
	virtual const MethodDescription * findMethod( const char * attr ) const;

	PY_KEYWORD_METHOD_DECLARE( pyCall )

private:
	RecordingOption recordingOption_;
};

PY_SCRIPT_CONVERTERS_DECLARE( ClientViaBaseMailBox )

/*~	class NoModule.CellViaBaseMailBox
 *  @components{ cell }
 *
 *	CellViaBaseMailBox inherits from BaseEntityMailBox.
 *
 *	A CellViaBaseMailBox is used to communicate with a Cell entity channeling
 *	the communication through the associated Base entity. This may occur
 *	when a Cell entity, entity A, has another Cell entity's, entity B,
 *	associated Base mailbox and uses this mailbox to reference the associated
 *	Cell entity. For example:
 *
 *	class EntityA( ... ):
 *	   ...
 *
 *	   def example( self ):
 *	      ...
 *	      entityB_BaseMailbox.cell.doSomething()
 *
 *	It is important to note that there are some BigWorld methods that will
 *	not accept a CellViaBaseMailbox in place of a regular cell mailbox.
 *	For an example of how to work around this issue, please refer to the
 *	BaseApp Python API documention for the Base.createCellEntity() method.
 *
 *	Unlike direct Cell entity mailboxes, these mailboxes may be stored and used
 *	later.
 */
PY_TYPEOBJECT_WITH_CALL( ClientViaBaseMailBox )
PY_BEGIN_METHODS( ClientViaBaseMailBox )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( ClientViaBaseMailBox )
PY_END_ATTRIBUTES()

PY_SCRIPT_CONVERTERS( ClientViaBaseMailBox )

BinaryOStream * ClientViaBaseMailBox::getStream(
		const MethodDescription & methodDesc,
		std::auto_ptr< Mercury::ReplyMessageHandler > pHandler )
{
	Mercury::Bundle & bundle = this->bundle();

	// Not supporting return values
	if (pHandler.get())
	{
		PyErr_Format( PyExc_TypeError,
				"Cannot call two-way method '%s' from CellApp",
				methodDesc.name().c_str() );
		return NULL;
	}

	bundle.startMessage( BaseAppIntInterface::callClientMethod );
	bundle << methodDesc.internalIndex();
	bundle << uint8( recordingOption_ );

	return &bundle;
}

/**
 *  Say what type of component we are
 */
EntityMailBoxRef::Component ClientViaBaseMailBox::component() const
{
	return EntityMailBoxRef::CLIENT_VIA_BASE;
}


/**
 *  This method overrides the virtual findMethod function. It returns the
 *  client MethodDescription with the input name.
 */
const MethodDescription * ClientViaBaseMailBox::findMethod(
	const char * attr ) const
{
	return pLocalType_->description().client().find( attr );
}


/**
 *	This method implements the tp_call operator for Python.
 */
PyObject * ClientViaBaseMailBox::pyCall( PyObject * args, PyObject * kwargs )
{
	RecordingOption recordingOption = RECORDING_OPTION_METHOD_DEFAULT;

	if (!this->parseRecordingOptionFromPythonCall( args, kwargs,
			recordingOption ))
	{
		return NULL;
	}

	if (recordingOption == recordingOption_)
	{
		this->incRef();
		return this;
	}

	return new ClientViaBaseMailBox( pLocalType_, this->address(), this->id(),
		recordingOption );
}


// -----------------------------------------------------------------------------
// Section: ClientViaCellMailBox
// -----------------------------------------------------------------------------
/**
 *	This class is used to create a mailbox to a client entity. Traffic for the
 *	entity is sent via the cell entity.
 */
class ClientViaCellMailBox : public CellEntityMailBox
{
	Py_Header( ClientViaCellMailBox, CellEntityMailBox )

public:
	ClientViaCellMailBox( EntityTypePtr pBaseType,
				const Mercury::Address & addr, EntityID id,
				RecordingOption recordingOption =
					RECORDING_OPTION_METHOD_DEFAULT,
				PyTypeObject * pType = &s_type_ ):
		CellEntityMailBox( pBaseType, addr, id, pType ),
		recordingOption_( recordingOption )
	{}

	virtual ~ClientViaCellMailBox() { }

	virtual BinaryOStream * getStream( const MethodDescription & methodDesc,
		std::auto_ptr< Mercury::ReplyMessageHandler > pHandler );
	virtual EntityMailBoxRef::Component component() const;
	virtual const MethodDescription * findMethod( const char * attr ) const;

	PY_KEYWORD_METHOD_DECLARE( pyCall )

private:
	RecordingOption recordingOption_;
};

PY_SCRIPT_CONVERTERS_DECLARE( ClientViaCellMailBox )

PY_TYPEOBJECT_WITH_CALL( ClientViaCellMailBox )
PY_BEGIN_METHODS( ClientViaCellMailBox )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( ClientViaCellMailBox )
PY_END_ATTRIBUTES()

PY_SCRIPT_CONVERTERS( ClientViaCellMailBox )


BinaryOStream * ClientViaCellMailBox::getStream(
		const MethodDescription & methodDesc,
		std::auto_ptr< Mercury::ReplyMessageHandler > pHandler )
{
	Mercury::Channel * pChannel = this->pChannel();
	if (!pChannel)
	{
		PyErr_Format( PyExc_TypeError,
				"No channel to CellApp %s",
				this->address().c_str() );
		return NULL;
	}

	Mercury::Bundle & bundle = pChannel->bundle();

	// Not supporting return values
	if (pHandler.get())
	{
		PyErr_Format( PyExc_TypeError,
				"Cannot call two-way method '%s' from CellApp",
				methodDesc.name().c_str() );
		return NULL;
	}

	bundle.startMessage( CellAppInterface::callClientMethod );
	bundle << id_; // Which cell entity
	bundle << id_; // Which entity on the client app

	uint8 flags = 0;

	if (recordingOption_ != RECORDING_OPTION_RECORD_ONLY)
	{
		flags |= MSG_FOR_OWN_CLIENT;
	}

	if (methodDesc.shouldRecord( recordingOption_ ))
	{
		flags |= MSG_FOR_REPLAY;
	}

	bundle << flags;
	bundle << methodDesc.internalIndex();

	return &bundle;
}


/**
 *  Say what type of component we are
 */
EntityMailBoxRef::Component ClientViaCellMailBox::component() const
{
	return EntityMailBoxRef::CLIENT_VIA_CELL;
}


/**
 *  This method overrides the virtual findMethod function. It returns the
 *  client MethodDescription with the input name.
 */
const MethodDescription * ClientViaCellMailBox::findMethod(
	const char * attr ) const
{
	return pLocalType_->description().client().find( attr );
}


/**
 *	This method implements the tp_call operator for Python.
 */
PyObject * ClientViaCellMailBox::pyCall( PyObject * args, PyObject * kwargs )
{
	RecordingOption recordingOption = RECORDING_OPTION_METHOD_DEFAULT;

	if (!this->parseRecordingOptionFromPythonCall( args, kwargs,
			recordingOption ))
	{
		return NULL;
	}

	if (recordingOption == recordingOption_)
	{
		this->incRef();
		return this;
	}

	return new ClientViaCellMailBox( pLocalType_, this->address(), this->id(),
		recordingOption );
}


// -----------------------------------------------------------------------------
// Section: ServerEntityMailBox
// -----------------------------------------------------------------------------

/*~	class NoModule.ServerEntityMailBox
 *  @components{ cell }
 *
 *	This class implements a mailbox that can send to a server Entity
 *	(ie, Cell or Base Entity). It is not instantiated directly, but used
 *	by CellEntityMailBox and BaseEntityMailBox.
 */
PY_TYPEOBJECT( ServerEntityMailBox )

PY_BEGIN_METHODS( ServerEntityMailBox )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( ServerEntityMailBox )
	/*~ attribute ServerEntityMailBox.component
	 *  @components{ cell }
	 *
	 *	The server component of the associated Entity.
	 *	@type string
	 */
	PY_ATTRIBUTE( component )
	/*~ attribute ServerEntityMailBox.className
	 *  @components{ cell }
	 *
	 *	The class name of the associated Entity.
	 *	@type string
	 */
	PY_ATTRIBUTE( className )
	/*~ attribute ServerEntityMailBox.ip
	 *  @components{ cell }
	 *
	 *	The IP address value of the associated Entity. If zero, the mailbox is
	 *	invalid.
	 *	@type integer
	 */
	PY_ATTRIBUTE( ip )
PY_END_ATTRIBUTES()

PY_SCRIPT_CONVERTERS( ServerEntityMailBox )


/**
 *	Constructor.
 */
ServerEntityMailBox::ServerEntityMailBox( EntityTypePtr pLocalType,
		const Mercury::Address & addr, EntityID id,
		PyTypeObject * pType ) :
	PyEntityMailBox( pType ),
	addr_( addr ),
	id_( id ),
	pLocalType_( pLocalType )
{
}

/**
 *	Destructor
 */
ServerEntityMailBox::~ServerEntityMailBox()
{
}


/**
 *	Get a packed ref representation of this mailbox
 */
EntityMailBoxRef ServerEntityMailBox::ref() const
{
	EntityMailBoxRef mbr; mbr.init(
		id_, addr_, this->component(), pLocalType_->description().index() );
	return mbr;
}


/**
 *	Get the component name of this mailbox
 */
const char * ServerEntityMailBox::componentName() const
{
	switch (this->component())
	{
		case EntityMailBoxRef::CELL:			return "cell";
		case EntityMailBoxRef::BASE:			return "base";
		case EntityMailBoxRef::SERVICE:			return "service";
		case EntityMailBoxRef::CLIENT:			return "client";
		case EntityMailBoxRef::BASE_VIA_CELL:		return "base_via_cell";
		case EntityMailBoxRef::CLIENT_VIA_CELL:		return "client_via_cell";
		case EntityMailBoxRef::CELL_VIA_BASE:		return "cell_via_base";
		case EntityMailBoxRef::CLIENT_VIA_BASE:		return "client_via_base";
		default:								return "<invalid>";
	}
}


/**
 *	This static method migrates all the mailboxes by updating their stored
 *	type pointer. If their type index has changed, then no worries, the new
 *	one will be adopted since the ref method looks it up every time.
 */
void ServerEntityMailBox::migrate()
{
	pLocalType_ = EntityType::getType( pLocalType_->name() );

	if (!pLocalType_)
	{
		id_ = 0;	// invalid mailbox ... but prevent crash:
		pLocalType_ = EntityType::getType( EntityTypeID( 0 ) );
	}
}


/**
 *	Migrate mailboxes to new types.
 */
void ServerEntityMailBox::migrateMailBoxes()
{
	MigrateMailBoxVisitor visitor;
	PyEntityMailBox::visit( visitor );
}


/**
 *	This method is called when a BaseApp has died. It allows us to redirect the
 *	mailboxes to the new location.
 */
void ServerEntityMailBox::adjustForDeadBaseApp(
			const Mercury::Address & deadAddr,
			const BackupHash & backupHash )
{
	BackupHashChain hashChain;
	hashChain.adjustForDeadBaseApp( deadAddr, backupHash );

	BaseBackupSwitchMailBoxVisitor visitor( hashChain, deadAddr );
	PyEntityMailBox::visit( visitor );
}


// -----------------------------------------------------------------------------
// Section: CellEntityMailBox
// -----------------------------------------------------------------------------

/*~	class NoModule.CellEntityMailBox
 *  @components{ cell }
 *
 *	CellEntityMailBox inherits from ServerEntityMailBox.
 *
 *	On a Cell, the CellEntityMailBox is used in a variety of functions to
 *	determine Space or locational information.
 *
 *	For example, to teleport an NPC to another Entity in a different Space:
 *
 *		self.teleport( otherEntity.cell,
 *					   otherEntity.position,
 *					   otherEntity.direction )
 */
/*~	attribute CellEntityMailBox.base
 *	@components{ cell }
 *	An indirect mailbox to the base representative this cell mailbox is associated with.
 *	For example, to call the base methods via a cell mailbox 'cellMB', use:
 *
 *	cellMB.base.baseMethod()
 *
 *	The method call will first be sent to the cell this mailbox belongs to and then redirected to the
 *	base counterpart.
 */
/*~	attribute CellEntityMailBox.client
 *	@components{ cell }
 *	An indirect mailbox to the client representative this cell mailbox is associated with.
 *	For example, to call the client methods via a cell mailbox 'cellMB', use:
 *
 *	cellMB.client.clientMethod()
 *
 *	The method call will first be sent to the cell this mailbox belongs to and then redirected to the
 *	client counterpart.
 *
 *	You can only call the methods on own client.
 */
/*~	attribute CellEntityMailBox.ownClient
 *	@components{ cell }
 *	An alias to CellEntityMailBox.client.
 */

PY_TYPEOBJECT( CellEntityMailBox )

PY_BEGIN_METHODS( CellEntityMailBox )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( CellEntityMailBox )
	// PY_ATTRIBUTE( __class__ )

	// ServerEntityMailBox attributes
	// PY_ATTRIBUTE( component )
	// PY_ATTRIBUTE( className )
	// PY_ATTRIBUTE( ip )

	// Mailboxes
	PY_ATTRIBUTE( base )
	PY_ATTRIBUTE( client )
	PY_ATTRIBUTE_ALIAS( client, ownClient )
PY_END_ATTRIBUTES()

PY_SCRIPT_CONVERTERS( CellEntityMailBox )


/**
 *	This method retrieves the base mailbox associated to this mailbox.
 */
PyObject * CellEntityMailBox::pyGet_base()
{
	if (!pLocalType_->canBeOnBase())
	{
		PyErr_Format( PyExc_AttributeError,
			"Base has no defined script methods." );
		return NULL;
	}

	return new BaseViaCellMailBox( pLocalType_, addr_, id_ );
}


/**
 *	This method retrieves the client mailbox associated to this mailbox.
 */
PyObject * CellEntityMailBox::pyGet_client()
{
	if (!pLocalType_->description().canBeOnClient())
	{
		PyErr_Format( PyExc_AttributeError,
			"Client has no defined script methods." );
		return NULL;
	}

	return new ClientViaCellMailBox( pLocalType_, addr_, id_ );
}


/**
 *  This method returns the most appropriate channel for this mailbox.  It will
 *  either return a CellAppChannel or nothing.
 */
Mercury::UDPChannel * CellEntityMailBox::pChannel() const
{
	CellAppChannel * pCellAppChannel = CellAppChannels::instance().get( addr_ );
	if (pCellAppChannel != NULL)
	{
		return &pCellAppChannel->channel();
	}
	return NULL;
}


/**
 *	This method sends the stream that getStream got.
 */
void CellEntityMailBox::sendStream()
{
	Mercury::UDPChannel * pChannel = this->pChannel();

	if (pChannel)
	{
		if (pChannel->addr().ip != 0)
		{
			Mercury::ChannelSender sender( *this->pChannel() );
		}
		else
		{
			INFO_MSG( "CellEntityMailBox::sendStream: cell mailbox channel"
					" not established, buffering message for entity %d\n",
				id_ );
		}
	}
}


/**
 *	This method gets a stream to send a message to the cell on.
 */
BinaryOStream * CellEntityMailBox::getStream(
		const MethodDescription & methodDesc, 
		std::auto_ptr< Mercury::ReplyMessageHandler > pHandler )
{
	Mercury::Channel * pChannel = this->pChannel();
	if (!pChannel)
	{
		return NULL;
	}

	// Get the bundle to the real's app.
	Mercury::Bundle & bundle = pChannel->bundle();

	// Not supporting return values
	if (pHandler.get())
	{
		PyErr_Format( PyExc_TypeError,
				"Cannot call two-way method '%s' from CellApp",
				methodDesc.name().c_str() );
		return NULL;
	}

	// Start the message
	bundle.startMessage( CellAppInterface::runScriptMethod );
	bundle << id_;
	bundle << methodDesc.internalIndex();

	return &bundle;
}


/**
 *	This method overrides the virtual findMethod function. It returns the
 *	cell MethodDescription with the input name.
 */
const MethodDescription * CellEntityMailBox::findMethod(
	const char * attr ) const
{
	return pLocalType_->description().cell().find( attr );
}


/**
 *	Say what type of component we are
 */
EntityMailBoxRef::Component CellEntityMailBox::component() const
{
	return EntityMailBoxRef::CELL;
}


// -----------------------------------------------------------------------------
// Section: CommonBaseEntityMailBox
// -----------------------------------------------------------------------------

PY_TYPEOBJECT( CommonBaseEntityMailBox )

PY_BEGIN_METHODS( CommonBaseEntityMailBox )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( CommonBaseEntityMailBox )
PY_END_ATTRIBUTES()


/**
 *  This method returns the most appropriate channel for this mailbox.  It will
 *  use the entity channel if it can, otherwise it just falls through to the
 *  base implementation.
 */
Mercury::UDPChannel & CommonBaseEntityMailBox::channel() const
{
	Entity * pEntity = CellApp::instance().findEntity( id_ );
	return this->channel( pEntity );
}


/**
 *  This method returns the most appropriate channel for this mailbox.  The
 *  entity is expected to have already been looked up.  It will use the entity
 *  channel if it can, otherwise it just falls through to the base
 *  implementation.
 */
Mercury::UDPChannel & CommonBaseEntityMailBox::channel( Entity * pEntity ) const
{
	return (pEntity && pEntity->isReal()) ?
		pEntity->pReal()->channel() : CellApp::getChannel( addr_ );
}


/**
 *	This method sends the stream that getStream got.
 */
void CommonBaseEntityMailBox::sendStream()
{
	Mercury::UDPChannel & channel = this->channel();

	if (channel.addr().ip == 0)
	{
		NOTICE_MSG( "CommonBaseEntityMailBox::sendStream: %s mailbox channel"
				" not established, buffering message for entity %d\n",
			this->componentName(),
			id_ );
		return;
	}

	// Using this form of delayedSend() so that we send it soon regardless of
	// whether we have a player making the channel regular. 
	channel.networkInterface().delayedSend( channel );
}


/**
 *  This method returns a bundle that will be sent to the base entity.  It
 *  overrides the base behaviour of just returning the channel's bundle by
 *  prefixing the bundle with a setClient message if we're not sending on the
 *  entity channel.
 */
Mercury::Bundle & CommonBaseEntityMailBox::bundle() const
{
	Entity * pEntity = CellApp::instance().findEntity( id_ );
	Mercury::Bundle & bundle = this->channel( pEntity ).bundle();

	if (!pEntity || !pEntity->isReal())
	{
		BaseAppIntInterface::setClientArgs::start( bundle ).id = id_;
	}

	return bundle;
}


// -----------------------------------------------------------------------------
// Section: BaseEntityMailBox
// -----------------------------------------------------------------------------

/*~ class NoModule.BaseEntityMailBox
 *	@components{ cell }
 *
 *	BaseEntityMailBox inherits from ServerEntityMailBox.
 *
 *	A BaseEntityMailBox is used by a Cell Entity to communicate with its
 *	associated Base Entity.  This allows the Cell Entity to call the methods on
 *	the Base Entity.
 *	
 *	For example:
 *
 *		self.base.baseMethod()
 *
 *	where baseMethod() is described in the .def file.
 */
/*~	attribute BaseEntityMailBox.cell
 *	@components{ cell }
 *	An indirect mailbox to the cell representative this base mailbox is
 *	associated with.
 *
 *	For example, to call the cell methods via a base mailbox 'baseMB', use:
 *
 *	baseMB.cell.cellMethod()
 *
 *	The method call will first be sent to the base this mailbox belongs to and
 *	then redirected to the cell counterpart.
 */
/*~ attribute BaseEntityMailBox.client
 *	@components{ cell }
 *	An indirect mailbox to the client representative this base mailbox is
 *	associated with.
 *
 *	For example, to call the client methods via a base mailbox 'baseMB', use:
 *
 *	baseMB.client.clientMethod()
 *
 *	The method call will first be sent to the base this mailbox belongs to and
 *	then redirected to the client counterpart.
 */
/*~	attribute BaseEntityMailBox.ownClient
 *	@components{ cell }
 *	An alias to BaseEntityMailBox.client.
 */

PY_TYPEOBJECT( BaseEntityMailBox )

PY_BEGIN_METHODS( BaseEntityMailBox )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( BaseEntityMailBox )

	// ServerEntityMailBox attributes
	// PY_ATTRIBUTE( component )
	// PY_ATTRIBUTE( className )
	// PY_ATTRIBUTE( ip )

	// Mailboxes
	PY_ATTRIBUTE( cell )
	PY_ATTRIBUTE( client )
	PY_ATTRIBUTE_ALIAS( client, ownClient )

PY_END_ATTRIBUTES()

PY_SCRIPT_CONVERTERS( BaseEntityMailBox )

/**
 *	This method retrieves the cell mailbox associated to this mailbox.
 */
PyObject * BaseEntityMailBox::pyGet_cell()
{
	if (!pLocalType_->canBeOnCell())
	{
		PyErr_Format( PyExc_AttributeError,
			"Cell has no defined script methods." );
		return NULL;
	}

	return new CellViaBaseMailBox( pLocalType_, addr_, id_ );
}


/**
 *	This method retrieves the client mailbox associated to this mailbox.
 */
PyObject * BaseEntityMailBox::pyGet_client()
{

	if (!pLocalType_->description().canBeOnClient())
	{
		PyErr_Format( PyExc_AttributeError,
			"Client has no defined script methods." );
		return NULL;
	}

	return new ClientViaBaseMailBox( pLocalType_, addr_, id_ );
}


/**
 *	This method gets a stream to send a message to a base.
 */
BinaryOStream * BaseEntityMailBox::getStream(
		const MethodDescription & methodDesc,
		std::auto_ptr< Mercury::ReplyMessageHandler > pHandler )
{
	Mercury::Bundle & bundle = this->bundle();

	// Not supporting return values
	if (pHandler.get())
	{
		PyErr_Format( PyExc_TypeError,
				"Cannot call two-way method '%s' from CellApp",
				methodDesc.name().c_str() );
		return NULL;
	}

	bundle.startMessage( BaseAppIntInterface::callBaseMethod );
	bundle << methodDesc.internalIndex();

	return &bundle;
}


/**
 *	This method overrides the virtual findMethod function. It returns the
 *	base MethodDescription with the input name.
 */
const MethodDescription * BaseEntityMailBox::findMethod(
	const char * attr ) const
{
	return pLocalType_->description().base().find( attr );
}


/**
 *	Say what type of component we are
 */
EntityMailBoxRef::Component BaseEntityMailBox::component() const
{
	return pLocalType_->description().isService() ? EntityMailBoxRef::SERVICE :
		EntityMailBoxRef::BASE;
}


/**
 *	This class registers our classes into the PyEntityMailBox system,
 *	and provides some glue/helper functions for it.
 */
static class CellAppPostOfficeAttendant
{
public:
	CellAppPostOfficeAttendant()
	{
		PyEntityMailBox::registerMailBoxComponentFactory(
			EntityMailBoxRef::CELL, newCellMB, &CellEntityMailBox::s_type_ );
		PyEntityMailBox::registerMailBoxComponentFactory(
			EntityMailBoxRef::SERVICE, newBaseMB, &BaseEntityMailBox::s_type_ );
		PyEntityMailBox::registerMailBoxComponentFactory(
			EntityMailBoxRef::BASE, newBaseMB, &BaseEntityMailBox::s_type_ );
		PyEntityMailBox::registerMailBoxComponentFactory(
			EntityMailBoxRef::BASE_VIA_CELL, newBaseViaCellMB, &BaseViaCellMailBox::s_type_ );
		PyEntityMailBox::registerMailBoxComponentFactory(
			EntityMailBoxRef::CELL_VIA_BASE, newCellViaBaseMB, &CellViaBaseMailBox::s_type_ );
		PyEntityMailBox::registerMailBoxComponentFactory(
			EntityMailBoxRef::CLIENT_VIA_CELL, newClientViaCellMB, &ClientViaCellMailBox::s_type_ );
		PyEntityMailBox::registerMailBoxComponentFactory(
			EntityMailBoxRef::CLIENT_VIA_BASE, newClientViaBaseMB, &ClientViaBaseMailBox::s_type_ );
		PyEntityMailBox::registerMailBoxRefEquivalent(
			ServerEntityMailBox::Check, ServerEntityMailBox::static_ref );
		PyEntityMailBox::registerMailBoxRefEquivalent(
			Entity::Check, cellReduce );
	}

	static PyObject * newCellMB( const EntityMailBoxRef & ref )
	{
		// first see if we have a ghost of that entity ID
		Entity * pEntity = CellApp::instance().findEntity( ref.id );
		if (pEntity != NULL)
		{
			if (pEntity->entityTypeID() != ref.type())
			{
				ERROR_MSG( "Cell MailBoxRef resolver: "
					"Found local cell entity %u for mailbox, "
					"but EntityTypes differ (local is %d not %d)!\n",
					ref.id, pEntity->entityTypeID(), ref.type() );
			}

			if (CellAppConfig::shouldResolveMailBoxes())
			{
				Py_INCREF( pEntity );
				return pEntity;
			}
		}

		// TODO: Consider always returning a mailbox so data is not lost,
		// even when type id exceeds range ... probably not worthwhile.
		EntityTypePtr pLocalType = EntityType::getType( ref.type() );
		if (pLocalType)
		{
			if (pLocalType->canBeOnCell())
			{
				return new CellEntityMailBox( pLocalType, ref.addr, ref.id );
			}
			else
			{
				WARNING_MSG( "newCellMB: %s is not a cell entity class\n",
					pLocalType->description().name().c_str() );
			}
		}
		else
		{
			WARNING_MSG( "newCellMB: Invalid type %d\n", ref.type() );
		}

		Py_RETURN_NONE;
	}

	static PyObject * newBaseMB( const EntityMailBoxRef & ref )
	{
		// TODO: Consider always returning a mailbox so data is not lost,
		// even when type id exceeds range ... probably not worthwhile.
		EntityTypePtr pLocalType = EntityType::getType( ref.type() );
		if (pLocalType)
		{
			if (pLocalType->canBeOnBase())
			{
				return new BaseEntityMailBox( pLocalType, ref.addr, ref.id );
			}
			else
			{
				WARNING_MSG( "newBaseMB: %s is not a base entity class\n",
					pLocalType->description().name().c_str() );
			}
		}
		else
		{
			WARNING_MSG( "newBaseMB: Invalid type %d\n", ref.type() );
		}
		Py_RETURN_NONE;
	}

	static PyObject * newBaseViaCellMB( const EntityMailBoxRef & ref )
	{
		EntityTypePtr pLocalType = EntityType::getType( ref.type() );
		if (pLocalType)
			return new BaseViaCellMailBox( pLocalType, ref.addr, ref.id );
		Py_RETURN_NONE;
	}

	static PyObject * newCellViaBaseMB( const EntityMailBoxRef & ref )
	{
		EntityTypePtr pLocalType = EntityType::getType( ref.type() );
		if (pLocalType)
			return new CellViaBaseMailBox( pLocalType, ref.addr, ref.id );
		Py_RETURN_NONE;
	}

	static PyObject * newClientViaCellMB( const EntityMailBoxRef & ref )
	{
		EntityTypePtr pLocalType = EntityType::getType( ref.type() );
		if (pLocalType)
			return new ClientViaCellMailBox( pLocalType, ref.addr, ref.id );
		Py_RETURN_NONE;
	}

	static PyObject * newClientViaBaseMB( const EntityMailBoxRef & ref )
	{
		EntityTypePtr pLocalType = EntityType::getType( ref.type() );
		if (pLocalType)
			return new ClientViaBaseMailBox( pLocalType, ref.addr, ref.id );
		Py_RETURN_NONE;
	}

	static EntityMailBoxRef cellReduce( PyObject * pObject )
	{
		Entity * pEntity = (Entity*)pObject;
		EntityMailBoxRef embr;
		embr.init( pEntity->id(), pEntity->realAddr(),
			EntityMailBoxRef::CELL, pEntity->entityTypeID() );
		return embr;
	}
} s_capoa;

BW_END_NAMESPACE

// mailbox.cpp
