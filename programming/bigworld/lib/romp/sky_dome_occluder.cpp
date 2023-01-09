#include "pch.hpp"
#include "cstdmf/debug.hpp"
#include "moo/moo_dx.hpp"
#include "moo/scissors_setter.hpp"
#include "moo/effect_visual_context.hpp"
#include "moo/draw_context.hpp"

#include "lens_effect.hpp"
#include "sky_dome_occluder.hpp"
#include "enviro_minder.hpp"
#include "time_of_day.hpp"

DECLARE_DEBUG_COMPONENT2( "Romp", 0 )


BW_BEGIN_NAMESPACE

//only the sun needs to use this occlusion tester.
static const uint16 SKY_DOME_MAX_LENS_FLARES = 1u;
static bool s_showOcclusionTests = false;
static uint32 s_alphaRef = 0x00000080;

// -----------------------------------------------------------------------------
// Section: Occlusion Testing Effect Constant
// -----------------------------------------------------------------------------
/**
 *	This class tells sky domes whether or not they should draw using their
 *	standard shaders, or their occlusion test shader
 */
class OcclusionTestEnable : public Moo::EffectConstantValue
{
public:
	OcclusionTestEnable():
	  value_( false )
	{		
	}
	bool operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle)
	{
		pEffect->SetInt(constantHandle, value_ ? 1 : 0);
		return true;
	}
	void value( bool value )
	{
		value_ = value;
	}
private:
	bool value_;
};

// destroyed by EffectConstantValue::fini
static OcclusionTestEnable* s_occlusionTestConstant = NULL;


// -----------------------------------------------------------------------------
// Section: Occlusion Testing Alpha Reference Constant
// -----------------------------------------------------------------------------
/**
 *	This class sets the alpha reference value for sky domes to use when doing
 *	their occlusion test pass.
 */
class OcclusionTestAlphaRef : public Moo::EffectConstantValue
{
public:
	OcclusionTestAlphaRef():
	  value_( 0x00000000 )
	{		
	}
	bool operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle)
	{
		pEffect->SetInt(constantHandle, value_);
		return true;
	}
	void value( uint32 value )
	{
		value_ = value;
	}
private:
	uint32	value_;
};

static OcclusionTestAlphaRef* s_occlusionTestAlphaRef = NULL;


// -----------------------------------------------------------------------------
// Section: Sky Dome Occluder.
//
// This occluder counts the number of occluding pixels over the sun.
// -----------------------------------------------------------------------------
bool SkyDomeOccluder::isAvailable()
{
	HRESULT hr = Moo::rc().device()->CreateQuery( D3DQUERYTYPE_OCCLUSION, NULL );	
	return ((hr==D3D_OK) && (Moo::ScissorsSetter::isAvailable()));
}


/**
 *	Constructor.
 */
SkyDomeOccluder::SkyDomeOccluder(EnviroMinder& enviroMinder):
	helper_( SKY_DOME_MAX_LENS_FLARES, 200000 ),
	lastResult_( 0 ),
	possiblePixels_( 1000 ),
	enviroMinder_( enviroMinder )
{
	BW_GUARD;
	if (!s_occlusionTestAlphaRef)
		s_occlusionTestAlphaRef = new OcclusionTestAlphaRef();

	if (!s_occlusionTestConstant)
		s_occlusionTestConstant = new OcclusionTestEnable();

	*Moo::rc().effectVisualContext().getMapping("EnvironmentOcclusionTest") = 
		s_occlusionTestConstant;

	*Moo::rc().effectVisualContext().getMapping("EnvironmentOcclusionAlphaRef") =
		s_occlusionTestAlphaRef;

	MF_WATCH( "Render/Sky Dome Occlusion/Last Result",
			lastResult_,
			Watcher::WT_READ_ONLY,
			"Latest result from the sky dome occlusion test.  Number of cloud "
			"pixels currently covering the sun." );
}


SkyDomeOccluder::~SkyDomeOccluder()
{
	BW_GUARD;
#if ENABLE_WATCHERS
	if (Watcher::hasRootWatcher())
	{
		Watcher::rootWatcher().removeChild(
			"Render/Sky Dome Occlusion/Last Result" );
	}
#endif
}


void SkyDomeOccluder::beginOcclusionTests()
{
	BW_GUARD;
	//reset used parameters.
	helper_.begin();
}


/**
 *	This method checks the ray between the photon source
 *	and the eye.
 */
float SkyDomeOccluder::collides(
	const Vector3 & photonSourcePosition,
	const Vector3 & cameraPosition,
	const LensEffect& le )
{
	BW_GUARD;
	if ( le.maxDistance() < Moo::rc().camera().farPlane() )
		return 1.f;

	HandlePool::Handle queryHandle;
	queryHandle = helper_.handlePool().handleFromId( le.id() );

	if (helper_.beginQuery(queryHandle))
	{		
		possiblePixels_ = this->drawOccluders( le.area() );		
		helper_.endQuery(queryHandle);		
	}

	int result =  helper_.avgResult(queryHandle);
	//if ( lastResult_ != result )
	//	DEBUG_MSG( "SkyDomeOccluder says it drawing %d\n--------------\n", result );

	lastResult_ = result;

	return 1.f - Math::clamp(0.f, ((float)lastResult_ / (float)possiblePixels_), 1.f);
}


void SkyDomeOccluder::endOcclusionTests()
{
	BW_GUARD;
	helper_.end();
}


/**
 *	This method constructs a view matrix that makes the
 *	camera look directly at the sun. (don't try this at
 *	home kids :)
 *
 *	This kind of matrix is useful for drawing all environment
 *	objects that are occluding the sun directly in the middle
 *	of the screen.
 *
 *	@param	out		view matrix
 */
void SkyDomeOccluder::getLookAtSunViewMatrix( Matrix& out )
{	
	BW_GUARD;
	out = enviroMinder_.timeOfDay()->lighting().sunTransform;
	out.postRotateX( DEG_TO_RAD( 2.f * enviroMinder_.timeOfDay()->sunAngle() ) );
	out.postRotateY( MATH_PI );
	out.invert();
}

/**
* Get a rectangle to represent the viewport used when rendering occluders
*
*/
void getOccluderScissorRect(float size, uint32 & x, uint32 & y, 
							uint32 & width, uint32 & height)
{
	// 'size' is the area of the flare which is the radius in pixels.
	// The area generated for the rect is a square in terms of number of
	// pixel and is the same in width and height. 640 is used as the
	// normalised value around which the area is calculated. So for a screen
	// res of 640 pixels in width, 'size' of 35 would generate a square
	// with side length of 35 pixels.
	// The rect generated is clamped so that it is inside the screen bounds.
	MF_ASSERT( size >= 0.f );
	
	// find half size of the scissor rectangle in pixels
	const int32 normalisedRes = 640;
	const int32 halfW = (int32)Moo::rc().halfScreenWidth();
	const int32 halfH = (int32)Moo::rc().halfScreenHeight();
	const int32 nPixels = (static_cast<int32>(size) * halfW) / normalisedRes;
	
	// find top left corner, clamp to zero
	const int32 originX = max(halfW - nPixels, 0);
	const int32 originY = max(halfH - nPixels, 0);

	// find bottom right corner, clamp to screen size
	const int32 fullW = (int32)Moo::rc().screenWidth();
	const int32 fullH = (int32)Moo::rc().screenHeight();
	const int32 farX = min(halfW + nPixels, fullW);
	const int32 farY = min(halfH + nPixels, fullH);
	
	// this should all be ok now because its been clamped to range already
	MF_ASSERT( originX >= 0 && originY >= 0 );
	MF_ASSERT( farX >= 0 && farY >= 0 );
	MF_ASSERT( farX >= originX && farY >= originY );

	// write out the results
	x = static_cast<uint32>(originX);
	y = static_cast<uint32>(originY);
	width = static_cast<uint32>(farX-originX);
	height = static_cast<uint32>(farY-originY);
}

/**
 *	This method draws all the occluder objects, such as the
 *	clouds and the sky boxes.  It uses the automatic effect
 *	variable "EnvironmentOcclusionTest" to signify to these
 *	visuals that they should draw themselves using their
 *	occlusion shaders instead of their ordinary ones.
 *
 *	@return	int	the number of pixels in the region we are drawing to.
 */
uint32 SkyDomeOccluder::drawOccluders( float size )
{
	BW_GUARD;
	static bool first = true;
	if (first)
	{
		first = false;		
		MF_WATCH( "Render/Sky Dome Occlusion/Alpha Reference",
			s_alphaRef,
			Watcher::WT_READ_WRITE,
			"Alpha Reference used by sky domes when they are drawing their"
			"occlusion test pass.  Lower values indicate the sun lens flare"
			"is more likely to shine through" );
		MF_WATCH( "Render/Sky Dome Occlusion/Show tests",
			s_showOcclusionTests,
			Watcher::WT_READ_WRITE,
			"turn this on to show the occlusion tests on-screen" );
	}

	s_occlusionTestConstant->value(true);
	s_occlusionTestAlphaRef->value(s_alphaRef);

	Matrix oldView( Moo::rc().view() );

	Matrix newView;
	this->getLookAtSunViewMatrix( newView );	
	Moo::rc().view( newView );

	//temporarily set a centered viewport of the desired size.
	//then we rotate the view matrix to look at the sun, thus
	//drawing sky domes etc. in the centre of the screen (and
	//thus centered in the viewport).  The viewport clips the
	//sky domes to the size of the occlusion test, and the
	//occlusion query stuff records how many pixels were drawn.
	uint32 originX, originY, width, height;
	getOccluderScissorRect( size, originX, originY, width, height );
	Moo::ScissorsSetter v( originX, originY, width, height );

	Moo::rc().pushRenderState( D3DRS_COLORWRITEENABLE );

	if (s_showOcclusionTests)		
	{
		Moo::rc().setRenderState( D3DRS_COLORWRITEENABLE,
			D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE );
	}
	else
	{
		Moo::rc().setRenderState( D3DRS_COLORWRITEENABLE, 0 );
	}

	if (s_showOcclusionTests)		
	{
		Moo::rc().setRenderState( D3DRS_COLORWRITEENABLE,
			D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE );
	}
	else
	{
		Moo::rc().setRenderState( D3DRS_COLORWRITEENABLE, 0 );
	}

	enviroMinder_.drawSkyDomes( true );	

	s_occlusionTestConstant->value(false);

	Moo::rc().view( oldView );

	Moo::rc().popRenderState();

	return (width * height);
}


/**
 *	This method releases all queries and resets the viz test status.
 *	It is called when the device is lost.
 *
 *	The entire class is reset
 */
void SkyDomeOccluder::deleteUnmanagedObjects()
{	
	lastResult_ = 0;
}


std::ostream& operator<<(std::ostream& o, const SkyDomeOccluder& t)
{
	o << "SkyDomeOccluder\n";
	return o;
}

BW_END_NAMESPACE

// sky_dome_occluder.cpp
