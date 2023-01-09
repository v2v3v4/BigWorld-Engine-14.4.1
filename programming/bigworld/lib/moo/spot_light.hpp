#ifndef SPOT_LIGHT_HPP
#define SPOT_LIGHT_HPP

#include "moo_math.hpp"
#include "cstdmf/guard.hpp"
#include "cstdmf/stdmf.hpp"
#include "cstdmf/smartpointer.hpp"
#include "cstdmf/vectornodest.hpp"
#include "math/boundbox.hpp"


BW_BEGIN_NAMESPACE

class BoundingBox;

namespace Moo {

class SpotLight;
typedef SmartPointer< SpotLight > SpotLightPtr;

/**
* This class represents an spot light with a colour, position, direction and a
* circular frustum.
*/
class SpotLight : public SafeReferenceCount
{
public:
	//-- for instanced rendering with 16 byte aligning.
	struct GPU
	{
		Vector4 m_pos;			//-- xyz
		Vector4 m_color;		//-- xyzw
		Vector4 m_attenuation;	//-- xyz
		Vector4 m_dir;			//-- xyz
	};

public:
	SpotLight();
	SpotLight( const Colour& colour, const Vector3& position, 
		const Vector3& direction, float innerRadius, float outerRadius, 
		float cosConeAngle );
	~SpotLight();

	const Vector3&		position( ) const;
	void				position( const Vector3& position );
	const Vector3&		direction( ) const;
	void				direction( const Vector3& direction );
	float				innerRadius( ) const;
	void				innerRadius( float innerRadius );
	float				outerRadius( ) const;
	void				outerRadius( float outerRadius );

	float				cosConeAngle( ) const;
	void				cosConeAngle( float cosConeAngle );
	const Colour&		colour( ) const;
	void				colour( const Colour& colour );

	void				worldTransform( const Matrix& transform );

	GPU					gpu( ) const;
	const BoundingBox&	worldBounds( ) const;
	const Vector3&		worldPosition( ) const;
	const Vector3&		worldDirection( ) const;
	float				worldInnerRadius( ) const;
	float				worldOuterRadius( ) const;

	bool				intersects( const BoundingBox& worldSpaceBB ) const;
	float				attenuation( const BoundingBox& worldSpaceBB ) const;

	Vector4*			getTerrainLight( uint32 timestamp, float lightScale_ );

#ifdef EDITOR_ENABLED
	float				multiplier() const { return multiplier_; }
	void				multiplier( float m ) { multiplier_ = m; }
#else//EDITOR_ENABLED
	float				multiplier() const { return 1.0f; }
#endif

	int					priority() const { return priority_; }
	void				priority(int b) { priority_ = b; }

private:
	void				createTerrainLight( float lightScale );	
	void				updateInternalBounds() const;

	mutable bool		dirty_;	

	Vector3				position_;
	Vector3				direction_;
	float				innerRadius_;
	float				outerRadius_;
	float				cosConeAngle_;
	Colour				colour_;

	Vector3				worldPosition_;
	Vector3				worldDirection_;
	float				worldInnerRadius_;
	float				worldOuterRadius_;

	uint32				terrainTimestamp_;
	Vector4				terrainLight_[4];

	int					priority_;
	mutable Matrix		lightView_;
	mutable BoundingBox lightBounds_;

#ifdef EDITOR_ENABLED
	float				multiplier_;
#endif

};
} // namespace Moo

typedef VectorNoDestructor< Moo::SpotLightPtr > SpotLightVector;

#ifdef CODE_INLINE
#include "spot_light.ipp"
#endif

BW_END_NAMESPACE


#endif // SPOT_LIGHT_HPP

