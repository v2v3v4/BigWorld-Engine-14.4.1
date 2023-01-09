#include "pch.hpp"
#include "worldeditor/scripting/world_editor_script.hpp"
#ifndef CODE_INLINE
#include "worldeditor/scripting/world_editor_script.ipp"
#endif
#include "worldeditor/framework/world_editor_app.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "worldeditor/world/editor_chunk_overlapper.hpp"
#include "worldeditor/world/editor_chunk_cache.hpp"
#include "worldeditor/misc/sync_mode.hpp"
#include "worldeditor/misc/world_editor_camera.hpp"
#include "worldeditor/misc/progress_bar_helper.hpp"
#include "worldeditor/gui/dialogs/resize_maps_dlg.hpp"
#include "worldeditor/gui/pages/panel_manager.hpp"
#include "worldeditor/gui/dialogs/string_input_dlg.hpp"
#include "worldeditor/terrain/terrain_locator.hpp"
#include "worldeditor/editor/snaps.hpp"
#include "worldeditor/project/space_helpers.hpp"
#include "common/base_camera.hpp"
#include "common/compile_time.hpp"
#include "common/tools_common.hpp"
#include "appmgr/commentary.hpp"
#include "appmgr/module_manager.hpp"
#include "appmgr/options.hpp"
#include "chunk/chunk.hpp"
#include "chunk/chunk_grid_size.hpp"
#include "chunk/chunk_manager.hpp"
#include "chunk/chunk_obstacle.hpp"
#include "chunk/chunk_space.hpp"
#include "chunk/geometry_mapping.hpp"
#include "gizmo/tool_manager.hpp"
#include "gizmo/undoredo.hpp"
#include "gizmo/general_properties.hpp"
#include "gizmo/current_general_properties.hpp"
#include "gizmo/item_view.hpp"
#include "input/input.hpp"
#include "particle/particle_system_manager.hpp"
#include "pyscript/py_import_paths.hpp"
#include "pyscript/res_mgr_script.hpp"
#include "pyscript/script.hpp"
#include "pyscript/script_math.hpp"
#include "resmgr/string_provider.hpp"
#include "entitydef/constants.hpp"
#include "romp/console_manager.hpp"
#include "romp/fog_controller.hpp"
#include "romp/progress.hpp"
#include "romp/weather.hpp"
#include "romp/xconsole.hpp"
#include "terrain/terrain2/terrain_block2.hpp"
#include "terrain/terrain_hole_map.hpp"
#include "terrain/terrain_settings.hpp"
#include "terrain/terrain_texture_layer.hpp"
#include "tools/common/utilities.hpp"
#include "cstdmf/debug.hpp"
#include "cstdmf/memory_load.hpp"
#include <queue>

#include "physics2/material_kinds.hpp"


DECLARE_DEBUG_COMPONENT2( "Script", 0 )

BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: Include doco for other modules
// -----------------------------------------------------------------------------

/*~ module BigWorld
 *  @components{ worldeditor }
 */

/*~ module GUI
 *  @components{ worldeditor }
 */

/*~ module PostProcessing
 *  @components{ worldeditor }
 */


// -----------------------------------------------------------------------------
// Section: 'WorldEditor' Module
// -----------------------------------------------------------------------------


/*~ function WorldEditor.exit
 *	@components{ worldeditor }
 *
 *	This function closes WorldEditor.
 */
static PyObject *py_exit(PyObject * /*args*/)
{
	BW_GUARD;

    AfxGetApp()->GetMainWnd()->PostMessage( WM_COMMAND, ID_APP_EXIT );

	Py_RETURN_NONE;
}
PY_MODULE_FUNCTION(exit, WorldEditor)

/*~ function WorldEditor.isKeyDown
 *	@components{ worldeditor }
 *	
 *	This function allows the script to check if a particular key has
 *	been pressed and is currently still down. The term 'key' is used here to refer
 *	to any control with an up/down status; it can refer to the keys of a
 *	keyboard, the buttons of a mouse or even that of a joystick. The complete
 *	list of keys recognised by the client can be found in the Keys module,
 *	defined in keys.py.
 *
 *	The return value is zero if the key is not being held down, and a non-zero
 *	value if it is being held down.
 *
 *	@param key	An integer value indexing the key of interest.
 *
 *	@return True (1) if the key is down, false (0) otherwise.
 *
 *	Code Example:
 *	@{
 *	if WorldEditor.isKeyDown( Keys.KEY_ESCAPE ):
 *	@}
 */
static PyObject * py_isKeyDown( PyObject * args )
{
	BW_GUARD;

	int	key;
	if (!PyArg_ParseTuple( args, "i", &key ))
	{
		PyErr_SetString( PyExc_TypeError,
			"py_isKeyDown: Argument parsing error." );
		return NULL;
	}

	return PyInt_FromLong( InputDevices::isKeyDown( (KeyCode::Key)key ) );
}
PY_MODULE_FUNCTION( isKeyDown, WorldEditor )


/*~ function WorldEditor.isCapsLockOn
 *	@components{ worldeditor }
 *
 *	This function returns whether CapsLock is on.
 *
 * @return Returns True (1) if Caps Lock is on, False (0) otherwise.
 */
static PyObject * py_isCapsLockOn( PyObject * args )
{
	BW_GUARD;

	return PyInt_FromLong(
		(::GetKeyState( VK_CAPITAL ) & 0x0001) == 0 ? 0 : 1 );
}
PY_MODULE_FUNCTION( isCapsLockOn, WorldEditor )


/*~ function WorldEditor.stringToKey
 *	@components{ worldeditor }
 *
 *	This function converts the name of a key to its corresponding
 *	key index as used by the 'isKeyDown' method. The string names
 *	for a key can be found in the keys.py file. If the name supplied is not on
 *	the list defined, the value returned is zero, indicating an error. This
 *	method has a inverse method, 'keyToString' which does the exact opposite.
 *
 *	@param string	A string argument containing the name of the key.
 *
 *	@return An integer value for the key with the supplied name.
 *
 *	Code Example:
 *	@{
 *	if BigWorld.isKeyDown( WorldEditor.stringToKey( "KEY_ESCAPE" ) ):
 *	@}
 */
static PyObject * py_stringToKey( PyObject * args )
{
	BW_GUARD;

	char * str;
	if (!PyArg_ParseTuple( args, "s", &str ))
	{
		PyErr_SetString( PyExc_TypeError,
			"py_stringToKey: Argument parsing error." );
		return NULL;
	}

	return PyInt_FromLong( KeyCode::stringToKey( str ) );
}
PY_MODULE_FUNCTION( stringToKey, WorldEditor )


/*~ function WorldEditor.keyToString
 *	@components{ worldeditor }
 *
 *	The 'keyToString' method converts from a key index to its corresponding
 *	string name. The string names returned by the integer index can be found in
 *	the keys.py file. If the index supplied is out of bounds, an empty string
 *	will be returned.
 *
 *	@param key	An integer representing a key index value.
 *
 *	@return A string containing the name of the key supplied.
 *
 *	Code Example:
 *	@{
 *	print WorldEditor.keyToString( key ), "pressed."
 *	@}
 */
static PyObject * py_keyToString( PyObject * args )
{
	BW_GUARD;

	int	key;
	if (!PyArg_ParseTuple( args, "i", &key ))
	{
		PyErr_SetString( PyExc_TypeError,
			"py_keyToString: Argument parsing error." );
		return NULL;
	}

	return PyString_FromString( KeyCode::keyToString(
		(KeyCode::Key) key ) );
}
PY_MODULE_FUNCTION( keyToString, WorldEditor )


/*~ function WorldEditor.axisValue
 *	@components{ worldeditor }
 *
 *	This function returns the value of the given joystick axis.
 *
 *	@param Axis The given joystick axis to get the joystick axis from.
 *
 *	@return The value of the given joystick axis.
 */
static PyObject * py_axisValue( PyObject * args )
{
	BW_GUARD;

	int	axis;
	if (!PyArg_ParseTuple( args, "i", &axis ))
	{
		PyErr_SetString( PyExc_TypeError,
			"py_axisValue: Argument parsing error." );
		return NULL;
	}

	Joystick::Axis a = InputDevices::joystick().getAxis( (AxisEvent::Axis)axis );

	return PyFloat_FromDouble( a.value() );
}

PY_MODULE_FUNCTION( axisValue, WorldEditor )


/*~ function WorldEditor.axisDirection
 *	@components{ worldeditor }
 *
 *	This function returns the direction the specified joystick is pointing in.
 *
 *	The return value indicates which direction the joystick is facing, as
 *	follows:
 *
 *	@{
 *	- 0 down and left
 *	- 1 down
 *	- 2 down and right
 *	- 3 left
 *	- 4 centred
 *	- 5 right
 *	- 6 up and left
 *	- 7 up
 *	- 8 up and right
 *	@}
 *
 *	@param	axis	This is one of AXIS_LX, AXIS_LY, AXIS_RX, AXIS_RY, with the
 *					first letter being L or R meaning left thumbstick or right
 *					thumbstick, the second, X or Y being the direction.
 *
 *	@return			An integer representing the direction of the specified
 *					thumbstick, as listed above.
 */
static PyObject * py_axisDirection( PyObject * args )
{
	BW_GUARD;

	int	axis;
	if (!PyArg_ParseTuple( args, "i", &axis ))
	{
		PyErr_SetString( PyExc_TypeError,
			"py_axisPosition: Argument parsing error." );
		return NULL;
	}

	int direction = InputDevices::joystick().stickDirection( (AxisEvent::Axis)axis );

	return PyInt_FromLong( direction );
}
PY_MODULE_FUNCTION( axisDirection, WorldEditor )


/*~ function WorldEditor.addCommentaryMsg
 *	@components{ worldeditor }
 *
 *	This function adds a message to the	Commentary Console.
 *
 *	@param CommentaryMsg The message to display in the Commentary Console.
 */
static PyObject * py_addCommentaryMsg( PyObject * args )
{
	BW_GUARD;

	int id = Commentary::COMMENT;
	char* tag;

	if (!PyArg_ParseTuple( args, "s|i", &tag, &id ))
	{
		PyErr_SetString( PyExc_TypeError, "py_addCommentaryMsg: Argument parsing error." );
		return NULL;
	}

	if ( stricmp( tag, "" ) )
	{
		Commentary::instance().addMsg( BW::string( tag ), id );

		if (tag[0] == '`')
		{
			dprintf( "Commentary: %s\n", LocaliseUTF8( tag + 1 ).c_str() );
		}
		else
		{
			dprintf( "Commentary: %s\n", tag );
		}
	}
	else
	{
		Commentary::instance().addMsg( BW::string( "NULL" ), Commentary::WARNING );
	}

	Py_RETURN_NONE;
}
PY_MODULE_FUNCTION( addCommentaryMsg, WorldEditor )


/*~ function WorldEditor.push
 *	@components{ worldeditor }
 *
 *	This function pushes a module onto the application's module stack.
 *
 *	@param Module	The name of the module to push onto the application's module stack.
 */
static PyObject * py_push( PyObject * args )
{
	BW_GUARD;

	char* id;

	if (!PyArg_ParseTuple( args, "s", &id ))
	{
		PyErr_SetString( PyExc_TypeError, "py_push: Argument parsing error." );
		return NULL;
	}

	ModuleManager::instance().push( BW::string(id) );

	Py_RETURN_NONE;
}
PY_MODULE_FUNCTION( push, WorldEditor )


/*~ function WorldEditor.pop
 *	@components{ worldeditor }
 *
 *	This function pops the current module from the application's module stack.
 */
static PyObject * py_pop( PyObject * args )
{
	BW_GUARD;

	ModuleManager::instance().pop();

	Py_RETURN_NONE;
}
PY_MODULE_FUNCTION( pop, WorldEditor )


/*~ function WorldEditor.pushTool
 *	@components{ worldeditor }
 *
 *	This function pushes a tool onto WorldEditor's tool stack.
 *
 *	@param tool	The tool to push onto WorldEditor's tool stack.
 */
static PyObject * py_pushTool( PyObject * args )
{
	BW_GUARD;

	PyObject* pTool;

	if (!PyArg_ParseTuple( args, "O", &pTool ) ||
		!Tool::Check( pTool ))
	{
		PyErr_SetString( PyExc_TypeError,
			"py_pushTool: Expected a Tool." );
		return NULL;
	}

	ToolManager::instance().pushTool( static_cast<Tool*>( pTool ) );

	Py_RETURN_NONE;
}
PY_MODULE_FUNCTION( pushTool, WorldEditor )


/*~ function WorldEditor.popTool
 *	@components{ worldeditor }
 *
 *	This function pops the current tool from WorldEditor's tool stack.
 */
static PyObject * py_popTool( PyObject * args )
{
	BW_GUARD;

	ToolManager::instance().popTool();

	Py_RETURN_NONE;
}
PY_MODULE_FUNCTION( popTool, WorldEditor )


/*~ function WorldEditor.tool
 *	@components{ worldeditor }
 *
 *	This function gets the current tool from WorldEditor's tool stack.
 *
 *	@return A reference to the current tool from WorldEditor's tool stack.
 */
static PyObject * py_tool( PyObject * args )
{
	BW_GUARD;

	ToolPtr spTool = ToolManager::instance().tool();

	if (spTool)
	{
		Py_INCREF( spTool.getObject() );
		return spTool.getObject();
	}
	else
	{
		Py_RETURN_NONE;
	}
}
PY_MODULE_FUNCTION( tool, WorldEditor )


/*~ function WorldEditor.undo
 *	@components{ worldeditor }
 *
 *	This function undoes the most recent operation, returning
 *	its description. If it is passed a positive integer argument,
 *	then it just returns the description for that level of the
 *	undo stack and doesn't actually undo anything.
 *	If there is no undo level, an empty string is returned.
 *
 *	@param undoLevel	The level of the undo stack to return the undo 
 *						description to. If not supplied, then the most recent
 *						operation will be undone.
 *
 *	@return	The description of the undo operation at the given level of the
 *			undo stack. If no description is found then an empty string is returned.
 */
static PyObject * py_undo( PyObject * args )
{
	BW_GUARD;

    CWaitCursor waitCursor;

	int forStep = -1;
	if (!PyArg_ParseTuple( args, "|i", &forStep ))
	{
		PyErr_SetString( PyExc_TypeError, "WorldEditor.undo() "
			"expects an optional integer argument" );
		return NULL;
	}

	BW::string what = UndoRedo::instance().undoInfo( max(0,forStep) );

	if (forStep < 0) UndoRedo::instance().undo();

	return Script::getData( what );
}
PY_MODULE_FUNCTION( undo, WorldEditor )

/*~ function WorldEditor.redo
 *	@components{ worldeditor }
 *
 *	This function works exactly like undo, only it redoes the last undo operation.
 *
 *	@see WorldEditor.undo
 */
static PyObject * py_redo( PyObject * args )
{
	BW_GUARD;

    CWaitCursor waitCursor;

	int forStep = -1;
	if (!PyArg_ParseTuple( args, "|i", &forStep ))
	{
		PyErr_SetString( PyExc_TypeError, "WorldEditor.redo() "
			"expects an optional integer argument" );
		return NULL;
	}

	BW::string what = UndoRedo::instance().redoInfo( max(0,forStep) );

	if (forStep < 0) UndoRedo::instance().redo();

	return Script::getData( what );
}
PY_MODULE_FUNCTION( redo, WorldEditor )

/*~ function WorldEditor.addUndoBarrier
 *	@components{ worldeditor }
 *
 *	Adds an undo/redo barrier with the given name.
 *
 *	@param name The name of the undo/redo barrier to add.
 *	@param skipIfNoChange	An optional int that specifies whether to force
 *							a barrier to be added even if no changes have been made.
 *							Defaults value is False (0), otherwise setting True (1) will not add 
 *							a barrier if no changes have been made.
 */
static PyObject * py_addUndoBarrier( PyObject * args )
{
	BW_GUARD;

	char* name;
	int skipIfNoChange = 0;
	if (!PyArg_ParseTuple( args, "s|i", &name, &skipIfNoChange ))
	{
		PyErr_SetString( PyExc_TypeError, "WorldEditor.addUndoBarrier() "
			"expects a string and an optional int" );
		return NULL;
	}

	// Add the undo barrier
	UndoRedo::instance().barrier( name, (skipIfNoChange != 0) );

	Py_RETURN_NONE;
}
PY_MODULE_FUNCTION( addUndoBarrier, WorldEditor )

/*~ function WorldEditor.clearUndoRedo
 *	@components{ worldeditor }
 *
 *	Clears the undo/redo lists.
 */
static void clearUndoRedo()
{
	UndoRedo::instance().clear();
}
PY_AUTO_MODULE_FUNCTION( RETVOID, clearUndoRedo, END, WorldEditor )

/*~ function WorldEditor.saveOptions
 *	@components{ worldeditor }
 *
 *	This function saves the options file.
 *
 *	@param filename The name of the file to save the options file as. If
 *					no name is given then it will overwrite the current 
 *					options file.
 *	
 *	@return Returns True if the save operation was successful, False otherwise.
 */
static PyObject * py_saveOptions( PyObject * args )
{
	BW_GUARD;

	char * filename = NULL;

	if (!PyArg_ParseTuple( args, "|s", &filename ))
	{
		PyErr_SetString( PyExc_TypeError, "WorldEditor.saveOptions() "
			"expects an optional string argument." );
		return NULL;
	}

	return Script::getData( Options::save( filename ) );
}
PY_MODULE_FUNCTION( saveOptions, WorldEditor )


/*~ function WorldEditor.camera
 *	@components{ worldeditor }
 *
 *	This function gets a WorldEditor camera.
 *
 *	For more information see the BaseCamera class.
 *	
 *	@param cameraType	The type of camera to return. If cameraType = 0 then the
 *						mouseLook camera is returned; if cameraType = 1 then the
 *						orthographic camera is returned; if cameraType is not given
 *						then the current camera is returned.
 *
 *	@return Returns a reference to a camera which corresponds to the supplied cameraType
 *			parameter. If cameraType is not supplied then the current camera is returned.
 */
static PyObject * py_camera( PyObject * args )
{
	BW_GUARD;

	int cameraType = -1;
	if (!PyArg_ParseTuple( args, "|i", &cameraType ))
	{
		PyErr_SetString( PyExc_TypeError, "WorldEditor.camera() "
			"expects an optional int argument." );
		return NULL;
	}

	if (cameraType == -1)
	{
		// if no camera specified, return the current camera
		return Script::getData( &(WorldEditorCamera::instance().currentCamera()) );
	}
	else
	{
		// else return the camera specified (only one type of each camera exists
		return Script::getData( &(WorldEditorCamera::instance().camera((WorldEditorCamera::CameraType)cameraType)) );
	}
}
PY_MODULE_FUNCTION( camera, WorldEditor )


/*~ function WorldEditor.changeToCamera
 *	@components{ worldeditor }
 *
 *	This function changes the current camera to the specified cameraType.
 *	
 *	@param cameraType	The cameraType to change the current camera to. If
 *						cameraType = 0 then the mouseLook camera is used;
 *						if cameraType = 1 then the orthographic camera is used.
 */
static PyObject * py_changeToCamera( PyObject * args )
{
	BW_GUARD;

	int cameraType = -1;
	if (!PyArg_ParseTuple( args, "i", &cameraType ))
	{
		PyErr_SetString( PyExc_TypeError, "WorldEditor.camera() "
			"expects an int argument." );
		return NULL;
	}

	if (cameraType != -1)
		WorldEditorCamera::instance().changeToCamera((WorldEditorCamera::CameraType)cameraType);

	Py_RETURN_NONE;
}
PY_MODULE_FUNCTION( changeToCamera, WorldEditor )


/*~ function WorldEditor.snapCameraToTerrain
 *	@components{ worldeditor }
 *
 *	This function snaps the camera to the ground.
 */
static PyObject * py_snapCameraToTerrain( PyObject * args )
{
	BW_GUARD;

	BaseCamera& cam = WorldEditorCamera::instance().currentCamera();

	Matrix view = cam.view();
	view.invert();
	Vector3 camPos( view.applyToOrigin() );

	ChunkSpacePtr space = ChunkManager::instance().cameraSpace();
	if ( space )
	{
		ClosestTerrainObstacle terrainCallback;

		// magic numbers are defined here:
		const float EXTENT_RANGE	= 5000.0f;
		const float CAM_RANGE		= 5000.0f;

		// start with the camera's vertical position at 0m
		camPos.y = 0;
		// cycle incrementing the camera's vertical position until a collision is 
		// found, or until the camera's maximum range is reached (set very high!).
		while (!terrainCallback.collided())
		{
			Vector3 extent = camPos + ( Vector3( 0, -EXTENT_RANGE, 0.f ) );

			space->collide( 
				camPos,
				extent,
				terrainCallback );

			// clamp the camera max height to something 'sensible'
			if ( camPos.y >= CAM_RANGE )
				break;

			if (!terrainCallback.collided())
			{
				// drop the camera from higher if no collision is detected
				camPos.y += 200;
			}
		}

		if (terrainCallback.collided())
		{
			camPos = camPos +
					( Vector3(0,-1,0) * terrainCallback.dist() );
			view.translation( camPos +
				Vector3( 0,
				(float)Options::getOptionFloat( "graphics/cameraHeight", 2.f ),
				0 ) );
			view.invert();
			cam.view( view );
		}
	}	

	Py_RETURN_NONE;
}
PY_MODULE_FUNCTION( snapCameraToTerrain, WorldEditor )

/*~ function WorldEditor.enterPlayerPreviewMode
 *	@components{ worldeditor }
 *
 *	This function enables the player preview mode view.
 */
static PyObject * py_enterPlayerPreviewMode( PyObject * args )
{
	BW_GUARD;

	WorldManager::instance().setPlayerPreviewMode( true );
	Py_RETURN_NONE;
}
PY_MODULE_FUNCTION( enterPlayerPreviewMode, WorldEditor )

/*~ function WorldEditor.leavePlayerPreviewMode
 *	@components{ worldeditor }
 *
 *	This function disables the player preview mode view.
 */
static PyObject * py_leavePlayerPreviewMode( PyObject * args )
{
	BW_GUARD;

	WorldManager::instance().setPlayerPreviewMode( false );
	Py_RETURN_NONE;
}
PY_MODULE_FUNCTION( leavePlayerPreviewMode, WorldEditor )

/*~ function WorldEditor.isInPlayerPreviewMode
 *	@components{ worldeditor }
 *
 *	This function asks WorldEditor if we are in playerPreviewMode
 *
 *	@return Returns True (1) if in player preview mode, False (0) otherwise.
 */
static PyObject * py_isInPlayerPreviewMode( PyObject * args )
{
	BW_GUARD;

	return PyInt_FromLong( WorldManager::instance().isInPlayerPreviewMode() );
}
PY_MODULE_FUNCTION( isInPlayerPreviewMode, WorldEditor )

/*~ function WorldEditor.fudgeOrthographicMode
 *	@components{ worldeditor }
 *
 *	This is a temporary function that simply makes the camera
 *	go top-down.
 */
static PyObject * py_fudgeOrthographicMode( PyObject * args )
{
	BW_GUARD;

	float height = -31000.f;
	float lag = 5.f;

	if ( !PyArg_ParseTuple( args, "|ff", &height, &lag ) )
	{
		PyErr_SetString( PyExc_TypeError, "WorldEditor.fudgeOrthographicMode() "
			"expects two optional float arguments - height and lag." );
		return NULL;
	}


	BaseCamera& cam = WorldEditorCamera::instance().currentCamera();

	Matrix view = cam.view();
	view.invert();
	Vector3 camPos( view.applyToOrigin() );

	if ( height > -30000.f )
	{
		if ( camPos.y != height )
		{
			float newCamY = ( ( (camPos.y*lag) + height ) / (lag+1.f) );
			float dy = ( newCamY - camPos.y ) * WorldManager::instance().dTime();
			camPos.y += dy;
		}
	}

	Matrix xform = cam.view();
	xform.setRotateX( 0.5f*MATH_PI ); 
	xform.postTranslateBy( camPos );
	xform.invert();
	cam.view(xform);

	Py_RETURN_NONE;
}
PY_MODULE_FUNCTION( fudgeOrthographicMode, WorldEditor )



/*~ function WorldEditor.unloadChunk
 *	@components{ worldeditor }
 *
 *	This function unloads the chunk under the current tool's locator.
 *	This has the effect of clearing all changes made to the chunk since the 
 *	last save.
 */
static PyObject * py_unloadChunk( PyObject * args )
{
	BW_GUARD;

	ToolPtr spTool = ToolManager::instance().tool();

	if (spTool && spTool->locator())
	{
		Vector3 cen = spTool->locator()->transform().applyToOrigin();
		Chunk * pChunk = ChunkManager::instance().cameraSpace()->
			findChunkFromPoint( cen );
		if (pChunk != NULL)
		{
			pChunk->unbind( false );
			pChunk->unload();

			Py_RETURN_NONE;
		}
	}

	PyErr_SetString( PyExc_ValueError, "WorldEditor.unloadChunk() "
		"could not find the chunk to unload." );
	return NULL;
}
PY_MODULE_FUNCTION( unloadChunk, WorldEditor )
PY_MODULE_FUNCTION_ALIAS( unloadChunk, ejectChunk, WorldEditor )


/*~ function WorldEditor.moveGroupTo
 *	@components{ worldeditor }
 *
 *	Move all current position properties to the given locator.
 *	It does not add an undo barrier, it is up to the Python code to do that.
 *
 *	@param ToolLocator	The ToolLocator object to move the current position properties to.
 */
static PyObject * py_moveGroupTo( PyObject * args )
{
	BW_GUARD;

	// get args
	PyObject * pPyLoc;
	if (!PyArg_ParseTuple( args, "O", &pPyLoc ) ||
		!ToolLocator::Check( pPyLoc ))
	{
		PyErr_SetString( PyExc_ValueError, "WorldEditor.moveGroupTo() "
			"expects a ToolLocator" );
		return NULL;
	}

	ToolLocator* locator = static_cast<ToolLocator*>( pPyLoc );

	//Move all group objects relatively by an offset.
	//The offset is a relative, snapped movement.
	Vector3 centrePos = CurrentPositionProperties::averageOrigin();
	Vector3 locPos = locator->transform().applyToOrigin();
	Vector3 newPos = locPos - centrePos;
	SnapProvider::instance()->snapPositionDelta( newPos );

	Matrix offset;
	offset.setTranslate( newPos );

	BW::vector<GenPositionProperty*> props = CurrentPositionProperties::properties();
	for (BW::vector<GenPositionProperty*>::iterator i = props.begin(); i != props.end(); ++i)
	{
		Matrix m;
		(*i)->pMatrix()->recordState();
		(*i)->pMatrix()->getMatrix( m );

		m.postMultiply( offset );

		if ( WorldManager::instance().terrainSnapsEnabled() )
		{
			Vector3 pos( m.applyToOrigin() );
			//snap to terrain only
			pos = Snap::toGround( pos );
			m.translation( pos );
		}
		else if ( WorldManager::instance().obstacleSnapsEnabled() )
		{
			Vector3 normalOfSnap = SnapProvider::instance()->snapNormal( m.applyToOrigin() );
			Vector3 yAxis( 0, 1, 0 );
			yAxis = m.applyVector( yAxis );

			Vector3 binormal = yAxis.crossProduct( normalOfSnap );

			normalOfSnap.normalise();
			yAxis.normalise();
			binormal.normalise();

			float angle = acosf( Math::clamp(-1.0f, yAxis.dotProduct( normalOfSnap ), +1.0f) );

			Quaternion q( binormal.x * sinf( angle / 2.f ),
				binormal.y * sinf( angle / 2.f ),
				binormal.z * sinf( angle / 2.f ),
				cosf( angle / 2.f ) );

			q.normalise();

			Matrix rotation;
			rotation.setRotate( q );

			Vector3 pos( m.applyToOrigin() );

			m.translation( Vector3( 0.f, 0.f, 0.f ) );
			m.postMultiply( rotation );

			m.translation( pos );
		}

		Matrix worldToLocal;
		(*i)->pMatrix()->getMatrixContextInverse( worldToLocal );

		m.postMultiply( worldToLocal );

		(*i)->pMatrix()->setMatrix( m );
		(*i)->pMatrix()->commitState( false, false );
	}

	Py_RETURN_NONE;
}
PY_MODULE_FUNCTION( moveGroupTo, WorldEditor )

/*~ function WorldEditor.showChunkReport
 *	@components{ worldeditor }
 *
 *	This function displays the chunk report of the selected chunk
 *
 *	@param chunk A ChunkItemRevealer object to the selected chunk.
 */
static PyObject * py_showChunkReport( PyObject * args )
{
	BW_GUARD;

	// get args
	PyObject * pPyRev;
	if (!PyArg_ParseTuple( args, "O", &pPyRev ) ||
		!ChunkItemRevealer::Check( pPyRev ))
	{
		PyErr_SetString( PyExc_ValueError, "WorldEditor.showChunkReport() "
			"expects a ChunkItemRevealer" );
		return NULL;
	}

	ChunkItemRevealer* pRevealer = static_cast<ChunkItemRevealer*>( pPyRev );

	ChunkItemRevealer::ChunkItems items;
	pRevealer->reveal( items );

	uint modelCount = 0;

	ChunkItemRevealer::ChunkItems::iterator i = items.begin();
	for (; i != items.end(); ++i)
	{
		ChunkItemPtr pItem = *i;
		Chunk* pChunk = pItem->chunk();

		if (pChunk)
		{
			BW::vector<DataSectionPtr>	modelSects;
			EditorChunkCache::instance( *pChunk ).pChunkSection()->openSections( "model", modelSects );

			modelCount += (int) modelSects.size();
		}
	}

	char buf[512];
	bw_snprintf( buf, sizeof(buf), "%d models in selection\n", modelCount );

	Commentary::instance().addMsg( buf );

	Py_RETURN_NONE;
}
PY_MODULE_FUNCTION( showChunkReport, WorldEditor )


/*~ function WorldEditor.setToolMode
 *	@components{ worldeditor }
 *
 *	This function sets the current WorldEditor tool mode.
 *
 *	@param mode The name of the tool mode to set.
 */
static PyObject * py_setToolMode( PyObject * args )
{
	BW_GUARD;

	char* mode = 0;
	if ( !PyArg_ParseTuple( args, "s", &mode ) )
	{
		PyErr_SetString( PyExc_TypeError, "WorldEditor.setToolMode() "
			"expects a string argument." );
		return NULL;
	}

	if ( mode )
		PanelManager::instance().setToolMode( bw_utf8tow( mode ) );

	Py_RETURN_NONE;
}
PY_MODULE_FUNCTION( setToolMode, WorldEditor )


/*~ function WorldEditor.showPanel
 *	@components{ worldeditor }
 *
 *	This function shows or hides a Tool Panel.
 *
 *	@param panel	The name of the panel to show/hide.
 *	@param show		If show = 0 then the panel will be hidden, otherwise it will be shown.
 */
static PyObject * py_showPanel( PyObject * args )
{
	BW_GUARD;

	char* nmode = NULL;
	wchar_t * wmode = NULL; 
	int show = -1;

	// the unicode version has to go first, since the string one will try to
	// encode it into a normal string.
	if ( PyArg_ParseTuple( args, "ui", &wmode, &show ) )
	{
		if ( wmode && show != -1 )
		{
			PanelManager::instance().showPanel( wmode, show );
		}
		Py_RETURN_NONE;
	}
	else if ( PyArg_ParseTuple( args, "si", &nmode, &show ) )
	{
		if ( nmode && show != -1 )
		{
			BW::wstring lwmode;
			bw_utf8tow( nmode, lwmode );
			PanelManager::instance().showPanel( lwmode, show );
		}
		Py_RETURN_NONE;
	}
	else
	{
		PyErr_SetString( PyExc_TypeError, "WorldEditor.showPanel() "
			"expects one string and one int arguments." );
		return NULL;
	}


}
PY_MODULE_FUNCTION( showPanel, WorldEditor )


/*~ function WorldEditor.isPanelVisible
 *	@components{ worldeditor }
 *
 *	This function checks whether a given panel is visible.
 *
 *	@param panel The name of the panel to query whether it is visible.
 *
 *	@return Returns True (1) if the panel is visible, False (0) otherwise.
 */
static PyObject * py_isPanelVisible( PyObject * args )
{
	BW_GUARD;

	char* mode = 0;
	if ( !PyArg_ParseTuple( args, "s", &mode ) )
	{
		PyErr_SetString( PyExc_TypeError, "WorldEditor.isPanelVisible() "
			"expects a string argument." );
		return NULL;
	}

	if ( mode )
		return PyInt_FromLong( PanelManager::instance().isPanelVisible( bw_utf8tow( mode ) ) );

	return NULL;
}
PY_MODULE_FUNCTION( isPanelVisible, WorldEditor )



/*~ function WorldEditor.addItemToHistory
 *	@components{ worldeditor }
 *
 *	This function adds the asset to the Asset Browser's history
 *
 *	@param path	The path of the asset.
 *	@param type	The type of the asset.
 */
static PyObject * py_addItemToHistory( PyObject * args )
{
	BW_GUARD;

	char* str = 0;
	char* type = 0;
	if ( !PyArg_ParseTuple( args, "ss", &str, &type ) )
	{
		PyErr_SetString( PyExc_TypeError, "WorldEditor.addItemToHistory()"
			"expects two string arguments." );
		return NULL;
	}

	if ( str && type )
		PanelManager::instance().ualAddItemToHistory( bw_utf8tow( str ), bw_utf8tow( type ) );

	Py_RETURN_NONE;
}
PY_MODULE_FUNCTION( addItemToHistory, WorldEditor )


/*~ function WorldEditor.launchTool
 *	@components{ worldeditor }
 *
 *	This function launches the specified tool.
 *
 *	@param tool The name of the tool to launch, e.g., ParticleEditor.
 *	@param cmd	Any startup command-line options that the tool should launch with.
 */
static PyObject* py_launchTool( PyObject * args )
{
	BW_GUARD;

	char* name = 0;
	char* cmdline = 0;
	// TODO:UNICODE: Get unicode out instead of just string
	if ( !PyArg_ParseTuple( args, "ss", &name, &cmdline ) )
	{
		PyErr_SetString( PyExc_TypeError, "WorldEditor.launchTool()"
			"expects two string arguments." );
		return NULL;
	}

	if ( name && cmdline )
	{
		wchar_t exe[ MAX_PATH ];
		GetModuleFileName( NULL, exe, ARRAY_SIZE( exe ) );
		wchar_t* lastSlash = wcsrchr( exe, L'\\' );
		if( lastSlash != NULL )
		{
			BW::wstring wname, wcmdline;
			bw_utf8tow( name, wname );
			bw_utf8tow( cmdline, wcmdline );

			// null terminate at last slash
			*lastSlash = 0;
			BW::wstring path = exe;
			BW::wstring::size_type lastSlash = wname.find_last_of( L'\\' );
			BW::wstring appName;

			if (lastSlash != BW::wstring::npos)
			{
				appName = wname.substr( lastSlash + 1 );
				wname.assign( wname, 0, lastSlash );
				path += L'\\' + wname;
			}
			else
			{
				appName = wname;
			}

			std::replace( path.begin(), path.end(), L'/', L'\\' );
			std::replace( wcmdline.begin(), wcmdline.end(), L'/', L'\\' );

			BW::wstring commandLine = path + L'\\' + appName + L' '+ wcmdline;

			PROCESS_INFORMATION pi;
			STARTUPINFO si;
			GetStartupInfo( &si );

			// note, capital 'S' in the formatting.
			// TRACE_MSG( "WorldEditor.launchTool: cmdline = %S, path = %S\n", commandLine.c_str(), path.c_str() );

			if( CreateProcess( NULL, (LPWSTR) commandLine.c_str(), NULL, NULL, FALSE, 0, NULL, path.c_str(),
				&si, &pi ) )
			{
				CloseHandle( pi.hThread );
				CloseHandle( pi.hProcess );
			}
			else
			{
				ERROR_MSG( "Failed to launch tool (0x%08x) with command line %s\n", GetLastError(), bw_wtoacp(commandLine).c_str() );
			}
		}
	}

	Py_RETURN_NONE;
}
PY_MODULE_FUNCTION( launchTool, WorldEditor )


/**
 *  finds a chunk with a terrain block, and returns the block
 *  TODO: this code is a bit hacky. Should get it from space.settings or
 *  similar.
 */
static Terrain::EditorBaseTerrainBlock* anyTerrainBlock()
{
	BW_GUARD;

	for ( BW::set<Chunk*>::iterator it = EditorChunkCache::chunks_.begin();
		it != EditorChunkCache::chunks_.end() ; ++it )
	{
		if ( ChunkTerrainCache::instance( *(*it) ).pTerrain() )
		{
			return 
				static_cast<Terrain::EditorBaseTerrainBlock*>(
					ChunkTerrainCache::instance( *(*it) ).pTerrain()->block().getObject() );
		}
	}

	return NULL;
}


/*~ function WorldEditor.terrainHeightMapRes
 *	@components{ worldeditor }
 *
 *	This function returns the current terrain height map resolution.
 *
 *	@return current terrain height map resolution, or 1 if not available.
 */
static PyObject * py_terrainHeightMapRes( PyObject * args )
{
	BW_GUARD;

	int res = 0;

	Terrain::EditorBaseTerrainBlock* tb = anyTerrainBlock();
	if ( tb )
		res = tb->heightMap().blocksWidth();

	return PyInt_FromLong( res );
}
PY_MODULE_FUNCTION( terrainHeightMapRes, WorldEditor )


/*~ function WorldEditor.terrainBlendsRes
 *	@components{ worldeditor }
 *
 *	This function returns the current terrain layer blend resolution.
 *
 *	@return current terrain layer blend resolution, or 1 if not available.
 */
static PyObject * py_terrainBlendsRes( PyObject * args )
{
	BW_GUARD;

	int res = 0;

	Terrain::EditorBaseTerrainBlock* tb = anyTerrainBlock();
	if ( tb && tb->numberTextureLayers() )
		res = tb->textureLayer( 0 ).width() - 1;

	return PyInt_FromLong( res );
}
PY_MODULE_FUNCTION( terrainBlendsRes, WorldEditor )


/*~ function WorldEditor.terrainHoleMapRes
 *	@components{ worldeditor }
 *
 *	This function returns the current terrain hole map resolution.
 *
 *	@return current terrain hole map resolution, or 1 if not available.
 */
static PyObject * py_terrainHoleMapRes( PyObject * args )
{
	BW_GUARD;

	int res = 0;

	Terrain::EditorBaseTerrainBlock* tb = anyTerrainBlock();
	if ( tb )
		res = tb->holeMap().width();

	return PyInt_FromLong( res );
}
PY_MODULE_FUNCTION( terrainHoleMapRes, WorldEditor )




/*~ function WorldEditor.resaveAllTerrainBlocks
 *	@components{ worldeditor }
 *
 *	This function resaves all terrain blocks in the space. This is used when the
 *	file format changes and the client does not support the same format.
 */
static void resaveAllTerrainBlocks()
{
	BW_GUARD;

	WorldManager::instance().resaveAllTerrainBlocks();
}
PY_AUTO_MODULE_FUNCTION( RETVOID, resaveAllTerrainBlocks, END, WorldEditor )

/*~ function WorldEditor.restitchAllTerrainBlocks
 *	@components{ worldeditor }
 *
 *	This function restitches all chunks in the space to eliminate seams in the terrain.
 */
static void restitchAllTerrainBlocks()
{
	BW_GUARD;

	WorldManager::instance().restitchAllTerrainBlocks();
}
PY_AUTO_MODULE_FUNCTION( RETVOID, restitchAllTerrainBlocks, END, WorldEditor )


/*~ function WorldEditor.reloadAllChunks
 *	@components{ worldeditor }
 *
 *	This function forces all chunks to be reloaded.
 *
 *	@param bool	(optional) If false, then no warnings will be issued 
 *						to the user about needing to save. 
 *						Defaults to True.
 */
static bool reloadAllChunks( bool askBeforeProceed )
{
	BW_GUARD;

	if ( askBeforeProceed && !WorldManager::instance().canClose(
			LocaliseUTF8( L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/RELOAD" ) ))
	{
		return false;
	}

	CWaitCursor wait;
	WorldManager::instance().reloadAllChunks( askBeforeProceed );

	return true;
}
PY_AUTO_MODULE_FUNCTION( RETDATA, reloadAllChunks, OPTARG( bool, true, END ), WorldEditor );


/*~ function WorldEditor.regenerateThumbnails
 *	@components{ worldeditor }
 *
 *	This function goes through all chunks, both loaded and unloaded, and
 *	recalculates the thumbnails then saves them directly to disk. Chunks
 *	that were unloaded are ejected when it finishes with them, so large
 *	spaces can be regenerated. The downside is that there is no undo/redo, and
 *	the .cdata files are modified directly. It also assumes that the shadow
 *	data is up to date.
 *
 *	This function also deletes the time stamps and dds files.
 */
static void regenerateThumbnails()
{
	BW_GUARD;

	WorldManager::instance().regenerateThumbnailsOffline();
}
PY_AUTO_MODULE_FUNCTION( RETVOID, regenerateThumbnails, END, WorldEditor )

/*~ function WorldEditor.createSpaceImage
 *	@components{ worldeditor }
 *
 *	This function goes through all chunks of the current space and 
 *	create space image to a specify folder. 
 */
static void createSpaceImage()
{
	BW_GUARD;

	WorldManager::instance().createSpaceImage();
}

PY_AUTO_MODULE_FUNCTION( RETVOID, createSpaceImage, END, WorldEditor )




static int pyProgressTaskCnt = 0;
ScopedProgressBar *s_pyProgressBar = NULL;
ProgressBarTask* current_pyProgressTask = NULL;

/*~ function WorldEditor.expandCurrentSpace
 *	@components{ worldeditor }
 *
 *	This function shows up a dialog, in which we can specify the numbers
 *  indicating how to expand current Space. 
 */
static void expandCurrentSpace()
{
	BW_GUARD;

	WorldManager::instance().expandSpaceWithDialog();
}

PY_AUTO_MODULE_FUNCTION( RETVOID, expandCurrentSpace, END, WorldEditor )
/*~ function WorldEditor.startProgress
 *	@components{ worldeditor }
 *
 *	This function shows a progress bar.
 *
 *	@param msg: Message to display on the progress bar.
 *	@param steps: Steps of current progress.
 *	@param escapable: Is the progress cancelable.
 *  @param taskCnt: How much tasks in current progress, if taskCnt more than 1, a MultiTaskProgressBar will be used.
 *  
 *  Usage(in python script):
 *		#Starting a multiTaskProgress
 *		WorldEditor.startProgress( msg, 100, True, 3 )
 *		WorldEditor.progressStep( 1 )
 *		...
 *		WorldEditor.stopProgress()
 *		#Next task in the same progress
 *		WorldEditor.startProgress( msg, 100, True, 3 )
 *		...
 *		WorldEditor.stopProgress()
 *		#The last task in the same progress
 *		WorldEditor.startProgress( msg, 100, True, 3 )
 *		...
 *		WorldEditor.stopProgress()
 *	
 *		#Notice the task here will use a SingleTaskProgressBar
 *		WorldEditor.startProgress( msg, 100, True, 3 )
 *		...
 *		WorldEditor.stopProgress()
 *	
 *	
 */

static void startProgress( const BW::string& msg, float steps, bool escapable, int taskCnt )
{
	BW_GUARD;

	if ( ProgressBar::getCurrentProgressBar() == NULL )
	{
		s_pyProgressBar = new ScopedProgressBar( taskCnt );
	}
	
	if ( pyProgressTaskCnt == 0 )
	{
		pyProgressTaskCnt = taskCnt;
	}
	current_pyProgressTask = new ProgressBarTask( msg, steps, escapable );
}

PY_AUTO_MODULE_FUNCTION( RETVOID, startProgress, ARG( BW::string, ARG( float, ARG( bool, OPTARG( int, 1, END ) ) ) ), WorldEditor )

/*~ function WorldEditor.stopProgress
 *	@components{ worldeditor }
 *
 *	This function stops a progress bar. 
 */
static void stopProgress()
{
	BW_GUARD;

	bool cancelled = false;
	if ( current_pyProgressTask )
	{
		cancelled = current_pyProgressTask->isCancelled();
		bw_safe_delete( current_pyProgressTask );
	}
	if ( ( cancelled || --pyProgressTaskCnt == 0 ) && s_pyProgressBar)
	{
		pyProgressTaskCnt = 0;
		bw_safe_delete( s_pyProgressBar );
	}
}

PY_AUTO_MODULE_FUNCTION( RETVOID, stopProgress, END, WorldEditor )

/*~ function WorldEditor.progressStep
 *	@components{ worldeditor }
 *
 *	This function makes a step of a progress. 
 */
static void progressStep( float progress )
{
	BW_GUARD;

	if ( current_pyProgressTask )
	{
		current_pyProgressTask->step( progress );
	}
}

PY_AUTO_MODULE_FUNCTION( RETVOID, progressStep, ARG( float, END ), WorldEditor )

/*~ function WorldEditor.isProgressCancelled
 *	@components{ worldeditor }
 *
 *	This function returns true if current progress is cancelled. 
 */
static bool isProgressCancelled()
{
	BW_GUARD;
	if (ProgressBar::getCurrentProgressBar())
	{
		return ProgressBar::getCurrentProgressBar()->isCancelled();
	}
	else
	{
		return true;
	}
}

PY_AUTO_MODULE_FUNCTION( RETDATA, isProgressCancelled, END, WorldEditor )

/*~ function WorldEditor.convertSpaceToZip
 *	@components{ worldeditor }
 *
 *	This function goes through all .cdata files of the current space and 
 *	converts them to use zip sections.  It is okay to call this even if
 *	the current space uses zip sections.
 */
static void convertSpaceToZip()
{
	BW_GUARD;

	WorldManager::instance().convertSpaceToZip();
}
PY_AUTO_MODULE_FUNCTION( RETVOID, convertSpaceToZip, END, WorldEditor )


/*~ function WorldEditor.regenerateLODs
 *	@components{ worldeditor }
 *
 *	This function goes through all chunks, both loaded and unloaded, and
 *	recalculates the terrain LOD textures then saves them directly to disk. 
 *	Chunks that were unloaded are ejected when it finishes with them, so large
 *	spaces can be regenerated. The downside is that there is no undo/redo, and
 *	the .cdata files are modified directly. 
 */
static void regenerateLODs()
{
	BW_GUARD;
	MF_ASSERT( WorldManager::instance().pTerrainSettings().hasObject() );

	if (WorldManager::instance().pTerrainSettings()->version() >=
		Terrain::TerrainSettings::TERRAIN2_VERSION)
	{
		WorldManager::instance().regenerateLODsOffline();
	}
}
PY_AUTO_MODULE_FUNCTION( RETVOID, regenerateLODs, END, WorldEditor )


/*~ function WorldEditor.canRegenerateLODs
 *	@components{ worldeditor }
 *
 *	This function returns 1 if the terrain LODs can be regenerated.
 *
 *	@return 1 if the current terrain LODs can be regenerated, or 0 if not.
 */
static PyObject * py_canRegenerateLODs( PyObject * args )
{
	BW_GUARD;

	int res = 0;
	if (WorldManager::instance().pTerrainSettings().hasObject() &&
		WorldManager::instance().pTerrainSettings()->version() >=
			Terrain::TerrainSettings::TERRAIN2_VERSION)
	{
		res = 1;
	}

	return PyInt_FromLong( res );
}
PY_MODULE_FUNCTION( canRegenerateLODs, WorldEditor )


/*~ function WorldEditor.terrainCollide
 *	@components{ worldeditor }
 *
 *	This function tests if a given line collides with the terrain, and returns
 *  the distance traveled from the start to the collision point.
 *
 *  @param start	Start point of the line to collide against the terrain.
 *  @param end		End point of the line to collide against the terrain.
 *	@return	Distance from start to the collision point, -1 if no collision.
 */
static float terrainCollide( Vector3 start, Vector3 end )
{
	BW_GUARD;

	ChunkSpacePtr space = ChunkManager::instance().cameraSpace();
	if ( !space )
		return -1.0f;

	ClosestTerrainObstacle terrainCallback;
	space->collide( start, end, terrainCallback );
	if (terrainCallback.collided())
	{
		return terrainCallback.dist();
	}
	else
	{
		return -1.0;
	}
}
PY_AUTO_MODULE_FUNCTION( RETDATA, terrainCollide, ARG( Vector3, ARG( Vector3, END) ), WorldEditor );


/*~ function WorldEditor.touchAllChunks
 *	@components{ worldeditor }
 *
 *	This function marks all chunks in the current space as dirty.
 */
static void touchAllChunks()
{
	BW_GUARD;

	WorldManager::instance().touchAllChunks();
}
PY_AUTO_MODULE_FUNCTION( RETVOID, touchAllChunks, END, WorldEditor );

/*~ function WorldEditor.performingModalOperation
 *	@components{ worldeditor }
 *
 *	This function checks if a modal operation is on going.
 *  i.e. when there is a progress bar on the screen which is
 *	blocking the main application tick.
 */
static bool performingModalOperation()
{
	BW_GUARD;

	return WorldManager::instance().performingModalOperation();
}
PY_AUTO_MODULE_FUNCTION( RETDATA, performingModalOperation, END, WorldEditor );


/*~ function WorldEditor.isUserEditingPostProcessing
 *	@components{ worldeditor }
 *
 *	This function returns WE's current state for allowing changes to the chain.
 *	If the post processing chain can be changed. If this function return 'true',
 *	the chain won't be changed.
 */
static bool isUserEditingPostProcessing()
{
	BW_GUARD;

	return WorldManager::instance().userEditingPostProcessing();
}
PY_AUTO_MODULE_FUNCTION( RETDATA, isUserEditingPostProcessing, END, WorldEditor );


/*~ function WorldEditor.userEditingPostProcessing
 *	@components{ worldeditor }
 *
 *	This function resets WE's state to allow changes to the chain again. This
 *	function must be used with care since it will allow replacing what the user
 *	has been editing.
 */
static void userEditingPostProcessing( bool editing )
{
	BW_GUARD;

	WorldManager::instance().userEditingPostProcessing( editing );
}
PY_AUTO_MODULE_FUNCTION( RETVOID, userEditingPostProcessing, ARG( bool, END ), WorldEditor );


/*~ function WorldEditor.changedPostProcessing
 *	@components{ worldeditor }
 *
 *	This function flags the current post processing chain as dirty so WE can
 *	refresh it.
 */
static void changedPostProcessing( bool changed )
{
	BW_GUARD;

	WorldManager::instance().changedPostProcessing( changed );
}
PY_AUTO_MODULE_FUNCTION( RETVOID, changedPostProcessing, ARG( bool, END ), WorldEditor );


/*~ function WorldEditor.reloadMaterialKinds
 *	@components{ worldeditor }
 *
 *	This function reloads the material kinds XML file.
 */
static void reloadMaterialKinds()
{
	BW_GUARD;

	MaterialKinds::instance().reload();	
}
PY_AUTO_MODULE_FUNCTION( RETVOID, reloadMaterialKinds, END, WorldEditor );

/*~ function WorldEditor.expandSpace
 *	@components{ worldeditor }
 *
 *	This function expands the current space a specific new size, expanded Chunks would be empty.
 *	@param westCnt	number of additional chunks to the west.
 *	@param eastCnt	number of additional chunks to the east.
 *	@param northCnt	number of additional chunks to the north.
 *	@param southCnt	number of additional chunks to the south.
 */
static void expandSpace( int westCnt, int eastCnt, int northCnt, int southCnt, bool withTerrain )
{
	BW_GUARD;

	WorldManager::instance().expandSpace( westCnt, eastCnt, northCnt, southCnt, withTerrain );
}
PY_AUTO_MODULE_FUNCTION( RETVOID, expandSpace, ARG( int, ARG( int, ARG( int, ARG( int, OPTARG( bool, true, END ) ) ) ) ), WorldEditor );

/*~ function WorldEditor.chunkID
 *	@components{ worldeditor }
 *
 *	This function returns chunkID according the grid position specified.
 *	@param x	x extents of the grid.
 *	@param y	y extents of the grid.
 *	@param checkBounds	set to true if we need to check whether or not the position is out of bound.
 */
static BW::string chunkID( int x, int y, bool checkBounds )
{
	BW_GUARD;
	BW::string id = "";
	chunkID( id, x, y, checkBounds );
	return id;
}
PY_AUTO_MODULE_FUNCTION( RETDATA, chunkID, ARG( int, ARG( int, OPTARG( bool, false, END ) ) ), WorldEditor );

/*~ function WorldEditor.memoryLoad
 *	@components{ worldeditor }
 *
 *	Returns an integer percentage of the system's memory load.
 */
static int memoryLoad()
{
	BW_GUARD;
	return (int)Memory::memoryLoad();
}

PY_AUTO_MODULE_FUNCTION( RETDATA, memoryLoad, END, WorldEditor );

/*~ function WorldEditor.chunkDirtyCounts
 *	@components{ worldeditor }
 *
 *	Returns a dictionary containing the dirty counts for each
 *	type of chunk cache (name -> count).
 */
static PyObject* chunkDirtyCounts()
{
	BW_GUARD;

	RecursiveMutexHolder lock( WorldManager::instance().dirtyChunkListsMutex() );
	EditorChunkCache::lock();

	const DirtyChunkLists& dirtyChunkLists = 
		WorldManager::instance().dirtyChunkLists();

	PyObject* dict = PyDict_New();
	for (size_t i = 0; i < dirtyChunkLists.size(); ++i)
	{
		PyDict_SetItemString( dict,
			dirtyChunkLists.name(i).c_str(),
			PyInt_FromLong( dirtyChunkLists[i].num() ) );
	}

	EditorChunkCache::unlock();

	return dict;
}

PY_AUTO_MODULE_FUNCTION( RETOWN, chunkDirtyCounts, END, WorldEditor );


/*~ function WorldEditor.messageBox
 *	@components{ worldeditor }
 *
 *	This function displays a message box.
 *	Return values are "no", "yes", "ok", "cancel".
 */
static BW::string messageBox( const BW::string & text, const BW::string & title, const BW::string & type )
{
	BW_GUARD;

	BW::string ret = "";
	HWND parentWnd = WorldEditorApp::instance().mainWnd()->GetSafeHwnd();

	BW::string typeStr( type );
	std::transform( typeStr.begin(), typeStr.end(), typeStr.begin(), tolower );

	UINT flags = 0;
	if (typeStr.find( "information" ) != BW::string::npos)
	{
		flags |= MB_ICONINFORMATION;
	}
	else if (typeStr.find( "error" ) != BW::string::npos)
	{
		flags |= MB_ICONERROR;
	}
	else if (typeStr.find( "warning" ) != BW::string::npos)
	{
		flags |= MB_ICONWARNING;
	}
	else if (typeStr.find( "question" ) != BW::string::npos)
	{
		flags |= MB_ICONQUESTION;
	}

	bool isQuestion = false;

	if (typeStr.find( "yesnocancel" ) != BW::string::npos)
	{
		isQuestion = true;
		flags |= MB_YESNOCANCEL;
	}
	else if (typeStr.find( "yesno" ) != BW::string::npos)
	{
		isQuestion = true;
		flags |= MB_YESNO;
	}
	else if (typeStr.find( "okcancel" ) != BW::string::npos)
	{
		isQuestion = true;
		flags |= MB_OKCANCEL;
	}
	else if (typeStr.find( "retrycancel" ) != BW::string::npos)
	{
		isQuestion = true;
		flags |= MB_RETRYCANCEL;
	}

	BW::wstring textW( bw_utf8tow( text ) );
	BW::wstring titleW( bw_utf8tow( title ) );

	if (!text.empty() && text[0] == '`')
	{
		textW = Localise( textW.c_str() );
	}

	if (!title.empty() && title[0] == '`')
	{
		titleW = Localise( titleW.c_str() );
	}


	int questionResult = MessageBox( parentWnd, textW.c_str(), titleW.c_str(), flags );

	if (isQuestion)
	{
		if (questionResult == IDOK)
		{
			ret = "ok";
		}
		else if (questionResult == IDYES)
		{
			ret = "yes";
		}
		else if (questionResult == IDCANCEL)
		{
			ret = "cancel";
		}
		else
		{
			ret = "no";
		}
	}

	return ret;
}
PY_AUTO_MODULE_FUNCTION( RETDATA, messageBox,
						ARG( BW::string,
						OPTARG( BW::string, "WorldEditor",
						OPTARG( BW::string, "information", END ) ) ), WorldEditor );

/*~ function WorldEditor.stringInputBox
 *	@components{ worldeditor }
 *
 *	This function displays a string input box, and returns the string typed by the user.
 *	If the user hits the "Cancel" button, it returns the special string "<cancel>".
 */
static BW::string stringInputBox( const BW::string & label, const BW::string & title, int maxlen, const BW::string & str )
{
	BW_GUARD;

	BW::wstring labelW( bw_utf8tow( label ) );
	BW::wstring titleW( bw_utf8tow( title ) );
	BW::string strL( str );

	if (!label.empty() && label[0] == '`')
	{
		labelW = Localise( labelW.c_str() );
	}

	if (!title.empty() && title[0] == '`')
	{
		titleW = Localise( titleW.c_str() );
	}

	if (!str.empty() && str[0] == '`')
	{
		strL = LocaliseUTF8( strL.c_str() );
	}

	BW::string ret = "<cancel>";

	StringInputDlg strInput( WorldEditorApp::instance().mainWnd() );
	strInput.init( titleW, labelW, maxlen, strL );
	if (strInput.DoModal() == IDOK)
	{
		ret = strInput.result();
	}

	return ret;
}
PY_AUTO_MODULE_FUNCTION( RETDATA, stringInputBox,
						ARG( BW::string,
						OPTARG( BW::string, "WorldEditor",
						OPTARG( int, 80, // max length
						OPTARG( BW::string, "", END ) ) ) ), WorldEditor );

/*~ function WorldEditor.gridBounds
 *	@components{ worldeditor }
 *
 *	This method calculates the bounding box of the space in world coords.
 *
 *	@return A 2-tuple containing the min bounds and max bounds as Vector3's.
 *
 */
static PyObject* gridBounds()
{
	BW_GUARD;

	ChunkSpacePtr space = ChunkManager::instance().cameraSpace();
	if (!space)
	{
		Py_RETURN_NONE;
	}

	BoundingBox bounds = space->gridBounds();

	PyObject* tuple = PyTuple_New( 2 );

	PyTuple_SetItem( tuple, 0, Script::getData( bounds.minBounds() ) );
	PyTuple_SetItem( tuple, 1, Script::getData( bounds.maxBounds() ) );

	return tuple;
}

PY_AUTO_MODULE_FUNCTION( RETOWN, gridBounds, END, WorldEditor )


static bool goToBookmark( const BW::string & bookmark )
{
	BW::string spaceName = WorldManager::instance().getCurrentSpace();
	DataSectionPtr locationData = BWResource::openSection(
		spaceName + "/locations.xml" );

	if (!locationData)
	{
		PyErr_SetString( PyExc_TypeError, 
			"goToBookmark: Could not open location data." );
		return false;
	}

	BW::vector<DataSectionPtr> bookmarks;
	locationData->openSections( "bookmark", bookmarks );
	for ( size_t i = 0; i < bookmarks.size(); i++ )
	{
		if ( bookmarks[i]->readString( "name", "" ) == bookmark )
		{
			WorldEditorCamera::instance().currentCamera().view(
				bookmarks[i]->readMatrix34( "view",
					WorldEditorCamera::instance().currentCamera().view() ) );
			return true;
		}
	}

	BW::string errorMsg = "goToBookmark: Could not find bookmark " + bookmark;
	PyErr_SetString( PyExc_TypeError, errorMsg.c_str() );
	return false;
}

PY_AUTO_MODULE_FUNCTION( RETDATA,
						goToBookmark,
						ARG( BW::string, END ),
						WorldEditor );


static bool capturePanel( const BW::string& panelName, 
						  const BW::string& fileName )
{
	BW_GUARD;

	BW::wstring panelID = PanelManager::instance().getContentID( 
		bw_utf8tow( panelName ) );
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
PY_AUTO_MODULE_FUNCTION( RETDATA,
						capturePanel,
						ARG( BW::string, ARG( BW::string, END ) ),
						WorldEditor );


static bool captureScene( const BW::string& fileName )
{
	BW_GUARD;

	BW::string fname ( fileName );
	BW::string fext = BWResource::getExtension( fname ).to_string();
	if (fext.empty())
	{
		fext = "bmp";
	}
	else
	{
		fname = BWResource::removeExtension( fname ).to_string();
	}

	return !Moo::rc().screenShot( fext, fname, false ).empty();
}
PY_AUTO_MODULE_FUNCTION( RETDATA,
						captureScene,
						ARG( BW::string, END ),
						WorldEditor );

// -----------------------------------------------------------------------------
// Section: Common stuff (should be elsewhere...)
// -----------------------------------------------------------------------------

/// Yes, these are globals
struct TimerRecord
{
	/**
	 *	This method returns whether or not the input record occurred later than
	 *	this one.
	 *
	 *	@return True if input record is earlier (higher priority),
	 *		false otherwise.
	 */
	bool operator <( const TimerRecord & b ) const
	{
		return b.time < this->time;
	}

	float		time;			///< The time of the record.
	PyObject	* function;		///< The function associated with the record.
};

typedef std::priority_queue<TimerRecord>	Timers;
Timers	gTimers;


/*~ function WorldEditor.callback
 *	@components{ worldeditor }
 *
 *	Registers a callback function to be called after a certain time,
 *	but not before the next tick. (If registered during a tick
 *	and it has expired then it will go off still - add a minuscule
 *	amount of time to BigWorld.time() to prevent this if unwanted)
 *	Non-positive times are interpreted as offsets from the current time.
 *
 *	@param time The amount of time to pass before the function is called.
 *	@param function The callback function.
 */
static PyObject * py_callback( PyObject * args )
{
	BW_GUARD;

	float		time = 0.f;
	PyObject *	function = NULL;

	if (!PyArg_ParseTuple( args, "fO", &time, &function ) ||
		function == NULL || !PyCallable_Check( function ) )
	{
		PyErr_SetString( PyExc_TypeError, "py_callback: Argument parsing error." );
		return NULL;
	}

	if (time < 0) time = 0.f;

	//TODO
	//time = EntityManager::getTimeNow() + time;
	Py_INCREF( function );

	TimerRecord		newTR = { time, function };
	gTimers.push( newTR );

	Py_RETURN_NONE;
}
PY_MODULE_FUNCTION( callback, WorldEditor )



// -----------------------------------------------------------------------------
// Section: WorldEditorScript namespace functions
// -----------------------------------------------------------------------------


namespace
{
	PyObject* s_keyModule = NULL;
}

// Link the external libraries we want to be builtin
extern "C" void init_imaging(void);

/**
 *	This method initialises the WorldEditor script.
 */
bool WorldEditorScript::init( DataSectionPtr pDataSection )
{
	BW_GUARD;

	// Particle Systems are creatable from Python code
	MF_VERIFY( ParticleSystemManager::init() );

	PyImportPaths importPaths;
	importPaths.addResPath( "resources/scripts" );
	importPaths.addResPath( EntityDef::Constants::entitiesEditorPath() );
	importPaths.addResPath( EntityDef::Constants::entitiesClientPath() );
	importPaths.addResPath( EntityDef::Constants::userDataObjectsEditorPath() );

	// Append the external libraries we want to be builtin
	PyImport_AppendInittab( "_imaging", init_imaging );

	// Call the general init function
	if (!Script::init( importPaths, "editor" ))
	{
		CRITICAL_MSG( "WorldEditorScript::init: Failed to init Script.\n" );
		return false;
	}
	Options::initLoggers();

	s_keyModule = PyImport_ImportModule( "Keys" );

	if (PyObject_HasAttrString( s_keyModule, "init" ))
	{
		PyObject * pInit =
			PyObject_GetAttrString( s_keyModule, "init" );
		PyRun_SimpleString( PyString_AsString(pInit) );
		Py_DECREF( pInit );
	}

	PyRun_SimpleString("import GUI" );
	PyRun_SimpleString("import Math" );
	PyRun_SimpleString("import Pixie" );
	PyRun_SimpleString("import Keys" );
	PyRun_SimpleString("import WorldEditor" );
	PyRun_SimpleString("import BigWorld" );

	PyErr_Clear();

	return true;
}


/**
 *	This method does the script clean up.
 */
void WorldEditorScript::fini()
{
	BW_GUARD;

	Py_XDECREF( s_keyModule );

	Script::fini();
	ParticleSystemManager::fini();
}
BW_END_NAMESPACE

