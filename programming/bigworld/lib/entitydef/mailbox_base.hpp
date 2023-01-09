#ifndef MAILBOX_BASE_HPP
#define MAILBOX_BASE_HPP

#include "script/script_object.hpp"

#include "cstdmf/binary_stream.hpp"
#include "network/basictypes.hpp"
#include "network/interfaces.hpp"
#include "pyscript/pyobject_plus.hpp"
#include "server/recording_options.hpp"

#include "cstdmf/bw_list.hpp"
#include <memory>


BW_BEGIN_NAMESPACE

class EntityType;
class MethodDescription;
class PyEntityMailBox;


/**
 *	Interface for visiting mailboxes in PyEntityMailBox::visit()
 */
class PyEntityMailBoxVisitor
{
public:

	/**
	 *	Constructor. 
	 */
	PyEntityMailBoxVisitor() {}


	/** 
	 *	Destructor. 
	 */
	virtual ~PyEntityMailBoxVisitor() {}


	/**
	 *	Virtual method for visiting a single mail box.
	 */
	virtual void onMailBox( PyEntityMailBox * pMailBox ) = 0;
};


/**
 *	This class is used to represent a destination of an entity that messages
 *	can be sent to.
 *
 *	Its virtual methods are implemented differently on each component.
 */
class PyEntityMailBox: public PyObjectPlus
{
	Py_Header( PyEntityMailBox, PyObjectPlus )

public:
	PyEntityMailBox( PyTypeObject * pType = &PyEntityMailBox::s_type_ );
	virtual ~PyEntityMailBox();

	virtual ScriptObject pyGetAttribute( const ScriptString & attrObj );

	PyObject * pyRepr();

	virtual const MethodDescription * findMethod( const char * attr ) const = 0;

	/**
	 *	Get a stream for the remote method to add arguments to. 
	 *
	 *	@param methodDesc	The method description.
	 *	@param pHandler		If the method requires a request, this is the
	 *						reply handler to use.
	 */
	virtual BinaryOStream * getStream( const MethodDescription & methodDesc, 
			std::auto_ptr< Mercury::ReplyMessageHandler > pHandler =
				std::auto_ptr< Mercury::ReplyMessageHandler >() ) = 0;
	virtual void sendStream() = 0;
	static PyObject * constructFromRef( const EntityMailBoxRef & ref );
	static bool reduceToRef( PyObject * pObject, EntityMailBoxRef * pRefOutput );

	virtual EntityID id() const = 0;
	virtual void address( const Mercury::Address & addr ) = 0;
	virtual const Mercury::Address address() const = 0;

	virtual void migrate() {}

	typedef PyObject * (*FactoryFn)( const EntityMailBoxRef & ref );
	static void registerMailBoxComponentFactory(
		EntityMailBoxRef::Component c, FactoryFn fn,
		PyTypeObject * pType );

	typedef bool (*CheckFn)( PyObject * pObject );
	typedef EntityMailBoxRef (*ExtractFn)( PyObject * pObject );
	static void registerMailBoxRefEquivalent( CheckFn cf, ExtractFn ef );

	PY_RO_ATTRIBUTE_DECLARE( this->id(), id );
	PyObject * pyGet_address();
	PY_RO_ATTRIBUTE_SET( address );
	
	PY_AUTO_METHOD_DECLARE( RETOWN, callMethod, 
		ARG( ScriptString, ARG( ScriptTuple, END ) ) );
	PyObject * callMethod(
		const ScriptString & methodName, const ScriptTuple & arguments  );

	PyObject * callMethod( 
		const MethodDescription * methodDescription,
		const ScriptTuple & args );

	PY_PICKLING_METHOD_DECLARE( MailBox )

	static void visit( PyEntityMailBoxVisitor & visitor );
	static bool parseRecordingOptionFromPythonCall(
		PyObject * args, PyObject * kwargs, RecordingOption & recordingOption );
private:
	typedef BW::list< PyEntityMailBox * > Population;
	static Population s_population_;

	Population::iterator populationIter_;
};

PY_SCRIPT_CONVERTERS_DECLARE( PyEntityMailBox )

namespace Script
{
	int setData( PyObject * pObj, EntityMailBoxRef & mbr,
		const char * varName = "" );
	PyObject * getData( const EntityMailBoxRef & mbr );
};

BW_END_NAMESPACE

#endif // MAILBOX_BASE_HPP

