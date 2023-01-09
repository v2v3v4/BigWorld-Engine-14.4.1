#ifndef ALPHA_GUI_SHADER_HPP
#define ALPHA_GUI_SHADER_HPP


#include "gui_shader.hpp"


BW_BEGIN_NAMESPACE

/*~ class GUI.AlphaGUIShader
 *	@components{ client, tools }
 *
 *	An AlphaGUIShader is used to change the alpha values of the components 
 *	(instances of SimpleGUIComponent or its subclasses) it
 *	is applied to.  AlphaGUIShader is a subclass of GUIShader.
 *
 *	It can apply alpha either evenly over the component, or fading from one
 *	edge towards the opposite edge.
 *
 *	A new AlphaGUIShader is created using the GUI.AlphaShader function.
 *
 *	For example:
 *	@{
 *	# create a component to apply the shader to
 *	comp = GUI.Simple( "maps/scroll.dds" )
 *	GUI.addRoot( comp )
 *
 *	# now create the shader.  It will be fully transparent on the left
 *	# and fully opaque on the right.
 *	shad = GUI.AlphaShader()
 *	shad.start = -0.5
 *	shad.stop = 0.5
 *	shad.mode = "RIGHT"
 *	shad.alpha = 1.0
 *
 *	# now add the shader to the component
 *	comp.addShader( shad )
 *	@}
 */
/**
 *	This class is a GUI shader that changes the alpha values of the
 *	components it is applied to
 */
class AlphaGUIShader : public GUIShader
{
	Py_Header( AlphaGUIShader, GUIShader )

public:
	enum Mode
	{
		FADE_ALL,
		FADE_RIGHT,
		FADE_UP,
		FADE_DOWN,
		FADE_LEFT
	};
	PY_BEGIN_ENUM_MAP( Mode, FADE_ )
		PY_ENUM_VALUE( FADE_ALL )
		PY_ENUM_VALUE( FADE_RIGHT )
		PY_ENUM_VALUE( FADE_UP )
		PY_ENUM_VALUE( FADE_DOWN )
		PY_ENUM_VALUE( FADE_LEFT )
	PY_END_ENUM_MAP()

	AlphaGUIShader( Mode fadeMode, PyTypeObject * pType = &s_type_ );
	~AlphaGUIShader();

	PY_FACTORY_DECLARE()

	PY_DEFERRED_ATTRIBUTE_DECLARE( mode )

	PY_READABLE_ATTRIBUTE_GET( constants_[0], stop )
	PY_WRITABLE_ACCESSOR_ATTRIBUTE_SET( float, stop, stop )
	PY_READABLE_ATTRIBUTE_GET( constants_[1], start )
	PY_WRITABLE_ACCESSOR_ATTRIBUTE_SET( float, start, start )
	PY_READABLE_ATTRIBUTE_GET( constants_[2], alpha )
	PY_WRITABLE_ACCESSOR_ATTRIBUTE_SET( float, value, alpha )
	PY_READABLE_ATTRIBUTE_GET( constants_[2], value )
	PY_WRITABLE_ACCESSOR_ATTRIBUTE_SET( float, value, value )

	PY_RW_ATTRIBUTE_DECLARE( constants_[3], speed )
	PY_RO_ATTRIBUTE_DECLARE( currentAlpha_, currentAlpha )

	///constants are
	/// t1 : the clip position where the alpha blend starts
	/// t2 : the clip position where the alpha blend ends
	/// t3 : the maximum alpha value
	/// speed : the speed at which a transition to a new alpha value occurs
	void	constants( float t1, float t2, float alpha, float speed = 0.f );
	void	stop( float t );
	void	start( float t );
	void	value( float v );

	///This method returns the current alpha T-value.
	///It is a value between 0 and 1, representing the current position
	///between 0.f and the alpha set in the component's colour property
	float	currentAlphaParam( void )	{ return currentAlpha_; }
	///This method resets the shader to the constant alpha value.
	void	reset();
	PY_AUTO_METHOD_DECLARE( RETVOID, reset, END )

	bool	processComponent( SimpleGUIComponent& component, float dTime );

private:
	AlphaGUIShader(const AlphaGUIShader&);
	AlphaGUIShader& operator=(const AlphaGUIShader&);

	void touch();

	Mode	fadeMode_;
	float	constants_[4];
	float	currentAlpha_;
	float	oldAlpha_;
	float	timer_;

	virtual bool	load( DataSectionPtr pSect );
	virtual void	save( DataSectionPtr pSect );

	SHADER_FACTORY_DECLARE( AlphaGUIShader( FADE_ALL ) )
};

/// declare the enum converter functions
PY_ENUM_CONVERTERS_DECLARE( AlphaGUIShader::Mode )


BW_END_NAMESPACE

#ifdef CODE_INLINE
#include "alpha_gui_shader.ipp"
#endif

#endif // ALPHA_GUI_SHADER_HPP
