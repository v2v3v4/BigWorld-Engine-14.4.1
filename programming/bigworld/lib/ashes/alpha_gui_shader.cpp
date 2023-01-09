#include "pch.hpp"

#include "cstdmf/debug.hpp"

#include "simple_gui_component.hpp"
#include "alpha_gui_shader.hpp"
#include "pyscript/script.hpp"
#include "math/colour.hpp"

#ifndef CODE_INLINE
#include "alpha_gui_shader.ipp"
#endif

DECLARE_DEBUG_COMPONENT2( "2DComponents", 0 );

BW_BEGIN_NAMESPACE

#undef PY_ATTR_SCOPE
#define PY_ATTR_SCOPE AlphaGUIShader::

PY_RW_ATTRIBUTE_DECLARE( fadeMode_, mode )

PY_TYPEOBJECT( AlphaGUIShader )

PY_BEGIN_METHODS( AlphaGUIShader )
	PY_METHOD( reset )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( AlphaGUIShader )
	/*~ attribute AlphaGUIShader.mode
	 *	@components{ client, tools }
	 *
	 *	An AlphaGUIShader can apply an alpha gradient, which is where the
	 *	alpha applied to the component varies linearly either horizontally
	 *	or vertically.
	 *
	 *	The mode attribute specifies which direction the alpha gradient runs in.
	 *	It can take two settings:
	 *	
	 *	- "ALL" - There is no gradient.  The entire component has the alpha
	 *	specified by the alpha attribute.
	 *
	 *	- "RIGHT" - The gradient starts from the alpha attribute on the 
	 *	right and fades to zero on the left.  The alpha gradient begins 
	 *	at the location specified by the start attribute or the edge of 
	 *	the affected GUI component(s), whichever occurs first.  The 
	 *	gradient then linearly decreases to the point specified by the 
	 *	stop attribute, or the edge of the affected GUI component(s), 
	 *	whichever occurs last.  In this mode, start should always be a 
	 *	higher value than stop.
	 *
	 *	@type String
	 */

	 /* These are not yet implemented...
	 *	- "LEFT" - The gradient starts from the alpha attribute on the 
	 *	left and fades to zero on the right.  The alpha gradient begins 
	 *	at the location specified by the start attribute or the edge of 
	 *	the affected GUI component(s), whichever occurs first.  The 
	 *	gradient then linearly decreases to the point specified by the 
	 *	stop attribute, or the edge of the affected GUI component(s), 
	 *	whichever occurs last.  In this mode, start should always be a 
	 *	lower value than stop.
	 *
	 *	- "UP" - The gradient starts from the alpha attribute on the 
	 *	top and fades to zero on the bottom.  The alpha gradient begins 
	 *	at the location specified by the start attribute or the edge of 
	 *	the affected GUI component(s), whichever occurs first.  The 
	 *	gradient then linearly decreases to the point specified by the 
	 *	stop attribute, or the edge of the affected GUI component(s), 
	 *	whichever occurs last.  In this mode, start should always be a 
	 *	higher value than stop.
	 *
	 *	- "DOWN" - The gradient starts from the alpha attribute on the 
	 *	bottom and fades to zero on the top.  The alpha gradient begins 
	 *	at the location specified by the start attribute or the edge of 
	 *	the affected GUI component(s), whichever occurs first.  The 
	 *	gradient then linearly decreases to the point specified by the 
	 *	stop attribute, or the edge of the affected GUI component(s), 
	 *	whichever occurs last.  In this mode, start should always be a 
	 *	lower value than stop.
	 *	
	 *	@type String
	 */
	PY_ATTRIBUTE( mode )
	/*~ attribute AlphaGUIShader.stop
	 *	@components{ client, tools }
	 *
	 *	The stop attribute specifies the point on the screen (measured in 
	 *	clip coordinates, with (-1,-1) being the bottom left and (1,1) being
	 *	the top right), at which the alpha gradient will be zero.
	 *	
	 *	It is a float, and is interpreted as either the x or y component of a
	 *	Vector2, depending on mode.
	 *
	 *	@type Float
	 */
	PY_ATTRIBUTE( stop )
	/*~ attribute AlphaGUIShader.start
	 *	@components{ client, tools }
	 *
	 *	The start attribute specifies the point on the screen (measured in 
	 *	clip coordinates, with (-1,-1) being the bottom left and (1,1) being
	 *	the top right), at which the alpha gradient will be the value 
	 *	specified by the alpha attribute.
	 *	
	 *	It is a float, and is interpreted as either the x or y component of a
	 *	Vector2, depending on mode.
	 *
	 *	@type Float
	 */
	PY_ATTRIBUTE( start )
	/*~ attribute AlphaGUIShader.alpha
	 *	@components{ client, tools }
	 *
	 *	The alpha attribute specifies the alpha value for the component.  
	 *
	 *	If the mode attribute is anything other than "ALL", then this will be
	 *	the upper limit for the gradient, which fades to zero over the
	 *	width/height specified by the start and stop attributes.  
	 *	
	 *	If the speed attribute is non-zero,	then the currentAlpha attribute
	 *	will move from its current value to alpha over speed seconds, in a
	 *	linear fashion.  The currentAlpha attribute specifies what alpha is
	 *	actually applied to the component at this moment in time.
	 *
	 *	This attribute is synonymous with the value attribute.
	 *
	 *  @see mode
	 *	@type Float
	 */
	PY_ATTRIBUTE( alpha )
	/*~ attribute AlphaGUIShader.value
	 *	@components{ client, tools }
	 *
	 *	The value attribute specifies the alpha value for the component.  
	 *
	 *	If the mode attribute is anything other than "ALL", then this will be
	 *	the upper limit for the gradient, which fades to zero over the
	 *	width/height specified by the start and stop attributes.
	 *	
	 *	If the speed attribute is non-zero,	then the currentAlpha attribute
	 *	will move from its current value to alpha over speed seconds, in a
	 *	linear fashion.  The currentAlpha attribute specifies what alpha is
	 *	actually applied to the component at this moment in time.
	 *
	 *	This attribute is synonymous with the alpha attribute.
	 *
	 *  @see mode
	 *	@type Float
	 */
	PY_ATTRIBUTE( value )
	/*~ attribute AlphaGUIShader.speed
	 *	@components{ client, tools }
	 *
	 *	The speed attribute is the time in seconds that it takes the
	 *	currentAlpha attribute to move from its current value to the new alpha
	 *	value, when either the alpha attribute or the value attribute is set.
	 *
	 *	If speed is zero, then the assignment is instantaneous.
	 *
	 *	@type Float
	 */
	PY_ATTRIBUTE( speed )
	/*~ attribute AlphaGUIShader.currentAlpha
	 *	@components{ client, tools }
	 *
	 *	This attribute is the instantaneous value of alpha that is applied to
	 *	the component.  If speed is non-zero then, when a new value is assigned
	 *	to the alpha or value attributes, currentValue will move linearly over
	 *	speed seconds, from	whatever value it had at the moment of assignment
	 *	to the newly assigned value.
	 *
	 *	@type Float
	 */
	PY_ATTRIBUTE( currentAlpha )
PY_END_ATTRIBUTES()

/*~ function GUI.AlphaShader
 *	@components{ client, tools }
 *
 *	This function creates a new AlphaGUIShader.  An AlphaGUIShader is used to
 *	control the alpha on a GUI component and its children.
 *
 *	@return		a newly created AlphaGUIShader.
 */
PY_FACTORY_NAMED( AlphaGUIShader, "AlphaShader", GUI )

PY_ENUM_MAP( AlphaGUIShader::Mode )
PY_ENUM_CONVERTERS_CONTIGUOUS( AlphaGUIShader::Mode )


SHADER_FACTORY( AlphaGUIShader )


/**
 *	Constructor
 */
AlphaGUIShader::AlphaGUIShader( Mode fadeMode, PyTypeObject * pType )
:GUIShader( pType ),
 fadeMode_( fadeMode ),
 timer_( 0.f ),
 currentAlpha_( 1.f ),
 oldAlpha_( 1.f )
{
	BW_GUARD;
	constants_[0] = -1.f;
	constants_[1] = 1.f;
	constants_[2] = 1.f;
	constants_[3] = 0.f;
}


/**
 *	Destructor
 */
AlphaGUIShader::~AlphaGUIShader()
{
	BW_GUARD;	
}


/**
 *	Static python factory method
 */
PyObject * AlphaGUIShader::pyNew( PyObject * args )
{
	BW_GUARD;
	char * fadeModeStr = NULL;
	if (!PyArg_ParseTuple( args, "|s", &fadeModeStr ))
	{
		PyErr_SetString( PyExc_TypeError, "GUI.AlphaShader: "
			"Argument parsing error: Expected an optional fade mode string" );
		return NULL;
	}

	AlphaGUIShader * ags = new AlphaGUIShader( FADE_ALL );

	if (fadeModeStr != NULL )
	{
		bool isOK = ags->pySetAttribute( ScriptString::create( "mode" ),
			ScriptString::create( fadeModeStr ) );
		if (!isOK) // exception set by pySetAttribute
		{
			Py_DECREF( ags );
			return NULL;
		}
	}

	return ags;
}



/**
 *	This method processes a GUI component, applying this shader to its vertices
 *
 *	@param component	The component to process
 *	@param dTime		The delta time for the current frame of the application
 */
bool
AlphaGUIShader::processComponent( SimpleGUIComponent& component, float dTime )
{
	BW_GUARD;
	int nVerts;
	GUIVertex* verts = component.vertices( &nVerts );

	//calculate the current t-value
	timer_ += dTime;
	float dt;
	if ( constants_[3] > 0.f )
		dt = timer_ / constants_[3];
	else
		dt = 1.f;
	dt = Math::clamp( 0.f, dt, 1.f );	

	//current alpha is the alpha T-value
	currentAlpha_ = Math::clamp( 0.f, oldAlpha_ + ( ( constants_[2] - oldAlpha_ ) * dt ), 1.f );	

    int i;

	switch ( fadeMode_ )
	{
	case FADE_ALL:
		{			
			Vector4 newCol = Colour::getVector4Normalised( component.runTimeColour() );
			newCol.w *= currentAlpha_;
			component.runTimeColour( Colour::getUint32FromNormalised(newCol) );
			if (newCol.w == 0.f)
			{
				component.momentarilyInvisible( true );
			}			
		}
		break;
	case FADE_RIGHT:
		//componentAlpha is the current alpha T-value multiplied by the component's
		//actual alpha ( set in the colour property )
		{
			float origAlpha = (float)( ( component.runTimeColour() & 0xff000000 ) >> 24 );
			origAlpha /= 255.f;
			float componentAlpha = Math::clamp( 0.f, currentAlpha_ * origAlpha, origAlpha );		

			for( i = 0; i < nVerts; i++ )
			{
				float t;

				if ( verts[i].pos_.x > constants_[1] )
					t = 1.f;
				else if ( verts[i].pos_.x < constants_[0] )
					t = 0.f;
				else
					t = ( verts[i].pos_.x - constants_[0] ) / ( constants_[1] - constants_[0] );

				//scale the componentAlpha by the position
				float fAlpha = componentAlpha * t;

				int alpha = (int)( fAlpha  * 255.f );
				verts[i].colour_ = verts[i].colour_ & 0x00ffffff;
				verts[i].colour_ = verts[i].colour_ | ( alpha << 24 );
			}
		}
		break;
	default:
		break;
	}

	return true;
}


/**
 *	Load method
 */
bool AlphaGUIShader::load( DataSectionPtr pSect )
{
	BW_GUARD;
	if (!GUIShader::load( pSect )) return false;

	fadeMode_ = Mode( pSect->readInt( "mode", int(fadeMode_) ) );
	constants_[0] = pSect->readFloat( "stop", constants_[0] );
	constants_[1] = pSect->readFloat( "start", constants_[1] );
	constants_[2] = pSect->readFloat( "alpha", constants_[2] );
	constants_[3] = pSect->readFloat( "speed", constants_[3] );

	currentAlpha_ = constants_[2];
	this->touch();

	return true;
}


/**
 *	Save method
 */
void AlphaGUIShader::save( DataSectionPtr pSect )
{
	BW_GUARD;
	GUIShader::save( pSect );

	pSect->writeInt( "mode", int(fadeMode_) );
	pSect->writeFloat( "stop", constants_[0] );
	pSect->writeFloat( "start", constants_[1] );
	pSect->writeFloat( "alpha", constants_[2] );
	pSect->writeFloat( "speed", constants_[3] );
}

BW_END_NAMESPACE

// alpha_gui_shader.cpp
