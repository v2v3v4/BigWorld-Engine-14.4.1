#ifndef MAILBOX_HPP
#define MAILBOX_HPP


#include "entitydef/entity_description.hpp"
#include "entitydef/mailbox_base.hpp"

#include "network/udp_channel.hpp"

#include "pyscript/pyobject_plus.hpp"
#include "pyscript/script.hpp"

#include "server/backup_hash.hpp"

#include "entity_type.hpp"


BW_BEGIN_NAMESPACE

class EntityType;


/**
 *	This class implements a mailbox that can send to a server object. This
 *	object may be on a cell or may be a base.
 *
 *	@see CellEntityMailBox
 *	@see BaseEntityMailBox
 */
class ServerEntityMailBox: public PyEntityMailBox
{
	Py_Header( ServerEntityMailBox, PyEntityMailBox )

public:
	ServerEntityMailBox( EntityTypePtr pBaseType,
			const Mercury::Address & addr, EntityID id,
			PyTypeObject * pType = &s_type_ );
	virtual ~ServerEntityMailBox();

	virtual const Mercury::Address		address() const		{ return addr_; }
	virtual void address( const Mercury::Address & addr )	{ addr_ = addr; }
	virtual void migrate();

	virtual EntityID			id() const			{ return id_; }

	PY_RO_ATTRIBUTE_DECLARE( this->componentName(), component );
	PY_RO_ATTRIBUTE_DECLARE( pLocalType_->name(), className );
	PY_RO_ATTRIBUTE_DECLARE( addr_.ip, ip );

	EntityMailBoxRef ref() const;
	virtual EntityMailBoxRef::Component component() const = 0;
	const char * componentName() const;

	static EntityMailBoxRef static_ref( PyObject * pThis )
		{ return ((const ServerEntityMailBox*)pThis)->ref(); }

	static void migrateMailBoxes();
	static void adjustForDeadBaseApp( const Mercury::Address & deadAddr,
			const BackupHash & backupHash );

protected:

	Mercury::Address			addr_;
	EntityID					id_;

	EntityTypePtr	pLocalType_;
};

PY_SCRIPT_CONVERTERS_DECLARE( ServerEntityMailBox )


/**
 *	This class implements a mailbox that can send to an object on a cell.
 */
class CellEntityMailBox: public ServerEntityMailBox
{
	Py_Header( CellEntityMailBox, ServerEntityMailBox )

public:
	CellEntityMailBox( EntityTypePtr pBaseType,
			const Mercury::Address & addr, EntityID id,
			PyTypeObject * pType = &s_type_ ) :
		ServerEntityMailBox( pBaseType, addr, id, pType )
	{}

	// Mailbox getter methods and generated setters.
	PyObject * pyGet_base();
	PY_RO_ATTRIBUTE_SET( base )

	PyObject * pyGet_client();
	PY_RO_ATTRIBUTE_SET( client )

	virtual BinaryOStream * getStream( const MethodDescription & methodDesc, 
		std::auto_ptr< Mercury::ReplyMessageHandler > pHandler );
	void sendStream();
	virtual const MethodDescription * findMethod( const char * attr ) const;
	virtual EntityMailBoxRef::Component component() const;
protected:
	Mercury::UDPChannel * pChannel() const;
};

PY_SCRIPT_CONVERTERS_DECLARE( CellEntityMailBox )

typedef SmartPointer<CellEntityMailBox> CellEntityMailBoxPtr;


/**
 *	This class is common to all mailboxes that send to the base entity or via
 *	the base entity.
 */
class CommonBaseEntityMailBox : public ServerEntityMailBox
{
	Py_Header( CommonBaseEntityMailBox, ServerEntityMailBox )

public:
	CommonBaseEntityMailBox( EntityTypePtr pBaseType,
			const Mercury::Address & addr, EntityID id,
			PyTypeObject * pType = &s_type_ ) :
		ServerEntityMailBox( pBaseType, addr, id, pType )
	{}
	void sendStream();

protected:
	Mercury::Bundle & bundle() const;

private:
	virtual Mercury::UDPChannel & channel() const;
	Mercury::UDPChannel & channel( Entity * pEntity ) const;
};


/**
 *	This class implements a mailbox that can send to a base object.
 */
class BaseEntityMailBox: public CommonBaseEntityMailBox
{
	Py_Header( BaseEntityMailBox, CommonBaseEntityMailBox )

public:
	BaseEntityMailBox( EntityTypePtr pBaseType,
			const Mercury::Address & addr, EntityID id,
			PyTypeObject * pType = &s_type_ ) :
		CommonBaseEntityMailBox( pBaseType, addr, id, pType )
	{}

	// Mailbox getter methods and generated setters.
	PyObject * pyGet_cell();
	PY_RO_ATTRIBUTE_SET( cell )

	PyObject * pyGet_client();
	PY_RO_ATTRIBUTE_SET( client )

	virtual BinaryOStream * getStream( const MethodDescription & methodDesc,
			std::auto_ptr< Mercury::ReplyMessageHandler > pHandler );
	virtual const MethodDescription * findMethod( const char * attr ) const;
	virtual EntityMailBoxRef::Component component() const;
};

PY_SCRIPT_CONVERTERS_DECLARE( BaseEntityMailBox )

BW_END_NAMESPACE

#endif // MAILBOX_HPP
