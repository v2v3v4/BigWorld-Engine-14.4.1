#include "script/first_include.hpp"
#include "unittest_mailbox.hpp"

namespace BW
{

PY_TYPEOBJECT( UnittestMailBox )

PY_BEGIN_METHODS( UnittestMailBox )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( UnittestMailBox )
PY_END_ATTRIBUTES()

namespace
{

static PyObject * factory( const EntityMailBoxRef & ref )
{
	return new UnittestMailBox( ref );
}

/**
 *	This class is used to register the UnittestMailBox with PyEntityMailBox
 */
class StaticIniter
{
public:
	StaticIniter()
	{
		PyEntityMailBox::registerMailBoxRefEquivalent(
			UnittestMailBox::Check, UnittestMailBox::static_ref );
		PyEntityMailBox::registerMailBoxComponentFactory(
			EntityMailBoxRef::CELL, factory, &UnittestMailBox::s_type_ );
		PyEntityMailBox::registerMailBoxComponentFactory(
			EntityMailBoxRef::BASE, factory, &UnittestMailBox::s_type_ );
		PyEntityMailBox::registerMailBoxComponentFactory(
			EntityMailBoxRef::CLIENT, factory, &UnittestMailBox::s_type_ );
		PyEntityMailBox::registerMailBoxComponentFactory(
			EntityMailBoxRef::BASE_VIA_CELL, factory,
			&UnittestMailBox::s_type_ );
		PyEntityMailBox::registerMailBoxComponentFactory(
			EntityMailBoxRef::CLIENT_VIA_CELL, factory,
			&UnittestMailBox::s_type_ );
		PyEntityMailBox::registerMailBoxComponentFactory(
			EntityMailBoxRef::CELL_VIA_BASE, factory,
			&UnittestMailBox::s_type_ );
		PyEntityMailBox::registerMailBoxComponentFactory(
			EntityMailBoxRef::CLIENT_VIA_BASE, factory,
			&UnittestMailBox::s_type_ );
		PyEntityMailBox::registerMailBoxComponentFactory(
			EntityMailBoxRef::SERVICE, factory, &UnittestMailBox::s_type_ );

			
	}
};

StaticIniter staticIniter;

} // anonymous namespace

} // BW namespace

// unittest_mailbox.cpp
