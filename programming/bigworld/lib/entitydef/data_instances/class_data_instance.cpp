#include "pch.hpp"

#include "class_data_instance.hpp"

#include "entitydef/script_data_sink.hpp"

#include "entitydef/data_types/class_data_type.hpp"


BW_BEGIN_NAMESPACE

class ClassDataType;

namespace
{

const char * EMPTY_CLASS_NAME = "PickleableClass";	// was "EmptyClass"

// This class handles the registration on deregistration of an empty
// class object that we use for pickling/unpickling of PyClassDataInstance.
class PyEmptyClassObject : public Script::InitTimeJob,
						   public Script::FiniTimeJob
{
	ScriptObject pEmptyClassObject_;

public:
	PyEmptyClassObject() :
		Script::InitTimeJob( 1 ), Script::FiniTimeJob( 1 )
	{}

	virtual void init()
	{
		ScriptObject pDict( PyDict_New(), ScriptObject::STEAL_REFERENCE );
		MF_ASSERT_DEV( pDict );
		ScriptObject pModuleName( PyString_FromString( "_BWp" ),
								 ScriptObject::STEAL_REFERENCE );
		MF_VERIFY( PyDict_SetItemString( pDict.get(), "__module__",
										pModuleName.get() ) != -1 );
		ScriptObject pClassName( PyString_FromString( EMPTY_CLASS_NAME ),
								ScriptObject::STEAL_REFERENCE );
		MF_ASSERT_DEV( pClassName );
		pEmptyClassObject_ = ScriptObject(
			PyClass_New( NULL, pDict.get(), pClassName.get() ),
			ScriptObject::STEAL_REFERENCE );
		MF_VERIFY( PyObject_SetAttrString( PyImport_AddModule( "_BWp" ),
						const_cast<char*>(EMPTY_CLASS_NAME),
						pEmptyClassObject_.get() ) != -1 );
	}

	virtual void fini()
	{
		pEmptyClassObject_ = NULL;
	}

	PyObject* get()
	{
		MF_ASSERT_DEV( pEmptyClassObject_ );
		return pEmptyClassObject_.get();
	}
};

} // anonymous namespace


#ifdef _MSC_VER
#undef FN
PyClassDataInstance::Interrogator PyClassDataInstance::FOREIGN_MAPPING =
		PyClassDataInstance::FOREIGN_MAPPING_FN;
PyClassDataInstance::Interrogator PyClassDataInstance::FOREIGN_ATTRS =
		PyClassDataInstance::FOREIGN_ATTRS_FN;
#endif

// -----------------------------------------------------------------------------
// Section: PyClassDataInstance implementation
// -----------------------------------------------------------------------------

// These are out-of-line because we need to use ClassDataType which is still
// undeclared during our declaration.

namespace
{
PyEmptyClassObject s_emptyClassObject_;
}

PY_TYPEOBJECT( PyClassDataInstance )

PY_BEGIN_METHODS( PyClassDataInstance )
	PY_PICKLING_METHOD()
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PyClassDataInstance )
PY_END_ATTRIBUTES()

PY_UNPICKLING_FACTORY( PyClassDataInstance, Class )


/**
 *	Constructor for PyClassDataInstance.
 */
PyClassDataInstance::PyClassDataInstance( const ClassDataType * pDataType,
		PyTypeObject * pPyType ) :
	IntermediatePropertyOwner( pPyType ),
	pDataType_( pDataType ),
	fieldValues_( pDataType->fields().size() )
{
}

/**
 *	Destructor for PyClassDataInstance
 */
PyClassDataInstance::~PyClassDataInstance()
{
	for (uint i = 0; i < fieldValues_.size(); ++i)
		if (fieldValues_[i])
			pDataType_->fields()[i].type_->detach( fieldValues_[i] );
}

/**
 *	Set to the default values for our data types (from blank)
 *	(Note: no defaults in our type specs... that could be good too)
 */
void PyClassDataInstance::setToDefault()
{
	uint fieldCount = static_cast<uint>(fieldValues_.size());
	for (uint i = 0; i < fieldCount; ++i)
	{
		DataType & dt = *pDataType_->fields()[i].type_;
		ScriptDataSink sink;
		MF_VERIFY( dt.getDefaultValue( sink ) );
		ScriptObject temp = sink.finalise();
		MF_ASSERT_DEV( temp );
		fieldValues_[i] = dt.attach( temp, this, i );
		MF_ASSERT_DEV( fieldValues_[i] );
	}
}

/**
 *	Make a copy of the given object (from blank).
 */
void PyClassDataInstance::setToCopy( PyClassDataInstance & other )
{
	IF_NOT_MF_ASSERT_DEV( pDataType_ == other.pDataType_ )
	{
		return;
	}

	uint fieldCount = static_cast<uint>(fieldValues_.size());
	for (uint i = 0; i < fieldCount; ++i)
	{
		fieldValues_[i] = pDataType_->fields()[i].type_->attach(
			other.fieldValues_[i], this, i );
		MF_ASSERT_DEV( fieldValues_[i] );
	}
}

/**
 *	Attempt to set to the given foreign object (from blank).
 *	Returns false and sets a Python error on failure.
 */
bool PyClassDataInstance::setToForeign( PyObject * pForeign,
	Interrogator interrogator )
{
	uint fieldCount = static_cast<uint>(fieldValues_.size());
	for (uint i = 0; i < fieldCount; ++i)
	{
		PyObject * gotcha = interrogator( pForeign,
			pDataType_->fields()[i].name_.c_str() );
		if (!gotcha) return false;			// leave python error there

		fieldValues_[i] = pDataType_->fields()[i].type_->attach( ScriptObject( 
			gotcha, ScriptObject::FROM_BORROWED_REFERENCE ), this, i );
		Py_DECREF( gotcha );

		if (!fieldValues_[i])
		{
			PyErr_Format( PyExc_TypeError, "Class::setToForeign: "
				"Foreign object elt has wrong data type for prop '%s'",
				pDataType_->fields()[i].name_.c_str() );
			return false;
		}
	}
	return true;
}

/**
 *	Set the value of an uninitialised field.
 */
void PyClassDataInstance::setFieldValue( int index, ScriptObject val )
{
	fieldValues_[index] =
		pDataType_->fields()[index].type_->attach( val, this, index );
}

/**
 *	Get the value of a field.
 */
ScriptObject PyClassDataInstance::getFieldValue( int index )
{
	return fieldValues_[index];
}


/**
 *	Someone wants to know how we have divided our property
 */
int PyClassDataInstance::getNumOwnedProperties() const
{
	return static_cast<int>(fieldValues_.size());
}

/**
 *	Someone wants to know if this property is an owner in its own right.
 */
PropertyOwnerBase * PyClassDataInstance::getChildPropertyOwner( int ref ) const
{
	if (uint(ref) >= fieldValues_.size())
	{
		ERROR_MSG( "PyClassDataInstance::getChildPropertyOwner: "
					"Bad index %d. size = %zd\n",
				ref, fieldValues_.size() );
		return NULL;
	}

	return pDataType_->fields()[ref].type_->asOwner( fieldValues_[ref] );
}

/**
 *	Someone wants us to change the value of this property.
 */
ScriptObject PyClassDataInstance::setOwnedProperty( int ref,
	BinaryIStream & data )
{
	DataType & dataType = *pDataType_->fields()[ref].type_;
	ScriptDataSink sink;
	if (!dataType.createFromStream( data, sink, false ))
	{
		return ScriptObject();
	}

	ScriptObject pNewValue = sink.finalise();
	ScriptObject & valRef = fieldValues_[ref];
	ScriptObject pOldValue = valRef;
	if (valRef != pNewValue)
	{
		// detach old value and attach new one
		dataType.detach( valRef );
		valRef = dataType.attach( pNewValue, this, ref );
	}

	return pOldValue;
}


/**
 *	This method returns a Python object representing the child at the given
 *	index.
 */
ScriptObject PyClassDataInstance::getPyIndex( int index ) const
{
	return ScriptObject(
			PyString_FromString( pDataType_->fields()[ index ].name_.c_str() ),
			ScriptObject::STEAL_REFERENCE );
}


/**
 *	Python get attribute method
 */
ScriptObject PyClassDataInstance::pyGetAttribute( const ScriptString & attrObj )
{
	int index = pDataType_->fieldIndexFromName( attrObj.c_str() );
	if (index >= 0)
	{
		return ScriptObject::createFrom( fieldValues_[index] );
	}

	return PropertyOwner::pyGetAttribute( attrObj );
}


/**
 *	Python set attribute method
 */
bool PyClassDataInstance::pySetAttribute( const ScriptString & attrObj,
	const ScriptObject & value )
{
	const char * attr = attrObj.c_str();

	int index = pDataType_->fieldIndexFromName( attr );
	if (index >= 0)
	{
		DataType & dataType = *pDataType_->fields()[index].type_;

		if (!this->changeOwnedProperty( fieldValues_[ index ], 
				value, dataType, index ))
		{
			if (!PyErr_Occurred())
			{
				PyErr_Format( PyExc_TypeError,
					"Class property %s must be set to type %s",
					attr, dataType.typeName().c_str() );
			}
			return false;
		}

		return true;
	}

	return PropertyOwner::pySetAttribute( attrObj, value );
}

/**
 *	Return additional members for dir.
 */
void PyClassDataInstance::pyAdditionalMembers( const ScriptList & pList ) const
{
	for (ClassDataType::Fields_iterator it = pDataType_->fields().begin();
		it != pDataType_->fields().end();
		++it)
	{
		pList.append( ScriptString::create( it->name_ ) );
	}
}

/**
 *	Python pickling helper. Returns the second element of the tuple expected of
 * 	Python's __reduce__ method. Declared in PY_PICKLING_METHOD_DECLARE.
 */
PyObject * PyClassDataInstance::pyPickleReduce()
{
	// Make an equivalent pickle-able Python class instance.
	const ClassDataType::Fields& fields = pDataType_->getFields();
	PyObject* pClassInstance =
		PyInstance_New( s_emptyClassObject_.get(), NULL, NULL );
	for ( ClassDataType::Fields::size_type i = 0; i < fields.size(); ++i )
	{
		PyObject_SetAttrString( pClassInstance,
								const_cast<char*>(fields[i].name_.c_str()),
								fieldValues_[i].get() );
	}

	// Make a tuple (method arguments for PickleResolve())
	PyObject * pArgs = PyTuple_New( 1 );
	PyTuple_SET_ITEM( pArgs, 0, pClassInstance );

	return pArgs;
}

/**
 *	Python unpickling helper. Creates a PyClassDataInstance from the
 *	information returned by PyClassDataInstance::pyPickleReduce().
 * 	Required by PY_AUTO_UNPICKLING_FACTORY_DECLARE.
 */
PyObject * PyClassDataInstance::PickleResolve( ScriptObject pClassInstance )
{
	// NOTE: Currently doesn't return a PyClassDataInstance
	// since it's too hard to find the right ClassDataType for pDataType_.
	// We'd also need pOwner_ and ownerRef_ and re-attach
	// PyClassDataInstance to the right entity here.

	// Simply return the equivalent class instance for now.
	Py_XINCREF( pClassInstance.get() );
	return pClassInstance.get();
}

BW_END_NAMESPACE

// class_data_instance.cpp
