#include "pch.hpp"
#include "py_factory_method_link.hpp"

#include "script.hpp"


BW_BEGIN_NAMESPACE

/**
 *	Constructor
 */
PyFactoryMethodLink::PyFactoryMethodLink( const char * moduleName,
		const char * methodName, PyTypeObject * pType ) :
	Script::InitTimeJob( 0 ),
	moduleName_( moduleName ),
	methodName_( methodName ),
	pType_( pType ),
	origTypeName_( NULL )
{
	MF_ASSERT( moduleName != NULL );
	MF_ASSERT( methodName != NULL );
	MF_ASSERT( pType != NULL );
}


/**
 *	Destructor
 */
PyFactoryMethodLink::~PyFactoryMethodLink()
{
}


/**
 *	This method adds this factory method to the module
 */
void PyFactoryMethodLink::init()
{
	// get the module
	PyObject * pModule =
		PyImport_AddModule( const_cast<char *>(moduleName_) );
	MF_ASSERT( pModule != NULL );

	// make sure the type is ready or can be made so
	int isReady = PyType_Ready( pType_ );
	MF_ASSERT( isReady == 0 );

	//
	// Setup the fully qualified name. This is required for prettier error
	// messages, but is especially important for pickling.
	// Reference
	//	http://docs.python.org/2/extending/newtypes.html
	//	http://docs.python.org/2/c-api/typeobj.html (tp_name section)

	// Would be nice if this wasn't dynamcially allocated, but at the moment
	// the way the PyObjectPlus macro's work makes it a bit awkward
	// (PY_FACTORY_NAMED would be easy by doing:
	//
	// #MODULE_NAME"."METHOD_NAME
	//
	// but PY_FACTORY passes in s_type_.tp_name as method name to
	// this class.
	//
	size_t modNameLen = strlen( moduleName_ );
	size_t typNameLen = strlen( methodName_ );

	// +1 for . in-between module and method names
	// +1 for \0 at the end of the string
	size_t qualifiedLen = (modNameLen + 1 + typNameLen) + 1;

	char * pQualifiedName = new char[ qualifiedLen ];
	bw_snprintf( pQualifiedName, qualifiedLen,
		"%s.%s", moduleName_, methodName_ );

	origTypeName_ = pType_->tp_name;
	pType_->tp_name = pQualifiedName;

	// add the type to the module
	PyModule_AddObject( pModule, methodName_, (PyObject *)pType_ );
}


/**
 *	This method cleans up the type on shutdown
 */
void PyFactoryMethodLink::fini()
{
	// Restore type name to original, non-dynamically allocated string.
	if (origTypeName_ != NULL)
	{
		delete[] pType_->tp_name;
		pType_->tp_name = origTypeName_;
		origTypeName_ = NULL;
	}	
}

BW_END_NAMESPACE

// py_factory_method_link.cpp
