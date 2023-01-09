#ifndef PY_SERVER_HPP
#define PY_SERVER_HPP

#include "pyscript/pyobject_plus.hpp"
#include "script/script_object.hpp"


BW_BEGIN_NAMESPACE

class Entity;
class PyEntity;
typedef WeakPyPtr< PyEntity > PyEntityWPtr;

/*~ class BigWorld.PyServer
 *  @components{ bots }
 *
 *  A PyServer is the device through which an entity is able to call methods
 *  on its instance on a server. BigWorld creates two PyServer objects
 *  as attributes for each client entity, "cell" and "base", which can be
 *  used to send method calls to the entity's cell and base instances,
 *  respectively. These cannot be created in script.
 *
 *  PyServer objects have no default attributes or methods, however BigWorld
 *  populates them with ServerCaller objects which represent the methods that
 *  can be called. These are built using information provided in the entity
 *  def files. A ServerCaller object can then be called as if it were the
 *  method that it represents, which causes it to add a request to call the
 *  appropriate function on the next bundle that is sent to the server.
 *
 *  This is not supported for licensees who have not licensed the BigWorld
 *  server technology.
 */
/**
 *	This class presents the interface of the server part of an
 *	entity to the scripts that run on the client.
 */
class PyServer : public PyObjectPlus
{
	Py_Header( PyServer, PyObjectPlus )

public:
	PyServer( PyEntity * pPyEntity,
		bool isProxyCaller,
		PyTypeObject * pType = &PyServer::s_type_ );
	~PyServer();

	ScriptObject pyGetAttribute( const ScriptString & attrObj );

	void pyAdditionalMembers( const ScriptList & pList ) const;

private:
	Entity * pEntity() const;

	PyEntityWPtr wpPyEntity_;
	bool isProxyCaller_;
};

BW_END_NAMESPACE

#endif // PY_SERVER_HPP
