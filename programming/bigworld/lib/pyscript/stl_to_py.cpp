#include "pch.hpp"
#include "stl_to_py.hpp"

#include "script.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: PySTLSequence
// -----------------------------------------------------------------------------


PY_TYPEOBJECT_WITH_SEQUENCE( PySTLSequence, &PySTLSequence::s_seq_methods_ )

PY_BEGIN_METHODS( PySTLSequence )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PySTLSequence )
	/*~ attribute PySTLSequence.length
	 *	@components{ all }
	 *
	 *	This attribute returns the number of elements in the sequence
	 *	
	 *	@type	Integer
	 */
	PY_ATTRIBUTE( length )
PY_END_ATTRIBUTES()


PySequenceMethods PySTLSequence::s_seq_methods_ = {
	_pySeq_length,			// inquiry sq_length;				len(x)
	_pySeq_concat,			// binaryfunc sq_concat;			x + y
	_pySeq_repeat,			// intargfunc sq_repeat;			x * n
	_pySeq_item,			// intargfunc sq_item;				x[i]
	_pySeq_slice,			// intintargfunc sq_slice;			x[i:j]
	_pySeq_ass_item,		// intobjargproc sq_ass_item;		x[i] = v
	_pySeq_ass_slice,		// intintobjargproc sq_ass_slice;	x[i:j] = v
	_pySeq_contains,		// objobjproc sq_contains;			v in x
	_pySeq_inplace_concat,	// binaryfunc sq_inplace_concat;	x += y
	_pySeq_inplace_repeat	// intargfunc sq_inplace_repeat;	x *= n
};



/**
 *	Constructor.
 */
PySTLSequence::PySTLSequence( PySTLSequenceHolderBase & holder,
		PyObject * pOwner, PyTypeObject * pType ) :
	PyObjectPlus( pType ),
	holder_( holder ),
	pOwner_( pOwner )
{
	Py_XINCREF( pOwner_ );
}


/**
 *	Destructor.
 */
PySTLSequence::~PySTLSequence()
{
	Py_XDECREF( pOwner_ );
}


/**
 *	Get length
 */
Py_ssize_t PySTLSequence::pySeq_length()
{
	return holder_.size();
}


/**
 *	Concatenate ourselves with another sequence, returning a new sequence
 */
PyObject * PySTLSequence::pySeq_concat( PyObject * pOther )
{
	if (!PySequence_Check( pOther ))
	{
		PyErr_SetString( PyExc_TypeError, "PySTLSequence: "
			"Argument to + must be a sequence" );
		return NULL;
	}

	int szA = holder_.size();
	Py_ssize_t szB = PySequence_Size( pOther );
	PyObject * pList = PyList_New( szA + szB );

	for (int i = 0; i < szA; i++)
		PyList_SET_ITEM( pList, i, holder_[i] );
	for (int i = 0; i < szB; i++)
		PyList_SET_ITEM( pList, szA + i, PySequence_GetItem( pOther, i ) );

	return pList;
}


/**
 *	Repeat ourselves a number of times, returning a new sequence
 */
PyObject * PySTLSequence::pySeq_repeat( Py_ssize_t n )
{
	if (n <= 0)
	{
		return PyList_New( 0 );
	}

	int sz = holder_.size();

	PyObject * pList = PyList_New( sz * n );
	if (pList == NULL)
	{
		return NULL;	// e.g. out of memory!
	}

	// add the first repetition
	for (int j = 0; j < sz; j++)
	{
		PyList_SET_ITEM( pList, j, holder_[j] );
	}

	// add the others (from the first lot)
	for (int i = 1; i < n; i++)
	{
		for (int j = 0; j < sz; j++)
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
PyObject * PySTLSequence::pySeq_item( Py_ssize_t index )
{
	int sz = holder_.size();

	//if (index < 0) index += sz;
	if (index < 0 || index >= sz)
	{
		PyErr_SetString( PyExc_IndexError,
			"PySTLSequence index out of range" );
		return NULL;
	}

	MF_ASSERT( index <= INT_MAX );
	return holder_[ ( int ) index ];
}


/**
 *	Get the given slice
 */
PyObject * PySTLSequence::pySeq_slice( Py_ssize_t indexA, Py_ssize_t indexB )
{
	int sz = holder_.size();

	// put indices in range (slices don't generate index errors)
	//if (indexA < 0) indexA += sz;
	//if (indexB < 0) indexB += sz;
	if (indexA < 0) indexA = 0;
	if (indexA > sz ) indexA = sz;
	if (indexB < 0) indexB = 0;
	if (indexB > sz ) indexB = sz;

	// quick test for empty list
	size_t nsz = indexB - indexA;
	if (sz == 0 || nsz <= 0) return PyList_New(0);

	// build the list
	PyObject * pList = PyList_New( nsz );
	MF_ASSERT( sz <= INT_MAX );
	for (size_t i = 0; i < nsz; i++)
	{
		PyList_SET_ITEM( pList, i, holder_[( int ) ( indexA + i ) ] );
	}

	// and return it
	return pList;
}


/**
 *	Swap the item currently at the given index with the given one.
 */
int PySTLSequence::pySeq_ass_item( Py_ssize_t index, PyObject * pItem )
{
	// check that the holder is writable
	if (!holder_.writable())
	{
		PyErr_Format( PyExc_TypeError,
			"Cannot assign an item in a read-only PySTLSequence" );
		return -1;
	}

	// put the index in range
	int sz = holder_.size();
	//if (index < 0) index += sz;
	if (index < 0 || index >= sz)
	{
		PyErr_SetString( PyExc_IndexError,
			"PySTLSequence assignment index out of range" );
		return -1;
	}

	// replace the item
	MF_ASSERT( index + 1 <= INT_MAX );
	holder_.erase( ( int ) index, ( int ) ( index + 1 ) );
	holder_.insertRange( ( int ) index, 1 );
	int err = holder_.insert( pItem );

	if (err != 0)	holder_.cancel();
	else			holder_.commit();

	return err;
}


/**
 *	Swap the slice defined by the given range with the given one.
 */
int PySTLSequence::pySeq_ass_slice( Py_ssize_t indexA, Py_ssize_t indexB,
		PyObject * pOther )
{
	// check that the holder is writable
	if (!holder_.writable())
	{
		PyErr_Format( PyExc_TypeError,
			"Cannot assign a slice in a read-only PySTLSequence" );
		return -1;
	}

	// make sure we're setting it to a sequence
	if (!PySequence_Check( pOther ))
	{
		PyErr_Format( PyExc_TypeError,
			"PySTLSequence slices can only be assigned to a sequence" );
		return -1;
	}

	int sz = holder_.size();
	Py_ssize_t osz = PySequence_Size( pOther );

	// put indices in range (slices don't generate index errors)
	//if (indexA < 0) indexA += sz;
	//if (indexB < 0) indexB += sz;
	if (indexA > sz ) indexA = sz;
	if (indexA < 0) indexA = 0;
	if (indexB > sz ) indexB = sz;
	if (indexB < 0) indexB = 0;

	// only erase if there's something to erase
	if (indexA < indexB) holder_.erase( ( int ) indexA, ( int ) indexB );

	// add our candidates
	//  (this should work even if pOther uses the same holder as we do)
	int err = 0;
	MF_ASSERT( osz <= INT_MAX );
	holder_.insertRange( ( int ) indexA, ( int ) osz );
	for (int i = 0; i < osz && !err; i++)
	{
		PyObject * pTemp = PySequence_GetItem( pOther, i );
		err |= holder_.insert( pTemp );
		Py_XDECREF( pTemp );
	}

	if (err != 0)	holder_.cancel();
	else			holder_.commit();

	return err;
}


/**
 *	See if the given object is in the sequence
 */
int PySTLSequence::pySeq_contains( PyObject * pObject )
{
	// call straight to the holder, because it can
	//  turn the object into its internal type and
	//  do the check much more quickly than we can
	return holder_.contains( pObject );
}


/**
 *	Concatenate the given sequence to ourselves
 */
PyObject * PySTLSequence::pySeq_inplace_concat( PyObject * pOther )
{
	// check that the holder is writable
	if (!holder_.writable())
	{
		PyErr_Format( PyExc_TypeError,
			"Cannot do += on a read-only PySTLSequence" );
		return NULL;
		//return -1;
	}

	// check that we're dealing with a sequence
	if (!PySequence_Check( pOther ))
	{
		PyErr_SetString( PyExc_TypeError, "PySTLSequence: "
			"Argument to += must be a sequence" );
		return NULL;
		//return -1;
	}

	// get the sizes
	int szA = holder_.size();
	Py_ssize_t szB = PySequence_Size( pOther );

	// and concatenate away
	//  (this should work even if pOther uses the same holder as we do)
	int err = 0;
	MF_ASSERT( szA <= INT_MAX );
	MF_ASSERT( szB <= INT_MAX );
	holder_.insertRange( ( int ) szA, ( int ) szB );
	for (int i = 0; i < szB && !err; i++)
	{
		PyObject * pTemp = PySequence_GetItem( pOther, i );
		err |= holder_.insert( pTemp );
		Py_XDECREF( pTemp );
	}

	if (err != 0)	holder_.cancel();
	else			holder_.commit();

	if (err)
		return NULL;
	else
	{
		Py_INCREF( this );
		return this;
	}
	//return err;
}



/**
 *	Repeat ourselves a number of times
 */
PyObject * PySTLSequence::pySeq_inplace_repeat( Py_ssize_t n )
{
	// check that the holder is writable
	if (!holder_.writable())
	{
		PyErr_Format( PyExc_TypeError,
			"Cannot do *= on a read-only PySTLSequence" );
		return NULL;
		//return -1;
	}

	int sz = holder_.size();

	int err = 0;
	if (n <= 0)
	{
		holder_.erase( 0, sz );
		holder_.commit();
	}
	else
	{
		Py_ssize_t endRange = ( n- 1 ) * sz;
		MF_ASSERT( endRange <= INT_MAX );
		holder_.insertRange( sz, ( int ) endRange );
		for (int i = 1; i < n && !err; i++)
		{
			for (int j = 0; j < sz && !err; j++)
			{
				// bit of a waste going to python and back I know...
				PyObject * pTemp = holder_[j];
				err |= holder_.insert( pTemp );
				Py_DECREF( pTemp );
			}
		}

		if (err != 0)	holder_.cancel();
		else			holder_.commit();
	}

	if (err)
		return NULL;
	else
	{
		Py_INCREF( this );
		return this;
	}
	//return err;
}


/**
 *	This method returns the string representation of this object
 */
PyObject * PySTLSequence::pyRepr()
{
	PyObject *s, *comma;
	int i;

	// Copied wholesale from listobject.c's list_repr

	i = Py_ReprEnter( this );		// no recursion thanks
	if (i != 0) {
		if (i > 0)
			return PyString_FromString("[...]");
		return NULL;
	}
	s = PyString_FromString("[");
	comma = PyString_FromString(", ");
	for (i = 0; i < holder_.size() && s != NULL; i++) {
		if (i > 0)
			PyString_Concat(&s, comma);
		PyObject * pTemp = holder_[i];
		PyString_ConcatAndDel(&s, PyObject_Repr(pTemp));
		Py_DECREF( pTemp );
	}
	Py_XDECREF(comma);
	PyString_ConcatAndDel(&s, PyString_FromString("]"));
	Py_ReprLeave( this );
	return s;
}


// -----------------------------------------------------------------------------
// Section: Script namespace functions
// -----------------------------------------------------------------------------


// Static script method to overwrite this object
int Script::setData( PyObject * pObject, PySTLSequenceHolderBase & holder,
		const char * varName )
{
	// check that the holder is writable
	if (!holder.writable())
	{
		PyErr_Format( PyExc_TypeError,
			"%s is a read-only sequence", varName );
		return -1;
	}

	// make sure we're setting it to a sequence
	if (!PySequence_Check( pObject ))
	{
		PyErr_Format( PyExc_TypeError,
			"%s must be set to a sequence", varName );
		return -1;
	}

	// make sure we're not setting it to ourselves
	//  (so we don't erase our elements then try to find them again :)
	if (PySTLSequence::Check( pObject ))
	{
		PySTLSequence * pSS = static_cast<PySTLSequence*>( pObject );
		if (&pSS->holder() == &holder)
			return 0;
	}

	// get the sequence size
	Py_ssize_t sz = PySequence_Size( pObject );

	// remove everything from us
	holder.erase( 0, holder.size() );

	// add new elts into it
	int err = 0;
	MF_ASSERT( sz <= INT_MAX );
	holder.insertRange( 0, ( int ) sz );
	for (int i = 0; i < sz && !err; i++)
	{
		PyObject * pTemp = PySequence_GetItem( pObject, i );
		err |= holder.insert( pTemp );
		Py_XDECREF( pTemp );
		// decref in-loop OK 'coz holder keeps a reference
		//  (so object can't delete itself which may delete the sequence :)
	}

	// commit or cancel depending on err
	if (err != 0)	holder.cancel();
	else			holder.commit();

	return err;
}

// Static script method to get a reference to this object
PyObject * Script::getData( const PySTLSequenceHolderBase & holder )
{
	return new PySTLSequence(
		const_cast<PySTLSequenceHolderBase&>(holder), holder.pOwner() );
}






// -----------------------------------------------------------------------------
// Section: PySTLMapping
// -----------------------------------------------------------------------------

/*
typedef struct {
	inquiry mp_length;
	binaryfunc mp_subscript;
	objobjargproc mp_ass_subscript;
} PyMappingMethods;
*/

BW_END_NAMESPACE


// stl_to_py.cpp
