#ifndef PARTICLE_HPP
#define PARTICLE_HPP

#include "math/vector3.hpp"
#include "math/vector4.hpp"
#include "math/colour.hpp"


BW_BEGIN_NAMESPACE

//calculated num constants in vshader for a hard-skinned mesh (no texture transforms):
// 4		WVP
// 2		fog
// 2		sky light mapping (world space style only)
// 1		ambient light
// 4		directional lights
//12		point lights
// 8		spot lights
//--
//33
//
//so 96 - 33 is 63 constants left
//
//num constants required per mesh particle
// 3		world transform
// 1		colour
//
//thus you've got 63 / 4 which is 15
const int PARTICLE_MAX_MESHES = 15;	//limited by number of constants in vshader.

const float PARTICLE_MESH_MAX_SPIN = 20.f;	//since the spin value is clamped 
									// 0..1, this multiplier allows greater spin 
									// speeds.

// Pack as tightly as possible.
#pragma pack(push, 1 )

/**
 *	This public class stores the information unique to each individual
 *	particle.
 */
class Particle
{
public:
	struct PositionColour
	{
		Vector3			position_;	///< Position, units in metres.								[12 bytes]
		uint32			colour_;	///< Shade/Tint with R,G,B and Alpha.						[4 bytes]
	};

	Particle()
	{
		this->kill();
	};

	Particle
    ( 
        const Vector3   &position,
		const Vector3   &v,
		const Vector4   &colour,
		float           sz,
		float           startingPitch   = 0.f,
		float           startingYaw     = 0.f,
		float           startingAge     = 0.0f 
    );                                          // non mesh particles only

	Particle
    ( 
        const Vector3   &position,
		const Vector3   &v,
		float           startingPitch   = 0.0f,
		float           startingYaw     = 0.0f,		
        const Vector3   &spinAxis       = Vector3(),
        float           spinSpeed       = 0.0f,
        float           startingAge     = 0.0f
    );                                          // mesh particles only

	static bool sortParticlesReverse( const Particle& p1, const Particle& p2 );

	static size_t sizeInBytes( void );

	static float sizeRawToFloat( uint16 sizeRaw );

    Vector3 const &position() const;            // all particles
    Vector3 &position();                        // all particles

	PositionColour* positionColour();			// all particles

	void getVelocity( Vector3& ret ) const; // all particles
	void setVelocity( const Vector3& v );       // all particles
	void setXVelocity( float x );               // all particles
	void setYVelocity( float y );               // all particles
	void setZVelocity( float z );               // all particles

	float size() const;                         // non mesh particles only
	void size( float f );                       // non mesh particles only

	uint16 sizeRaw() const;					    // non mesh particles only

	bool isAlive() const;						// all particles
	void kill();								// all particles
	float age() const;                          // all particles
	void age( float f );                        // all particles
	static uint16 nAgeIncrements( float f );	// conversion helper
	static float age( uint16 accurateAge );		// conversion helper
	static uint16 ageMax();						// get the maximum particle age
	uint16 ageAccurate() const;					// all particles
	void ageAccurate( uint16 a );				// all particles

	float pitch() const;                        // all particles
    void pitch( float p );                      // all particles

    float yaw() const;                          // all particles
    void yaw( float y );                        // all particles

    float meshSpinAge() const;                // mesh particles only
    void meshSpinAge(float s);                // mesh particles only

    Vector3 meshSpinAxis() const;                // mesh particles only
    void meshSpinAxis( const Vector3 & dir );    // mesh particles only

    float meshSpinSpeed() const;                // mesh particles only
    void meshSpinSpeed( float amount );         // mesh particles only

    uint32 colour() const;                      // all particles
    void colour(uint32 clr);                    // all particles

    uint16 index() const;                       // non mesh particles only
    void index(uint16 idx);                     // non mesh particles only

    uint16 distance() const;                    // non mesh particles only
    void distance(uint16 dist);                 // non mesh particles only

private:
    struct MeshData
    {
        uint16  meshSpinAge_;	///< For calculating total spin, set on Collide.
        uint8   spinSpeed_;		///< Rate of spin (revs/s)
        uint8   spinAxisX_;		///< Axis of spin.
        uint8   spinAxisY_;
        uint8   spinAxisZ_;
    };

    struct NonMeshData
    {
        uint16  size_;
        uint16  idx_;
        uint16  distance_;
    };

    union ExtraData
    {
        MeshData    meshData_;
        NonMeshData nonMeshData_;
    };

	PositionColour	positionColour_;

	int16			xVel_;		///< Packed velocity in 256 metres / sec increments			[6 bytes]
	int16			yVel_;		///< Packed velocity in 256 metres / sec increments
	int16			zVel_;		///< Packed velocity in 256 metres / sec increments
	uint16			age_;		///< Total time updated in seconds.							[2 bytes]
	uint16			pitchYaw_;	///< Used for storing rotational data. 						[2 bytes]
			    				///( when a particle bounces, it can have one of 256 directions in pitch and yaw. )
    ExtraData		extraData_; ///< idx, distance for non mesh, packed angle info for meshes [6 bytes]
};

// Go back to default packing.
#pragma pack( pop )


#ifdef CODE_INLINE
#include "particle.ipp"
#endif

BW_END_NAMESPACE

#endif // PARTICLE_HPP
