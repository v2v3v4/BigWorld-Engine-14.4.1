#include "pch.hpp"

#include "sea.hpp"

#include "moo/render_context.hpp"
#include "moo/geometrics.hpp"

#include "math/colour.hpp"


BW_BEGIN_NAMESPACE

/**
 *	Constructor
 */
Sea::Sea() :
	seaLevel_( 0.f ),
	wavePeriod_( 1.f ),
	waveExtent_( 0.f ),
	tidePeriod_( 1.f ),
	tideExtent_( 0.f ),
	surfaceTopColour_( 0x602000ff ),
	surfaceBotColour_( 0x40ffffff ),
	underwaterColour_( 0x602000ff ),
	waveTime_( 0.f )
{
	BW_GUARD;
}


/**
 *	This method loads a sea from a data section
 */
void Sea::load( DataSectionPtr pSect )
{
	BW_GUARD;
	this->seaLevel( pSect->readFloat( "seaLevel", seaLevel_ ) );
	this->wavePeriod( pSect->readFloat( "wavePeriod", wavePeriod_ ) );
	this->waveExtent( pSect->readFloat( "waveExtent", waveExtent_ ) );
	this->tidePeriod( pSect->readFloat( "tidePeriod", tidePeriod_ ) );
	this->tideExtent( pSect->readFloat( "tideExtent", tideExtent_ ) );

	this->surfaceTopColour( Colour::getUint32(
		pSect->readVector4( "surfaceTopColour",
			Colour::getVector4( surfaceTopColour_ ) ) ) );

	this->surfaceBotColour( Colour::getUint32(
		pSect->readVector4( "surfaceBotColour",
			Colour::getVector4( surfaceBotColour_ ) ) ) );

	this->underwaterColour( Colour::getUint32(
		pSect->readVector4( "underwaterColour",
			Colour::getVector4( underwaterColour_ ) ) ) );

	waveTime_ = 0.f;
}


/**
 *	This method saves a sea into a data section
 */
void Sea::save( DataSectionPtr pSect )
{
	BW_GUARD;
	pSect->writeFloat( "seaLevel", seaLevel_ );
	pSect->writeFloat( "wavePeriod", wavePeriod_ );
	pSect->writeFloat( "waveExtent", waveExtent_ );
	pSect->writeFloat( "tidePeriod", tidePeriod_ );
	pSect->writeFloat( "tideExtent", tideExtent_ );

	pSect->writeVector4( "surfaceTopColour", Colour::getVector4( surfaceTopColour_ ) );
	pSect->writeVector4( "surfaceBotColour", Colour::getVector4( surfaceBotColour_ ) );
	pSect->writeVector4( "underwaterColour", Colour::getVector4( underwaterColour_ ) );
}




/**
 *	This method draws the sea
 */
void Sea::draw( float dTime, float timeOfDay )
{
	BW_GUARD;
	float seaNow = seaLevel_ +
		waveExtent_ * sinf( waveTime_ * 2.f * MATH_PI / wavePeriod_ ) +
		tideExtent_ * sinf( timeOfDay * 2.f * MATH_PI / tidePeriod_ );
	Vector3 pos( Moo::rc().invView().applyToOrigin() );

	Moo::rc().push();
	Matrix bigFlat;
	if (pos.y >= seaNow)
	{
		bigFlat.setScale( 5000, 0, 5000 );
		bigFlat.translation( Vector3( pos.x, seaNow, pos.z ) );
		Moo::rc().world( bigFlat );
		Geometrics::drawUnitSquareOnXZPlane( Moo::Colour( surfaceTopColour_ ) );
	}
	else
	{
		bigFlat.setScale( -5000, 0, 5000 );
		bigFlat.translation( Vector3( pos.x, seaNow, pos.z ) );
		Moo::rc().world( bigFlat );
		Geometrics::drawUnitSquareOnXZPlane( Moo::Colour( surfaceBotColour_ ) );

		Geometrics::drawRect(
			Vector2( 0.f, 0.f ),
			Vector2( Moo::rc().screenWidth(), Moo::rc().screenHeight() ),
			Moo::Colour( underwaterColour_ ) );
	}
	Moo::rc().pop();

	waveTime_ += dTime;
	if (waveTime_ > wavePeriod_) waveTime_ -= wavePeriod_;
}

BW_END_NAMESPACE

// sea.cpp
