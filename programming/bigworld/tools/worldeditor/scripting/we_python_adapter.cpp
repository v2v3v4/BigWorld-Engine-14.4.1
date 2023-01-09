#include "pch.hpp"
#include "gizmo/item_view.hpp"
#include "worldeditor/scripting/we_python_adapter.hpp"
#include "worldeditor/terrain/texture_mask_cache.hpp"
#include "worldeditor/gui/controls/limit_slider.hpp"
#include "pyscript/script.hpp"
#include "resmgr/bwresource.hpp"
#include <afxcmn.h>


DECLARE_DEBUG_COMPONENT2( "PythonAdapter", 0 )

BW_BEGIN_NAMESPACE

WEPythonAdapter::WEPythonAdapter() :
	PythonAdapter()
{
}


WEPythonAdapter::~WEPythonAdapter()
{
}


void WEPythonAdapter::onBrowserObjectItemSelect( const BW::string& theTabName, 
		const BW::string& itemName, bool dblClick )
{
	BW_GUARD;

	if ( !proActive_ || !pScriptObject_ )
		return;

    BW::string fnName( "brwObject" );
	fnName += theTabName + "ItemSelect";
    BW::string selectedFile( itemName );

    if ( !itemName.empty() )
    {
        // Get the relative path, but preserve the case on the filename
		selectedFile = BWResource::getFilePath(
            BWResource::dissolveFilename( itemName )) +
            BWResource::getFilename( itemName );
    }


	// tell the python which object is selected
	PyObject * pFunction = PyObject_GetAttrString( pScriptObject_,
                const_cast<char*>( fnName.c_str() ) );

    if (pFunction != NULL)
    {
		Script::call(
	       	pFunction,
            Py_BuildValue( "(si)",
			selectedFile.c_str(), dblClick ? 1 : 0 ),
            "PythonAdapter::onBrowserObjectItemSelect: " );
    }
    else
	{
		PyErr_Clear();
	    Script::call(
			PyObject_GetAttrString( pScriptObject_,
				"brwObjectItemSelect" ),
			Py_BuildValue( "(ss)",
                theTabName.c_str(),
                selectedFile.c_str() ),
			"PythonAdapter::onBrowserObjectItemSelect: " );
    }


	// update the selection criteria
	BW::string tabName = "tabObject" + theTabName;
	Script::call( 
		PyObject_GetAttrString( pScriptObject_,
			"pgcObjectsTabSelect" ),
		Py_BuildValue( "(s)",
			tabName.c_str() ),
		"PythonAdapter::onBrowserObjectItemSelect: " );
}

void WEPythonAdapter::onBrowserObjectItemAdd()
{
	BW_GUARD;

	if ( !proActive_ || !pScriptObject_ )
		return;

	PyObject * pFunction = PyObject_GetAttrString( pScriptObject_, "brwObjectItemAdd");

    if (pFunction != NULL)
    {
		Script::call(
	       	pFunction,
            Py_BuildValue( "()" ),
            "PythonAdapter::onBrowserObjectItemAdd: " );
    }
}

void WEPythonAdapter::onPageControlTabSelect( const BW::string& fnPrefix, const BW::string& theTabName )
{
	BW_GUARD;

	if ( !proActive_ || !pScriptObject_ )
		return;

    const BW::string fnName = fnPrefix + "TabSelect";
	const BW::string tabName = "tab" + theTabName;

    PyObject * pFunction = PyObject_GetAttrString( pScriptObject_,
                const_cast<char*>( fnName.c_str() ) );

    if ( pFunction != NULL )
    {
        Script::call( pFunction,
                   		Py_BuildValue( "(s)",
               			tabName.c_str() ),
                   		"PythonAdapter::onPageControlTabSelect: " );
    }
    else
	{
		PyErr_Clear();
        Script::call(
            PyObject_GetAttrString( pScriptObject_,
                "pgcAllToolsTabSelect" ),
            Py_BuildValue( "(s)",
                tabName.c_str() ),
            "PythonAdapter::onPageControlTabSelect: " );
    }
}

void WEPythonAdapter::onBrowserTextureItemSelect( const BW::string& itemName )
{
	BW_GUARD;

	if ( !proActive_ || !pScriptObject_ )
		return;

	const BW::string fnName( "brwTexturesItemSelect" );
    BW::string selectedFile( itemName );

    if ( !itemName.empty() )
    {
        // Get the relative path, but preserve the case on the filename
		selectedFile = BWResource::getFilePath(
            BWResource::dissolveFilename( itemName )) +
            BWResource::getFilename( itemName );
    }


	// tell the python which object is selected
	PyObject * pFunction = PyObject_GetAttrString( pScriptObject_,
                const_cast<char*>( fnName.c_str() ) );

    if (pFunction != NULL)
    {
		Script::call(
	       		pFunction,
				Py_BuildValue( "(s)",
				selectedFile.c_str() ),
				"PythonAdapter::onBrowserTextureItemSelect: " );
    }
    else
	{
		ERROR_MSG( "script call [%s] does not exist\n", fnName.c_str() );
		PyErr_Clear();
    }
}


void WEPythonAdapter::setTerrainPaintMode(int mode)
{
	BW_GUARD;

	if ( !proActive_ || !pScriptObject_ )
		return;

    BW::string fnName( "setTerrainPaintMode" );

	PyObject * pFunction = 
        PyObject_GetAttrString
        ( 
            pScriptObject_,
            fnName.c_str() 
        );

    if (pFunction != NULL)
    {
		Script::call
        (
            pFunction,
            Py_BuildValue("(i)", mode),
            "PythonAdapter::setTerrainPaintMode: " 
        );
    }
    else
	{
		ERROR_MSG( "script call [%s] does not exist\n", fnName.c_str() );
		PyErr_Clear();
    }
}


void WEPythonAdapter::setTerrainPaintBrush( TerrainPaintBrushPtr paintBrush )
{
	BW_GUARD;

	if ( !proActive_ || !pScriptObject_ )
		return;

    BW::string fnName( "setTerrainPaintBrush" );

	PyObject * pFunction = 
        PyObject_GetAttrString
        ( 
            pScriptObject_,
            fnName.c_str() 
        );

    if (pFunction != NULL)
    {
		Script::call
        (
            pFunction,
            Py_BuildValue("(O)", paintBrush.getObject()),
            "PythonAdapter::setTerrainPaintBrush: " 
        );
    }
    else
	{
		ERROR_MSG( "script call [%s] does not exist\n", fnName.c_str() );
		PyErr_Clear();
    }
}


void WEPythonAdapter::onLimitSliderAdjust( const BW::string& name, float pos, float min, float max )
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

void WEPythonAdapter::limitSliderUpdate( LimitSlider* control, const BW::string& controlName )
{
	BW_GUARD;

	if ( !pScriptObject_ )
		return;

    proActive_ = false;
    
	BW::string fnName = controlName + "Update";

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
			control->setValue((float)PyFloat_AsDouble( pResult ));
		}
		else if (PyInt_Check( pResult ))
		{
			control->setValue((float)PyInt_AsLong( pResult ));
		}
		else
		{
			float value;
			float min;
			float max;

			if (PyArg_ParseTuple( pResult, "fff", &value, &min, &max ))
			{
				control->setRange( min, max );
				control->setValue( value);
			}
			else
			{
				ERROR_MSG( "PythonAdapter::sliderUpdate - %s did not return a float (or three).\n",
					controlName.c_str() );
				PyErr_Clear();
			}
		}

		Py_DECREF( pResult );
	}

    proActive_ = true;
}


void WEPythonAdapter::selectFilterChange( const BW::string& value )
{
	BW_GUARD;

	if ( proActive_ && pScriptObject_ )
	{
		const BW::string fnName( "cmbSelectFilterChange" );

		PyObject * pFunction = PyObject_GetAttrString( pScriptObject_,
					const_cast<char*>( fnName.c_str() ) );

		if ( pFunction != NULL )
		{
			Script::call( pFunction,
					Py_BuildValue( "(s)",
						value.c_str() ),
					"PythonAdapter::selectFilterChange: " );
		}
	}
}


void WEPythonAdapter::selectFilterUpdate( CComboBox* comboList )
{
	BW_GUARD;

	if ( !pScriptObject_ )
		return;

    proActive_ = false;
    
	if (comboList->GetCount() == 0)
		fillFilterKeys( comboList );

	BW::string fnName = "cmbSelectFilterUpdate";

	PyObject * pFunction = PyObject_GetAttrString( pScriptObject_,
				const_cast<char*>( fnName.c_str() ) );

	PyObject * pResult = NULL;

	if ( pFunction != NULL )
	{
		pResult = Script::ask( pFunction,
				PyTuple_New(0),
				"PythonAdapter::selectFilterUpdate: " );
	}
	else
	{
		PyErr_Clear();
	}

	if (pResult != NULL)
	{
		char * filterValue;
		// TODO:UNICODE: extract Unicode
		if (PyArg_ParseTuple( pResult, "s", &filterValue ))
		{
			BW::wstring wfilterValue;
			bw_utf8tow( filterValue, wfilterValue );
			int index = comboList->FindStringExact( -1, wfilterValue.c_str() );
			if( index != comboList->GetCurSel() )
				comboList->SetCurSel( index );
		}
		else
		{
			ERROR_MSG( "PythonAdapter::selectFilterUpdate - "
						"%s did not return a tuple of one string.\n",
						fnName.c_str() );
			PyErr_Clear();
		}

		Py_DECREF( pResult );
	}

    proActive_ = true;
}


void WEPythonAdapter::fillFilterKeys( CComboBox* comboList )
{
	BW_GUARD;

	if ( !pScriptObject_ )
		return;

    proActive_ = false;
    
	BW::string fnName = "cmbSelectFilterKeys";

	PyObject * pFunction = PyObject_GetAttrString( pScriptObject_,
				const_cast<char*>( fnName.c_str() ) );

	PyObject * pResult = NULL;

	if ( pFunction != NULL )
	{
		pResult = Script::ask( pFunction,
				PyTuple_New(0),
				"PythonAdapter::selectFilterKeys: " );
	}
	else
	{
		PyErr_Clear();
	}

	if (pResult != NULL)
	{
		MF_ASSERT(PySequence_Check( pResult ));
		Py_ssize_t size = PySequence_Size( pResult );
		for (Py_ssize_t i = 0; i < size; i++)
		{
			PyObject * pItem = PySequence_GetItem( pResult, i );
			MF_ASSERT(PyString_Check( pItem ));
			BW::wstring wstr;
			bw_utf8tow( PyString_AsString( pItem ), wstr );
			MF_ASSERT( i <= INT_MAX );
			comboList->InsertString( (int) i, wstr.c_str() );
		}

		Py_DECREF( pResult );
	}

    proActive_ = true;
}


void WEPythonAdapter::coordFilterChange( const BW::string& value )
{
	BW_GUARD;

	if ( proActive_ && pScriptObject_ )
	{
		const BW::string fnName( "cmbCoordFilterChange" );

		PyObject * pFunction = PyObject_GetAttrString( pScriptObject_,
					const_cast<char*>( fnName.c_str() ) );

		if ( pFunction != NULL )
		{
			Script::call( pFunction,
					Py_BuildValue( "(s)",
						value.c_str() ),
					"PythonAdapter::coordFilterChange: " );
		}
	}
}


void WEPythonAdapter::coordFilterUpdate( CComboBox* comboList )
{
	BW_GUARD;

	if ( !pScriptObject_ )
		return;

    proActive_ = false;
    
	if (comboList->GetCount() == 0)
		fillCoordFilterKeys( comboList );

	BW::string fnName = "cmbCoordFilterUpdate";

	PyObject * pFunction = PyObject_GetAttrString( pScriptObject_,
				const_cast<char*>( fnName.c_str() ) );

	PyObject * pResult = NULL;

	if ( pFunction != NULL )
	{
		pResult = Script::ask( pFunction,
				PyTuple_New(0),
				"PythonAdapter::coordFilterUpdate: " );
	}
	else
	{
		PyErr_Clear();
	}

	if (pResult != NULL)
	{
		char * filterValue;
		if (PyArg_ParseTuple( pResult, "s", &filterValue ))
		{
			BW::wstring wfilterValue;
			bw_utf8tow( filterValue, wfilterValue );
			int index = comboList->FindStringExact( -1, wfilterValue.c_str() );
			comboList->SetCurSel( index == CB_ERR ? 0 : index );
		}
		else
		{
			ERROR_MSG( "PythonAdapter::coordFilterUpdate - "
						"%s did not return a tuple of one string.\n",
						fnName.c_str() );
			PyErr_Clear();
		}

		Py_DECREF( pResult );
	}

    proActive_ = true;
}


void WEPythonAdapter::fillCoordFilterKeys( CComboBox* comboList )
{
	BW_GUARD;

	if ( !pScriptObject_ )
		return;

    proActive_ = false;
    
	BW::string fnName = "cmbCoordFilterKeys";

	PyObject * pFunction = PyObject_GetAttrString( pScriptObject_,
				const_cast<char*>( fnName.c_str() ) );

	PyObject * pResult = NULL;

	if ( pFunction != NULL )
	{
		pResult = Script::ask( pFunction,
				PyTuple_New(0),
				"PythonAdapter::coordFilterKeys: " );
	}
	else
	{
		PyErr_Clear();
	}

	if (pResult != NULL)
	{
		MF_ASSERT(PyTuple_Check( pResult ));
		Py_ssize_t size = PyTuple_Size( pResult );
		for (Py_ssize_t i = 0; i < size; i++)
		{
			PyObject * pItem = PyTuple_GetItem( pResult, i );
			MF_ASSERT(PyTuple_Check( pItem ));
			BW::wstring wstr;
			bw_utf8tow( PyString_AsString( PyTuple_GetItem( pItem, 0 ) ), wstr );
			MF_ASSERT( i <= INT_MAX );
			comboList->InsertString( (int)i, wstr.c_str() );
		}

		Py_DECREF( pResult );
	}

    proActive_ = true;
}


void WEPythonAdapter::projectLock( const BW::string& commitMessage )
{
	BW_GUARD;

	const BW::string fnName = "projectLock";
	PyObject * pFunction = PyObject_GetAttrString( pScriptObject_,
				const_cast<char*>( fnName.c_str() ) );

	if ( pFunction == NULL )
		return;

	Script::call( pFunction, Py_BuildValue( "(s)", commitMessage.c_str() ),
			"PageProject::OnBnClickedProjectSelectionLock: " );
}


void WEPythonAdapter::commitChanges(const BW::string& commitMessage, bool keepLocks)
{
	BW_GUARD;

	const BW::string fnName = "projectCommitChanges";
	PyObject * pFunction = PyObject_GetAttrString( pScriptObject_,
				const_cast<char*>( fnName.c_str() ) );

	if ( pFunction == NULL )
		return;

	Script::call( pFunction, Py_BuildValue( "(si)", commitMessage.c_str(), keepLocks ),
			"PageProject::OnBnClickedProjectCommitAll: " );
}


void WEPythonAdapter::discardChanges(const BW::string& commitMessage, bool keepLocks)
{
	BW_GUARD;

	const BW::string fnName = "projectDiscardChanges";
	PyObject * pFunction = PyObject_GetAttrString( pScriptObject_,
				const_cast<char*>( fnName.c_str() ) );

	if ( pFunction == NULL )
		return;

	Script::call( pFunction, Py_BuildValue( "(si)", commitMessage.c_str(), keepLocks ),
			"PageProject::OnBnClickedProjectDiscardAll: " );
}


void WEPythonAdapter::updateSpace()
{
	BW_GUARD;

	const BW::string fnName = "projectUpdateSpace";
	PyObject * pFunction = PyObject_GetAttrString( pScriptObject_,
				const_cast<char*>( fnName.c_str() ) );

	if ( pFunction == NULL )
		return;

	Script::call( pFunction, PyTuple_New(0),
			"PageProject::OnBnClickedProjectUpdate: " );
}


void WEPythonAdapter::calculateMap()
{
	BW_GUARD;

	const BW::string fnName = "projectCalculateMap";
	PyObject * pFunction = PyObject_GetAttrString( pScriptObject_,
				const_cast<char*>( fnName.c_str() ) );

	if ( pFunction != NULL )
	{
		Script::call( pFunction,
				PyTuple_New(0),
				"PythonAdapter::calculateMap: " );
	}
	else
	{
		PyErr_Clear();
	}
}


void WEPythonAdapter::exportMap()
{
	BW_GUARD;

	const BW::string fnName = "projectExportMap";
	PyObject * pFunction = PyObject_GetAttrString( pScriptObject_,
				const_cast<char*>( fnName.c_str() ) );

	if ( pFunction != NULL )
	{
		Script::call( pFunction,
				PyTuple_New(0),
				"PythonAdapter::exportMap: " );
	}
	else
	{
		PyErr_Clear();
	}
}


bool WEPythonAdapter::canSavePrefab()
{
	BW_GUARD;

	if ( pScriptObject_ )
    {
		const BW::string fnName = "canSavePrefab";

		PyObject * pFunction = PyObject_GetAttrString( pScriptObject_,
					const_cast<char*>( fnName.c_str() ) );

		if (pFunction != NULL)
		{
			PyObject * pResult = Script::ask(
				pFunction,
				Py_BuildValue( "()" ),
				"PythonAdapter::canSavePrefab: " );
				
			if (pResult != NULL)
			{
				if(PySizeT_Check( pResult ))
				{
					return (int)PySizeT_AsLong( pResult ) != 0;
				}
				else
				{
					ERROR_MSG( "PythonAdapter::canSavePrefab - canSavePrefab did not return an int.\n" );
					PyErr_Print();
				}
				Py_DECREF( pResult );
			}
		}
		else
		{
			PyErr_Clear();
		}
    }
	return false;
}


void WEPythonAdapter::saveSelectionPrefab( BW::string fileName )
{
	BW_GUARD;

	if ( pScriptObject_ )
    {
		const BW::string fnName = "savePrefab";

		PyObject * pFunction = PyObject_GetAttrString( pScriptObject_,
					const_cast<char*>( fnName.c_str() ) );

		if (pFunction != NULL)
		{
			Script::call(
				pFunction,
				Py_BuildValue( "(s)",
				fileName.c_str() ),
				"PythonAdapter::saveSelectionPrefab: " );
		}
    }
}

void WEPythonAdapter::expandSpace( const BW::string& spacePath, const BW::string& blankCDataFilePath,
		int westCnt, int eastCnt, int northCnt, int southCnt, bool chunkWithTerrain )
{
	BW_GUARD;

	if ( pScriptObject_ )
	{
		const BW::string fnName = "expandSpace";

		PyObject * pFunction = PyObject_GetAttrString( pScriptObject_,
			const_cast<char*>( fnName.c_str() ) );

		if (pFunction != NULL)
		{
			Script::call(
				pFunction,
				Py_BuildValue( "(ssiiiii)",
				spacePath.c_str(), blankCDataFilePath.c_str(), westCnt, eastCnt, northCnt, southCnt, chunkWithTerrain ),
				"PythonAdapter::expandSpace: " );
		}
	}
}

/**
 *	This method is called to delete the chunk items in the group.
 */
void WEPythonAdapter::deleteChunkItems( ChunkItemGroupPtr group )
{
	BW_GUARD;

	if (pScriptObject_)
    {
		char * fnName = "deleteChunkItems";

		PyObject * pFunc = PyObject_GetAttrString( pScriptObject_, fnName );

		if (pFunc != NULL)
		{
			Script::call( pFunc,
				Py_BuildValue( "(O)", static_cast<PyObject *>( group.get() ) ),
				"PythonAdapter::deleteChunkItems: " );
		}
    }
}

PyObjectPtr WEPythonAdapter::addChunkItem( const BW::string &resourceID )
{
	BW_GUARD;

	if (pScriptObject_)
	{
		char * fnName = "addChunkItem";

		PyObject * pFunc = PyObject_GetAttrString( pScriptObject_, fnName );

		if (pFunc != NULL)
		{
			PyObject *pArgs = PyTuple_New(1);
			PyTuple_SetItem(pArgs, 0, Script::getData(resourceID));

			PyObjectPtr ret = Script::ask( pFunc,
				pArgs,
				"PythonAdapter::addChunkItem: " );

			return ret;
		}
	}

	return NULL;
}

void WEPythonAdapter::setSelection( ChunkItemGroupPtr group )
{
	BW_GUARD;

	if (pScriptObject_)
	{
		char * fnName = "setSelection";

		PyObject * pFunc = PyObject_GetAttrString( pScriptObject_, fnName );

		if (pFunc != NULL)
		{
			Script::call( pFunc,
				Py_BuildValue( "(O)", static_cast<PyObject *>( group.get() ) ),
				"PythonAdapter::setSelection: " );
		}
	}
}


void WEPythonAdapter::selectAll()
{
	BW_GUARD;

	if (pScriptObject_)
	{
		char * fnName = "selectAll";

		PyObject * pFunc = PyObject_GetAttrString( pScriptObject_, fnName );

		if (pFunc != NULL)
		{
			Script::call( pFunc,
				PyTuple_New(0),
				"PythonAdapter::selectAll: " );
		}
	}
}

bool WEPythonAdapter::createSpaceLevelImage( const BW::string& path, int imgSize, int ChunkSizeX, int ChunkSizeY,
	float pixelsPerMetre, int topLeftGridPosX,  int topLeftGridPosY, int blockSize )
{
	BW_GUARD;
	bool result = false;
	if (pScriptObject_)
	{
		char * fnName = "createSpaceLevelImage";

		PyObject * pFunc = PyObject_GetAttrString( pScriptObject_, fnName );

		if (pFunc != NULL)
		{
			PyObject * pResult = Script::ask( pFunc,
				Py_BuildValue( "(siiifiii)", path.c_str(), imgSize, ChunkSizeX, ChunkSizeY, pixelsPerMetre, topLeftGridPosX, topLeftGridPosY, blockSize ),
				"PythonAdapter::createSpaceLevelImage: " );

			if(PyInt_Check( pResult ))
			{
				result = (int)PyInt_AsLong( pResult ) != 0;
				}
				
				Py_DECREF( pResult );
		}
	}
	return result;
}

bool WEPythonAdapter::createSpaceSingleMap( const BW::string& path, int imgSize, int ChunkSizeX, int ChunkSizeY, int outputWidth, int outputHeight )
{
	BW_GUARD;
	bool result = false;
	if (pScriptObject_)
	{
		char * fnName = "createSpaceSingleMap";

		PyObject * pFunc = PyObject_GetAttrString( pScriptObject_, fnName );

		if (pFunc != NULL)
		{
			PyObject * pResult = Script::ask( pFunc,
				Py_BuildValue( "(siiiii)", path.c_str(), imgSize, ChunkSizeX, ChunkSizeY, outputWidth, outputHeight ),
				"PythonAdapter::createSpaceSingleMap: " );

			if(PyInt_Check( pResult ))
			{
			result = (int)PyInt_AsLong( pResult ) != 0;
			}
			
			Py_DECREF( pResult );
			}
	}
	return result;
}
BW_END_NAMESPACE

