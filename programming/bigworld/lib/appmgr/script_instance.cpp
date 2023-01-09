
#include "pch.hpp"
#include "script_instance.hpp"

#ifndef CODE_INLINE
#include "script_instance.ipp"
#endif

DECLARE_DEBUG_COMPONENT2( "Script", 0 )

// -----------------------------------------------------------------------------
// Section: ScriptInstance
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
ScriptInstance::ScriptInstance( PyTypeObject * pType ) : PyObjectPlus( pType )
{
}


/**
 *	This method initialises this object from a data section.
 */
bool ScriptInstance::init( DataSectionPtr pSection,
	const char * moduleName, const char * defaultTypeName )
{
#error "This is not in working order: has not been updated for python 2.3"
#if 0
	// Load the player module
	PyObject * pModule =
		PyImport_ImportModule( const_cast<char *>( moduleName ) );

	if (pModule == NULL)
	{
		ERROR_MSG( "ScriptInstance::init: Could not load module %s.py\n",
			moduleName );
		PyErr_Print();
	}

	// Find the class
	PyObject * pClass = NULL;

	if (pModule != NULL)
	{
		BW::string typeName = pSection->readString( "type", defaultTypeName );

		pClass = PyObject_GetAttrString( pModule,
			const_cast<char *>( typeName.c_str() ) );
	}

	if (PyErr_Occurred())
	{
		PyErr_Clear();
	}

	this->in_dict = PyDict_New();

	this->setClass( pClass );
	Py_XDECREF( pClass );

	Script::call( PyObject_GetAttrString( this, "__init__" ),
		PyTuple_New(0), "ScriptInstance::init: ", true );

	return this->in_class != NULL;
#endif
	return true;
}

// script_instance.cpp
