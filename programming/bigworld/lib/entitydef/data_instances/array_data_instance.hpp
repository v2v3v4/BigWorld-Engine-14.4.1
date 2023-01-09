#ifndef ARRAY_DATA_INSTANCE_HPP
#define ARRAY_DATA_INSTANCE_HPP

#include "intermediate_property_owner.hpp"

#include "entitydef/data_types/array_data_type.hpp"

#include "script/script_object.hpp"

BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: PyArrayDataInstance
// -----------------------------------------------------------------------------

/**
 *	This class is the Python object for instances of a mutable array.
 */
class PyArrayDataInstance : public IntermediatePropertyOwner
{
	Py_Header( PyArrayDataInstance, PropertyOwner );

public:
	PyArrayDataInstance( const ArrayDataType * pDataType,
		size_t initialSize, PyTypeObject * pPyType = &s_type_ );
	~PyArrayDataInstance();

	void setInitialItem( int index, ScriptObject pItem );
	void setInitialSequence( ScriptObject pSeq );

	const ArrayDataType * dataType() const			{ return pDataType_.get(); }
	void setDataType( ArrayDataType * pDataType )	{ pDataType_ = pDataType; }

	virtual int getNumOwnedProperties() const;
	virtual PropertyOwnerBase * getChildPropertyOwner( int ref ) const;
	virtual ScriptObject setOwnedProperty( int ref, BinaryIStream & data );
	virtual ScriptObject setOwnedSlice( int startIndex, int endIndex,
			BinaryIStream & data );
	virtual ScriptObject getPyIndex( int index ) const;

	PyObject * pyRepr();

	int strictSize() const;
	PY_RO_ATTRIBUTE_DECLARE( strictSize(), strictSize );


	PY_SIZE_INQUIRY_METHOD(			pySeq_length )			// len(x)
	PY_BINARY_FUNC_METHOD(			pySeq_concat )			// x + y
	PY_INTARG_FUNC_METHOD(			pySeq_repeat )			// x * n
	PY_INTARG_FUNC_METHOD(			pySeq_item )			// x[i]
	PY_INTINTARG_FUNC_METHOD(		pySeq_slice )			// x[i:j]
	PY_INTOBJARG_PROC_METHOD(		pySeq_ass_item )		// x[i] = v
	PY_INTINTOBJARG_PROC_METHOD(	pySeq_ass_slice )		// x[i:j] = v
	PY_OBJOBJ_PROC_METHOD(			pySeq_contains )		// v in x
	PY_BINARY_FUNC_METHOD(			pySeq_inplace_concat )	// x += y
	PY_INTARG_FUNC_METHOD(			pySeq_inplace_repeat )	// x *= n

	bool append( ScriptObject pObject );
	PY_AUTO_METHOD_DECLARE( RETOK, append, ARG( ScriptObject, END ) )

	int count( ScriptObject pObject );
	PY_AUTO_METHOD_DECLARE( RETDATA, count, ARG( ScriptObject, END ) )

	bool extend( ScriptObject pObject );
	PY_AUTO_METHOD_DECLARE( RETOK, extend, ARG( ScriptObject, END ) )

	PY_METHOD_DECLARE( py_index );

	bool insert( int before, ScriptObject pObject );
	PY_AUTO_METHOD_DECLARE( RETOK, insert, ARG( int, ARG( ScriptObject, END ) ) )

	PyObject * pop( int index );
	PY_AUTO_METHOD_DECLARE( RETOWN, pop, OPTARG( int, -1, END ) )

	bool remove( ScriptObject pObject );
	PY_AUTO_METHOD_DECLARE( RETOK, remove, ARG( ScriptObject, END ) )

	bool equals_seq( ScriptObject pObject );
	PY_AUTO_METHOD_DECLARE( RETDATA, equals_seq, ARG( ScriptObject, END ) )

	PY_PICKLING_METHOD_DECLARE( Array )

	static PyObject * PickleResolve( ScriptObject list );
	PY_AUTO_UNPICKLING_FACTORY_DECLARE( ARG( ScriptObject, END ), Array )

	static int pyCompare( PyObject * a, PyObject * b );
private:

	bool isSmart() const	{ return true; }

	int findFrom( uint beg, PyObject * needle );

	void deleteElements( Py_ssize_t startIndex, Py_ssize_t endIndex,
			ScriptObject * ppOldValues );

	bool setSlice( Py_ssize_t startIndex, Py_ssize_t endIndex,
		const BW::vector< ScriptObject > & newValues, bool notifyOwner,
		ScriptObject * ppOldValues = NULL );

	bool getValuesFromSequence( PyObject * pSeq,
		BW::vector< ScriptObject > & values );

	ConstSmartPointer<ArrayDataType>	pDataType_;

	BW::vector<ScriptObject>			values_;

};

typedef ScriptObjectPtr< PyArrayDataInstance > PyArrayDataInstancePtr;

BW_END_NAMESPACE

#endif // ARRAY_DATA_INSTANCE_HPP
