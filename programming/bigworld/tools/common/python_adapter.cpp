#include "pch.hpp"

#include "python_adapter.hpp"

#include "pyscript/script.hpp"
#include "resmgr/bwresource.hpp"

#include "afxcmn.h"

DECLARE_DEBUG_COMPONENT2( "PythonAdapter", 0 )

BW_BEGIN_NAMESPACE

PythonAdapter::PythonAdapter() : 
	pScriptObject_(NULL),
	proActive_(true)
{
	BW_GUARD;

	BW::string scriptFile = "UIAdapter";

    PyObject * pModule = PyImport_ImportModule(
    		const_cast<char *>( scriptFile.c_str()) );

    if (PyErr_Occurred())
    {
		ERROR_MSG( "PythonAdapter - Failed to init adapter\n" );
        PyErr_Print();
	}

    if (pModule != NULL)
    {
        pScriptObject_ = pModule;
        DEBUG_MSG( "PythonAdapter was loaded correctly\n" );
    }
    else
    {
        ERROR_MSG( "PythonAdapter - Could not get module %s\n", scriptFile.c_str() );
        PyErr_Print();
    }
}


PythonAdapter::~PythonAdapter()
{
	BW_GUARD;

	Py_XDECREF( pScriptObject_ );
}

bool PythonAdapter::hasScriptObject() const
{
	return pScriptObject_ != NULL;
}


void PythonAdapter::reloadUIAdapter()
{
	BW_GUARD;

	BW::string scriptFile = "UIAdapter";

    // get rid of script object
	Py_XDECREF( pScriptObject_ );
	pScriptObject_ = NULL;

	//Check if python has ever loaded the module
	PyObject *modules = PyImport_GetModuleDict();
	PyObject *m;

	// if so, reload it
	if ((m = PyDict_GetItemString(modules,
    		const_cast<char*>( scriptFile.c_str() ))) != NULL &&
	    	PyModule_Check(m))
	{
		pScriptObject_ = PyImport_ReloadModule( m );
	}
	else
	{
		pScriptObject_ = PyImport_ImportModule(
        	const_cast<char*>( scriptFile.c_str() ) );
	}

    if ( pScriptObject_ == NULL )
    {
    	ERROR_MSG( "PythonAdapter - Could not load UIAdapter Module\n" );
        PyErr_Print();
    }
}


bool PythonAdapter::call(const BW::string& fnName)
{
	BW_GUARD;

	if (!pScriptObject_)
		return false;

	bool handled = false;

	PyObject * pFunction = PyObject_GetAttrString( pScriptObject_,
							const_cast<char*>( fnName.c_str() ) );

	if ( pFunction != NULL )
	{
		handled = Script::call( pFunction,
							PyTuple_New(0),
							"PythonAdapter::call: " );
	}
	else
	{
		ERROR_MSG( "PythonAdapter::call - no script for [%s]\n", fnName.c_str());
		PyErr_Print();
	}

	return handled;
}

bool PythonAdapter::callString( const BW::string& fnName, const BW::string& param )
{
	BW_GUARD;

	if (!pScriptObject_)
		return false;

	bool handled = false;

	PyObject * pFunction = PyObject_GetAttrString( pScriptObject_,
							const_cast<char*>( fnName.c_str() ) );

	if ( pFunction != NULL )
	{
		handled = Script::call( pFunction,
							Py_BuildValue( "(s)", param.c_str() ),
							"PythonAdapter::callString: " );
	}
	else
	{
		ERROR_MSG( "PythonAdapter::callString - no script for [%s]\n", fnName.c_str());
		PyErr_Print();
	}

	return handled;
}

bool PythonAdapter::callString2( const BW::string& fnName, const BW::string& param1, const BW::string& param2 )
{
	BW_GUARD;

	if (!pScriptObject_)
		return false;

	bool handled = false;

	PyObject * pFunction = PyObject_GetAttrString( pScriptObject_,
							const_cast<char*>( fnName.c_str() ) );

	if ( pFunction != NULL )
	{
		handled = Script::call( pFunction,
							Py_BuildValue( "(ss)", param1.c_str(), param2.c_str() ),
							"PythonAdapter::callString: " );
	}
	else
	{
		ERROR_MSG( "PythonAdapter::callString - no script for [%s]\n", fnName.c_str());
		PyErr_Print();
	}

	return handled;
}


bool PythonAdapter::ActionScriptExecute(const BW::string & actionName)
{
	BW_GUARD;

	if (!pScriptObject_)
		return false;

	bool handled = false;

	BW::string fnName = actionName + "Execute";

	PyObject * pFunction = PyObject_GetAttrString( pScriptObject_,
							const_cast<char*>( fnName.c_str() ) );

	if ( pFunction != NULL )
	{
		handled = Script::call( pFunction,
							PyTuple_New(0),
							"PythonAdapter::ActionScriptExecute: " );
	}
	else
	{
		// use the default handler
		PyObject * pDefault = PyObject_GetAttrString( pScriptObject_,
							"defaultActionExecute" );

		if (pDefault != NULL)
		{
			handled = Script::call(	pDefault,
							Py_BuildValue( "(s)", actionName.c_str() ),
							"UI::defaultActionExecute: " );
		}
		else
		{
			ERROR_MSG( "PythonAdapter::ActionScriptExecute - "
					"No default action handler (defaultActionExecute).\n" );
			PyErr_Print();
		}
	}

	return handled;
}


bool PythonAdapter::ActionScriptUpdate(const BW::string & actionName, int & enabled, int & checked)
{
	BW_GUARD;

	if ( !pScriptObject_ )
		return false;

	bool handled = false;

	BW::string fnName = actionName + "Update";

	PyObject * pFunction = PyObject_GetAttrString( pScriptObject_,
				const_cast<char*>( fnName.c_str() ) );

	PyObject * pResult = NULL;

	if ( pFunction != NULL )
	{
		pResult = Script::ask( pFunction,
				PyTuple_New(0),
				"PythonAdapter::ActionScriptUpdate: " );
	}
	else
	{
		// use the default handler
		PyErr_Clear();
		PyObject * pDefaultFunction =
			PyObject_GetAttrString( pScriptObject_,
				"defaultActionUpdate" );

		if (pDefaultFunction != NULL)
		{
			pResult = Script::ask(
				pDefaultFunction,
				Py_BuildValue( "(s)", actionName.c_str() ),
				"PythonAdapter::ActionScriptUpdate: " );
		}
		else
		{
			PyErr_Clear();
		}
	}

	if (pResult != NULL)
	{
		if (PyArg_ParseTuple( pResult, "ii", &enabled, &checked ))
		{
			handled = true;
		}
		else
		{
			ERROR_MSG( "PythonAdapter::ActionScriptUpdate - "
						"%s did not return a tuple of two integers.\n",
						actionName.c_str() );
			PyErr_Print();
		}

		Py_DECREF( pResult );
	}

	return handled;
}

void PythonAdapter::onSliderAdjust( const BW::string& name, int pos, int min, int max )
{
	BW_GUARD;

	if ( !proActive_ || !pScriptObject_ )
		return;

	BW::string fnName = name + "Adjust";

	PyObject * pFunction = PyObject_GetAttrString( pScriptObject_,
				const_cast<char*>( fnName.c_str() ) );

	if ( pFunction != NULL )
	{
		Script::call( pFunction,
			Py_BuildValue(
				"(fff)",
				(float)pos,
				(float)min,
				(float)max ),
			"PythonAdapter::onSliderAdjust: " );
	}
	else
	{
		PyErr_Clear();
		Script::call(
			PyObject_GetAttrString( pScriptObject_,
				"onSliderAdjust" ),
			Py_BuildValue(
				"(sfff)",
				name.c_str(),
				(float)pos,
				(float)min,
				(float)max ),
			"PythonAdapter::onSliderAdjust: " );
	}
}

void PythonAdapter::sliderUpdate( CSliderCtrl* slider, const BW::string& sliderName )
{
	BW_GUARD;

	if ( !pScriptObject_ )
		return;

    proActive_ = false;
    
	BW::string fnName = sliderName + "Update";

	PyObject * pFunction = PyObject_GetAttrString( pScriptObject_,
				const_cast<char*>( fnName.c_str() ) );

	PyObject * pResult = NULL;

	if ( pFunction != NULL )
	{
		pResult = Script::ask( pFunction,
				PyTuple_New(0),
				"PythonAdapter::sliderUpdate: " );
	}
	else
	{
		PyErr_Clear();
	}

	if (pResult != NULL)
	{
		if (PyFloat_Check( pResult ))
		{
			slider->SetPos((int)PyFloat_AsDouble( pResult ));
		}
		else if (PyInt_Check( pResult ))
		{
			slider->SetPos((int)PyInt_AsLong( pResult ));
		}
		else
		{
			float value;
			float min;
			float max;

			if (PyArg_ParseTuple( pResult, "fff", &value, &min, &max ))
			{
				slider->SetRangeMin((int)min);
				slider->SetRangeMax((int)max);
				slider->SetPos((int)value);
			}
			else
			{
				ERROR_MSG( "PythonAdapter::sliderUpdate - %s did not return a float (or three).\n",
					sliderName.c_str() );
				PyErr_Clear();
			}
		}

		Py_DECREF( pResult );
	}

    proActive_ = true;
}


void PythonAdapter::onListItemSelect( const BW::string& name, int index )
{
	BW_GUARD;

	if ( proActive_ && pScriptObject_ )
    {
    	BW::string fnName = name + "ItemSelect";

      PyObject * pFunction = PyObject_GetAttrString( pScriptObject_,
                	const_cast<char*>( fnName.c_str() ) );

        if ( pFunction != NULL )
        {
        	Script::call(
        		pFunction,
                Py_BuildValue( "(i)", index ),
                "PythonAdapter::onListItemSelect: " );
        }
        else
		{
			PyErr_Clear();
    		Script::call(
				PyObject_GetAttrString( pScriptObject_,
					"onListItemSelect" ),
				Py_BuildValue( "(si)", name.c_str(), index ),
				"PythonAdapter::onListItemSelect: " );
        }
    }
}

void PythonAdapter::onListSelectUpdate( CListBox* listBox, const BW::string& listBoxName )
{
	BW_GUARD;

	if ( !pScriptObject_ )
		return;

    proActive_ = false;
    
	BW::string fnName = listBoxName + "Update";

	PyObject * pFunction = PyObject_GetAttrString( pScriptObject_,
				const_cast<char*>( fnName.c_str() ) );

	PyObject * pResult = NULL;

	if ( pFunction != NULL )
	{
		pResult = Script::ask( pFunction,
				PyTuple_New(0),
				"PythonAdapter::onListSelectUpdate: " );
	}
	else
	{
		PyErr_Clear();
	}

	if (pResult != NULL)
	{
		if (PyInt_Check( pResult ))
		{
			if( (int)PyInt_AsLong( pResult ) != -1 )
				listBox->SetCurSel((int)PyInt_AsLong( pResult ));
		}
		else
		{
			ERROR_MSG( "PythonAdapter::onListSelectUpdate - %s did not return a int.\n",
				listBoxName.c_str() );
			PyErr_Clear();
		}

		Py_DECREF( pResult );
	}

    proActive_ = true;
}


void PythonAdapter::onListItemToggleState( const BW::string & name,
									 unsigned int idx, bool state )
{
	BW_GUARD;

	if ( !pScriptObject_ )
		return;

    BW::string fnName = name + "ItemToggleState";

	PyObject * pFunction = PyObject_GetAttrString( pScriptObject_,
				const_cast<char*>( fnName.c_str() ) );

	Script::call( pFunction,
		Py_BuildValue( "(ii)", idx, state?1:0 ),
			"PythonAdapter::onListItemToggleState: ", true );
}


void PythonAdapter::contextMenuGetItems( const BW::wstring& type, const BW::wstring & path,
	BW::map<int,BW::wstring>& items )
{
	BW_GUARD;

	if ( !pScriptObject_ )
		return;

	const BW::string fnName = "contextMenuGetItems";

	PyObject * pFunction = PyObject_GetAttrString( pScriptObject_,
				const_cast<char*>( fnName.c_str() ) );

	if (pFunction == NULL)
	{
		PyErr_Clear();
		return;
	}

	PyObject * pResult = Script::ask(
		pFunction,
		Py_BuildValue( "(ss)",
		bw_wtoutf8( type ).c_str(), bw_wtoutf8( path ).c_str() ),
		"PythonAdapter::contextMenuGetItems: " );

	if ( pResult == NULL )
		return;

	MF_ASSERT(PyList_Check( pResult ));
	Py_ssize_t size = PyList_Size( pResult );
	for ( Py_ssize_t i = 0; i < size; i++ )
	{
		PyObject * pItem = PyList_GetItem( pResult, i );
		MF_ASSERT(PyTuple_Check( pItem ));
		MF_ASSERT(PyTuple_Size( pItem ) == 2);
		items.insert( std::pair<int,BW::wstring>(
			PyInt_AS_LONG( PyTuple_GetItem( pItem, 0 ) ),
			bw_utf8tow( PyString_AsString( PyTuple_GetItem( pItem, 1 ) ) ) ) );
	}

	Py_DECREF( pResult );
}

void PythonAdapter::contextMenuHandleResult( const BW::wstring& type, const BW::wstring & path, int result )
{
	BW_GUARD;

	if ( !pScriptObject_ )
		return;

	const BW::string fnName = "contextMenuHandleResult";

	PyObject * pFunction = PyObject_GetAttrString( pScriptObject_,
				const_cast<char*>( fnName.c_str() ) );

	if (pFunction == NULL)
	{
		PyErr_Clear();
		return;
	}

	Script::call(
		pFunction,
		Py_BuildValue( "(ssi)",
		bw_wtoutf8( type ).c_str(), bw_wtoutf8( path ).c_str(), result ),
		"PythonAdapter::contextMenuHandleResult: " );
}
BW_END_NAMESPACE

