#include "script/first_include.hpp"

#include "cellapp/entity.hpp"
#include "cellapp/real_entity.hpp"

#include "entitydef/script_data_source.hpp"

#include "server/stream_helper.hpp"

// This file contains some example Python methods that may be useful during
// debugging. These will be placed in a BWDebug module.


BW_BEGIN_NAMESPACE

/*
 *	This class is used by to generate a Python list of the data associated with
 *	an entity as it woud appear on a network stream.
 */
class StreamSizeVisitor : public IDataDescriptionVisitor
{
public:
	StreamSizeVisitor( ScriptObject & pEntity ) :
		pEntity_( pEntity ),
		pList_( ScriptList::create() )
	{
	}

	~StreamSizeVisitor()
	{
	}

	/*
	 *	This method is called for each of the DataDescription objects associated
	 *	with the entity in a specified subset. For example, all data flagged as
	 *	CELL_DATA.
	 */
	virtual bool visit( const DataDescription & propDesc )
	{
		ScriptObject pProp = pEntity_.getAttribute( propDesc.name().c_str(),
			ScriptErrorPrint() );

		if (!pProp)
		{
			return false;
		}

		MemoryOStream stream;
		uint64 startTime = timestamp();
		ScriptDataSource source( pProp );
		propDesc.addToStream( source, stream, /*isPersistentOnly*/ false );
		double timeTaken = (timestamp() - startTime)/stampsPerSecondD();

		ScriptTuple pTuple = ScriptTuple::create( 4 );
		
		pTuple.setItem( 0, ScriptString::create( propDesc.name() ) );
		
		pTuple.setItem( 1, ScriptString::create( 
			static_cast<char *>( stream.data() ), stream.size() ) );

		pTuple.setItem( 2, ScriptFloat::create( timeTaken ) );
		
		pTuple.setItem( 3, 
			ScriptString::create( propDesc.getDataFlagsAsStr() ) );
			
		pList_.append( pTuple );

		return true;
	}

	ScriptList pList() const	{ return pList_; }

private:
	ScriptObject pEntity_;
	ScriptList pList_;
};


/*
 *	This method returns some debug information about the entity's streamed data
 *	sizes.
 *
 *	It returns a Python list. Each entry corresponds to a CELL_DATA property is
 *	a tuple of size four containing the property's name, a blob as would appear
 *	on the network, the time taken to generate the blob and the flags associated
 *	with the property.
 */
ScriptList calcStreamedProperties( ScriptObject pEnt )
{
	if (!Entity::Check( pEnt.get() ))
	{
		PyErr_SetString( PyExc_TypeError,
				"Expected an entity as the argument" );
		return ScriptList();
	}

	Entity * pEntity = static_cast< Entity * >( pEnt.get() );
	if (!pEntity->isRealToScript())
	{
		PyErr_SetString( PyExc_TypeError,
				"Can only be called on a real entity" );
		return ScriptList();
	}

	EntityTypePtr pType = pEntity->pType();
	const EntityDescription & desc = pType->description();

	StreamSizeVisitor visitor( pEnt );
	// Currently hard-coded to be only CELL_DATA. This could be made to be
	// configurable.
	desc.visit( EntityDescription::CELL_DATA, visitor );

	ScriptList pReturn = visitor.pList();

	return pReturn;
}
PY_AUTO_MODULE_FUNCTION( RETDATA,
		calcStreamedProperties, ARG( ScriptObject, END ), BWDebug )


/*
 *	This method returns a Python blob (string) containing the data that would
 *	be added to the network when a real entity is being offloaded.
 */
PyObject * calcOffloadData( ScriptObject pEnt )
{
	if (!Entity::Check( pEnt.get() ))
	{
		PyErr_SetString( PyExc_TypeError,
				"Expected an entity as the argument" );
		return NULL;
	}

	Entity * pEntity = static_cast< Entity * >( pEnt.get() );

	if (!pEntity->isRealToScript())
	{
		PyErr_SetString( PyExc_TypeError,
			"BWDebug.calcOffloadData: can only be called on a real entity" );
		return NULL;
	}

	MemoryOStream stream;
	pEntity->writeRealDataToStream( stream, true );

	return PyString_FromStringAndSize( static_cast< char * >( stream.data() ),
			stream.size() );
}
PY_AUTO_MODULE_FUNCTION( RETOWN,
		calcOffloadData, ARG( ScriptObject, END ), BWDebug )


/*
 *	This method returns a Python blob (string) containing the data that would
 *	be added to the network when a real entity is being backed up.
 */
PyObject * calcCellBackupData( PyObjectPtr pEnt )
{
	if (!Entity::Check( pEnt.get() ))
	{
		PyErr_SetString( PyExc_TypeError,
				"Expected an entity as the argument" );
		return NULL;
	}

	Entity * pEntity = static_cast< Entity * >( pEnt.get() );

	if (!pEntity->isRealToScript())
	{
		PyErr_SetString( PyExc_TypeError,
			"BWDebug.calcCellBackupData: can only be called on a real entity" );
		return NULL;
	}

	MemoryOStream stream;
	pEntity->pReal()->writeBackupProperties( stream );

	return PyString_FromStringAndSize( static_cast< char * >( stream.data() ),
			stream.size() );
}
PY_AUTO_MODULE_FUNCTION( RETOWN,
		calcCellBackupData, ARG( PyObjectPtr, END ), BWDebug )


BW_END_NAMESPACE

// debug_extension.cpp
