#ifndef SUN_AND_MOON_HPP
#define SUN_AND_MOON_HPP

#include <iostream>
#include "moo/custom_mesh.hpp"
#include "lens_effect.hpp"
#include "moo/effect_material.hpp"


BW_BEGIN_NAMESPACE

class TimeOfDay;
typedef SmartPointer< class ShaderSet > ShaderSetPtr;

/**
 * TODO: to be documented.
 */
class SunAndMoon
{
public:
	SunAndMoon();
	~SunAndMoon();

	void				timeOfDay( TimeOfDay* timeOfDay )		{ timeOfDay_ = timeOfDay; }

	void				create( void );
	void				destroy( void );

	void				update();
	void				draw();	

private:
	void				createMoon( void );
	void				createSun( void );

	void drawSquareMesh( const Matrix & world, Vector4 color, CustomMesh<Moo::VertexXYZNDUV> * pMesh, Moo::EffectMaterialPtr pMat );

	SunAndMoon(const SunAndMoon&);
	SunAndMoon& operator=(const SunAndMoon&);

	friend std::ostream& operator<<(std::ostream&, const SunAndMoon&);

	CustomMesh<Moo::VertexXYZNDUV>*	moon_;
	CustomMesh<Moo::VertexXYZNDUV>*	sun_;

	Moo::EffectMaterialPtr	moonMat_;
	Moo::EffectMaterialPtr	sunMat_;	

	TimeOfDay*			timeOfDay_;

	LensEffect			sunLensEffect_;	
};

#ifdef CODE_INLINE
#include "sun_and_moon.ipp"
#endif

BW_END_NAMESPACE


#endif
/*sun_and_moon.hpp*/
