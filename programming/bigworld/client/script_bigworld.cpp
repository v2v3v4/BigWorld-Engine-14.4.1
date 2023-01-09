#include "pch.hpp"
#include "script_bigworld.hpp"

#include "app.hpp"
#include "app_config.hpp"
#include "connection_control.hpp"
#include "entity_manager.hpp"
#include "entity_type.hpp"
#include "physics.hpp"
#include "pathed_filename.hpp"
#include "py_entity.hpp"

#include "common/closest_triangle.hpp"
#include "common/space_data_types.hpp"

#include "connection/client_server_protocol_version.hpp"
#include "connection/entity_def_constants.hpp"

#include "connection_model/bw_connection.hpp"
#include "connection_model/bw_replay_connection.hpp"
#include "connection_model/bw_server_connection.hpp"
#include "connection_model/bw_null_connection.hpp"

#include "space/client_space.hpp"
#include "space/space_manager.hpp"

#include "chunk/chunk_manager.hpp"
#include "chunk/chunk_obstacle.hpp"
#include "chunk/chunk_overlapper.hpp"
#include "chunk/chunk_space.hpp"
#include "chunk/chunk_terrain.hpp"
#include "chunk/user_data_object.hpp"
#include "chunk_scene_adapter/client_chunk_space_adapter.hpp"

#include "cstdmf/allocator.hpp"
#include "cstdmf/debug.hpp"

#include "entitydef/constants.hpp"

#include "input/input.hpp"
#include "input/py_input.hpp"

#include "moo/animation_manager.hpp"
#include "moo/graphics_settings.hpp"
#include "moo/texture_manager.hpp"
#include "moo/texture_streaming_manager.hpp"

#include "particle/particle_system_manager.hpp"

#include "physics2/material_kinds.hpp"
#include "physics2/worldtri.hpp"

#include "pyscript/personality.hpp"
#include "pyscript/pickler.hpp"
#include "pyscript/py_data_section.hpp"
#include "pyscript/py_import_paths.hpp"
#include "pyscript/script.hpp"
#include "pyscript/script_math.hpp"

#include "resmgr/multi_file_system.hpp"
#include "resmgr/xml_section.hpp"

#include "romp/weather.hpp"
#include "romp/flora.hpp"
#if ENABLE_CONSOLES
#include "romp/console_manager.hpp"
#include "romp/xconsole.hpp"
#endif // ENABLE_CONSOLES

#include "terrain/base_terrain_block.hpp"
#include "terrain/terrain2/terrain_lod_controller.hpp"

#include "waypoint/chunk_waypoint_set.hpp"
#include "waypoint/navigator.hpp"
#include "waypoint/navigator_cache.hpp"
#include "waypoint/waypoint_neighbour_iterator.hpp"

#include "camera/base_camera.hpp"
#include "camera/projection_access.hpp"

#include "space/deprecated_space_helpers.hpp"

#include <queue>

DECLARE_DEBUG_COMPONENT2( "Script", 0 )


BW_BEGIN_NAMESPACE

typedef SmartPointer<PyObject> PyObjectPtr;

typedef std::pair<KeyCode::Key, PyObject*> KeyPyObjectPair;
typedef BW::list<KeyPyObjectPair> KeyPyObjectPairList;

namespace {
	KeyPyObjectPairList g_keyEventSinks;

	BW::string g_cmdLineUsername;
	bool g_hasCmdLineUsername = false;

	BW::string g_cmdLinePassword;
	bool g_hasCmdLinePassword = false;
}


// -----------------------------------------------------------------------------
// Section: BigWorld module: Chunk access functions
// -----------------------------------------------------------------------------
typedef SmartPointer<PyObject> PyObjectPtr;


/*~ function BigWorld.findDropPoint
 *
 *	@param spaceID The ID of the space you want to do the raycast in
 *	@param vector3 Start point for the collision test
 *	@return A pair of the drop point, and the triangle it hit,
 *		or None if nothing was hit.
 *
 *	Finds the point directly beneath the start point that collides with
 *	the collision scene and terrain (if present in that location)
 */
/**
 *	Finds the drop point under the input point using the collision
 *	scene and terrain (if present in that location)
 *
 *	@return A pair of the drop point, and the triangle it hit,
 *		or None if nothing was hit.
 */
static PyObject * findDropPoint( SpaceID spaceID, const Vector3 & inPt )
{
	BW_GUARD;
	Vector3		outPt;

	ClientSpacePtr pSpace = SpaceManager::instance().space( spaceID );
	if (!pSpace)
	{
		PyErr_Format( PyExc_ValueError, "BigWorld.findDropPoint(): "
			"No space ID %d", spaceID );
		return NULL;
	}

	ClosestTriangle fdpt;
	Vector3 ndPt( inPt.x, inPt.y-100.f, inPt.z );
	float dist = pSpace->collide( inPt, ndPt, fdpt );
	if (dist < 0.f)
	{
		Py_RETURN_NONE;
	}
	outPt.set( inPt.x, inPt.y-dist, inPt.z );

	WorldTriangle resultTri = fdpt.triangle();

	return Py_BuildValue( "(N,(N,N,N))",
				Script::getData( outPt ),
				Script::getData( resultTri.v0() ),
				Script::getData( resultTri.v1() ),
				Script::getData( resultTri.v2() ) );
}
PY_AUTO_MODULE_FUNCTION( RETOWN, findDropPoint,
	ARG( SpaceID, ARG( Vector3, END ) ), BigWorld )


/*~ function BigWorld.cameraSpaceID
 *
 *	@param spaceID (optional) Sets the space that the camera exists in.
 *	@return spaceID The spaceID that the camera currently exists in.
 *		If the camera is not in any space then 0 is returned.
 */
/**
 *	This function returns the id of the space that the camera is currently in.
 *	If the camera is not in any space then 0 is returned.
 *	You can optionally set the spaceID by passing it as an argument.
 */
static PyObject * cameraSpaceID( SpaceID newSpaceID = 0 )
{
	BW_GUARD;
	if (newSpaceID != 0)
	{
		ClientSpacePtr pSpace =
			SpaceManager::instance().space( newSpaceID );
		if (!pSpace)
		{
			PyErr_Format( PyExc_ValueError, "BigWorld.cameraSpaceID: "
				"No such space ID %d\n", newSpaceID );
			return NULL;
		}

		DeprecatedSpaceHelpers::cameraSpace( pSpace );
		ChunkManager::instance().camera( Moo::rc().invView(), 
			ClientChunkSpaceAdapter::getChunkSpace(pSpace) );
	}
	else 
	{ 
		DeprecatedSpaceHelpers::cameraSpace( NULL );
		ChunkManager::instance().camera( Moo::rc().invView(), NULL ); 
	} 


	ClientSpacePtr pSpace = DeprecatedSpaceHelpers::cameraSpace();
	return Script::getData( pSpace ? pSpace->id() : 0 );
}
PY_AUTO_MODULE_FUNCTION( RETOWN, cameraSpaceID,
	OPTARG( SpaceID, 0, END ), BigWorld )


/**
 *	This helper function gets the given client space ID.
 */
static ClientSpacePtr getClientSpace( SpaceID spaceID, const char * methodName )
{
	BW_GUARD;
	ClientSpacePtr pSpace = SpaceManager::instance().space( spaceID );
	if (!pSpace)
	{
		PyErr_Format( PyExc_ValueError, "%s: No space ID %d",
			methodName, spaceID );
		return NULL;
	}

	ConnectionControl & connectionControl = ConnectionControl::instance();
	SpaceManager & spaceManager = SpaceManager::instance();

	if (!spaceManager.isLocalSpace( spaceID ) ||
		!connectionControl.pSpaceConnection( spaceID ))
	{
		PyErr_Format( PyExc_ValueError, "%s: Space ID %d is not a client space "
			"(or is no longer held)", methodName, spaceID );
		return NULL;
	}

	return pSpace;
}


/*~ function BigWorld.addSpaceGeometryMapping
 *
 *	This function maps geometry into the given client space ID.
 *	It cannot be used with spaces created by the server since the server
 *	controls the geometry mappings in those spaces.
 *
 *	The given transform must be aligned to the chunk grid. That is, it should
 *	be a translation matrix whose position is in multiples of the space's chunkSize
 *	on the X and Z axis. Any other transform will result in undefined behaviour. 
 *
 *	Any extra space mapped in must use the same terrain system as the first,
 *	with the same settings, the behaviour of anything else is undefined.
 *
 *	Raises a ValueError if the space for the given spaceID is not found.
 *
 *	@param spaceID 		The ID of the space
 *	@param matrix 		The transform to apply to the geometry. None may be
 *						passed in if no transform is required (the identity
 *						matrix).
 *	@param filepath 	The path to the directory containing the space data
 *	@return 			(integer) handle that is used when removing mappings
 *						using BigWorld.delSpaceGeometryMapping().
 *
 */
/**
 *	This function maps geometry into the given client space ID.
 *
 *	It cannot be used with spaces created by the server since the server
 *	controls the geometry mappings in those spaces.
 *
 *	It returns an integer handle which can later be used to unmap it.
 */
static PyObject * addSpaceGeometryMapping( SpaceID spaceID,
	MatrixProviderPtr pMapper, const BW::string & path )
{
	BW_GUARD;

	BWResource::WatchAccessFromCallingThreadHolder watchAccess( false );

	ConnectionControl & cc = ConnectionControl::instance();
	
	BWServerConnection * pServerConn = cc.pServerConnection();
	if (pServerConn && pServerConn->spaceID() == spaceID)
	{
		PyErr_SetString( PyExc_ValueError,
			"Cannot add space geometry mapping for server space" );
		return NULL;
	}

	BWReplayConnection * pReplayConn = cc.pReplayConnection();
	if (pReplayConn && pReplayConn->spaceID() == spaceID)
	{
		PyErr_SetString( PyExc_ValueError,
			"Cannot add space geometry mapping for replayed space" );
		return NULL;
	}

	SpaceEntryID spaceEntryID = BigWorldClientScript::nextSpaceEntryID();

	Matrix matrix = Matrix::identity;
	if (pMapper)
	{
		pMapper->matrix( matrix );
	}
	
	// Feed space data through BWNullConnection to mimic online and replay
	BWNullConnection * pNullConn = cc.pNullConnection();
	if (pNullConn && cc.pPlayer() && cc.pPlayer()->spaceID() == spaceID)
	{
		BW::string data( (char*)(float*)matrix, sizeof( Matrix ) );
		data.append( path );

		pNullConn->spaceData(
			spaceID, spaceEntryID, SPACE_DATA_MAPPING_KEY_CLIENT_SERVER, data );

		return Script::getData( spaceEntryID.salt );
	}

	ClientSpacePtr pSpace = getClientSpace(
		spaceID, "BigWorld.addSpaceGeometryMapping()" );

	// getClientSpace sets the Python error state if it fails
	if (!pSpace)
	{
		return NULL;
	}

	SpaceDataMapping * pSpaceDataMapping = &pSpace->spaceData();

	IF_NOT_MF_ASSERT_DEV( pSpaceDataMapping != NULL )
	{
		PyErr_Format( PyExc_ValueError, "Could not map %s into space ID %d "
				"(could not find space data mapping)",
			path.c_str(), spaceID );
		return NULL;
	}

	// Incredibly small chance we already used this id
	while (pSpaceDataMapping->dataRetrieveSpecific( spaceEntryID ).valid())
	{
		spaceEntryID = BigWorldClientScript::nextSpaceEntryID();
	}

	BW::string emptyString;

	pSpaceDataMapping->addDataEntry( spaceEntryID,
				SPACE_DATA_MAPPING_KEY_CLIENT_SERVER, 
				emptyString );

	// See if we can add the mapping
	// NOTE: Currently not using the asynchronous version as we wouldn't be able
	// to tell if there is a script error. We may want to reconsider or make
	// this an argument.
	if (!pSpace->addMapping(
			*(ClientSpace::GeometryMappingID*)&spaceEntryID, matrix, path ))
	{
		// No good, so remove the data entry
		pSpaceDataMapping->delDataEntry( spaceEntryID );

		PyErr_Format( PyExc_ValueError, "BigWorld.addSpaceGeometryMapping(): "
			"Could not map %s into space ID %d (probably no space.settings)",
			path.c_str(), spaceID );
		return NULL;
	}

	Personality::instance().callMethod( "onGeometryMapped",
		ScriptArgs::create( spaceID, path ),
		ScriptErrorPrint( "EntityManager::spaceData geometry notifier: " ),
		/* allowNullMethod */ true );

	// Everything's hunky dory
	return Script::getData( spaceEntryID.salt );
}
PY_AUTO_MODULE_FUNCTION( RETOWN, addSpaceGeometryMapping, ARG( SpaceID,
	ARG( MatrixProviderPtr, ARG( BW::string, END ) ) ), BigWorld )


/*~ function BigWorld.delSpaceGeometryMapping
 *
 *	This function unmaps geometry from the given client space ID.
 *
 *	It cannot be used with spaces created by the server since the server
 *	controls the geometry mappings in those spaces.
 *
 *	Raises a ValueError if the space for the given spaceID is not found, or if
 *	the handle does not refer to a mapped space geometry.
 *
 *	@param spaceID 	The ID of the space
 *	@param handle 	An integer handle to the space that was returned when
 *					created
 */
/**
 *	This function unmaps geometry from the given client space ID.
 *
 *	It cannot be used with spaces created by the server since the server
 *	controls the geometry mappings in those spaces.
 */
static bool delSpaceGeometryMapping( SpaceID spaceID, uint32 handle )
{
	BW_GUARD;
	
	// Corresponding addSpaceGeometryMapping uses salt as the handle
	SpaceEntryID spaceEntryID;
	spaceEntryID.salt = handle;

	ConnectionControl & cc = ConnectionControl::instance();
	
	BWServerConnection * pServerConn = cc.pServerConnection();
	if (pServerConn && pServerConn->spaceID() == spaceID)
	{
		PyErr_SetString( PyExc_ValueError, 
			"Cannot del space geometry mapping for server space" ); 
		return NULL;
	}

	BWReplayConnection * pReplayConn = cc.pReplayConnection();
	if (pReplayConn && pReplayConn->spaceID() == spaceID)
	{
		PyErr_SetString( PyExc_ValueError, 
			"Cannot del space geometry mapping for replayed space" ); 
		return NULL;
	}

	// Feed space data through BWNullConnection to mimic online and replay
	BWNullConnection * pNullConn = cc.pNullConnection();
	if (pNullConn && cc.pPlayer() && cc.pPlayer()->spaceID() == spaceID)
	{
		BW::string emptyString;
		pNullConn->spaceData( spaceID, spaceEntryID, uint16(-1), emptyString );
		
		return true;
	}

	ClientSpacePtr pSpace = getClientSpace(
		spaceID, "BigWorld.delSpaceGeometryMapping()" );

	// getClientSpace sets the Python error state if it fails
	if (!pSpace)
	{
		return NULL;
	}

	SpaceDataMapping * pSpaceDataMapping = &pSpace->spaceData();

	if (pSpaceDataMapping == NULL)
	{
		return false;
	}

	if (!pSpaceDataMapping->delDataEntry( spaceEntryID ))
	{
		PyErr_Format( PyExc_ValueError, "BigWorld.delSpaceGeometryMapping(): "
			"Could not unmap entry id %d from space ID %d (no such entry)",
			int(handle), spaceID );
		return false;
	}

	pSpace->delMapping( *(ClientSpace::GeometryMappingID*)&spaceEntryID );

	return true;
}
PY_AUTO_MODULE_FUNCTION( RETOK, delSpaceGeometryMapping,
	ARG( SpaceID, ARG( uint32, END ) ), BigWorld )


/*~ function BigWorld.spaceLoadStatus
*
*	This function queries the chunk loader to see how much of the current camera
*	space has been loaded.  It queries the chunk loader to see the distance
*  to the currently loading chunk ( the chunk loader loads the closest chunks
*	first ).  A percentage is returned so that scripts can use the information
*	to create for example a teleportation progress bar.
*
*	@param (optional) distance The distance to check for.  By default this is
*	set to the current far plane.
*	@return float Rough percentage of the loading status
*/
/**
*	This function queries the chunk loader to see how much of the current camera
*	space has been loaded.  It queries the chunk loader to see the distance
*  to the currently loading chunk ( the chunk loader loads the closest chunks
*	first. )  A percentage is returned so that scripts can use the information
*	to create for example a teleportation progress bar.
*/
static float spaceLoadStatus( float distance = -1.f )
{
	BW_GUARD;
	ClientSpacePtr pSpace = DeprecatedSpaceHelpers::cameraSpace();
	return pSpace ? pSpace->loadStatus( distance ) : 0.f;
}
PY_AUTO_MODULE_FUNCTION( RETDATA, spaceLoadStatus, OPTARG( float, -1.f, END ), BigWorld )



/*~ function BigWorld.restartGame
 *
 *  This function restarts the game.  It can be used to restart the game
 *	after certain graphics options have been changed that require a restart.
 */
/**
 *  This function restarts the game.  It can be used to restart the game
 *	after certain graphics options have been changed that require a restart.
 */
static void restartGame()
{
	BW_GUARD;
	App::instance().quit(true);
}
PY_AUTO_MODULE_FUNCTION( RETVOID, restartGame, END, BigWorld )


/*~ function BigWorld.listVideoModes
 *
 *	Lists video modes available on current display device.
 *	@return	list of 5-tuples (int, int, int, int, string).
 *				(mode index, width, height, BPP, description)
 */
static PyObject * listVideoModes()
{
	BW_GUARD;
	Moo::DeviceInfo info = Moo::rc().deviceInfo( Moo::rc().deviceIndex() );
	PyObject * result = PyList_New(info.displayModes_.size());

	typedef BW::vector< D3DDISPLAYMODE >::const_iterator iterator;
	iterator modeIt = info.displayModes_.begin();
	iterator modeEnd = info.displayModes_.end();
	for (int i=0; modeIt < modeEnd; ++i, ++modeIt)
	{
		PyObject * entry = PyTuple_New(5);
		PyTuple_SetItem(entry, 0, Script::getData(i));
		PyTuple_SetItem(entry, 1, Script::getData(modeIt->Width));
		PyTuple_SetItem(entry, 2, Script::getData(modeIt->Height));

		int bpp = 0;
		switch (modeIt->Format)
		{
		case D3DFMT_A2R10G10B10:
			bpp = 32;
			break;
		case D3DFMT_A8R8G8B8:
			bpp = 32;
			break;
		case D3DFMT_X8R8G8B8:
			bpp = 32;
			break;
		case D3DFMT_A1R5G5B5:
			bpp = 16;
			break;
		case D3DFMT_X1R5G5B5:
			bpp = 16;
			break;
		case D3DFMT_R5G6B5:
			bpp = 16;
			break;
		default:
			bpp = 0;
		}
		PyTuple_SetItem(entry, 3, Script::getData(bpp));
		PyObject * desc = PyString_FromFormat( "%dx%dx%d",
			modeIt->Width, modeIt->Height, bpp);
		PyTuple_SetItem(entry, 4, desc);
		PyList_SetItem(result, i, entry);
	}
	return result;
}
PY_AUTO_MODULE_FUNCTION( RETOWN, listVideoModes, END, BigWorld )


/*~ function BigWorld.changeVideoMode
 *
 *  This function allows you to change between fullscreen and
 *	windowed mode.  If switching to fullscreen mode, the video
 *	mode index is used to determine the new resolution.  If
 *	switching to windowed mode, this parameter is ignored.
 *
 *	The exception to the above is if you set the modeIndex
 *	to -1, then both parameters are ignored, and the device
 *	will simply be reset and remain with its current settings.
 *
 *	The video mode index is reported via the listVideoModes function.
 *	@see BigWorld.listVideoModes 
 *
 *	@param	new			int - fullscreen video mode to use.
 *	@param	windowed	bool - True windowed mode is desired.
 *	@return				bool - True on success. False otherwise.
 */
static bool changeVideoMode( int modeIndex, bool windowed )
{
	BW_GUARD;
	return (
		Moo::rc().device() != NULL &&
		Moo::rc().changeMode( modeIndex, windowed ) );
}
PY_AUTO_MODULE_FUNCTION( RETDATA, changeVideoMode, ARG( int, ARG( bool, END ) ), BigWorld )


/*~ function BigWorld.isVideoVSync
 *
 *  Returns current display's vertical sync status.
 *	@return		bool - True if vertical sync is on. False if it is off.
 */
static bool isVideoVSync()
{
	BW_GUARD;
	return Moo::rc().device() != NULL
		? Moo::rc().waitForVBL()
		: false;
}
PY_AUTO_MODULE_FUNCTION( RETDATA, isVideoVSync, END, BigWorld )


/*~ function BigWorld.setVideoVSync
 *
 *  Turns vertical sync on/off.
 *	@param	doVSync		bool - True to turn vertical sync on.
 *						False to turn if off.
 */
static void setVideoVSync( bool doVSync )
{
	BW_GUARD;
	if (Moo::rc().device() != NULL && doVSync != Moo::rc().waitForVBL())
	{
		Moo::rc().waitForVBL(doVSync);
		Moo::rc().changeMode(Moo::rc().modeIndex(), Moo::rc().windowed());
	}
}
PY_AUTO_MODULE_FUNCTION( RETVOID, setVideoVSync, ARG( bool, END ), BigWorld )

/*~ function BigWorld.isTripleBuffered
 *
 *  Returns current display's triple buffering status.
 *	@return		bool - True if triple buffering is on. False if it is off.
 */
static bool isTripleBuffered()
{
	BW_GUARD;
	return Moo::rc().device() != NULL
		? Moo::rc().tripleBuffering()
		: false;
}
PY_AUTO_MODULE_FUNCTION( RETDATA, isTripleBuffered, END, BigWorld )


/*~ function BigWorld.setTripleBuffering
 *
 *  Turns triple buffering on/off.
 *	@param	doTripleBuffering bool - True to turn triple buffering on.
 *						False to turn if off.
 */
static void setTripleBuffering( bool doTripleBuffering )
{
	BW_GUARD;
	if (Moo::rc().device() != NULL && doTripleBuffering != Moo::rc().tripleBuffering())
	{
		Moo::rc().tripleBuffering(doTripleBuffering);
		Moo::rc().changeMode(Moo::rc().modeIndex(), Moo::rc().windowed());
	}
}
PY_AUTO_MODULE_FUNCTION( RETVOID, setTripleBuffering, ARG( bool, END ), BigWorld )

/*~ function BigWorld.videoModeIndex
 *
 *  Retrieves index of current video mode.
 *	@return	int		Index of current video mode or zero if render
 *					context has not yet been initialised.
 */
static int videoModeIndex()
{
	BW_GUARD;
	return
		Moo::rc().device() != NULL
			? Moo::rc().modeIndex()
			: 0;
}
PY_AUTO_MODULE_FUNCTION( RETDATA, videoModeIndex, END, BigWorld )


/*~ function BigWorld.isVideoWindowed
 *
 *  Queries current video windowed state.
 *	@return	bool	True is video is windowed. False if fullscreen.
 */
static bool isVideoWindowed()
{
	BW_GUARD;
	return
		Moo::rc().device() != NULL
		? Moo::rc().windowed()
		: false;
}
PY_AUTO_MODULE_FUNCTION( RETDATA, isVideoWindowed, END, BigWorld )


/*~ function BigWorld.resizeWindow
 *
 *  Sets the size of the application window (client area) when running
 *	in windowed mode. Does nothing when running in fullscreen mode.
 *
 *	@param width	The desired width of the window's client area.
 *	@param height	The desired height of the window's client area.
 */
static void resizeWindow( int width, int height )
{
	BW_GUARD;
	if (Moo::rc().windowed())
	{
		App::instance().resizeWindow(width, height);
	}
}
PY_AUTO_MODULE_FUNCTION( RETVOID, resizeWindow, ARG( int, ARG( int, END ) ),  BigWorld )


/*~ function BigWorld.windowSize
 *
 *  Returns size of application window when running in windowed mode. This
 *	is different to the screen resolution when running in fullscreen mode
 *	(use listVideoModes and videoModeIndex functions to get the screen
 *	resolution in fullscreen mode).
 *
 *	This function is deprecated. Use screenSize, instead.
 *
 *	@return		2-tuple of floats (width, height)
 */
static PyObject * py_windowSize( PyObject * args )
{
	BW_GUARD;
	float width = Moo::rc().screenWidth();
	float height = Moo::rc().screenHeight();
	PyObject * pTuple = PyTuple_New( 2 );
	PyTuple_SetItem( pTuple, 0, Script::getData( width ) );
	PyTuple_SetItem( pTuple, 1, Script::getData( height ) );
	return pTuple;
}
PY_MODULE_FUNCTION( windowSize, BigWorld )


/*~ function BigWorld.changeFullScreenAspectRatio
 *
 *  Changes screen aspect ratio for full screen mode.
 *	@param	ratio		the desired aspect ratio: float (width/height).
 */
static void changeFullScreenAspectRatio( float ratio )
{
	BW_GUARD;
	if (Moo::rc().device() != NULL)
	{
		Moo::rc().fullScreenAspectRatio( ratio );
	}
}
PY_AUTO_MODULE_FUNCTION( RETVOID, changeFullScreenAspectRatio, ARG( float, END ), BigWorld )

/*~ function BigWorld.getFullScreenAspectRatio
 *	@components{ client }
 *
 *	This function returns an estimate of the amount of memory the application
 *	is currently using.
 */
static float getFullScreenAspectRatio()
{
	BW_GUARD;
	return Moo::rc().fullScreenAspectRatio();
}
PY_AUTO_MODULE_FUNCTION( RETDATA, getFullScreenAspectRatio, END, BigWorld )


/*~ function BigWorld.sinkKeyEvents
 *
 *	Adds a global event handler for the given key. The handler will exist up to and
 *	including the next "key up" event for the given key. This is useful if you process 
 *	the	key down event and want to stop	all subsequent key events for a particular key 
 *	from occuring. For example, it is useful when the GUI state is changed in the
 *	GUI component's handleKeyDown and you don't want the new GUI state to receive
 *	the subsequent char or key up events (i.e. the user would have to fully let go
 *	of the key and press it again before anything receives more events from that key).
 *
 *	The handler should be a	class instance with methods "handleKeyEvent" and 
 *	"handleCharEvent". The handler methods should return True to override the
 *	event and stop it from being passed to any other handleres, or False to
 *	allow the event to continue as per normal.
 *
 *	If no event handler is specified, then it will sink all events up to and
 *	including the next key-up event.
 *
 *	When the 
 *
 *	@param key				The key code to be routed to the sink.
 *	@param [optional] sink	The class instance that will process the key events.
 *							If not specified, it will sink all events for the 
 *							given key up to and including the next key up.
 */
static PyObject* py_sinkKeyEvents( PyObject* args )
{
	BW_GUARD;

	int keycode=0;
	PyObject* pyhandler=NULL;

	if (!PyArg_ParseTuple(args, "i|O", &keycode, &pyhandler))
	{
		PyErr_Format(PyExc_TypeError, "Expects key code as first parameter, and an optional handler class instance as second parameter.");
		return NULL;
	}

	g_keyEventSinks.push_back( std::make_pair(KeyCode::Key(keycode), pyhandler) );
	Py_XINCREF(pyhandler);

	Py_RETURN_NONE;
}
PY_MODULE_FUNCTION( sinkKeyEvents, BigWorld )


// -----------------------------------------------------------------------------
// Section: BigWorld module: Basic script services (callbacks and consoles)
// -----------------------------------------------------------------------------

/*~ function BigWorld.time
 *
 *	This function returns the client time.  This is the time that the player's
 *	entity is currently ticking at.  Other entities are further back in time,
 *	the further they are from the player entity.
 *
 *	@return		a float.  The time on the client.
 */
/**
 *	Returns the current time.
 */
static float time()
{
	BW_GUARD;
	return float(App::instance().getGameTimeFrameStart());
}
PY_AUTO_MODULE_FUNCTION( RETDATA, time, END, BigWorld )


/*~ function BigWorld.serverTime
 *
 *	This function returns the server time.  This is the time that all entities
 *	are at, as far as the server itself is concerned.  This is different from
 *	the value returned by the BigWorld.time() function, which is the time on
 *	the client.
 *
 *	@return	A float. This is the current time on the server (negative if no
 *		server connection).
 */
/**
 *	@return The current server time (negative if no server connection).
 */
static double serverTime()
{
	BW_GUARD;

	BWConnection * pConnection = ConnectionControl::instance().pConnection();

	if (pConnection == NULL)
	{
		return -1.f;
	}

	return pConnection->serverTime();
}
PY_AUTO_MODULE_FUNCTION( RETDATA, serverTime, END, BigWorld )


// -----------------------------------------------------------------------------
// Section: BWExceptionHook
// -----------------------------------------------------------------------------
static PyObject * py__BWExceptHook( PyObject * args )
{
	BW_GUARD;	
	PyObject* internalHook = PySys_GetObject( "__excepthook__" );
	if (!internalHook)
	{
		CRITICAL_MSG( "_BWExceptHook: sys.__excepthook__ does not exist!" );
		return NULL;
	}

	// Turn off warnings about accessing files from the main thread and then
	// call the normal exception hook.
	BWResource::WatchAccessFromCallingThreadHolder holder( false );
	return PyObject_CallObject( internalHook, args );
}
PY_MODULE_FUNCTION( _BWExceptHook, BigWorld )



// -----------------------------------------------------------------------------
// Section: BigWorldClientScript namespace functions
// -----------------------------------------------------------------------------

/// Reference the modules we want to use, to make sure they are linked in.
extern int BoundingBoxGUIComponent_token;
extern int PySceneRenderer_token;
static int GUI_extensions_token =
	BoundingBoxGUIComponent_token |
	PySceneRenderer_token;

extern int FlexiCam_token;
extern int CursorCamera_token;
extern int FreeCamera_token;
static int Camera_token =
	FlexiCam_token |
	CursorCamera_token |
	FreeCamera_token;

extern int PyChunkModel_token;
static int Chunk_token = PyChunkModel_token;

extern int Math_token;
extern int GUI_token;
extern int ResMgr_token;
extern int Pot_token;
extern int PyScript_token;
extern int PyLogging_token;
extern int PyURLRequest_token;
static int s_moduleTokens =
	Math_token |
	GUI_token |
	GUI_extensions_token |
	ResMgr_token |
	Camera_token |
	Chunk_token |
	Pot_token |
	PyScript_token |
	PyLogging_token |
	PyURLRequest_token;


//extern void memNow( const BW::string& token );


template <class T>
void addToModule( PyObject * pModule, T value, const char * pName )
{
	PyObject * pObj = Script::getData( value );
	if (pObj)
	{
		PyObject_SetAttrString( pModule, pName, pObj );
		Py_DECREF( pObj );
	}
	else
	{
		PyErr_Clear();
	}
}


/**
 *	Client scripting initialisation function
 */
bool BigWorldClientScript::init( DataSectionPtr engineConfig )
{
	BW_GUARD;	
//	memNow( "Before base script init" );

	// Particle Systems are creatable from Python code
	MF_VERIFY( ParticleSystemManager::init() );

	// Call the general init function
	PyImportPaths paths;
	paths.addResPath( EntityDef::Constants::entitiesClientPath() );
	paths.addResPath( EntityDef::Constants::userDataObjectsClientPath() );

	if (!Script::init( paths, "client" ) )
	{
		return false;
	}

	if (!MaterialKinds::init())
	{
		ERROR_MSG( "BigWorldClientScript::init: failed to initialise MaterialKinds\n" );
		return false;
	}

	if (!Pickler::init())
	{
		return false;
	}

	// Initialise the BigWorld module
	PyObject * pBWModule = PyImport_AddModule( "BigWorld" );

	PyObjectPtr protocolVersion( PyString_FromString( 
			ClientServerProtocolVersion::currentVersion().c_str() ),
		PyObjectPtr::STEAL_REFERENCE );
	if (PyObject_SetAttrString( pBWModule, "protocolVersion",
			protocolVersion.get() ) == -1)
	{
		return false;
	}
	protocolVersion = NULL;

	// Set the 'Entity' class into it as an attribute
	if (PyObject_SetAttrString( pBWModule, "Entity",
			(PyObject *)&PyEntity::s_type_ ) == -1)
	{
		return false;
	}
	// Set the 'UserDataObject' class into it as an attribute
	if (PyObject_SetAttrString( pBWModule, "UserDataObject",
			(PyObject *)&UserDataObject::s_type_ ) == -1)
	{
		return false;
	}

	// Override the exception hook to avoid warnings during tracebacks
	PyObject* bwExceptHook = PyObject_GetAttrString( pBWModule, "_BWExceptHook" );
	if (bwExceptHook)
	{
		PySys_SetObject( "excepthook", bwExceptHook );
		Py_DecRef( bwExceptHook );
	}

	// Insert physics constants
#	define INSERT_PHYSICS_CONSTANT( NAME )									\
	PyModule_AddIntConstant( pBWModule, #NAME, Physics::NAME );				\

	INSERT_PHYSICS_CONSTANT( DUMMY_PHYSICS );
	INSERT_PHYSICS_CONSTANT( STANDARD_PHYSICS );
	INSERT_PHYSICS_CONSTANT( HOVER_PHYSICS );
	INSERT_PHYSICS_CONSTANT( CHASE_PHYSICS );
	INSERT_PHYSICS_CONSTANT( TURRET_PHYSICS );

#	undef INSERT_PHYSICS_CONSTANT

	// Insert ConnectionControl constants
#	define INSERT_CONNECTION_CONTROL_CONSTANT( NAME )									\
	PyModule_AddIntConstant( pBWModule, #NAME, ConnectionControl::NAME );				\

	INSERT_CONNECTION_CONTROL_CONSTANT( STAGE_INITIAL );
	INSERT_CONNECTION_CONTROL_CONSTANT( STAGE_LOGIN );
	INSERT_CONNECTION_CONTROL_CONSTANT( STAGE_DATA );
	INSERT_CONNECTION_CONTROL_CONSTANT( STAGE_DISCONNECTED );

#	undef INSERT_CONNECTION_CONTROL_CONSTANT

	// Load all the standard entity scripts
	bool ret = EntityType::init();
	// Load all the User Data Object Types
	ret = ret && UserDataObjectType::init();

	if (ret)
	{
		PyObjectPtr digestString( PyString_FromString( 
			EntityType::entityDefConstants().digest().quote().c_str() ),
			PyObjectPtr::STEAL_REFERENCE );

		if (PyObject_SetAttrString( pBWModule, "digest", 
				digestString.get() ) == -1)
		{
			return false;
		}
		digestString = NULL;
	}

	addToModule( pBWModule, (uint16) TRIANGLE_TERRAIN, "TRIANGLE_TERRAIN" );

	// By default, import some modules onto the __main__ module so that they're
	// available on the Python console.
	PyRun_SimpleString("import BigWorld, GUI, Math, Pixie, Keys");
	PyErr_Clear();

	return ret;
}


/**
 *	Client scripting termination function
 */
void BigWorldClientScript::fini()
{
	BW_GUARD;

	for (KeyPyObjectPairList::iterator it = g_keyEventSinks.begin(); 
		it != g_keyEventSinks.end(); it++)
	{
		Py_XDECREF(it->second);
	}

	g_keyEventSinks.clear();

	MaterialKinds::fini();
	Script::fini();
	EntityType::fini();
	MetaDataType::fini();
	ParticleSystemManager::fini();
}

/**
 *	Does per-frame house keeping.
 */
void BigWorldClientScript::tick()
{
}

/**
 *	Posts the given event off to the script system 
 */
bool BigWorldClientScript::sinkKeyboardEvent( const InputEvent& event )
{
	if (g_keyEventSinks.empty())
	{
		return false;
	}

	bool handled = false;

	// Remember which item is the last one because the Python callbacks
	// may add another item to the list which we don't want to process
	// until next time through.
	KeyPyObjectPairList::iterator lastIt = --g_keyEventSinks.end();

	// Keep going until someone reports it as handled
	KeyPyObjectPairList::iterator it = g_keyEventSinks.begin();

	while (it != lastIt && !handled)
	{
		bool remove = false;
		KeyCode::Key sinkKey = it->first;
		PyObject* pyhandler = it->second;

		switch( event.type_ )
		{
		case InputEvent::KEY:
			{
				const KeyEvent& keyEvent = event.key_;
				if ( sinkKey == keyEvent.key() )
				{
					if ( pyhandler )
					{
						PyObject * ret = 
							Script::ask(PyObject_GetAttrString( pyhandler, "handleKeyEvent" ),
										Script::getData( keyEvent ),
										"Keyboard event sink handleKeyEvent: " );

						Script::setAnswer( ret, handled, "Keyboard event sink handleKeyEvent retval" );
					}
					else
					{
						handled = true;
					}

					// pop the event off if this is the keyup.
					remove = keyEvent.isKeyUp();
				}
				break;
			}

		default:
			break;
		}

		if (remove)
		{
			it = g_keyEventSinks.erase(it);
			Py_XDECREF(pyhandler);
		}
		else
		{
			++it;
		}
	}

	return handled;
}


/**
 *	Returns the next SpaceEntryID unique within this process.
 */
SpaceEntryID BigWorldClientScript::nextSpaceEntryID()
{
	static uint16 salt = 0;

	SpaceEntryID spaceEntryID;
	spaceEntryID.salt = salt++;

	return spaceEntryID;
}


// TODO:PM This is probably not the best place for this.
/**
 *	This function adds an alert message to the display.
 */
bool BigWorldClientScript::addAlert( const char * alertType, const char * alertName )
{
	BW_GUARD;
	bool succeeded = false;

	PyObjectPtr pModule = PyObjectPtr(
			PyImport_ImportModule( "Helpers.alertsGui" ),
			PyObjectPtr::STEAL_REFERENCE );

	if (pModule)
	{
		PyObjectPtr pInstance = PyObjectPtr(
			PyObject_GetAttrString( pModule.getObject(), "instance" ),
			PyObjectPtr::STEAL_REFERENCE );

		if (pInstance)
		{
			PyObjectPtr pResult = PyObjectPtr(
				PyObject_CallMethod( pInstance.getObject(),
									"add", "ss", alertType, alertName ),
				PyObjectPtr::STEAL_REFERENCE );

			if (pResult)
			{
				succeeded = true;
			}
		}
	}

	if (!succeeded)
	{
		PyErr_PrintEx(0);
		WARNING_MSG( "BigWorldClientScript::addAlert: Call failed.\n" );
	}

	return succeeded;
}


/**
 *	This method sets the username that has been set from the command line.
 */
void BigWorldClientScript::setUsername( const BW::string & username )
{
	g_cmdLineUsername = username;
	g_hasCmdLineUsername = true;
}


/**
 *	This method sets the password that has been set from the command line.
 */
void BigWorldClientScript::setPassword( const BW::string & password )
{
	g_cmdLinePassword = password;
	g_hasCmdLinePassword = true;
}

/*~	function BigWorld.commandLineLoginInfo
 *	@components{ client }
 *
 *	This function returns the username and password that where specified on
 *	the command line as a tuple. If none were specified, None is returned.
 *
 *	The command line flags are --username and --password. (-u and -p can
 *	also be used).
 */
namespace
{
PyObject * commandLineLoginInfo()
{
	if (!g_hasCmdLineUsername && !g_hasCmdLinePassword)
	{
		Py_RETURN_NONE;
	}

	PyObjectPtr pUsername = Script::getData( g_cmdLineUsername );
	PyObjectPtr pPassword = Script::getData( g_cmdLinePassword );

	if (!g_hasCmdLineUsername)
	{
		pUsername = Py_None;
	}

	if (!g_hasCmdLinePassword)
	{
		pPassword = Py_None;
	}

	return PyTuple_Pack( 2, pUsername.get(), pPassword.get() );
}

}
PY_AUTO_MODULE_FUNCTION( RETOWN, commandLineLoginInfo, END, BigWorld )


/**
 *	Wrapper for list of strings used by the
 *	createTranslationOverrideAnim method.
 */
class MyFunkySequence : public PySTLSequenceHolder< BW::vector<BW::string> >
{
public:
	MyFunkySequence() :
		PySTLSequenceHolder< BW::vector<BW::string> >( strings_, NULL, true )
	{}

	BW::vector<BW::string>	strings_;
};

/*~ function BigWorld createTranslationOverrideAnim
 *  This function is a tool which can be used to alter skeletal animations so
 *  that they can be used with models which have skeletons of different
 *  proportions. This is achieved by creating a new animation which is based
 *  on a given animation, but replaces the translation component for each node
 *  with that of the beginning of the same node in a reference animation. As
 *  the translation should not change in a skeletal system (bones do not change
 *  size or shape), this effectively re-fits the animation on to a differently
 *  proportioned model. This operates by creating a new animation file, and
 *  is not intended for in-game use.
 *  @param baseAnim A string containing the name (including path) of the
 *  animation file on which the new file is to be based.
 *  @param translationReferenceAnim A string containing the name (including path)
 *  of the animation file whose first frame contains the translation which will
 *  be used for the new animation. This should have the same proportions as are
 *  desired for the new animation.
 *  @param noOverrideChannels A list of strings containing the names of the nodes
 *  that shouldn't have their translation overridden. These nodes will not be
 *  scaled to the proportions provided by translationReferenceAnim in the new
 *  animation.
 *  @param outputAnim A string containing the name (including path) of the
 *  animation file to which the new animation will be saved.
 *  @return None
 */
static void createTranslationOverrideAnim( const BW::string& baseAnim,
										  const BW::string& translationReferenceAnim,
										  const MyFunkySequence& noOverrideChannels,
										  const BW::string& outputAnim )
{
	BW_GUARD;
	Moo::AnimationPtr pBase = Moo::AnimationManager::instance().find( baseAnim );
	if (!pBase.hasObject())
	{
		ERROR_MSG( "createTranslationOverrideAnim - Unable to open animation %s\n", baseAnim.c_str() );
		return;
	}

	Moo::AnimationPtr pTransRef = Moo::AnimationManager::instance().find( translationReferenceAnim );
	if (!pTransRef.hasObject())
	{
		ERROR_MSG( "createTranslationOverrideAnim - Unable to open animation %s\n", translationReferenceAnim.c_str() );
		return;
	}
	Moo::AnimationPtr pNew = new Moo::Animation();

	pNew->translationOverrideAnim( pBase, pTransRef, noOverrideChannels.strings_ );

	pNew->save( outputAnim );
}

PY_AUTO_MODULE_FUNCTION( RETVOID, createTranslationOverrideAnim, ARG(
	BW::string, ARG( BW::string, ARG( MyFunkySequence, ARG( BW::string,
	END ) ) ) ), BigWorld )

#if ENABLE_WATCHERS
/*~ function BigWorld.memUsed
 *	@components{ client }
 *
 *	This function returns an estimate of the amount of memory the application
 *	is currently using.
 */
extern uint32 memUsed();
PY_AUTO_MODULE_FUNCTION( RETDATA, memUsed, END, BigWorld )
#endif

/*~ function BigWorld.screenWidth
 *	Returns the width of the current game window.
 *	@return float
 */
float screenWidth()
{
	BW_GUARD;
	return Moo::rc().screenWidth();
}
PY_AUTO_MODULE_FUNCTION( RETDATA, screenWidth, END, BigWorld )

/*~ function BigWorld.screenHeight
 *	Returns the height of the current game window.
 *	@return float
 */
float screenHeight()
{
	BW_GUARD;
	return Moo::rc().screenHeight();
}
PY_AUTO_MODULE_FUNCTION( RETDATA, screenHeight, END, BigWorld )

/*~ function BigWorld.screenSize
 *	Returns the width and height of the current game window as a tuple.
 *	@return (float, float)
 */
PyObject * screenSize()
{
	BW_GUARD;
	float width = Moo::rc().screenWidth();
	float height = Moo::rc().screenHeight();
	PyObject * pTuple = PyTuple_New( 2 );
	PyTuple_SetItem( pTuple, 0, Script::getData( width ) );
	PyTuple_SetItem( pTuple, 1, Script::getData( height ) );
	return pTuple;
}
PY_AUTO_MODULE_FUNCTION( RETOWN, screenSize, END, BigWorld )


/*~ function BigWorld.screenShot
 *	This method takes a screenshot and writes the image to disk. The output folder
 *	is configured by the 'screenShot/path' section in engine_config.xml.
 *	@param format Optional string. The format of the screenshot to be outputed,
 *  can be on of "bmp", "jpg", "tga", "png" or "dds". The default comes from resources.xml.
 *  @param name Optional string. This is the root name of the screenshot to generate.
 *  A unique number will be postpended to this string. The default comes from resources.xml.
 */
 void screenShot( BW::string format, BW::string name )
{
	BW_GUARD;
	DataSectionPtr settingsDS = 
		AppConfig::instance().pRoot()->openSection( "screenShot/path" );

	PathedFilename pathedFile( settingsDS,
								"", PathedFilename::BASE_EXE_PATH );

	BW::string fullName = pathedFile.resolveName() + "/" + name;
	BWResource::ensureAbsolutePathExists( fullName );

	Moo::rc().screenShot( format, fullName );
}
PY_AUTO_MODULE_FUNCTION( RETVOID, screenShot,
	OPTARG( BW::string, AppConfig::instance().pRoot()->readString("screenShot/extension", "bmp"),
	OPTARG( BW::string, AppConfig::instance().pRoot()->readString("screenShot/name", "shot"), END ) ), BigWorld )


/*~ function BigWorld.connectedEntity
 *	This method returns the entity that this application is connected to. The
 *	connected entity is the server entity that is responsible for collecting and
 *	sending data to this client application. It is also the only client entity
 *	that has an Entity.base property.
 */
static PyEntity * connectedEntity()
{
	BW_GUARD;

	Entity * pPlayer = ConnectionControl::instance().pPlayer();

	if (pPlayer == NULL)
	{
		return NULL;
	}

	MF_ASSERT( !pPlayer->isDestroyed() );

	return pPlayer->pPyEntity().get();
}
PY_AUTO_MODULE_FUNCTION( RETDATA, connectedEntity, END, BigWorld )


/*~ function BigWorld.savePreferences
 *
 *  Saves the current preferences (video, graphics and script) into
 *	a XML file. The name of the file to be written is defined in the
 *	<preferences> field in engine_config.xml.
 *
 *	@return		bool	True on success. False on error.
 */
static bool savePreferences()
{
	BW_GUARD;
	BWResource::WatchAccessFromCallingThreadHolder watchAccess( false );
	return App::instance().savePreferences();
}
PY_AUTO_MODULE_FUNCTION( RETDATA, savePreferences, END, BigWorld )

typedef BW::map< PyObject* , BW::vector<PyObject*> > ReferersMap;
/*~ function BigWorld.dumpRefs
 *	Dumps references to each object visible from the entity tree.
 *	This does not include all objects, and may not include all references.
 */
void dumpRefs()
{
	BW_GUARD;
	// Build a map of the objects that reference each other object.
	ReferersMap	referersMap;

	PyObject * pSeed = PyImport_AddModule( "BigWorld" );
	referersMap[pSeed].push_back( pSeed );	// refers to itself for seeding

	BW::vector<PyObject*> stack;
	stack.push_back( pSeed );
	while (!stack.empty())
	{
		PyObject * pLook = stack.back();
		stack.pop_back();

		// go through all objects accessible from here

		// look in the dir and get attributes
		PyObject * pDirSeq = PyObject_Dir( pLook );
		Py_ssize_t dlen = 0;
		if (pDirSeq != NULL)
		{
			dlen = PySequence_Length( pDirSeq );
		} else { PyErr_Clear(); }
		for (Py_ssize_t i = 0; i < dlen; i++)
		{
			PyObject * pRefereeName = PySequence_GetItem( pDirSeq, i );
			PyObject * pReferee = PyObject_GetAttr( pLook, pRefereeName );
			Py_DECREF( pRefereeName );
			if (pReferee == NULL)
			{
				// shouldn't get errors like this (hmmm)
				PyErr_Clear();
				WARNING_MSG( "%s in dir of 0x%p but cannot access it\n",
					PyString_AsString( pRefereeName ), pLook );
				continue;
			}
			if (pReferee->ob_refcnt == 1)
			{
				// if it was created just for us we don't care
				Py_DECREF( pReferee );
				continue;
			}
			// ok we have an object that is part of the tree in pReferee

			// find/create the vector of referers to pReferee
			BW::vector<PyObject*> & referers = referersMap[pReferee];
			// if pLook is first to refer to this obj then traverse it (later)
			if (referers.empty())
				stack.push_back( pReferee );
			// record the fact that pLook refers to this obj
			referers.push_back( pLook );

			Py_DECREF( pReferee );
		}
		Py_XDECREF( pDirSeq );

		// look in the sequence
		Py_ssize_t slen = 0;
		if (PySequence_Check( pLook )) slen = PySequence_Size( pLook );
		for (Py_ssize_t i = 0; i < slen; i++)
		{
			PyObject * pReferee = PySequence_GetItem( pLook, i );
			if (pReferee == NULL)
			{
				// _definitely_ shouldn't get errors like this! (but do)
				PyErr_Clear();
				WARNING_MSG( "%d seq in 0x%p but cannot access item %d\n",
					static_cast<int>(slen), pLook, static_cast<int>(i) );
				continue;
			}
			MF_ASSERT_DEV( pReferee != NULL );
			if (pReferee->ob_refcnt == 1)
			{
				// if it was created just for us we don't care
				Py_DECREF( pReferee );
				continue;
			}

			// find/create the vector of referers to pReferee
			BW::vector<PyObject*> & referers = referersMap[pReferee];
			// if pLook is first to refer to this obj then traverse it (later)
			if (referers.empty())
				stack.push_back( pReferee );
			// record the fact that pLook refers to this obj
			referers.push_back( pLook );

			Py_DECREF( pReferee );
		}

		// look in the mapping
		PyObject * pMapItems = NULL;
		Py_ssize_t mlen = 0;
		if (PyMapping_Check( pLook ))
		{
			pMapItems = PyMapping_Items( pLook );
			mlen = PySequence_Size( pMapItems );
		}
		for (Py_ssize_t i = 0; i < mlen; i++)
		{
		  PyObject * pTuple = PySequence_GetItem( pMapItems, i );
		  Py_ssize_t tlen = PySequence_Size( pTuple );
		  for (Py_ssize_t j = 0; j < tlen; j++)
		  {
			PyObject * pReferee = PySequence_GetItem( pTuple, j );
			MF_ASSERT_DEV( pReferee != NULL );
			if (pReferee->ob_refcnt == 2)
			{
				// if it was created just for us we don't care
				Py_DECREF( pReferee );
				continue;
			}

			// find/create the vector of referers to pReferee
			BW::vector<PyObject*> & referers = referersMap[pReferee];
			// if pLook is first to refer to this obj then traverse it (later)
			if (referers.empty())
				stack.push_back( pReferee );
			// record the fact that pLook refers to this obj
			referers.push_back( pLook );

			Py_DECREF( pReferee );
		  }
		  Py_DECREF( pTuple );
		}
		Py_XDECREF( pMapItems );
	}

	time_t now = ::time( &now );
	BW::string nowStr = ctime( &now );
	nowStr.erase( nowStr.end()-1 );
	FILE * f = BWResource::instance().fileSystem()->posixFileOpen(
		"py ref table.txt", "a" );
	fprintf( f, "\n" );
	fprintf( f, "List of references to all accessible from 'BigWorld':\n" );
	fprintf( f, "(as at %s)\n", nowStr.c_str() );
	fprintf( f, "-----------------------------------------------------\n" );

	// Now print out all the objects and their referers
	ReferersMap::iterator it;
	for (it = referersMap.begin(); it != referersMap.end(); it++)
	{
		PyObject * pReferee = it->first;
		PyObject * pRefereeStr = PyObject_Str( pReferee );
		fprintf( f, "References to object at 0x%p type %s "
				"aka '%s' (found %d/%d):\n",
			pReferee, pReferee->ob_type->tp_name,
			PyString_AsString( pRefereeStr ),
			it->second.size(), pReferee->ob_refcnt );
		Py_DECREF( pRefereeStr );

		for (uint i = 0; i < it->second.size(); i++)
			fprintf( f, "\t0x%p\n", it->second[i] );
		fprintf( f, "\n" );
	}

	fprintf( f, "-----------------------------------------------------\n" );
	fprintf( f, "\n" );
	fclose( f );
}
PY_AUTO_MODULE_FUNCTION( RETVOID, dumpRefs, END, BigWorld )


/*~ function BigWorld.reloadChunks
 *	Unload all chunks, purge all ".chunk" data sections and begin reloading
 *	the space.
 */
void reloadChunks()
{
	BW_GUARD;
	// remember the camera's chunk
	Chunk * pCameraChunk = ChunkManager::instance().cameraChunk();
	if (pCameraChunk)
	{
		GeometryMapping * pCameraMapping = pCameraChunk->mapping();
		ChunkSpacePtr pSpace = pCameraChunk->space();

		// unload every chunk in sight
		ChunkMap & chunks = pSpace->chunks();
		for (ChunkMap::iterator it = chunks.begin(); it != chunks.end(); it++)
		{
			for (uint i = 0; i < it->second.size(); i++)
			{
				Chunk * pChunk = it->second[i];
				if (pChunk->isBound())
				{
					BWResource::instance().purge( pChunk->resourceID() );
					pChunk->unbind( false );
					pChunk->unload();
				}
			}
		}

		// now reload the camera chunk
		ChunkManager::instance().loadChunkExplicitly(
			pCameraChunk->identifier(), pCameraMapping );

		// and repopulate the flora
		Flora::floraReset();
	}
}
PY_AUTO_MODULE_FUNCTION( RETVOID, reloadChunks, END, BigWorld )

/*~	function BigWorld.saveAllocationsToFile
 *
 *  Save all recorded memory allocations into the text file for further analysis
 *  File format description:
 *
 *	number_of_callstack_hashes
 *  64bit hash;callstack
 *  64bit hash;callstack
 *  ....
 *  64bit hash;callstack
 *  slotId;64bit callstack hash;allocation size
 *  ...
 *  slotId;64bit callstack hash;allocation size
 *
 *  Use condense_mem_allocs.py script to convert saved file into the following format:
 *  slot id;allocationSize;number of allocations;callstack
 *  ...
 *
 *	@param filename 	The name of file to save data
 */
void saveAllocationsToFile( const BW::string& filename )
{
	Allocator::saveAllocationsToFile( filename.c_str() );
}
PY_AUTO_MODULE_FUNCTION( RETVOID, saveAllocationsToFile, ARG( BW::string, END ), BigWorld );

/*~	function BigWorld.saveAllocationStatsToFile
 *
 *  Save the current memory statistics of the process to a CSV file.
 *  For each allocation slot, the following statistics will be output;
 *  Peak allocation, Total number of allocations, Live allocation, and Live allocation count.
 *
 *	@param filename 	The name of file to save data
 */
void saveAllocationStatsToFile( const BW::string& filename )
{
	Allocator::saveStatsToFile( filename.c_str() );
}
PY_AUTO_MODULE_FUNCTION( RETVOID, saveAllocationStatsToFile, ARG( BW::string, END ), BigWorld );

/*~	function BigWorld.saveAllocationsToCacheGrindFile
 *
 *	Save current memory allocations to callgrind format for loading into
 *	KCacheGrind.
 *
 *	@param filename 	The name of file to save data
 */
void saveAllocationsToCacheGrindFile( const BW::string & filename )
{
	Allocator::saveAllocationsToCacheGrindFile( filename.c_str() );
}
PY_AUTO_MODULE_FUNCTION( RETVOID, saveAllocationsToCacheGrindFile, ARG( BW::string, END ), BigWorld );

/*~	function BigWorld.saveMemoryUsagePerTexture
 *
 *  Save information about currently loaded textures
 *
 */
void saveMemoryUsagePerTexture( const BW::string& filename )
{
	Moo::TextureManager::instance()->saveMemoryUsagePerTexture( filename.c_str() );
}

PY_AUTO_MODULE_FUNCTION( RETVOID, saveMemoryUsagePerTexture, ARG( BW::string, END ), BigWorld );

/*~	function BigWorld.getMemoryInfoKB
 *
 *  Get memory info in kilobytes.
 *	NOTE: Value in bytes come as 64bit ints and can be larger than 4GB.
 *        There appears to be no script int support for this. Hence use of kb.
 */
PyObject * getMemoryInfoKB()
{
	Moo::GpuInfo::MemInfo memInfo;
	Moo::rc().getGpuMemoryInfo( &memInfo );

	ScriptDict retVal = ScriptDict::create();
	ScriptErrorPrint errPrint( "BigWorld.getMemoryInfoKB" );

	retVal.setItem("systemMemReserved", ScriptInt::create(static_cast<long>(memInfo.systemMemReserved_/1024)), errPrint);
	retVal.setItem("systemMemUsed", ScriptInt::create(static_cast<long>(memInfo.systemMemUsed_/1024)), errPrint);
	retVal.setItem("dedicatedMemTotal", ScriptInt::create(static_cast<long>(memInfo.dedicatedMemTotal_/1024)), errPrint);
	retVal.setItem("dedicatedMemCommitted", ScriptInt::create(static_cast<long>(memInfo.dedicatedMemCommitted_/1024)), errPrint);
	retVal.setItem("sharedMemTotal", ScriptInt::create(static_cast<long>(memInfo.sharedMemTotal_/1024)), errPrint);
	retVal.setItem("sharedMemCommitted", ScriptInt::create(static_cast<long>(memInfo.sharedMemCommitted_/1024)), errPrint);
	retVal.setItem("virtualAddressSpaceTotal", ScriptInt::create(static_cast<long>(memInfo.virtualAddressSpaceTotal_/1024)), errPrint);
	retVal.setItem("virtualAddressSpaceUsage", ScriptInt::create(static_cast<long>(memInfo.virtualAddressSpaceUsage_/1024)), errPrint);

	return retVal.newRef();
}

PY_AUTO_MODULE_FUNCTION( RETOWN, getMemoryInfoKB, END, BigWorld );


/*~	function BigWorld.setTextureStreamingMode
 *
 *  adjust the debug set of textures
 *
 */
void setTextureStreamingMode( uint32 mode )
{
	Moo::TextureManager::instance()->streamingManager()->debugMode( 
		(Moo::DebugTextures::TextureSet)mode);
}

PY_AUTO_MODULE_FUNCTION( RETVOID, setTextureStreamingMode, ARG( uint32, END ), BigWorld );

/*~	function BigWorld.setTextureMinMip
 *
 *  adjust the debug set of textures
 *
 */
void setTextureMinMip( uint32 size )
{
	Moo::TextureManager::instance()->streamingManager()->minMipLevel( 
		size);
}

PY_AUTO_MODULE_FUNCTION( RETVOID, setTextureMinMip, ARG( uint32, END ), BigWorld );

/*~ function BigWorld.registerTextureStreamingViewpoint
 *
 * Add a viewpoint reference to the texture streaming system.
 * @param pObjCamera Camera object defining direction and position of a 
 *     camera that is used for rendering.
 * @param pObjProjection Projection object defining projection matrix
 *     used when the given camera is rendering.
 *
 * Example usage:
 * BigWorld.registerTextureStreamingViewpoint(BigWorld.camera(), BigWorld.projection())
 */
void registerTextureStreamingViewpoint( BaseCameraPtr pObjCamera, ProjectionAccessPtr pObjProjection )
{
	BaseCamera* pCam = pObjCamera.get();
	ProjectionAccess* pProj = pObjProjection.get();

	// TODO: Move this class definition into the camera library alongside
	// BaseCamera and ProjectionAccess.
	class CameraViewpoint : public Moo::TextureStreamingManager::Viewpoint
	{
	public:
		CameraViewpoint( BaseCamera * pCam, ProjectionAccess * pProj ) :
			camera_(pCam),
			proj_(pProj)
		{

		}

		virtual QueryReturnCode query( Moo::TextureStreamingManager::ViewpointData & data )
		{
			if (!camera_ || !proj_)
			{
				return UNREGISTER;
			}

			data.matrix = camera_->invView();
			data.fov = proj_->fov();
			data.resolution = static_cast< float >(Moo::rc().backBufferDesc().Width);

			return SUCCESS;
		}

		WeakPyPtr< BaseCamera > camera_;
		WeakPyPtr< ProjectionAccess > proj_;
	};

	Moo::TextureManager::instance()->streamingManager()->registerViewpoint( 
		new CameraViewpoint( pCam, pProj ) );
}

PY_AUTO_MODULE_FUNCTION( RETVOID, registerTextureStreamingViewpoint, ARG( BaseCameraPtr, ARG(ProjectionAccessPtr, END )), BigWorld );

/*~	function BigWorld.clearTextureStreamingViewpoints
 *
 * Remove all viewpoints from the texture streaming system.
 */
void clearTextureStreamingViewpoints()
{
	Moo::TextureManager::instance()->streamingManager()->clearViewpoints();
}

PY_AUTO_MODULE_FUNCTION( RETVOID, clearTextureStreamingViewpoints, END , BigWorld );

/*~	function BigWorld.clearTextureReuseList
 *
 * Remove all textures currently in the reuse cache.
 */
void clearTextureReuseList()
{
	Moo::rc().clearTextureReuseList();
}

PY_AUTO_MODULE_FUNCTION( RETVOID, clearTextureReuseList, END , BigWorld );

/*~	function BigWorld.navigatePathPoints
 * 	@components{ client }
 *
 * 	Return a path of points between the given source and destination points in
 * 	the camera space.
 *
 * 	@param src	(Vector3)		The source point in the space.
 * 	@param dst	(Vector3)		The destination point in the space.
 * 	@param maxSearchDistance (float)
 * 								The maximum search distance, defaults to 500m.
 * 	@param girth (float) 		The navigation girth grid to use, defaults to
 * 								0.5m.
 *
 * 	@return (list) 	A list of Vector3 points between the source point to the
 * 					destination point.
 *
 */
PyObject * navigatePathPoints( const Vector3 & src, const Vector3 & dst,
							  float maxSearchDistance, float girth )
{
	ChunkSpacePtr pChunkSpace = ChunkManager::instance().cameraSpace();

	if (!pChunkSpace)
	{
		PyErr_Format( PyExc_ValueError, "BigWorld.navigatePathPoints: "
			"Cannot get camera space" );

		return NULL;
	}

	BW::vector<Vector3> pathPoints;
	static Navigator navigator;
	bool result = false;

	result = navigator.findFullPath( pChunkSpace.get(), src, dst,
		maxSearchDistance, true, girth, pathPoints );

	if (result)
	{
		MF_ASSERT( !pathPoints.empty() );

		if (!almostEqual( pathPoints.front(), src )) 
		{
			pathPoints.insert( pathPoints.begin(), src );
		}

		PyObject * pList = PyList_New( pathPoints.size() );

		Vector3Path::const_iterator iPoint = pathPoints.begin();

		while (iPoint != pathPoints.end())
		{
			PyList_SetItem( pList, iPoint - pathPoints.begin(), 
				Script::getData( *iPoint ) );
			++iPoint;
		}

		return pList;
	}

	PyErr_Format( PyExc_ValueError, "BigWorld.navigatePathPoints:"
		" Cannot find path points. Check that client side navigation is"
		" enabled in space.settings." );

	return NULL;
}

PY_AUTO_MODULE_FUNCTION( RETOWN, navigatePathPoints, ARG( Vector3, 
		ARG( Vector3, OPTARG( float, 500.f, OPTARG( float, 0.5f, END ) ) ) ), 
	BigWorld )


/**
 *	Find a point nearby random point in a connected navmesh
 *
 *	@param funcName 	The name of python function for error report
 *	@param position	The position of the point
 *	@param minRadius	The minimum radius to search
 *	@param maxRadius	The maximum radius to search
 *	@param girth	Which navigation girth to use (optional and default to 0.5)
 *	@return			The random point found, as a Vector3
 */
PyObject * findRandomNeighbourPointWithRange( const char* funcName, 
		Vector3 position, float minRadius, float maxRadius, float girth )
{
	ChunkSpacePtr pChunkSpace = ChunkManager::instance().cameraSpace();

	if (!pChunkSpace)
	{
		PyErr_Format( PyExc_ValueError, "%s: Cannot get camera space",
			funcName );

		return NULL;
	}

	static Navigator navigator;

	Vector3 result;

	if (navigator.findRandomNeighbourPointWithRange( pChunkSpace.get(),
		position, minRadius, maxRadius, girth, result ))
	{
		return Script::getData( result );
	}

	PyErr_Format( PyExc_ValueError, "%s: Failed to find neighbour point",
		funcName );

	return NULL;
}


/*~ function BigWorld findRandomNeighbourPointWithRange
 *  @components{ client }
 *	This function can be used to find a random point in a connected navmesh.
 *	The result point is guaranteed to be connectable to the point.
 *	Note that in some conditions the result point might be closer than
 *	minRadius.
 *
 *	@param position		(Vector3)			The position of the point
 *	@param minRadius	(float) 			The minimum radius to search
 *	@param maxRadius	(float) 			The maximum radius to search
 *	@param girth		(optional float) 	Which navigation girth to use
 *												(optional and default to 0.5)
 *	@return				(Vector3)			The random point found
 */
/**
 *	Find a point nearby random point in a connected navmesh
 *
 *	@param position	The position of the point
 *	@param minRadius	The minimum radius to search
 *	@param maxRadius	The maximum radius to search
 *	@param girth	Which navigation girth to use (optional and default to 0.5)
 *	@return			The random point found, as a Vector3
 */
PyObject * findRandomNeighbourPointWithRange(
		const Vector3 & position, float minRadius, float maxRadius,
		float girth = 0.5 )
{
	return findRandomNeighbourPointWithRange( 
		"BigWorld.findRandomNeighbourPointWithRange",
		position, minRadius, maxRadius, girth );
}
PY_AUTO_MODULE_FUNCTION( RETOWN, findRandomNeighbourPointWithRange,
	ARG( Vector3, ARG( float, ARG( float, OPTARG( float, 0.5f, END ) ) ) ),
	BigWorld )


/*~ function BigWorld findRandomNeighbourPoint
 *  @components{ client }
 *	This function can be used to find a random point in a connected navmesh.
 *	The result point is guaranteed to be connectable to the point.
 *
 *	@param position	(Vector3)			The position of the point
 *	@param radius	(float) 			The radius to search
 *	@param girth	(optional float) 	Which navigation girth to use
 *										(optional and default to 0.5)
 *	@return			(Vector3)			The random point found
 */
/**
 *	Find a point nearby random point in a connected navmesh
 *
 *	@param position	The position of the point
 *	@param radius	The radius to search
 *	@param girth	Which navigation girth to use (optional and default to 0.5)
 *	@return			The random point found, as a Vector3
 */
PyObject * findRandomNeighbourPoint(
	Vector3 position, float radius, float girth = 0.5 )
{
	return findRandomNeighbourPointWithRange(
		"BigWorld.findRandomNeighbourPoint", position, 0.f, radius, girth );
}
PY_AUTO_MODULE_FUNCTION( RETOWN, findRandomNeighbourPoint,
	ARG( Vector3, ARG( float, OPTARG( float, 0.5f, END ) ) ), BigWorld )


/*~	function BigWorld.isLoadingChunks
 *	@components{ client }
 *
 *	This function returns true if chunks are still being loaded
 *
 *	@return True if chunks are being loaded
 */
static bool isLoadingChunks()
{
	ChunkManager& cm = ChunkManager::instance();
	bool busy = cm.loadPending();
	return busy;
}
PY_AUTO_MODULE_FUNCTION( RETDATA, isLoadingChunks, END, BigWorld )


// -----------------------------------------------------------------------------
// Section: Spaces creation and clearing
// -----------------------------------------------------------------------------

/*~ function BigWorld.createLocalSpace
 *
 *	This function creates a local space.
 *
 *	@return spaceID ID of local space created
 *
 *	@see BigWorld.clearLocalSpace
 */
static SpaceID createLocalSpace()
{
	BW_GUARD;
	return ConnectionControl::instance().createLocalSpace();
}
PY_AUTO_MODULE_FUNCTION( RETDATA, createLocalSpace, END, BigWorld )


/*~ function BigWorld.clearLocalSpace
 *
 *	Clears entities, space data and geometry in a local space.
 *
 *	@param	spaceID	ID of local space to clear
 *
 *	@see BigWorld.createLocalSpace
 */
static void clearLocalSpace( SpaceID spaceID )
{
	BW_GUARD;

	SpaceManager & spaceManager = SpaceManager::instance();
	ClientSpacePtr pSpace = spaceManager.space( spaceID );
	
	if (!pSpace || !spaceManager.isLocalSpace( spaceID ))
	{
		PyErr_Format( PyExc_ValueError, "No local space %d", spaceID );
	}

	// Clear geometry
	pSpace->clear();

	// Clear entities and space data
	ConnectionControl::instance().clearLocalSpace( spaceID );
}
PY_AUTO_MODULE_FUNCTION( RETVOID, clearLocalSpace, ARG( SpaceID, END ), BigWorld )

	
/*~ function BigWorld.clearSpaces
 *
 *	Clears entities, space data and geometry in spaces of an online, offline,
 *	or replay connection. A new connection is allowed after this call.
 *
 *	@see BigWorld.clearLocalSpace()
 */
static void clearSpaces()
{
	BW_GUARD;

	// Clear geometry
	SpaceManager::instance().clearSpaces();

	// Clear entities and space data
	ConnectionControl::instance().clearSpaces();
}
PY_AUTO_MODULE_FUNCTION( RETVOID, clearSpaces, END, BigWorld )


BW_END_NAMESPACE

// script_bigworld.cpp
