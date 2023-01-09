// -----------------------------------------------------------------------------
// Section: Angle compression functions
// -----------------------------------------------------------------------------
namespace
{
/**
 *	This method is used to convert an angle in the range [-pi, pi) into an int
 *	using only a certain number of bits.
 *
 *	The angle will be normalised, so values close to pi may round to -pi
 *
 *	@see intToAngle
 */
template< int BITS >
inline int angleToInt( float angle )
{
	const float upperBound = float(1 << (BITS - 1));
	return (int)floorf( angle * (upperBound / MATH_PI) + 0.5f );
}


/**
 *	This method is used to convert an compressed angle to an angle in the range
 *	[-pi, pi).
 *
 *	The angle will be normalised, so values close to pi may round to -pi
 *
 *	@see angleToInt
 */
template< int BITS >
inline float intToAngle( int compressedAngle )
{
	const float upperBound = float(1 << (BITS - 1));
	return float(compressedAngle) * (MATH_PI / upperBound);
}


/**
 *	This method is used to convert an angle in the range [-pi/2, pi/2) to an
 *	int using only a certain number of bits.
 *
 *	@see intToHalfAngle
 */
template< int BITS >
inline int halfAngleToInt( float angle )
{
	const float upperBound = float(1 << (BITS - 1));
	// TODO: (upperBound-1) here is because of the 254.f in the specialisation
	// for 8 bits. Find out why that was changed from 256.f in revision 103562
	return (int)Math::clamp( -upperBound,
		floorf( angle * ((upperBound-1) / (MATH_PI / 2.f) ) + 0.5f ),
		upperBound - 1 );
}


/**
 *	This method is used to convert an compressed angle to an angle in the range
 *	[-pi/2, pi/2).
 *
 *	@see halfAngleToInt
 */
template< int BITS >
inline float intToHalfAngle( int compressedAngle )
{
	const float upperBound = float(1 << (BITS - 1));
	// TODO: (upperBound-1) here is because of the 254.f in the specialisation
	// for 8 bits. Find out why that was changed from 256.f in revision 103562
	return float(compressedAngle) * ((MATH_PI / 2.f) / (upperBound-1));
}


// Specialisations to use the original fixed-size calculations
/**
 *	This method is used to convert an angle in the range [-pi, pi) into an int
 *	using only a certain number of bits.
 *
 *	@see intToAngle
 */
template<>
inline int angleToInt< 8 >( float angle )
{
	return (int8)floorf( (angle * 128.f) / MATH_PI + 0.5f );
}


/**
 *	This method is used to convert an compressed angle to an angle in the range
 *	[-pi, pi).
 *
 *	@see angleToInt
 */
template<>
inline float intToAngle< 8 >( int compressedAngle )
{
	return float(compressedAngle) * (MATH_PI / 128.f);
}


/**
 *	This method is used to convert an angle in the range [-pi/2, pi/2) to an int
 *	using only a certain number of bits.
 *
 *	@see intToHalfAngle
 */
template<>
inline int halfAngleToInt< 8 >( float angle )
{
	return (int8) Math::clamp( -128.f,
		floorf( (angle * 254.f) / MATH_PI + 0.5f ), 127.f );
}


/**
 *	This method is used to convert an compressed angle to an angle in the range
 *	[-pi/2, pi/2).
 *
 *	@see halfAngleToInt
 */
template<>
inline float intToHalfAngle< 8 >( int compressedAngle )
{
	return float(compressedAngle) * (MATH_PI / 254.f);
}
}


// -----------------------------------------------------------------------------
// Section: class PackedYaw
// -----------------------------------------------------------------------------
/**
 *	This method stores the supplied yaw value in our buffer
 */
template< int YAWBITS >
inline void PackedYaw< YAWBITS >::set( float yaw )
{
	BitWriter writer;
	writer.add( YAWBITS, angleToInt< YAWBITS >( yaw ) );
	MF_ASSERT( writer.usedBytes() == BYTES );
	memcpy( buff_, writer.bytes(), BYTES );
}


/**
 *	This method retrieves the stored yaw value from our buffer
 */
template< int YAWBITS >
inline void PackedYaw< YAWBITS >::get( float & yaw ) const
{
	MemoryIStream iStream( buff_, BYTES );
	BitReader reader( iStream );
	yaw = intToAngle< YAWBITS >( reader.getSigned( YAWBITS ) );
	MF_ASSERT( iStream.remainingLength() == 0 );
}


// -----------------------------------------------------------------------------
// Section: class PackedYawPitch
// -----------------------------------------------------------------------------
/**
 *	This method stores the supplied yaw and pitch values in our buffer
 */
template< bool HALFPITCH, int YAWBITS, int PITCHBITS >
inline void PackedYawPitch< HALFPITCH, YAWBITS, PITCHBITS >::set( float yaw,
	float pitch )
{
	BitWriter writer;
	writer.add( YAWBITS, angleToInt< YAWBITS >( yaw ) );
	writer.add( PITCHBITS,
		HALFPITCH ?
		halfAngleToInt< PITCHBITS >( pitch ) :
	angleToInt< PITCHBITS >( pitch ));
	MF_ASSERT( writer.usedBytes() == BYTES );
	memcpy( buff_, writer.bytes(), BYTES );
}


/**
 *	This method retrieves the supplied yaw and pitch values from our buffer
 */
template< bool HALFPITCH, int YAWBITS, int PITCHBITS >
inline void PackedYawPitch< HALFPITCH, YAWBITS, PITCHBITS >::get( float & yaw,
	float & pitch ) const
{
	MemoryIStream iStream( buff_, BYTES );
	BitReader reader( iStream );
	yaw = intToAngle< YAWBITS >( reader.getSigned( YAWBITS ) );
	pitch = HALFPITCH ?
		intToHalfAngle< PITCHBITS >( reader.getSigned( PITCHBITS )) :
	intToAngle< PITCHBITS >( reader.getSigned( PITCHBITS ));
	MF_ASSERT( iStream.remainingLength() == 0 );
}


// -----------------------------------------------------------------------------
// Section: class PackedYawPitchRoll
// -----------------------------------------------------------------------------
/**
 *	This method stores the supplied yaw, pitch and roll values in our buffer
 */
template< bool HALFPITCH, int YAWBITS, int PITCHBITS, int ROLLBITS >
inline void PackedYawPitchRoll< HALFPITCH, YAWBITS, PITCHBITS, ROLLBITS >::set(
	float yaw, float pitch, float roll )
{
	BitWriter writer;
	writer.add( YAWBITS, angleToInt< YAWBITS >( yaw ) );
	writer.add( PITCHBITS,
		HALFPITCH ?
		halfAngleToInt< PITCHBITS >( pitch ) :
	angleToInt< PITCHBITS >( pitch ));
	writer.add( ROLLBITS, angleToInt< ROLLBITS >( roll ) );
	MF_ASSERT( writer.usedBytes() == BYTES );
	memcpy( buff_, writer.bytes(), BYTES );
}


/**
 *	This method retrieves the supplied yaw, pitch and roll values from our
 *	buffer
 */
template< bool HALFPITCH, int YAWBITS, int PITCHBITS, int ROLLBITS >
inline void PackedYawPitchRoll< HALFPITCH, YAWBITS, PITCHBITS, ROLLBITS >::get(
	float & yaw, float & pitch, float & roll ) const
{
	MemoryIStream iStream( buff_, BYTES );
	BitReader reader( iStream );
	yaw = intToAngle< YAWBITS >( reader.getSigned( YAWBITS ) );
	pitch = HALFPITCH ?
		intToHalfAngle< PITCHBITS >( reader.getSigned( PITCHBITS )) :
	intToAngle< PITCHBITS >( reader.getSigned( PITCHBITS ));
	roll = intToAngle< ROLLBITS >( reader.getSigned( ROLLBITS ) );
	MF_ASSERT( iStream.remainingLength() == 0 );
}


// -----------------------------------------------------------------------------
// Section: Float compression functions
// -----------------------------------------------------------------------------
namespace
{
// IEEE752 binary32 format:
// Sign: 1 bit, Exponent: 8 bits, Mantissa: 23 bits.
union MultiType
{
	float	asFloat;
	uint32	asUint;
	int32	asInt;
};

/**
 *	This template function compresses the input into a BitWriter.
 *	It does this according to the bit-counts supplied as template parameters.
 *
 *	The input value must be in the range [-getLimit(), getLimit()) for the same
 *	template parameters, other values will be truncated to our
 *	furthest-from-zero possible value.
 */
// TODO: Specialise for EXPONENT_BITS == 8 and/or MANTISSA_BITS == 23
// TODO: Specialise for EXPONENT_BITS == 0 (Fixed-point)
// Partial specialisation means rewriting as static method of a struct.
template< int EXPONENT_BITS, int MANTISSA_BITS >
inline void packFloat( float value, BitWriter & writer )
{
	// These static asserts apply to the full set of functions
	BW_STATIC_ASSERT( sizeof( MultiType ) == 4,
		Unexpected_packing_of_MultiType );
	BW_STATIC_ASSERT( std::numeric_limits< float >::is_iec559,
		float_is_not_IEE754_format );
	BW_STATIC_ASSERT( sizeof( float ) == 4, float_is_not_IEEE754_binary32_format );

	// These are the maximums we can _write_ into the bitfields.
	const unsigned int maxMantissa = ( 1 << MANTISSA_BITS ) - 1;
	const unsigned int maxExponent = ( 1 << EXPONENT_BITS ) - 1;

	// For bit-manipulation
	MultiType multi;
	multi.asFloat = value;

	// Write the sign bit to the writer, and clear it in the MultiType.
	writer.add( 1, (multi.asUint >> 31) );
	multi.asUint &= 0x7fffffff;

	// This ensures that the minimum exponent is 1.
	multi.asFloat += 2.f;

	unsigned int exponent = (multi.asUint >> 23) & 0x000000ff;

	MF_ASSERT( exponent >= 128 );

	// The exponent of a float in IEEE752 binary32 format is biased by -127.
	// So the value 0b10000000 is an exponent of 1. So by ensuring that our
	// exponent is at least 1, we can keep only the low EXPONENT_BITS, and add
	// 128 when reading back.

	exponent -= 128;

	// Get the significant part of the mantissa
	unsigned int mantissa = ((multi.asUint & 0x007fffff) >>
		(23 - MANTISSA_BITS));

	MF_ASSERT( mantissa <= maxMantissa );

	// Get the next significant bit of the mantissa, for rounding.
	unsigned int nextBit = (((multi.asUint & 0x007fffff) >>
		(22 - MANTISSA_BITS)) & 0x1);

	if (nextBit == 1)
	{
		// Round up
		if (mantissa != maxMantissa)
		{
			++mantissa;
		}
		else
		{
			++exponent;
			mantissa = 0;
		}
	}

	// Catch overflow from either the input, or the rounding up.
	if (exponent > maxExponent)
	{
		exponent = maxExponent;
		mantissa = maxMantissa;
	}

	writer.add( EXPONENT_BITS, exponent );
	writer.add( MANTISSA_BITS, mantissa );
}


/**
 *	This template function uncompresses a float from the supplied BitReader
 *	It does this according to the bit-counts supplied as template parameters.
 */
// TODO: Specialise for EXPONENT_BITS == 8 and/or MANTISSA_BITS == 23
// TODO: Specialise for EXPONENT_BITS == 0 (Fixed-point)
// Partial specialisation means rewriting as static method of a struct.
template< int EXPONENT_BITS, int MANTISSA_BITS >
inline float unpackFloat( BitReader & reader )
{
	MultiType multi;
	multi.asUint = 0x00000000;
	unsigned int signBit = (reader.get( 1 ) << 31);
	multi.asUint |= ((reader.get( EXPONENT_BITS ) | 0x80) << 23);
	multi.asUint |= (reader.get( MANTISSA_BITS ) << (23 - MANTISSA_BITS));

	multi.asFloat -= 2.f;

	multi.asUint |= signBit;

	return multi.asFloat;
}


/**
 *	This template function calculates the error in a float from the supplied
 *	BitReader.
 *	It does this according to the bit-counts supplied as template parameters.
 */
// TODO: Specialise for EXPONENT_BITS == 8 and MANTISSA_BITS == 23
// TODO: Specialise for EXPONENT_BITS == 0 (Fixed-point)
// Partial specialisation means rewriting as static method of a struct.
template< int EXPONENT_BITS, int MANTISSA_BITS >
inline float getError( BitReader & reader )
{
	// Discard sign bit, read exponent, and discard mantissa.
	reader.get( 1 );
	int exponent = reader.get( EXPONENT_BITS );
	reader.get( MANTISSA_BITS );

	// The first range is 2 metres and the mantissa is MANTISSA_BITS bits. This
	// makes each step 2/(2**MANTISSA_BITS). The compression rounds and so the
	// error can be up to half step size. The error doubles for each additional
	// range.
	return (1.f / float(1 << MANTISSA_BITS)) * (1ULL << exponent);
}


/**
 *	This template function calculates the absolute upper bound of values we can
 *	pack without an overflow. Due to rounding error, this value will be larger
 *	than the largest possible number we can return, and is an exclusive limit
 *	when positive, and an inclusive limit when negated.
 *	It does this according to the bit-counts supplied as template parameters.
 */
template< int EXPONENT_BITS, int MANTISSA_BITS >
inline float getLimit()
{
	const unsigned int maxExponent = ( 1 << EXPONENT_BITS ) - 1;
	const unsigned int maxMantissa = ( 1 << MANTISSA_BITS ) - 1;

	// From getError
	const float error = (1.f / float(1 << MANTISSA_BITS)) * (1ULL << maxExponent);

	// Unpack as if it was all 1's except the sign.
	MultiType multi;
	multi.asUint = 0x00000000;
	multi.asUint |= ((maxExponent | 0x80) << 23);
	multi.asUint |= (maxMantissa << (23 - MANTISSA_BITS));

	multi.asFloat -= 2.f;

	return multi.asFloat + error;
}
}

// -----------------------------------------------------------------------------
// Section: class PackedGroundPos
// -----------------------------------------------------------------------------
/**
 *	This static method returns the exclusive upper limit of representable
 *	values for the X and Z of this type.
 */
template< int EXPONENT_BITS, int MANTISSA_BITS >
/* static */ inline float PackedGroundPos< EXPONENT_BITS,
	MANTISSA_BITS >::maxLimit( float scale )
{
	return getLimit< EXPONENT_BITS, MANTISSA_BITS >() * scale;
}


/**
 *	This static method returns the inclusive lower limit of representable
 *	values for the X and Z of this type.
 */
template< int EXPONENT_BITS, int MANTISSA_BITS >
/* static */ inline float PackedGroundPos< EXPONENT_BITS,
	MANTISSA_BITS >::minLimit( float scale )
{
	return maxLimit( scale ) * -1.f;
}


/**
 *	Constructor.
 */
template< int EXPONENT_BITS, int MANTISSA_BITS >
inline PackedGroundPos< EXPONENT_BITS, MANTISSA_BITS >::PackedGroundPos(
	float x, float z, float scale )
{
	this->packXZ( x, z, scale );
}


/**
 *	This method compresses the two input floats into our buffer using the
 *	packFloat function above.
 *
 *	The input values must be in the range (this->minLimit, this->maxLimit).
 */
template< int EXPONENT_BITS, int MANTISSA_BITS >
inline void PackedGroundPos< EXPONENT_BITS, MANTISSA_BITS >::packXZ( float x,
	float z, float scale )
{
	BitWriter writer;

	// We divide the two values by the scale. The value of scale is checked to
	// be non-zero in CellAppConfig::updateDerivedSettings.
	packFloat< EXPONENT_BITS, MANTISSA_BITS >( x / scale, writer );
	packFloat< EXPONENT_BITS, MANTISSA_BITS >( z / scale, writer );

	MF_ASSERT( writer.usedBytes() == BYTES );
	memcpy( buff_, writer.bytes(), BYTES );
}


/**
 *	This function uncompresses the values that were created by pack.
 */
template< int EXPONENT_BITS, int MANTISSA_BITS >
inline void PackedGroundPos< EXPONENT_BITS, MANTISSA_BITS >::unpackXZ(
	float & x, float & z, float scale ) const
{
	MemoryIStream iStream( buff_, BYTES );
	BitReader reader( iStream );

	x = unpackFloat< EXPONENT_BITS, MANTISSA_BITS >( reader ) * scale;
	z = unpackFloat< EXPONENT_BITS, MANTISSA_BITS >( reader ) * scale;
	MF_ASSERT( iStream.remainingLength() == 0 );
}


/**
 *	This method returns the maximum error in the x and z components caused by
 *	compression.
 */
template< int EXPONENT_BITS, int MANTISSA_BITS >
inline void PackedGroundPos< EXPONENT_BITS, MANTISSA_BITS >::getXZError(
	float & xError, float & zError, float scale ) const
{
	MemoryIStream iStream( buff_, BYTES );
	BitReader reader( iStream );

	xError = getError< EXPONENT_BITS, MANTISSA_BITS >( reader ) * scale;
	zError = getError< EXPONENT_BITS, MANTISSA_BITS >( reader ) * scale;
	MF_ASSERT( iStream.remainingLength() == 0 );
}


/**
 *	This operator streams a PackedGroundPos to the given BinaryStream
 */
template< int EXPONENT_BITS, int MANTISSA_BITS >
BinaryIStream& operator>>( BinaryIStream& is,
	PackedGroundPos< EXPONENT_BITS, MANTISSA_BITS > &xz )
{
	memcpy( xz.buff_, is.retrieve( xz.BYTES ), xz.BYTES );
	return is;
}


/**
 *	This operator streams a PackedGroundPos from the given BinaryStream
 */
template< int EXPONENT_BITS, int MANTISSA_BITS >
BinaryOStream& operator<<( BinaryOStream& os,
	const PackedGroundPos< EXPONENT_BITS, MANTISSA_BITS > & xz )
{
	memcpy( os.reserve( xz.BYTES ), xz.buff_, xz.BYTES );
	return os;
}


// Specialisations to use the original fixed-size calculations, and apply
// the relevant byte-swapping.
// We only do the byte-swapping for OldPackedXZ. The old version stores
// in 3-byte local-endianness, and streams out in little-endian.
namespace
{
	// Local typedef for the fixed-size version of the data type
typedef PackedGroundPos< /* EXPONENT_BITS */ 3, /* MANTISSA_BITS */ 8 >
	OldPackedXZ;
}
/**
 *	This method compresses the two input floats into our buffer using the
 *	packFloat function above.
 *
 *	The input values must be in the range (this->minLimit, this->maxLimit).
 */
template<>
inline void OldPackedXZ::packXZ( float xValue, float zValue, float scale )
{
	// We divide the two values by the scale. The value of scale is checked to
	// be non-zero in CellAppConfig::updateDerivedSettings.
	xValue /= scale;
	zValue /= scale;

	const float addValues[] = { 2.f, -2.f };
	const uint32 xCeilingValues[] = { 0, 0x7ff000 };
	const uint32 zCeilingValues[] = { 0, 0x0007ff };

	MultiType x; x.asFloat = xValue;
	MultiType z; z.asFloat = zValue;

	// We want the value to be in the range (-512, -2], [2, 512). Take 2 from
	// negative numbers and add 2 to positive numbers. This is to make the
	// exponent be in the range [2, 9] which for the exponent of the float in
	// IEEE format is between 10000000 and 10000111.

	x.asFloat += addValues[ x.asInt < 0 ];
	z.asFloat += addValues[ z.asInt < 0 ];

	uint result = 0;

	// Here we check if the input values are out of range. The exponent is meant
	// to be between 10000000 and 10000111. We check that the top 5 bits are
	// 10000.
	// We also need to check for the case that the rounding causes an overflow.
	// This occurs when the bits of the exponent and mantissa we are interested
	// in are all 1 and the next significant bit in the mantissa is 1.
	// If an overflow occurs, we set the result to the maximum result.
	result |= xCeilingValues[((x.asUint & 0x7c000000) != 0x40000000) ||
		((x.asUint & 0x3ffc000) == 0x3ffc000)];
	result |= zCeilingValues[((z.asUint & 0x7c000000) != 0x40000000) ||
		((z.asUint & 0x3ffc000) == 0x3ffc000)];


	// Copy the low 3 bits of the exponent and the high 8 bits of the mantissa.
	// These are the bits 15 - 25. We then add one to this value if the high bit
	// of the remainder of the mantissa is set. It magically works that if the
	// mantissa wraps around, the exponent is incremented by one as required.
	result |= ((x.asUint >>  3) & 0x7ff000) + ((x.asUint & 0x4000) >> 2);
	result |= ((z.asUint >> 15) & 0x0007ff) + ((z.asUint & 0x4000) >> 14);

	// We only need this for values in the range [509.5, 510.0). For these
	// values, the above addition overflows to the sign bit.
	result &= 0x7ff7ff;

	// Copy the sign bit (the high bit) from the values to the result.
	result |=  (x.asUint >>  8) & 0x800000;
	result |=  (z.asUint >> 20) & 0x000800;

	BW_PACK3( (char*)buff_, result );
}


/**
 *	This function uncompresses the values that were created by pack.
 */
template<>
inline void OldPackedXZ::unpackXZ( float & x, float & z, float scale ) const
{
	uint data = BW_UNPACK3( (const char*)buff_ );

	MultiType & xu = (MultiType&)x;
	MultiType & zu = (MultiType&)z;

	// The high 5 bits of the exponent are 10000. The low 17 bits of the
	// mantissa are all clear.
	xu.asUint = 0x40000000;
	zu.asUint = 0x40000000;

	// Copy the low 3 bits of the exponent and the high 8 bits of the mantissa
	// into the results.
	xu.asUint |= (data & 0x7ff000) << 3;
	zu.asUint |= (data & 0x0007ff) << 15;

	// The produced floats are in the range (-512, -2], [2, 512). Change this
	// back to the range (-510, 510). [Note: the sign bit is not on yet.]
	xu.asFloat -= 2.0f;
	zu.asFloat -= 2.0f;

	// Copy the sign bit across.
	xu.asUint |= (data & 0x800000) << 8;
	zu.asUint |= (data & 0x000800) << 20;

	// Multiply by the scale to decompress the value
	xu.asFloat *= scale;
	zu.asFloat *= scale;
}


/**
 *	This method returns the maximum error in the x and z components caused by
 *	compression.
 */
template<>
inline void OldPackedXZ::getXZError( float & xError, float & zError,
	float scale ) const
{
	uint32 data = BW_UNPACK3( (const char*)buff_ );
	//
	// The exponents are the 3 bits from bit 8.
	int xExp = (data >> 20) & 0x7;
	int zExp = (data >>  8) & 0x7;

	// The first range is 2 metres and the mantissa is 8 bits. This makes each
	// step 2/(2**8). The x and z compression rounds and so the error can be
	// up to half step size. The error doubles for each additional range.
	xError = (1.f/256.f) * (1 << xExp) * scale;
	zError = (1.f/256.f) * (1 << zExp) * scale;
}


/**
 *	This operator streams a PackedGroundPos to the given BinaryStream
 */
template<>
inline BinaryIStream& operator>>( BinaryIStream& is, OldPackedXZ &xz )
{
	BW_STATIC_ASSERT( sizeof( xz.buff_ ) == 3,
		Unexpected_size_of_oldPackedXZ );
	const void *src = is.retrieve( sizeof( xz.buff_ ) );
	BW_NTOH3_ASSIGN( reinterpret_cast<char *>( xz.buff_ ),
		reinterpret_cast<const char*>( src ) );
	return is;
}


/**
 *	This operator streams a PackedGroundPos from the given BinaryStream
 */
template<>
inline BinaryOStream& operator<<( BinaryOStream& os, const OldPackedXZ & xz )
{
	BW_STATIC_ASSERT( sizeof( xz.buff_ ) == 3,
		Unexpected_size_of_oldPackedXZ );
	void * dest = os.reserve( sizeof( xz.buff_ ) );
	BW_HTON3_ASSIGN( reinterpret_cast< char * >( dest ),
		reinterpret_cast< const char* >( xz.buff_ ) );
	return os;
}


// -----------------------------------------------------------------------------
// Section: class PackedFullPos
// -----------------------------------------------------------------------------
/**
 *	This static method returns the exclusive upper limit of representable
 *	values for the X and Z of this type.
 */
template< int EXPONENT_BITS, int MANTISSA_BITS, int EXPONENT_BITS_Y,
	int MANTISSA_BITS_Y >
/* static */ inline float PackedFullPos< EXPONENT_BITS, MANTISSA_BITS,
	EXPONENT_BITS_Y, MANTISSA_BITS_Y >::maxLimit( float xzscale )
{
	return getLimit< EXPONENT_BITS, MANTISSA_BITS >() * xzscale;
}


/**
 *	This static method returns the inclusive lower limit of representable
 *	values for the X and Z of this type.
 */
template< int EXPONENT_BITS, int MANTISSA_BITS, int EXPONENT_BITS_Y,
	int MANTISSA_BITS_Y >
/* static */ inline float PackedFullPos< EXPONENT_BITS, MANTISSA_BITS,
	EXPONENT_BITS_Y, MANTISSA_BITS_Y >::minLimit( float xzscale )
{
	return maxLimit( xzscale ) * -1.f;
}


/**
 *	This static method returns the exclusive upper limit of representable
 *	values for the Y of this type.
 */
template< int EXPONENT_BITS, int MANTISSA_BITS, int EXPONENT_BITS_Y,
	int MANTISSA_BITS_Y >
/* static */ inline float PackedFullPos< EXPONENT_BITS, MANTISSA_BITS,
	EXPONENT_BITS_Y, MANTISSA_BITS_Y >::maxYLimit()
{
	return getLimit< EXPONENT_BITS_Y, MANTISSA_BITS_Y >();
}


/**
 *	This static method returns the inclusive lower limit of representable
 *	values for the Y of this type.
 */
template< int EXPONENT_BITS, int MANTISSA_BITS, int EXPONENT_BITS_Y,
	int MANTISSA_BITS_Y >
/* static */ inline float PackedFullPos< EXPONENT_BITS, MANTISSA_BITS,
	EXPONENT_BITS_Y, MANTISSA_BITS_Y >::minYLimit()
{
	return maxYLimit() * -1.f;
}


/**
 *	Constructor.
 */
template< int EXPONENT_BITS, int MANTISSA_BITS, int EXPONENT_BITS_Y,
	int MANTISSA_BITS_Y >
inline PackedFullPos< EXPONENT_BITS, MANTISSA_BITS, EXPONENT_BITS_Y,
	MANTISSA_BITS_Y >::PackedFullPos( float x, float y, float z, float xzscale )
{
	this->packXYZ( x, y, z, xzscale );
}


/**
 *	This method compresses the two input floats into our buffer using the
 *	packFloat function above.
 *
 *	The input values must be in the range (this->minLimit, this->maxLimit).
 */
template< int EXPONENT_BITS, int MANTISSA_BITS, int EXPONENT_BITS_Y,
	int MANTISSA_BITS_Y >
inline void PackedFullPos< EXPONENT_BITS, MANTISSA_BITS, EXPONENT_BITS_Y,
	MANTISSA_BITS_Y >::packXYZ( float x, float y, float z, float xzscale )
{
	BitWriter writer;

	// We divide the two values by the xzscale. The value of xzscale is checked to
	// be non-zero in CellAppConfig::updateDerivedSettings.
	packFloat< EXPONENT_BITS, MANTISSA_BITS >( x / xzscale, writer );
	packFloat< EXPONENT_BITS, MANTISSA_BITS >( z / xzscale, writer );
	packFloat< EXPONENT_BITS_Y, MANTISSA_BITS_Y >( y, writer );

	MF_ASSERT( writer.usedBytes() == BYTES );
	memcpy( buff_, writer.bytes(), BYTES );
}


/**
 *	This function uncompresses the values that were created by pack.
 */
template< int EXPONENT_BITS, int MANTISSA_BITS, int EXPONENT_BITS_Y,
	int MANTISSA_BITS_Y >
inline void PackedFullPos< EXPONENT_BITS, MANTISSA_BITS, EXPONENT_BITS_Y,
	MANTISSA_BITS_Y >::unpackXYZ( float & x, float & y, float & z,
	float xzscale ) const
{
	MemoryIStream iStream( buff_, BYTES );
	BitReader reader( iStream );

	x = unpackFloat< EXPONENT_BITS, MANTISSA_BITS >( reader ) * xzscale;
	z = unpackFloat< EXPONENT_BITS, MANTISSA_BITS >( reader ) * xzscale;
	y = unpackFloat< EXPONENT_BITS_Y, MANTISSA_BITS_Y >( reader );
	MF_ASSERT( iStream.remainingLength() == 0 );
}


/**
 *	This method returns the maximum error in the x and z components caused by
 *	compression.
 */
template< int EXPONENT_BITS, int MANTISSA_BITS, int EXPONENT_BITS_Y,
	int MANTISSA_BITS_Y >
inline void PackedFullPos< EXPONENT_BITS, MANTISSA_BITS, EXPONENT_BITS_Y,
	MANTISSA_BITS_Y >::getXYZError( float & xError, float & yError,
	float & zError, float xzscale ) const
{
	MemoryIStream iStream( buff_, BYTES );
	BitReader reader( iStream );

	xError = getError< EXPONENT_BITS, MANTISSA_BITS >( reader ) * xzscale;
	zError = getError< EXPONENT_BITS, MANTISSA_BITS >( reader ) * xzscale;
	yError = getError< EXPONENT_BITS_Y, MANTISSA_BITS_Y >( reader );
	MF_ASSERT( iStream.remainingLength() == 0 );
}


/**
 *	This operator streams a PackedFullPos to the given BinaryStream
 */
template< int EXPONENT_BITS, int MANTISSA_BITS, int EXPONENT_BITS_Y,
	int MANTISSA_BITS_Y >
BinaryIStream& operator>>( BinaryIStream& is,
	PackedFullPos< EXPONENT_BITS, MANTISSA_BITS, EXPONENT_BITS_Y,
	MANTISSA_BITS_Y > &xyz )
{
	memcpy( xyz.buff_, is.retrieve( xyz.BYTES ), xyz.BYTES );
	return is;
}


/**
 *	This operator streams a PackedFullPos from the given BinaryStream
 */
template< int EXPONENT_BITS, int MANTISSA_BITS, int EXPONENT_BITS_Y,
	int MANTISSA_BITS_Y >
BinaryOStream& operator<<( BinaryOStream& os,
	const PackedFullPos< EXPONENT_BITS, MANTISSA_BITS, EXPONENT_BITS_Y,
	MANTISSA_BITS_Y > & xyz )
{
	memcpy( os.reserve( xyz.BYTES ), xyz.buff_, xyz.BYTES );
	return os;
}


// Specialisations to use the original fixed-size calculations, and apply
// the relevant byte-swapping.
// We only do the byte-swapping for OldPackedXYZ.  The old version stores
// XZ in 3-byte local-endianness, Y in 2-byte local-endianness, and streams out
// in little-endian for XZ and Y. (Using the uint16 streaming operator for the
// latter)
namespace
{
	// Local typedef for the fixed-size version of the data type
typedef PackedFullPos< /* EXPONENT_BITS */ 3, /* MANTISSA_BITS */ 8,
	/* EXPONENT_BITS_Y */ 4, /* MANTISSA_BITS_Y */ 11 > OldPackedXYZ;
}
/**
 *	This method compresses the two input floats into our buffer using the
 *	packFloat function above.
 *
 *	The input values must be in the range (this->minLimit, this->maxLimit).
 */
template<>
inline void OldPackedXYZ::packXYZ( float xValue, float yValue, float zValue,
	float scale )
{
	// We divide the two values by the scale. The value of scale is checked to
	// be non-zero in CellAppConfig::updateDerivedSettings.
	xValue /= scale;
	zValue /= scale;

	const float addValues[] = { 2.f, -2.f };
	const uint32 xCeilingValues[] = { 0, 0x7ff000 };
	const uint32 zCeilingValues[] = { 0, 0x0007ff };

	MultiType x; x.asFloat = xValue;
	MultiType z; z.asFloat = zValue;

	// We want the value to be in the range (-512, -2], [2, 512). Take 2 from
	// negative numbers and add 2 to positive numbers. This is to make the
	// exponent be in the range [2, 9] which for the exponent of the float in
	// IEEE format is between 10000000 and 10000111.

	x.asFloat += addValues[ x.asInt < 0 ];
	z.asFloat += addValues[ z.asInt < 0 ];

	uint result = 0;

	// Here we check if the input values are out of range. The exponent is meant
	// to be between 10000000 and 10000111. We check that the top 5 bits are
	// 10000.
	// We also need to check for the case that the rounding causes an overflow.
	// This occurs when the bits of the exponent and mantissa we are interested
	// in are all 1 and the next significant bit in the mantissa is 1.
	// If an overflow occurs, we set the result to the maximum result.
	result |= xCeilingValues[((x.asUint & 0x7c000000) != 0x40000000) ||
		((x.asUint & 0x3ffc000) == 0x3ffc000)];
	result |= zCeilingValues[((z.asUint & 0x7c000000) != 0x40000000) ||
		((z.asUint & 0x3ffc000) == 0x3ffc000)];


	// Copy the low 3 bits of the exponent and the high 8 bits of the mantissa.
	// These are the bits 15 - 25. We then add one to this value if the high bit
	// of the remainder of the mantissa is set. It magically works that if the
	// mantissa wraps around, the exponent is incremented by one as required.
	result |= ((x.asUint >>  3) & 0x7ff000) + ((x.asUint & 0x4000) >> 2);
	result |= ((z.asUint >> 15) & 0x0007ff) + ((z.asUint & 0x4000) >> 14);

	// We only need this for values in the range [509.5, 510.0). For these
	// values, the above addition overflows to the sign bit.
	result &= 0x7ff7ff;

	// Copy the sign bit (the high bit) from the values to the result.
	result |=  (x.asUint >>  8) & 0x800000;
	result |=  (z.asUint >> 20) & 0x000800;

	BW_PACK3( (char*)buff_, result );

	// TODO: no range checking is done on the input value or rounding of the
	// result.

	// Add bias to the value to force the floating point exponent to be in the
	// range [2, 15], which translates to an IEEE754 exponent in the range
	// 10000000 to 1000FFFF (which contains a +127 bias).
	// Thus only the least significant 4 bits of the exponent need to be
	// stored.

	MultiType y; y.asFloat = yValue;
	y.asFloat += addValues[ y.asInt < 0 ];

	uint16& yDataInt = *(uint16*)(buff_ + 3);

	// Extract the lower 4 bits of the exponent, and the 11 most significant
	// bits of the mantissa (15 bits all up).
	yDataInt = (y.asUint >> 12) & 0x7fff;

	// Transfer the sign bit.
	yDataInt |= ((y.asUint >> 16) & 0x8000);
}


/**
 *	This function uncompresses the values that were created by pack.
 */
template<>
inline void OldPackedXYZ::unpackXYZ( float & x, float & y, float & z,
	float scale ) const
{
	uint data = BW_UNPACK3( (const char*)buff_ );

	MultiType & xu = (MultiType&)x;
	MultiType & zu = (MultiType&)z;

	// The high 5 bits of the exponent are 10000. The low 17 bits of the
	// mantissa are all clear.
	xu.asUint = 0x40000000;
	zu.asUint = 0x40000000;

	// Copy the low 3 bits of the exponent and the high 8 bits of the mantissa
	// into the results.
	xu.asUint |= (data & 0x7ff000) << 3;
	zu.asUint |= (data & 0x0007ff) << 15;

	// The produced floats are in the range (-512, -2], [2, 512). Change this
	// back to the range (-510, 510). [Note: the sign bit is not on yet.]
	xu.asFloat -= 2.0f;
	zu.asFloat -= 2.0f;

	// Copy the sign bit across.
	xu.asUint |= (data & 0x800000) << 8;
	zu.asUint |= (data & 0x000800) << 20;

	// Multiply by the scale to decompress the value
	xu.asFloat *= scale;
	zu.asFloat *= scale;

	// Preload output value with 0(sign) 10000000(exponent) 00..(mantissa).
	MultiType & yu = (MultiType&)y;
	yu.asUint = 0x40000000;

	uint16& yDataInt = *(uint16*)(buff_ + 3);

	// Copy the 4-bit lower 4 bits of the exponent and the
	// 11 most significant bits of the mantissa.
	yu.asUint |= (yDataInt & 0x7fff) << 12;

	// Remove value bias.
	yu.asFloat -= 2.f;

	// Copy the sign bit.
	yu.asUint |= (yDataInt & 0x8000) << 16;
}


/**
 *	This method returns the maximum error in the x and z components caused by
 *	compression.
 */
template<>
inline void OldPackedXYZ::getXYZError( float & xError, float & yError,
	float & zError, float scale ) const
{
	uint32 data = BW_UNPACK3( (const char*)buff_ );
	//
	// The exponents are the 3 bits from bit 8.
	int xExp = (data >> 20) & 0x7;
	int zExp = (data >>  8) & 0x7;

	// The first range is 2 metres and the mantissa is 8 bits. This makes each
	// step 2/(2**8). The x and z compression rounds and so the error can be
	// up to half step size. The error doubles for each additional range.
	xError = (1.f/256.f) * (1 << xExp) * scale;
	zError = (1.f/256.f) * (1 << zExp) * scale;

	uint16& yDataInt = *(uint16*)(buff_ + 3);

	// The exponent is the 4 bits from bit 11.
	int yExp = (yDataInt >> 11) & 0xf;

	// The first range is 2 metres and the mantissa is 11 bits. This makes each
	// step 2/(2**11). The y compression only rounds and so the maximum error
	// is the step size. This doubles for each additional range.
	yError = (2.f/2048.f) * (1 << yExp);
}


/**
 *	This operator streams a PackedFullPos to the given BinaryStream
 */
template<>
inline BinaryIStream& operator>>( BinaryIStream& is, OldPackedXYZ &xyz )
{
	BW_STATIC_ASSERT( sizeof( xyz.buff_ ) == 5,
		Unexpected_size_of_oldPackedXYZ );
	const void *src = is.retrieve( 3 );
	BW_NTOH3_ASSIGN( reinterpret_cast<char *>( xyz.buff_ ),
		reinterpret_cast<const char*>( src ) );
	return is >> *(uint16*)(xyz.buff_ + 3);
}


/**
 *	This operator streams a PackedFullPos from the given BinaryStream
 */
template<>
inline BinaryOStream& operator<<( BinaryOStream& os, const OldPackedXYZ & xyz )
{
	BW_STATIC_ASSERT( sizeof( xyz.buff_ ) == 5,
		Unexpected_size_of_oldPackedXYZ );
	void * dest = os.reserve( 3 );
	BW_HTON3_ASSIGN( reinterpret_cast< char * >( dest ),
		reinterpret_cast< const char* >( xyz.buff_ ) );
	return os << *(uint16*)(xyz.buff_ + 3);
}

// msgtypes.ipp
