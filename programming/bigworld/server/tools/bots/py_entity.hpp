#ifndef PY_ENTITY_HPP
#define PY_ENTITY_HPP

#include "pyscript/pyobject_plus.hpp"
#include "pyscript/script.hpp"

#include "entity.hpp"

#include "common/py_timer.hpp"

#include "cstdmf/binary_stream.hpp"
#include "cstdmf/smartpointer.hpp"

BW_BEGIN_NAMESPACE

class ClientApp;
typedef WeakPyPtr< ClientApp > ClientAppWPtr;

/*~ class BigWorld.Entity
 *  @components{ bots }
 *
 *	This is the Bots component of the Distributed Entity Model.
 *	It is responsible for handling an automated mimicking of similar Client
 *	entities. This includes interacting with other entities, and responding
 *	to updates from the server.
 *
 *	For more information refer to the Server Operations Guide, chapter
 *	Stress Testing With Bots.
 */
class PyEntity : public PyObjectPlus
{
	Py_Header( PyEntity, PyObjectPlus )

public:
	PyEntity( Entity & entity );
	~PyEntity();

	EntityPtr pEntity() const	{ return pEntity_; }

	EntityID entityID() const;
	bool isDestroyed() const;
	ClientApp * pClientApp() const;

	void onEntityDestroyed();

	/* Forwarded initialisation streams from Entity */
	bool initCellEntityFromStream( BinaryIStream & data );
	bool initBasePlayerFromStream( BinaryIStream & data );
	bool initCellPlayerFromStream( BinaryIStream & stream );

	/* Python Methods */
	PY_METHOD_DECLARE( py_addTimer )
	PY_METHOD_DECLARE( py_delTimer )

	/* Python Attributes */
	PY_RO_ATTRIBUTE_DECLARE( entityID(), id );
	PY_RO_ATTRIBUTE_DECLARE( isDestroyed(), isDestroyed );
	PY_RO_ATTRIBUTE_DECLARE( pEntity_ && pEntity_->isInWorld(), isInWorld );
	PY_DEFERRED_ATTRIBUTE_DECLARE( position );
	PY_DEFERRED_ATTRIBUTE_DECLARE( yaw );
	PY_DEFERRED_ATTRIBUTE_DECLARE( pitch );
	PY_DEFERRED_ATTRIBUTE_DECLARE( roll );
	PY_DEFERRED_ATTRIBUTE_DECLARE( cell );
	PY_DEFERRED_ATTRIBUTE_DECLARE( base );
	PY_RO_ATTRIBUTE_DECLARE( pClientApp(), clientApp );


private:
	bool pySetAttribute( const ScriptString & attrObj,
		const ScriptObject & value );

	EntityPtr pEntity_;
	EntityID entityID_;
	mutable ClientAppWPtr wpClientApp_;

	ScriptObject cell_;
	ScriptObject base_;

	PyTimer pyTimer_;
};

typedef ScriptObjectPtr< PyEntity > PyEntityPtr;

BW_END_NAMESPACE

#endif // PY_ENTITY_HPP
