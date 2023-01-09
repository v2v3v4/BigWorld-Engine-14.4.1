#include "pch.hpp"

#include "cstdmf/dprintf.hpp"

#include "appmgr/commentary.hpp"
#include "appmgr/options.hpp"

#include "common/compile_time.hpp"

#include "gizmo/undoredo.hpp"

#include "particle/particle_system_manager.hpp"

#include "physics2/material_kinds.hpp"

#include "pyscript/personality.hpp"
#include "pyscript/py_import_paths.hpp"
#include "pyscript/script.hpp"

#include "resmgr/bwresource.hpp"
#include "resmgr/string_provider.hpp"

#include "romp/console_manager.hpp"
#include "romp/xconsole.hpp"

#include "me_module.hpp"
#include "me_app.hpp"
#include "me_shell.hpp"

#include "guimanager/gui_functor_cpp.hpp"
#include "panel_manager.hpp"

DECLARE_DEBUG_COMPONENT( 0 )
#include "me_error_macros.hpp"

#include "me_scripter.hpp"
#include "entitydef/constants.hpp"

#include "editor_shared/cursor/wait_cursor.hpp"
#include "tools/modeleditor_core/Models/mutant.hpp"

#include "guitabs/guitabs_content.hpp"
#include "tools/common/utilities.hpp"
#include "moo/draw_context.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: Include doco for other modules
// -----------------------------------------------------------------------------

/*~ module BigWorld
 *  @components{ modeleditor }
 */

/*~ module GUI
 *  @components{ modeleditor }
 */

/*~ module PostProcessing
 *  @components{ modeleditor }
 */


// -----------------------------------------------------------------------------
// Section: me_scripter
// -----------------------------------------------------------------------------

/*~ function ModelEditor.addCommentryMsg
 *	@components{ modeleditor }
 *
 *	This function adds a message to the	Commentary Console.
 *	
 *	@param	msg The commentary message.
 *	@param	id	An id indicating the type of message. Can be
 *			one of the family of Commentary::ERROR,
 *			Commentary::CRITICAL, etc.
 */
static PyObject * py_addCommentaryMsg( PyObject * args )
{
	BW_GUARD;

	int id = Commentary::COMMENT;
	char* tag;

	if (!PyArg_ParseTuple( args, "s|i", &tag, &id ))
	{
		PyErr_SetString( PyExc_TypeError, "ModelEditor.addCommentaryMsg(): Argument parsing error." );
		return NULL;
	}

	if ( stricmp( tag, "" ) )
	{
		Commentary::instance().addMsg( BW::string( tag ), id );
		dprintf( "Commentary: %s\n", tag );
	}
	else
	{
		Commentary::instance().addMsg( BW::string( "NULL" ), Commentary::WARNING );
	}

	Py_RETURN_NONE;
}
PY_MODULE_FUNCTION( addCommentaryMsg, ModelEditor )

/*~ function ModelEditor.undo
 *	@components{ modeleditor }
 *
 *	This function undoes the most recent operation, returning
 *	its description. If it is passed a positive integer argument,
 *	then it just returns the description for that level of the
 *	undo stack and does not actually undo anything.
 *	If there is no undo level, an empty string is returned.
 *
 *	@param level The level of the undo stack to get the description of.
 *
 *	@return Returns the description of the given level in the undo stack, 
 *			returns an empty string if no level was given.
 */
static PyObject * py_undo( PyObject * args )
{
	BW_GUARD;

	WaitCursor wait; // This could potentially take a while

	int forStep = -1;
	if (!PyArg_ParseTuple( args, "|i", &forStep ))
	{
		PyErr_SetString( PyExc_TypeError, "ModelEditor.undo() "
			"expects an optional integer argument" );
		return NULL;
	}

	BW::string what = UndoRedo::instance().undoInfo( max(0,forStep) );

	if (forStep < 0)
	{
		ME_INFO_MSGW( Localise(L"MODELEDITOR/APP/ME_SCRIPTER/UNDOING", what ) );
		UndoRedo::instance().undo();
	}

	return Script::getData( what );
}
PY_MODULE_FUNCTION( undo, ModelEditor )

/*~ function ModelEditor.redo
 *	@components{ modeleditor }
 *
 *	This function redoes the most recent undo operation, returning
 *	its description. If it is passed a positive integer argument,
 *	then it just returns the description for that level of the
 *	redo stack and does not actually redo anything.
 *	If there is no redo level, an empty string is returned.
 *
 *	@param level The level of the redo stack to get the description of.
 *
 *	@return Returns the description of the given level in the redo stack, 
 *			returns an empty string if no level was given.
 */
static PyObject * py_redo( PyObject * args )
{
	BW_GUARD;

	WaitCursor wait; // This could potentially take a while

	int forStep = -1;
	if (!PyArg_ParseTuple( args, "|i", &forStep ))
	{
		PyErr_SetString( PyExc_TypeError, "ModelEditor.redo() "
			"expects an optional integer argument" );
		return NULL;
	}

	BW::string what = UndoRedo::instance().redoInfo( max(0,forStep) );

	if (forStep < 0)
	{
		ME_INFO_MSGW( Localise(L"MODELEDITOR/APP/ME_SCRIPTER/REDOING", what ) );
		UndoRedo::instance().redo();
	}

	return Script::getData( what );
}
PY_MODULE_FUNCTION( redo, ModelEditor )

/*~ function ModelEditor.addUndoBarrier
 *	@components{ modeleditor }
 *
 *	Adds an undo/redo barrier with the given name.
 *
 *	@param name The name of the barrier to be added.
 *	@param skipIfNoChange If this value is set then no barrier 
 *		   will be set if the undo stack is empty. Default is 0.
 */
static PyObject * py_addUndoBarrier( PyObject * args )
{
	BW_GUARD;

	char* name;
	int skipIfNoChange = 0;
	if (!PyArg_ParseTuple( args, "s|i", &name, &skipIfNoChange ))
	{
		PyErr_SetString( PyExc_TypeError, "ModelEditor.addUndoBarrier() "
			"expects a string and an optional int" );
		return NULL;
	}

	// Add the undo barrier
	UndoRedo::instance().barrier( name, (skipIfNoChange != 0) );

	Py_RETURN_NONE;
}
PY_MODULE_FUNCTION( addUndoBarrier, ModelEditor )


/*~ function ModelEditor.saveOptions
 *	@components{ modeleditor }
 * 
 *	This function saves the options file.
 *
 *	@param filename The name of the file to save the options to.
 *
 *	@return Returns True if the save was successful, False otherwise.
 */
static PyObject * py_saveOptions( PyObject * args )
{
	BW_GUARD;

	char * filename = NULL;

	if (!PyArg_ParseTuple( args, "|s", &filename ))
	{
		PyErr_SetString( PyExc_TypeError, "ModelEditor.saveOptions() "
			"expects an optional string argument." );
		return NULL;
	}

	if (filename)
		return Script::getData( Options::save( filename ) );
	else
		return false;
}
PY_MODULE_FUNCTION( saveOptions, ModelEditor )

/*~ function ModelEditor.showPanel
 *	@components{ modeleditor }
 *
 *	This function shows or hides a Tool Panel.
 *
 * @param panel The name of the panel to show/hide.
 * @param show 1 to show the panel, and 0 to hide the panel.
 */
static PyObject * py_showPanel( PyObject * args )
{
	BW_GUARD;

	char * panel = NULL;
	int show = -1;
	if ( !PyArg_ParseTuple( args, "si", &panel, &show ) )
	{
		PyErr_SetString( PyExc_TypeError, "ModelEditor.showPanel() "
			"expects a string and an int argument." );
		return NULL;
	}

	if ( (panel != NULL) && (show != -1) )
		PanelManager::instance().showPanel( (BW::string)(panel), show );

	Py_RETURN_NONE;
}
PY_MODULE_FUNCTION( showPanel, ModelEditor )


/*~ function ModelEditor.isPanelVisible
 *	@components{ modeleditor }
 *
 *	This function returns whether a tool panel is visible.
 *
 *	@param panel The name of the panel to query if it is visible.
 *
 *	@return Returns True (1) if the panel is visible, False (0) otherwise.
 */
static PyObject * py_isPanelVisible( PyObject * args )
{
	BW_GUARD;

	char * panel = NULL;
	if ( !PyArg_ParseTuple( args, "s", &panel ) )
	{
		PyErr_SetString( PyExc_TypeError, "ModelEditor.isPanelVisible() "
			"expects a string argument." );
		return NULL;
	}

	if ( panel )
		return PyInt_FromLong( PanelManager::instance().isPanelVisible( (BW::string)(panel) ) );

	Py_RETURN_NONE;
}
PY_MODULE_FUNCTION( isPanelVisible, ModelEditor )

/*~ function ModelEditor.addItemToHistory
 *	@components{ modeleditor }
 *
 *	Adds the specified item to the user item history.
 *
 *	@param filePath The filepath to the item to be added to the user item history.
 */
static PyObject * py_addItemToHistory( PyObject * args )
{
	BW_GUARD;

	char* filePath = NULL;
	if ( !PyArg_ParseTuple( args, "s", &filePath ) )
	{
		PyErr_SetString( PyExc_TypeError, "ModelEditor.addItemToHistory()"
			"expects a string argument." );
		return NULL;
	}

	if ( filePath )
	{
		PanelManager::instance().ualAddItemToHistory( filePath );
	}

	Py_RETURN_NONE;
}
PY_MODULE_FUNCTION( addItemToHistory, ModelEditor )

/*~ function ModelEditor.makeThumbnail
 *	@components{ modeleditor }
 *
 *	This function makes a thumbnail picture for the model.
 *
 *	@param filePath The filepath of the thumbnail to make. If not given
 *		   then the filepath of the model is used.
 */
static PyObject * py_makeThumbnail( PyObject * args )
{
	BW_GUARD;

	const char* filePath = NULL;

	if ( !PyArg_ParseTuple( args, "|s", &filePath ) )
	{
		PyErr_SetString( PyExc_TypeError, "ModelEditor.makeThumbnail()"
			"expects an optional string argument." );
		return NULL;
	}

	if (filePath == NULL)
	{
		filePath = MeApp::instance().mutant()->modelName().c_str();
	}

	if ( strcmp( filePath, "") )
	{
		if (MeModule::instance().renderThumbnail( filePath ))
		{
			ME_INFO_MSGW( Localise(L"MODELEDITOR/APP/ME_SCRIPTER/MAKE_THUMBNAIL",  MeApp::instance().mutant()->modelName() ) );
		}
	}
	else
	{
		ME_WARNING_MSGW( Localise(L"MODELEDITOR/APP/ME_SCRIPTER/THUMBNAIL_ERROR") );
	}


	Py_RETURN_NONE;
}
PY_MODULE_FUNCTION( makeThumbnail, ModelEditor )

/**
 *  BW script interface.
 */

bool Scripter::init(DataSectionPtr pDataSection )
{
	BW_GUARD;

	// Particle Systems are creatable from Python code
	MF_VERIFY( ParticleSystemManager::init() );

	PyImportPaths importPaths;
	importPaths.addResPath( "resources/scripts" );
	importPaths.addResPath( EntityDef::Constants::entitiesEditorPath() );
	importPaths.addResPath( EntityDef::Constants::entitiesClientPath() );
	importPaths.addResPath( EntityDef::Constants::userDataObjectsEditorPath() );

	// Call the general init function
	if (!Script::init( importPaths, "editor" ))
	{
		ERROR_MSG( "Scripter::init: Failed to init Script.\n" );
		return false;
	}

	Options::initLoggers();

	if (!MaterialKinds::init())
	{
		ERROR_MSG( "Scripter::init: Failed to initialise MaterialKinds\n" );
		return false;
	}

	PyObject * pInit =
		PyObject_GetAttrString( PyImport_AddModule("keys"), "init" );
	if (pInit != NULL)
	{
		PyRun_SimpleString( PyString_AsString(pInit) );
	}
	PyErr_Clear();

	Personality::import( Options::getOptionString( "personality", "Personality" ) );

	return true;
}


void Scripter::fini()
{
	BW_GUARD;

	MaterialKinds::fini();
	// Fini scripts
	Script::fini();
	ParticleSystemManager::fini();
}


/*~ function ModelEditor.capturePanel
 *	@components{ modeleditor }
 * 
 *	This function captures the image of the tool panel.
 *
 *	@param	panelName	The name of the tool panel.
 *	@param	fileName	The name of the file to store the image.
 *
 *	@return	Returns True if the panel has been captured, False otherwise.
 */
static bool capturePanel( const BW::string& panelName, 
						  const BW::string& fileName )
{
	BW_GUARD;

	BW::wstring panelID = PanelManager::instance().getPanelID( panelName );
	if (panelID.empty())
	{
		return false;
	}

	GUITABS::Content* pContent = 
		PanelManager::instance().panels().getContent( panelID );
	if ( !pContent )
	{
		return false;
	}

	CWnd* pWnd = pContent->getCWnd();
	if ( !pWnd )
	{
		return false;
	}

	return Utilities::captureWindow( *pWnd, fileName );
}
PY_AUTO_MODULE_FUNCTION( RETDATA, capturePanel, ARG( BW::string, ARG( BW::string, END ) ), ModelEditor );


BW_END_NAMESPACE

