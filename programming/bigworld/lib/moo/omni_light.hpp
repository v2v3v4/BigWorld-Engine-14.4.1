#ifndef OMNI_LIGHT_HPP
#define OMNI_LIGHT_HPP

#include <iostream>
#include "moo_math.hpp"
#include "cstdmf/guard.hpp"
#include "cstdmf/stdmf.hpp"
#include "cstdmf/smartpointer.hpp"
#include "cstdmf/vectornodest.hpp"
#include "math/boundbox.hpp"


BW_BEGIN_NAMESPACE

class BoundingBox;
namespace Moo {

class OmniLight;
typedef SmartPointer< OmniLight > OmniLightPtr;

/**
 * This class represents an omni-directional light with a colour, position and
 * an attenuation range.
 */
class OmniLight : public SafeReferenceCount
{
public:
	//-- for instanced rendering with 16 byte aligning.
	struct GPU
	{
		Vector4	m_pos;			//-- xyz
		Vector4	m_color;		//-- xyzw
		Vector4	m_attenuation;  //-- xy
		Vector4	m_padding;		//-- padding
	};

	OmniLight();
	OmniLight( const D3DXCOLOR& colour, const Vector3& position, float innerRadius, float outerRadius );
	~OmniLight();

	const Vector3&		position( ) const;
	void				position( const Vector3& position );
	float				innerRadius( ) const;
	void				innerRadius( float innerRadius );
	float				outerRadius( ) const;
	void				outerRadius( float outerRadius );

	const Colour&		colour( ) const;
	void				colour( const Colour& colour );

	void				worldTransform( const Matrix& transform );

	GPU					gpu( ) const;
	const BoundingBox&	worldBounds( ) const;
	const Vector3&		worldPosition( void ) const;
	float				worldInnerRadius( void ) const;
	float				worldOuterRadius( void ) const;

	bool				intersects( const BoundingBox& worldSpaceBB ) const;

	Vector4*		getTerrainLight( uint32 timestamp, float lightScale );

	float				attenuation( const BoundingBox& worldSpaceBB ) const;

#ifdef EDITOR_ENABLED
	float				multiplier() const { return multiplier_; }
	void				multiplier( float m ) { multiplier_ = m; }
#else//EDITOR_ENABLED
	float				multiplier() const { return 1.0f; }
#endif

	int					priority() const { return priority_; }
	void				priority(int b) { priority_ = b; }


protected:
	void				createTerrainLight( float lightScale );

	BoundingBox			worldBounds_;
	Vector3				position_;
	float				innerRadius_;
	float				outerRadius_;
	Colour				colour_;

	Vector3				worldPosition_;
	float				worldInnerRadius_;
	float				worldOuterRadius_;

	uint32				terrainTimestamp_;
	Vector4				terrainLight_[3];

	int					priority_;

#ifdef EDITOR_ENABLED
	float				multiplier_;
#endif

// Allow the default copy constructor and assignment operator
//	OmniLight(const OmniLight&);
//	OmniLight& operator=(const OmniLight&);

	friend std::ostream& operator<<(std::ostream&, const OmniLight&);
};
}
//typedef BW::vector< Moo::OmniLightPtr > OmniLightVector;
typedef VectorNoDestructor< Moo::OmniLightPtr > OmniLightVector;

#ifdef CODE_INLINE
#include "omni_light.ipp"
#endif

BW_END_NAMESPACE


#endif // OMNI_LIGHT_HPP

