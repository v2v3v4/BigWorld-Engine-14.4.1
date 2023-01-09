#ifndef MSGTYPES_HPP
#define MSGTYPES_HPP

#include <float.h>
#include <math.h>

#include "basictypes.hpp"
#include "math/mathdef.hpp"
#include "cstdmf/binary_stream.hpp"
#include "cstdmf/bit_reader.hpp"
#include "cstdmf/bit_writer.hpp"
#include "cstdmf/memory_stream.hpp"

BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: Configuration
// -----------------------------------------------------------------------------
// These constants can be adjusted to control the allocation of bits for
// volatile (low-detail) updates.

// Set this to 1 to use absolute position volatile updates instead of
// relative position volatile updates in the X-Z plane.
// You probably want to set EXPONENTBITS_XZ to 0 in absolute mode.
// Y-values in off-Ground updates are always absolute
#define VOLATILE_POSITIONS_ARE_ABSOLUTE 0

// The range of the X and Z components of the position updates are spread over
// the maximumAoIRadius configured in the CellApp based on the least-accurate
// of on-Ground and off-Ground update ranges.
// So the exponent range of the two types of updates is the same, as any
// extra bits in one or the other would be wasted.
// Setting this to 0 will produce fixed-point volatile updates.
#define EXPONENTBITS_XZ 3

// When sending an on-Ground (XZ) position update:
// (EXPONENTBITS_XZ + XZ_MANTISSABITS_XZ + 1) * 2 bits
// Mantissa controls the maximum accuracy of a position update
#define XZ_MANTISSABITS_XZ 8

// When sending an off-Ground (XYZ) position update:
// ((EXPONENTBITS_XZ + XYZ_MANTISSABITS_XZ + 1) * 2) +
// (XYZ_EXPONENTBITS_Y + XYZ_MANTISSABITS_Y + 1) bits

// Mantissa controls the maximum accuracy of a position update
// If this is different from XZ_MANTISSABITS_XZ then there is a risk of
// position juddering as entities leave or return to the ground.
#define XYZ_MANTISSABITS_XZ 8

// Exponent controls the maximum range of a volatile position update, limiting
// Y to slightly less than +- 2*((2^2^XYZ_EXPONENTBITS_Y)-1)
// See getLimit() in msgtypes.ipp for the exact calculation.
#define XYZ_EXPONENTBITS_Y 4
// Mantissa controls the maximum accuracy of a position update
#define XYZ_MANTISSABITS_Y 11

// When sending a Yaw-only direction update:
#define YAW_YAWBITS 8

// When sending a Yaw and Pitch direction update:
#define YAWPITCH_YAWBITS 8
#define YAWPITCH_PITCHBITS 8
// Controls whether pitch should be [-pi,pi) or [-pi/2,pi/2)
#define YAWPITCH_HALFPITCH false

// When sending a Yaw, Pitch and Roll direction update:
#define YAWPITCHROLL_YAWBITS 8
#define YAWPITCHROLL_PITCHBITS 8
#define YAWPITCHROLL_ROLLBITS 8
// Controls whether pitch should be [-pi,pi) or [-pi/2,pi/2)
#define YAWPITCHROLL_HALFPITCH true


// -----------------------------------------------------------------------------
// Section: Configuration checking
// -----------------------------------------------------------------------------
#if EXPONENTBITS_XZ < 0 || EXPONENTBITS_XZ > 8
#error Invalid bit count for Exponent in onGround (XZ) position update
#endif

#if XZ_MANTISSABITS_XZ < 0 || XZ_MANTISSABITS_XZ > 23
#error Invalid bit count for Mantissa in onGround (XZ) position update
#endif

#if (EXPONENTBITS_XZ * 2 + XZ_MANTISSABITS_XZ * 2 + 2) % 8 != 0
#error Unused bits in onGround (XZ) position update
#endif

#if EXPONENTBITS_XZ < 0 || EXPONENTBITS_XZ > 8
#error Invalid bit count for Exponent in offGround (XYZ) position update
#endif

#if XYZ_MANTISSABITS_XZ < 0 || XYZ_MANTISSABITS_XZ > 23
#error Invalid bit count for Mantissa in offGround (XYZ) position update
#endif

#if XYZ_EXPONENTBITS_Y < 0 || XYZ_EXPONENTBITS_Y > 8
#error Invalid bit count for Y Exponent in offGround (XYZ) position update
#endif

#if XYZ_MANTISSABITS_Y < 0 || XYZ_MANTISSABITS_Y > 23
#error Invalid bit count for Y Mantissa in offGround (XYZ) position update
#endif

#if (EXPONENTBITS_XZ * 2 + XYZ_MANTISSABITS_XZ * 2 + 2 + XYZ_EXPONENTBITS_Y + XYZ_MANTISSABITS_Y + 1) % 8 != 0
#error Unused bits in offGround (XYZ) position update
#endif

#if YAW_YAWBITS < 1
#error No bits supplied for Yaw in Yaw-only update
#endif

#if YAWPITCH_YAWBITS < 1
#error No bits supplied for Yaw in Yaw+Pitch update
#endif

#if YAWPITCH_PITCHBITS < 1
#error No bits supplied for Pitch in Yaw+Pitch update
#endif

#if YAWPITCHROLL_YAWBITS < 1
#error No bits supplied for Yaw in Yaw+Pitch+Roll update
#endif

#if YAWPITCHROLL_PITCHBITS < 1
#error No bits supplied for Pitch in Yaw+Pitch+Roll update
#endif

#if YAWPITCHROLL_ROLLBITS < 1
#error No bits supplied for Roll in Yaw+Pitch+Roll update
#endif

#if YAW_YAWBITS % 8 != 0
#error Unused bits in Yaw-only direction update
#endif

#if (YAWPITCH_YAWBITS + YAWPITCH_PITCHBITS) % 8 != 0
#error Unused bits in Yaw+Pitch direction update
#endif

#if (YAWPITCHROLL_YAWBITS + YAWPITCHROLL_PITCHBITS+ YAWPITCHROLL_ROLLBITS) % 8 != 0
#error Unused bits in Yaw+Pitch+Roll direction update
#endif


// -----------------------------------------------------------------------------
// Section: class PackedYaw
// -----------------------------------------------------------------------------
/**
 *	This class is used to pack a yaw value for network transmission.
 *
 *	@ingroup network
 */
template< int YAWBITS = YAW_YAWBITS >
class PackedYaw
{
	static const int BYTES = ((YAWBITS - 1) / 8) + 1;
public:
	PackedYaw( float yaw )
	{
		this->set( yaw );
	}

	PackedYaw() {};

	void set( float yaw );
	void get( float & yaw ) const;

	friend BinaryIStream& operator>>( BinaryIStream& is,
		PackedYaw< YAWBITS > &y )
	{
		memcpy( y.buff_, is.retrieve( BYTES ), BYTES );
		return is;
	}

	friend BinaryOStream& operator<<( BinaryOStream& os,
		const PackedYaw< YAWBITS > &y )
	{
		memcpy( os.reserve( BYTES ), y.buff_, BYTES );
		return os;
	}

private:
	char buff_[ BYTES ];
};



// -----------------------------------------------------------------------------
// Section: class PackedYawPitch
// -----------------------------------------------------------------------------
/**
 *	This class is used to pack a yaw and pitch value for network transmission.
 *
 *	@ingroup network
 */
template< bool HALFPITCH = YAWPITCH_HALFPITCH, int YAWBITS = YAWPITCH_YAWBITS,
	int PITCHBITS = YAWPITCH_PITCHBITS >
class PackedYawPitch
{
	static const int BYTES = ((YAWBITS + PITCHBITS - 1) / 8) + 1;
public:
	PackedYawPitch( float yaw, float pitch )
	{
		this->set( yaw, pitch );
	}

	PackedYawPitch() {};

	void set( float yaw, float pitch );
	void get( float & yaw, float & pitch ) const;

	friend BinaryIStream& operator>>( BinaryIStream& is,
		PackedYawPitch< HALFPITCH, YAWBITS, PITCHBITS > &yp )
	{
		memcpy( yp.buff_, is.retrieve( BYTES ), BYTES );
		return is;
	}

	friend BinaryOStream& operator<<( BinaryOStream& os,
		const PackedYawPitch< HALFPITCH, YAWBITS, PITCHBITS > &yp )
	{
		memcpy( os.reserve( BYTES ), yp.buff_, BYTES );
		return os;
	}

private:
	char buff_[ BYTES ];
};



// -----------------------------------------------------------------------------
// Section: class PackedYawPitchRoll
// -----------------------------------------------------------------------------
/**
 *	This class is used to pack a yaw, pitch and roll value for network
 *	transmission.
 *
 *	@ingroup network
 */
template< bool HALFPITCH = YAWPITCHROLL_HALFPITCH,
	int YAWBITS = YAWPITCHROLL_YAWBITS, int PITCHBITS = YAWPITCHROLL_PITCHBITS,
	int ROLLBITS = YAWPITCHROLL_ROLLBITS >
class PackedYawPitchRoll
{
	static const int BYTES = ((YAWBITS + PITCHBITS + ROLLBITS - 1) / 8) + 1;
public:
	PackedYawPitchRoll( float yaw, float pitch, float roll )
	{
		this->set( yaw, pitch, roll );
	}

	PackedYawPitchRoll() {};

	void set( float yaw, float pitch, float roll );
	void get( float & yaw, float & pitch, float & roll ) const;

	friend BinaryIStream& operator>>( BinaryIStream& is,
		PackedYawPitchRoll< HALFPITCH, YAWBITS, PITCHBITS, ROLLBITS > &ypr )
	{
		memcpy( ypr.buff_, is.retrieve( BYTES ), BYTES );
		return is;
	}

	friend BinaryOStream& operator<<( BinaryOStream& os,
		const PackedYawPitchRoll< HALFPITCH, YAWBITS, PITCHBITS, ROLLBITS >
			&ypr )
	{
		memcpy( os.reserve( BYTES ), ypr.buff_, BYTES );
		return os;
	}

private:
	char buff_[ BYTES ];
};



// -----------------------------------------------------------------------------
// Section: class PackedGroundPos
// -----------------------------------------------------------------------------
// Predeclaration so the compiler knows the friend function is a template
template< int EXPONENT_BITS, int MANTISSA_BITS >
class PackedGroundPos;

template< int EXPONENT_BITS, int MANTISSA_BITS >
BinaryIStream& operator>>( BinaryIStream& is,
	PackedGroundPos< EXPONENT_BITS, MANTISSA_BITS > &xz );

template< int EXPONENT_BITS, int MANTISSA_BITS >
BinaryOStream& operator<<( BinaryOStream& os,
	const PackedGroundPos< EXPONENT_BITS, MANTISSA_BITS > & xz );

/**
 *	This class is used to store a packed x and z coordinate.
 */
template< int EXPONENT_BITS = EXPONENTBITS_XZ,
	int MANTISSA_BITS = XZ_MANTISSABITS_XZ >
class PackedGroundPos
{
	// X and Z are each EXPONENT + MANTISSA + 1 (for the sign)
	static const int BITS = (EXPONENT_BITS + MANTISSA_BITS + 1) * 2;
	static const int BYTES = ((BITS - 1) / 8) + 1;
public:
	static float maxLimit( float scale );
	static float minLimit( float scale );

	PackedGroundPos(){}
	PackedGroundPos( float x, float z, float scale );

	inline void packXZ( float x, float z, float scale );
	inline void unpackXZ( float & x, float & z, float scale ) const;

	inline void getXZError( float & xError, float & zError,
			float scale ) const;

	friend BinaryIStream& operator>> <>( BinaryIStream& is,
		PackedGroundPos< EXPONENT_BITS, MANTISSA_BITS > &xz );

	friend BinaryOStream& operator<< <>( BinaryOStream& os,
		const PackedGroundPos< EXPONENT_BITS, MANTISSA_BITS > & xz );

private:
	unsigned char buff_[ BYTES ];
};



// -----------------------------------------------------------------------------
// Section: class PackedFullPos
// -----------------------------------------------------------------------------
// Predeclaration so the compiler knows the friend function is a template
template< int EXPONENT_BITS, int MANTISSA_BITS, int EXPONENT_BITS_Y,
	int MANTISSA_BITS_Y >
class PackedFullPos;

template< int EXPONENT_BITS, int MANTISSA_BITS, int EXPONENT_BITS_Y,
	int MANTISSA_BITS_Y >
BinaryIStream& operator>>( BinaryIStream& is,
	PackedFullPos< EXPONENT_BITS, MANTISSA_BITS, EXPONENT_BITS_Y,
	MANTISSA_BITS_Y > &xyz );

template< int EXPONENT_BITS, int MANTISSA_BITS, int EXPONENT_BITS_Y,
	int MANTISSA_BITS_Y >
BinaryOStream& operator<<( BinaryOStream& os,
	const PackedFullPos< EXPONENT_BITS, MANTISSA_BITS, EXPONENT_BITS_Y,
	MANTISSA_BITS_Y > & xyz );

/**
 *	This class is used to store a packed x, y and z coordinate
 */
template< int EXPONENT_BITS = EXPONENTBITS_XZ,
	int MANTISSA_BITS = XYZ_MANTISSABITS_XZ,
	int EXPONENT_BITS_Y = XYZ_EXPONENTBITS_Y,
	int MANTISSA_BITS_Y = XYZ_MANTISSABITS_Y >
class PackedFullPos
{
	// X, Y and Z are each EXPONENT + MANTISSA + 1 (for the sign)
	static const int BITS = ((EXPONENT_BITS + MANTISSA_BITS + 1) * 2) +
		(EXPONENT_BITS_Y + MANTISSA_BITS_Y + 1);
	static const int BYTES = ((BITS - 1) / 8) + 1;

public:
	static float maxLimit( float xzscale );
	static float minLimit( float xzscale );

	static float maxYLimit();
	static float minYLimit();

	PackedFullPos(){}
	PackedFullPos( float x, float y, float z, float xzscale );

	inline void packXYZ( float x, float y, float z, float xzscale );
	inline void unpackXYZ( float & x, float & y, float & z,
		float xzscale ) const;

	inline void getXYZError( float & xError, float & yError, float & zError,
			float xzscale ) const;

	friend BinaryIStream& operator>> <>( BinaryIStream& is,
		PackedFullPos< EXPONENT_BITS, MANTISSA_BITS, EXPONENT_BITS_Y,
			MANTISSA_BITS_Y > &xyz );

	friend BinaryOStream& operator<< <>( BinaryOStream& os,
		const PackedFullPos< EXPONENT_BITS, MANTISSA_BITS, EXPONENT_BITS_Y,
		MANTISSA_BITS_Y > & xyz );

private:
	unsigned char buff_[ BYTES ];
};


// -----------------------------------------------------------------------------
// Section: Typedefs
// -----------------------------------------------------------------------------
typedef uint8 IDAlias;

// We will almost always want the default template parameter versions.
typedef PackedYaw<> Yaw;
typedef PackedYawPitch<> YawPitch;
typedef PackedYawPitchRoll<> YawPitchRoll;

typedef PackedGroundPos<> PackedXZ;
typedef PackedFullPos<> PackedXYZ;


#if !VOLATILE_POSITIONS_ARE_ABSOLUTE
// -----------------------------------------------------------------------------
// Section: ReferencePosition
// -----------------------------------------------------------------------------


/**
 *	This method returns the reference position associated with the input
 *	position.
 *
 *	The reason that it is important to round the reference position is that if
 *	the reference position changes by less than the least accurate offset from
 *	this position, entities that are meant to be stationary will move as the
 *	reference position moves.
 */
inline Vector3 calculateReferencePosition( const Vector3 & pos )
{
	return Vector3( BW_ROUNDF( pos.x ), BW_ROUNDF( pos.y ),
		BW_ROUNDF( pos.z ) );
}

#endif /* !VOLATILE_POSITIONS_ARE_ABSOLUTE */

// -----------------------------------------------------------------------------
// Section: Clean up Configuration macros
// -----------------------------------------------------------------------------
// Note that VOLATILE_POSITIONS_ARE_ABSOLUTE is exported from this header.
#undef EXPONENTBITS_XZ
#undef XZ_MANTISSABITS_XZ
#undef XYZ_MANTISSABITS_XZ
#undef XYZ_EXPONENTBITS_Y
#undef XYZ_MANTISSABITS_Y
#undef YAW_YAWBITS
#undef YAWPITCH_YAWBITS
#undef YAWPITCH_PITCHBITS
#undef YAWPITCH_HALFPITCH
#undef YAWPITCHROLL_YAWBITS
#undef YAWPITCHROLL_PITCHBITS
#undef YAWPITCHROLL_ROLLBITS
#undef YAWPITCHROLL_HALFPITCH

// -----------------------------------------------------------------------------
// Section: Template bodies
// -----------------------------------------------------------------------------
#include "msgtypes.ipp"

BW_END_NAMESPACE

#endif // MSGTYPES_HPP
