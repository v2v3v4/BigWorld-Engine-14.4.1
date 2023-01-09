#ifndef CLIP_GUI_SHADER_HPP
#define CLIP_GUI_SHADER_HPP


#include "gui_shader.hpp"
#include "pyscript/script_math.hpp"


BW_BEGIN_NAMESPACE

/*~ class GUI.ClipGUIShader
 *	@components{ client, tools }
 *
 *	This GUIShader clips the components that it is applied to.  It clips a
 *	component to a rectangle that has one dimension	the same as the component's
 *  rectangle, and the other dimension having one side that lines up with the
 *  corresponding component edge, and the other side having been scaled in from
 *  the component edge by the value attribute.  Which edge is scaled is
 *  determined by the mode attribute.
 *
 *	This can be useful for things like health bars, which need to be dynamically
 *	clipped.
 *
 *	For example, in the following code:
 *
 *	@{
 *	comp = GUI.Simple( "maps/gui/scroll.dds" )
 *	GUI.addRoot( comp )
 *	shad = GUI.ClipShader()
 *	comp.addShader( shad )
 *	
 *	shad.mode	= "RIGHT"
 *	shad.value	= 0.75
 *	shad.speed  = 10.0
 *	shad.value	= 0.25
 *	@}
 *
 *	This example will start with the leftmost three quarters of the component 
 *	visible and the rightmost quarter clipped off.  Over 10 seconds it 
 *	will linearly clip more and more until only the leftmost quarter is visible.
 *
 *	A ClipGUIShader is created using the GUI.ClipShader function.
 */
/**
 *	This class is a GUI shader that clips the components it is applied to
 */
class ClipGUIShader : public GUIShader
{
	Py_Header( ClipGUIShader, GUIShader )

public:
	enum Mode
	{
		CLIP_RIGHT,
		CLIP_UP,
		CLIP_DOWN,
		CLIP_LEFT
	};
	PY_BEGIN_ENUM_MAP( Mode, CLIP_ )
		PY_ENUM_VALUE( CLIP_RIGHT )
		PY_ENUM_VALUE( CLIP_UP )
		PY_ENUM_VALUE( CLIP_DOWN )
		PY_ENUM_VALUE( CLIP_LEFT )
	PY_END_ENUM_MAP()


	ClipGUIShader( Mode clipMode, PyTypeObject * pType = &s_type_ );
	~ClipGUIShader();

	PY_FACTORY_DECLARE()

	PY_DEFERRED_ATTRIBUTE_DECLARE( mode )

	PY_READABLE_ATTRIBUTE_GET( constants_[0], value )
	PY_WRITABLE_ACCESSOR_ATTRIBUTE_SET( float, value, value )
	PY_READABLE_ATTRIBUTE_GET( constants_[1], speed )
	PY_WRITABLE_ACCESSOR_ATTRIBUTE_SET( float, speed, speed )

	PY_READABLE_ATTRIBUTE_GET( constants_[2], delay )
	PY_WRITABLE_ACCESSOR_ATTRIBUTE_SET( float, delay, delay )

	PY_RW_ATTRIBUTE_DECLARE( dynValue_, dynValue )
	PY_RW_ATTRIBUTE_DECLARE( dynElement_, dynElement )
	PY_RW_ATTRIBUTE_DECLARE( slant_, slant )

	///Constants for clip shader :
	/// 1 ) the desired T-value ( the amount of the vertices left after clipping )
	/// 2 ) the time ( the time required to move T-value to new value )
	const float* getConstants( void ) const;
	void	constants( float param, float paramPerSeconds = 1.f );
	void	value( float v );
	void	speed( float t );
	void	delay( float t );

	float	currentValue() const	{ return currentT_; }

	///This method resets the shader to the constant alpha value.
	void	reset();
	PY_AUTO_METHOD_DECLARE( RETVOID, reset, END )
	PY_RO_ATTRIBUTE_DECLARE( currentT_, currentValue )

	bool	processComponent( SimpleGUIComponent& component, float dTime );

private:
	ClipGUIShader(const ClipGUIShader&);
	ClipGUIShader& operator=(const ClipGUIShader&);

	bool	processComponentWithSlant( SimpleGUIComponent& component, float dTime );

	void	touch();

	Mode	clipMode_;
	float	constants_[3];

	float	currentT_;
	float	oldT_;
	float	timer_;
	float	slant_;

	Vector4ProviderPtr	dynValue_;
	uint				dynElement_;

	virtual bool	load( DataSectionPtr pSect );
	virtual void	save( DataSectionPtr pSect );

	SHADER_FACTORY_DECLARE( ClipGUIShader( CLIP_RIGHT ) )
};

PY_ENUM_CONVERTERS_DECLARE( ClipGUIShader::Mode )

BW_END_NAMESPACE

#ifdef CODE_INLINE
#include "clip_gui_shader.ipp"
#endif


#endif // CLIP_GUI_SHADER_HPP
