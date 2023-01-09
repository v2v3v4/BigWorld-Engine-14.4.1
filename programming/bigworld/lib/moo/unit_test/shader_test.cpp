#include "pch.hpp"
#include "shader_test.hpp"

#include "moo/effect_material.hpp"
#include "moo/render_context.hpp"
#include "romp/geometrics.hpp"
#include "moo/effect_visual_context.hpp"

void BeginFrame()
{
	// Begin Frame
	Moo::rc().beginScene();
	Moo::rc().device()->Clear( 0, NULL, 
		D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL,
		0xFFFF00FF, 1, 0 );
	Moo::rc().nextFrame();
}

void EndFrame()
{
	// End Frame
	Moo::rc().endScene();
	Moo::rc().device()->Present( NULL, NULL, NULL, NULL );
	Moo::rc().resetPrimitiveCount();
}

ShaderTest::ShaderTest()
{
	// Prepare global settings for shader test scene.

	// Lights
	Moo::LightContainerPtr pLights = new Moo::LightContainer;
	pLights->ambientColour( Moo::Colour(0xff808080) );
	pLights->addDirectional( new Moo::DirectionalLight(
		Moo::Colour(0.75f,0.75f,0.75f,1.f), Vector3(0.5f,0.5f,-0.5f)));
	Moo::rc().lightContainer( pLights );

	// Camera
	Matrix newProj;
	newProj.perspectiveProjection( MATH_PI / 3.f, 1.0f, 0.25f, 500.f );

	Matrix m;
	m.lookAt( Vector3( 0, 0, -3 ), Vector3( 0, 0, 1 ), Vector3( 0, 1, 0 ) );
	Moo::rc().world( Matrix::identity );
	Moo::rc().view( m );
	Moo::rc().projection( newProj );
	Moo::rc().updateViewTransforms();

	// Constants
	Moo::rc().effectVisualContext().initConstants();
}

/**
 *	Initialise ShaderTest object.
 *
 *	@param   effectFileName The effect file to load, eg "lightonly.fx".
 *
 *	@returns A pointer to created EffectMaterial (owned by ShaderTest), or
 *		NULL if there was a problem creating the effect. 
*/
Moo::EffectMaterialPtr ShaderTest::init( const BW::string& effectFileName )    
{
	pEffect_ = new Moo::EffectMaterial;
	pEffect_->initFromEffect( effectFileName );

	Moo::EffectPropertyPtr pTexProperty = pEffect_->getProperty("diffuseMap");

	Moo::BaseTexturePtr tex =
		Moo::TextureManager::instance()->get( "helpers/maps/aid_portal.dds" );
	MF_ASSERT( tex != NULL );

	pTexProperty->be(tex);

	return pEffect_;
}

/**
 * Finalise ShaderTest object  
 */ 
void ShaderTest::fini()
{
	pEffect_ = NULL;
}

/**
* Test a shader against a reference image. 
*
* This function uses the D3D reference device to render a test scene with
* a given effect and technique. If the render differs from the reference image
* then the function returns false. This function can be used by a unit test
* for each shader.
*
* @param technique      The index of the technique to use. If unavailable, 
*                       use the best technique up to this index.
* @param refImageFile   The filename of the reference image to test against.
*                       eg "lightonly_ref.png".
*/
bool ShaderTest::test( uint32 technique, const BW::string& refImageFile )
{
	BeginFrame();

	{	
		// Action
		Geometrics::instance().drawSphere( Vector3( 0.0f, 0.0f, 0.0f),
			0.5f, Moo::Colour(0xffffffff), pEffect_.getObject() );
	}

	EndFrame();

	return false;
}

/**
* Get a standard reference image file name given an effect and technique.
* Typically this will be something like "effectName_techniqueIndex_ref.png"
**/
void ShaderTest::getImageFileNameForEffect(  
										const BW::string&  effectFileName,
										uint32				techniqueIndex,
										BW::string&		refImageFileName )
{
}
