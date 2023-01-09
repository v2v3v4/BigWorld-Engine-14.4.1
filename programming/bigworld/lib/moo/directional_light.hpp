#ifndef DIRECTIONAL_LIGHT_HPP
#define DIRECTIONAL_LIGHT_HPP

#include <iostream>

#include "cstdmf/stdmf.hpp"
#include "cstdmf/smartpointer.hpp"
#include "cstdmf/vectornodest.hpp"

#include "moo_math.hpp"


BW_BEGIN_NAMESPACE

namespace Moo {

class DirectionalLight;
typedef SmartPointer< DirectionalLight > DirectionalLightPtr;

/**
 * A directional light source (ala sunlight).
 */
class DirectionalLight : public SafeReferenceCount
{
public:
	DirectionalLight();
	DirectionalLight( const Colour& colour, const Vector3& direction_ );
	~DirectionalLight();

	const Vector3&		direction( ) const;
	void				direction( const Vector3& direction );

	const Colour&		colour( ) const;
	void				colour( const Colour& colour );

#ifdef EDITOR_ENABLED
	float				multiplier() const { return multiplier_; }
	void				multiplier( float m ) { multiplier_ = m; }
#else//EDITOR_ENABLED
	float				multiplier() const { return 1.0f; }
#endif


	void				worldTransform( const Matrix& transform );

	const Vector3&		worldDirection( ) const;
private:

	Vector3				direction_;
	Vector3				worldDirection_;

	Colour				colour_;

#ifdef EDITOR_ENABLED
	float				multiplier_;
#endif


// Allow the default copy constructor and assignment operator
//	DirectionalLight(const DirectionalLight&);
//	DirectionalLight& operator=(const DirectionalLight&);

	friend std::ostream& operator<<(std::ostream&, const DirectionalLight&); 
};

} // namespace Moo

typedef VectorNoDestructor< Moo::DirectionalLightPtr > DirectionalLightVector;

#ifdef CODE_INLINE
#include "directional_light.ipp"
#endif

BW_END_NAMESPACE


#endif // DIRECTIONAL_LIGHT_HPP
