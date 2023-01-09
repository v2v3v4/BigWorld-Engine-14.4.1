#ifndef PY_CLIENT_HPP
#define PY_CLIENT_HPP

#include "cstdmf/memory_stream.hpp"
#include "pyscript/pyobject_plus.hpp"
#include "server/recording_options.hpp"

BW_BEGIN_NAMESPACE

class Entity;


/*~ class NoModule.PyClient
 *  @components{ cell }
 *  A PyClient is the device through which a real cell Entity is able to call 
 *  methods on its client instances. A new PyClient object is created by 
 *  BigWorld each time an entity's "client", "otherClients" or "allClients"
 *  attributes are accessed, and each time an entity's "clientEntity" function
 *  is called. Objects of this type cannot otherwise be created via script.
 *
 *  A PyClient can be used to send method calls to either the client which owns
 *  the entity, all clients except the one which owns the entity, or all 
 *  clients. It can also be used to call methods on a different entity which is
 *  owned by the same client. These options are set when the PyClient is 
 *  created, and cannot be altered afterwards via python. The PyClient objects 
 *  provided by the entity members client, otherClients, allClients and 
 *  clientEntity are each set up differently in these respects, as described in
 *  the Entity class documentation.
 *
 *  PyClient objects have no default attributes or methods, however BigWorld 
 *  populates them with ClientCaller objects which represent the methods that
 *  can be called. These are built using information provided in the entity
 *  def files. A ClientCaller object can then be called as if it were the
 *  method that it represents, which causes it to add a request to call the
 *  appropriate function on the next bundles that are sent to the appropriate
 *  clients.
 *
 *  Example:
 *
 *  If the .def file "myEntityClass.def" contained the following
 *  @{
 *      &lt;ClientMethods&gt;
 *          &lt;pointAt&gt;
 *              &lt;Arg&gt; OBJECT_ID &lt;/Arg&gt; &lt;!-- entity to point at --&gt;
 *          &lt;/pointAt&gt;
 *      &lt;/ClientMethods&gt;
 *  @}
 *  then where "myEntity" is an instance of "myEntityClass", the following
 *  python code would be valid.
 *  @{
 *  # This example tells myEntity to point at the entity with ID #100, on a 
 *  # range of clients.
 *
 *  # the client which owns myEntity
 *  myEntity.client.pointAt( 100 )
 *
 *  # all clients other than the one which owns myEntity
 *  myEntity.otherClient.pointAt( 100 )
 *
 *  # all clients
 *  myEntity.allClients.pointAt( 100 )
 *  @}
 */
/**
 *	This class implements a Python type. It is used to implement the implicit
 *	properties "client", "otherClients" and "allClients" of server entities.
 *	It is also used to implement the clientEntity method.
 *
 *	These are used to send method calls to entities on client machines.
 */
class PyClient : public PyObjectPlus
{
	Py_Header( PyClient, PyObjectPlus )

public:

	PyClient( Entity & clientEntity, Entity & destEntity, bool isOwn,
			bool isForOthers, 
			RecordingOption recordingOption = RECORDING_OPTION_METHOD_DEFAULT,
			PyTypeObject * pType = &PyClient::s_type_ );
	~PyClient();

	ScriptObject pyGetAttribute( const ScriptString & attrObj );

	PY_KEYWORD_METHOD_DECLARE( pyCall )

	PyObject * pyRepr() const;

private:
	Entity & clientEntity_;
	Entity & destEntity_;
	bool isForOwn_;
	bool isForOthers_;

	RecordingOption recordingOption_;
};

BW_END_NAMESPACE

#endif // PY_CLIENT_HPP
