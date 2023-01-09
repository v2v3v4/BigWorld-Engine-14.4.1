#include "pch.hpp"
#include "enviro_minder_scene_provider.hpp"

#include <math/convex_hull.hpp>

#include <moo/light_container.hpp>

#include <romp/enviro_minder.hpp>
#include <romp/time_of_day.hpp>

#include <scene/scene.hpp>
#include <scene/scene_object.hpp>

#include <space/client_space.hpp>

namespace BW {


//-----------------------------------------------------------------------------
EnviroMinderSceneProvider::EnviroMinderSceneProvider( SpaceID id )
	: environment_( id )
	, pOutLight_( NULL )
	, sunLight_( NULL )
	, ambientLight_( 0.1f, 0.1f, 0.1f, 1.f )
	, lights_( new Moo::LightContainer() )
{
}


//-----------------------------------------------------------------------------
EnviroMinderSceneProvider::~EnviroMinderSceneProvider()
{
}


//-----------------------------------------------------------------------------
void * EnviroMinderSceneProvider::getView(
	const SceneTypeSystem::RuntimeTypeID & viewTypeID )
{
	void * result = NULL;

	exposeView<ITickSceneProvider>( this, viewTypeID, result );
	exposeView<ILightSceneViewProvider>( this, viewTypeID, result );

	return result;
}


//-----------------------------------------------------------------------------
void EnviroMinderSceneProvider::tick( float dTime )
{
}


//-----------------------------------------------------------------------------
void EnviroMinderSceneProvider::updateAnimations( float dTime )
{
	updateHeavenlyLighting();
	environment_.updateAnimations();
}


//-----------------------------------------------------------------------------
size_t EnviroMinderSceneProvider::intersect( const ConvexHull & hull,
	Moo::LightContainer & lightContainer ) const
{
	return addEnviroLights(lightContainer);
}


//-----------------------------------------------------------------------------
size_t EnviroMinderSceneProvider::intersect( const AABB & bbox,
	Moo::LightContainer & lightContainer ) const
{
	return addEnviroLights(lightContainer);
}


//-----------------------------------------------------------------------------
size_t EnviroMinderSceneProvider::addEnviroLights(
	Moo::LightContainer & lightContainer ) const
{
	lightContainer.addDirectional( sunLight_ );
	lightContainer.ambientColour( ambientLight_ );
	return 2;
}


//-----------------------------------------------------------------------------
const Moo::DirectionalLightPtr & EnviroMinderSceneProvider::sunLight() const
{
	return sunLight_;
}


// ----------------------------------------------------------------------------
const Moo::Colour & EnviroMinderSceneProvider::ambientLight() const
{
	return ambientLight_;
}


// ----------------------------------------------------------------------------
const Moo::LightContainerPtr & EnviroMinderSceneProvider::lights() const
{
	return lights_;
}


// ----------------------------------------------------------------------------
void EnviroMinderSceneProvider::loadEnvironment( const DataSectionPtr& pSpaceSettings )
{
	environment_.load( pSpaceSettings );

	// TODO: factor out of ChunkSpace and CompiledSpace - duplicate code.
	sunLight_ = new Moo::DirectionalLight(
		Moo::Colour( 0.8f, 0.5f, 0.1f, 1.f ),
		Vector3( 0, 1 ,0 ) );
	addEnviroLights( *lights_ );
	if (environment_.timeOfDay() != NULL)
	{
		pOutLight_ = &environment_.timeOfDay()->lighting();
	}
}


// ----------------------------------------------------------------------------
void EnviroMinderSceneProvider::updateHeavenlyLighting()
{
	// TODO: factor out of ChunkSpaceAdapter and this class - duplicate code.
	if (pOutLight_ != NULL && sunLight_)
	{
		sunLight_->colour( Moo::Colour( pOutLight_->sunColour ) );

		//the dawn/dusk sneaky swap changes moonlight for
		//sunlight when the moon is brighter

		Vector3 lightPos = pOutLight_->mainLightTransform().applyPoint( Vector3( 0, 0, -1 ) );
		Vector3 dir = Vector3( 0, 0, 0 ) - lightPos;
		dir.normalise();
		sunLight_->direction( dir );
		sunLight_->worldTransform( Matrix::identity );//a hack to set worldDirection_ as direction_

		ambientLight_ = Moo::Colour( pOutLight_->ambientColour );
		lights_->ambientColour( ambientLight_ );
	}
}

EnviroMinder & EnviroMinderSceneProvider::enviroMinder()
{
	return environment_;
}

} // namespace BW
