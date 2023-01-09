
#include "pch.hpp"

#include "star_dome.hpp"
#include "resmgr/bwresource.hpp"
#include "moo/render_context.hpp"
#include "moo/visual_manager.hpp"
#include "moo/texture_manager.hpp"

DECLARE_DEBUG_COMPONENT2( "Romp", 0 )


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "star_dome.ipp"
#endif

StarDome::StarDome()
:visual_( NULL )
{
}


StarDome::~StarDome()
{
	if ( visual_ )
		visual_ = NULL;
}


/**
 * Builds the star dome mesh and material
 */
void
StarDome::init( void )
{
#error ("DONT USE STAR DOME NO MORE" )
	BW::string visualName = "objects/models/level/general/Gen_StarDome.static.visual";

	if( BWResource::instance().rootSection()->openSection( visualName ) )
	{
		visual_ = Moo::VisualManager::instance().get( visualName );
	}	


	if ( visual_ )
	{
		texture_ = Moo::TextureManager::instance()->get( "maps/fx/environment/star_one.bmp" );

		Moo::TextureStage ts;

		ts.colourOperation( Moo::TextureStage::SELECTARG2, Moo::TextureStage::CURRENT, Moo::TextureStage::TEXTURE );
		ts.alphaOperation( Moo::TextureStage::DISABLE );
		ts.textureWrapMode( Moo::TextureStage::REPEAT );
		ts.pTexture( texture_ );

		Moo::TextureStage ts1;

		mat_.addTextureStage( ts );
		mat_.addTextureStage( ts1 );

		mat_.selfIllum( 255.f );
		mat_.srcBlend( Moo::Material::ONE );
		mat_.destBlend( Moo::Material::ZERO );
		mat_.zBufferRead( false );
		mat_.zBufferWrite( false );
		mat_.sorted( false );
		mat_.alphaBlended( false );
		mat_.fogged( false );

		visual_->overrideMaterial( "stars", mat_ );
	}
	else
	{
		CRITICAL_MSG( "StarDome::init - Could not load Gen_StarDome.static.visual" );

		visual_ = NULL;
	}
}


/**
 * Draws the star dome.  This must be called
 * before any other drawing, as the sky gradient is
 * painted on top of the stars
 */
void StarDome::draw( float timeOfDay )
{
	if (!visual_)
		return;

	Matrix oldViewTransform = Moo::rc().view();

	Moo::rc().push();

	//set the world matrix to identity
	Moo::rc().world( Matrix::identity );		

	//set the view matrix to rotate a little bit.
	//first, start off with just the rotation from the world to view.
	Matrix transform( oldViewTransform );
	//null out the transform
	transform._41 =0.f;
	transform._42 =0.f;
	transform._43 =0.f;
	//apply an additional rotation
	Matrix rotation;
	rotation.setRotateX( 0.6f );
	transform.preMultiply( rotation );
	rotation.setRotateY( ( timeOfDay / 24.f ) * float( 2.0 * MATH_PI ) );
	transform.preMultiply( rotation );

	Moo::rc().view( transform );
	Moo::rc().updateViewTransforms();

	visual_->draw();
	Moo::rc().pop();

	Moo::rc().view( oldViewTransform );
	Moo::rc().updateViewTransforms();
}

std::ostream& operator<<(std::ostream& o, const StarDome& t)
{
	o << "StarDome\n";
	return o;
}

BW_END_NAMESPACE

/*StarDome.cpp*/
