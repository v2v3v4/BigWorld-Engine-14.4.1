#include "pch.hpp"
#include "py_to_stl.hpp"

#include "cstdmf/debug.hpp"
#include "cstdmf/dprintf.hpp"
DECLARE_DEBUG_COMPONENT2( "Script", 0 )

#include "script.hpp"


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "py_to_stl.ipp"
#endif

// ----------------------------------------------------------------------------
// Section: PySequence
// ----------------------------------------------------------------------------

/**
 *	Constructor
 */
PySequenceSTL::PySequenceSTL( PyObject * pObject ) :
	pSeq_( pObject )
{
	Py_XINCREF( pSeq_ );

	// Should possibly PySequence_Check it... (and for NULL...)
}


/**
 *	Copy constructor
 */
PySequenceSTL::PySequenceSTL( const PySequenceSTL & toCopy ) :
	pSeq_( toCopy.pSeq_ )
{
	Py_XINCREF( pSeq_ );
}


/**
 *	Assignment operator
 */
PySequenceSTL & PySequenceSTL::operator =( const PySequenceSTL & toCopy )
{
	if (pSeq_ != toCopy.pSeq_)
	{
		Py_XDECREF( pSeq_ );
		pSeq_ = toCopy.pSeq_;
		Py_XINCREF( pSeq_ );
	}
	return *this;
}


/**
 *	Destructor
 */
PySequenceSTL::~PySequenceSTL()
{
	Py_XDECREF( pSeq_ );
}



/**
 *	Static function to check if the input object is a Sequence
 *	and could thus be wrapped by this class.
 */
bool PySequenceSTL::Check( PyObject * pObject )
{
	return pObject != NULL && PySequence_Check( pObject ) != 0;
}





// ----------------------------------------------------------------------------
// Section: PyMapping
// ----------------------------------------------------------------------------

/**
 *	Constructor
 */
PyMappingSTL::PyMappingSTL( PyObject * pObject ) :
	pMap_( pObject )
{
	Py_XINCREF( pMap_ );

	// Should possibly PyMapping_Check it... (and for NULL...)
}


/**
 *	Copy constructor
 */
PyMappingSTL::PyMappingSTL( const PyMappingSTL & toCopy ) :
	pMap_( toCopy.pMap_ )
{
	Py_XINCREF( pMap_ );
}


/**
 *	Assignment operator
 */
PyMappingSTL & PyMappingSTL::operator =( const PyMappingSTL & toCopy )
{
	if (pMap_ != toCopy.pMap_)
	{
		Py_XDECREF( pMap_ );
		pMap_ = toCopy.pMap_;
		Py_XINCREF( pMap_ );
	}
	return *this;
}


/**
 *	Destructor
 */
PyMappingSTL::~PyMappingSTL()
{
	Py_XDECREF( pMap_ );
}



/**
 *	Static function to check if the input object is a Mapping
 *	and could thus be wrapped by this class.
 */
bool PyMappingSTL::Check( PyObject * pObject )
{
	return pObject != NULL && PyMapping_Check( pObject ) != 0;
}





// ----------------------------------------------------------------------------
// Section: Streaming operators
// ----------------------------------------------------------------------------


/**
 *	Output streaming operator for a generic PyObject *
 */
std::ostream& operator<<( std::ostream &o, const PyObjectPtrRef & rpObject )
{
	const PyObject * pObject = rpObject;

	//dprintf( "ostream::operator<<(PyObjectPtrRef) 0x%08X\n", pObject );

	if (pObject == NULL)
	{
		o << "(null)";
	}
	else
	{
		if (PyMethod_Check( pObject ))
		{
			o <<  "Type: " << pObject->ob_type->tp_name <<
				" rc: " << pObject->ob_refcnt;
		}
		else
		{
			PyObject * pString = PyObject_Str( const_cast<PyObject*>( pObject ) );
			if (pString == NULL)
			{
				PyObject * pError = PyErr_Occurred();
				if (pError == NULL)
				{
					o << "(!!!Error without Exception!!!)";
				}
				else
				{
					o << "(" << pError->ob_type->tp_name << ")";
					PyErr_Clear();
				}
			}
			else
			{
				o << PyString_AsString( pString );
				Py_DECREF( pString );
			}
		}
	}

	Py_XDECREF( const_cast<PyObject*>(pObject) );

	return o;
}


/**
 *	Input streaming operator for a PyObjectPtrRef. Very unfortunately,
 *	we have to pass this as a reference to our 'reference' object.
 */
std::istream& operator>>( std::istream &i, PyObjectPtrRef & rpObject )
{
	char line[512];
	i.getline( line, sizeof(line) );

	dprintf( "istream::operator>>(PyObjectPtrRef) %s\n", line );

	PyObject * pResult = Script::runString( line, false );
	if (pResult == NULL)
	{
		PyObject * pErr = PyErr_Occurred();
		ERROR_MSG( "operator >>: Script execution returned the error '%s'\n",
			PyString_AsString( PyObject_Str( pErr ) ) );

		PyErr_PrintEx(0);
	}
	else
	{
		// TODO: Try to coerce pResult into the same type as rpObject.
		//  (watching out for NULL of course)
		rpObject = pResult;

		// the thing we just set it into keeps a reference if it wants one
		Py_DECREF( pResult );
	}

	PyErr_Clear();

	return i;
}

BW_END_NAMESPACE

// py_to_stl.cpp
