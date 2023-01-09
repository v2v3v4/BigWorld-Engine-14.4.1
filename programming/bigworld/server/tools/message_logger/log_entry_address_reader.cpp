#include <Python.h>

#include "log_entry_address_reader.hpp"

#include "cstdmf/binary_stream.hpp"


BW_BEGIN_NAMESPACE

bool LogEntryAddressReader::isValid() const
{
	return !suffix_.empty();
}


bool LogEntryAddressReader::fromPyTuple( PyObject *tuple )
{
	// Enforce the item we receive is actually a tuple
	if (!PyTuple_Check( tuple ))
	{
		PyErr_Format( PyExc_ValueError,
			"LogEntryAddressReader::fromPyTuple: Provided object is not a "
			"valid tuple." );
		return false;
	}

	// Both of these are borrowed references
	PyObject *pySuffix = PyTuple_GetItem( tuple, 0 );
	PyObject *pyIndex = PyTuple_GetItem( tuple, 1 );

	const char *suffixStr = PyString_AsString( pySuffix );
	if (suffixStr == NULL)
	{
		// PyString_AsString will raise TypeError in this case.
		return false;
	}

	long tmpLong = PyInt_AsLong( pyIndex );
	if ((tmpLong == -1) && (PyErr_Occurred()))
	{
		// PyInt_AsLong will raise an error
		return false;
	}

	suffix_ = suffixStr;
	index_ = tmpLong;

	return true;
}

BW_END_NAMESPACE

// log_entry_address_reader.cpp
