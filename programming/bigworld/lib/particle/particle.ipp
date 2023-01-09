#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif


//constants for packing velocity into 16 bits
const float VELOCITY_MIN_DATA	= -32768.0f;
const float VELOCITY_MAX_DATA	= +32767.0f;

//constants for packing and unpacking size data into 16 bits
const float SIZE_MAX_DATA		= 65535.f;
const float SIZE_MAX_VALUE		= 100.f;
const float PACK_SIZE			= SIZE_MAX_DATA / SIZE_MAX_VALUE;
const float UNPACK_SIZE			= SIZE_MAX_VALUE / SIZE_MAX_DATA;
const float PACK_MESH_SPIN      = SIZE_MAX_DATA / SIZE_MAX_VALUE;
const float UNPACK_MESH_SPIN    = SIZE_MAX_VALUE / SIZE_MAX_DATA;

//constants for packing and unpacking age data into 16 bits
//split up the age value into increments of 1/120 seconds.
//note this gives a maximum particle age of roughly 6 minutes,
//with enough resolution to handle running the game at 120 fps.
const float AGE_MIN_INCREMENT	= 1.f / 120.f;
const float AGE_MAX_DATA		= 65535.f;
const uint16 AGE_MAX_DATA_UINT	= 0xffff;
const float AGE_MAX_VALUE		= AGE_MAX_DATA * AGE_MIN_INCREMENT;
const float PACK_AGE			= AGE_MAX_DATA / AGE_MAX_VALUE;
const float UNPACK_AGE			= AGE_MAX_VALUE / AGE_MAX_DATA;

//constants for packing and unpacking pitch/yaw rotational data
const float ROT_MAX_DATA		= 256.f;
const float ROT_MAX_VALUE		= MATH_2PI;
const float PACK_ROT			= ROT_MAX_DATA / ROT_MAX_VALUE;
const float UNPACK_ROT			= ROT_MAX_VALUE / ROT_MAX_DATA;
const float	ABS_ROT				= MATH_2PI * 10.f;
const float	ROUND_ROT			= MATH_2PI / ROT_MAX_DATA;
const float SPIN_DIR_MAX_DATA	= 255.0f;
const float SPIN_SPEED_MAX_DATA = 255.0f;
const float SPIN_ANGLE_MAX_DATA = 65535.0f;

INLINE Particle::Particle
( 
    const Vector3   &position,
	const Vector3   &v,
	const Vector4   &colour,
	float           sz,
	float           startingPitch /*= 0.f*/,
	float           startingYaw /*= 0.f*/,
	float           startingAge /*= 0.0f*/ 
) 
{
	positionColour_.position_ = position;
	positionColour_.colour_ = Colour::getUint32FromNormalised(colour);
    extraData_.nonMeshData_.idx_      = 0;
    extraData_.nonMeshData_.distance_ = 0;
	this->setVelocity( v );
	this->size( sz );
	this->age( startingAge );
	this->pitch( startingPitch );
	this->yaw( startingYaw );
}


INLINE Particle::Particle
( 
    const Vector3   &position,
	const Vector3   &v,
	float           startingPitch       /*= 0.0f*/,
	float           startingYaw         /*= 0.0f*/,	
    const           Vector3 &spinAxis    /*= Vector3()*/,
    float           spinSpeed           /*= 0.0f*/,
    float           startingAge         /*= 0.0f*/
) 
{
	positionColour_.position_ = position;
	positionColour_.colour_ = 0xffffffff;		// solid white
	this->setVelocity( v );
	this->age( startingAge );
	this->pitch( startingPitch );
	this->yaw( startingYaw );
    this->meshSpinAxis(spinAxis);
    this->meshSpinSpeed(spinSpeed);
}


INLINE /*static*/ bool Particle::sortParticlesReverse( const Particle& p1, const Particle& p2 )
{
	return 
        p1.extraData_.nonMeshData_.distance_ 
        > 
        p2.extraData_.nonMeshData_.distance_ ;
}


INLINE /*static*/ size_t Particle::sizeInBytes( void ) 
{ 
    return sizeof(Particle); 
}


INLINE /*static*/ float Particle::sizeRawToFloat( uint16 sizeRaw )
{
	return float( sizeRaw ) * UNPACK_SIZE;
}


INLINE Vector3 const & Particle::position() const
{
    return positionColour_.position_;
}


INLINE Vector3 & Particle::position()
{
    return positionColour_.position_;
}


INLINE void Particle::getVelocity( Vector3& ret ) const
{
	ret.x = float(xVel_)/256.f;
	ret.y = float(yVel_)/256.f;
	ret.z = float(zVel_)/256.f;
}


INLINE void Particle::setVelocity( const Vector3& v ) 
{ 
    xVel_=(int16)(Math::clamp(VELOCITY_MIN_DATA, v.x*256.f, VELOCITY_MAX_DATA)); 
    yVel_=(int16)(Math::clamp(VELOCITY_MIN_DATA, v.y*256.f, VELOCITY_MAX_DATA)); 
    zVel_=(int16)(Math::clamp(VELOCITY_MIN_DATA, v.z*256.f, VELOCITY_MAX_DATA));  
}


INLINE void Particle::setXVelocity( float x ) 
{ 
    xVel_=(int16)(Math::clamp(VELOCITY_MIN_DATA, x*256.f, VELOCITY_MAX_DATA)); 
}


INLINE void Particle::setYVelocity( float y ) 
{ 
	yVel_=(int16)(Math::clamp(VELOCITY_MIN_DATA, y*256.f, VELOCITY_MAX_DATA)); 
}


INLINE void Particle::setZVelocity( float z ) 
{ 
    zVel_=(int16)(Math::clamp(VELOCITY_MIN_DATA, z*256.f, VELOCITY_MAX_DATA)); 
}


INLINE float Particle::size() const
{ 
    return float(extraData_.nonMeshData_.size_) * UNPACK_SIZE; 
}


INLINE void Particle::size( float f ) 
{ 
    extraData_.nonMeshData_.size_ = (uint16)(Math::clamp(0.0f, f*PACK_SIZE, SIZE_MAX_DATA)); 
}


INLINE uint16 Particle::sizeRaw() const
{ 
    return extraData_.nonMeshData_.size_; 
}


INLINE bool Particle::isAlive() const
{
	return (age_ != AGE_MAX_DATA_UINT);
}


INLINE void Particle::kill()
{
	age_ = AGE_MAX_DATA_UINT;
}


INLINE float Particle::age() const	
{ 
    return float(age_) * UNPACK_AGE; 
}


INLINE void Particle::age( float f ) 
{
    age_ = (uint16)Math::clamp( 0.f, f * PACK_AGE + 0.5f , AGE_MAX_DATA );
}


INLINE uint16 Particle::nAgeIncrements( float f )
{
	//purposefully truncating, so there is always a positive error left over
	return (uint16)Math::clamp( 0.f, f * PACK_AGE, AGE_MAX_DATA );
}


INLINE float Particle::age( uint16 accurateAge )
{
	return float(accurateAge) * UNPACK_AGE;
}


INLINE uint16 Particle::ageMax()
{
	return (uint16)AGE_MAX_DATA;
}


INLINE uint16 Particle::ageAccurate() const
{
	return age_;
}


INLINE void Particle::ageAccurate( uint16 a )
{
	//TODO : change following to
	//age_ = min(AGE_MAX_DATA_UINT-1,a)
	age_ = a;
}


INLINE float Particle::pitch() const
{
	return (float)(pitchYaw_>>8) * UNPACK_ROT;
}


INLINE void Particle::pitch( float p )
{
	uint16 pt = ((uint16)(((p+ABS_ROT)*PACK_ROT)+ROUND_ROT) & 0xff) << 8;
	pitchYaw_ = (pitchYaw_ & 0xff) | pt;
}


INLINE float Particle::yaw() const
{
	return (float)(pitchYaw_&0xff) * UNPACK_ROT;
}


INLINE void Particle::yaw( float y )
{
	uint16 yw = (uint16)(((y+ABS_ROT)*PACK_ROT)+ROUND_ROT) & 0xff;
	pitchYaw_ = (pitchYaw_ & 0xff00) | yw;
}


INLINE Vector3 Particle::meshSpinAxis() const
{
    return 
        Vector3
        (
            extraData_.meshData_.spinAxisX_/255.0f,
            extraData_.meshData_.spinAxisY_/255.0f, 
            extraData_.meshData_.spinAxisZ_/255.0f
        );
}


INLINE void Particle::meshSpinAxis( const Vector3 & axis )
{
	extraData_.meshData_.spinAxisX_ = (uint8)(Math::clamp(0.0f, 255.0f*axis.x, SPIN_DIR_MAX_DATA));
    extraData_.meshData_.spinAxisY_ = (uint8)(Math::clamp(0.0f, 255.0f*axis.y, SPIN_DIR_MAX_DATA));
    extraData_.meshData_.spinAxisZ_ = (uint8)(Math::clamp(0.0f, 255.0f*axis.z, SPIN_DIR_MAX_DATA));
}


INLINE float Particle::meshSpinSpeed() const
{
    return extraData_.meshData_.spinSpeed_/255.0f;
}


INLINE void Particle::meshSpinSpeed( float amount )
{
	extraData_.meshData_.spinSpeed_ = (uint8)(Math::clamp(0.0f, 255.0f*amount, SPIN_SPEED_MAX_DATA));
}


INLINE float Particle::meshSpinAge() const
{
    return float(extraData_.meshData_.meshSpinAge_) * UNPACK_MESH_SPIN; 
}


INLINE void Particle::meshSpinAge(float s)
{
	extraData_.meshData_.meshSpinAge_ = (uint16)(Math::clamp(0.0f, s * PACK_MESH_SPIN, SPIN_ANGLE_MAX_DATA)); 
}


INLINE uint32 Particle::colour() const
{
    return positionColour_.colour_;
}


INLINE void Particle::colour(uint32 clr)
{
    positionColour_.colour_ = clr;
}


INLINE Particle::PositionColour* Particle::positionColour()
{
	return &positionColour_;
}


INLINE uint16 Particle::index() const
{
    return extraData_.nonMeshData_.idx_;
}


INLINE void Particle::index(uint16 idx) 
{
    extraData_.nonMeshData_.idx_ = idx;
}


INLINE uint16 Particle::distance() const
{
    return extraData_.nonMeshData_.distance_;
}


INLINE void Particle::distance(uint16 dist)
{
    extraData_.nonMeshData_.distance_ = dist;
}
