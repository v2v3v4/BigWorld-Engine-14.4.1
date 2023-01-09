#include "pch.hpp"

#include "clip_gui_shader.hpp"
#include "simple_gui_component.hpp"

#include "pyscript/script.hpp"

#include "cstdmf/debug.hpp"

#ifndef CODE_INLINE
#include "clip_gui_shader.ipp"
#endif

DECLARE_DEBUG_COMPONENT2( "2DComponents", 0 )


BW_BEGIN_NAMESPACE

#undef PY_ATTR_SCOPE
#define PY_ATTR_SCOPE ClipGUIShader::
PY_RW_ATTRIBUTE_DECLARE( clipMode_, mode )


// Python statics
PY_TYPEOBJECT( ClipGUIShader )

PY_BEGIN_METHODS( ClipGUIShader )
	PY_METHOD( reset )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( ClipGUIShader )
	/* UP and DOWN modes are not yet implemented */
	/* attribute ClipGUIShader.mode
	 *	@components{ client, tools }
	 *
	 *	The mode attribute determines which edge moves to clip the component.
	 *	It can have a value of "LEFT", "RIGHT", "UP" and "DOWN".  If mode is
	 *	"LEFT", then the value attribute determines the rightmost fraction of
	 *	the component to be shown.  If mode is "RIGHT", then the value
	 *	attribute determines the leftmost fraction of the component to be
	 *	shown.  If mode is "UP", then the value attribute determines the
	 *	bottommost fraction	of the component to be shown.  If mode is "DOWN",
	 *	then the value attribute determines the topmost fraction of the
	 *	component to be shown.  
	 *
	 *	@type	String
	 */

	/*~ attribute ClipGUIShader.mode
	 *	@components{ client, tools }
	 *
	 *	The mode attribute determines which edge moves to clip the component.
	 *	It can have a value of "LEFT" or "RIGHT".  If mode is "LEFT", then the 
	 *	value attribute determines the rightmost fraction of the component to 
	 *	be shown.  If mode is "RIGHT", then the value attribute determines the 
	 *	leftmost fraction of the component to be shown.
	 *
	 *	@type	String
	 */
	PY_ATTRIBUTE( mode )
	/*~ attribute ClipGUIShader.value
	 *	@components{ client, tools }
	 *
	 *	This attribute determines the proportion of the component to reveal.
	 *	Values less than zero are treated the same as zero, values above one
	 *	are treated as one.  How to align the revealed rectangle with the
	 *	underlying component is determined by the mode attribute.
	 *
	 *	If the speed attribute is non-zero, then, when a new value is assigned,
	 *	the actual size of the clip rect will take speed seconds to move from
	 *	its initial value to the newly assigned value.  The currentValue
	 *	attribute will give read access to actual size of the clip rect as it
	 *	is moving.
	 *
	 *	@type	Float
	 */
	PY_ATTRIBUTE( value )
	/*~ attribute ClipGUIShader.speed
	 *	@components{ client, tools }
	 *
	 *	The speed attribute is the time it takes, when the attribute value is
	 *	assigned to, to adjust from its current value to the newly assigned
	 *	value.  The clip rectangle size is linearly interpolated over this 
	 *	time period.  
	 *
	 *	The currentValue attribute gives the current size of the clip rectangle
	 *	during the interpolation.
	 *
	 *	@type	Float
	 */
	PY_ATTRIBUTE( speed )
	/*~ attribute ClipGUIShader.delay
	 *	@components{ client, tools }
	 *
	 *	The delay attribute is the time it takes, when the attribute value is
	 *	assigned to, before the clipper is adjust from its current value to 
	 *  the newly assigned value.  
	 *
	 *	@type	Float
	 */
	PY_ATTRIBUTE( delay )
	/*~ attribute ClipGUIShader.currentValue
	 *	@components{ client, tools }
	 *
	 *	The currentValue attribute gives read access to the current size (in 
	 *	clip coordinates, with (-1,-1) in the bottom left of the screen and (1,1)
	 *	in the top right) of the
	 *	cliprect while it is linearly interpolating	from its initial value to
	 *	the newly assigned value attribute.  It interpolates over a number of
	 *	seconds	specified by the speed attribute.  
	 *
	 *	@type	Read-Only Float
	 */
	PY_ATTRIBUTE( currentValue )
	/*~ attribute ClipGUIShader.dynValue
	 *	@components{ client, tools }
	 *
	 *	This attribute is unsupported.
	 */
	PY_ATTRIBUTE( dynValue )
	/*~ attribute ClipGUIShader.slant
	 *	@components{ client, tools }
	 *
	 *	The slant value offers support for clipping gui components where the cut
	 *	needs to be on a slant ( for example a slanted health bar ).  It only
	 *	works on simple gui components ( i.e. not text nor mesh components ).
	 *
	 *	The slant is a percentage of the component's width , and represents
	 *	the x-offset between the top and the bottom of the component.  If
	 *	your component's texture has a backwards slope ( like a backslash )
	 *	then set the slant to a negative value.
	 */
	PY_ATTRIBUTE( slant )
PY_END_ATTRIBUTES()

/*~ function GUI.ClipShader
 *	@components{ client, tools }
 *
 *	This function creates a new ClipGUIShader.  This is used to clip the
 *  specified portion off one side of the component.
 *
 *	@return		a new ClipGUIShader.
 */
PY_FACTORY_NAMED( ClipGUIShader, "ClipShader", GUI )

// Mode map static
PY_ENUM_MAP( ClipGUIShader::Mode )
PY_ENUM_CONVERTERS_CONTIGUOUS( ClipGUIShader::Mode )


SHADER_FACTORY( ClipGUIShader )


/**
 *	Constructor
 */
ClipGUIShader::ClipGUIShader( Mode clipMode, PyTypeObject * pType )
:GUIShader( pType ),
 clipMode_( clipMode ),
 dynElement_( 0 ),
 slant_( 0.f )
{
	BW_GUARD;
	constants_[0] = 1.f;
	constants_[1] = 0.f;
	constants_[2] = 0.f;
	currentT_ = 1.f;
	oldT_ = 1.f;
	timer_ = 0.f;
}

/**
 *	Destructor
 */
ClipGUIShader::~ClipGUIShader()
{
}


/**
 *	Static python factory method
 */
PyObject * ClipGUIShader::pyNew( PyObject * args )
{
	BW_GUARD;
	char * clipModeStr = NULL;
	if (!PyArg_ParseTuple( args, "|s", &clipModeStr ))
	{
		PyErr_SetString( PyExc_TypeError, "GUI.ClipShader: "
			"Argument parsing error: Expected an optional clip mode string" );
		return NULL;
	}

	ClipGUIShader * cgs = new ClipGUIShader( CLIP_RIGHT );

	if (clipModeStr != NULL)
	{
		bool isOK = cgs->pySetAttribute( ScriptString::create( "mode" ),
			ScriptString::create( clipModeStr ) );
		if (!isOK)
		{
			Py_DECREF( cgs );
			return NULL;
		}
	}

	return cgs;
}




/**
 *	This method processes a GUI component, applying this shader to its vertices
 *
 *	@param component	The component to process
 *	@param dTime		The delta time for the current frame of the application
 */
bool
ClipGUIShader::processComponent( SimpleGUIComponent& component, float dTime )
{
	BW_GUARD;
	int nVerts;
	GUIVertex* verts = component.vertices( &nVerts );

	GUIVertex min, max;
	min = verts[0];
	max = verts[0];

    //calculate the current t-value
	float dt;
	if ( constants_[2] > 0.f )
	{
		dt = 0.f;
		constants_[2] -= dTime;
		if (constants_[2] < 0.f)
			constants_[2] = 0.f;
	}
	else
		timer_ += dTime;

	if ( constants_[1] > 0.f )
		dt = timer_ / constants_[1];
	else
		dt = 1.f;
	if ( dt > 1.f )
		dt = 1.f;
	if ( dt < 0.f )
		dt = 0.f;

	float val = constants_[0];
	if (dynValue_)
	{
		Vector4 dv;
		dynValue_->output( dv );
		val += dv[ dynElement_ & 3 ];
	}
	currentT_ = oldT_ + ( ( val - oldT_ ) * dt );

	bool slant = ( fabsf(slant_) > 0.f );
	if ( slant )
	{
		if (nVerts == 4)
			return this->processComponentWithSlant( component, dTime );
		else
			WARNING_MSG( "ClipGUIShader::processComponent - slant only implemented for gui components with 4 vertices ... ignoring slant\n" );
	}

	//find the min/max of the vertices
	for( int i = 1; i < nVerts; i++ )
	{
		if ( verts[i].pos_.x < min.pos_.x )
			min.pos_.x = verts[i].pos_.x;
		if ( verts[i].pos_.y < min.pos_.y )
			min.pos_.y = verts[i].pos_.y;
		if ( verts[i].uv_[0] < min.uv_[0] )
			min.uv_[0] = verts[i].uv_[0];
		if ( verts[i].uv_[1] < min.uv_[1] )
			min.uv_[1] = verts[i].uv_[1];

		if ( verts[i].pos_.x > max.pos_.x )
			max.pos_.x = verts[i].pos_.x;
		if ( verts[i].pos_.y > max.pos_.y )
			max.pos_.y = verts[i].pos_.y;
		if ( verts[i].uv_[0] > max.uv_[0] )
			max.uv_[0] = verts[i].uv_[0];
		if ( verts[i].uv_[1] > max.uv_[1] )
			max.uv_[1] = verts[i].uv_[1];
	}

	//apply the clipping value ( currentT_ ) to the vertices.

	//note : this code assumes that the te.uv_[0]re coordinates
	//go from left to right.  A grave error, since you can
	//use ROT_90 ... on the simple gui component's angle property.
	switch ( clipMode_ )
	{
	case CLIP_RIGHT:
		for( int i = 0; i < nVerts; i++ )
		{
			float maxX = min.pos_.x + currentT_ * ( max.pos_.x - min.pos_.x );
			float maxU = min.uv_[0] + currentT_ * ( max.uv_[0] - min.uv_[0] );
			if ( verts[i].pos_.x > maxX )
			{
				verts[i].pos_.x = maxX;
				verts[i].uv_[0] = maxU;
			}
		}
		break;
	case CLIP_LEFT:
		for( int i = 0; i < nVerts; i++ )
		{
			float minX = max.pos_.x - currentT_ * ( max.pos_.x - min.pos_.x );
			float minU = max.uv_[0] - currentT_ * ( max.uv_[0] - min.uv_[0] );
			if ( verts[i].pos_.x < minX )
			{
				verts[i].pos_.x = minX;
				verts[i].uv_[0] = minU;
			}
		}
		break;
	default:
		break;
	}

	return true;
}


/**
 *	This method processes a GUI component, applying this shader to its vertices
 *	This method is only called when a slant is in effect on the shader.
 *
 *	@param component	The component to process
 *	@param dTime		The delta time for the current frame of the application
 */
bool
ClipGUIShader::processComponentWithSlant( SimpleGUIComponent& component, float dTime )
{
	BW_GUARD;
	int nVerts;
	GUIVertex* verts = component.vertices( &nVerts );

	float uRange = verts[3].uv_[0] - verts[0].uv_[0];
	float width = verts[3].pos_.x - verts[0].pos_.x;

	GUIVertex* upper;
	GUIVertex* lower;

	float slant = slant_;
	if ( slant > 0.f )
	{
		lower = &verts[2];
		upper = &verts[3];
	}
	else
	{
		slant = -slant;
		lower = &verts[3];
		upper = &verts[2];
	}

	float t = ( currentT_ * (1.f - slant) );

	switch ( clipMode_ )
	{
	case CLIP_RIGHT:
		upper->pos_.x = verts[0].pos_.x + (t+slant) * width;
		lower->pos_.x = verts[0].pos_.x + t * width;

		upper->uv_[0] = verts[0].uv_[0] + (t+slant) * uRange;
		lower->uv_[0] = verts[0].uv_[0] + t * uRange;
		break;
	case CLIP_LEFT:
		upper->pos_.x = verts[3].pos_.x - t * width;
		lower->pos_.x = verts[3].pos_.x - (t+slant) * width;

		upper->uv_[0] = verts[3].uv_[0] - t * uRange;
		lower->uv_[0] = verts[3].uv_[0] - (t+slant) * uRange;
		break;
	default:
		break;
	}

	return true;
}


/**
 *	Load method
 */
bool ClipGUIShader::load( DataSectionPtr pSect )
{
	BW_GUARD;
	if (!GUIShader::load( pSect )) return false;

	clipMode_ = Mode( pSect->readInt( "mode", int(clipMode_) ) );
	constants_[0] = pSect->readFloat( "value", constants_[0] );
	constants_[1] = pSect->readFloat( "speed", constants_[1] );
	constants_[2] = pSect->readFloat( "delay", constants_[2] );
	slant_ = pSect->readFloat( "slant", slant_ );

	currentT_ = constants_[0];
	this->touch();

	return true;
}


/**
 *	Save method
 */
void ClipGUIShader::save( DataSectionPtr pSect )
{
	BW_GUARD;
	GUIShader::save( pSect );

	pSect->writeInt( "mode", int(clipMode_) );
	pSect->writeFloat( "value", constants_[0] );
	pSect->writeFloat( "speed", constants_[1] );
	pSect->writeFloat( "delay", constants_[2] );
	pSect->writeFloat( "slant", slant_ );
}

BW_END_NAMESPACE

// clip_gui_shader.cpp
