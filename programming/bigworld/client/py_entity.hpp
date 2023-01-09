#ifndef PY_ENTITY_HPP
#define PY_ENTITY_HPP

#include "entity.hpp"

#include "cstdmf/binary_stream.hpp"
#include "cstdmf/smartpointer.hpp"

#include "pyscript/pyobject_plus.hpp"

#include "script/script_object.hpp"

BW_BEGIN_NAMESPACE

class PyFilter;
typedef ScriptObjectPtr< PyFilter > PyFilterPtr;

/*~ class BigWorld.Entity
 *  @components{ client }
 *
 *	This is the Client component of the Distributed Entity Model.
 *	It is responsible for handling the visual aspects of the Entity,
 *	updates from the server, etc...
 *
 *	It is possible to create a client-only Entity using
 *	BigWorld.createEntity function.
 */
/**
 *	Instances of this class are client representation of entities,
 *	the basic addressable object in the BigWorld system.
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

	void onEntityDestroyed();

	void setFilterOnEntity();

	/* Forwarded initialisation streams from Entity */
	bool initCellEntityFromStream( BinaryIStream & data );
	bool initBasePlayerFromStream( BinaryIStream & data );
	bool initCellPlayerFromStream( BinaryIStream & stream );

	/* Script stuff */
	bool makePlayer();
	bool makeNonPlayer();
	void swizzleClass( PyTypeObject * pOldClass, PyTypeObject * pNewClass );

	/* Python Methods */
	bool prints( BW::string );
	PY_AUTO_METHOD_DECLARE( RETOK, prints, ARG( BW::string, END ) )

	bool addModel( IEntityEmbodimentPtr pCA );
	bool delModel( PyObjectPtr pEmbodimentPyObject );
	PY_AUTO_METHOD_DECLARE( RETOK, addModel, NZARG( IEntityEmbodimentPtr, END ) )
	PY_AUTO_METHOD_DECLARE( RETOK, delModel, NZARG( PyObjectPtr, END ) )

	PyObject * addTrap( float radius, ScriptObject callback );
	PY_AUTO_METHOD_DECLARE( RETOWN, addTrap, ARG( float,
		CALLABLE_ARG( ScriptObject, END ) ) )
	bool delTrap( int trapID );
	PY_AUTO_METHOD_DECLARE( RETOK, delTrap, ARG( int, END ) )

	PyObject * hasTargetCap( uint i ) const;
	PY_AUTO_METHOD_DECLARE( RETOWN, hasTargetCap, ARG( uint, END ) )

	bool setInvisible( bool invis = true );
	PY_AUTO_METHOD_DECLARE( RETOK, setInvisible, OPTARG( bool, true, END ) )

	bool setPortalState( bool isOpen, WorldTriangle::Flags collisionFlags );
	PY_AUTO_METHOD_DECLARE( RETOK, setPortalState,
		ARG( bool, OPTARG( WorldTriangle::Flags, 0, END ) ) )

	PyObject * aoiEntities();

	bool isPlayer() const;

	/* Python Attributes */
	PY_RO_ATTRIBUTE_DECLARE( entityID(), id );
	PY_RO_ATTRIBUTE_DECLARE( isDestroyed(), isDestroyed );
	PY_DEFERRED_ATTRIBUTE_DECLARE( aoiEntities );
	PY_DEFERRED_ATTRIBUTE_DECLARE( isPlayer );
	PY_DEFERRED_ATTRIBUTE_DECLARE( isWitness );
	PY_DEFERRED_ATTRIBUTE_DECLARE( isClientOnly );
	PY_DEFERRED_ATTRIBUTE_DECLARE( position );
	PY_DEFERRED_ATTRIBUTE_DECLARE( velocity );
	PY_DEFERRED_ATTRIBUTE_DECLARE( yaw );
	PY_DEFERRED_ATTRIBUTE_DECLARE( pitch );
	PY_DEFERRED_ATTRIBUTE_DECLARE( roll );
	PY_DEFERRED_ATTRIBUTE_DECLARE( cell );
	PY_DEFERRED_ATTRIBUTE_DECLARE( base );
	PY_DEFERRED_ATTRIBUTE_DECLARE( model );
	PY_DEFERRED_ATTRIBUTE_DECLARE( models );
	PY_DEFERRED_ATTRIBUTE_DECLARE( filter );
	PY_DEFERRED_ATTRIBUTE_DECLARE( isPoseVolatile );
	PY_DEFERRED_ATTRIBUTE_DECLARE( targetCaps );
	PY_DEFERRED_ATTRIBUTE_DECLARE( inWorld );
	PY_DEFERRED_ATTRIBUTE_DECLARE( spaceID );
	PY_DEFERRED_ATTRIBUTE_DECLARE( vehicle );
	PY_DEFERRED_ATTRIBUTE_DECLARE( physics );
	PY_DEFERRED_ATTRIBUTE_DECLARE( matrix );
	PY_DEFERRED_ATTRIBUTE_DECLARE( targetFullBounds );

private:
	ScriptObject pyGetAttribute( const ScriptString & attrObj );
	bool pySetAttribute( const ScriptString & attrObj, 
		const ScriptObject & value );

	void setClass( PyTypeObject * pClass );

	EntityPtr pEntity_;
	PySTLSequenceHolder< EntityEmbodiments > auxModelsHolder_;
	ScriptObject cell_;
	ScriptObject base_;
	ScriptObject pyAoIEntities_;

	PyFilterPtr pPyFilter_;
};

#ifndef PyEntity_CONVERTERS
#define PyEntity_CONVERTERS
PY_SCRIPT_CONVERTERS_DECLARE( PyEntity )
#endif // PyEntity_CONVERTERS

BW_END_NAMESPACE

#endif // PY_ENTITY_HPP

/* py_entity.hpp */
