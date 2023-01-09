#include "pch.hpp"
#include "worldeditor/height/height_module.hpp"
#include "worldeditor/height/height_map.hpp"
#include "worldeditor/import/terrain_utils.hpp"
#include "worldeditor/gui/pages//page_terrain_import.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "worldeditor/misc/world_editor_camera.hpp"
#include "worldeditor/framework/world_editor_app.hpp"
#include "worldeditor/framework/mainframe.hpp"
#include "worldeditor/gui/dialogs/cvs_info_dialog.hpp"
#include "worldeditor/gui/pages/panel_manager.hpp"
#include "worldeditor/world/editor_chunk_cache.hpp"
#include "worldeditor/import/elevation_blit.hpp"
#include "worldeditor/import/texture_mask_blit.hpp"
#include "appmgr/app.hpp"
#include "appmgr/module_manager.hpp"
#include "appmgr/options.hpp"
#include "appmgr/closed_captions.hpp"
#include "ashes/text_gui_component.hpp"
#include "moo/effect_visual_context.hpp"
#include "moo/vertex_formats.hpp"
#include "moo/draw_context.hpp"
#include "moo/custom_mesh.hpp"
#include "romp/time_of_day.hpp"
#include "romp/font_manager.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/string_provider.hpp"
#include "chunk/chunk_manager.hpp"
#include "chunk/chunk_space.hpp"
#include "chunk/geometry_mapping.hpp"
#include "common/base_camera.hpp"
#include "cstdmf/memory_load.hpp"
#include "ashes/simple_gui.hpp"
#include "moo/camera.hpp"


DECLARE_DEBUG_COMPONENT2( "HeightModule", 2 );

BW_BEGIN_NAMESPACE

typedef ModuleManager ModuleFactory;

namespace 
{
    enum ModifierKey
    {
        CTRL_KEY,
        SHIFT_KEY,
        ALT_KEY
    };

    // Modifier keys for editing.  If these are changed, the terrain import
    // panel's resource's help text needs to be changed too.
    ModifierKey     SnapToGridKey   = SHIFT_KEY;
    ModifierKey     SetMinKey       = CTRL_KEY;
    ModifierKey     SetMaxKey       = ALT_KEY;

    // Return true if the given key is down:
    bool isKeyDown(ModifierKey key)
    {
		BW_GUARD;

        switch (key)
        {
        case CTRL_KEY:
            return InputDevices::isCtrlDown();
        case SHIFT_KEY:
            return InputDevices::isShiftDown();
        case ALT_KEY:
            return InputDevices::isAltDown();
            break;
        default:
            return false;
        }
    }

    // Return true if snapping should occur:
    bool shouldGridSnap()
    {
		BW_GUARD;

        return isKeyDown(SnapToGridKey);
    }

    // Return true if getting the max. height should occur:
    bool shouldGetMaxHeight()
    {
		BW_GUARD;

        return isKeyDown(SetMaxKey);
    }

    // Return true if getting the min. height should occur:
    bool shouldGetMinHeight()
    {
		BW_GUARD;

        return isKeyDown(SetMinKey);
    }

    template<class V>
    void quad
    ( 
        CustomMesh<V>   &mesh,
        Vector3         const &bottomLeft, 
        float           xextent, 
        float           yextent,
        Vector2         const &bottomLeftUV, 
        float           xuvextent, 
        float           yuvextent,
        Vector2         const &bottomLeftUV2, 
        float           xuvextent2, 
        float           yuvextent2,
        bool            asLines     = false
    )
    {
		BW_GUARD;

        // Bottom left:
        V bl;
        bl.pos_ = bottomLeft;
        bl.uv_  = bottomLeftUV;
        bl.uv2_ = bottomLeftUV2;

        // Top left:
        V tl;
        tl.pos_ = bottomLeft    + Vector3(0.0f, yextent, 0.0f);
        tl.uv_  = bottomLeftUV  + Vector2(0.0f, yuvextent );
        tl.uv2_ = bottomLeftUV2 + Vector2(0.0f, yuvextent2);

        // Bottom right
        V br;
        br.pos_ = bottomLeft    + Vector3(xextent   , 0.0f, 0.0f);
        br.uv_  = bottomLeftUV  + Vector2(xuvextent , 0.0f);
        br.uv2_ = bottomLeftUV2 + Vector2(xuvextent2, 0.0f);

        // Top right
        V tr;
        tr.pos_ = bottomLeft    + Vector3(xextent, yextent, 0.0f);
        tr.uv_  = bottomLeftUV  + Vector2(xuvextent , yuvextent );
        tr.uv2_ = bottomLeftUV2 + Vector2(xuvextent2, yuvextent2);

        if (asLines)
        {
            mesh.push_back(tl);
            mesh.push_back(tr);
            mesh.push_back(br);
            mesh.push_back(bl);
            mesh.push_back(tl);
        }
        else
        {
            mesh.push_back(tl);
            mesh.push_back(tr);
            mesh.push_back(br);

            mesh.push_back(br);
            mesh.push_back(bl);
            mesh.push_back(tl);
        }
    }

    template<class V>
    void quad
    ( 
        CustomMesh<V>       &mesh, 
        Vector3             const &bottomLeft, 
        float               xextent, 
        float               yextent, 
        DWORD               colour,
        bool                asLines = false
    )
    {
		BW_GUARD;

        // Bottom left:
        V bl;
        bl.pos_    = bottomLeft;
        bl.colour_ = colour;

        // Top left:
        V tl;
        tl.pos_    = bottomLeft + Vector3( 0.0f, yextent, 0.0f );
        tl.colour_ = colour;

        // Bottom right:
        V br;
        br.pos_    = bottomLeft + Vector3( xextent, 0.0f, 0.0f );
        br.colour_ = colour;

        // Top right:
        V tr;
        tr.pos_    = bottomLeft + Vector3( xextent, yextent, 0.0f );
        tr.colour_ = colour;

        if (asLines)
        {
            mesh.push_back(tl);
            mesh.push_back(tr);
            mesh.push_back(br);
            mesh.push_back(bl);
            mesh.push_back(tl);
        }
        else
        {
            mesh.push_back(tl);
            mesh.push_back(tr);
            mesh.push_back(br);

            mesh.push_back(br);
            mesh.push_back(bl);
            mesh.push_back(tl);
        }
    }

    template<class V>
    void quad
    (
        CustomMesh<V>       &mesh, 
        Vector2             const &topLeft, 
        Vector2             const &bottomRight,
        DWORD               colour,
        bool                asLines = false
    )
    {
		BW_GUARD;

        // Bottom left:
        V bl;
        bl.pos_.x   = topLeft.x;
        bl.pos_.y   = bottomRight.y;
        bl.pos_.z   = 0.0f;
        bl.colour_  = colour;

        // Top left:
        V tl;
        tl.pos_.x   = topLeft.x;
        tl.pos_.y   = topLeft.y;
        tl.pos_.z   = 0.0f;
        tl.colour_  = colour;

        // Bottom right:
        V br;
        br.pos_.x   = bottomRight.x;
        br.pos_.y   = bottomRight.y;
        br.pos_.z   = 0.0f;
        br.colour_  = colour;

        // Top right:
        V tr;
        tr.pos_.x   = bottomRight.x;
        tr.pos_.y   = topLeft.y;
        tr.pos_.z   = 0.0f;
        tr.colour_  = colour;

        if (asLines)
        {
            mesh.push_back(tl);
            mesh.push_back(tr);
            mesh.push_back(br);
            mesh.push_back(bl);
            mesh.push_back(tl);
        }
        else
        {
            mesh.push_back(tl);
            mesh.push_back(tr);
            mesh.push_back(br);

            mesh.push_back(br);
            mesh.push_back(bl);
            mesh.push_back(tl);
        }
    }

    template<class V>
    void vertQuad
    (
        CustomMesh<V>       &mesh, 
        float               x,
        float               hx,
        DWORD               colour,
        bool                asLines = false
    )
    {
		BW_GUARD;

        // Bottom left:
        V bl;
        bl.pos_.x   = x - hx;
        bl.pos_.y   = 65535.0f;
        bl.pos_.z   = 0.0f;
        bl.colour_  = colour;

        // Top left:
        V tl;
        tl.pos_.x   = x - hx;
        tl.pos_.y   = -65535.0f;
        tl.pos_.z   = 0.0f;
        tl.colour_  = colour;

        // Bottom right:
        V br;
        br.pos_.x   = x + hx;
        br.pos_.y   = 65535.0f;
        br.pos_.z   = 0.0f;
        br.colour_  = colour;

        // Top right:
        V tr;
        tr.pos_.x   = x + hx;
        tr.pos_.y   = -65535.0f;
        tr.pos_.z   = 0.0f;
        tr.colour_  = colour;

        if (asLines)
        {
            mesh.push_back(tl);
            mesh.push_back(tr);
            mesh.push_back(br);
            mesh.push_back(bl);
            mesh.push_back(tl);
        }
        else
        {
            mesh.push_back(tl);
            mesh.push_back(tr);
            mesh.push_back(br);

            mesh.push_back(br);
            mesh.push_back(bl);
            mesh.push_back(tl);
        }
    }

    template<class V>
    void horzQuad
    (
        CustomMesh<V>       &mesh, 
        float               y,
        float               hy,
        DWORD               colour,
        bool                asLines = false
    )
    {
		BW_GUARD;

        // Bottom left:
        V bl;
        bl.pos_.x   = 65535.0f;
        bl.pos_.y   = y - hy;        
        bl.pos_.z   = 0.0f;
        bl.colour_  = colour;

        // Top left:
        V tl;
        tl.pos_.x   = -65535.0f;
        tl.pos_.y   = y - hy;        
        tl.pos_.z   = 0.0f;
        tl.colour_  = colour;

        // Bottom right:
        V br;
        br.pos_.x   = 65535.0f;
        br.pos_.y   = y + hy;        
        br.pos_.z   = 0.0f;
        br.colour_  = colour;

        // Top right:
        V tr;
        tr.pos_.x   = -65535.0f;
        tr.pos_.y   = y + hy;        
        tr.pos_.z   = 0.0f;
        tr.colour_  = colour;

        if (asLines)
        {
            mesh.push_back(tl);
            mesh.push_back(tr);
            mesh.push_back(br);
            mesh.push_back(bl);
            mesh.push_back(tl);
        }
        else
        {
            mesh.push_back(tl);
            mesh.push_back(tr);
            mesh.push_back(br);

            mesh.push_back(br);
            mesh.push_back(bl);
            mesh.push_back(tl);
        }
    }

    template<class V>
    void quad
    ( 
        CustomMesh<V>       &mesh,
        Vector3             const &bottomLeft, 
        float               xextent, 
        float               yextent,
	    Vector2             const &bottomLeftUV, 
        float               xuvextent, 
        float               yuvextent 
    )
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
}


IMPLEMENT_CREATOR(HeightModule, Module);


HeightModule *		HeightModule::s_currentInstance_	= NULL;
ImportImagePtr		HeightModule::s_elevationData_;
ImportImagePtr		HeightModule::s_maskData_;
Vector2				HeightModule::s_terrainTopLeft_		= Vector2::zero();
Vector2				HeightModule::s_terrainBottomRight_	= Vector2::zero();
Vector2				HeightModule::s_maskTopLeft_		= Vector2::zero();
Vector2				HeightModule::s_maskBottomRight_	= Vector2::zero();
float				HeightModule::s_texStrength_		= 1.0f;


HeightModule::HeightModule() : 
	hasStarted_( false ),
    gridWidth_(0), 
    gridHeight_(0),
    minX_(0),
    minY_(0),
    spaceAlpha_(0.5f),
    localToWorld_(GridCoord::zero()),
    mouseDownOn3DWindow_(false),
    mode_(EXPORT_TERRAIN),
    terrainShader_(NULL),
    selectionShader_(NULL),
    impMode_(ElevationBlit::REPLACE),
    impMinV_(0.0f),
    impMaxV_(0.0f),
    impStrength_(1.0f),
    gettingHeight_(false),
    lastCursorPos_(-1, -1),
	absoluteHeights_(false)
{
    s_currentInstance_ = this;
}


HeightModule::~HeightModule()
{
	BW_GUARD;

	if( abortedText_.exists() )
	{
		SimpleGUI::instance().removeSimpleComponent( *abortedText_ );
		abortedText_ = NULL;
	}
    s_currentInstance_ = NULL;
}


bool HeightModule::init(DataSectionPtr pSection)
{
    return true;
}


/**
 *	This static method clears the static smart pointers so they get deleted.
 */
/*static*/ void HeightModule::fini()
{
	BW_GUARD;

	if (s_currentInstance_)
	{
		ERROR_MSG( "HeightModule::fini: Detected memory leak, "
						"HeightModule instance was not deleted properly.\n" );
	}

	s_elevationData_ = NULL;
	s_maskData_ = NULL;
}


void HeightModule::onStart()
{
	BW_GUARD;

    // Needed, otherwise the mouse cursor is hidden when we start?!
    ::ShowCursor(1);

    CWaitCursor waitCursor; // this may take a while

    // Read some options in:
    spaceAlpha_ = 
        Options::getOptionFloat("render/project/spaceMapAlpha", 0.5f);
    spaceAlpha_ = Math::clamp(0.0f, spaceAlpha_, 1.0f);

    BW::string space = WorldManager::instance().getCurrentSpace();

    handDrawnMap_ = 
        Moo::TextureManager::instance()->get
        (
            space + "/space.map.bmp",
            false,                      // allowAnimation
            false                       // mustExist
        );

    if (!handDrawnMap_)
    {
        handDrawnMap_ = 
            Moo::TextureManager::instance()->get
            (
                "resources/maps/default.map.bmp",
                false,                      // allowAnimation
                false                       // mustExist
            );
    }

    // Work out grid size:
    int minX, minY, maxX, maxY;
	float gridSize;
    TerrainUtils::terrainSize(space, gridSize, minX, minY, maxX, maxY);
	gridSize_		= gridSize;
	minX_           = minX;
	minY_           = minY;
    gridWidth_      = maxX - minX + 1;
    gridHeight_     = maxY - minY + 1;
    localToWorld_   = GridCoord( minX, minY );

    // Work out the selection area:
    static BW::string lastSpace;
    if (lastSpace != space)
    {
        lastSpace = space;

		maskTopLeft_		= Vector2(0.0f, static_cast<float>(gridHeight_));
		maskBottomRight_	= Vector2(static_cast<float>(gridWidth_), 0.0f);
		terrainTopLeft_     = Vector2(0.0f, static_cast<float>(gridHeight_));
		terrainBottomRight_ = Vector2(static_cast<float>(gridWidth_), 0.0f);
	
		updateWorldRect();
    }
    else
    {
		maskTopLeft_ = 
			Options::getOptionVector2
			(
				"render/height/maskTopLeft", 
				Vector2(0.0f, static_cast<float>(gridHeight_))
			);
		maskBottomRight_ = 
			Options::getOptionVector2
			(
				"render/height/maskBottomRight", 
				Vector2(static_cast<float>(gridWidth_), 0.0f)
			);

		terrainTopLeft_ = 
			Options::getOptionVector2
			(
				"render/height/selTopLeft", 
				Vector2(0.0f, static_cast<float>(gridHeight_))
			);
		terrainBottomRight_ = 
			Options::getOptionVector2
			(
				"render/height/selBottomRight", 
				Vector2(static_cast<float>(gridWidth_), 0.0f)
			);

		updateWorldRect();
    }

    viewPosition_ = Vector3(gridWidth_/2.0f, gridHeight_/2.0f, -1.0f);

    // Set the zoom to the extents of the grid
    float angle = Moo::rc().camera().fov()/2.0f;
    float yopp  = gridHeight_/2.0f;
    float xopp  = gridWidth_ /2.0f;

    // Get the distance away we have to be to see the x points and the y points
    float yheight = yopp/tanf(angle);
    float xheight = xopp/tanf(angle*Moo::rc().camera().aspectRatio());
    
    // Go back the furthest amount between the two of them
    viewPosition_.z = min(-1.05f*xheight, -1.05f*yheight);
    minViewZ_ = viewPosition_.z*1.1f;
    savedFarPlane_ = Moo::rc().camera().farPlane();
    Moo::Camera camera = Moo::rc().camera();
    camera.farPlane(minViewZ_*-1.1f);
    Moo::rc().camera(camera);

    // Create an automatic space map, to display the current progress
    HeightMap::instance().spaceInformation
    (
        space, 
        localToWorld_.x, 
        localToWorld_.y,
        gridWidth_, 
        gridHeight_
    );

    cc_ = SmartPointer<ClosedCaptions>(new ClosedCaptions(), true);
    Commentary::instance().addView(&*cc_);
    cc_->visible( true );

    // Load the shaders:
    BW::string terrainShaderFile = "resources/shaders/heightmap.fx";
    if (HeightMap::instance().isPS2())
    {
        terrainShaderFile = "resources/shaders/heightmap2.fx";
    }
    terrainShader_ = new Moo::EffectMaterial();
    terrainShader_->initFromEffect(terrainShaderFile);

    BW::string selShaderFile = "resources/shaders/heightmap_sel.fx";
    selectionShader_ = new Moo::EffectMaterial();
    selectionShader_->initFromEffect(selShaderFile);

	hasStarted_ = true;
}


int HeightModule::onStop()
{
	BW_GUARD;

	hasStarted_ = false;

    ::ShowCursor(0);

    HeightMap::instance().save();
    HeightMap::instance().onStop();

    Options::setOptionFloat
    (
        "render/project/spaceMapAlpha", 
        spaceAlpha_
    );

    Options::setOptionVector2
    (
        "render/height/selTopLeft", 
        terrainTopLeft_
    );
    Options::setOptionVector2
    (
        "render/height/selBottomRight", 
        terrainBottomRight_
    );

    Options::setOptionVector2
    (
        "render/height/maskTopLeft", 
        maskTopLeft_
    );
    Options::setOptionVector2
    (
        "render/height/maskBottomRight", 
        maskBottomRight_
    );

    cc_->visible(false);
    Commentary::instance().delView(&*cc_);
    cc_ = NULL;

    Moo::Camera camera = Moo::rc().camera();
    camera.farPlane(savedFarPlane_);
    Moo::rc().camera(camera);

	terrainShader_   = NULL;
	selectionShader_ = NULL;

    return 0;
}


void HeightModule::onPause()
{
	BW_GUARD;

	if (!hasStarted_)
	{
		return;
	}

    cc_->visible(false);
    Commentary::instance().delView(&*cc_);
}


void HeightModule::onResume( int exitCode )
{
	BW_GUARD;

	if (!hasStarted_)
	{
		return;
	}

    Commentary::instance().addView(&*cc_);
    cc_->visible( true );
}


bool HeightModule::updateState(float dTime)
{
	BW_GUARD;

	if (!hasStarted_)
	{
		return true;
	}

    cc_->update( dTime );
    HeightMap::instance().update(dTime);

    // Set input focus as appropriate:
    bool acceptInput = WorldManager::instance().cursorOverGraphicsWnd();
    InputDevices::setFocus(acceptInput, this);

    // Handle show/hide cursor for panning:
    if (InputDevices::isKeyDown(KeyCode::KEY_RIGHTMOUSE))
    {
        if (lastCursorPos_.x == -1 && lastCursorPos_.y == -1)
        {
            ::ShowCursor(FALSE);
            ::GetCursorPos(&lastCursorPos_);
        }
        ::SetCursorPos(lastCursorPos_.x, lastCursorPos_.y);
    }
    else
    {
        if (lastCursorPos_.x != -1 && lastCursorPos_.y != -1)
        {
            ::ShowCursor(TRUE);
            lastCursorPos_ = CPoint(-1, -1);
        }
    }

    SimpleGUI::instance().update(dTime);

    return true;
}


/**
 *	Update the animations of scales in this module.
 */
void HeightModule::updateAnimations()
{
	BW_GUARD_PROFILER( HeightModule_updateAnimations );

	static DogWatch updateAnimations( "Scene update" );
	ScopedDogWatch sdw( updateAnimations );

	if (!hasStarted_)
	{
		return;
	}

	// Update the scales.
	CPoint pt(0, 0);
	Vector2 worldPos = gridPos(pt);
	++pt.x;
	Vector2 xWorldPos = gridPos(pt);
	--pt.x; ++pt.y;
	Vector2 yWorldPos = gridPos(pt);
	scaleX_ = fabsf(xWorldPos.x - worldPos.x);
	scaleY_ = fabsf(yWorldPos.y - worldPos.y);
}


void HeightModule::render( float dTime )
{
	BW_GUARD;

	if (!hasStarted_)
	{
		return;
	}

    if (!Moo::rc().device())
        return;

    if (!handDrawnMap_)
        return;

    DX::Device* device_ = Moo::rc().device();

    device_->Clear
    ( 
        0, 
        NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
        0xff004e98, // blue
        1, 
        0 
    );
    Moo::rc().reset();
	Moo::rc().updateProjectionMatrix();
    Moo::rc().updateViewTransforms();

	Moo::rc().effectVisualContext().initConstants();
    ComObjectWrap<ID3DXEffect> pEffect = terrainShader_->pEffect()->pEffect();

	if( HeightMap::instance().aborted() )
	{
		if( !abortedText_.exists() )
		{
			abortedText_ = new TextGUIComponent( FontManager::instance().getCachedFont("default_medium.font") );
			abortedText_->filterType( SimpleGUIComponent::FT_LINEAR );
			abortedText_->slimLabel( "Height map preview generation was aborted" );
			abortedText_->colour( 0xffC76535 );//199,101,53
			abortedText_->position( Vector3( 0.f, 0.5f, 1.f ) );
			SimpleGUI::instance().addSimpleComponent( *abortedText_ );
		}

		abortedText_->update( dTime, Moo::rc().screenWidth(), Moo::rc().screenHeight() );
		Moo::DrawContext drawContext( Moo::RENDERING_PASS_COLOR );
		drawContext.begin( Moo::DrawContext::OPAQUE_CHANNEL_MASK );
		drawContext.pushImmediateMode();
		abortedText_->draw( drawContext, true );
		drawContext.popImmediateMode();
		drawContext.end( Moo::DrawContext::OPAQUE_CHANNEL_MASK );
	}

	Matrix view;
    view.setTranslate(viewPosition_);
    view.invert();
    Matrix WVP( Moo::rc().world() );
    WVP.postMultiply( view );
    WVP.postMultiply( Moo::rc().projection() );
    pEffect->SetMatrix("worldViewProjection", &WVP);

    HeightMap::instance().update(dTime);

    float minV = HeightMap::normaliseHeight(HeightMap::instance().minHeight());
    float maxV = HeightMap::normaliseHeight(HeightMap::instance().maxHeight());

    float invScaleHeight = 1.0f/(maxV - minV);
    pEffect->SetFloat("minHeight", minV);
    pEffect->SetFloat("invScaleHeight", invScaleHeight);
    pEffect->SetFloat("alpha", spaceAlpha_);
    pEffect->SetTexture("handDrawnMap", handDrawnMap_->pTexture());
    pEffect->SetTexture("heightMap", HeightMap::instance().texture());

    bool flipTerrain = !HeightMap::instance().isPS2();

    terrainShader_->begin();
    terrainShader_->beginPass(0);
    {
		CustomMesh<Moo::VertexXYZUV2> mesh;
        quad
        ( 
            mesh, 
            Vector3(0.0f, 0.0f, 0.0f),
            static_cast<float>(gridWidth_ ),
            static_cast<float>(gridHeight_),
            flipTerrain ? Vector2(0.0f, 0.0f) : Vector2(0.0f, 1.0f), 
            1.0f,   
            flipTerrain ? +1.0f : -1.0f,
            Vector2( 0.0f, 0.0f ), 
            1.0f, 
            1.0f
        );
        mesh.drawEffect();
    }
    terrainShader_->endPass();
    terrainShader_->end();

    // Draw the current grid, selection:
    pEffect = selectionShader_->pEffect()->pEffect();
    pEffect->SetMatrix("worldViewProjection", &WVP);

    selectionShader_->begin();
    selectionShader_->beginPass(0);
    {
        // If getting height, draw the selected rectangle:
        if (gettingHeight_)
        {
            bool showMaxV = shouldGetMaxHeight();
            bool showMinV = shouldGetMinHeight();
            DWORD colour = 0x40000000;
            if (showMaxV)
                colour |= 0x000000ff;
            if (showMinV)
                colour |= 0x0000ff00;
            drawRect(heightDownPt_, heightCurrentPt_, colour);
        }

        // Draw the grid if Alt is down:
        if (shouldGridSnap())
        {
            for (float x = -4.0f; x <= gridWidth_ + 4.0f; x += 1.0f)
            {
                drawVLine(x, 0x40007f00, 0.5);
            }
            for (float y = -4.0f; y <= gridHeight_ + 4.0f; y += 1.0f)
            {
                drawHLine(y, 0x40007f00, 0.5);
            }
        }

		Vector2 tl = (mode_ == IMPORT_MASK) ? maskTopLeft_     : terrainTopLeft_    ;
		Vector2 br = (mode_ == IMPORT_MASK) ? maskBottomRight_ : terrainBottomRight_;

		drawRect(tl, br, 0x7f0000ff, 1);

        drawHandle(Vector2(tl.x              , tl.y              ), 0xff0000ff, 3, 3);
        drawHandle(Vector2(tl.x              , 0.5f*(br.y + tl.y)), 0xff0000ff, 3, 3);
        drawHandle(Vector2(tl.x              , br.y              ), 0xff0000ff, 3, 3);
        drawHandle(Vector2(br.x              , tl.y              ), 0xff0000ff, 3, 3);
        drawHandle(Vector2(br.x              , 0.5f*(br.y + tl.y)), 0xff0000ff, 3, 3);
        drawHandle(Vector2(br.x              , br.y              ), 0xff0000ff, 3, 3);
        drawHandle(Vector2(0.5f*(tl.x + br.x), tl.y              ), 0xff0000ff, 3, 3);
        drawHandle(Vector2(0.5f*(tl.x + br.x), br.y              ), 0xff0000ff, 3, 3);

        // Draw the camera position:
        //Matrix view = WorldEditorCamera::instance().currentCamera().view();
        //view.invert();
        //Vector3 pos = view.applyToOrigin();
        //Vector2 gridPos = worldPosToGridPos(pos);
        //drawHandle
        //(
        //    gridPos,
        //    0xffff0000, 5, 1
        //);   
        //drawHandle
        //(
        //    gridPos,
        //    0xffff0000, 1, 5
        //); 
    }
    selectionShader_->endPass();
    selectionShader_->end();

    SimpleGUI::instance().draw();
    writeStatus();
}


bool HeightModule::handleKeyEvent(const KeyEvent & keyEvent)
{
	BW_GUARD;

	if (!hasStarted_)
	{
		return false;
	}

    updateCursor();
        
    if (keyEvent.key() == KeyCode::KEY_LEFTMOUSE)
    {
        if ( keyEvent.isKeyDown() )
        {
            mouseDownOn3DWindow_ = true;
        }
        else
        {
            mouseDownOn3DWindow_ = false;
        }
    }

    // Go to the world location when the middle mouse button is held down
    if (keyEvent.key() == KeyCode::KEY_MIDDLEMOUSE && !keyEvent.isKeyDown())
    {
        gotoWorldPos(keyEvent);
        return true;
    }

    // Start getting height:
    if
    (
        keyEvent.key() == KeyCode::KEY_LEFTMOUSE 
        &&
        keyEvent.isKeyDown()
        &&
		(
			mode_ == IMPORT_TERRAIN
			||
			mode_ == EXPORT_TERRAIN
		)
        &&
        (
            shouldGetMaxHeight()
            ||
            shouldGetMinHeight()
        )
    )
    {
        startGetHeight(keyEvent);
    }

    // Stop getting height:
    if
    (
        keyEvent.key() == KeyCode::KEY_LEFTMOUSE 
        &&
        !keyEvent.isKeyDown()
        &&
        gettingHeight_
    )
    {
        gettingHeight_ = false;
    }

    // Editing imported terrain case:
    if
    (
        keyEvent.key() == KeyCode::KEY_LEFTMOUSE 
        && 
        keyEvent.isKeyDown()
        && 
        !gettingHeight_
    )
    {
        startTerrainEdit(keyEvent);
    }

	if (keyEvent.key() == KeyCode::KEY_1 && keyEvent.isKeyDown() && !keyEvent.modifiers())
	{
		PanelManager::instance().setToolMode( L"Objects" );
	}
	if (keyEvent.key() == KeyCode::KEY_2 && keyEvent.isKeyDown() && !keyEvent.modifiers())
	{
		PanelManager::instance().setToolMode( L"TerrainTexture" );
	}
	if (keyEvent.key() == KeyCode::KEY_3 && keyEvent.isKeyDown() && !keyEvent.modifiers())
	{
		PanelManager::instance().setToolMode( L"TerrainHeight" );
	}
	if (keyEvent.key() == KeyCode::KEY_4 && keyEvent.isKeyDown() && !keyEvent.modifiers())
	{
		PanelManager::instance().setToolMode( L"TerrainFilter" );
	}
	if (keyEvent.key() == KeyCode::KEY_5 && keyEvent.isKeyDown() && !keyEvent.modifiers())
	{
		PanelManager::instance().setToolMode( L"TerrainMesh" );
	}
	if (keyEvent.key() == KeyCode::KEY_7 && keyEvent.isKeyDown() && !keyEvent.modifiers())
	{
		PanelManager::instance().setToolMode( L"Project" );
	}
	if (keyEvent.key() == KeyCode::KEY_8 && keyEvent.isKeyDown() && !keyEvent.modifiers())
	{
		PanelManager::instance().setToolMode( L"TerrainRuler" );
	}
	if (keyEvent.key() == KeyCode::KEY_9 && keyEvent.isKeyDown() && !keyEvent.modifiers())
	{
		PanelManager::instance().setToolMode( L"Fences" );
	}

    return false;
}

void HeightModule::gotoWorldPos(const KeyEvent & /*keyEvent*/)
{
	BW_GUARD;

	// Get where we click in grid coords
	Vector2 gridPos = currentGridPos();
	Vector3 world = gridPosToWorldPos(gridPos);
    
    // Find the height at this point
    float height = TerrainUtils::heightAtPos(world.x, world.z, true);
    world.y = height + Options::getOptionInt( "graphics/farclip", 500 )/10.0f;

	// Set the view matrix to the new world coords
	Matrix view = WorldEditorCamera::instance().currentCamera().view();
	view.setTranslate(world);
	view.preRotateX(DEG_TO_RAD(30.f));
	view.invert();
	WorldEditorCamera::instance().currentCamera().view(view);

    if (Options::getOptionInt("camera/ortho") == WorldEditorCamera::CT_Orthographic)
    {
        WorldEditorCamera::instance().changeToCamera(WorldEditorCamera::CT_MouseLook   );
        WorldEditorCamera::instance().changeToCamera(WorldEditorCamera::CT_Orthographic);
    }

    // Now, change back to object mode
    PyObject* pModule = PyImport_ImportModule("WorldEditorDirector");
    if (pModule != NULL)
    {
        PyObject* pScriptObject = PyObject_GetAttrString( pModule, "bd" );

        if (pScriptObject != NULL)
        {
            Script::call
            (
                PyObject_GetAttrString(pScriptObject, "changeToMode"),
                Py_BuildValue( "(s)", "Object" ),
                "HeightModule"
            );                
            Py_DECREF(pScriptObject);
        }
        PanelManager::instance().setDefaultToolMode();
        Py_DECREF(pModule);
    }
}


void HeightModule::startTerrainEdit(const KeyEvent & /*keyevent*/)
{
	BW_GUARD;

    normaliseTerrainRect();
    POINT cursorPt = WorldManager::instance().currentCursorPosition();
    terrainDownPt_ = gridPos(cursorPt);
    terrainDir_    = pointToDir(cursorPt);
	terrainTLDown_ = (mode_ == IMPORT_MASK) ? maskTopLeft_     : terrainTopLeft_   ;
    terrainBRDown_ = (mode_ == IMPORT_MASK) ? maskBottomRight_ : terrainBottomRight_;
}


void HeightModule::startGetHeight(const KeyEvent & /*keyevent*/)
{
	BW_GUARD;

    gettingHeight_   = true;
    POINT cursorPt   = WorldManager::instance().currentCursorPosition();
    heightDownPt_    = gridPos(cursorPt);
    heightCurrentPt_ = heightDownPt_;
    heightQuery();
}


bool HeightModule::handleMouseEvent(const MouseEvent &mouseEvent)
{
	BW_GUARD;

	if (!hasStarted_)
	{
		return false;
	}

    bool handled = false;

    updateCursor();

    // Adjust zoom:
    if (mouseEvent.dz() != 0)
    {
        handleZoom(mouseEvent);
        handled = true;
    }

    // Pan around:
    if 
    (
        (mouseEvent.dx() != 0 || mouseEvent.dy() != 0) 
        && 
        InputDevices::isKeyDown(KeyCode::KEY_RIGHTMOUSE)
    )
    {
        handlePan(mouseEvent);
        handled = true;
    }

    // Editing the terrain's extents:
    if 
    (
        (mouseEvent.dx() != 0 || mouseEvent.dy() != 0) 
        && 
        InputDevices::isKeyDown( KeyCode::KEY_LEFTMOUSE ) 
        && 
        mouseDownOn3DWindow_
        && 
        !gettingHeight_
    )
    {
        handleTerrainEdit(mouseEvent);
        handled = true;
    }

    // Getting height:
    if
    (
        (mouseEvent.dx() != 0 || mouseEvent.dy() != 0)
        && 
        mouseDownOn3DWindow_
        && 
        gettingHeight_
    )
    {
        handleGetHeight(mouseEvent);
    }

    return handled;
}


void HeightModule::handleZoom(const MouseEvent &mouseEvent)
{
    viewPosition_.z += (mouseEvent.dz() > 0) ? -minViewZ_/25.0f : minViewZ_/25.0f;

    if (viewPosition_.z > -1.0f)
        viewPosition_.z = -1.0f;

    if (viewPosition_.z < minViewZ_)
        viewPosition_.z = minViewZ_;
}


void HeightModule::handlePan(const MouseEvent &mouseEvent)
{
    viewPosition_.x -= scaleX_*mouseEvent.dx();
    viewPosition_.y += scaleY_*mouseEvent.dy();
}


void HeightModule::handleTerrainEdit(const MouseEvent &/*mouseEvent*/)
{
	BW_GUARD;

	Vector2 &tl = (mode_ == IMPORT_MASK) ? maskTopLeft_     : terrainTopLeft_;
	Vector2 &br = (mode_ == IMPORT_MASK) ? maskBottomRight_ : terrainBottomRight_;

    Vector2 curPos = currentGridPos();
    bool redraw = true;
    switch (terrainDir_)
    {
    case TERRAIN_MISS:
        redraw = false;
        break;
    case TERRAIN_MIDDLE:
        {
        Vector2 delta = curPos - terrainDownPt_;
        tl = terrainTLDown_ + delta;
        br = terrainBRDown_ + delta;		
        Vector2 snapTL(tl);
        Vector2 snapBR(br);
        snapTL.x = snapTerrainCoord(snapTL.x);
        snapTL.y = snapTerrainCoord(snapTL.y);
        snapBR.x = snapTerrainCoord(snapBR.x);
        snapBR.y = snapTerrainCoord(snapBR.y);
		// this handles snap to grid correctly but not edges
        if 
        (
            fabs(snapTL.x - tl.x) 
            <= 
            fabs(snapBR.x - br.x)
        )
        {
            float diff = br.x - tl.x;
            tl.x = snapTL.x;
            br.x = tl.x + diff;
        }
        else
        {
            float diff = br.x - tl.x;            
            br.x = snapBR.x;
            tl.x = br.x - diff;
        }
        if 
        (
            fabs(snapTL.y - tl.y) 
            <= 
            fabs(snapBR.y - br.y)
        )
        {
            float diff = br.y - tl.y;
            tl.y = snapTL.y;
            br.y = tl.y + diff;
        }
        else
        {
            float diff = br.y - tl.y;            
            br.y = snapBR.y;
            tl.y = br.y - diff;
        }
		// now clamp to edge
		if ( tl.x < 0 )
		{
			br.x = br.x - tl.x;
			tl.x = 0.f;
		}
		else if ( br.x > gridWidth_ )
		{
			tl.x = gridWidth_ - ( br.x - tl.x );
			br.x = (float)gridWidth_;
		}
		if ( tl.y > gridHeight_ )
		{
			br.y = gridHeight_ + br.y - tl.y;
			tl.y = (float)gridHeight_;
		}
		else if ( br.y < 0 )
		{
			tl.y = -( br.y - tl.y );
			br.y = 0.f;
		}
        }
        break;
    case TERRAIN_NORTH:
        tl.y = snapTerrainCoord(curPos.y);
        break;
    case TERRAIN_NORTH_EAST:
        br.x = snapTerrainCoord(curPos.x);
        tl.y = snapTerrainCoord(curPos.y);
        break;
    case TERRAIN_EAST:
        br.x = snapTerrainCoord(curPos.x);
        break;
    case TERRAIN_SOUTH_EAST:
        br.x = snapTerrainCoord(curPos.x);
        br.y = snapTerrainCoord(curPos.y);
        break;
    case TERRAIN_SOUTH:
        br.y = snapTerrainCoord(curPos.y);
        break;
    case TERRAIN_SOUTH_WEST:
        tl.x = snapTerrainCoord(curPos.x);
        br.y = snapTerrainCoord(curPos.y);
        break;
    case TERRAIN_WEST:
        tl.x = snapTerrainCoord(curPos.x);
        break;
    case TERRAIN_NORTH_WEST:
        tl.x = snapTerrainCoord(curPos.x);
        tl.y = snapTerrainCoord(curPos.y);
        break;
    }
	updateWorldRect();
    if (redraw)
        redrawImportedTerrain();
}


void HeightModule::handleGetHeight(const MouseEvent & /*mouseEvent*/)
{
	BW_GUARD;

    heightQuery();
}


void HeightModule::projectMapAlpha(float a)
{
    spaceAlpha_ = a;
}


float HeightModule::projectMapAlpha()
{
    return spaceAlpha_;
}


HeightModule::Mode HeightModule::mode() const
{
    return mode_;
}


void HeightModule::mode(Mode m)
{
	if (m != mode_)
	{
		BW_GUARD;

		mode_ = m;
		updateWorldRect();
		redrawImportedTerrain();
		updateMinMax();
	}
}


bool HeightModule::hasImportData() const
{
	BW_GUARD;

	return importImage() != NULL && !importImage()->isEmpty();
}


/*static*/ ImportImagePtr HeightModule::elevationImage()
{
	return s_elevationData_;
}


/*static*/ ImportImagePtr HeightModule::maskImage()
{
	return s_maskData_;
}


/*static*/ Vector2 HeightModule::topLeft(bool mask)
{
	if (mask)
		return s_maskTopLeft_;
	else
		return s_terrainTopLeft_;
}


/*static*/ Vector2 HeightModule::bottomRight(bool mask)
{
	if (mask)
		return s_maskBottomRight_;
	else
		return s_terrainBottomRight_;
}


void HeightModule::importData
(
    ImportImagePtr	    			image,
    float                           *left	/* = NULL */,
    float                           *top	/* = NULL */,
    float                           *right	/* = NULL */,
    float                           *bottom	/* = NULL */
)
{
	BW_GUARD;

	Vector2 &tl = (mode_ == IMPORT_MASK) ? maskTopLeft_     : terrainTopLeft_;
	Vector2 &br = (mode_ == IMPORT_MASK) ? maskBottomRight_ : terrainBottomRight_;

    if (left != NULL)
        tl.x = *left; 
    if (top != NULL)
        tl.y = *top;
    if (right != NULL)
        br.x = *right;
    if (bottom != NULL)
        br.y = *bottom;
    normaliseTerrainRect();	

    // If not inside the space's area then do some hueristics:
    // (note assymetry between x and y below is because +y is up).
    if 
    (
        tl.x < 0 || br.x > gridWidth_
        ||
        tl.y > gridHeight_ || br.y < 0     
    )
    {
        // If the imported area still fits inside the space, then import to 
        // (0,gridHeight):
        float w = std::abs(br.x - tl.x);
        float h = std::abs(br.y - tl.y);
        if (w <= gridWidth_ && h <= gridHeight_)
        {
            WARNING_MSG( "Imported terrain outside space, it was moved to the top-left.\n" );
            tl = Vector2(0.0f, static_cast<float>(gridHeight_));
            br = Vector2(w, static_cast<float>(gridHeight_) - h);
        }
        // The imported area doesn't fit, import into the entire space:
        else
        {
            WARNING_MSG( "Imported terrain does not fit into space, it was rescaled to fit.\n" );
            tl = Vector2(0.0f, static_cast<float>(gridHeight_));
            br = Vector2(static_cast<float>(gridWidth_), 0.0f);
        }        
    }

	updateWorldRect();

	if (mode_ == IMPORT_MASK)
		s_maskData_ = image;
	else
		s_elevationData_ = image;
    if (importImage() != NULL)
    {
		// min/max heights are calculated correctly.
        redrawImportedTerrain();
        HeightMap::instance().updateMinMax();
    }
    else
    {
        HeightMap::instance().clearImpTerrain();
    }
}


void HeightModule::rotateImportData(bool clockwise)
{
	BW_GUARD;

    if (importImage() != NULL)
    {
        importImage()->rotate(clockwise);
        redrawImportedTerrain();
    }
}


void HeightModule::flipImportData(FlipDir dir)
{
	BW_GUARD;

    if (importImage() != NULL)
    {
        switch (dir)
        {
        case FLIP_X:
            importImage()->flip(true);  // true = flip x
            break;
        case FLIP_Y:
            importImage()->flip(false); // false = flip y
            break;
        case FLIP_HEIGHT:
            importImage()->flipHeight();
            break;
        }
        redrawImportedTerrain();
    }
}


void HeightModule::terrainImportOptions
(
    ElevationBlit::Mode mode,
    float               minV, 
    float               maxV,
    float               strength,
	bool				absolute
)
{
	BW_GUARD;

    impMode_			= mode;
    impMinV_			= minV;
    impMaxV_			= maxV;
    impStrength_		= strength;
	absoluteHeights_	= absolute;
    if (importImage() != NULL)
    {
        redrawImportedTerrain();
    }
}


void HeightModule::textureImportOptions
(
	BW::string			const &texture,
	Vector4				const &uProjection,
	Vector4				const &vProjection,
	float				strength
)
{
	BW_GUARD;

	texture_		= texture;
	uProj_			= uProjection;
	vProj_			= vProjection;
	s_texStrength_	= strength;
    if (importImage() != NULL)
    {
		redrawImportedTerrain();
	}
}


void HeightModule::textureImportStrength(float strength)
{
	BW_GUARD;

	if (s_texStrength_ == strength)
	{
		return;
	}
	s_texStrength_ = strength;
    if (importImage() != NULL)
    {
		redrawImportedTerrain();
	}
}


/*static*/ float HeightModule::textureImportStrength()
{
	return s_texStrength_;
}


size_t HeightModule::terrainUndoSize() const
{
	BW_GUARD;

    return 
        ElevationBlit::importUndoSize
        (
            terrainTopLeft_    .x, terrainTopLeft_    .y,
            terrainBottomRight_.x, terrainBottomRight_.y
        );
}


bool HeightModule::doTerrainImport(bool doUndo, bool showProgress, bool forceToMem)
{
	BW_GUARD;

    if (importImage() != NULL)
    {
        bool result = true;
        if (doUndo)
        {
            result =
                ElevationBlit::saveUndoForImport
                (
                    terrainTopLeft_    .x, terrainTopLeft_    .y,
                    terrainBottomRight_.x, terrainBottomRight_.y,
                    LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/HEIGHT/HEIGHT_MODULE/IMPORT_TERRAIN"),
                    showProgress
                );
        }
        if (result)
        {
			if (absoluteHeights_)
				importImage()->setScale(0.0f, 1.0f);
			result = 
				ElevationBlit::import
				(
					*importImage(),
					terrainTopLeft_    .x, terrainTopLeft_    .y,
					terrainBottomRight_.x, terrainBottomRight_.y,
					impMode_,
					impMinV_,
					impMaxV_,
					impStrength_,
					showProgress,
					forceToMem
				);
            HeightMap::instance().invalidateImportedTerrain();
        }
        return result;
    }
    else
    {
        return false;
    }
}


bool HeightModule::doTerrainExport
(
	char				const *filename, 
	bool				showProgress
)
{
	BW_GUARD;

	float minHeight, maxHeight;
	if (absoluteHeights_)
	{
		minHeight = impMinV_;
		maxHeight = impMaxV_;
	}
	else
	{
		ElevationBlit::heightRange
		(
			terrainTopLeft_    .x, terrainTopLeft_    .y,
			terrainBottomRight_.x, terrainBottomRight_.y,
			minHeight, maxHeight,
			showProgress
		);
	}
    ImportImage image;
	bool heightsOk;
    ElevationBlit::exportTo
    (
        image,
        terrainTopLeft_    .x, terrainTopLeft_    .y,
        terrainBottomRight_.x, terrainBottomRight_.y,
		minHeight, maxHeight,
		heightsOk,
        showProgress
    );
	if (!heightsOk)
		ERROR_MSG("Some heights in export were clipped");
    return 
        image.save
        (
            filename,
            &terrainTopLeft_    .x, &terrainTopLeft_    .y,
            &terrainBottomRight_.x, &terrainBottomRight_.y,
			&absoluteHeights_,
			&minHeight, &maxHeight
        );
}


void HeightModule::updateMinMax()
{
	BW_GUARD;

    HeightMap::instance().updateMinMax();
}


/*static*/ void HeightModule::ensureHeightMapCalculated()
{
	BW_GUARD;

    HeightModule *heightModule = HeightModule::currentInstance();
    bool haveHeightModule = heightModule != NULL;
    if (!haveHeightModule)
    {
        heightModule = new HeightModule();
        heightModule->onStart();
    }
    HeightMap::instance().makeValid();
    if (!haveHeightModule)
    {
        heightModule->onStop();
        bw_safe_delete(heightModule);
    }    
}


/*static*/ void HeightModule::doNotSaveHeightMap()
{
	BW_GUARD;

    HeightModule *heightModule = HeightModule::currentInstance();
    bool haveHeightModule = heightModule != NULL;
    if (!haveHeightModule)
    {
        heightModule = new HeightModule();
        heightModule->onQuickStart();
    }
    HeightMap::instance().doNotSaveHeightMap();
    if (!haveHeightModule)
    {
        bw_safe_delete(heightModule);
    }    
}


/*static*/ HeightModule* HeightModule::currentInstance()
{
    return s_currentInstance_;
}


/*static*/ bool HeightModule::hasStarted()
{
	return s_currentInstance_ != NULL && s_currentInstance_->hasStarted_;
}


/**
 *  This function starts the HeightModule with enough information to
 *  access the HeightMap instance.
 */
void HeightModule::onQuickStart()
{
	BW_GUARD;

    BW::string space = WorldManager::instance().getCurrentSpace();
	float gridSize;
    int minX, minY, maxX, maxY;
    TerrainUtils::terrainSize(space, gridSize, minX, minY, maxX, maxY);
	minX_ = minX;
	minY_ = minY;
    gridWidth_          = maxX - minX + 1;
    gridHeight_         = maxY - minY + 1;
    localToWorld_       = GridCoord( minX, minY );

    HeightMap::instance().spaceInformation
    (
        space, 
        localToWorld_.x, 
        localToWorld_.y,
        gridWidth_, 
        gridHeight_,
        false
    );
}


void HeightModule::writeStatus()
{
	BW_GUARD;

    WorldManager::instance().setStatusMessage(1, "");
    WorldManager::instance().setStatusMessage(2, "");
    WorldManager::instance().setStatusMessage(4, "");

	//Panel 0 - memory load
	WorldManager::instance().setStatusMessage( 0,
		LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/HEIGHT/HEIGHT_MODULE/MEMORY_LOAD",
		(int)Memory::memoryLoad() ) );

    // Panel 3 - locator position and height:
    Vector2 gridPos = currentGridPos();
    Vector3 world = gridPosToWorldPos(gridPos);
    GeometryMapping *dirMap = WorldManager::instance().geometryMapping();
    BW::stringstream output;
    output << dirMap->outsideChunkIdentifier(world);
    float height = HeightMap::instance().heightAtGridPos(gridPos);
    if (height != std::numeric_limits<float>::max())
        output << ' ' << height;
    output << '\0';
    BW::string panelText = output.str();
    WorldManager::instance().setStatusMessage(3, panelText);

    // Panel 5 - number of chunks loaded:
	BW::string dirtyChunks = WorldManager::instance().describeDirtyChunkStatus();
	if (!dirtyChunks.empty())
	{
		WorldManager::instance().setStatusMessage(5,
			LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/HEIGHT/HEIGHT_MODULE/CHUNK_LOADED_WITH_DIRTY",
			EditorChunkCache::chunks_.size(), dirtyChunks ) );
	}
	else
	{
		WorldManager::instance().setStatusMessage(5,
			LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/HEIGHT/HEIGHT_MODULE/CHUNK_LOADED",
			EditorChunkCache::chunks_.size() ) );
	}

    MainFrame *mainFrame = reinterpret_cast<MainFrame *>(AfxGetMainWnd());
    mainFrame->frameUpdate();
}


Vector2 HeightModule::gridPos(POINT pt) const
{
	BW_GUARD;

    Vector3 cursorPos = Moo::rc().camera().nearPlanePoint(
            (float(pt.x) / Moo::rc().screenWidth()) * 2.0f - 1.0f,
            1.0f - (float(pt.y) / Moo::rc().screenHeight()) * 2.0f );

    Matrix view;
    view.setTranslate( viewPosition_ );

    Vector3 worldRay = view.applyVector( cursorPos );
    worldRay.normalise();

    PlaneEq gridPlane( Vector3(0.0f, 0.0f, 1.0f), .0001f );

    Vector3 gridPos = gridPlane.intersectRay( viewPosition_, worldRay );

    return Vector2( gridPos.x, gridPos.y );
}


Vector2 HeightModule::currentGridPos() const
{
	BW_GUARD;

    POINT pt = WorldManager::instance().currentCursorPosition();
    return gridPos(pt);
}


Vector3 HeightModule::gridPosToWorldPos(Vector2 const &gridPos) const
{
    Vector2 w = 
        (gridPos + Vector2(float(localToWorld_.x), float(localToWorld_.y)))*gridSize_;

    return Vector3(w.x, 0, w.y);
}


Vector2 HeightModule::worldPosToGridPos(Vector3 const &pos) const
{
    return 
        Vector2
        (
            pos.x/gridSize_ - float(localToWorld_.x),
            pos.z/gridSize_ - float(localToWorld_.y)
        );
}


HeightModule::TerrainDir HeightModule::pointToDir(POINT pt) const
{
	BW_GUARD;

	Vector2 tl = (mode_ == IMPORT_MASK) ? maskTopLeft_     : terrainTopLeft_    ;
	Vector2 br = (mode_ == IMPORT_MASK) ? maskBottomRight_ : terrainBottomRight_;


    // Get the position in the world:
    Vector2 worldPos = gridPos(pt);

    // Normalised coordinates of the terrain selection:
    float left   = std::min(tl.x, br.x);
    float right  = std::max(tl.x, br.x);
    float top    = std::max(tl.y, br.y);
    float bottom = std::min(tl.y, br.y);

    // See if the point is close to any of the corners:
    if
    (
        fabsf(left - worldPos.x) < 3*scaleX_ &&
        fabsf(top  - worldPos.y) < 3*scaleY_
    )
    {
        return TERRAIN_NORTH_WEST;
    }
    if
    (
        fabsf(right - worldPos.x) < 3*scaleX_ &&
        fabsf(top   - worldPos.y) < 3*scaleY_
    )
    {
        return TERRAIN_NORTH_EAST;
    }
    if
    (
        fabsf(right  - worldPos.x) < 3*scaleX_ &&
        fabsf(bottom - worldPos.y) < 3*scaleY_
    )
    {
        return TERRAIN_SOUTH_EAST;
    }
    if
    (
        fabsf(left   - worldPos.x) < 3*scaleX_ &&
        fabsf(bottom - worldPos.y) < 3*scaleY_
    )
    {
        return TERRAIN_SOUTH_WEST;
    }

    // See if the point is close to any of the edges:
    if 
    (
        left <= worldPos.x && worldPos.x <= right 
        &&
        fabsf(top  - worldPos.y) < 3*scaleY_
    )
    {
        return TERRAIN_NORTH;
    }
    if 
    (
        bottom <= worldPos.y && worldPos.y <= top
        &&
        fabsf(right - worldPos.x) < 3*scaleX_
    )
    {
        return TERRAIN_EAST;
    }
    if 
    (
        left <= worldPos.x && worldPos.x <= right 
        &&
        fabsf(bottom  - worldPos.y) < 3*scaleY_
    )
    {
        return TERRAIN_SOUTH;
    }
    if 
    (
        bottom <= worldPos.y && worldPos.y <= top
        &&
        fabsf(left - worldPos.x) < 3*scaleX_
    )
    {
        return TERRAIN_WEST;
    }

    // See if the point is inside the selected terrain:
    if
    (
        left <= worldPos.x && worldPos.x <= right
        &&
        bottom <= worldPos.y && worldPos.y <= top
    )
    {
        return TERRAIN_MIDDLE;
    }

    // Must have missed:
    return TERRAIN_MISS;
}


void HeightModule::normaliseTerrainRect()
{
    if (terrainTopLeft_.x > terrainBottomRight_.x)
        std::swap(terrainTopLeft_.x, terrainBottomRight_.x);
    if (terrainTopLeft_.y < terrainBottomRight_.y)
        std::swap(terrainTopLeft_.y, terrainBottomRight_.y);
    if (maskTopLeft_.x > maskBottomRight_.x)
        std::swap(maskTopLeft_.x, maskBottomRight_.x);
    if (maskTopLeft_.y < maskBottomRight_.y)
        std::swap(maskTopLeft_.y, maskBottomRight_.y);
}


float HeightModule::snapTerrainCoord(float v) const
{
	BW_GUARD;

    if (shouldGridSnap())
    {
        if (v >= 0.0f)
            return static_cast<float>(static_cast<int>((v + 0.5f)));
        else
            return static_cast<float>(static_cast<int>((v - 0.5f)));
    }
    else
    {
        return v;
    }
}


void 
HeightModule::drawRect
(
    Vector2     const &ul, 
    Vector2     const &br, 
    uint32      colour, 
    int         ht
)
{
	BW_GUARD;

    CustomMesh<Moo::VertexXYZL> leftEdge  ;
    CustomMesh<Moo::VertexXYZL> rightEdge ;
    CustomMesh<Moo::VertexXYZL> topEdge   ;
    CustomMesh<Moo::VertexXYZL> bottomEdge;

    quad
    (
        leftEdge,
        Vector2(ul.x - ht*scaleX_, ul.y - ht*scaleY_),
        Vector2(ul.x + ht*scaleX_, br.y - ht*scaleY_),
        colour
    );
    quad
    (
        rightEdge,
        Vector2(br.x - ht*scaleX_, ul.y - ht*scaleY_),
        Vector2(br.x + ht*scaleX_, br.y - ht*scaleY_),
        colour
    );
    quad
    (
        topEdge,
        Vector2(ul.x - ht*scaleX_, ul.y - ht*scaleY_),
        Vector2(br.x + ht*scaleX_, ul.y + ht*scaleY_),
        colour
    );
    quad
    (
        bottomEdge,
        Vector2(ul.x - ht*scaleX_, br.y - ht*scaleY_),
        Vector2(br.x + ht*scaleX_, br.y + ht*scaleY_),
        colour
    );

    leftEdge   .drawEffect();
    rightEdge  .drawEffect();
    topEdge    .drawEffect();
    bottomEdge .drawEffect();
}


void 
HeightModule::drawRect
(
    Vector2     const &ul, 
    Vector2     const &br, 
    uint32      colour
)
{
	BW_GUARD;

    CustomMesh<Moo::VertexXYZL> mesh;
    quad(mesh, ul, br, colour);
    mesh.drawEffect();
}


void 
HeightModule::drawHandle
( 
    Vector2     const &v, 
    uint32      colour, 
    int         hx, 
    int         hy 
) const
{
	BW_GUARD;

    CustomMesh<Moo::VertexXYZL> handleMesh;
    quad
    (
        handleMesh,
        Vector3(v.x - hx*scaleX_, v.y - hy*scaleY_, 0.0f),
        (2*hx + 1)*scaleX_,
        (2*hy + 1)*scaleX_,
        colour
    );
    handleMesh.drawEffect();
}


void HeightModule::drawVLine(float x, uint32 colour, float hx) const
{
	BW_GUARD;

    CustomMesh<Moo::VertexXYZL> lineMesh;
    vertQuad(lineMesh, x, hx*scaleX_, colour);
    lineMesh.drawEffect();
}


void HeightModule::drawHLine(float y, uint32 colour, float hy) const
{
	BW_GUARD;

    CustomMesh<Moo::VertexXYZL> lineMesh;
    horzQuad(lineMesh, y, hy*scaleY_, colour);
    lineMesh.drawEffect();
}


void HeightModule::redrawImportedTerrain()
{
	BW_GUARD;

    if (importImage() != NULL)
    {
		if (mode_ == IMPORT_TERRAIN)
		{
			float minV, maxV;
			if (absoluteHeights_)
			{
				importImage()->getScale( minV, maxV );
				importImage()->setScale( 0.0f, 1.0f );
			}
			HeightMap::instance().drawImportedTerrain
			(
				terrainTopLeft_    .x, terrainTopLeft_    .y ,
				terrainBottomRight_.x, terrainBottomRight_.y,
				impMode_,
				impMinV_,
				impMaxV_,
				impStrength_,
				*importImage()
			);
			if (absoluteHeights_)
			{
				importImage()->setScale( minV, maxV );
			}
		}
		else if (mode_ == IMPORT_MASK)
		{
			float minV, maxV;
			importImage()->getScale( minV, maxV );
			importImage()->setScale( 0.0f, 1.0f );
			HeightMap::instance().drawImportedTerrain
			(
				maskTopLeft_    .x, maskTopLeft_    .y,
				maskBottomRight_.x, maskBottomRight_.y,
				ElevationBlit::REPLACE,
				// We set impMinV_/impMaxV_ to (0,255) so the resolution is big
				// enough, because for terrain mask there is not min/max height
				// changeable which does in height map import.
				0,
				255,
				s_texStrength_,
				*importImage()
			);
			importImage()->setScale( minV, maxV );
		}
    }
}


void HeightModule::heightQuery()
{
	BW_GUARD;

    HeightMap::instance().undrawImportedTerrain();
    POINT cursorPt   = WorldManager::instance().currentCursorPosition();
    heightCurrentPt_ = gridPos(cursorPt);
    float minv, maxv;
    HeightMap::instance().heightRange
    (
        heightDownPt_,
        heightCurrentPt_,
        minv,
        maxv
    );
    if (!shouldGetMaxHeight()) 
        maxv = -std::numeric_limits<float>::max();
    if (!shouldGetMinHeight()) 
        minv = +std::numeric_limits<float>::max();
    PageTerrainImport *pti = PageTerrainImport::instance();
    if (pti != NULL)
        pti->setQueryHeightRange(minv, maxv);
}


void HeightModule::updateCursor()
{
	BW_GUARD;

    // Is the user sampling:
    if 
    (
		(
			mode_ == IMPORT_TERRAIN 
			|| 
			mode_ == EXPORT_TERRAIN
		)
        && 
        (
            shouldGetMaxHeight() 
            || 
            shouldGetMinHeight()
        )
    )
    {
        HCURSOR cursor = ::AfxGetApp()->LoadCursor(MAKEINTRESOURCE(IDC_HEIGHTPICKER));
        SetCursor(cursor);
    }
    // Is the mouse over the selection cursor?
    else
    {    
        TerrainDir dir = TERRAIN_MISS;
        if (InputDevices::isKeyDown(KeyCode::KEY_LEFTMOUSE))
            dir = terrainDir_;
        else
            dir = pointToDir(WorldManager::instance().currentCursorPosition());
        LPTSTR cursorID = NULL;
        switch (dir)
        {    
        case TERRAIN_MIDDLE:
            cursorID = IDC_SIZEALL;
            break;
        case TERRAIN_NORTH:         // fall through
        case TERRAIN_SOUTH:
            cursorID = IDC_SIZENS;
            break;
        case TERRAIN_EAST:          // fall through
        case TERRAIN_WEST:
            cursorID = IDC_SIZEWE;
            break;
        case TERRAIN_NORTH_EAST:    // fall through
        case TERRAIN_SOUTH_WEST:
            cursorID = IDC_SIZENESW;
            break;    
        case TERRAIN_SOUTH_EAST:    // fall through
        case TERRAIN_NORTH_WEST:
            cursorID = IDC_SIZENWSE;
            break;   
        case TERRAIN_MISS:          // fall through
        default:
            cursorID = IDC_ARROW;
            break;
        }
        SetCursor(::LoadCursor(NULL, cursorID));
    }
}


void HeightModule::validateWorldRect()
{
	maskTopLeft_.x = max(0.f, min(maskTopLeft_.x, (float)gridWidth_ ));
	maskTopLeft_.y = max(0.f, min(maskTopLeft_.y, (float)gridHeight_ ));
	maskBottomRight_.x = max(0.f, min(maskBottomRight_.x, (float)gridWidth_ ));
	maskBottomRight_.y = max(0.f, min(maskBottomRight_.y, (float)gridHeight_ ));
	terrainTopLeft_.x = max(0.f, min(terrainTopLeft_.x, (float)gridWidth_ ));
	terrainTopLeft_.y = max(0.f, min(terrainTopLeft_.y, (float)gridHeight_ ));
	terrainBottomRight_.x = max(0.f, min(terrainBottomRight_.x, (float)gridWidth_ ));
	terrainBottomRight_.y = max(0.f, min(terrainBottomRight_.y, (float)gridHeight_ ));
}


void HeightModule::updateWorldRect()
{
	BW_GUARD;

	validateWorldRect();
	if (mode_ == IMPORT_MASK)
	{
		Vector3 tl = gridPosToWorldPos(maskTopLeft_);
		Vector3 br = gridPosToWorldPos(maskBottomRight_);

		s_maskTopLeft_			= Vector2(tl.x, tl.z);
		s_maskBottomRight_		= Vector2(br.x, br.z);
	}
	else
	{
		Vector3 tl = gridPosToWorldPos(terrainTopLeft_);
		Vector3 br = gridPosToWorldPos(terrainBottomRight_);

		s_terrainTopLeft_		= Vector2(tl.x, tl.z);
		s_terrainBottomRight_	= Vector2(br.x, br.z);
	}
}


ImportImagePtr HeightModule::importImage() const
{
	if (mode_ == IMPORT_MASK)
		return s_maskData_;
	else
		return s_elevationData_;
}
BW_END_NAMESPACE

