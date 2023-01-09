#ifndef MAILBOX_HPP
#define MAILBOX_HPP


#include "entitydef/entity_description.hpp"
#include "entitydef/mailbox_base.hpp"
#include "network/udp_channel.hpp"
#include "pyscript/pyobject_plus.hpp"
#include "pyscript/script.hpp"

#include "entity_type.hpp"


BW_BEGIN_NAMESPACE

class BackupHashChain;


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

	virtual ScriptObject pyGetAttribute( const ScriptString & attrObj );
	void sendStream();

	virtual const Mercury::Address		address() const		{ return addr_; }
	virtual void address( const Mercury::Address & addr );

	virtual Mercury::UDPChannel  * pChannel() const;
	Mercury::Bundle & bundle() const { return this->pChannel()->bundle(); }

	virtual EntityID				id() const			{ return id_; }

	EntityMailBoxRef ref() const;
	virtual EntityMailBoxRef::Component component() const = 0;
	const char * componentName() const;

	virtual BinaryOStream * getStream( const MethodDescription & methodDesc,
		   std::auto_ptr< Mercury::ReplyMessageHandler > pHandler );

	static EntityMailBoxRef static_ref( PyObject * pThis )
		{ return ((const ServerEntityMailBox*)pThis)->ref(); }

	static void adjustForDeadBaseApp( const Mercury::Address & deadAddr,
			const BackupHashChain & hash );

	EntityType & localType() const { return *pLocalType_; }

	static void migrateMailBoxes();
	virtual void migrate();

	PY_RO_ATTRIBUTE_DECLARE( this->componentName(), component );
	PY_RO_ATTRIBUTE_DECLARE( pLocalType_->name(), className );
	PY_RO_ATTRIBUTE_DECLARE( addr_.ip, ip );

	static PyObjectPtr coerce( PyObject * pObject );

protected:
	virtual BinaryOStream * getStreamEx( const MethodDescription & methodDesc,
		   std::auto_ptr< Mercury::ReplyMessageHandler > pHandler ) = 0;

	Mercury::Address			addr_;
	EntityID					id_;

	EntityTypePtr				pLocalType_;
};

PY_SCRIPT_CONVERTERS_DECLARE( ServerEntityMailBox )

/**
 *	This class is common to all mailboxes that send to the cell entity or via
 *	the cell entity.
 */
class CommonCellEntityMailBox : public ServerEntityMailBox
{
	Py_Header( CommonCellEntityMailBox, ServerEntityMailBox )

public:
	CommonCellEntityMailBox( EntityTypePtr pBaseType,
			const Mercury::Address & addr, EntityID id,
			PyTypeObject * pType = &s_type_ ) :
		ServerEntityMailBox( pBaseType, addr, id, pType )
	{}

	virtual Mercury::UDPChannel * pChannel() const;
protected:
	BinaryOStream * getStreamCommon( const MethodDescription & methodDesc,
			const Mercury::InterfaceElement & ie, 
			std::auto_ptr< Mercury::ReplyMessageHandler > pHandler );

};


/**
 *	This class implements a mailbox that can send to an object on a cell.
 */
class CellEntityMailBox: public CommonCellEntityMailBox
{
	Py_Header( CellEntityMailBox, CommonCellEntityMailBox )

public:
	CellEntityMailBox( EntityTypePtr pBaseType,
			const Mercury::Address & addr, EntityID id,
			PyTypeObject * pType = &s_type_ ) :
		CommonCellEntityMailBox( pBaseType, addr, id, pType )
	{}

	// Mailbox getter methods and generated setters.
	PyObject * pyGet_base();
	PY_RO_ATTRIBUTE_SET( base )

	PyObject * pyGet_client();
	PY_RO_ATTRIBUTE_SET( client )

	virtual BinaryOStream * getStreamEx( const MethodDescription & methodDesc, 
			std::auto_ptr< Mercury::ReplyMessageHandler > pHandler );

	virtual const MethodDescription * findMethod( const char * attr ) const;
	virtual EntityMailBoxRef::Component component() const;
};

PY_SCRIPT_CONVERTERS_DECLARE( CellEntityMailBox )



/**
 *	This class implements a mailbox that can send to a base object.
 */
class BaseEntityMailBox: public ServerEntityMailBox
{
	Py_Header( BaseEntityMailBox, ServerEntityMailBox )

public:
	BaseEntityMailBox( EntityTypePtr pBaseType,
			const Mercury::Address & addr, EntityID id,
			PyTypeObject * pType = &s_type_ ) :
		ServerEntityMailBox( pBaseType, addr, id, pType )
	{}

	// Mailbox getter methods and generated setters.
	PyObject * pyGet_cell();
	PY_RO_ATTRIBUTE_SET( cell )

	PyObject * pyGet_client();
	PY_RO_ATTRIBUTE_SET( client )

	virtual BinaryOStream * getStreamEx( const MethodDescription & methodDesc, 
			std::auto_ptr< Mercury::ReplyMessageHandler > pHandler );
	virtual const MethodDescription * findMethod( const char * attr ) const;
	virtual EntityMailBoxRef::Component component() const;

//	static PyObject * New( EntityID id, PyObjectPtr pEntityType,
//		uint32 ip, uint16 port );
//	PY_AUTO_FACTORY_DECLARE( BaseEntityMailBox, ARG( EntityID,
//		ARG( PyObjectPtr, ARG( uint32, ARG( uint16, END ) ) ) ) )
};

PY_SCRIPT_CONVERTERS_DECLARE( BaseEntityMailBox )

BW_END_NAMESPACE


#endif // MAILBOX_HPP
