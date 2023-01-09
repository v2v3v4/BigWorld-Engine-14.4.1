#include "pch.hpp"
#include "gui_functor_python.hpp"
#include "py_gui_item.hpp"
#include "pyscript/script.hpp"

BW_BEGIN_NAMESPACE
BEGIN_GUI_NAMESPACE

PyObject* PythonFunctor::call( BW::string function, ItemPtr item )
{
	BW_GUARD;

	BW::string module = defaultModule_;
	if( function.find( '.' ) != function.npos )
	{
		module = function.substr( 0, function.find( '.' ) );
		function = function.substr( function.find( '.' ) + 1 );
	}
	if( modules_.find( module ) == modules_.end() )
	{
		PyObject* obj = PyImport_ImportModule( (char*)module.c_str() );
		if( !obj )
		{
			PyErr_Clear();
			return NULL;
		}
		modules_[ module ] = obj;
	}


	PyObject* result = NULL;
	PyObject* func = PyObject_GetAttrString( modules_[ module ], (char*)( function.c_str() ) );
	if ( func != NULL )
	{
		PyItem* pyitem = new PyItem( item );
		PyObject* obj = Py_BuildValue( "(O)", pyitem );
		Py_XDECREF( pyitem );
		if( obj )
		{
			result = Script::ask( func,  obj, ( module + "." + function + ':' ).c_str() );
			if( !result )
				PyErr_Clear();
		}
		else
			PyErr_Clear();
	}
	else
		PyErr_Clear();
	return result;
}
const BW::string& PythonFunctor::name() const
{
	static BW::string name = "Python";
	return name;
}

bool PythonFunctor::text( const BW::string& textor, ItemPtr item, BW::string& result )
{
	BW_GUARD;

	result.clear();
	PyObjectPtr obj( call( textor, item ), PyObjectPtr::STEAL_REFERENCE);
	if( obj.exists() )
	{
		char* str;
		if( PyString_Check( obj.get() ) && ( str = PyString_AsString( obj.get() ) ) )
		{
			result = str;
			return true;
		}
	}
	return false;
}

bool PythonFunctor::update( const BW::string& updater, ItemPtr item, unsigned int& result )
{
	BW_GUARD;

	PyObjectPtr obj( call( updater, item ), PyObjectPtr::STEAL_REFERENCE) ;
	if( obj.exists() )
	{
		if (PyInt_Check( obj ) )
		{
			result = PyInt_AsLong( obj.get() );
			return true;
		}
	}
	return false;
}

bool PythonFunctor::act( const BW::string& action, ItemPtr item, bool& result )
{
	BW_GUARD;

	PyObjectPtr obj( call( action, item ), PyObjectPtr::STEAL_REFERENCE) ;
	if( obj.exists() )
	{
		if (PyInt_Check( obj ) )
		{
			result = ( PyInt_AsLong( obj.get() ) != 0 );
			return true;
		}
	}
	return false;
}

void PythonFunctor::defaultModule( const BW::string& module )
{
	defaultModule_ = module;
}

END_GUI_NAMESPACE
BW_END_NAMESPACE

