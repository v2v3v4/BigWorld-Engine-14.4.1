#include "pch.hpp"
#include "i_model_editor_app.hpp"

#include "app/me_app.hpp"
#include "app/mru.hpp"

#include "gui/panel_manager.hpp"
#include "models/mutant.hpp"
#include "moo/managed_texture.hpp"

#include "resmgr/string_provider.hpp"

#include "editor_shared/dialogs/file_dialog.hpp"
#include "editor_shared/dialogs/message_box.hpp"
#include "editor_shared/cursor/wait_cursor.hpp"

BW_BEGIN_NAMESPACE

namespace
{
	IModelEditorApp * s_modelEditorApp = NULL;
}

//==============================================================================
IModelEditorApp::IModelEditorApp()
{
	s_modelEditorApp = this;
}


//==============================================================================
void IModelEditorApp::loadModel( const char* modelName )
{
	modelToLoad_ = modelName;
}


//==============================================================================
void IModelEditorApp::addModel( const char* modelName )
{
	BW_GUARD;

	//If there is no model loaded then load this one...
	if (MeApp::instance().mutant()->modelName() == "")
	{
		loadModel( modelName );
	}
	else if (!MeApp::instance().mutant()->nodefull())
	{
		ERROR_MSG( "Models can only be added to nodefull models.\n\n"
			"  \"%s\"\n\n"
			"is not a nodefull model.\n",
			MeApp::instance().mutant()->modelName().c_str() );
	}
	else if (!MeApp::instance().mutant()->nodefull( modelName ))
	{
		ERROR_MSG( "Only nodefull models can be added to other models.\n\n"
			"  \"%s\"\n\n"
			"is not a nodefull model.\n",
			modelName );
	}
	else if ( !MeApp::instance().mutant()->canAddModel( modelName ))
	{
		ERROR_MSG( "The model cannot be added since it shares no nodes in common with the loaded model.\n" );
	}
	else
	{
		modelToAdd_ = modelName;
	}
}

//==============================================================================
void IModelEditorApp::OnFileReloadTextures()
{
	BW_GUARD;

	WaitCursor wait;

	Moo::ManagedTexture::accErrs( true );

	Moo::TextureManager::instance()->reloadAllTextures();

	BW::string errStr = Moo::ManagedTexture::accErrStr();
	if (errStr != "")
	{
		ERROR_MSG( "Moo:ManagedTexture::load, unable to load the following textures:\n"
			"%s\n\nPlease ensure these textures exist.\n", errStr.c_str() );
	}

	Moo::ManagedTexture::accErrs( false );

}

//==============================================================================
// File Menu Open
void IModelEditorApp::OnFileOpen()
{
	BW_GUARD;

	static wchar_t szFilter[] =	L"Model (*.model)|*.model||";
	BWFileDialog::FDFlags flags = ( BWFileDialog::FDFlags )
		( BWFileDialog::FD_HIDEREADONLY | BWFileDialog::FD_FILEMUSTEXIST );
	BWFileDialog fileDlg ( true, L"", L"", flags, szFilter);

	BW::string modelsDir;
	MRU::instance().getDir( "models" ,modelsDir );
	BW::wstring wmodelsDir = bw_utf8tow( modelsDir );
	fileDlg.initialDir( wmodelsDir.c_str() );

	if (fileDlg.showDialog())
	{
		modelToLoad_ = BWResource::dissolveFilename( bw_wtoutf8( fileDlg.getFileName() ));
		if (!BWResource::validPath( modelToLoad_ ))
		{
			MessageBoxFlags messageBoxFlags = ( MessageBoxFlags )
				(	MessageBoxFlags::BW_MB_OK |
					MessageBoxFlags::BW_MB_ICONWARNING );
			BW::MessageBox( NULL,
				Localise(L"MODELEDITOR/GUI/MODEL_EDITOR/BAD_DIR_MODEL_LOAD"),
				Localise(L"MODELEDITOR/GUI/MODEL_EDITOR/UNABLE_RESOLVE_MODEL"),
				messageBoxFlags );
			modelToLoad_ = "";
		}
	}
}

//==============================================================================
/*~ function ModelEditor.loadModel
 *	@components{ modeleditor }
 *
 *	Loads the specified model into the editor.
 *
 *	@param modelName The name of the model to load.
 */
static PyObject * py_loadModel( PyObject * args )
{
	BW_GUARD;

	char* modelName;

	if (!PyArg_ParseTuple( args, "s", &modelName ))
	{
		PyErr_SetString( PyExc_TypeError, "py_openModel: Expected a string argument." );
		return NULL;
	}
	
	//Either Ctrl or Alt will result in the model being added
	if ((GetAsyncKeyState(VK_LCONTROL) < 0) ||
		(GetAsyncKeyState(VK_RCONTROL) < 0) ||
		(GetAsyncKeyState(VK_LMENU) < 0) ||
		(GetAsyncKeyState(VK_RMENU) < 0))
	{
		s_modelEditorApp->addModel( modelName );
	}
	else if (MeApp::instance().canExit( false ))
	{
		s_modelEditorApp->loadModel( modelName );
	}

	Py_RETURN_NONE;
}
PY_MODULE_FUNCTION( loadModel, ModelEditor )

/*~ function ModelEditor.addModel
 *	@components{ modeleditor }
 *
 *	Adds the specified model to the currently loaded model. 
 *	Only Nodefull models can be added to other Nodefull models.
 *
 *	@param modelName The name of the model to be added.
 */
static PyObject * py_addModel( PyObject * args )
{
	BW_GUARD;

	char* modelName;

	if (!PyArg_ParseTuple( args, "s", &modelName ))
	{
		PyErr_SetString( PyExc_TypeError, "py_addModel: Expected a string argument." );
		return NULL;
	}

	if ( BWResource::openSection( modelName ) == NULL )
	{
		PyErr_SetString( PyExc_IOError, "py_addModel: The model was not found." );
		return NULL;
	}

	s_modelEditorApp->addModel( modelName );

	Py_RETURN_NONE;
}
PY_MODULE_FUNCTION( addModel, ModelEditor )

/*~ function ModelEditor.removeAddedModels
 *	@components{ modeleditor }
 *
 *	This function removes any added models from the loaded model. 
 */
static PyObject * py_removeAddedModels( PyObject * args )
{
	BW_GUARD;

	if (MeApp::instance().mutant())
		MeApp::instance().mutant()->removeAddedModels();

	Py_RETURN_NONE;
}
PY_MODULE_FUNCTION( removeAddedModels, ModelEditor )

/*~ function ModelEditor.hasAddedModels
 *	@components{ modeleditor }
 *	
 *	This function checks whether the loaded model currently has any added models.
 *
 *	@return Returns True (1) if there are any added models, False (0) otherwise.
 */
static PyObject * py_hasAddedModels( PyObject * args )
{
	BW_GUARD;

	if (MeApp::instance().mutant())
		return PyInt_FromLong( MeApp::instance().mutant()->hasAddedModels() );

	return PyInt_FromLong( 0 );
}
PY_MODULE_FUNCTION( hasAddedModels, ModelEditor )

	/*~ function ModelEditor.loadLights
 *	@components{ modeleditor }
 *
 *	This function loads the specified light setup. 
 *
 *	@param lightsName The name of the light to be loaded.
 */
static PyObject * py_loadLights( PyObject * args )
{
	BW_GUARD;

	char* lightsName;

	if (!PyArg_ParseTuple( args, "s", &lightsName ))
	{
		PyErr_SetString( PyExc_TypeError, "py_openModel: Expected a string argument." );
		return NULL;
	}
	
	s_modelEditorApp->loadLights( lightsName );

	Py_RETURN_NONE;
}
PY_MODULE_FUNCTION( loadLights, ModelEditor )

/*~ function ModelEditor.exit
 *	@components{ modeleditor }
 *
 *	This function closes ModelEditor.
 * 
 *	@param	ignoreChanges	(optional) If true then ME does not bother asking
 * to save changes. Defaults to false.
 */
static void exit( bool ignoreChanges )
{
	BW_GUARD;
	s_modelEditorApp->exit( ignoreChanges );
}
PY_AUTO_MODULE_FUNCTION( RETVOID, exit, OPTARG( bool, false, END ), ModelEditor );

/*~ function ModelEditor.openFile
 *	@components{ modeleditor }
 * 
 *	This function enables the Open File dialog, which allows a model to be loaded.
 */
static PyObject * py_openFile( PyObject * args )
{
	BW_GUARD;

	if (MeApp::instance().canExit( false ))
		s_modelEditorApp->OnFileOpen();

	Py_RETURN_NONE;
}
PY_MODULE_FUNCTION( openFile, ModelEditor )

/*~ function ModelEditor.addFile
 *	@components{ modeleditor }
 *
 *	This function enables the Open File dialog, which allows a model to be 
 *	added to the currently loaded model.
 */
static PyObject * py_addFile( PyObject * args )
{
	BW_GUARD;

	s_modelEditorApp->OnFileAdd();
	Py_RETURN_NONE;
}
PY_MODULE_FUNCTION( addFile, ModelEditor )

	/*~	function ModelEditor.reloadTextures
 *	@components{ modeleditor }
 *
 *	This function forces all textures to be reloaded.
 */
static PyObject * py_reloadTextures( PyObject * args )
{
	BW_GUARD;

	s_modelEditorApp->OnFileReloadTextures();
	Py_RETURN_NONE;
}
PY_MODULE_FUNCTION( reloadTextures, ModelEditor )

/*~ function ModelEditor.regenBoundingBox
 *	@components{ modeleditor }
 * 
 *	This function forces the model's Visibility Bounding Box to be recalculated.
 */
static PyObject * py_regenBoundingBox( PyObject * args )
{
	BW_GUARD;

	s_modelEditorApp->OnFileRegenBoundingBox();
	Py_RETURN_NONE;
}
PY_MODULE_FUNCTION( regenBoundingBox, ModelEditor )

/*~ function ModelEditor.appPrefs
 *	@components{ modeleditor }
 *
 *	This function opens the ModelEditor's Preferences dialog.
 */
static PyObject * py_appPrefs( PyObject * args )
{
	BW_GUARD;

	s_modelEditorApp->OnAppPrefs();
	Py_RETURN_NONE;
}
PY_MODULE_FUNCTION( appPrefs, ModelEditor )

/*~ function ModelEditor.canCaptureModel
 *	@components{ modeleditor }
 * 
 *	This function checks if the model is ready to capture.
 *	
 *	@return	Returns True if the model is ready to capture, False otherwise.
 */
static bool canCaptureModel()
{
	BW_GUARD;

	return MeApp::instance().mutant()->canCaptureModel();
}
PY_AUTO_MODULE_FUNCTION( RETDATA, canCaptureModel, END, ModelEditor );

/*~ function ModelEditor.captureModel
 *	@components{ modeleditor }
 * 
 *	This function captures a picture of the rendered model.
 *
 *	@param	fileName	The name of the file to store the image.
 *
 *	@return	Returns True if the model has been captured, False otherwise.
 */
static bool captureModel( const BW::string& fileName )
{
	BW_GUARD;

	return MeApp::instance().mutant()->canCaptureModel() &&
		   MeApp::instance().mutant()->captureModel( fileName );
}
PY_AUTO_MODULE_FUNCTION( RETDATA, captureModel, ARG( BW::string, END ), ModelEditor );

BW_END_NAMESPACE