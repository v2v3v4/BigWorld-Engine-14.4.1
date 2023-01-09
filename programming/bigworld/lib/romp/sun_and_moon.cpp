#include "pch.hpp"

#include "sun_and_moon.hpp"
#include "enviro_minder.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/datasection.hpp"
#include "resmgr/auto_config.hpp"
#include "moo/render_context.hpp"
#include "moo/texture_manager.hpp"
#include "lens_effect_manager.hpp"
#include "fog_controller.hpp"

#include "time_of_day.hpp"

DECLARE_DEBUG_COMPONENT2( "Romp", 0 );


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "sun_and_moon.ipp"
#endif


static AutoConfigString s_sunMaterial( "environment/sunMaterial" );
static AutoConfigString s_moonMaterial( "environment/moonMaterial" );
static AutoConfigString s_sunFlareXML( "environment/sunFlareXML" );
static AutoConfigString s_moonFlareXML( "environment/moonFlareXML" );


///Constructor
SunAndMoon::SunAndMoon()
:moon_	( NULL ),
 sun_( NULL ),
 timeOfDay_( NULL ),
 sunMat_( NULL ),
 moonMat_( NULL ) 
{
	BW_GUARD;
}


///Destructor
SunAndMoon::~SunAndMoon()
{
	BW_GUARD;
	this->destroy();	
}


/**
 *	This method draws a square mesh, using a given material.
 *	This method is used for the moon, moon mask, sun and flare.
 *
 *	@param cam		the world matrix
 *	@param pMesh	a pointer to the square mesh to draw
 *	@param pMat		a pointer to the material to draw with
 */
void SunAndMoon::drawSquareMesh( const Matrix & world, Vector4 color, CustomMesh<Moo::VertexXYZNDUV> * pMesh, Moo::EffectMaterialPtr pMat )
{	
	BW_GUARD;
	Moo::rc().push();	
	Moo::rc().world( world );

	if (pMat->begin())
	{
		for (uint32 i=0; i<pMat->numPasses(); i++)
		{
			pMat->beginPass(i);
			D3DXHANDLE handle = pMat->pEffect()->pEffect()->GetParameterByName( NULL, "world" );
			pMat->pEffect()->pEffect()->SetMatrix( handle, &world );
			handle = pMat->pEffect()->pEffect()->GetParameterByName( NULL, "color" );
			pMat->pEffect()->pEffect()->SetVector( handle, &color );
			handle = pMat->pEffect()->pEffect()->GetParameterByName( NULL, "dist" );
			pMat->pEffect()->pEffect()->SetFloat( handle, Moo::rc().camera().farPlane() );
			pMat->pEffect()->pEffect()->CommitChanges();
			pMesh->drawEffect();
			pMat->endPass();
		}
		pMat->end();
	}

	Moo::rc().pop();
}


/**
 *	This method creates the sun and moon meshes / materials
 */
void SunAndMoon::create( void )
{
	BW_GUARD;
	this->destroy();

	createMoon( );
	createSun( );

	//Sun flare
	DataSectionPtr pLensEffectsSection = BWResource::openSection( s_sunFlareXML );
	if ( pLensEffectsSection )
	{
		LensEffect l;
		l.load( pLensEffectsSection );
		sunLensEffect_ = l;
		sunLensEffect_.clampToFarPlane( true );
	}
	else
	{
		WARNING_MSG( "Could not find lens effects definitions for the Sun\n" );
	}	
}


/**
 *	This method cleans up memory allocated for the sun and moon
 */
void SunAndMoon::destroy( void )
{
	BW_GUARD;
	bw_safe_delete(moon_);
	bw_safe_delete(sun_);
	moonMat_ = NULL;
	sunMat_ = NULL;
}


/**
 *	This method creates the mesh and material for the sun + flares.
 */
void SunAndMoon::createSun( void )
{
	BW_GUARD;
	sun_ = new CustomMesh<Moo::VertexXYZNDUV>( D3DPT_TRIANGLESTRIP );

	//this value scales its apparent size
	float z = -6.78f;

	Moo::VertexXYZNDUV vert;
	vert.colour_ = 0x80777777;
	vert.normal_.set( 0,0,1 );

	vert.pos_.set(-1, 1, z );
	vert.uv_.set( 0, 0 );
	sun_->push_back( vert );

	vert.pos_.set( 1, 1, z );
	vert.uv_.set( 1, 0 );
	sun_->push_back( vert );

	vert.pos_.set( -1, -1, z );
	vert.uv_.set( 0, 1 );
	sun_->push_back( vert );

	vert.pos_.set( 1, -1, z );
	vert.uv_.set( 1,1 );
	sun_->push_back( vert );

	sunMat_ = new Moo::EffectMaterial();
	sunMat_->load( BWResource::openSection( s_sunMaterial ) );
}


/**
 *	This method creates the mesh + materials for the moon and mask
 */
void SunAndMoon::createMoon( void )
{	
	BW_GUARD;
	moon_ = new CustomMesh<Moo::VertexXYZNDUV>( D3DPT_TRIANGLESTRIP );

	//this value scales its apparent size
	float z = -6.78f;

	Moo::VertexXYZNDUV vert;
	vert.colour_ = 0x80777777;
	vert.normal_.set( 0,0,1 );

	vert.pos_.set( -0.6f, 0.6f, z );
	vert.uv_.set( 0, 0 );	
	moon_->push_back( vert );

	vert.pos_.set( 0.6f, 0.6f, z );
	vert.uv_.set( 1, 0 );	
	moon_->push_back( vert );

	vert.pos_.set( -0.6f, -0.6f, z );
	vert.uv_.set( 0, 1 );	
	moon_->push_back( vert );

	vert.pos_.set( 0.6f, -0.6f, z );
	vert.uv_.set( 1,1 );	
	moon_->push_back( vert );	

	//Create the moon material
	moonMat_ = new Moo::EffectMaterial();	
	moonMat_->load( BWResource::openSection( s_moonMaterial ) );
}

#define VEC3TOD3DCOLOR4(v) (Vector4(v.x/255.0f, v.y/255.0f, v.z/255.0f, 1.0f))

/**
 *	This method draws the sun and moon.
 *	The sun and moon use the timeOfDay object to position themselves
 *	in the sky.
 */
void SunAndMoon::draw()
{	
	BW_GUARD;
	if ( !timeOfDay_ )
		return;

	//apply the texture factor
	//TODO : put this into the effect
	//uint32 additiveTextureFactor = FogController::instance().additiveFarObjectTFactor();
	//uint32 textureFactor = FogController::instance().farObjectTFactor();
	//sunMat_->textureFactor( additiveTextureFactor );
	//moonMat_->textureFactor( textureFactor );

	Vector3 col(timeOfDay_->sunColor());

	drawSquareMesh( timeOfDay_->lighting().sunTransform, VEC3TOD3DCOLOR4(col), sun_, sunMat_ );

	col = timeOfDay_->moonColor();
	drawSquareMesh( timeOfDay_->lighting().moonTransform, VEC3TOD3DCOLOR4(col), moon_, moonMat_ );

	//post request for sun flare
	if (!Moo::rc().reflectionScene())
	{
		float SUN_DISTANCE = Moo::rc().camera().farPlane() * 0.9999f;
		Vector3 sunPosition(
			timeOfDay_->lighting().sunTransform.applyPoint( Vector3(0,0,-1.f) ) );	
		sunPosition.normalise();
		float dotp = sunPosition.dotProduct( Moo::rc().invView().applyToUnitAxisVector(2) );	
		Vector3 actualSunPosition( Moo::rc().invView().applyToOrigin() + sunPosition * SUN_DISTANCE );

		LensEffectManager::instance().add( 0, actualSunPosition, sunLensEffect_ );			
	}

	Moo::rc().setVertexShader( NULL );
	Moo::rc().setPixelShader( NULL );
}


void SunAndMoon::update()
{
	if ( !timeOfDay_ )
		return;

	Vector3 col(timeOfDay_->sunColor());
	sunLensEffect_.colour(D3DCOLOR_ARGB(255, (int)col.x, (int)col.y, (int)col.z));
}

std::ostream& operator<<(std::ostream& o, const SunAndMoon& t)
{
	o << "SunAndMoon\n";
	return o;
}

BW_END_NAMESPACE

// sun_and_moon.cpp
