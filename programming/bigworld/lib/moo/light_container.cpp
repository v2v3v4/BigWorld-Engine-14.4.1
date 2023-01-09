#include "pch.hpp"

#include "light_container.hpp"
#include "render_context.hpp"
#include "math/boundbox.hpp"


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "light_container.ipp"
#endif

namespace Moo
{

/**
 *	Constructor
 */
LightContainer::LightContainer( const LightContainerPtr & pLC, const BoundingBox& bb, bool limitToRenderable )
{
	BW_GUARD;
	init( pLC, bb, limitToRenderable );
}

/**
 *	Constructor
 */
LightContainer::LightContainer()
:	ambientColour_( 0.1f, 0.1f, 0.1f, 0 )
{
}


/**
 *	Helper class used to sort the omni lights by their attenuation.
 */
class SortOmnis
{
public:
	SortOmnis( const BoundingBox& bb )
	: bb_( bb )
	{
	}

	bool operator () ( OmniLightPtr& p1, OmniLightPtr& p2 )
	{
		if (p1->priority() != p2->priority())
		{
			return p1->priority() > p2->priority();
		}
		else
		{
			return p1->attenuation( bb_ ) > p2->attenuation( bb_ );
		}
	}

	BoundingBox bb_;
};

/**
 *	Helper class used to sort spot lights by their attenuation and cone.
 */
class SortSpots
{
public:
	SortSpots( const BoundingBox& bb )
	: bb_( bb )
	{
	}

	bool operator () ( SpotLightPtr& p1, SpotLightPtr& p2 )
	{
		if (p1->priority() != p2->priority())
		{
			return p1->priority() > p2->priority();
		}
		else
		{
			return p1->attenuation( bb_ ) > p2->attenuation( bb_ );
		}
	}

	BoundingBox bb_;
};


void LightContainer::assign( const LightContainerPtr & pLC )
{
	ambientColour_ = pLC->ambientColour_;
	spotLights_.assign( pLC->spotLights_.begin(), pLC->spotLights_.end() );
	omniLights_.assign( pLC->omniLights_.begin(), pLC->omniLights_.end() );
	directionalLights_.assign( pLC->directionalLights_.begin(), pLC->directionalLights_.end() );
}

void LightContainer::clear( )
{
	ambientColour_ = Colour(0,0,0,1);
	spotLights_.clear();
	omniLights_.clear();
	directionalLights_.clear();
}

void LightContainer::init( const LightContainerPtr & pLC, const BoundingBox& bb, bool limitToRenderable )
{
	BW_GUARD_PROFILER( LightContainer_init );

	if( pLC )
	{

		ambientColour_ = pLC->ambientColour_;

		directionalLights_ = pLC->directionalLights_;
		while (limitToRenderable && directionalLights_.size() > 2)
		{
			directionalLights_.pop_back();
		}

		omniLights_.clear();

		OmniLightVector::iterator it = pLC->omniLights_.begin();
		OmniLightVector::iterator end = pLC->omniLights_.end();

		while (it != end)
		{
			if ((*it)->intersects( bb ))
			{
				omniLights_.push_back( *it );
			}
			++it;
		}

		std::sort( omniLights_.begin(), omniLights_.end(), SortOmnis(bb) );
		if (limitToRenderable)
		{
			if (omniLights_.size() > 4 )
				omniLights_.resize( 4 );
		}

		spotLights_.clear();
		for (uint32 i = 0; i < pLC->nSpots(); i++)
		{
			const SpotLightPtr& pSpot = pLC->spot( i );

			if( pSpot->intersects( bb ) )
			{
				spotLights_.push_back( pSpot );
			}
		}

		std::sort( spotLights_.begin(), spotLights_.end(), SortSpots(bb) );
		if (limitToRenderable)
		{
			if (spotLights_.size() > 2 )
				spotLights_.resize( 2 );
		}
	}
	else
	{
		ambientColour_ = Colour( 0, 0, 0, 0 );
		directionalLights_.clear();
		omniLights_.clear();
		spotLights_.clear();
	}
}

void LightContainer::addToSelfOnlyVisibleLights(const LightContainerPtr & pLC, const Matrix& camera)
{
	BW_GUARD;
	if (pLC)
	{
		for (uint32 i = 0; i < pLC->nOmnis(); ++i)
		{
			const OmniLightPtr& pOmni = pLC->omni(i);
			if (std::find(omniLights_.begin(), omniLights_.end(), pOmni) == omniLights_.end())
			{
				//-- resolve light visibility.
				pOmni->worldBounds().calculateOutcode(camera);
				if (pOmni->worldBounds().combinedOutcode())
					continue;

				omniLights_.push_back(pOmni);
			}
		}

		for (uint32 i = 0; i < pLC->nSpots(); ++i)
		{
			const SpotLightPtr& pSpot = pLC->spot(i);
			if (std::find(spotLights_.begin(), spotLights_.end(), pSpot) == spotLights_.end())
			{
				//-- resolve light visibility.
				pSpot->worldBounds().calculateOutcode(Moo::rc().viewProjection());
				if (pSpot->worldBounds().combinedOutcode())
					continue;

				spotLights_.push_back(pSpot);
			}
		}
	}
}


/**
 *	Copy these lights into our own container.
 */
void LightContainer::addToSelf( const LightContainerPtr & pLC, bool addOmnis, bool addSpots, bool addDirectionals  )
{
	BW_GUARD;
	if (pLC)
	{
		if (addDirectionals)
		{
			for ( uint32 i = 0; i < pLC->nDirectionals(); i++ )
			{
				const DirectionalLightPtr& pDir = pLC->directional( i );
				if (std::find( this->directionalLights_.begin(),
							this->directionalLights_.end(), pDir ) == this->directionalLights_.end() )
				{
					directionalLights_.push_back( pDir );
				}
			}
		}

		if (addOmnis)
		{
			for ( uint32 i = 0; i < pLC->nOmnis(); i++ )
			{
				const OmniLightPtr& pOmni = pLC->omni( i );
				if (std::find( this->omniLights_.begin(),
							this->omniLights_.end(), pOmni ) == this->omniLights_.end() )
				{
					omniLights_.push_back( pOmni );
				}
			}
		}

		if (addSpots)
		{
			for ( uint32 i = 0; i < pLC->nSpots(); i++ )
			{
				const SpotLightPtr& pSpot = pLC->spot( i );
				if (std::find( this->spotLights_.begin(),
							this->spotLights_.end(), pSpot ) == this->spotLights_.end() )
				{
					spotLights_.push_back( pSpot );
				}
			}
		}
	}
}

/**
 *	Copy these lights into our own container.
 */
void LightContainer::addToSelf( const LightContainerPtr & pLC, const BoundingBox & bb,
	bool limitToRenderable )
{
	BW_GUARD;
	if (pLC)
	{
		for (uint32 i = 0; ( i < pLC->nDirectionals() ) && !( limitToRenderable && directionalLights_.size() > 2 ) ; i++ )
		{
			const DirectionalLightPtr& pDir = pLC->directional( i );
			bool good = true;
			for (uint32 j=0; j < this->nDirectionals(); j++)
			{
				if (this->directional(j) == pDir) good = false;
			}
			if (!good) continue;
			directionalLights_.push_back( pDir );
		}

		for (uint32 i = 0; i < pLC->nOmnis(); i++ )
		{
			const OmniLightPtr& pOmni = pLC->omni( i );

			if( pOmni->intersects( bb ))
			{
				bool good = true;
				for (uint32 j=0; j < this->nOmnis(); j++)
				{
					if (this->omni(j) == pOmni) good = false;
				}
				if (!good) continue;
				omniLights_.push_back( pOmni );
			}
		}

		std::sort( omniLights_.begin(), omniLights_.end(), SortOmnis(bb) );
		if (limitToRenderable)
		{
			if (omniLights_.size() > 4 )
				omniLights_.resize( 4 );
		}

		for (uint32 i = 0; i < pLC->nSpots(); i++ )
		{
			const SpotLightPtr& pSpot = pLC->spot( i );

			if( pSpot->intersects( bb ) )
			{
				bool good = true;
				for (uint32 j=0; j < this->nSpots(); j++)
				{
					if (this->spot(j) == pSpot) good = false;
				}
				if (!good) continue;
				spotLights_.push_back( pSpot );
			}
		}

		std::sort( spotLights_.begin(), spotLights_.end(), SortSpots(bb) );
		if (limitToRenderable)
		{
			if (spotLights_.size() > 2 )
				spotLights_.resize( 2 );
		}
	}
}

// Helper function for adding vertex shader constants incrementally
static inline void addVertexShaderConstant( int& constantIndex, const float* constants, int count )
{
	BW_GUARD;
	Moo::rc().device()->SetVertexShaderConstantF(  constantIndex, constants,  count );
	constantIndex += count;
}


/**
 *	Adds a maximum of two omnis in world space to the space set aside for spot lights.
 */
void LightContainer::addExtraOmnisInWorldSpace( ) const
{
	BW_GUARD;
	uint8	nPointLights = 0;
	int constCount = 33;

	// iterate through and add omni lights.
	// TODO: cull them, probably not the place to do it though...

	for( OmniLightVector::const_iterator it = omniLights_.begin(); ( it != omniLights_.end() ) && ( nPointLights < 4 ); it++ )
	{
		const OmniLightPtr omni = *it;
//		if( omni->intersects( bb ) )
		{
			Vector4 position;
			addVertexShaderConstant( constCount, (const float*)&omni->worldPosition(),  1 );
			Colour c = omni->colour() * omni->multiplier();
			addVertexShaderConstant( constCount, (const float*)&c,  1 );

			float innerRadius = omni->worldInnerRadius();
			float outerRadius = omni->worldOuterRadius();

			addVertexShaderConstant( constCount, (const float*)Vector4( outerRadius, 1.f / ( outerRadius - innerRadius ), 0, 0 ),  1 );

			nPointLights++;
		}
	}
}

/**
 *	Adds a maximum of two omnis in model space to the space set aside for spot lights.
 *
 *	@param invWorld the inverse world transform, used to transform lights to object space.
 */
void LightContainer::addExtraOmnisInModelSpace( const Matrix& invWorld ) const
{
	BW_GUARD;
	uint8	nPointLights = 0;
	int constCount = 33;

	MF_ASSERT(invWorld.isUniformlyScaled());
	float invWorldScale = invWorld.uniformScale();

	// iterate through, transform and add omni lights.
	// TODO: cull them, probably not the place to do it though...

	for( OmniLightVector::const_iterator it = omniLights_.begin(); ( it != omniLights_.end() ) && ( nPointLights < 2 ); it++ )
	{
		OmniLightPtr omni = *it;
//		if( omni->intersects( bb ) )
		{
			Vector4 position;
			addVertexShaderConstant( constCount, (const float*)XPVec3Transform( &position, &omni->worldPosition(), &invWorld ),  1 );
			Colour c = omni->colour() * omni->multiplier();
			addVertexShaderConstant( constCount, (const float*)&c,  1 );

			float innerRadius = omni->worldInnerRadius() * invWorldScale;
			float outerRadius = omni->worldOuterRadius() * invWorldScale;

			addVertexShaderConstant( constCount, (const float*)Vector4( outerRadius, 1.f / ( outerRadius - innerRadius ), 0, 0 ),  1 );

			nPointLights++;
		}
	}
}

void LightContainer::commitToFixedFunctionPipeline()
{
	BW_GUARD;
	DX::Material d3dmaterial;
	d3dmaterial.Diffuse = Moo::Colour( 1, 1, 1, 1 );
	d3dmaterial.Ambient = Moo::Colour( 1, 1, 1, 1 );
	d3dmaterial.Specular = Moo::Colour( 0, 0, 0, 0 );
	d3dmaterial.Emissive = ambientColour();
	d3dmaterial.Power = 0;
	rc().device()->SetMaterial( &d3dmaterial );
    rc().setRenderState( D3DRS_LIGHTING, TRUE );

	int lightIndex = 0;

	//Add directional lights
	uint numDirectionals = static_cast<uint>( directionals().size() );
	uint i;
	for ( i = 0; i < numDirectionals; i++ )
	{
		DX::Light light;

		light.Type = D3DLIGHT_DIRECTIONAL;
		light.Diffuse = directionals()[i]->colour() * directionals()[i]->multiplier();
		light.Specular = Colour( 0, 0, 0, 0 );
		light.Ambient = Colour( 0, 0, 0, 0 );
		light.Position = Vector3( 0, 0, 0 );
		light.Direction = directionals()[i]->worldDirection();

		light.Attenuation0 = 1;
		light.Attenuation1 = 0;
		light.Attenuation2 = 0;
		light.Range = 0;

		rc().device()->SetLight( lightIndex, &light );
		rc().device()->LightEnable( lightIndex++, TRUE );
	}

	//Add spotlights
	uint numSpots = static_cast<uint>( spots().size() );
	for ( i = 0; i < numSpots; i++ )
	{
		DX::Light light;

		light.Type = D3DLIGHT_SPOT;
		light.Diffuse = spots()[i]->colour() * spots()[i]->multiplier();
		light.Specular = Colour( 0, 0, 0, 0 );
		light.Ambient = Colour( 0, 0, 0, 0 );
		light.Position = spots()[i]->worldPosition();
		light.Direction = spots()[i]->worldDirection();

		light.Attenuation0 = 1;
		light.Attenuation1 = 0;
		light.Attenuation2 = 0;
		light.Range = 0;
		light.Theta = acosf( spots()[i]->cosConeAngle() ) * 2.f;
		light.Phi = acosf( spots()[i]->cosConeAngle() ) * 2.f;

		rc().device()->SetLight( lightIndex, &light );
		rc().device()->LightEnable( lightIndex++, TRUE );
	}

	//Add omnis
	uint numOmnis = static_cast<uint>( omnis().size() );
	for ( i = 0; i < numOmnis; i++ )
	{
		DX::Light light;

		light.Type = D3DLIGHT_POINT;
		light.Diffuse = omnis()[i]->colour() * omnis()[i]->multiplier();
		light.Specular = Colour( 0, 0, 0, 0 );
		light.Ambient = Colour( 0, 0, 0, 0 );
		light.Position = omnis()[i]->worldPosition();
		light.Direction = Vector3( 0, 0, 0 );
		light.Attenuation0 = 1;
		light.Attenuation1 = 0;
		light.Attenuation2 = 0;
		light.Range = omnis()[i]->outerRadius();

		rc().device()->SetLight( lightIndex, &light );
		rc().device()->LightEnable( lightIndex++, TRUE );
	}

	//disable the next light, and all lights following.
	rc().device()->LightEnable( lightIndex, FALSE );
}


/**
 *	Destructor
 */
LightContainer::~LightContainer()
{
}

} // namespace Moo

BW_END_NAMESPACE

// light_container.cpp
