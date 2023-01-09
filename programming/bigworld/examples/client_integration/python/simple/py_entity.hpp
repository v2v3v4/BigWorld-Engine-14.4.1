#ifndef PY_ENTITY_HPP
#define PY_ENTITY_HPP


#include "entity.hpp"

#include "pyscript/pyobject_plus.hpp"


BW_BEGIN_NAMESPACE

class PyEntity : public PyObjectPlus
{
	Py_InstanceHeader( PyEntity )
public:
	PyEntity( Entity & entity );
	~PyEntity();

	void onEntityDestroyed();

	Entity * pEntity() const;

	/// Python-exposed entity interface
	PY_DEFERRED_ATTRIBUTE_DECLARE( cell );
	PY_DEFERRED_ATTRIBUTE_DECLARE( base );

	PY_DEFERRED_ATTRIBUTE_DECLARE( position );
	PY_DEFERRED_ATTRIBUTE_DECLARE( yaw );
	PY_DEFERRED_ATTRIBUTE_DECLARE( pitch );
	PY_DEFERRED_ATTRIBUTE_DECLARE( roll );
	PY_DEFERRED_ATTRIBUTE_DECLARE( id );
	PY_DEFERRED_ATTRIBUTE_DECLARE( spaceID );

	PY_RO_ATTRIBUTE_DECLARE( pEntity_ == NULL, isDestroyed );

	/// Forwarded entity initialisation events from Entity
	bool initFromStream( BinaryIStream & data );
	bool initBasePlayerFromStream( BinaryIStream & data );
	bool initCellPlayerFromStream( BinaryIStream & data );

private:
	void callInit();

	BWEntity * pEntity_;

	PyObject * pPyCell_;
	PyObject * pPyBase_;
};

BW_END_NAMESPACE

#endif // PY_ENTITY_HPP

