#include "pch.hpp"

#include "array_data_instance.hpp"

#include "entitydef/data_types/array_data_type.hpp"
#include "entitydef/property_change.hpp"

#include "entitydef/script_data_sink.hpp"
#include "entitydef/script_data_source.hpp"

#include "cstdmf/binary_stream.hpp"
#include "math/mathdef.hpp"

BW_BEGIN_NAMESPACE

namespace // anonymous
{
PySequenceMethods s_seq_methods =
{
	PyArrayDataInstance::_pySeq_length,		// inquiry sq_length; len(x)
	PyArrayDataInstance::_pySeq_concat,		// binaryfunc sq_concat; x + y
	PyArrayDataInstance::_pySeq_repeat,		// intargfunc sq_repeat; x * n
	PyArrayDataInstance::_pySeq_item,		// intargfunc sq_item; x[i]
	PyArrayDataInstance::_pySeq_slice,		// intintargfunc sq_slice; x[i:j]
	PyArrayDataInstance::_pySeq_ass_item,	// intobjargproc sq_ass_item; 
											// x[i] = v
	PyArrayDataInstance::_pySeq_ass_slice,	// intintobjargproc sq_ass_slice;
											// x[i:j] = v
	PyArrayDataInstance::_pySeq_contains,	// objobjproc sq_contains;
											// v in x
	PyArrayDataInstance::_pySeq_inplace_concat,	
											// binaryfunc sq_inplace_concat;
											// x += y
	PyArrayDataInstance::_pySeq_inplace_repeat	
											// intargfunc sq_inplace_repeat;
											// x *= n
};
} // end namespace (anonymous)

PY_TYPEOBJECT_SPECIALISE_CMP( PyArrayDataInstance, 
	&PyArrayDataInstance::pyCompare );
PY_TYPEOBJECT_SPECIALISE_SEQ( PyArrayDataInstance, &s_seq_methods )
PY_TYPEOBJECT( PyArrayDataInstance )

PY_BEGIN_METHODS( PyArrayDataInstance )
	PY_METHOD( append )
	PY_METHOD( count )
	PY_METHOD( extend )
	PY_METHOD( index )
	PY_METHOD( insert )
	PY_METHOD( pop )
	PY_METHOD( remove )
	PY_METHOD( equals_seq )
	PY_PICKLING_METHOD()
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PyArrayDataInstance )
	PY_ATTRIBUTE( strictSize )
PY_END_ATTRIBUTES()

PY_UNPICKLING_FACTORY( PyArrayDataInstance, Array )


/**
 *	Constructor.
 */
PyArrayDataInstance::PyArrayDataInstance( const ArrayDataType * pDataType,
		size_t initialSize, PyTypeObject * pPyType ) :
	IntermediatePropertyOwner( pPyType ),
	pDataType_( pDataType ),
	values_( initialSize )
{
}

/**
 *	Destructor
 */
PyArrayDataInstance::~PyArrayDataInstance()
{
	DataType & elemType = pDataType_->getElemType();
	for (uint i = 0; i < values_.size(); ++i)
	{
		if (values_[i])
		{
			elemType.detach( values_[i] );
		}
	}
}

/**
 *	Set one of the initial items that initialSize allocated space for.
 */
void PyArrayDataInstance::setInitialItem( int index, ScriptObject pItem )
{
	values_[index] = pDataType_->getElemType().attach( 
		pItem, this, index );
}

/**
 *	Set all of the initial items to the given sequence which is guaranteed
 *	to be the correct size with the correct type elements.
 */
void PyArrayDataInstance::setInitialSequence( ScriptObject pSeq )
{
	Py_ssize_t sz = PySequence_Size( pSeq.get() );
	IF_NOT_MF_ASSERT_DEV( pDataType_->getSize() == 0 || pDataType_->getSize() == sz )
	{
		return;
	}
	IF_NOT_MF_ASSERT_DEV( uint(sz) == values_.size() )
	{
		return;
	}

	DataType & elemType = pDataType_->getElemType();
	MF_ASSERT( sz <= INT_MAX );
	for ( Py_ssize_t i = 0; i < sz; ++i)
	{
		ScriptObject pItem( PySequence_GetItem( pSeq.get(), i ),
				ScriptObject::STEAL_REFERENCE );
		values_[i] = elemType.attach( pItem, this, ( int ) i );
	}
}


/**
 *	Someone wants to know how we have divided our property
 */
int PyArrayDataInstance::getNumOwnedProperties() const
{
	return int( values_.size() );
}

/**
 *	Someone wants to know if this element is an owner in its own right.
 */
PropertyOwnerBase * PyArrayDataInstance::getChildPropertyOwner( int ref ) const
{
	if (uint(ref) >= values_.size())
	{
		ERROR_MSG( "PyArrayDataInstance::getChildPropertyOwner: "
					"Bad index %d. size = %zd\n",
				ref, values_.size() );
		return NULL;
	}

	return pDataType_->getElemType().asOwner( values_[ref] );
}

/**
 *	Someone wants us to change the value of this element.
 */
ScriptObject PyArrayDataInstance::setOwnedProperty( int ref,
	BinaryIStream & data )
{
	DataType & dataType = pDataType_->getElemType();
	ScriptDataSink sink;
	if (!dataType.createFromStream( data, sink, false ))
	{
		return ScriptObject();
	}

	ScriptObject pNewValue = sink.finalise();

	ScriptObject & valRef = values_[ref];
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
 *	Someone wants us to change a slice of values.
 */
ScriptObject PyArrayDataInstance::setOwnedSlice( int startIndex, int endIndex,
			BinaryIStream & data )
{
	if (endIndex < startIndex)
	{
		ERROR_MSG( "PyArrayDataInstance::setOwnedSlice: "
				"Invalid values. startIndex = %d. endIndex = %d. "
				"values = %" PRIzu " data.remainingLength = %d\n",
			startIndex, endIndex, values_.size(), data.remainingLength() );
	}

	MF_ASSERT_DEV( 0 <= startIndex );
	MF_ASSERT_DEV( startIndex <= endIndex );
	MF_ASSERT_DEV( endIndex <= int( values_.size() ) );

	DataType & dataType = pDataType_->getElemType();
	BW::vector< ScriptObject > newValues;

	while (data.remainingLength() > 0)
	{
		ScriptDataSink sink;

		if (!dataType.createFromStream( data, sink, false ))
		{
			ERROR_MSG( "PyArrayDataInstance::setOwnedSlice: "
					"Unable to create slice\n" );
			return ScriptObject();
		}

		newValues.push_back( sink.finalise() );
	}

	ScriptObject pOldValues;

	this->setSlice( startIndex, endIndex, newValues,
			/*notifyOwner:*/false, &pOldValues );

	return pOldValues;
}


/**
 *	This method returns a Python object representing the child at the given
 *	index.
 */
ScriptObject PyArrayDataInstance::getPyIndex( int index ) const
{
	return ScriptObject::createFrom( index );
}


/**
 *	Get the representation of this object
 */
PyObject * PyArrayDataInstance::pyRepr()
{
	if (values_.empty()) return Script::getData( "[]" );

	BW::string compo = "[";
	for (uint i = 0; i < values_.size(); ++i)
	{
		PyObject * pSubRepr = PyObject_Repr( values_[i].get() );

		if (pSubRepr == NULL) 
		{
			return NULL;
		}

		if (i != 0) 
		{
			compo += ", ";
		}

		compo += PyString_AsString( pSubRepr );
		Py_DECREF( pSubRepr );
	}
	return Script::getData( compo + "]" );
}


/**
 *	Helper accessor to return the strict size of this array, if one applies.
 */
int PyArrayDataInstance::strictSize() const
{
	return pDataType_->getSize();
}


/**
 *	Get length
 */
Py_ssize_t PyArrayDataInstance::pySeq_length()
{
	return values_.size();
}


/**
 *	Concatenate ourselves with another sequence, returning a new sequence
 */
PyObject * PyArrayDataInstance::pySeq_concat( PyObject * pOther )
{
	if (!PySequence_Check( pOther ))
	{
		PyErr_SetString( PyExc_TypeError,
			"Array argument to + must be a sequence" );
		return NULL;
	}

	size_t szA = values_.size();
	size_t szB = PySequence_Size( pOther );
	PyObject * pList = PyList_New( szA + szB );

	for (size_t i = 0; i < szA; i++)
	{
		PyList_SET_ITEM( pList, i, Script::getData( values_[i] ) );
	}

	for (size_t i = 0; i < szB; i++)
	{
		PyList_SET_ITEM( pList, szA + i, PySequence_GetItem( pOther, i ) );
	}

	return pList;
}


/**
 *	Repeat ourselves a number of times, returning a new sequence
 */
PyObject * PyArrayDataInstance::pySeq_repeat( Py_ssize_t n )
{
	if (n <= 0)
	{
		return PyList_New( 0 );
	}

	Py_ssize_t sz = values_.size();

	PyObject * pList = PyList_New( sz * n );
	if (pList == NULL)
	{
		return NULL;	// e.g. out of memory!
	}

	// add the first repetition
	for (Py_ssize_t j = 0; j < sz; j++)
	{
		PyList_SET_ITEM( pList, j, Script::getData( values_[j] ) );
	}

	// add the others (from the first lot)
	for (Py_ssize_t i = 1; i < n; i++)
	{
		for (Py_ssize_t j = 0; j < sz; j++)
		{
			PyObject * pTemp = PyList_GET_ITEM( pList, j );
			PyList_SET_ITEM( pList, i * sz + j, pTemp );
			Py_INCREF( pTemp );
		}
	}

	return pList;
}


/**
 *	Get the given item index
 */
PyObject * PyArrayDataInstance::pySeq_item( Py_ssize_t index )
{
	if ((0 <= index) && index < Py_ssize_t( values_.size() ))
	{
		return Script::getData( values_[ index ] );
	}

	PyErr_SetString( PyExc_IndexError, "Array index out of range" );
	return NULL;
}


/**
 *	Get the given slice
 */
PyObject * PyArrayDataInstance::pySeq_slice( Py_ssize_t startIndex,
	Py_ssize_t endIndex )
{
	if (startIndex < 0)
	{
		startIndex = 0;
	}

	if (endIndex > Py_ssize_t( values_.size() ))
	{
		endIndex = values_.size();
	}

	if (endIndex < startIndex)
	{
		endIndex = startIndex;
	}

	size_t length = endIndex - startIndex;

	PyObject * pResult = PyList_New( length );
	for ( Py_ssize_t i = startIndex; i < endIndex; ++i)
	{
		PyList_SET_ITEM( pResult, i-startIndex, Script::getData( values_[i] ) );
	}
	// always make a detached list for partial copies...
	// alternative would be to make a new type that just points to us

	return pResult;
}


/**
 *	Swap the item currently at the given index with the given one.
 */
int PyArrayDataInstance::pySeq_ass_item( Py_ssize_t index, PyObject * value )
{
	if ((index < 0) || index >= Py_ssize_t( values_.size() ))
	{
		PyErr_SetString( PyExc_IndexError,
				"Array assignment index out of range" );
		return -1;
	}

	if (value == NULL)
	{
		// Actually a delete
		return this->pySeq_ass_slice( index, index + 1, NULL );
	}

	DataType & dataType = pDataType_->getElemType();

	MF_ASSERT( index <= INT_MAX );
	if (!this->changeOwnedProperty(
			values_[ ( int ) index ], 
			ScriptObject( value, ScriptObject::FROM_BORROWED_REFERENCE ),
			dataType, ( int ) index ))
	{
		if (!PyErr_Occurred())
		{
			PyErr_Format( PyExc_TypeError,
				"Array elements must be set to type %s "
					"(setting index %" PRIzu ")",
				dataType.typeName().c_str(), index );
		}
		return -1;
	}

	return 0;
}


/**
 *	This method deletes the specified slice from this array.
 *
 *	@param startIndex  The index of the first element to delete.
 *	@param endIndex    One greater than the index of the last element to delete.
 */
void PyArrayDataInstance::deleteElements( Py_ssize_t startIndex,
		Py_ssize_t endIndex, ScriptObject * ppOldValues )
{
	DataType & elementType = pDataType_->getElemType();

	// TODO: Can look to not create this when tuple is not going to be used.
	// This includes when updating ghosts and when there is no setSlice_
	// method.
	if (ppOldValues)
	{
		Py_ssize_t size = std::max( endIndex - startIndex, Py_ssize_t( 0 ) );
		*ppOldValues = ScriptTuple::create( size );
	}


	// only erase if there's something to erase
	if (startIndex < endIndex)	// this behaviour is the same as PyList's
	{
		for (Py_ssize_t i = startIndex; i < endIndex; ++i)
		{
			PyObject * pOldValue = values_[i].get();

			if (ppOldValues)
			{
				Py_INCREF( pOldValue );
				PyTuple_SET_ITEM( (*ppOldValues).get(),
						i - startIndex, pOldValue );
			}

			elementType.detach( ScriptObject( pOldValue,
				ScriptObject::FROM_BORROWED_REFERENCE ) );
		}
		values_.erase( values_.begin() + startIndex,
				values_.begin() + endIndex );
	}

	// We need to detach and re-attach these values because the
	// ownerRefs will be different.
	MF_ASSERT( values_.size() < INT_MAX );
	for (Py_ssize_t i = startIndex; i < Py_ssize_t( values_.size() ); ++i)
	{
		ScriptObject value = values_[ ( int ) i ];
		elementType.detach( value );
		elementType.attach( value, this, /*ownerRef=*/ ( int ) i );
	}
}


/**
 *	This method populates a vector with the values extracted from a Python
 *	sequence.
 *
 *	@return true if successful, otherwise false.
 */
bool PyArrayDataInstance::getValuesFromSequence( PyObject * pSeq,
		BW::vector< ScriptObject > & values )
{
	if (pSeq == NULL)
	{
		// NULL is valid for an empty sequence.
		return true;
	}

	// make sure we're setting it to a sequence
	if (!PySequence_Check( pSeq ))
	{
		PyErr_Format( PyExc_TypeError,
			"PyArrayDataInstance slices can only be assigned to a sequence" );
		return false;
	}

	// make sure they are all the correct type
	DataType & dataType = pDataType_->getElemType();
	size_t seqSize = PySequence_Size( pSeq );

	for (size_t i = 0; i < seqSize; ++i)
	{
		PyObject * pVal = PySequence_GetItem( pSeq, i );

		values.push_back( ScriptObject( pVal, ScriptObject::STEAL_REFERENCE ) );

		if (!dataType.isSameType( ScriptObject( pVal,
			ScriptObject::FROM_BORROWED_REFERENCE ) ))
		{
			PyErr_Format( PyExc_TypeError,
				"Array elements must be set to type %s",
				dataType.typeName().c_str() );
			return false;
		}
	}

	return true;
}


/**
 *	Swap the slice defined by the given range with the given one.
 */
int PyArrayDataInstance::pySeq_ass_slice( Py_ssize_t startIndex,
	Py_ssize_t endIndex, PyObject * pOther )
{
	BW::vector< ScriptObject > newValues;

	if (!this->getValuesFromSequence( pOther, newValues ))
	{
		return -1;
	}

	if (!this->setSlice( startIndex, endIndex, newValues,
				/*notifyOwner:*/true ))
	{
		if (!PyErr_Occurred())
		{
			PyErr_SetString( PyExc_TypeError, "Failed to set slice" );
		}
		return -1;
	}

	return 0;
}


/**
 *	This method replaces a slice of values with a new sequence of values. This
 *	method is useful for appending, extending, removing and replacing.
 *
 *	To insert, for example, startIndex and endIndex are set to the value to
 *	insert before.
 *
 *	To delete, newValues is empty.
 *
 *	@param startIndex  The index of the first value to replace.
 *	@param endIndex    The index one after the last value to replace.
 *	@param newValues   The new values to insert.
 *	@param notifyOwner If true, the owner (entity) is told of this change so
 *				that others can be informed.
 *
 *	@return true on success, otherwise false.
 */
bool PyArrayDataInstance::setSlice( Py_ssize_t startIndex, Py_ssize_t endIndex,
		const BW::vector< ScriptObject > & newValues,
		bool notifyOwner, ScriptObject * ppOldValues )
{
	Py_ssize_t origSize = values_.size();
	MF_ASSERT( origSize <= INT_MAX );
	int numToInsert = int( newValues.size() );

	// put indices in range (slices don't generate index errors)
	startIndex = Math::clamp( Py_ssize_t( 0 ), startIndex, origSize );
	endIndex = Math::clamp( startIndex, endIndex, origSize );

	// make sure the sequnce will still be the right size
	if ((pDataType_->getSize() != 0) && (endIndex - startIndex != numToInsert))
	{
		WARNING_MSG( "PyArrayDataInstance::setSlice: "
			   "slice assignment would create array of wrong size" );

		return false;
	}

	// make sure they are all the correct type
	DataType & dataType = pDataType_->getElemType();

	PropertyOwnerBase * pTopLevelOwner = NULL;
	SlicePropertyChange change( startIndex, endIndex, 
		values_.size(), newValues, dataType );

	if (notifyOwner)
	{
		if (!this->getTopLevelOwner( change, pTopLevelOwner ))
		{
			WARNING_MSG( "PyArrayDataInstance::setSlice: "
					"Invalid top-level owner\n" );
			return false;
		}
	}

	this->deleteElements( startIndex, endIndex, ppOldValues );

	// make room for the new elements
	values_.insert( values_.begin() + startIndex, numToInsert, ScriptObject() );

	// and set them in there
	for (int i = 0; i < numToInsert; ++i)
	{
		values_[ ( int ) startIndex + i ] =
			dataType.attach( newValues[i], this, ( int ) startIndex + i );
	}

	// Update the ownerRefs for these elements that have now shifted because of
	// the new inserted values.
	for (size_t i = startIndex + numToInsert; i < values_.size(); ++i)
	{
		ScriptObject value = values_[ ( int ) i ];
		dataType.detach( value );
		dataType.attach( value, this, /*ownerRef=*/ static_cast<int>(i) );
	}

	if (pTopLevelOwner)
	{
		pTopLevelOwner->onOwnedPropertyChanged( change );
	}

	return true;
}


/**
 *	See if the given object is in the sequence
 */
int PyArrayDataInstance::pySeq_contains( PyObject * pObj )
{
	return this->findFrom( 0, pObj ) >= 0;
}


/**
 *	Concatenate the given sequence to ourselves
 */
PyObject * PyArrayDataInstance::pySeq_inplace_concat( PyObject * pOther )
{
	if (pySeq_ass_slice( values_.size(), values_.size(), pOther ) != 0)
	{
		return NULL;
	}

	Py_INCREF( this );
	return this;
}



/**
 *	Repeat ourselves a number of times
 */
PyObject * PyArrayDataInstance::pySeq_inplace_repeat( Py_ssize_t n )
{
	if (n == 1)
	{
		return Script::getData( this );
	}

	if (pDataType_->getSize() != 0)
	{
		PyErr_SetString( PyExc_TypeError, "PyArrayDataInstance "
			"repetition with *= would yield wrong size array" );
		return NULL;
	}

	BW::vector< ScriptObject > newValues;

	if (n <= 0)
	{
		return PyList_New( 0 );
	}

	Py_ssize_t size = values_.size();
	newValues.reserve( (n-1) * size );

	for (Py_ssize_t i = 1; i < n; ++i)
	{
		for (Py_ssize_t j = 0; j < size; ++j)
		{
			newValues.push_back( values_[j] );
		}
	}

	this->setSlice( size, size, newValues, /*notifyOwner:*/ true );

	return Script::getData( this );
}


/**
 *	Python method: append the given object
 */
bool PyArrayDataInstance::append( ScriptObject pObject )
{
	ScriptObject pTuple( PyTuple_New( 1 ), ScriptObject::STEAL_REFERENCE );
	PyTuple_SET_ITEM( pTuple.get(), 0, pObject.newRef() );

	return this->pySeq_ass_slice(
		values_.size(), values_.size(), pTuple.get() ) == 0;
}

/**
 *	Python method: count the occurrences of the given object
 */
int PyArrayDataInstance::count( ScriptObject pObject )
{
	int count = 0, cur;
	for (uint i = 0; (cur = this->findFrom( i, pObject.get() )) >= 0; i = cur+1)
	{
		++count;
	}
	return count;
}

/**
 *	Python method: append the given iterable
 */
bool PyArrayDataInstance::extend( ScriptObject pObject )
{
	return this->pySeq_ass_slice(
		values_.size(), values_.size(), pObject.get() ) == 0;
}

/**
 *	Python method: find index of given value
 */
PyObject * PyArrayDataInstance::py_index( PyObject * args )
{
	PyObject * pObject = NULL;
	if (!PyArg_ParseTuple( args, "O", &pObject ))
		return NULL;
	int index = this->findFrom( 0, pObject );
	if (index == -1)
	{
		PyErr_SetString( PyExc_ValueError,
			"PyArrayDataInstance.index: value not found" );
		return NULL;
	}
	return Script::getData( index );
}

/**
 *	Python method: insert value at given location
 */
bool PyArrayDataInstance::insert( int before, ScriptObject pObject )
{
	ScriptObject pTuple( PyTuple_New( 1 ), ScriptObject::STEAL_REFERENCE );
	PyTuple_SET_ITEM( pTuple.get(), 0, Script::getData( pObject ) );

	// insert shouldn't generate index errors, so ass_slice is perfect
	return this->pySeq_ass_slice( before, before, pTuple.get() ) == 0;
}

/**
 *	Python method: pop the last element
 */
PyObject * PyArrayDataInstance::pop( int index )
{
	if (values_.empty())
	{
		PyErr_SetString( PyExc_IndexError,
			"PyArrayDataInstance.pop: empty array" );
		return NULL;
	}

	if (index < 0) index += static_cast<int>(values_.size());
	if (uint(index) >= values_.size())
	{
		PyErr_SetString( PyExc_IndexError,
			"PyArrayDataInstance.pop: index out of range" );
		return NULL;
	}

	ScriptObject pItem = values_[index];

	ScriptObject pTuple( PyTuple_New( 0 ), ScriptObject::STEAL_REFERENCE );
	if (this->pySeq_ass_slice( index, index+1, pTuple.get() ) != 0)
	{
		return NULL;
	}

	Py_INCREF( pItem.get() );
	return pItem.get();
}

/**
 *	Python method: remove first occurrence of given value
 */
bool PyArrayDataInstance::remove( ScriptObject pObject )
{
	int index = this->findFrom( 0, pObject.get() );

	if (index == -1)
	{
		PyErr_SetString( PyExc_ValueError,
			"PyArrayDataInstance.remove: value not found" );
		return false;
	}

	return this->pySeq_ass_slice( index, index+1, NULL ) == 0;
}


/**
 *	Function to test equality with a sequence.
 *
 *	@param pOther The sequence to test against.
 *
 *	@return True if each element of this array is equal to each corresponding
 *			element of the other sequence, False otherwise. 
 */
bool PyArrayDataInstance::equals_seq( ScriptObject pOther )
{
	if (!PySequence_Check( pOther.get() ))
	{
		return false;
	}

	Py_ssize_t ourLength( values_.size() );

	// Quick return for length difference.
	if (ourLength != PySequence_Size( pOther.get() ))
	{
		return false;
	}

	for (Py_ssize_t i = 0; i < ourLength; ++i)
	{
		ScriptObject pOtherElement( PySequence_GetItem( pOther.get(), i ), 
			ScriptObject::STEAL_REFERENCE );

		if (0 != PyObject_Compare( pOtherElement.get(), values_[i].get() ))
		{
			return false;
		}
	}

	return true;
}


/**
 *	Helper method to find an object at or after a given index
 */
int PyArrayDataInstance::findFrom( uint beg, PyObject * needle )
{
	for (uint i = beg; i < values_.size(); ++i)
	{
		if (needle == values_[i].get()) 
		{
			return i;
		}
	}

	for (uint i = beg; i < values_.size(); ++i)
	{
		if (PyObject_Compare( needle, values_[i].get() ) == 0)
		{
			return i;
		}
	}

	return -1;
}

/**
 *	Reduce this array into something that can be serialised
 */
PyObject * PyArrayDataInstance::pyPickleReduce()
{
	PyObject * pList = PyList_New( values_.size() );
	for (uint i = 0; i < values_.size(); ++i)
		PyList_SET_ITEM( pList, i, Script::getData( values_[i] ) );

	PyObject * pArgs = PyTuple_New( 1 );
	PyTuple_SET_ITEM( pArgs, 0, pList );
	return pArgs;
}

/**
 *	Resolve this deserialised object into an array
 */
PyObject * PyArrayDataInstance::PickleResolve( ScriptObject list )
{
	// since there's no good way to get back to the ArrayDataType,
	// we just return the list that the reduce method produced

	Py_XINCREF( list.get() );
	return list.get();
}


/**
 *	Python comparison function for PyArrayDataInstance objects. 
 */
int PyArrayDataInstance::pyCompare( PyObject * a, PyObject * b )
{
	// Both a and b should be PyArrayDataInstances, Python enforces that
	// comparisons between disparate types should compare False always.

	MF_ASSERT(PyArrayDataInstance::Check( a ) && 
		PyArrayDataInstance::Check( b ));

	// Which means that downcasting to PyArrayDataInstance is safe.

	PyArrayDataInstance * pArrayA = static_cast< PyArrayDataInstance * >( a );
	PyArrayDataInstance * pArrayB = static_cast< PyArrayDataInstance * >( b );

	typedef BW::vector< ScriptObject > PyObjects;

	const PyObjects & aValues = pArrayA->values_;
	const PyObjects & bValues = pArrayB->values_;

	PyObjects::const_iterator iterA = aValues.begin();
	PyObjects::const_iterator iterB = bValues.begin();

	while (iterA != aValues.end() && iterB != bValues.end())
	{
		int compareResult = 0;

		if (-1 == PyObject_Cmp( iterA->get(), iterB->get(), &compareResult ))
		{
			// Error has been raised, we should return -1 in this case.
			return -1;
		}

		if (compareResult != 0)
		{
			return compareResult;
		}

		++iterA;
		++iterB;
	}

	return (iterA != aValues.end()) ? 1 : 
		(iterB != bValues.end()) ? -1 : 0;
}

BW_END_NAMESPACE

// array_data_instance.cpp
