#ifndef PY_FIXED_DICT_DATA_INSTANCE_HPP
#define PY_FIXED_DICT_DATA_INSTANCE_HPP

#include "intermediate_property_owner.hpp"

#include "entitydef/data_types/fixed_dict_data_type.hpp"

#include "script/script_object.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class is the Python object for instances of a FIXED_DICT.
 *	It monitors changes to its properties, and tells its owner when they
 *	change so that its owner can propagate the changes around.
 */
class PyFixedDictDataInstance : public IntermediatePropertyOwner
{
	Py_Header( PyFixedDictDataInstance, PropertyOwner );

public:
	PyFixedDictDataInstance( FixedDictDataType* pDataType );
	PyFixedDictDataInstance( FixedDictDataType* pDataType,
		const PyFixedDictDataInstance& other );
	~PyFixedDictDataInstance();

	void initFieldValue( int index, ScriptObject val );

	ScriptObject getFieldValue( int index )	{ return fieldValues_[index]; }

	const FixedDictDataType& dataType() const		{ return *pDataType_; }
	void setDataType( FixedDictDataType* pDataType ){ pDataType_ = pDataType; }

	static bool isSameType( ScriptObject pValue,
			const FixedDictDataType& dataType );

	// PropertyOwner overrides
	virtual int getNumOwnedProperties() const;
	virtual PropertyOwnerBase * getChildPropertyOwner( int ref ) const;
	virtual ScriptObject setOwnedProperty( int ref, BinaryIStream & data );
	virtual ScriptObject getPyIndex( int index ) const;

	// PyObjectPlus framework
	ScriptObject pyGetAttribute( const ScriptString & attrObj );
	bool pySetAttribute( const ScriptString & attrObj,
		const ScriptObject & value );

	PY_PICKLING_METHOD_DECLARE( FixedDict )
	static PyObject * PickleResolve( ScriptObject list );
	PY_AUTO_UNPICKLING_FACTORY_DECLARE( ARG( ScriptObject, END ), FixedDict )

	// Python Mapping interface functions
	PY_METHOD_DECLARE( py_has_key )
	PY_METHOD_DECLARE( py_keys )
	PY_METHOD_DECLARE( py_values )
	PY_METHOD_DECLARE( py_items )

	static Py_ssize_t pyGetLength( PyObject* self );
	static PyObject* pyGetFieldByKey( PyObject* self, PyObject* key );
	static int pySetFieldByKey( PyObject* self, PyObject* key, PyObject* value );

	PyObject * pyRepr();

	PY_METHOD_DECLARE( py_getFieldNameForIndex )

	size_t getNumFields() const	{ return fieldValues_.size(); }

	PyObject* getFieldByKey( PyObject * key );
	PyObject* getFieldByKey( const char * keyString );

	int setFieldByKey( PyObject * key, PyObject * value );
	int setFieldByKey( const char * key, PyObject * value );

	static int pyCompare( PyObject * a, PyObject * b );

private:
	static int pyCompareKeys( PyFixedDictDataInstance * pFixedDictA, 
		PyFixedDictDataInstance * pFixedDictB );

	static bool pyCompareValuesWithEqualKeys( 
		PyFixedDictDataInstance * pFixedDictA, 
		PyFixedDictDataInstance * pFixedDictB,
		int & compareResult );

	bool isSmart() const	{ return true; }

	SmartPointer<FixedDictDataType>	pDataType_;

	typedef	BW::vector<ScriptObject> FieldValues;
	FieldValues					fieldValues_;
};

typedef ScriptObjectPtr< PyFixedDictDataInstance > PyFixedDictDataInstancePtr;

BW_END_NAMESPACE

#endif // PY_FIXED_DICT_DATA_INSTANCE_HPP
