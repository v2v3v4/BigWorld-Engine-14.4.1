// clip_gui_shader.ipp

#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif

BW_BEGIN_NAMESPACE

INLINE const float* ClipGUIShader::getConstants( void ) const
{
	return &constants_[0];
}


INLINE void ClipGUIShader::constants( float t, float speed )
{
	constants_[0] = t;
	constants_[1] = speed;
	this->touch();
}


INLINE void ClipGUIShader::value( float v )
{
	constants_[0] = v;
	this->touch();
}

INLINE void ClipGUIShader::speed( float t )
{
	constants_[1] = t;
	this->touch();
}

INLINE void ClipGUIShader::delay( float d )
{
	constants_[2] = d;
	this->touch();
}

INLINE void ClipGUIShader::touch()
{
	oldT_ = currentT_;
	timer_ = 0.f;
}

/*~ function ClipGUIShader.reset
 *	@components{ client, tools }
 *
 *	This function forces current size of the clip rectangle to move instantly
 *  to the size specified by the
 *	value attribute
 *
 */
INLINE void	ClipGUIShader::reset()
{
	oldT_ = constants_[0];
	currentT_ = constants_[0];
	timer_ = 0.f;
}

BW_END_NAMESPACE

// clip_gui_shader.ipp