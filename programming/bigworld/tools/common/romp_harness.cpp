//---------------------------------------------------------------------------
#include "pch.hpp"

#include "cstdmf/debug.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/string_provider.hpp"
#include "romp_harness.hpp"
#include "romp/fog_controller.hpp"
#include "romp/lens_effect_manager.hpp"
#include "romp/engine_statistics.hpp"
#include "romp/console_manager.hpp"
#include "romp/resource_manager_stats.hpp"
#include "romp/time_of_day.hpp"
#include "romp/weather.hpp"
#include "romp/distortion.hpp"
#include "moo/geometrics.hpp"
#include "romp/enviro_minder.hpp"
#include "post_processing/manager.hpp"
#include "moo/render_context.hpp"
#include "moo/moo_math.hpp"
#include "moo/draw_context.hpp"
#include "romp/water.hpp"
#include "romp/sky_gradient_dome.hpp"
#include "chunk/chunk_manager.hpp"
#include "chunk/chunk_space.hpp"
#include "chunk/chunk.hpp"
#include "gizmo/tool_manager.hpp"
#include "appmgr/options.hpp"
#include "romp/histogram_provider.hpp"
#include "terrain/base_terrain_renderer.hpp"

DECLARE_DEBUG_COMPONENT2( "WorldEditor", 2 )

BW_BEGIN_NAMESPACE

PY_TYPEOBJECT( RompHarness )

PY_BEGIN_METHODS( RompHarness )
	PY_METHOD( setTime )
	PY_METHOD( setSecondsPerHour )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( RompHarness )
	/*~ attribute RompHarness.fogEnable
	 *	@components{ tools }
	 *
	 *	The fogEnable attribute determines whether or not fog is enabled.
	 */
	PY_ATTRIBUTE( fogEnable )
PY_END_ATTRIBUTES()

RompHarness::RompHarness( PyTypeObject * pType ) :
	PyObjectPlus( pType ),
    dTime_( 0.033f ),
    inited_( false ),
	distortion_( NULL )
{
	BW_GUARD;

	waterMovement_[0] = Vector3(0,0,0);
	waterMovement_[1] = Vector3(0,0,0);

	if ( ChunkManager::instance().cameraSpace() )
	{
		MF_WATCH_REF( LocaliseACP(L"COMMON/ROMP_HARNESS/TIME_OF_DAY").c_str(), *enviroMinder().timeOfDay(),
			&TimeOfDay::getTimeOfDayAsString, &TimeOfDay::setTimeOfDayAsString );
		MF_WATCH( LocaliseACP(L"COMMON/ROMP_HARNESS/SECS_PER_HOUR").c_str(), *enviroMinder().timeOfDay(),
			MF_ACCESSORS( float, TimeOfDay, secondsPerGameHour ) );
	}
}

RompHarness::~RompHarness()
{
	BW_GUARD;

	if (inited_)
	{
		bw_safe_delete(distortion_);

		Waters::instance().fini();

		EnviroMinder::fini();

		ResourceManagerStats::fini();
	}
}


bool
RompHarness::init()
{
	BW_GUARD;

	if (!ChunkManager::instance().cameraSpace())
	{
		return false;
	}

    inited_ = true;

	EnviroMinder::init();

	setSecondsPerHour( 0.f );
	DataSectionPtr pWatcherSection = Options::pRoot()->openSection( "romp/watcherValues" );
	if (pWatcherSection)
	{
		pWatcherSection->setWatcherValues();
	}

	if (Distortion::isSupported())
	{
		// Initialised on the first draw
		distortion_ = new Distortion;
	}
	else
	{
		INFO_MSG( "Distortion is not supported on this hardware\n" );
	}

	Waters::instance().init();

	return true;
}

void RompHarness::changeSpace()
{
	BW_GUARD;

	setSecondsPerHour( 0.f );
}

void RompHarness::initWater( DataSectionPtr pProject )
{
}


void RompHarness::setTime( float t )
{
	BW_GUARD;

	enviroMinder().timeOfDay()->gameTime( t );
}


void RompHarness::setSecondsPerHour( float sph )
{
	BW_GUARD;

	enviroMinder().timeOfDay()->secondsPerGameHour( sph );
}


void RompHarness::update( float dTime, bool globalWeather )
{
	BW_GUARD;

	dTime_ = dTime;

	if ( inited_ )
    {
		this->fogEnable( !!Options::getOptionInt( "render/environment/drawFog", 0 ));

		Chunk * pCC = ChunkManager::instance().cameraChunk();
		bool outside = (pCC == NULL || pCC->isOutsideChunk());
		enviroMinder().tick( dTime_, outside );
        FogController::instance().tick();        

		if (distortion_)
			distortion_->tick(dTime);
    }

    //Intersect tool with water
	this->disturbWater();
}

void RompHarness::drawPreSceneStuff( bool sparkleCheck /* = false */, bool renderEnvironment /* = true */ )
{
	BW_GUARD;

	//TODO : this is specific to heat shimmer, which may or may not be included
	//via the post processing chain.  Make generic.
	Moo::rc().setRenderState( 
		D3DRS_COLORWRITEENABLE, 
			D3DCOLORWRITEENABLE_RED | 
			D3DCOLORWRITEENABLE_GREEN | 
			D3DCOLORWRITEENABLE_BLUE );

	Moo::Material::shimmerMaterials = true;
	//end TODO

	PostProcessing::Manager::instance().tick(dTime_);

	uint32 flags;
	flags = 0;
	flags |= ( Options::getOptionInt(
		"render/environment/drawSunAndMoon", 0 ) ? EnviroMinder::DRAW_SUN_AND_MOON : 0 );
	flags |= ( Options::getOptionInt(
		"render/environment/drawSky", 0 ) ? EnviroMinder::DRAW_SKY_GRADIENT : 0 );
	flags |= ( Options::getOptionInt(
		"render/environment/drawSkyBoxes", 0 ) ? EnviroMinder::DRAW_SKY_BOXES : 0 );
	flags |= ( Options::getOptionInt(
		"render/environment/drawSunAndMoon", 0 ) ? EnviroMinder::DRAW_SUN_FLARE : 0 );	

	flags = renderEnvironment && !!Options::getOptionInt( "render/environment", 1 ) ? flags : 0;
	
	enviroMinder().drawHind( dTime_, flags, renderEnvironment );
}


void RompHarness::drawSceneStuff( bool showWeather /* = true */,
								 bool showFlora /* = true */,
								 bool showFloraShadowing /* = false */ )
{
	BW_GUARD;

	// check if flora can be seen
	bool canDrawFlora = showFlora && Terrain::BaseTerrainRenderer::instance()->canSeeTerrain();

	enviroMinder().drawFore( dTime_, showWeather, canDrawFlora, showFloraShadowing, false, true );
}


void RompHarness::drawDelayedSceneStuff( bool renderEnvironment /* = true */ )
{
	BW_GUARD;

	uint32 flags;
	flags = 0;
	flags |= ( Options::getOptionInt(
		"render/environment/drawSunAndMoon", 0 ) ? EnviroMinder::DRAW_SUN_AND_MOON : 0 );
	flags |= ( Options::getOptionInt(
		"render/environment/drawSky", 0 ) ? EnviroMinder::DRAW_SKY_GRADIENT : 0 );
	flags |= ( Options::getOptionInt(
		"render/environment/drawSkyBoxes", 0 ) ? EnviroMinder::DRAW_SKY_BOXES : 0 );
	flags |= ( Options::getOptionInt(
		"render/environment/drawSunAndMoon", 0 ) ? EnviroMinder::DRAW_SUN_FLARE : 0 );	
		
	flags = renderEnvironment && !!Options::getOptionInt( "render/environment", 1 ) ? flags : 0;

	enviroMinder().drawHindDelayed( dTime_, flags );
}


void RompHarness::drawPostSceneStuff( Moo::DrawContext& drawContext, bool showWeather /* = true */)
{
	BW_GUARD;

	Waters::instance().tick( dTime_ );

	// update any dynamic textures
	TextureRenderer::updateCachableDynamics( dTime_ );	

	bool drawWater = Options::getOptionInt( "render/scenery", 1 ) &&
		Options::getOptionInt( "render/scenery/drawWater", 1 );
	Waters::drawWaters( drawWater );

	bool drawWaterReflection = drawWater &&
		Options::getOptionInt( "render/scenery/drawWater/reflection", 1 );
	Waters::drawReflection( drawWaterReflection );

	bool drawWire = Options::getOptionInt( "render/scenery/wireFrame", 0 )==1;
	Waters::instance().drawWireframe(drawWire);

	if( drawWater )
	{
		Waters::instance().updateSimulations( dTime_ );
	}

	if (distortion_ && distortion_->begin())
	{
		ToolPtr spTool = ToolManager::instance().tool();
		if ( spTool )
		{
			spTool->render( drawContext );
			drawContext.end( Moo::DrawContext::TRANSPARENT_CHANNEL_MASK );
			drawContext.flush( Moo::DrawContext::TRANSPARENT_CHANNEL_MASK, false );
			drawContext.begin( Moo::DrawContext::TRANSPARENT_CHANNEL_MASK );
		}
		enviroMinder().drawFore( dTime_, showWeather, false, false, true, false );
		distortion_->end();
	}
	else
	{
		Waters::instance().drawDrawList( dTime_ );
	}
		
	enviroMinder().drawFore( dTime_, showWeather, false, false, true, false );

	drawContext.end( Moo::DrawContext::TRANSPARENT_CHANNEL_MASK );
	drawContext.flush( Moo::DrawContext::TRANSPARENT_CHANNEL_MASK );
	drawContext.begin( Moo::DrawContext::TRANSPARENT_CHANNEL_MASK );
}

void RompHarness::drawPostProcessStuff()
{
	BW_GUARD;

	LensEffectManager::instance().tick( dTime_ );	
	LensEffectManager::instance().draw();

	//TODO : this is specific to shimmer channel. remove
	Moo::Material::shimmerMaterials = false;
	//end TODO

	PostProcessing::Manager::instance().draw();	

	HistogramProvider::instance().update();
}


TimeOfDay* RompHarness::timeOfDay() const
{
	BW_GUARD;

	return enviroMinder().timeOfDay();
}

EnviroMinder& RompHarness::enviroMinder() const
{
	BW_GUARD;

	return ChunkManager::instance().cameraSpace()->enviro();
}


/*~ function RompHarness.setTime
 *	@components{ tools }
 *
 *	This method sets the current time of the day.
 *
 *	@param time		The new time of the day, in hours.
 */
PyObject * RompHarness::py_setTime( PyObject * args )
{
	BW_GUARD;

	float t;

	if (!PyArg_ParseTuple( args, "f", &t ))
	{
		PyErr_SetString( PyExc_TypeError, "RompHarness.setTime() "
			"expects a float time" );
		return NULL;
	}

	enviroMinder().timeOfDay()->gameTime( t );

	Py_RETURN_NONE;
}


/*~ function RompHarness.setSecondsPerHour
 *	@components{ tools }
 *
 *	This method sets the time scale, by setting the number of real world
 *	seconds that an in-game hour has.
 *
 *	@param seconds	The number of seconds per hour.
 */
PyObject * RompHarness::py_setSecondsPerHour( PyObject * args )
{
	BW_GUARD;

	float t;

	if (!PyArg_ParseTuple( args, "f", &t ))
	{
		PyErr_SetString( PyExc_TypeError, "RompHarness.setSecondsPerHour() "
			"expects a float time" );
		return NULL;
	}

	enviroMinder().timeOfDay()->secondsPerGameHour( t );

	Py_RETURN_NONE;
}


/**
 *	This method enables or disables global fogging
 */
void RompHarness::fogEnable( bool state )
{
	BW_GUARD;

	FogController::instance().enable( state );

	Options::setOptionInt( "render/environment/drawFog", state ? 1 : 0 );
}


/**
 *	This method returns the global fogging state.
 */
bool RompHarness::fogEnable() const
{
	BW_GUARD;

	return FogController::instance().enable();
}

BW_END_NAMESPACE
//---------------------------------------------------------------------------
