#ifndef CLASS_DATA_INSTANCE_HPP
#define CLASS_DATA_INSTANCE_HPP

#include "intermediate_property_owner.hpp"


BW_BEGIN_NAMESPACE

class ClassDataType;


/**
 *	This class is the Python object for instances of a CLASS.
 *	It monitors changes to its properties, and tells its owner when they
 *	change so that its owner can propagate the changes around.
 */
class PyClassDataInstance : public IntermediatePropertyOwner
{
	Py_Header( PyClassDataInstance, PropertyOwner );

public:
	PyClassDataInstance( const ClassDataType * pDataType,
		PyTypeObject * pPyType = &s_type_ );
	~PyClassDataInstance();

	void setToDefault();
	void setToCopy( PyClassDataInstance & other );

	typedef PyObject * (*Interrogator)( PyObject * pObj, const char * prop );
	bool setToForeign( PyObject * pForeign, Interrogator interrogator );

#ifndef _MSC_VER
	#define FN(X) X
#else
#define FN(X) X##_FN
	static Interrogator FOREIGN_MAPPING, FOREIGN_ATTRS;
#endif
	static PyObject * FN(FOREIGN_MAPPING)( PyObject * pObj, const char * prop )
		{ return PyMapping_GetItemString( pObj, const_cast<char*>(prop) ); }
	static PyObject * FN(FOREIGN_ATTRS)( PyObject * pObj, const char * prop )
		{ return PyObject_GetAttrString( pObj, const_cast<char*>(prop) ); }


	PY_PICKLING_METHOD_DECLARE( Class )
	static PyObject * PickleResolve( ScriptObject list );
	PY_AUTO_UNPICKLING_FACTORY_DECLARE( ARG( ScriptObject, END ), Class )


	void setFieldValue( int index, ScriptObject val );
	ScriptObject getFieldValue( int index );


	const ClassDataType * dataType() const			{ return pDataType_.get(); }
	void setDataType( ClassDataType * pDataType )	{ pDataType_ = pDataType; }


	virtual int getNumOwnedProperties() const;
	virtual PropertyOwnerBase * getChildPropertyOwner( int ref ) const;
	virtual ScriptObject setOwnedProperty( int ref, BinaryIStream & data );

	ScriptObject getPyIndex( int index ) const;

	ScriptObject pyGetAttribute( const ScriptString & attrObj );
	bool pySetAttribute( const ScriptString & attrObj,
		const ScriptObject & value );

	void pyAdditionalMembers( const ScriptList & pList ) const;

private:
	bool isSmart() const	{ return true; }

	ConstSmartPointer<ClassDataType>	pDataType_;

	BW::vector<ScriptObject>			fieldValues_;
};

typedef ScriptObjectPtr<PyClassDataInstance> PyClassDataInstancePtr;

BW_END_NAMESPACE

#endif // CLASS_DATA_INSTANCE_HPP
