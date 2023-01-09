#ifndef COLOUR_GUI_SHADER_HPP
#define COLOUR_GUI_SHADER_HPP

#include "gui_shader.hpp"
#include "math/vector4.hpp"
#include "math/linear_animation.hpp"
#include "pyscript/script_math.hpp"


BW_BEGIN_NAMESPACE

/*~ class GUI.ColourGUIShader
 *	@components{ client, tools }
 *
 *	This class modifies the colours of the GUI components it is applied to.  It
 *  is a subclass of GUIShader.
 *
 *	There are two ways it can specify which colour to shade the components.
 *	The first is to have a ColourProvider (which is a Vector4Provider, with
 *	its components interpreted as ( red, green, blue, alpha ),
 *	assigned to its colourProvider attribute.  If this is not None it
 *	overrides the interpolation.
 *	
 *	The second is to interpolate between the colour specified by
 *  its start, middle and end attributes.  Interpolation is based on the value
 *  attribute.  If value is between 0.0 and 0.5, interpolation is between
 *	start and middle.  If it is from 0.5 to 1.0, it is between middle and end.
 *	
 *	A new ColourGUIShader is created using the GUI.ColourShader function.
 */
/**
 *	This class is a GUI shader that shades the colours of the
 *	components it is applied to
 */
class ColourGUIShader : public GUIShader
{
	Py_Header( ColourGUIShader, GUIShader )

public:

	ColourGUIShader( PyTypeObject * pType = &s_type_ );
	~ColourGUIShader();

	PY_FACTORY_DECLARE()

	PY_READABLE_ATTRIBUTE_GET( constants_[0], start )
	PY_WRITABLE_ACCESSOR_ATTRIBUTE_SET( Vector4, start, start )

	PY_READABLE_ATTRIBUTE_GET( constants_[1], middle )
	PY_WRITABLE_ACCESSOR_ATTRIBUTE_SET( Vector4, middle, middle )

	PY_READABLE_ATTRIBUTE_GET( constants_[2], end )
	PY_WRITABLE_ACCESSOR_ATTRIBUTE_SET( Vector4, end, end )

	PY_READABLE_ATTRIBUTE_GET( desiredT_, value )
	PY_WRITABLE_ACCESSOR_ATTRIBUTE_SET( float, value, value )

	PY_RW_ATTRIBUTE_DECLARE( speed_, speed )

	PY_RW_ATTRIBUTE_DECLARE( colourProvider_, colourProvider )

	///Constants for colour shader :
	/// 1 ) the colour at t = 0
	/// 2 ) the colour at t = 0.5
	/// 3 ) the colour at t = 1
	/// 4 ) the current value
	/// 5 ) the speed ( the time required to move T-value to new value )
	void	constants( const Vector4& colour1, const Vector4& colour2, const Vector4& colour3, float param, float speed = 1.f );
	void	start( const Vector4 & v );
	void	middle( const Vector4 & v );
	void	end( const Vector4 & v );
	void	value( float v );

	Vector4	currentColour();

	bool	processComponent( SimpleGUIComponent& component, float dTime );

	void	reset();
	PY_AUTO_METHOD_DECLARE( RETVOID, reset, END )
	PY_READABLE_ATTRIBUTE_GET( this->currentColour(), currentValue )

private:
	ColourGUIShader(const ColourGUIShader&);
	ColourGUIShader& operator=(const ColourGUIShader&);

	void	resetAnimation();

	Vector4	constants_[3];
	float	desiredT_;
	float	speed_;
	float	currentT_;
	float	oldT_;
	float	timer_;
	LinearAnimation<Vector4>	animation_;

	Vector4ProviderPtr colourProvider_;

	virtual bool	load( DataSectionPtr pSect );
	virtual void	save( DataSectionPtr pSect );

	SHADER_FACTORY_DECLARE( ColourGUIShader() )
};
BW_END_NAMESPACE

#ifdef CODE_INLINE
#include "colour_gui_shader.ipp"
#endif


#endif // COLOUR_GUI_SHADER_HPP
