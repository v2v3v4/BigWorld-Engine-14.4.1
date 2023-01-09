#ifndef LIGHT_CONTAINER_HPP
#define LIGHT_CONTAINER_HPP

#include <iostream>

#include "moo_math.hpp"
#include "directional_light.hpp"
#include "omni_light.hpp"
#include "spot_light.hpp"

#include "cstdmf/guard.hpp"
#include "cstdmf/smartpointer.hpp"


BW_BEGIN_NAMESPACE

namespace Moo
{

typedef SmartPointer< class LightContainer > LightContainerPtr;

/**
 *	A bucket of lights grouped to light regions of a world.
 *	Contains helper methods to sort, setup and transform the lights for 
 *	use.
 */
class LightContainer : public SafeReferenceCount
{
public:
	LightContainer( const LightContainerPtr & pLC, const BoundingBox& bb, bool limitToRenderable = false);
	LightContainer();
	~LightContainer();

	void							assign( const LightContainerPtr & pLC );
	void							clear();

	void							init( const LightContainerPtr & pLC, const BoundingBox& bb, 
		bool limitToRenderable );

	void							addToSelfOnlyVisibleLights(const LightContainerPtr & pLC, const Matrix& camera);
	void							addToSelf( const LightContainerPtr & pLC, bool addOmnis=true, bool addSpots=true, bool addDirectionals=true  );
	void							addToSelf( const LightContainerPtr & pLC, const BoundingBox& bb,
		bool limitToRenderable );

	const Colour&					ambientColour( ) const;
	void							ambientColour( const Colour& colour );

	const DirectionalLightVector&	directionals( ) const;
	DirectionalLightVector&			directionals( );
	bool							addDirectional( const DirectionalLightPtr & pDirectional, bool checkExisting=false );
	bool							delDirectional( const DirectionalLightPtr & pDirectional );
	uint32							nDirectionals( ) const;
	const DirectionalLightPtr&		directional( uint32 i ) const;

	const OmniLightVector&			omnis( ) const;
	OmniLightVector&				omnis( );
	bool							addOmni( const OmniLightPtr & pOmni, bool checkExisting=false );
	bool							delOmni( const OmniLightPtr & pOmni );
	uint32							nOmnis( ) const;
	const OmniLightPtr&				omni( uint32 i ) const;

	const SpotLightVector&			spots( ) const;
	SpotLightVector&				spots( );
	bool							addSpot( const SpotLightPtr & pSpot, bool checkExisting=false );
	bool							delSpot( const SpotLightPtr & pSpot );
	uint32							nSpots( ) const;
	const SpotLightPtr&				spot( uint32 i ) const;

	bool							addLight( const OmniLightPtr & pOmni, bool checkExisting=false );
	bool							delLight( const OmniLightPtr & pOmni );
	bool							addLight( const SpotLightPtr & pSpot, bool checkExisting=false );
	bool							delLight( const SpotLightPtr & pSpot );
	bool							addLight( const DirectionalLightPtr & pDir, bool checkExisting=false );
	bool							delLight( const DirectionalLightPtr & pDir );

	void							addExtraOmnisInWorldSpace( ) const;
	void							addExtraOmnisInModelSpace( const Matrix& invWorld ) const;

	static void						addLightsInWorldSpace( );
	static void						addLightsInModelSpace( const Matrix& invWorld );

	void							commitToFixedFunctionPipeline();

	void							operator=( const LightContainerPtr& pLC );

private:

	Colour					ambientColour_;

	DirectionalLightVector	directionalLights_;
	OmniLightVector			omniLights_;
	SpotLightVector			spotLights_;

/*	LightContainer(const LightContainer&);
	LightContainer& operator=(const LightContainer&);*/

	friend std::ostream& operator<<(std::ostream&, const LightContainer&);
};

}

#ifdef CODE_INLINE
#include "light_container.ipp"
#endif

BW_END_NAMESPACE


#endif // LIGHT_CONTAINER_HPP
