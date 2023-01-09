#include "pch.hpp"
#include "worldeditor/project/project_module.hpp"
#include "worldeditor/project/space_map_debug.hpp"
#include "worldeditor/project/space_information.hpp"
#include "worldeditor/project/forced_lod_map.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "worldeditor/world/editor_chunk_cache.hpp"
#include "worldeditor/misc/world_editor_camera.hpp"
#include "worldeditor/misc/cvswrapper.hpp"
#include "worldeditor/import/terrain_utils.hpp"
#include "worldeditor/height/height_module.hpp"
#include "worldeditor/framework/world_editor_app.hpp"
#include "worldeditor/framework/mainframe.hpp"
#include "worldeditor/gui/pages/panel_manager.hpp"
#include "worldeditor/gui/dialogs/cvs_info_dialog.hpp"
#include "appmgr/app.hpp"
#include "appmgr/module_manager.hpp"
#include "appmgr/options.hpp"
#include "appmgr/closed_captions.hpp"
#include "moo/custom_mesh.hpp"
#include "romp/font_manager.hpp"
#include "romp/time_of_day.hpp"
#include "resmgr/bwresource.hpp"
#include "chunk/chunk_grid_size.hpp"
#include "chunk/chunk_manager.hpp"
#include "chunk/chunk_space.hpp"
#include "chunk/geometry_mapping.hpp"
#include "common/base_camera.hpp"
#include "ashes/simple_gui.hpp"
#include "chunk/chunk_format.hpp"
#include "cstdmf/memory_load.hpp"
#include "resmgr/string_provider.hpp"
#include "moo/camera.hpp"
#include "math/colour.hpp"


DECLARE_DEBUG_COMPONENT2( "ProjectModule", 2 );

BW_BEGIN_NAMESPACE


typedef ModuleManager ModuleFactory;

IMPLEMENT_CREATOR(ProjectModule, Module);

ProjectModule* ProjectModule::currentInstance_ = 0;

ProjectModule::ProjectModule() : gridSize_(0.f),
	gridWidth_(0), gridHeight_(0),minX_(0),minY_(0),
	selectionStart_( Vector2::zero() ),
	currentSelection_( GridRect::zero() ),
	currentSelectedCoord_( GridCoord::invalid() ),
	spaceAlpha_( 0.5f ),
	spaceColour_( 0x00ffffff ),
	localToWorld_( GridCoord::zero() ),
	mouseDownOn3DWindow_( false )
{
	BW_GUARD;

	lastCursorPosition_.x = -1;
	lastCursorPosition_.y = -1;

	currentInstance_ = this;

	shadow_ = new TextGUIComponent(  FontManager::instance().getCachedFont("default_medium.font")  );
	shadow_->filterType( SimpleGUIComponent::FT_LINEAR );
	shadow_->colour( 0xff000000 );
	shadow_->slimLabel( "" );
	SimpleGUI::instance().addSimpleComponent( *shadow_ );

	text_ = new TextGUIComponent(  FontManager::instance().getCachedFont("default_medium.font")  );
	text_->filterType( SimpleGUIComponent::FT_LINEAR );
	text_->colour( 0xffffffff );
	text_->slimLabel( "" );
	SimpleGUI::instance().addSimpleComponent( *text_ );

	font_ = FontManager::instance().get( Options::getOptionString( "project/normalFont", "system_medium.font" ) );
	boldFont_ = FontManager::instance().get(  Options::getOptionString( "project/boldFont", "verdana_medium.font" ) );
	if( font_ && boldFont_ )
	{
		font_->colour( 0xff000000 );
		boldFont_->colour( 0xff000000 );
	}
}

ProjectModule::~ProjectModule()
{
	BW_GUARD;

	currentInstance_ = NULL;

	SimpleGUI::instance().removeSimpleComponent( *text_ );
	Py_XDECREF( text_ );
	text_ = NULL;

	SimpleGUI::instance().removeSimpleComponent( *shadow_ );
	Py_XDECREF( shadow_ );
	shadow_ = NULL;
}

bool ProjectModule::init( DataSectionPtr pSection )
{
	BW_GUARD;

	if ( pSection )
	{
		lockMap_.init( pSection->openSection( "lockMap" ) );
		SpaceMap::instance().init( pSection->openSection( "spaceMap" ) );
	}
	return true;
}


void ProjectModule::onStart()
{
	BW_GUARD;

	// needed, otherwise the mouse cursor is hidden when we start?!
	::ShowCursor(TRUE);

	// read some options in
	spaceAlpha_ = Options::getOptionFloat(
			"render/project/spaceMapAlpha", 0.5f );
	spaceAlpha_ = Math::clamp( 0.f, spaceAlpha_, 1.f );

	// get the space colour from 12:00 time of day for the space.
	EnviroMinder& em = ChunkManager::instance().cameraSpace()->enviro();
	Vector3 sunColour = em.timeOfDay()->sunAnimation().animate( 12.f );
	Vector3 ambColour = em.timeOfDay()->ambientAnimation().animate( 12.f );
	Vector3 combinedColour = sunColour + ambColour;
	
	float sunTooDarkThreshold = Options::getOptionFloat( "render/project/sunTooDarkThreshold", 80.f );
	sunTooDarkThreshold = Math::clamp( 0.1f, sunTooDarkThreshold, 255.f );

	if (combinedColour.x < sunTooDarkThreshold &&
		combinedColour.y < sunTooDarkThreshold &&
		combinedColour.z < sunTooDarkThreshold)
	{
		// Multiply the space colour so we get something above (0,0,0) to draw
		// the thumbnails.

		// Avoid haven 0 value in components, so the following division and
		// multiplication works.
		combinedColour.x = std::max( 0.1f, combinedColour.x );
		combinedColour.y = std::max( 0.1f, combinedColour.y );
		combinedColour.z = std::max( 0.1f, combinedColour.z );

		float maxComp = std::max( std::max( combinedColour.x, combinedColour.y ) , combinedColour.z );

		float multiplier = sunTooDarkThreshold / maxComp;

		combinedColour *= multiplier;
	}

	spaceColour_ = Colour::getUint32( combinedColour );
	//DEBUG_MSG( "sun %0.2f, %0.2f, %0.2f ....... amb %0.2f, %0.2f, %0.2f ........ space %lx\n",
	//	sunColour.x, sunColour.y, sunColour.z, ambColour.x, ambColour.y, ambColour.z, spaceColour_ );

	// Get the handdrawn map for the current space
	BW::string space = WorldManager::instance().getCurrentSpace();

	handDrawnMap_ = Moo::TextureManager::instance()->get(
		space + "/space.map.bmp",
		false,						// allowAnimation
		false						// mustExist
		);

	if (!handDrawnMap_)
	{
		handDrawnMap_ = Moo::TextureManager::instance()->get(
			"resources/maps/default.map.bmp",
			false,						// allowAnimation
			false						// mustExist
			);
	}

	// work out grid size
	DataSectionPtr	pDS = BWResource::openSection( space + "/" + SPACE_SETTING_FILE_NAME );	
	if (pDS)
	{
		int minX = pDS->readInt( "bounds/minX", 1 );
		int minY = pDS->readInt( "bounds/minY", 1 );
		int maxX = pDS->readInt( "bounds/maxX", -1 );
		int maxY = pDS->readInt( "bounds/maxY", -1 );
		gridSize_ = pDS->readFloat( "chunkSize", DEFAULT_GRID_RESOLUTION );

		minX_ = minX;
		minY_ = minY;
		gridWidth_ = maxX - minX + 1;
		gridHeight_ = maxY - minY + 1;

		localToWorld_ = GridCoord( minX, minY );
	}

	viewPosition_ = Vector3( gridWidth_ / 2.f, gridHeight_ / 2.f, -1.f );

	// set the zoom to the extents of the grid
	float angle =  Moo::rc().camera().fov() / 2.f;
	float yopp = gridHeight_ / 2.f;
	float xopp = gridWidth_ / 2.f;

	// Get the distance away we have to be to see the x points and the y points
	float yheight = yopp / tanf( angle );
	float xheight = xopp / tanf( angle * Moo::rc().camera().aspectRatio() );
	
	// Go back the furthest amount between the two of them
	viewPosition_.z = min( -xheight, -yheight );
	minViewZ_ = viewPosition_.z * 1.1f;
	savedFarPlane_ = Moo::rc().camera().farPlane();
	Moo::Camera camera = Moo::rc().camera();
	camera.farPlane( minViewZ_ * -1.1f );
	Moo::rc().camera( camera );

	// Create an automatic space map, to display the current progress
	SpaceInformation spaceInfo(space, localToWorld_, gridWidth_, gridHeight_);
	SpaceMap::instance().spaceInformation(spaceInfo);

	// Create us a lock texture, to display the locks with
	lockMap_.gridSize( gridWidth_, gridHeight_ );

	updateLockData();

	// Update forced LOD map with space details
	ForcedLODMap& forcedLodMap = WorldManager::instance().forcedLodMap();
	forcedLodMap.setSpaceInformation(spaceInfo);

	cc_ = SmartPointer<ClosedCaptions>(new ClosedCaptions(), true);
	Commentary::instance().addView( &*cc_ );
	cc_->visible( true );
}

int ProjectModule::onStop()
{
	BW_GUARD;

	::ShowCursor( FALSE );

	Options::setOptionFloat( "render/project/spaceMapAlpha", spaceAlpha_ );

	cc_->visible( false );
	Commentary::instance().delView( &*cc_ );
	//delete cc_;
	cc_ = NULL;

	Moo::Camera camera = Moo::rc().camera();
	camera.farPlane( savedFarPlane_ );
	Moo::rc().camera( camera );

	return 0;
}

void ProjectModule::onPause()
{
	BW_GUARD;

	cc_->visible( false );
	Commentary::instance().delView( &*cc_ );
}

void ProjectModule::onResume( int exitCode )
{
	BW_GUARD;

	Commentary::instance().addView( &*cc_ );
	cc_->visible( true );
}

bool ProjectModule::updateState( float dTime )
{
	BW_GUARD;

	cc_->update( dTime );

	bool userInteracting =
				InputDevices::isKeyDown( KeyCode::KEY_LEFTMOUSE ) ||
				InputDevices::isKeyDown( KeyCode::KEY_MIDDLEMOUSE ) ||
				InputDevices::isKeyDown( KeyCode::KEY_RIGHTMOUSE );

	SpaceMap::instance().update( dTime, !userInteracting );

	// set input focus as appropriate
	bool acceptInput = WorldManager::instance().cursorOverGraphicsWnd();
	InputDevices::setFocus( acceptInput, this );

	SimpleGUI::instance().update( dTime );

	//Update the locked regions once a second
#ifndef BIGWORLD_CLIENT_ONLY
	static float s_lockDataUpdate = 0.f;
	s_lockDataUpdate -= dTime;

	if (s_lockDataUpdate < 0.f)
	{
		WorldManager::instance().connection().tick();
		updateLockData();
		s_lockDataUpdate = 1.f;
	}
#endif BIGWORLD_CLIENT_ONLY

	if ( InputDevices::isKeyDown( KeyCode::KEY_RIGHTMOUSE ) )
	{
		// Keep cursor's click position
		// Hide the cursor when the right mouse is held down
		if ( lastCursorPosition_.x == -1 && lastCursorPosition_.y == -1 )
		{
			::ShowCursor(0);
			::GetCursorPos( &lastCursorPosition_ );
		}
		::SetCursorPos( lastCursorPosition_.x, lastCursorPosition_.y );
	}
	else
	{
		if ( lastCursorPosition_.x != -1 || lastCursorPosition_.y != -1 )
		{
			::ShowCursor(1);
			lastCursorPosition_.x = -1;
			lastCursorPosition_.y = -1;
		}
	}

	return true;
}

/**
 * Generate a mesh for a quad, given the starting locations in
 * world and texture space, and the extents for both
 */
namespace 
{

template<class V>
void projectModuleQuad( CustomMesh<V>& mesh,
		  Vector3 bottomLeft, float xextent, float yextent,
		  Vector2 bottomLeftUV, float xuvextent, float yuvextent )
{
	BW_GUARD;

	// bottom left
	V bl;
	bl.pos_ = bottomLeft;
	bl.uv_ = bottomLeftUV;

	// top left
	V tl;
	tl.pos_ = bottomLeft + Vector3( 0.f, yextent, 0.f );
	tl.uv_ = bottomLeftUV + Vector2( 0.f, yuvextent );

	// bottom right
	V br;
	br.pos_ = bottomLeft + Vector3( xextent, 0.f, 0.f );
	br.uv_ = bottomLeftUV + Vector2( xuvextent, 0.f );

	// top right
	V tr;
	tr.pos_ = bottomLeft + Vector3( xextent, yextent, 0.f );
	tr.uv_ = bottomLeftUV + Vector2( xuvextent, yuvextent );


	mesh.push_back( tl );
	mesh.push_back( tr );
	mesh.push_back( br );

	mesh.push_back( br );
	mesh.push_back( bl );
	mesh.push_back( tl );
}

template<class V>
void projectModuleQuad( CustomMesh<V>& mesh,
		  Vector3 bottomLeft, float xextent, float yextent,
		  Vector2 bottomLeftUV, float xuvextent, float yuvextent, uint32 colour )
{
	BW_GUARD;

	// bottom left
	V bl;
	bl.pos_ = bottomLeft;
	bl.uv_ = bottomLeftUV;
	bl.colour_ = colour;

	// top left
	V tl;
	tl.pos_ = bottomLeft + Vector3( 0.f, yextent, 0.f );
	tl.uv_ = bottomLeftUV + Vector2( 0.f, yuvextent );
	tl.colour_ = colour;

	// bottom right
	V br;
	br.pos_ = bottomLeft + Vector3( xextent, 0.f, 0.f );
	br.uv_ = bottomLeftUV + Vector2( xuvextent, 0.f );
	br.colour_ = colour;

	// top right
	V tr;
	tr.pos_ = bottomLeft + Vector3( xextent, yextent, 0.f );
	tr.uv_ = bottomLeftUV + Vector2( xuvextent, yuvextent );
	tr.colour_ = colour;


	mesh.push_back( tl );
	mesh.push_back( tr );
	mesh.push_back( br );

	mesh.push_back( br );
	mesh.push_back( bl );
	mesh.push_back( tl );
}

template<class V>
void projectModuleQuad( CustomMesh<V>& mesh, Vector3 bottomLeft, float xextent, float yextent, DWORD colour )
{
	BW_GUARD;

	// bottom left
	V bl;
	bl.pos_ = bottomLeft;
	bl.colour_ = colour;

	// top left
	V tl;
	tl.pos_ = bottomLeft + Vector3( 0.f, yextent, 0.f );
	tl.colour_ = colour;

	// bottom right
	V br;
	br.pos_ = bottomLeft + Vector3( xextent, 0.f, 0.f );
	br.colour_ = colour;

	// top right
	V tr;
	tr.pos_ = bottomLeft + Vector3( xextent, yextent, 0.f );
	tr.colour_ = colour;

	mesh.push_back( tl );
	mesh.push_back( tr );
	mesh.push_back( br );

	mesh.push_back( br );
	mesh.push_back( bl );
	mesh.push_back( tl );
}

template<class V>
void wiredQuad( CustomMesh<V>& mesh, Vector3 bottomLeft, float xextent, float yextent, DWORD colour )
{
	BW_GUARD;

	// bottom left
	V bl;
	bl.pos_ = bottomLeft;
	bl.colour_ = colour;

	// top left
	V tl;
	tl.pos_ = bottomLeft + Vector3( 0.f, yextent, 0.f );
	tl.colour_ = colour;

	// bottom right
	V br;
	br.pos_ = bottomLeft + Vector3( xextent, 0.f, 0.f );
	br.colour_ = colour;

	// top right
	V tr;
	tr.pos_ = bottomLeft + Vector3( xextent, yextent, 0.f );
	tr.colour_ = colour;

	mesh.push_back( tl );
	mesh.push_back( tr );
	mesh.push_back( br );
	mesh.push_back( bl );
	mesh.push_back( tl );
}

}


/**
 *	Update the animations of chunks and the environment in this module.
 */
void ProjectModule::updateAnimations()
{
	BW_GUARD_PROFILER( ProjectModule_updateAnimations );

	static DogWatch updateAnimations( "Scene update" );
	ScopedDogWatch sdw( updateAnimations );

	ChunkManager& chunkManager = ChunkManager::instance();
	chunkManager.updateAnimations();

	ChunkSpacePtr pSpace = chunkManager.cameraSpace();
	if (pSpace.exists())
	{
		EnviroMinder & enviro = pSpace->enviro();
		enviro.updateAnimations();
	}
}


void ProjectModule::render( float dTime )
{
	BW_GUARD;

    if (!Moo::rc().device())
		return;

	if (!handDrawnMap_)
		return;

	DX::Device* device_ = Moo::rc().device();

	device_->Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
		0xff004e98, 1, 0 );

	Moo::rc().setPixelShader( NULL );

	Moo::rc().reset();
	Moo::rc().updateProjectionMatrix();
	Moo::rc().updateViewTransforms();


	Moo::rc().setRenderState( D3DRS_LIGHTING, FALSE );
	Moo::rc().setRenderState( D3DRS_CLIPPING, TRUE );
	Moo::rc().setRenderState( D3DRS_CULLMODE, D3DCULL_CCW );
	Moo::rc().setRenderState( D3DRS_ALPHATESTENABLE, FALSE );
	Moo::rc().setRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
	Moo::rc().setRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
	Moo::rc().setRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
	Moo::rc().setRenderState( D3DRS_LIGHTING, FALSE );

	Moo::rc().setTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_MODULATE );
	Moo::rc().setTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
	Moo::rc().setTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
	Moo::rc().setTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1 );
	Moo::rc().setTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );

	Moo::rc().setTextureStageState( 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
	Moo::rc().setTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_DISABLE );


	Moo::rc().device()->SetTransform( D3DTS_WORLD, &Moo::rc().world() );

	Matrix view;
	view.setTranslate( viewPosition_ );
	view.invert();

	Moo::rc().device()->SetTransform( D3DTS_VIEW, &view );
	Moo::rc().device()->SetTransform( D3DTS_PROJECTION, &Moo::rc().projection() );

	// Draw the overhead view
	Moo::rc().setTexture( 0, handDrawnMap_->pTexture() );

	CustomMesh<Moo::VertexXYZUV> mesh;
	// The bitmap is apparently upside down, so we draw the texture upside down too
	projectModuleQuad( mesh, Vector3( 0.f, 0.f, 0.f ),
		static_cast<float>(gridWidth_),
		static_cast<float>(gridHeight_),
		Vector2( 0.f, 1.f ), 1.f, -1.f );

	mesh.draw();

	Moo::rc().setRenderState( D3DRS_ALPHABLENDENABLE, TRUE);

	// Draw the autogen map
	SpaceMap::instance().setTexture( 0 );

	Moo::rc().setSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
	Moo::rc().setSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );

	uint32 alpha = uint32(spaceAlpha_ * 255.f) << 24;
	CustomMesh<Moo::VertexXYZDUV> cmesh;
	projectModuleQuad( cmesh, Vector3( 0.f, 0.f, 0.f ),
		static_cast<float>(gridWidth_),
		static_cast<float>(gridHeight_),
		Vector2( 0.f, 0.f ), 1.f, 1.f,
		alpha | spaceColour_ );

	Moo::rc().setTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_CURRENT );
	cmesh.draw();
	Moo::rc().setTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );

	Moo::rc().setTexture( 0, NULL );

	// Draw the locks Texture or the LOD lock Texture
	const bool renderLocksOverlay = (WorldManager::instance().connection().enabled() && 
									Options::getOptionInt( "render/misc/drawOverlayLocks", 1 ) == 1);
	const bool renderLODOverlay = (Options::getOptionInt( "render/misc/drawOverlayLOD", 1 ) == 1);

	if (renderLocksOverlay || renderLODOverlay)
	{
		if (renderLODOverlay)
		{
			WorldManager::instance().forcedLodMap().setTexture( 0 );
		}
		else //if (renderLocksOverlay)
		{
			lockMap_.setTexture( 0 );
		}

		Moo::rc().setSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_POINT );
		Moo::rc().setSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_POINT );

		mesh.clear();
		projectModuleQuad( mesh, Vector3( 0.f, 0.f, 0.f ),
			static_cast<float>(gridWidth_),
			static_cast<float>(gridHeight_),
			Vector2( 0.f, 0.f ), 1.f, 1.f );

		mesh.draw();

		Moo::rc().setSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
		Moo::rc().setSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );

		Moo::rc().setTexture( 0, NULL );
	}

	// Draw the current selection
	GridCoord gc( GridCoord::invalid() );

	if (currentSelection_.valid())
	{
		CustomMesh<Moo::VertexXYZL> sel;

		BWLock::BWLockDConnection& conn = WorldManager::instance().connection();
		if( conn.connected() )
		{
			float x = (float) currentSelection_.bottomLeft.x - conn.xExtent();
			float y = (float) currentSelection_.bottomLeft.y - conn.zExtent();
			float width = (float) currentSelection_.topRight.x - currentSelection_.bottomLeft.x + conn.xExtent() * 2;
			float height = (float) currentSelection_.topRight.y - currentSelection_.bottomLeft.y + conn.zExtent() * 2;
			if( x < 0 )
			{
				width += x;
				x = 0;
			}
			if( y < 0 )
			{
				height += y;
				y = 0;
			}
			if( x + width > gridWidth_ )
				width = (float)gridWidth_ - x;
			if( y +height > gridHeight_ )
				height = (float)gridHeight_ - y;
			projectModuleQuad( sel, Vector3( x, y, 0.f), width, height, 0x40202020 );

			CustomMesh<Moo::VertexXYZL> wired( D3DPT_LINESTRIP );
			wiredQuad( wired, Vector3( x, y, 0.f), width, height, 0xffffffff );
			wired.draw();
		}

		projectModuleQuad( sel, Vector3( (float) currentSelection_.bottomLeft.x, (float) currentSelection_.bottomLeft.y, 0.f),
			(float) currentSelection_.topRight.x - currentSelection_.bottomLeft.x,
			(float) currentSelection_.topRight.y - currentSelection_.bottomLeft.y,
			0x40202020 );

		sel.draw();

		CustomMesh<Moo::VertexXYZL> wired( D3DPT_LINESTRIP );
		wiredQuad( wired, Vector3( (float) currentSelection_.bottomLeft.x, (float) currentSelection_.bottomLeft.y, 0.f),
			(float) currentSelection_.topRight.x - currentSelection_.bottomLeft.x,
			(float) currentSelection_.topRight.y - currentSelection_.bottomLeft.y, 0xffffff7f );
		wired.draw();
	}
	else if( memcmp( &currentSelectedCoord_, &gc, sizeof( GridCoord ) ) != 0
		&& WorldManager::instance().connection().connected() && WorldManager::instance().connection().isLockedByMe(
			currentSelectedCoord_.x + minX_, currentSelectedCoord_.y + minY_ ) )
	{
		BW::set<BWLock::Rect> rects = WorldManager::instance().connection().getLockRects(
			currentSelectedCoord_.x + minX_, currentSelectedCoord_.y + minY_ );

		for( BW::set<BWLock::Rect>::iterator iter = rects.begin(); iter != rects.end(); ++iter )
		{
			CustomMesh<Moo::VertexXYZL> sel;

			int left = max( (int)iter->left_, minX_ );
			int top = max( (int)iter->top_, minY_ );
			int right = min( (int)iter->right_, (int)gridWidth_ + minX_ - 1 );
			int bottom = min( (int)iter->bottom_, (int)gridHeight_ + minY_ - 1 );
			projectModuleQuad( sel, Vector3( (float)left - minX_, (float)top - minY_, 0.f ),
				(float)right - left + 1, (float)bottom - top + 1,
				0x4400ffff );

			sel.draw();
		}
	}

	Vector2 grid = currentGridPos();
	if( WorldManager::instance().connection().connected() &&
		grid.x >= 0 && grid.y >= 0 && grid.x < gridWidth_ && grid.y < gridHeight_ &&
		WorldManager::instance().cursorOverGraphicsWnd() && font_ && boldFont_ )
	{
		Moo::rc().setPixelShader( NULL );

		BWLock::GridInfo gridInfo = WorldManager::instance().connection().getGridInformation(
			int16( floor( grid.x + minX_ ) ), int16( floor( grid.y + minY_ ) ) );

		if( !gridInfo.empty() )
		{
			Vector2 gridPos = currentGridPos();
			Vector3 world = gridPosToWorldPos( gridPos );
			GeometryMapping* dirMap = WorldManager::instance().geometryMapping();

			gridInfo.insert( gridInfo.end() - 1, std::make_pair( "Where:", dirMap->outsideChunkIdentifier(world) ) );

			static const int MARGIN = 5;
			POINT cursorPos = WorldManager::instance().currentCursorPosition();

			int width = 512;

			int wb, w, hb, h, W = 0, H = 0;

			for( unsigned int i = 0; i < gridInfo.size(); ++i )
			{
				boldFont_->metrics().stringDimensions( bw_utf8tow( gridInfo[ i ].first ), wb, hb );
				font_->metrics().stringDimensions( bw_utf8tow( gridInfo[ i ].second ), w, h );
				if( i == gridInfo.size() - 1 && wb + w > width )
				{
					W = width;
					w = W - wb;
					font_->metrics().breakString( bw_utf8tow( gridInfo[ i ].second ), w, h );
					H += h;
				}
				else
				{
					if( wb + w > W )
						W = wb + w;
					H += hb;
				}
			}

			int x = cursorPos.x + MARGIN * 2;
			if( x + W + MARGIN * 4 > Moo::rc().screenWidth() )
				x = (int)Moo::rc().screenWidth() - W - MARGIN * 4;
			int y = cursorPos.y + MARGIN * 2;
			if( y + H + MARGIN * 4 > Moo::rc().screenHeight() )
				y = (int)Moo::rc().screenHeight() - H - MARGIN * 4;

			CustomMesh<Moo::VertexXYZL> border;
			Vector2 leftBottom = pixelToGridPos( x - MARGIN - 1, y + H + MARGIN + 1 );
			Vector2 rightTop = pixelToGridPos( x + W + MARGIN + 1, y - MARGIN - 1 );

			projectModuleQuad( border, Vector3( leftBottom.x, leftBottom.y, 0.f ),
				rightTop.x - leftBottom.x, rightTop.y - leftBottom.y, 0xff000000 );

			border.draw();

			CustomMesh<Moo::VertexXYZL> mesh;
			leftBottom = pixelToGridPos( x - MARGIN, y + H + MARGIN );
			rightTop = pixelToGridPos( x + W + MARGIN, y - MARGIN );

			projectModuleQuad( mesh, Vector3( leftBottom.x, leftBottom.y, 0.f ),
				rightTop.x - leftBottom.x, rightTop.y - leftBottom.y, 0xffffffE1 );

			mesh.draw();

			if ( FontManager::instance().begin( *boldFont_ ) )
			{
				for( unsigned int i = 0; i < gridInfo.size(); ++i )
				{				
					boldFont_->drawString( gridInfo[ i ].first, x, y );
					boldFont_->metrics().stringDimensions( bw_utf8tow( gridInfo[ i ].first ), wb, hb );
					font_->metrics().stringDimensions( bw_utf8tow( gridInfo[ i ].second ), w, h );
					font_->drawString( bw_utf8tow( gridInfo[ i ].second ), x + wb, y + hb - h, width - wb, 0x7fffffff );
					y += hb;
				}
				FontManager::instance().end();
			}
		}
	}

	// Draw the space map debug overlay
	SpaceMapDebug::instance().draw();

	this->writeStatus();
}


/**
 *	This method writes out some status panel sections that are done every frame.
 *	i.e. FPS and cursor location.
 */
void ProjectModule::writeStatus()
{
	BW_GUARD;

    WorldManager::instance().setStatusMessage(1, "");
    WorldManager::instance().setStatusMessage(2, "");
    WorldManager::instance().setStatusMessage(4, "");

	//Panel 0 - memory load
	WorldManager::instance().setStatusMessage( 0,
		LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/PROJECT/PROJECT_MODULE/MEMORY_LOAD", (int)Memory::memoryLoad() ) );

	//Panel 3 - locator position
	Vector2 gridPos = currentGridPos();
	Vector3 world = gridPosToWorldPos( gridPos );
	GeometryMapping* dirMap = WorldManager::instance().geometryMapping();
	BW::string name = dirMap->outsideChunkIdentifier(world);
	WorldManager::instance().setStatusMessage( 3, name );

	// Panel 5 - number of chunks loaded
	BW::string dirtyChunks = WorldManager::instance().describeDirtyChunkStatus();
	if (!dirtyChunks.empty())
	{
		WorldManager::instance().setStatusMessage( 5,
			LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/PROJECT/PROJECT_MODULE/CHUNK_LOADED_WITH_DIRTY",
				EditorChunkCache::chunks_.size(), dirtyChunks ) );
	}
	else
	{
		WorldManager::instance().setStatusMessage( 5,
			LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/PROJECT/PROJECT_MODULE/CHUNK_LOADED",
				EditorChunkCache::chunks_.size() ) );
	}

	MainFrame *mainFrame = reinterpret_cast<MainFrame *>(AfxGetMainWnd());
    mainFrame->frameUpdate();
}


Vector2 ProjectModule::currentGridPos()
{
	BW_GUARD;

	POINT pt = WorldManager::instance().currentCursorPosition();
	return pixelToGridPos( pt.x, pt.y );
}

Vector2 ProjectModule::pixelToGridPos( int x, int y )
{
	BW_GUARD;

	Vector3 cursorPos = Moo::rc().camera().nearPlanePoint(
			(float(x) / Moo::rc().screenWidth()) * 2.f - 1.f,
			1.f - (float(y) / Moo::rc().screenHeight()) * 2.f );

	Matrix view;
	view.setTranslate( viewPosition_ );

	Vector3 worldRay = view.applyVector( cursorPos );
	worldRay.normalise();

	PlaneEq gridPlane( Vector3(0.f, 0.f, 1.f), .0001f );

	Vector3 gridPos = gridPlane.intersectRay( viewPosition_, worldRay );

	return Vector2( gridPos.x, gridPos.y );
}

Vector3 ProjectModule::gridPosToWorldPos( Vector2 gridPos )
{
	BW_GUARD;

	Vector2 w = (gridPos + Vector2( float(localToWorld_.x), float(localToWorld_.y) )) *
		gridSize_;

	return Vector3( w.x, 0, w.y);
}

bool ProjectModule::handleKeyEvent( const KeyEvent & event )
{
	BW_GUARD;

	if (event.key() == KeyCode::KEY_LEFTMOUSE)
	{
		if ( event.isKeyDown() )
		{
			mouseDownOn3DWindow_ = true;
		}
		else
		{
			mouseDownOn3DWindow_ = false;
		}
	}

	// Go to the world location when the middle mouse button is held down
	if (event.key() == KeyCode::KEY_MIDDLEMOUSE && !event.isKeyDown())
	{
		// Get where we click in grid coords
		Vector2 gridPos = currentGridPos();
		Vector3 world = gridPosToWorldPos( gridPos );
        
        // Find the height at this point
        float height = TerrainUtils::heightAtPos(world.x, world.z, true);
        world.y = height + Options::getOptionInt( "graphics/farclip", 500 )/10.0f;

		// Set the view matrix to the new world coords
		Matrix view = WorldEditorCamera::instance().currentCamera().view();
		view.setTranslate( world );
		view.preRotateX( DEG_TO_RAD( 30.f ) );
		view.invert();
		WorldEditorCamera::instance().currentCamera().view( view );

		if( Options::getOptionInt( "camera/ortho" ) == WorldEditorCamera::CT_Orthographic )
		{
			WorldEditorCamera::instance().changeToCamera( WorldEditorCamera::CT_MouseLook );
			WorldEditorCamera::instance().changeToCamera( WorldEditorCamera::CT_Orthographic );
		}

		// Now, change back to object mode
		PyObject* pModule = PyImport_ImportModule( "WorldEditorDirector" );
		if (pModule != NULL)
		{
			PyObject* pScriptObject = PyObject_GetAttrString( pModule, "bd" );

			if (pScriptObject != NULL)
			{
				Script::call(
					PyObject_GetAttrString( pScriptObject, "changeToMode" ),
					Py_BuildValue( "(s)", "Object" ),
					"ProjectModule");
					
				Py_DECREF( pScriptObject );
			}

			PanelManager::instance().setDefaultToolMode();
			Py_DECREF( pModule );
		}

		return true;
	}

	// Drag a rectangle snapped to chunk boundaries with the left mouse button
	if (event.key() == KeyCode::KEY_LEFTMOUSE && event.isKeyDown())
	{
		// Start the selection
		selectionStart_ = currentGridPos();
		currentSelection_ = GridRect::fromCoords( selectionStart_, selectionStart_ );


		currentSelectedCoord_ = GridCoord( currentGridPos() );
		if( currentSelectedCoord_.x < 0 || currentSelectedCoord_.x >= (int)gridWidth_ ||
			currentSelectedCoord_.y < 0 || currentSelectedCoord_.y >= (int)gridHeight_ )
			currentSelectedCoord_ = GridCoord::invalid();
	}

	if (event.key() == KeyCode::KEY_1 && event.isKeyDown() && !event.modifiers())
	{
		PanelManager::instance().setToolMode( L"Objects" );
	}
	if (event.key() == KeyCode::KEY_2 && event.isKeyDown() && !event.modifiers())
	{
		PanelManager::instance().setToolMode( L"TerrainTexture" );
	}
	if (event.key() == KeyCode::KEY_3 && event.isKeyDown() && !event.modifiers())
	{
		PanelManager::instance().setToolMode( L"TerrainHeight" );
	}
	if (event.key() == KeyCode::KEY_4 && event.isKeyDown() && !event.modifiers())
	{
		PanelManager::instance().setToolMode( L"TerrainFilter" );
	}
	if (event.key() == KeyCode::KEY_5 && event.isKeyDown() && !event.modifiers())
	{
		PanelManager::instance().setToolMode( L"TerrainMesh" );
	}
	if (event.key() == KeyCode::KEY_6 && event.isKeyDown() && !event.modifiers())
	{
		PanelManager::instance().setToolMode( L"TerrainImpExp" );
	}
	if (event.key() == KeyCode::KEY_8 && event.isKeyDown() && !event.modifiers())
	{
		PanelManager::instance().setToolMode( L"TerrainRuler" );
	}
	if (event.key() == KeyCode::KEY_9 && event.isKeyDown() && !event.modifiers())
	{
		PanelManager::instance().setToolMode( L"Fences" );
	}

	return false;
}

bool ProjectModule::handleMouseEvent( const MouseEvent & event )
{
	BW_GUARD;

	bool handled = false;

	// Adjust zoom
	if (event.dz() != 0)
	{
		viewPosition_.z += (event.dz() > 0) ? -minViewZ_/25.f : minViewZ_/25.f;

		if (viewPosition_.z > -1.f)
			viewPosition_.z = -1.f;

		if (viewPosition_.z < minViewZ_)
			viewPosition_.z = minViewZ_;

		handled = true;
	}

	// Pan around
	if ((event.dx() != 0 || event.dy() != 0) && InputDevices::isKeyDown( KeyCode::KEY_RIGHTMOUSE ))
	{
        // The current zoom scale:
        CPoint pt(0, 0);
        Vector2 worldPos = gridPos(pt);
        ++pt.x;
        Vector2 xWorldPos = gridPos(pt);
        --pt.x; ++pt.y;
        Vector2 yWorldPos = gridPos(pt);
        float scaleX_ = fabsf(xWorldPos.x - worldPos.x);
        float scaleY_ = fabsf(yWorldPos.y - worldPos.y);

	    viewPosition_.x -= scaleX_*event.dx();
	    viewPosition_.y += scaleY_*event.dy();

		handled = true;
	}

	// Update the selection
	if( InputDevices::isKeyDown( KeyCode::KEY_LEFTMOUSE ) && mouseDownOn3DWindow_ )
	{
		if( event.dx() != 0 || event.dy() != 0 )
		{
			Vector2 curPos = currentGridPos();
			Vector2 initPos = selectionStart_;
			if (curPos.x > selectionStart_.x) 
				curPos.x = ceilf( curPos.x );
			else
				initPos.x = ceilf( initPos.x );

			if (curPos.y > selectionStart_.y)
				curPos.y = ceilf( curPos.y );
			else
				initPos.y = ceilf( initPos.y );

			currentSelection_ = GridRect::fromCoords( initPos, curPos );
			currentSelection_.clamp( 0,0,gridWidth_, gridHeight_ );
		}
	}

	return handled;
}

#ifndef BIGWORLD_CLIENT_ONLY
bool readLine( SOCKET sock, BW::string& line )
{
	BW_GUARD;

	char buf[1024];

	char* p = buf;

	for (uint i = 0; i < 1023; i++)
	{
		if ( recv( sock, p, 1, 0) != 1 )
			break;

		if (*p == '\n')
		{
			*p = '\0';
			line = buf;
			return true;
		}

		p++;
	}

	line = "";
	return false;
}
#endif//BIGWORLD_CLIENT_ONLY

bool ProjectModule::isReadyToLock() const
{
	BW_GUARD;

	BWLock::BWLockDConnection& conn = WorldManager::instance().connection();
	if( !currentSelection_.valid() || !conn.connected() )
		return false;

	// if all chunks are locked, we shouldn't lock them again
	bool allWritable = true;
	GridRect region = currentSelection_ + localToWorld_;
	for( int16 gridX = region.bottomLeft.x; gridX < region.topRight.x && allWritable; ++gridX )
	{
		for( int16 gridZ = region.bottomLeft.y; gridZ < region.topRight.y && allWritable; ++gridZ )
		{
			if( !conn.isWritableByMe( gridX, gridZ ) )
			{
				allWritable = false;
				break;
			}
		}
	}

	if( allWritable )
		return false;

	// if one of the chunks are locked by others, we cannot lock them
	for( int16 gridX = region.bottomLeft.x - conn.xExtent(); gridX < region.topRight.x + conn.xExtent(); ++gridX )
	{
		for( int16 gridZ = region.bottomLeft.y - conn.zExtent(); gridZ < region.topRight.y + conn.zExtent(); ++gridZ )
		{
			if( conn.isLockedByOthers( gridX, gridZ ) )
			{
				return false;
			}
		}
	}

	return true;
}

bool ProjectModule::isReadyToCommitOrDiscard() const
{
	BW_GUARD;

	GridCoord gc( GridCoord::invalid() );
	return memcmp( &currentSelectedCoord_, &gc, sizeof( GridCoord ) ) != 0 &&
		WorldManager::instance().connection().connected() && WorldManager::instance().connection().isLockedByMe(
		currentSelectedCoord_.x + minX_, currentSelectedCoord_.y + minY_ );
}

BW::set<BW::string> ProjectModule::lockedChunksFromSelection( bool insideOnly )
{
	BW_GUARD;

	BW::set<BW::string> result;
	GridCoord gc( GridCoord::invalid() );
	if( memcmp( &currentSelectedCoord_, &gc, sizeof( GridCoord ) ) != 0 &&
		WorldManager::instance().connection().isLockedByMe( currentSelectedCoord_.x + minX_, currentSelectedCoord_.y + minY_ ) )
	{
		BW::set<BWLock::Rect> rects = WorldManager::instance().connection().getLockRects(
			currentSelectedCoord_.x + minX_, currentSelectedCoord_.y + minY_ );
		for( BW::set<BWLock::Rect>::iterator iter = rects.begin(); iter != rects.end(); ++iter )
			getChunksFromRect( result, *iter, insideOnly );
	}
	return result;
}

BW::set<BW::string> ProjectModule::graphFilesFromSelection()
{
	BW_GUARD;

	BW::set<BW::string> result;
	GridCoord gc( GridCoord::invalid() );

	if( memcmp( &currentSelectedCoord_, &gc, sizeof( GridCoord ) ) != 0 &&
		WorldManager::instance().connection().isLockedByMe( currentSelectedCoord_.x + minX_, currentSelectedCoord_.y + minY_ ) )
	{
		BW::set<BWLock::Rect> rects = WorldManager::instance().connection().getLockRects(
			currentSelectedCoord_.x + minX_, currentSelectedCoord_.y + minY_ );

		WIN32_FIND_DATA wfd;
		BW::wstring toFind;
		bw_utf8tow( BWResolver::resolveFilename( currentSpaceDir() ) + "\\*.graph", toFind );
		HANDLE find = FindFirstFile( toFind.c_str(), &wfd );
		if( find != INVALID_HANDLE_VALUE )
		{
			do
			{
				if( !( wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) )
				{
					BW::string nfileName;
					bw_wtoutf8( wfd.cFileName, nfileName );
					DataSectionPtr graph = BWResource::openSection( currentSpaceDir() + '/' + nfileName );
					if( graph )
					{
						bool added = false;
						for( int i = 0; i < graph->countChildren() && !added; ++i )
						{
							DataSectionPtr wp = graph->openChild( i )->openSection( "worldPosition" );
							if( wp )
							{
								Vector3 worldPosition = wp->asVector3();
								int cx = (int)floorf( worldPosition.x / gridSize_ );
								int cz = (int)floorf( worldPosition.z / gridSize_ );
								for( BW::set<BWLock::Rect>::iterator iter = rects.begin(); iter != rects.end(); ++iter )
									if( iter->in( cx, cz ) )
									{
										result.insert( nfileName);
										added = true;
										break;
									}
							}
						}
					}
				}
			}
			while( FindNextFile( find, &wfd ) );
			FindClose( find );
		}
	}
	return result;
}

BW::set<BW::string> ProjectModule::vloFilesFromSelection()
{
	BW_GUARD;

	BW::set<BW::string> result;
	GridCoord gc( GridCoord::invalid() );

	if( memcmp( &currentSelectedCoord_, &gc, sizeof( GridCoord ) ) != 0 &&
		WorldManager::instance().connection().isLockedByMe( currentSelectedCoord_.x + minX_, currentSelectedCoord_.y + minY_ ) )
	{
		BW::set<BWLock::Rect> rects = WorldManager::instance().connection().getLockRects(
			currentSelectedCoord_.x + minX_, currentSelectedCoord_.y + minY_ );

		WIN32_FIND_DATA wfd;
		BW::wstring toFind;
		bw_utf8tow( BWResolver::resolveFilename( currentSpaceDir() ) + "\\*.vlo", toFind );
		HANDLE find = FindFirstFile( toFind.c_str(), &wfd );
		if( find != INVALID_HANDLE_VALUE )
		{
			do
			{
				if( !( wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) )
				{
					BW::string nfileName;
					bw_wtoutf8( wfd.cFileName, nfileName );
					DataSectionPtr vlo = BWResource::openSection( currentSpaceDir() + '/' + nfileName );
					if( vlo && vlo->countChildren() )
					{
						if( vlo->openSection( "water" ) )
						{
							Vector3 position = vlo->readVector3( "water/position" );
							Vector3 size = vlo->readVector3( "water/size" );

							int cxMin = (int)floorf( position.x / gridSize_ );
							int czMin = (int)floorf( position.z / gridSize_ );
							int cxMax = (int)floorf( (position.x + size.x) / gridSize_ );
							int czMax = (int)floorf( (position.z + size.z) / gridSize_ );

							BWLock::Rect rect( cxMin, czMin, cxMax, czMax );

							for( BW::set<BWLock::Rect>::iterator iter = rects.begin(); iter != rects.end(); ++iter )
								if( iter->intersect( rect ) )
								{
									result.insert( nfileName );
									BW::string odata = BWResource::changeExtension( nfileName, ".odata" );
									if( BWResource::fileExists( ProjectModule::currentSpaceDir() + '/' + odata ) )
										result.insert( odata );
									break;
								}
						}
						else
						{
							ERROR_MSG( "ProjectModule::vloFilesFromSelection: WE has encountered an unknown type of VLO: %s",
								vlo->openChild( 0 )->sectionName().c_str() );
						}
					}
				}
			}
			while( FindNextFile( find, &wfd ) );
			FindClose( find );
		}
	}
	return result;
}

void ProjectModule::getChunksFromRectRecursive( const BW::string& spaceDir, const BW::string& subDir,
												BW::set<BW::string>& chunks, BWLock::Rect rect, bool insideOnly )
{
	BW_GUARD;

	WIN32_FIND_DATA wfd;
	BW::wstring toFind;
	bw_utf8tow( spaceDir + subDir + "*i.chunk", toFind );
	HANDLE find = FindFirstFile( toFind.c_str(), &wfd );
	if( find != INVALID_HANDLE_VALUE )
	{
		do
		{
			BW::string nfileName;
			bw_wtoutf8( wfd.cFileName, nfileName );
			BW::string chunkFileName = BWResource::dissolveFilename( spaceDir + subDir + nfileName );
			DataSectionPtr pDS = BWResource::openSection( chunkFileName );
			BoundingBox bb( pDS->readVector3( "boundingBox/min" ), pDS->readVector3( "boundingBox/max" ) );
			int left = (int)floorf( bb.minBounds().x / gridSize_ );
			int top = (int)floorf( bb.minBounds().z / gridSize_ );
			int right = (int)floorf( bb.maxBounds().x / gridSize_ );
			int bottom = (int)floorf( bb.maxBounds().z / gridSize_ );
			nfileName.resize( nfileName.find_last_of( '.' ) );
			if( insideOnly )
			{
				if( left >= rect.left_ && left <= rect.right_ &&
					top >= rect.top_ && top <= rect.bottom_ &&
					right >= rect.left_ && right <= rect.right_ && 
					bottom >= rect.top_ && bottom <= rect.bottom_ )
					chunks.insert( subDir + nfileName );
			}
			else
			{
				if( ( left >= rect.left_ && left <= rect.right_ &&
					top >= rect.top_ && top <= rect.bottom_ ) ||
					( right >= rect.left_ && right <= rect.right_ && 
					bottom >= rect.top_ && bottom <= rect.bottom_ ) )
					chunks.insert( subDir + nfileName );
			}
		}
		while( FindNextFile( find, &wfd ) );
		FindClose( find );
	}
	bw_utf8tow( spaceDir + subDir + "*i.~chunk~", toFind );
	find = FindFirstFile( toFind.c_str(), &wfd );
	if( find != INVALID_HANDLE_VALUE )
	{
		do
		{
			BW::string nfileName;
			bw_wtoutf8( wfd.cFileName, nfileName );
			BW::string chunkFileName = BWResource::dissolveFilename( spaceDir + subDir + nfileName );
			DataSectionPtr pDS = BWResource::openSection( chunkFileName );
			BoundingBox bb( pDS->readVector3( "boundingBox/min" ), pDS->readVector3( "boundingBox/max" ) );
			int left = (int)floorf( bb.minBounds().x / gridSize_ );
			int top = (int)floorf( bb.minBounds().z / gridSize_ );
			int right = (int)floorf( bb.maxBounds().x / gridSize_ );
			int bottom = (int)floorf( bb.maxBounds().z / gridSize_ );
			nfileName.resize( nfileName.find_last_of( '.' ) );
			if( insideOnly )
			{
				if( left >= rect.left_ && left <= rect.right_ &&
					top >= rect.top_ && top <= rect.bottom_ &&
					right >= rect.left_ && right <= rect.right_ && 
					bottom >= rect.top_ && bottom <= rect.bottom_ )
					chunks.insert( subDir + nfileName );
			}
			else
			{
				if( ( left >= rect.left_ && left <= rect.right_ &&
					top >= rect.top_ && top <= rect.bottom_ ) ||
					( right >= rect.left_ && right <= rect.right_ && 
					bottom >= rect.top_ && bottom <= rect.bottom_ ) )
					chunks.insert( subDir + nfileName );
			}
		}
		while( FindNextFile( find, &wfd ) );
		FindClose( find );
	}

	bw_utf8tow( spaceDir + subDir + "*", toFind );
	find = FindFirstFile( toFind.c_str(), &wfd );
	if ( find != INVALID_HANDLE_VALUE )
	{
		do
		{
			if ( (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0 &&
				 (wfd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) == 0 &&
				 (wfd.cFileName[0] != L'.') )
			{
				BW::string curFolder = bw_wtoutf8( wfd.cFileName );
				if (!CVSWrapper::ignoreDir( curFolder ))
				{
					getChunksFromRectRecursive( spaceDir, subDir + curFolder + "/", chunks, rect, insideOnly );
				}
			}
		}
		while( FindNextFile( find, &wfd ) );
		FindClose( find );
	}
}


void ProjectModule::getChunksFromRect( BW::set<BW::string>& chunks, BWLock::Rect rect, bool insideOnly )
{
	BW_GUARD;

	for( int x = rect.left_; x <= rect.right_; ++x )
		for( int z = rect.top_; z <= rect.bottom_; ++z )
			if( x >= minX_ && z >= minY_ && ( x < minX_ + (int)gridWidth_ ) && ( z < minY_ + (int)gridHeight_ ) )
				chunks.insert( ChunkFormat::outsideChunkIdentifier( x, z ) );

	BW::string spaceDir = BWResource::resolveFilename( Options::getOptionString( "space/mru0" ) ) + "/";

	getChunksFromRectRecursive( spaceDir, "", chunks, rect, insideOnly );
}


/**
 * Don't forget to call updateLockData() after this
 */
bool ProjectModule::lockSelection( const BW::string& description )
{
	BW_GUARD;

#ifdef BIGWORLD_CLIENT_ONLY
	return false;
#else
	if (!currentSelection_.valid())
		return false;

	if ( !WorldManager::instance().connection().connected() )
	{
		INFO_MSG( "Unable to connect to bwlockd\n" );
		WorldManager::instance().addCommentaryMsg( LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/PROJECT/PROJECT_MODULE/UNABLE_TO_CONNECT" ) );
		return false;
	}

	if( CVSWrapper::enabled() && !CVSWrapper( currentSpaceResDir() ).isInCVS( currentSpaceDir() ) )
	{
		currentSelection_ = GridRect::fromCoords( GridCoord( 0, 0 ),
			GridCoord( gridWidth_, gridHeight_ ) );
	}
	currentSelectedCoord_.x = min( currentSelection_.bottomLeft.x, currentSelection_.topRight.x );
	currentSelectedCoord_.y = min( currentSelection_.bottomLeft.y, currentSelection_.topRight.y );
	GridRect region = currentSelection_ + localToWorld_;
	// clear the selection
	currentSelection_ = GridRect::zero();

	CWaitCursor wait;

	bool result = WorldManager::instance().connection().lock( region, description );
	if( result )
		updateLockData();
	return result;
#endif // BIGWORLD_CLIENT_ONLY
}

bool ProjectModule::discardLocks( const BW::string& description )
{
	BW_GUARD;

#ifdef BIGWORLD_CLIENT_ONLY
	return false;
#else

	GridCoord gc( GridCoord::invalid() );

	if( memcmp( &currentSelectedCoord_, &gc, sizeof( GridCoord ) ) != 0
		&& WorldManager::instance().connection().connected() &&
		WorldManager::instance().connection().isLockedByMe( currentSelectedCoord_.x + minX_, currentSelectedCoord_.y + minY_ ) )
	{
		BW::set<BWLock::Rect> rects = WorldManager::instance().connection().getLockRects(
			currentSelectedCoord_.x + minX_, currentSelectedCoord_.y + minY_ );

		CWaitCursor wait;
		for( BW::set<BWLock::Rect>::iterator iter = rects.begin(); iter != rects.end(); ++iter )
			WorldManager::instance().connection().unlock( *iter, description );

		updateLockData();
		return true;
	}
	return false;
#endif
}

void ProjectModule::commitDone()
{
	BW_GUARD;

#ifndef BIGWORLD_CLIENT_ONLY
	if( !WorldManager::instance().connection().connected() )
	{
		ERROR_MSG( "Unable to connect to bwlockd\n" );
		WorldManager::instance().addCommentaryMsg( LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/PROJECT/PROJECT_MODULE/UNABLE_TO_CONNECT" ) );
		return;
	}
	// TODO: possibly do sth. here?
#endif
}

void ProjectModule::updateLockData()
{
	BW_GUARD;

#ifdef BIGWORLD_CLIENT_ONLY
	return;
#else
	BW::vector<unsigned char> lockData = WorldManager::instance().connection().getLockData(
		minX_, minY_, gridWidth_, gridHeight_ );
	lockMap_.updateLockData( gridWidth_, gridHeight_, &lockData[0] );
#endif
}


ProjectModule* ProjectModule::currentInstance()
{
	return currentInstance_;
}

BW::string ProjectModule::currentSpaceDir()
{
	return Options::getOptionString( "space/mru0" );
}

BW::string ProjectModule::currentSpaceResDir()
{
	BW_GUARD;

	BW::string spacePath = currentSpaceDir();
	BW::string fullPath = BWResource::resolveFilename( spacePath );

	if (!fullPath.empty())
	{
		if (*spacePath.rbegin() != '/' && *fullPath.rbegin() == '/')
		{
			fullPath.resize( fullPath.size() - 1 );
		}
		else if (*spacePath.rbegin() == '/' && *fullPath.rbegin() != '/')
		{
			fullPath += '/';
		}

		fullPath.resize( fullPath.size() - spacePath.size() );
	}

	return fullPath;
}

BW::string ProjectModule::currentSpaceAbsoluteDir()
{
	BW_GUARD;

	return BWResource::resolveFilename( Options::getOptionString( "space/mru0" ) );
}

void ProjectModule::projectMapAlpha( float a )
{
	spaceAlpha_ = a;
}


float ProjectModule::projectMapAlpha()
{
	return spaceAlpha_;
}


void ProjectModule::regenerateAllDirty()
{
	BW_GUARD;

    ProjectModule *projectModule = ProjectModule::currentInstance();
    bool haveProjectModule = projectModule != NULL;
    if (!haveProjectModule)
    {
        projectModule = new ProjectModule();
        projectModule->onStart();
    }
    SpaceMap::instance().regenerateAllDirty(true);
    if (!haveProjectModule)
    {
        projectModule->onStop();
        bw_safe_delete(projectModule);
    }

}


Vector2 ProjectModule::gridPos(POINT pt) const
{
	BW_GUARD;

	Vector3 cursorPos = Moo::rc().camera().nearPlanePoint(
			(float(pt.x) / Moo::rc().screenWidth()) * 2.f - 1.f,
			1.f - (float(pt.y) / Moo::rc().screenHeight()) * 2.f );

	Matrix view;
	view.setTranslate( viewPosition_ );

	Vector3 worldRay = view.applyVector( cursorPos );
	worldRay.normalise();

	PlaneEq gridPlane( Vector3(0.f, 0.f, 1.f), .0001f );

	Vector3 gridPos = gridPlane.intersectRay( viewPosition_, worldRay );

	return Vector2( gridPos.x, gridPos.y );
}

/*~ function WorldEditor.projectLock
 *	@components{ worldeditor }
 *
 *	This function locks the project for editing the locked chunks.
 *	Locked chunks can only be edited with the editor that locked them.
 *
 *	@param commitMsg A lock message.
 */
static PyObject* py_projectLock( PyObject* args )
{
	BW_GUARD;

	char* commitMsg;
	if (!PyArg_ParseTuple( args, "s", &commitMsg ))	{
		PyErr_SetString( PyExc_TypeError, "projectLock() "
			"expects a string argument" );
		return NULL;
	}
	if( !WorldManager::instance().connection().connected() )
	{
		WorldManager::instance().addCommentaryMsg( LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/PROJECT/PROJECT_MODULE/CANNOT_LOCK" ) );
		Py_RETURN_NONE;
	}

	ProjectModule* instance = ProjectModule::currentInstance();

	if (instance)
	{
		if (instance->lockSelection( commitMsg ))
		{
			CVSInfoDialog dlg(
				 Localise(L"WORLDEDITOR/WORLDEDITOR/PROJECT/PROJECT_MODULE/LOCK_SELECTION_TITLE" ) );

			CVSWrapper cvs( ProjectModule::currentSpaceDir(), &dlg );

			BW::set<BW::string> chunks = instance->lockedChunksFromSelection( false );

			BW::vector<BW::string> filesToUpdate;
			for( BW::set<BW::string>::iterator iter = chunks.begin(); iter != chunks.end(); ++iter )
			{
				filesToUpdate.push_back( *iter + ".chunk" );
				filesToUpdate.push_back( *iter + ".cdata" );
			}

			BW::set<BW::string> graphs = instance->graphFilesFromSelection();
			for( BW::set<BW::string>::iterator iter = graphs.begin();
				iter != graphs.end(); ++iter )
			{
				if( cvs.isInCVS( *iter ) )
					filesToUpdate.push_back( *iter );
			}

			BW::set<BW::string> vlos = instance->vloFilesFromSelection();
			for( BW::set<BW::string>::iterator iter = vlos.begin();
				iter != vlos.end(); ++iter )
			{
				if( cvs.isInCVS( *iter ) )
					filesToUpdate.push_back( *iter );
			}

			if (!cvs.updateFolder( "" )||!cvs.editFiles( filesToUpdate ))
			{
				dlg.add( Localise(L"WORLDEDITOR/WORLDEDITOR/PROJECT/PROJECT_MODULE/UPDATE_FAILED" ) );
			}
			else
			{
				INFO_MSG( "cvs update done\n" );

				EditorChunkCache::forwardReadOnlyMark();

				// Clear WorldEditors's lists of what's been changed
				WorldManager::instance().resetChangedLists();

				WorldManager::instance().reloadAllChunks( false );
			}
			instance->updateLockData();
			dlg.add( Localise(L"WORLDEDITOR/WORLDEDITOR/PROJECT/PROJECT_MODULE/REGION_LOCKED" ) );
		}
	}

	Py_RETURN_NONE;
}
PY_MODULE_FUNCTION( projectLock, WorldEditor )

static PyObject* py_projectProgress( PyObject* args )
{
	Py_RETURN_NONE;
}
PY_MODULE_FUNCTION( projectProgress, WorldEditor )

/*~ function WorldEditor.invalidateSpaceMap
 *	@components{ worldeditor }
 *
 *	This function invalidates the space map, forcing a full recalculation.
 */
static PyObject* py_invalidateSpaceMap( PyObject* args )
{
	BW_GUARD;

	SpaceMap::instance().invalidateAllChunks();	
	Py_RETURN_NONE;
}
PY_MODULE_FUNCTION( invalidateSpaceMap, WorldEditor )

/*~ function WorldEditor.projectCommit
 *	@components{ worldeditor }
 *
 *	This function commits the changes to the locked chunks in the project 
 *	to the repository.
 *
 *	@param commitMsg	A commit message.
 *	@param keepLocks	An int to specify whether to keep (1) the project lock
 *						or unlock (0) the project after committing.
 */
static PyObject* py_projectCommit( PyObject* args )
{
	BW_GUARD;

	if( !WorldManager::instance().connection().connected() )
	{
		WorldManager::instance().addCommentaryMsg( LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/PROJECT/PROJECT_MODULE/CANNOT_COMMIT" ) );
		Py_RETURN_NONE;
	}

	// parse arguments
	char* commitMsg;
	int keepLocks;
	if (!PyArg_ParseTuple( args, "si", &commitMsg, &keepLocks ))	{
		PyErr_SetString( PyExc_TypeError, "projectCommitChanges() "
			"expects a string and an int argument" );
		return NULL;
	}

	WorldManager::instance().quickSave();

	ProjectModule* instance = ProjectModule::currentInstance();
	if (instance)
	{
		CVSInfoDialog dlg(
			Localise(L"WORLDEDITOR/WORLDEDITOR/PROJECT/PROJECT_MODULE/COMMIT_CHANGES_TITLE" ) );

		dlg.add( Localise(L"WORLDEDITOR/WORLDEDITOR/PROJECT/PROJECT_MODULE/COMMITTING" ) );

		typedef BW::set<BW::string> StringSet;
		StringSet chunks = instance->lockedChunksFromSelection( false );

		StringSet filesToCommit;
		StringSet foldersToCommit;

		bool inCVS = true;
		if( !CVSWrapper( ProjectModule::currentSpaceResDir(), &dlg ).isInCVS( ProjectModule::currentSpaceDir() ) )
		{
			foldersToCommit = CVSWrapper( ProjectModule::currentSpaceResDir(), &dlg ).addFolder( ProjectModule::currentSpaceDir(), commitMsg );
			inCVS = false;
		}

		CVSWrapper cvs( ProjectModule::currentSpaceDir(), &dlg );

		if( !inCVS || !cvs.isInCVS( SPACE_SETTING_FILE_NAME ) )
		{
			cvs.addFile( SPACE_SETTING_FILE_NAME, false, false );
			cvs.addFile( "*.chunk", false, true );
			cvs.addFile( "*.cdata", true, true );
			filesToCommit.insert( SPACE_SETTING_FILE_NAME );
		}

		StringSet graphs = instance->graphFilesFromSelection();
		for( StringSet::iterator iter = graphs.begin();
			iter != graphs.end(); ++iter )
		{
			if( !inCVS || !cvs.isInCVS( *iter ) )
				cvs.addFile( *iter, false, false );
		}
		filesToCommit.insert( graphs.begin(), graphs.end() );

		StringSet vlos = instance->vloFilesFromSelection();
		for( StringSet::iterator iter = vlos.begin();
			iter != vlos.end(); ++iter )
		{
			if( !inCVS || !cvs.isInCVS( *iter ) )
			{
				if( strstr( iter->c_str(), ".odata" ) )
					cvs.addFile( *iter, true, false );
				else
					cvs.addFile( *iter, false, false );
			}
		}
		filesToCommit.insert( vlos.begin(), vlos.end() );

		for( StringSet::iterator iter = chunks.begin(); iter != chunks.end(); ++iter )
		{
			filesToCommit.insert( *iter + ".chunk" );
			if( ( BWResource::fileExists( ProjectModule::currentSpaceDir() + '/' + *iter + ".cdata" ) ||
				cvs.isInCVS( *iter + ".cdata" ) ) )
				filesToCommit.insert( *iter + ".cdata" );
		}

		StringSet filesToCommitFullPaths;
		BW::string path = ProjectModule::currentSpaceDir() + '/';
		for( StringSet::iterator iter = filesToCommit.begin(); iter != filesToCommit.end(); ++iter )
		{
			filesToCommitFullPaths.insert( path + *iter );
		}

		if( CVSWrapper( ProjectModule::currentSpaceResDir(), &dlg )
			.commitFiles( filesToCommitFullPaths, foldersToCommit, commitMsg ) )
		{
			CVSWrapper( ProjectModule::currentSpaceDir(), &dlg ).refreshFolder( "" );

			for( StringSet::iterator iter = chunks.begin(); iter != chunks.end(); ++iter )
			{
				BW::wstring wfilename;
				bw_utf8tow( ( ProjectModule::currentSpaceAbsoluteDir() + '/' + *iter + ".~chunk~" ), wfilename );
				::DeleteFile( wfilename.c_str() );
			}

			instance->commitDone();

			if (keepLocks)
			{
				CVSWrapper( ProjectModule::currentSpaceResDir(), &dlg )
					.editFiles( BW::vector<BW::string>( filesToCommitFullPaths.begin(), filesToCommitFullPaths.end() ) );
			}
			else
				instance->discardLocks( commitMsg );

			EditorChunkCache::forwardReadOnlyMark();

			dlg.add( Localise(L"WORLDEDITOR/WORLDEDITOR/PROJECT/PROJECT_MODULE/CHANGES_COMMITED" ) );
		}
	}

	Py_RETURN_NONE;
}
PY_MODULE_FUNCTION( projectCommit, WorldEditor )

/*~ function WorldEditor.projectDiscard
 *	@components{ worldeditor }
 *
 *	This function discards changes made to the locked chunks while the project was locked.
 *
 *	@param commitMsg	A commit message.
 *	@param keepLocks	An int to specify whether to keep (1) the project lock
 *						or unlock (0) the project after discarding.
 */
static PyObject* py_projectDiscard( PyObject* args )
{
	BW_GUARD;

	if( !WorldManager::instance().connection().connected() )
		Py_RETURN_NONE;

	// parse arguments
	char* commitMsg;
	int keepLocks;
	if (!PyArg_ParseTuple( args, "si", &commitMsg, &keepLocks ))	{
		PyErr_SetString( PyExc_TypeError, "projectDiscardChanges() "
			"expects a string and an int argument" );
		return NULL;
	}

	CVSInfoDialog dlg(
		Localise(L"WORLDEDITOR/WORLDEDITOR/PROJECT/PROJECT_MODULE/DISCARD_LOCKS_TITLE" ) );

	ProjectModule* instance = ProjectModule::currentInstance();

	if( !CVSWrapper( ProjectModule::currentSpaceResDir(), &dlg ).isInCVS( ProjectModule::currentSpaceDir() ) )
	{
		dlg.add( Localise(L"WORLDEDITOR/WORLDEDITOR/PROJECT/PROJECT_MODULE/CHANGES_DISCARDED" ) );
		WorldManager::instance().addCommentaryMsg( LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/PROJECT/PROJECT_MODULE/CHUNK_LOCKS_DISCARDED" ) );
		if( !keepLocks && instance )
			instance->discardLocks( commitMsg );
		Py_RETURN_NONE;
	}

	if (instance)
	{
		BW::set<BW::string> chunks = instance->lockedChunksFromSelection( false );

		CVSWrapper cvs( ProjectModule::currentSpaceDir(), &dlg );

		BW::vector<BW::string> filesToUpdate;
		for( BW::set<BW::string>::iterator iter = chunks.begin(); iter != chunks.end(); ++iter )
		{
			filesToUpdate.push_back( *iter + ".chunk" );
			filesToUpdate.push_back( *iter + ".cdata" );
		}

		BW::set<BW::string> graphs = instance->graphFilesFromSelection();
		for( BW::set<BW::string>::iterator iter = graphs.begin();
			iter != graphs.end(); ++iter )
		{
			if( cvs.isInCVS( *iter ) )
				filesToUpdate.push_back( *iter );
		}

		BW::set<BW::string> vlos = instance->vloFilesFromSelection();
		for( BW::set<BW::string>::iterator iter = vlos.begin();
			iter != vlos.end(); ++iter )
		{
			if( cvs.isInCVS( *iter ) )
				filesToUpdate.push_back( *iter );
		}

		if( cvs.revertFiles( filesToUpdate ) )
		{
			for( BW::set<BW::string>::iterator iter = chunks.begin(); iter != chunks.end(); ++iter )
			{
				BW::wstring wfilename;
				bw_utf8tow( ( ProjectModule::currentSpaceAbsoluteDir() + '/' + *iter + ".~chunk~" ), wfilename );
				::DeleteFile( wfilename.c_str() );
			}

			if (keepLocks)
				cvs.editFiles( filesToUpdate );
			else
				instance->discardLocks( commitMsg );

			EditorChunkCache::forwardReadOnlyMark();

			// Clear the undo buffer
			UndoRedo::instance().clear();

			// Clear WorldEditors's lists of what's been changed
			WorldManager::instance().resetChangedLists();

			WorldManager::instance().reloadAllChunks( false );

			INFO_MSG( "Changes discarded\n" );
			dlg.add( Localise(L"WORLDEDITOR/WORLDEDITOR/PROJECT/PROJECT_MODULE/CHANGES_DISCARDED" ) );
		}
	}

	Py_RETURN_NONE;
}
PY_MODULE_FUNCTION( projectDiscard, WorldEditor )


/*~ function WorldEditor.projectUpdateLODOverlay
 *	@components{ worldeditor }
 *
 *	This function updates the LOD overlay texture, potentially scanning all of the chunks on disk.
 */
static PyObject* py_projectUpdateLODOverlay( PyObject* args )
{
	// Update forced LOD map with space details
	ForcedLODMap& forcedLodMap = WorldManager::instance().forcedLodMap();
	forcedLodMap.updateTexture();

	Py_RETURN_NONE;
}
PY_MODULE_FUNCTION( projectUpdateLODOverlay, WorldEditor )

/*~ function WorldEditor.projectUpdateSpace
 *	@components{ worldeditor }
 *
 *	This function retrieves updates made to the space from the repository.
 */
static PyObject* py_projectUpdateSpace( PyObject* args )
{
	BW_GUARD;

	CVSInfoDialog dlg(
		Localise(L"WORLDEDITOR/WORLDEDITOR/PROJECT/PROJECT_MODULE/UPDATE_SPACE_TITLE" ) );

	ProjectModule* instance = ProjectModule::currentInstance();

	if( !CVSWrapper( ProjectModule::currentSpaceResDir(), &dlg ).isInCVS( ProjectModule::currentSpaceDir() ) )
	{
		INFO_MSG( "Space not under version control\n" );
		WorldManager::instance().addCommentaryMsg( LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/PROJECT/PROJECT_MODULE/SPACE_NOT_CONTROLLED" ) );
		Py_RETURN_NONE;
	}

	if (instance)
	{
		CVSWrapper cvs( ProjectModule::currentSpaceDir(), &dlg );

		if( cvs.updateFolder( "" ) )
		{
			// Clear WorldEditors's lists of what's been changed
			WorldManager::instance().resetChangedLists();

			WorldManager::instance().reloadAllChunks( false );

			INFO_MSG( "Space updated\n" );
			WorldManager::instance().addCommentaryMsg( LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/PROJECT/PROJECT_MODULE/SPACE_UPDATED" ) );
		}
	}


	Py_RETURN_NONE;
}
PY_MODULE_FUNCTION( projectUpdateSpace, WorldEditor )

/*~ function WorldEditor.projectMapAlpha
 *	@components{ worldeditor }
 *
 *	This function gets the current project map's alpha value if no alpha is passed in.
 *	If an alpha is passed as a parameter then the current project map's alpha
 *	will be set to that value.
 *
 *	@param alpha	Optional Float value. If this is provided then the current
 *					project's map alpha will be set to this value.
 *
 *	@return Returns the current project's map alpha if no alpha was provided, otherwise
 *			returns 0.
 */
static PyObject* py_projectMapAlpha( PyObject* args )
{
	BW_GUARD;

	ProjectModule *pinstance = ProjectModule::currentInstance();
    HeightModule  *hinstance = HeightModule ::currentInstance();
	if (pinstance != NULL || hinstance != NULL)
	{
		float alpha;
		if (PyArg_ParseTuple( args, "f", &alpha ))
		{
			if (pinstance != NULL)
                pinstance->projectMapAlpha(alpha);
            if (hinstance != NULL)
                hinstance->projectMapAlpha(alpha);
			return PyFloat_FromDouble( 0 );
		}
		else
		{
			PyErr_Clear();
            if (hinstance != NULL)
			    return PyFloat_FromDouble( hinstance->projectMapAlpha() );
            else
                return PyFloat_FromDouble( pinstance->projectMapAlpha() );
		}
	}

	return PyFloat_FromDouble( 0 );
}
PY_MODULE_FUNCTION( projectMapAlpha, WorldEditor )
BW_END_NAMESPACE

