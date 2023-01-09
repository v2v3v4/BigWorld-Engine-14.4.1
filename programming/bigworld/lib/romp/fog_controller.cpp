#include "pch.hpp"
#include "fog_controller.hpp"

#include "moo/render_context.hpp"
#include "math/colour.hpp"
#include "cstdmf/debug.hpp"

#include "pyscript/pyobject_plus.hpp"
#include "pyscript/script.hpp"

DECLARE_DEBUG_COMPONENT2( "Romp", 2 );


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "fog_controller.ipp"
#endif

// ----------------------------------------------------------------------------
// Section: FogController
// ----------------------------------------------------------------------------


FogController::FogController()
:colour_( 0xffffffff ),
 enabled_( true ),
 global_emitter_id_( 0 ),
 farObjectTFactor_( 0xffffffff ),
 additiveFarObjectTFactor_( 0xffffffff ),
 nearMultiplier_( 0.f ),
 multiplierTotal_( 0.f ),
 v4Colour_( 0.f, 0.f, 0.f, 0.f )
{
	BW_GUARD;
	MF_WATCH( "Client Settings/std fog/enabled",
		enabled_,
		Watcher::WT_READ_WRITE,
		"Enable visual display of fogging" );
	MF_WATCH( "Client Settings/std fog/currentNear",
		nearMultiplier_,
		Watcher::WT_READ_ONLY,
		"The current multiplier for near fog amount." );
	MF_WATCH( "Client Settings/std fog/currentFar",
		multiplier_,
		Watcher::WT_READ_ONLY,
		"The current far clip plane multiplier for fog." );
	MF_WATCH( "Client Settings/std fog/currentColour",
		v4Colour_,
		Watcher::WT_READ_ONLY,
		"The current fog colour." );
}


FogController & FogController::instance()
{
	REGISTER_SINGLETON( FogController )
	SINGLETON_MANAGER_WRAPPER( FogController )
	static FogController fc;
	return fc;
}


void FogController::tick()
{
	BW_GUARD;
	Vector3 camPos = Moo::rc().invView().applyToOrigin();

	Vector3 fogEmitterColour;
	Emitter * currentFogEmitter = NULL;
	float maxMultiplier = 0.f;
	float nearM = 0.f;
	Vector4 weightedFogColour( 0.f, 0.f, 0.f, 0.f );
	multiplierTotal_ = 0.f;

	//Accumulate the effect of all fog emitters
	Emitters::iterator it = emitters_.begin();
	Emitters::iterator end = emitters_.end();
	while( it != end )
	{
		Emitter & e = *it++;

		float distanceSq = 0.f;
		if ( e.localised_ )
		{
			Vector3 dist = e.position_ - camPos;
			distanceSq = dist.lengthSquared();
		}

		//if within the range of this fog emitter
		if ( distanceSq < e.outerRadiusSquared() )
		{
			//calculate the effective t value
			float t = 1.f;
			if ( distanceSq > e.innerRadiusSquared() )
			{
				float dist = sqrtf(distanceSq);
				t = ( dist - e.innerRadius() ) / ( e.falloffRange() );
				t = 1.f - t;
			}

			float effectiveMultiplier = t * e.maxMultiplier_;
			float effectiveNear = t * e.nearMultiplier_;

			Vector4 col = Colour::getVector4( e.colour_ );
			multiplierTotal_ += effectiveMultiplier;

			// TODO: This doesn't use += because of what appears to be a compiler bug
			weightedFogColour = weightedFogColour + ( col * effectiveMultiplier );

			//update maxMultiplier
			if ( effectiveMultiplier > maxMultiplier )
			{
				maxMultiplier = effectiveMultiplier;
			}

			if ( nearM < effectiveNear )
			{
				nearM = effectiveNear;
			}
		}
	}

	//Now, calculate the resultant fog

	//return if multiplierTotal_ is too small
	//and avoid the possible div 0
	if ( multiplierTotal_ < 0.1f )
		return;

	weightedFogColour[0] /= multiplierTotal_;
	weightedFogColour[1] /= multiplierTotal_;
	weightedFogColour[2] /= multiplierTotal_;
	weightedFogColour[3] /= multiplierTotal_;
	this->colour( Colour::getUint32( weightedFogColour ) );
	v4Colour_ = Colour::getVector4( this->colour() );
	this->multiplier( maxMultiplier );
	this->nearMultiplier( nearM );

	//calculate the texture factor for non-fogged distant objects
	//farObjectTFactor_ = blend from WHITE to FOG COLOUR as multiplier
	//goes from 1.f to 2.f
	if ( this->multiplier_ <= 1.f )
	{
		farObjectTFactor_ = 0xffffffff;
		additiveFarObjectTFactor_ = 0xffffffff;
	}
	else if ( this->multiplier_ > 2.f )
	{
		farObjectTFactor_ = this->colour();
		additiveFarObjectTFactor_ = 0x10101010;
	}
	else
	{
		float t = 1.f - ( this->multiplier() - 1.f );
		weightedFogColour[0] = ( 255.f * t ) + ( ( 1.f - t ) * weightedFogColour[0] );
		weightedFogColour[1] = ( 255.f * t ) + ( ( 1.f - t ) * weightedFogColour[1] );
		weightedFogColour[2] = ( 255.f * t ) + ( ( 1.f - t ) * weightedFogColour[2] );
		weightedFogColour[3] = ( 255.f * t ) + ( ( 1.f - t ) * weightedFogColour[3] );

		farObjectTFactor_ = Colour::getUint32( weightedFogColour );

		t *= t;
		t *= 255.f;
		if ( t < 16.f )
			t = 16.f;
		additiveFarObjectTFactor_ = Colour::getUint32( Vector4( t, t, t, t ) );
	}

	//Now apply the fog if we need to.
	updateFogParams();
}


/**
 *	Commits the emitter info to the fog controller's list
 */
void FogController::update( Emitter & emitter )
{
	BW_GUARD;
	Emitters::iterator it = emitters_.begin();
	Emitters::iterator end = emitters_.end();

	while( it != end )
	{
		Emitter & e = *it++;

		if ( e.id_ == emitter.id_ )
		{
			e = emitter;
			break;
		}
	}
}


/**
 *	This method commits the fog settings.
 */
void FogController::updateFogParams()
{
	BW_GUARD;
	//set the fog onto the direct 3D device
	float farFog = Moo::rc().camera().farPlane() / multiplier_;
	float nearFog = farFog * nearMultiplier_;

	if (!enabled_)
	{
		nearFog = Moo::rc().camera().farPlane() * 100.f;
		farFog = Moo::rc().camera().farPlane() * 1000.f;
	}


	// make sure the render context has the correct settings
	// this sets it on the device as well
	Moo::FogParams params = Moo::FogHelper::pInstance()->fogParams();
	params.m_color =  colour_; 
	params.m_enabled = enabled_; 
	params.m_start =  nearFog;
	params.m_end =  farFog;
	Moo::FogHelper::pInstance()->fogParams(params);
}


// ----------------------------------------------------------------------------
// Section: Python stuff
// ----------------------------------------------------------------------------

/*~ function BigWorld.addFogEmitter
 *	@components{ client, tools }
 *
 *	This function adds a fog emitter to the world at the specified location,
 *	with the specified density, radii and colour.
 *
 *	There are two types of emitters, non-localised and localised.  A
 *	non-localised fog emitter adds fog to the entire world, causing
 *	all cameras to render their views with fog of the specified density.
 *	The emitters position, innerRadius and outerRadius values are ignored.
 *
 *	A localised fog emitter causes a camera to add fog to a scene if it is
 *	within the outer radius of the fog emitter.  The density of the fog
 *	(try numbers between 2 and 10, with 10 being denser) tapers off linearly
 *	between the specified density at the inner radius to zero at the outer
 *	radius.  Note that a localised fog emitter does not actually
 *	appear in the camera as a patch of fog, if the camera is outside it. It
 *	merely applies a uniform fog to any cameras which are within it.
 *
 *	Fog emitters are identified by an integer id, which is returned by this
 *	function.  This can be passed to the delFogEmitter function to remove that
 *	emitter.
 *
 *	@param	position	a Vector3.  This is the position for the fogEmitter.
 *						Ignored if localised is zero (false).
 *	@param	density		a float.  This is how dense to make the fog.  Try
 *						numbers between 2 and 10, with 10 being denser.
 *	@param	innerRadius	a float.  This is the inner radius for the fog emitter.
 *						Ignored if localised is zero (false).  If the camera
 *						is within this radius of the emitter, then this emitter
 *						will apply fog of the specified density.
 *	@param	outerRadius	a float.  This is the outer radius for the fog emitter.
 *						Ignored if localised is zero (false).  If the camera is
 *						between the inner radius and the outer radius then the
 *						density of the fog applied to the camera will be
 *						linearly interpolated between the specified density and
 *						zero.
 *	@param	colour		an integer. This is the colour of the fog to apply.  It
 *						is in packed rgb format.
 *	@param	localised	An integer, treated as a boolean.  If this is true,
 *						then the fogEmitter will be localised, otherwise
 *						it will be non-localised.
 *
 *	@return				an integer.  This is the id of this fog emitter.
 */
/**
 *	adds a fog emitter to the current location
 *
 *	args are : density ( try between 2 and 10 )
 *			   radius ( in metres )
 *			   colour ( argb in packed 32 bit fmt )
 */
static PyObject * py_addFogEmitter( PyObject * args )
{
	BW_GUARD;
	float density;
	float inner;
	float outer;
	float x,y,z;
	unsigned int colour;
	int localised;

	if (!PyArg_ParseTuple( args, "(fff)fffii", &x, &y, &z, &density, &inner, &outer, &colour, &localised ))
	{
		PyErr_SetString( PyExc_TypeError, "BigWorld.addFogEmitter: "
			"Argument parsing error. "
			"Expected (x, y, z), density, inner, outer, colour, localised" );
		return NULL;
	}

	FogController::Emitter emitter;
	emitter.colour_ = colour;
	emitter.maxMultiplier_ = density;
	emitter.position_ = Vector3( x, y, z );
	emitter.innerRadius( inner );
	emitter.outerRadius( outer );
	emitter.localised_ = !!(localised);

	int fogEmitterID = FogController::instance().addEmitter( emitter );

	return PyInt_FromLong( (long)fogEmitterID );
}
PY_MODULE_FUNCTION( addFogEmitter, BigWorld )

/*~ function BigWorld.delFogEmitter
 *	@components{ client, tools }
 *
 *	This function removes a fog emitter which was previously created by
 *	addFogEmitter.
 *
 *	@param	id		An integer.  The id of the fog emitter to delete.  This is
 *					returned by addFogEmitter when a fog emitter is created.
 */
/**
 *	deletes a fog emitter
 *
 *	args are : the fog emitter
 */
static PyObject * py_delFogEmitter( PyObject * args )
{
	BW_GUARD;
	int fogEmitterID;

	if (!PyArg_ParseTuple( args, "i", &fogEmitterID ))
	{
		PyErr_SetString( PyExc_TypeError, "BigWorld.delFogEmitter: "
			"Argument parsing error." );
		return NULL;
	}

	FogController::instance().delEmitter( fogEmitterID );

	Py_RETURN_NONE;
}
PY_MODULE_FUNCTION( delFogEmitter, BigWorld )

BW_END_NAMESPACE

// fog_controller.cpp
