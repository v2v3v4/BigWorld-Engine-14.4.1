#ifndef CLIENT_ENTITY_MAILBOX_HPP
#define CLIENT_ENTITY_MAILBOX_HPP

#include "entitydef/mailbox_base.hpp"


BW_BEGIN_NAMESPACE

class EntityDescription;
class MethodDescription;
class Proxy;
class RemoteClientMethod;

/**
 *	This class is a mailbox that delivers to the client.
 */
class ClientEntityMailBox: public PyEntityMailBox
{
	Py_Header( ClientEntityMailBox, PyEntityMailBox )

public:
	ClientEntityMailBox( Proxy & proxy );

	virtual EntityID id() const;

	virtual void address( const Mercury::Address & address ) {}
	virtual const Mercury::Address address() const;

	virtual ScriptObject pyGetAttribute( const ScriptString & attrObj );
	virtual BinaryOStream * getStream( const MethodDescription & methodDesc, 
		std::auto_ptr< Mercury::ReplyMessageHandler > pHandler );
	virtual void sendStream();
	virtual const MethodDescription * findMethod( const char * attr ) const;

	const EntityDescription& getEntityDescription() const;

	EntityMailBoxRef ref() const;

	Proxy & proxy() { return proxy_; }

	PY_KEYWORD_METHOD_DECLARE( pyCall );

	static EntityMailBoxRef static_ref( PyObject * pThis )
		{ return ((const ClientEntityMailBox*)pThis)->ref(); }

private:
	Proxy & proxy_;
};

BW_END_NAMESPACE

#endif // CLIENT_ENTITY_MAILBOX_HPP
