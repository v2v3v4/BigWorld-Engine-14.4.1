#include "script/first_include.hpp"

#include "mailbox.hpp"

#include "baseapp.hpp"
#include "baseapp_config.hpp"
#include "proxy.hpp"

#include "cellapp/cellapp_interface.hpp"

#include "cstdmf/debug.hpp"

#include "entitydef/remote_entity_method.hpp"

#include "network/bundle.hpp"
#include "network/channel_sender.hpp"
#include "network/udp_channel.hpp"

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
class CellViaBaseMailBox : public ServerEntityMailBox
{
	Py_Header( CellViaBaseMailBox, ServerEntityMailBox )

public:
	CellViaBaseMailBox( EntityTypePtr pBaseType,
				const Mercury::Address & addr, EntityID id,
				PyTypeObject * pType = &s_type_ ):
		ServerEntityMailBox( pBaseType, addr, id, pType )
	{}

	~CellViaBaseMailBox() { }

	virtual ScriptObject pyGetAttribute( const ScriptString & attrObj );
	virtual BinaryOStream * getStreamEx( const MethodDescription & methodDesc, 
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
	if (!strcmp( attrObj.c_str(), "base" ) && pLocalType_->canBeOnBase())
	{
		return ScriptObject(
			new BaseEntityMailBox( pLocalType_, addr_, id_ ),
			ScriptObject::FROM_NEW_REFERENCE );
	}

	return this->ServerEntityMailBox::pyGetAttribute( attrObj );
}

BinaryOStream * CellViaBaseMailBox::getStreamEx(
		const MethodDescription & methodDesc,
		std::auto_ptr< Mercury::ReplyMessageHandler > pHandler )
{
	Mercury::Bundle & bundle = this->bundle();

	BaseAppIntInterface::setClientArgs::start( bundle ).id = id_;

	if (pHandler.get())
	{
		bundle.startRequest( BaseAppIntInterface::callCellMethod,
				pHandler.release() );
	}
	else
	{
		bundle.startMessage( BaseAppIntInterface::callCellMethod );
	}

	bundle << methodDesc.internalIndex();

	return &bundle;
}

/**
 *	Say what type of component we are
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
class BaseViaCellMailBox : public CommonCellEntityMailBox
{
	Py_Header( BaseViaCellMailBox, CommonCellEntityMailBox )

public:
	BaseViaCellMailBox( EntityTypePtr pBaseType,
				const Mercury::Address & addr, EntityID id,
				PyTypeObject * pType = &s_type_ ):
		CommonCellEntityMailBox( pBaseType, addr, id, pType )
	{}

	~BaseViaCellMailBox() { }

	virtual ScriptObject pyGetAttribute( const ScriptString & attrObj );
	virtual BinaryOStream * getStreamEx( const MethodDescription & methodDesc,
		std::auto_ptr< Mercury::ReplyMessageHandler > pHandler );
	virtual EntityMailBoxRef::Component component() const;
	virtual const MethodDescription * findMethod( const char * attr ) const;
};

PY_SCRIPT_CONVERTERS_DECLARE( BaseViaCellMailBox )

PY_TYPEOBJECT( BaseViaCellMailBox )
PY_BEGIN_METHODS( BaseViaCellMailBox )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( BaseViaCellMailBox )
PY_END_ATTRIBUTES()

PY_SCRIPT_CONVERTERS( BaseViaCellMailBox )

ScriptObject BaseViaCellMailBox::pyGetAttribute( const ScriptString & attrObj )
{
	if (!strcmp( attrObj.c_str(), "cell" ) && pLocalType_->canBeOnCell())
	{
		return ScriptObject(
			new CellEntityMailBox( pLocalType_, addr_, id_ ),
			ScriptObject::FROM_NEW_REFERENCE );
	}

	return this->CommonCellEntityMailBox::pyGetAttribute( attrObj );
}


/**
 *	This method gets a stream to send a message to the cell on.
 */
BinaryOStream * BaseViaCellMailBox::getStreamEx(
		const MethodDescription & methodDesc,
		std::auto_ptr< Mercury::ReplyMessageHandler > pHandler )
{
	return this->getStreamCommon(
			methodDesc, CellAppInterface::callBaseMethod, pHandler );
}


/**
 *	This method returns the type of this mailbox.
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
class ClientViaBaseMailBox : public ServerEntityMailBox
{
	Py_Header( ClientViaBaseMailBox, ServerEntityMailBox )

public:
	ClientViaBaseMailBox( EntityTypePtr pBaseType,
				const Mercury::Address & addr, EntityID id,
				RecordingOption recordingOption =
					RECORDING_OPTION_METHOD_DEFAULT,
				PyTypeObject * pType = &s_type_ ):
		ServerEntityMailBox( pBaseType, addr, id, pType ),
		recordingOption_( recordingOption )
	{}

	virtual ~ClientViaBaseMailBox() { }

	virtual BinaryOStream * getStreamEx( const MethodDescription & methodDesc, 
		std::auto_ptr< Mercury::ReplyMessageHandler > pHandler );
	virtual EntityMailBoxRef::Component component() const;
	virtual const MethodDescription * findMethod( const char * attr ) const;

	PY_KEYWORD_METHOD_DECLARE( pyCall );

private:
	RecordingOption recordingOption_;
};

PY_SCRIPT_CONVERTERS_DECLARE( ClientViaBaseMailBox )

PY_TYPEOBJECT_WITH_CALL( ClientViaBaseMailBox )
PY_BEGIN_METHODS( ClientViaBaseMailBox )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( ClientViaBaseMailBox )
PY_END_ATTRIBUTES()

PY_SCRIPT_CONVERTERS( ClientViaBaseMailBox )


/*
 *	Override from base class.
 */
BinaryOStream * ClientViaBaseMailBox::getStreamEx( 
		const MethodDescription & methodDesc,
		std::auto_ptr< Mercury::ReplyMessageHandler > pHandler )
{
	Mercury::Bundle & bundle = this->bundle();

	// Return values not supported

	if (pHandler.get())
	{
		PyErr_Format( PyExc_TypeError,
				"Cannot call two-way method '%s' to Client",
				methodDesc.name().c_str() );
		return NULL;
	}

	BaseAppIntInterface::setClientArgs::start( bundle ).id = id_;

	bundle.startMessage( BaseAppIntInterface::callClientMethod );
	bundle << methodDesc.internalIndex();
	bundle << uint8( recordingOption_ );
	return &bundle;
}


/**
 *	This method returns the type of this mailbox.
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

	return new ClientViaBaseMailBox( EntityTypePtr( &this->localType() ),
		this->address(), this->id(), recordingOption );
}


// -----------------------------------------------------------------------------
// Section: ClientViaCellMailBox
// -----------------------------------------------------------------------------

/**
 *	This class is used to create a mailbox to a client entity. Traffic for the
 *	entity is sent via the cell entity.
 */
class ClientViaCellMailBox : public ServerEntityMailBox
{
	Py_Header( ClientViaCellMailBox, ServerEntityMailBox )

public:
	ClientViaCellMailBox( EntityTypePtr pBaseType,
				const Mercury::Address & addr, EntityID id,
				RecordingOption recordingOption =
					RECORDING_OPTION_METHOD_DEFAULT,
				PyTypeObject * pType = &s_type_ ):
		ServerEntityMailBox( pBaseType, addr, id, pType ),
		recordingOption_( recordingOption )
	{}

	virtual ~ClientViaCellMailBox() { }

	virtual BinaryOStream * getStreamEx( const MethodDescription & methodDesc,
			std::auto_ptr< Mercury::ReplyMessageHandler > pHandler );
	virtual EntityMailBoxRef::Component component() const;
	virtual const MethodDescription * findMethod( const char * attr ) const;

	PY_KEYWORD_METHOD_DECLARE( pyCall );

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


/*
 *	Override from base class.
 */
BinaryOStream * ClientViaCellMailBox::getStreamEx( 
		const MethodDescription & methodDesc,
		std::auto_ptr< Mercury::ReplyMessageHandler > pHandler )
{
	Mercury::Bundle & bundle = this->bundle();

	// Return values not supported

	if (pHandler.get())
	{
		PyErr_Format( PyExc_TypeError,
				"Cannot call two-way method '%s' to Client",
				methodDesc.name().c_str() );
		return NULL;
	}

	bundle.startMessage( CellAppInterface::callClientMethod );
	bundle << id_; // The CellApp entity
	bundle << id_; // The client entity

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
 *	This method returns the type of this mailbox.
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

	return new ClientViaCellMailBox( EntityTypePtr( &this->localType() ),
		this->address(), this->id(), recordingOption );
}


// -----------------------------------------------------------------------------
// Section: ServerEntityMailBox
// -----------------------------------------------------------------------------

/*~	class NoModule.ServerEntityMailBox
 *  @components{ base }
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
	 *	@components{ base }
	 *
	 *	The server component of the associated Entity.
	 *	@type string
	 */
	PY_ATTRIBUTE( component )

	/*~ attribute ServerEntityMailBox.className
	 *	@components{ base }
	 *
	 *	The class name of the associated Entity.
	 *	@type string
	 */
	PY_ATTRIBUTE( className )

	/*~ attribute ServerEntityMailBox.ip
	 *	@components{ base }
	 *
	 *	The IP address of the mailbox. If zero, the mailbox is invalid.
	 *	@type int32
	 */
	PY_ATTRIBUTE( ip )

PY_END_ATTRIBUTES()

PY_SCRIPT_CONVERTERS( ServerEntityMailBox )


/**
 *	Constructor.
 */
ServerEntityMailBox::ServerEntityMailBox( EntityTypePtr pBaseType,
		const Mercury::Address & addr, EntityID id,
		PyTypeObject * pType ) :
	PyEntityMailBox( pType ),
	addr_( addr ),
	id_( id ),
	pLocalType_( pBaseType )
{
}

/**
 *	Destructor
 */
ServerEntityMailBox::~ServerEntityMailBox()
{
}


/**
 *	This method overrides the PyEntityMailBox method so that we can add the id
 *	attribute.
 */
ScriptObject ServerEntityMailBox::pyGetAttribute( const ScriptString & attrObj )
{
	if (!MainThreadTracker::isCurrentThreadMain())
	{
		PyErr_Format( PyExc_AttributeError,
				"Mailbox property is not available in background threads" );
		return ScriptObject();
	}

	return this->PyEntityMailBox::pyGetAttribute( attrObj );
}


/**
 *	This method gets the stream to send a remote method call on.
 */
BinaryOStream * ServerEntityMailBox::getStream(
					const MethodDescription & methodDesc,
					std::auto_ptr< Mercury::ReplyMessageHandler > pHandler )
{
	if (!MainThreadTracker::isCurrentThreadMain())
	{
		ERROR_MSG( "ServerEntityMailBox::getStream: "
				"Cannot get stream in background thread for %s mailbox\n",
			this->componentName() );
		PyErr_Format( PyExc_TypeError,
			"Cannot get stream in background thread for %s mailbox\n",
			this->componentName() );
		return NULL;
	}

	return this->getStreamEx( methodDesc, pHandler );
}


/**
 *	This method sends the stream that getStream got.
 */
void ServerEntityMailBox::sendStream()
{
	// Sometimes a CellEntityMailbox is used even though it isn't pointing
	// anywhere, not technically needed but it helps prevent future
	// strangeness.
	Mercury::UDPChannel * pChannel = this->pChannel();

	if (!pChannel)
	{
		ERROR_MSG( "ServerEntityMailBox::sendStream: Channel is NULL."
			"Address: %s, id:%d\n", addr_.c_str(), id_ );
		return;
	}

	if (pChannel->addr().ip == 0)
	{
		INFO_MSG( "ServerEntityMailBox::sendStream: %s mailbox channel "
				"not established, buffering message for entity %d\n", 
			this->componentName(),
			id_ );
		return;
	}

	// Using this form of delayedSend() so that we send it soon regardless of
	// whether we have a player making the channel regular. 
	pChannel->networkInterface().delayedSend( *pChannel );
}


/**
 *  This method returns the most appropriate channel for this mailbox.  The
 *  simplest implementation is a regular addressed channel, but in some cases we
 *  may be able to use the entity channel.
 */
Mercury::UDPChannel * ServerEntityMailBox::pChannel() const
{
	if (addr_.ip == 0)
	{
		return NULL;
	}

	return &BaseApp::getChannel( addr_ );
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
	return EntityMailBoxRef::componentAsStr( this->component() );
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
			const BackupHashChain & backupHashChain )
{
	BaseBackupSwitchMailBoxVisitor visitor( backupHashChain, deadAddr );
	PyEntityMailBox::visit( visitor );
}


void ServerEntityMailBox::address( const Mercury::Address & addr )
{
	addr_ = addr;
}


PyObjectPtr ServerEntityMailBox::coerce( PyObject * pObject )
{
	if (Base::Check( pObject ))
	{
		Base * pBase = static_cast< Base * >( pObject );
		return PyObjectPtr(
				new BaseEntityMailBox( pBase->pType(),
					BaseApp::instance().intInterface().address(),
					pBase->id() ),
				PyObjectPtr::STEAL_REFERENCE );
	}

	return pObject;
}


// -----------------------------------------------------------------------------
// Section: CommonCellEntityMailBox
// -----------------------------------------------------------------------------

PY_SCRIPT_CONVERTERS_DECLARE( CommonCellEntityMailBox )

PY_TYPEOBJECT( CommonCellEntityMailBox )
PY_BEGIN_METHODS( CommonCellEntityMailBox )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( CommonCellEntityMailBox )
PY_END_ATTRIBUTES()


/**
 *  This method returns the best channel for this mailbox.
 */
Mercury::UDPChannel * CommonCellEntityMailBox::pChannel() const
{
	Base * pBase = BaseApp::instance().bases().findEntity( id_ );

	if (!pBase)
	{
		Mercury::UDPChannel * pChannel = this->ServerEntityMailBox::pChannel();

		if (!pChannel)
		{
			PyErr_SetString( PyExc_ValueError, "Invalid mailbox" );
			return NULL;
		}

		return pChannel;
	}

	// Disallow the call if we don't have a cell entity and we're not pending cell
	// creation, or if we're pending cell destruction.

	if ((!pBase->hasCellEntity() && 
			!pBase->isGetCellPending()) ||
		(pBase->isDestroyCellPending()))
	{
		PyErr_SetString( PyExc_ValueError, 
			"Base entity has no cell entity "
			"or cell entity is pending for destroy" );
		return NULL;
	}

	return &pBase->channel();
}


/**
 *	This method is used by derived classes to the initial part of their
 *	getStream methods.
 */
BinaryOStream * CommonCellEntityMailBox::getStreamCommon(
		const MethodDescription & methodDesc, 
		const Mercury::InterfaceElement & ie,
		std::auto_ptr< Mercury::ReplyMessageHandler > pHandler )
{
	Mercury::UDPChannel * pChannel = this->pChannel();

	if (!pChannel)
	{
		return NULL;
	}

	Mercury::Bundle & bundle = pChannel->bundle();

	if (pHandler.get())
	{
		bundle.startRequest( ie, pHandler.release() );
	}
	else
	{
		bundle.startMessage( ie );
	}

	bundle << id_;
	bundle << methodDesc.internalIndex();

	return &bundle;
}



// -----------------------------------------------------------------------------
// Section: CellEntityMailBox
// -----------------------------------------------------------------------------

/*~ class NoModule.CellEntityMailBox
 *	@components{ base }
 *
 *	CellEntityMailBox inherits from ServerEntityMailBox.
 *
 *	A CellEntityMailBox is used by a Base Entity to communicate with its
 *	associated Cell Entity.  This allows the Base Entity to call the methods
 *	on the Cell Entity.
 *
 *	For example:
 *
 *		self.cell.cellMethod()
 *
 *	where cellMethod() is described in the .def file.
 *
 *	These mailboxes have the same restrictions as Cell mailboxes and should not
 *	be stored, they should be used immediately then discarded.
 */
/*~ attribute CellEntityMailBox.base
 *	@components{ base }
 *
 *	An indirect mailbox to the base representitive this cell mailbox is
 *	associated with.
 *
 *	For example, to call the base methods via a cell mailbox 'cellMB', use:
 *
 *	cellMB.base.baseMethod()
 *
 *	The method call will first be sent to the cell this mailbox belongs to and
 *	then redirected to the base counterpart.
 *
 *	@type mailbox
 */
/*~ attribute CellEntityMailBox.client
 *	@components{ base }
 *
 *	An indirect mailbox to the client representitive this cell mailbox is
 *	associated with.
 *
 *	For example, to call the client methods via a cell mailbox 'cellMB', use:
 *
 *	cellMB.client.clientMethod()
 *
 *	The method call will first be sent to the cell this mailbox belongs to and
 *	then redirected to the client counterpart.
 *
 *	You can only call the methods on own client.
 *
 *	@type mailbox
 */
/*~ attribute CellEntityMailBox.ownClient
 *	@components{ base }
 *	An alias to CellEntityMailBox.client.
 *
 *	@type mailbox
 */
PY_TYPEOBJECT( CellEntityMailBox )

PY_BEGIN_METHODS( CellEntityMailBox )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( CellEntityMailBox )
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
 *	This method gets a stream to send a message to the cell on.
 */
BinaryOStream * CellEntityMailBox::getStreamEx(
	const MethodDescription & methodDesc,
	std::auto_ptr< Mercury::ReplyMessageHandler > pHandler )
{
	return this->getStreamCommon( methodDesc,
		CellAppInterface::runScriptMethod, pHandler );
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
// Section: BaseEntityMailBox
// -----------------------------------------------------------------------------

/*~	class NoModule.BaseEntityMailBox
 *  @components{ base }
 *
 *	BaseEntityMailBox inherits from ServerEntityMailBox.
 *
 *	On a Base, the BaseEntityMailBox can be used to communicate with other
 *	Base Entity components, even if on different machines.
 *
 *	For example, to befriend an entity you meet, use:
 *
 *		self.befriend( friendEntity )
 *
 *	The friend Entity will automatically be converted to a BaseEntityMailBox,
 *	so is identical to:
 *
 *		self.befriend( friendEntity.base )
 *
 *	In either case, the befriend function definition could look like:
 *
 *		def befriend( self, friendMailbox ):
 *			self.myFriend = friendMailbox
 *
 *	To communicate with the friend, you could:
 *
 *		def talkToFriend( self ):
 *			self.myFriend.chat( "Hi Friend..." )
 *
 *	where the chat method is implemented on the Base of the friend Entity.
 *	The stored BaseEntityMailBox will directly communicate with the Base
 *	Entity it represents, regardless of which machine it is on.

 *	Unlike direct Cell entity mailboxes, these mailboxes may be stored and used
 *	later.
 */
/*~ attribute BaseEntityMailBox.cell
 *	@components{ base }
 *	An indirect mailbox to the cell representitive this base mailbox is
 *	associated with.
 *
 *	For example, to call the cell methods via a base mailbox 'baseMB', use:
 *
 *	baseMB.cell.cellMethod()
 *
 *	The method call will first be sent to the base this mailbox belongs to and
 *	then redirected to the cell counterpart.
 *
 *	@type mailbox
 */
/*~ attribute BaseEntityMailBox.client
 *	@components{ base }
 *	An indirect mailbox to the client representitive this base mailbox is
 *	associated with.
 *
 *	For example, to call the client methods via a base mailbox 'baseMB', use:
 *
 *	baseMB.client.clientMethod()
 *
 *	The method call will first be sent to the base this mailbox belongs to and
 *	then redirected to the client counterpart.
 *
 *	@type mailbox
 */
/*~ attribute BaseEntityMailBox.ownClient
 *	@components{ base }
 *	An alias to BaseEntityMailBox.client.
 *
 *	@type mailbox
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

//PY_FACTORY_NAMED( BaseEntityMailBox, "BaseMailBox", BigWorld )


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

	if (!pLocalType_->isProxy())
	{
		PyErr_Format( PyExc_AttributeError,
			"Client mailbox does not refer to a proxy." );
		return NULL;
	}

	return new ClientViaBaseMailBox( pLocalType_, addr_, id_ );
}


/**
 *	This method gets a stream to send a message to a base.
 */
BinaryOStream * BaseEntityMailBox::getStreamEx(
		const MethodDescription & methodDesc,
	 	std::auto_ptr< Mercury::ReplyMessageHandler > pHandler )
{
	Mercury::Bundle & bundle = this->bundle();

	BaseAppIntInterface::setClientArgs::start( bundle ).id = id_;

	if (pHandler.get())
	{
		bundle.startRequest( BaseAppIntInterface::callBaseMethod,
				pHandler.release() );
	}
	else
	{
		bundle.startMessage( BaseAppIntInterface::callBaseMethod );
	}

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
	return pLocalType_->isService() ? EntityMailBoxRef::SERVICE :
		EntityMailBoxRef::BASE;
}


/**
 *	This class registers our classes into the PyEntityMailBox system,
 *	and provides some glue/helper functions for it.
 */
static class BaseAppPostOfficeAttendant
{
public:
	BaseAppPostOfficeAttendant()
	{
		PyEntityMailBox::registerMailBoxComponentFactory(
			EntityMailBoxRef::CELL,
			newCellMB, &CellEntityMailBox::s_type_ );
		PyEntityMailBox::registerMailBoxComponentFactory(
			EntityMailBoxRef::BASE,
			newBaseMB, &BaseEntityMailBox::s_type_ );
		PyEntityMailBox::registerMailBoxComponentFactory(
			EntityMailBoxRef::SERVICE,
			newBaseMB, &BaseEntityMailBox::s_type_ );
		PyEntityMailBox::registerMailBoxComponentFactory(
			EntityMailBoxRef::BASE_VIA_CELL,
			newBaseViaCellMB, &BaseViaCellMailBox::s_type_ );
		PyEntityMailBox::registerMailBoxComponentFactory(
			EntityMailBoxRef::CELL_VIA_BASE,
			newCellViaBaseMB, &CellViaBaseMailBox::s_type_ );
		PyEntityMailBox::registerMailBoxComponentFactory(
			EntityMailBoxRef::CLIENT_VIA_CELL,
			newClientViaCellMB, &ClientViaCellMailBox::s_type_ );
		PyEntityMailBox::registerMailBoxComponentFactory(
			EntityMailBoxRef::CLIENT_VIA_BASE,
			newClientViaBaseMB, &ClientViaBaseMailBox::s_type_ );
		PyEntityMailBox::registerMailBoxRefEquivalent(
			ServerEntityMailBox::Check, ServerEntityMailBox::static_ref );
		PyEntityMailBox::registerMailBoxRefEquivalent(
			Base::Check, baseReduce );
	}

	static PyObject * newCellMB( const EntityMailBoxRef & ref )
	{
		// TODO: Consider always returning a mailbox so data is not lost,
		// even when type id exceeds range ... probably not worthwhile.
		EntityTypePtr pLocalType = EntityType::getType( ref.type(),
			/*shouldExcludeServices*/ true );

		if (pLocalType && pLocalType->canBeOnCell())
		{
			return new CellEntityMailBox( pLocalType, ref.addr, ref.id );
		}

		Py_RETURN_NONE;
	}


	static PyObject * newBaseMB( const EntityMailBoxRef & ref )
	{
		// first see if we have that base ID already
		BaseApp & baseApp = BaseApp::instance();

		Mercury::Address addr = 
			baseApp.backupHashChain().addressFor( ref.addr, ref.id );

		bool shouldReturnMailBox = !BaseAppConfig::shouldResolveMailBoxes();

		if (addr == baseApp.intInterface().address())
		{
			// above test allows bases to all have own id addr spaces
			// not too sure whether or not that is a good thing 'tho
			const Bases & collection =
				(ref.component() == EntityMailBoxRef::SERVICE) ? 
					baseApp.localServiceFragments() : baseApp.bases();

			Base * pBase = collection.findEntity( ref.id );
			if (pBase != NULL)
			{
				if (pBase->pType()->description().index() != ref.type())
				{
					ERROR_MSG( "Base MailBoxRef resolver: "
						"Found local base entity %u for mailbox, "
						"but EntityTypes differ (local is %d not %d)!\n",
						ref.id, pBase->pType()->description().index(),
						ref.type() );
				}
			}
			else
			{
				EntityTypePtr pType = EntityType::getType( ref.type() );
				ERROR_MSG( "newBaseMB: Failed to resolve mailbox "
						"to this BaseApp for entity %d of type %d (%s)\n",
					ref.id, ref.type(), pType ? pType->name() : "Invalid" );

				// return an invalid mailbox
				shouldReturnMailBox = true;
			}

			if (!shouldReturnMailBox)
			{
				Py_XINCREF( pBase );
				return pBase;
			}
		}

		// TODO: Consider always returning a mailbox so data is not lost,
		// even when type id exceeds range ... probably not worthwhile.
		EntityTypePtr pLocalType = EntityType::getType( ref.type() );
		if (pLocalType)
		{
			return new BaseEntityMailBox( pLocalType, addr, ref.id );
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

	static EntityMailBoxRef baseReduce( PyObject * pObject )
	{
		Base * pBase = (Base*)pObject;
		return pBase->baseEntityMailBoxRef();
	}
} s_bapoa;

BW_END_NAMESPACE

// mailbox.cpp
