#ifndef UNITTEST_MAILBOX_HPP
#define UNITTEST_MAILBOX_HPP

#include "entitydef/mailbox_base.hpp"


namespace BW
{

/**
 *	This class is a PyEntityMailBox concrete implementation that cannot really
 *	be used, but can be streamed around as needed for entitydef unit tests.
 */
class UnittestMailBox : public PyEntityMailBox
{
	Py_Header( UnittestMailBox, PyEntityMailBox )
public:
	UnittestMailBox() : PyEntityMailBox( &UnittestMailBox::s_type_ ) {}
	UnittestMailBox( const EntityMailBoxRef & ref ) :
		PyEntityMailBox( &UnittestMailBox::s_type_ ),
		id_( ref.id ),
		address_( ref.addr ),
		component_( ref.component() ),
		typeID_( ref.type() )
	{}

	// PyEntityMailBox implementation
	const MethodDescription * findMethod( const char * attr ) const
	{
		return NULL;
	}

	BinaryOStream * getStream( const MethodDescription & methodDesc, 
		std::auto_ptr< Mercury::ReplyMessageHandler > pHandler )
	{
		return NULL;
	}

	void sendStream() {}

	EntityID id() const { return id_; };
	void address( const Mercury::Address & addr ) { address_ = addr; }
	const Mercury::Address address() const { return address_; }

	// PyEntityMailBox registration and to-from-Python mechanics
	static EntityMailBoxRef static_ref( PyObject * pThis )
	{
		return ((const UnittestMailBox*)pThis)->ref();
	}

	EntityMailBoxRef ref() const
	{
		EntityMailBoxRef mbr;
		mbr.init( id_, address_, component_, typeID_ );
		return mbr;
	}

	// UnittestMailBox methods
	void id( EntityID value ) { id_ = value; };
	EntityMailBoxRef::Component component() const { return component_; }
	void component( const EntityMailBoxRef::Component & value )
	{
		component_ = value;
	}
	EntityTypeID typeID() const { return typeID_; }
	void typeID( const EntityTypeID & value ) { typeID_ = value; }

private:
	EntityID id_;
	Mercury::Address address_;
	EntityMailBoxRef::Component component_;
	EntityTypeID typeID_;
};

} // namespace BW

#endif // UNITTEST_MAILBOX_HPP
