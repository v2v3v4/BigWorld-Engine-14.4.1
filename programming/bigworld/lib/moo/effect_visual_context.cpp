#include "pch.hpp"

#include "effect_visual_context.hpp"
#include "render_context_callback.hpp"
#include "texture_lock_wrapper.hpp"
#include "fog_helper.hpp"

#include "cstdmf/stdmf.hpp"

DECLARE_DEBUG_COMPONENT2( "Moo", 0 )


BW_BEGIN_NAMESPACE

BW_SINGLETON_STORAGE( Moo::EffectVisualContext );


namespace
{

	//-- Debug lighting visualization.
	bool g_showAmbientTerm  = true;
	bool g_showDiffuseTerm  = true;
	bool g_showSpecularTerm = true;

	//-- 
	Vector4 g_specularParams(1.5f, 50, 0, 0);

	//----------------------------------------------------------------------------------------------
	bool getBit(uint32 value, uint32 bit)
	{
		return ((value / (1 << bit)) % 2) == 1;
	}

} //-- unnamed


namespace Moo
{

#define MAX_POINT_LIGHTS 		4U
#define MAX_SPEC_POINT_LIGHTS 	2U

#define MAX_DIRECTIONAL_LIGHTS 	2U

#define MAX_SPOT_LIGHTS 		2U

//-----------------------------------------------------------------------------
// Section: EffectVisualContext
//-----------------------------------------------------------------------------

/**
 * This method sets up the constant mappings to prepare for rendering a visual.
 */
void EffectVisualContext::initConstants()
{
	BW_GUARD;
	if (!overrideConstants_)
	{
		initConstantsInternal(AUTO_CONSTANT_TYPE_PER_OBJECT);
	}
}

//--------------------------------------------------------------------------------------------------
void EffectVisualContext::registerAutoConstant(
	EAutoConstantType type, const BW::string& name, const Moo::EffectConstantValuePtr& autoConst)
{
	BW_GUARD;
	MF_ASSERT(autoConst.exists());

	constantMappings_[type][this->getMapping(name)] = autoConst;
}

//--------------------------------------------------------------------------------------------------
void EffectVisualContext::unregisterAutoConstant(EAutoConstantType type, const BW::string& name)
{
	BW_GUARD;

	constantMappings_[type][this->getMapping(name)] = NULL;
}

//--------------------------------------------------------------------------------------------------
void EffectVisualContext::initConstantsInternal(EAutoConstantType type)
{
	BW_GUARD;
	ConstantMappings::iterator it  = constantMappings_[type].begin();
	ConstantMappings::iterator end = constantMappings_[type].end();
	for (; it != end; ++it)
	{
		*(it->first) = it->second;
	}
}

//--------------------------------------------------------------------------------------------------
void EffectVisualContext::updateSharedConstants(uint32 flags)
{
	BW_GUARD;
	
	if (flags & CONSTANTS_PER_FRAME)
	{
		initConstantsInternal(AUTO_CONSTANT_TYPE_PER_FRAME);
		pEffectPools_[AUTO_CONSTANT_TYPE_PER_FRAME]->setAutoConstants();
	}
	if (flags & CONSTANTS_PER_SCREEN)
	{
		initConstantsInternal(AUTO_CONSTANT_TYPE_PER_SCREEN);
		pEffectPools_[AUTO_CONSTANT_TYPE_PER_SCREEN]->setAutoConstants();
	}
	if (flags & CONSTANTS_PER_VIEW)
	{
		initConstantsInternal(AUTO_CONSTANT_TYPE_PER_VIEW);
		pEffectPools_[AUTO_CONSTANT_TYPE_PER_VIEW]->setAutoConstants();
	}
}

// -----------------------------------------------------------------------------
// Section: FloatSetter - set an automatic float variable on an effect.
// -----------------------------------------------------------------------------

// 
//------------------------------------------

class FloatSetter : public Moo::EffectConstantValue
{
public:
	FloatSetter( float& value ):
	  value_(value)
	  {
	  };

	  bool operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle)
	  {
		  pEffect->SetFloat( constantHandle, value_ );
		  return true;
	  };

	  void value( float v )
	  {
		  value_ = v;
	  }
private:
	float& value_;
};

/**
 *	World transformation palette. Used for skinning.
 */
class WorldPaletteConstant : public EffectConstantValue
{
public:

	bool operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle)
	{
		BW_GUARD;
		uint32 nConstants = rc().effectVisualContext().worldMatrixPaletteSize() *
							EffectVisualContext::NUM_VECTOR4_PER_PALETTE_ENTRY;
		const Vector4* transforms = rc().effectVisualContext().worldMatrixPalette();
		if (transforms)
		{		
			pEffect->SetVectorArray( constantHandle, transforms, nConstants );
			pEffect->SetArrayRange( constantHandle, 0, nConstants );
		}
		return true;
	}
};


/**
 *	The ViewProjection transformation matrix constant.
 */
class ViewProjectionConstant : public EffectConstantValue
{
public:
	bool operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle)
	{
		BW_GUARD;
		pEffect->SetMatrix( constantHandle, &Moo::rc().viewProjection() );
		return true;
	}
};


/**
 *	The InvViewProjection transformation matrix constant.
 */
class InvViewProjectionConstant : public EffectConstantValue
{
public:
	bool operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle)
	{
		BW_GUARD;
		Matrix m(Moo::rc().viewProjection());
		m.invert();
		pEffect->SetMatrix( constantHandle, &m );
		return true;
	}
};



/**
 *	The ViewProjection transformation matrix constant.
 */
class LastViewProjectionConstant : public EffectConstantValue
{
public:
	bool operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle)
	{
		BW_GUARD;
		pEffect->SetMatrix( constantHandle, &Moo::rc().lastViewProjection() );
		return true;
	}
};

/**
 *	The World transformation matrix constant.
 */
class WorldConstant : public EffectConstantValue
{
public:
	bool operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle)
	{
		BW_GUARD;
		pEffect->SetMatrix( constantHandle, rc().effectVisualContext().worldMatrix() );
		return true;
	}
};

/**
 *	The InverseWorld transformation matrix constant.
 */
class InverseWorldConstant : public EffectConstantValue
{
public:
	bool operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle)
	{
		BW_GUARD;
		pEffect->SetMatrix( constantHandle, &rc().effectVisualContext().invWorld() );
		return true;
	}
};

/**
 *	The View transformation matrix constant.
 */
class ViewConstant : public EffectConstantValue
{
public:
	bool operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle)
	{
		BW_GUARD;
		pEffect->SetMatrix( constantHandle, &Moo::rc().view() );
		return true;
	}
};

/**
 *	The InvView transformation matrix constant.
 */
class InvViewConstant : public EffectConstantValue
{
public:
	bool operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle)
	{
		BW_GUARD;
		pEffect->SetMatrix( constantHandle, &Moo::rc().invView() );
		return true;
	}
};

/**
 *	The Projection transformation matrix constant.
 */
class ProjectionConstant : public EffectConstantValue
{
public:
	bool operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle)
	{
		BW_GUARD;
		pEffect->SetMatrix( constantHandle, &Moo::rc().projection() );
		return true;
	}
};

/**
 *	Screen dimensions stored in a vector4 constant.
 */
class ScreenConstant : public EffectConstantValue
{
public:
	bool operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle)
	{
		BW_GUARD;
		Vector4 v( rc().screenWidth(),
			rc().screenHeight(),
			rc().halfScreenWidth(),
			rc().halfScreenHeight() );
		pEffect->SetVector( constantHandle, &v );
		return true;
	}
};

/**
 *	Screen dimensions stored in a vector4 constant.
 */
class InvScreenConstant : public EffectConstantValue
{
public:
	bool operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle)
	{
		BW_GUARD;
		Vector4 v(
				rc().screenWidth(), rc().screenHeight(),
				1.0f / rc().screenWidth(), 1.0f / rc().screenHeight()
				);
		pEffect->SetVector( constantHandle, &v );
		return true;
	}
};

/**
 *	The WorldView transformation matrix constant.
 */
class WorldViewConstant : public EffectConstantValue
{
public:
	bool operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle)
	{
		BW_GUARD;
		if (rc().effectVisualContext().worldMatrixPalette( ))
		{
			pEffect->SetMatrix( constantHandle, &rc().view() );
		}
		else
		{
			Matrix wv;
			wv.multiply( *rc().effectVisualContext().worldMatrix(), Moo::rc().view() );
			pEffect->SetMatrix( constantHandle, &wv );
		}
		return true;
	}
};


/**
 *	The WorldViewProjection transformation matrix constant.
 */
class WorldViewProjectionConstant : public EffectConstantValue
{
public:
	bool operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle)
	{
		BW_GUARD;
		if (rc().effectVisualContext().worldMatrixPalette())
		{
			pEffect->SetMatrix( constantHandle, &rc().viewProjection() );
		}
		else
		{
			Matrix wvp;
			wvp.multiply( *rc().effectVisualContext().worldMatrix(), 
				Moo::rc().viewProjection() );
			pEffect->SetMatrix( constantHandle, &wvp );

		}
		return true;
	}
};


/**
 *	The EnvironmentTransform transformation matrix constant. 
 *	(ViewProjection matrix with the View translation removed)
 */
class EnvironmentTransformConstant : public EffectConstantValue
{
public:
	bool operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle)
	{		
		BW_GUARD;
		Matrix tr( Moo::rc().view() );
		tr.translation(Vector3(0,0,0));
		tr.postMultiply( Moo::rc().projection() );

		pEffect->SetMatrix( constantHandle, &tr );
		return true;
	}	
};


/**
 *	The EnvironmentShadowTransformConstant makes a matrix that
 *	take a position from world coordinates to clip coordinates.
 */
class EnvironmentShadowTransformConstant : public EffectConstantValue
{
public:
	bool operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle)
	{			
		BW_GUARD;
		float farPlane = Moo::rc().camera().farPlane();

		Matrix m;
		m.row(0, Vector4(  1.f / farPlane,	0,					0,		0 ) );
		m.row(1, Vector4(  0,				0,					0,		0 ) );
		m.row(2, Vector4(  0,				1.f / farPlane,		0,		0 ) );		
		m.row(3, Vector4(  0,				0,					0,		1 ) );		

		pEffect->SetMatrix( constantHandle, &m );
		return true;
	}	
};


//-- Note (b_sviglo): needed in the most post effects and for the shadow mappings to restore world
//--				  position from the mrt depth buffer.
class FarNearPlanesConstant : public EffectConstantValue
{
public:
	bool operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle)
	{	
		BW_GUARD;

		Vector4 fp;
		fp.x = Moo::rc().camera().farPlane();
		fp.y = 1.f / fp.x;
		fp.z = Moo::rc().camera().nearPlane();
		fp.w = 1.f / fp.z;		
		pEffect->SetVector( constantHandle, &fp );
		return true;
	}	
};


/**
 *	The FarPlane constant (fp, 1.f/fp, skyfp, 1.f/skyfp)
 *	The sky has a different, 'virtual' far plane to allow
 *	it to appear as though it's much bigger than the world.
 */
class FarPlaneConstant : public EffectConstantValue
{
public:
	bool operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle)
	{	
		BW_GUARD;
		float skyFarPlane = 1500.f;

		Vector4 fp;
		fp.x = Moo::rc().camera().farPlane();
		fp.y = 1.f / fp.x;
		fp.z = skyFarPlane;
		fp.w = 1.f / fp.z;		
		pEffect->SetVector( constantHandle, &fp );
		return true;
	}	
};


/**
*	A generic setter Vector4 effect constant.
*/
class Vector4Setter : public EffectConstantValue
{
public:
	Vector4Setter(const Vector4& value) : m_value(value) {}

	bool operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle)
	{	
		BW_GUARD;
		pEffect->SetVector( constantHandle, &m_value );
		return true;
	}

private:
	const Vector4& m_value;
};


/**
 *	Ambient light colour constant.
 */
class AmbientConstant : public EffectConstantValue
{
public:
	bool operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle)
	{
		BW_GUARD;
		if (Moo::rc().lightContainer())
			pEffect->SetVector( constantHandle, (Vector4*)(&Moo::rc().lightContainer()->ambientColour())  );
		else
		{
			Vector4 ones( 1.f, 1.f, 1.f, 1.f );
			pEffect->SetVector( constantHandle, &ones);
		}
		return true;
	}
};



/**
 *	A directional light effect constant.
 */
class DirectionalConstant : public EffectConstantValue
{
public:
	bool operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle)
	{
		BW_GUARD;
		struct ShadDirLight
		{
			Vector3 direction_;
			Colour colour_;
		};
		static ShadDirLight constant;

		const Matrix& invWorld =  rc().effectVisualContext().invWorld();

		LightContainer * pContainer;
		if (diffuse_)
		{
			pContainer = Moo::rc().lightContainer().get();
		}
		else
		{
			pContainer = Moo::rc().specularLightContainer().get();
		}

		if (pContainer != NULL)
		{

			DirectionalLightVector& dLights = pContainer->directionals();
			uint32 nLights =  MAX_DIRECTIONAL_LIGHTS < dLights.size() ?
								MAX_DIRECTIONAL_LIGHTS : (uint32)dLights.size();
			for (uint32 i = 0; i < nLights; i++)
			{
				D3DXHANDLE elemHandle = pEffect->GetParameterElement( constantHandle, i );
				if (elemHandle)
				{
					if (objectSpace_)
					{
						invWorld.applyVector( constant.direction_, dLights[i]->worldDirection() );
						constant.direction_.normalise();
					}
					else
					{
						constant.direction_ = dLights[i]->worldDirection();
					}
					constant.colour_ = dLights[i]->colour() * dLights[i]->multiplier();
					pEffect->SetValue( elemHandle, &constant, sizeof( constant ) );
				}
			}
			pEffect->SetArrayRange( constantHandle, 0, nLights );
		}
		return true;
	}
	DirectionalConstant( bool diffuse = true, bool objectSpace = false ) : 
		objectSpace_( objectSpace ),
		diffuse_( diffuse )
	{

	}
private:
	bool objectSpace_;
	bool diffuse_;
};


/**
 *	A omni directional light source constant.
 */
class OmniConstant : public EffectConstantValue
{
public:
	bool operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle)
	{
		BW_GUARD;
		struct ShadPointLight
		{
			Vector3 position_;
			Colour colour_;
			Vector2 attenuation_;
		};
		static ShadPointLight constant;

		const Matrix& invWorld =  rc().effectVisualContext().invWorld();
		float scale =  rc().effectVisualContext().invWorldScale();

		LightContainer * pContainer;
		uint32 maxLights;
		if (diffuse_)
		{
			maxLights = MAX_POINT_LIGHTS;
			pContainer = Moo::rc().lightContainer().get();
		}
		else
		{
			maxLights = MAX_SPEC_POINT_LIGHTS;
			pContainer = Moo::rc().specularLightContainer().get();
		}
		if (pContainer != NULL)
		{
			OmniLightVector& oLights = pContainer->omnis();
			uint32 nLights = maxLights < oLights.size() ? maxLights : (uint32)oLights.size();
			for (uint32 i = 0; i < nLights; i++)
			{
				D3DXHANDLE elemHandle = pEffect->GetParameterElement( constantHandle, i );
				if (elemHandle)
				{
					float innerRadius = oLights[i]->worldInnerRadius();
					float outerRadius = oLights[i]->worldOuterRadius();
					if (objectSpace_)
					{
						innerRadius *= scale;
						outerRadius *= scale;
						invWorld.applyPoint( constant.position_, oLights[i]->worldPosition() );
					}
					else
					{
						constant.position_ = oLights[i]->worldPosition();
					}

					constant.colour_ = oLights[i]->colour() * oLights[i]->multiplier();
					constant.attenuation_.set( outerRadius, 1.f / ( outerRadius - innerRadius ) );
					pEffect->SetValue( elemHandle, &constant, sizeof( constant ) );
				}
			}
			pEffect->SetArrayRange( constantHandle, 0, nLights );
		}
		return true;
	}
	OmniConstant( bool diffuse = true, bool objectSpace = false ):
		objectSpace_( objectSpace ),
		diffuse_( diffuse )
	{

	}
private:
	bool objectSpace_;
	bool diffuse_;
};

/**
 *	A spot light source constant.
 */
class SpotDiffuseConstant : public EffectConstantValue
{
public:
	bool operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle)
	{
		BW_GUARD;
		struct ShadSpotLight
		{
			Vector3 position;
			Colour colour;
			Vector3 attenuation;
			Vector3 direction;
		};

		static ShadSpotLight constant;

		const Matrix& invWorld =  rc().effectVisualContext().invWorld();
		float scale =  rc().effectVisualContext().invWorldScale();

		LightContainer * pContainer = Moo::rc().lightContainer().get();
		if (pContainer != NULL)
		{
			SpotLightVector& sLights = pContainer->spots();
			uint32 nLights = MAX_SPOT_LIGHTS < sLights.size() ? 
								MAX_SPOT_LIGHTS : (uint32)sLights.size();
			for (uint32 i = 0; i < 2 && i < sLights.size(); i++)
			{
				D3DXHANDLE elemHandle = pEffect->GetParameterElement( constantHandle, i );
				if (elemHandle)
				{
					float innerRadius = sLights[i]->worldInnerRadius();
					float outerRadius = sLights[i]->worldOuterRadius();
					if (!objectSpace_)
					{
						constant.position = sLights[i]->worldPosition();
						constant.direction = sLights[i]->worldDirection();
					}
					else
					{
						innerRadius *= scale;
						outerRadius *= scale;
						constant.position = invWorld.applyPoint( sLights[i]->worldPosition() );
						constant.direction = invWorld.applyVector( sLights[i]->worldDirection() );
					}

					constant.colour = sLights[i]->colour() * sLights[i]->multiplier();
					constant.attenuation.set( outerRadius, 1.f / ( outerRadius - innerRadius ), sLights[i]->cosConeAngle() );
					pEffect->SetValue( elemHandle, &constant, sizeof( constant ) );
				}
			}
			pEffect->SetArrayRange( constantHandle, 0, nLights );
		}
		return true;
	}
	SpotDiffuseConstant( bool objectSpace = false )
	: objectSpace_( objectSpace )
	{

	}
private:
	bool objectSpace_;
};


/**
 *	A constant integer containing the number of directional light sources.
 */
class NDirectionalsConstant : public EffectConstantValue
{
public:
	bool operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle)
	{
		BW_GUARD;
		LightContainer * pContainer = Moo::rc().lightContainer().get();
		if (pContainer != NULL)
		{
			pEffect->SetInt( constantHandle,
				min((uint)pContainer->directionals().size(),
					 MAX_DIRECTIONAL_LIGHTS) );
		}
		else
		{
			pEffect->SetInt( constantHandle, 0 );
		}
		return true;
	}
};

/**
 *	A constant integer containing the number of omni light sources.
 */
class NOmnisConstant : public EffectConstantValue
{
public:
	bool operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle)
	{
		BW_GUARD;
		LightContainer * pContainer = Moo::rc().lightContainer().get();
		if (pContainer != NULL)
		{
			pEffect->SetInt( constantHandle, 
				min( (uint)pContainer->omnis().size(),
					MAX_POINT_LIGHTS )  );
		}
		else
		{
			pEffect->SetInt( constantHandle, 0 );
		}
		return true;
	}
};

/**
 *	A constant integer containing the number of spot light sources.
 */
class NSpotsConstant : public EffectConstantValue
{
public:
	bool operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle)
	{
		BW_GUARD;
		LightContainer * pContainer = Moo::rc().lightContainer().get();
		if (pContainer != NULL)
			pEffect->SetInt( constantHandle,
				min((uint)pContainer->spots().size(),MAX_SPOT_LIGHTS) );
		else
			pEffect->SetInt( constantHandle, 0 );
		return true;
	}
};

/**
 *	A constant integer containing the number of specular directional light 
 *	sources.
 */
class NSpecularDirectionalsConstant : public EffectConstantValue
{
public:
	bool operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle)
	{
		BW_GUARD;
		LightContainer * pContainer = Moo::rc().specularLightContainer().get();
		if (pContainer != NULL)
		{
			pEffect->SetInt( constantHandle,
				min((uint)pContainer->directionals().size(),
					MAX_DIRECTIONAL_LIGHTS) );
		}
		else
		{
			pEffect->SetInt( constantHandle, 0 );
		}
		return true;
	}
};

/**
 *	A constant integer containing the number of specular omni light 
 *	sources.
 */
class NSpecularOmnisConstant : public EffectConstantValue
{
public:
	bool operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle)
	{
		BW_GUARD;
		LightContainer * pContainer = Moo::rc().specularLightContainer().get();
		if (pContainer != NULL)
			pEffect->SetInt( constantHandle,
				min((uint)pContainer->omnis().size(),MAX_SPEC_POINT_LIGHTS));
		else
			pEffect->SetInt( constantHandle, 0 );
		return true;
	}
};

/**
 *	The camera position constant.
 */
class CameraPosConstant : public EffectConstantValue
{
public:
	bool operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle)
	{
		BW_GUARD;
		Vector4 camPos;
		if (objectSpace_)
		{
			reinterpret_cast<Vector3&>(camPos) = rc().effectVisualContext().invWorld().applyPoint( rc().invView()[ 3 ] );
		}
		else
		{
			camPos = rc().invView().row(3);
		}
		pEffect->SetVector( constantHandle, &camPos );
		return true;
	}
	CameraPosConstant( bool objectSpace = false )
	: objectSpace_( objectSpace )
	{

	}
private:
	bool objectSpace_;
};

/**
 *	The lod camera position constant.
 */
class LodCameraPosConstant : public EffectConstantValue
{
public:
	bool operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle)
	{
		BW_GUARD;
		Vector4 camPos( rc().lodInvView().applyToOrigin(), rc().lodZoomFactor() );
		pEffect->SetVector(constantHandle, &camPos);
		return true;
	}
};

//--------------------------------------------------------------------------------------------------
class CameraDirConstant : public EffectConstantValue
{
public:
	bool operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle)
	{
		BW_GUARD;
		Vector4 camDir(rc().invView().applyToUnitAxisVector(2), 0);
		pEffect->SetVector(constantHandle, &camDir);
		return true;
	}
};

//--------------------------------------------------------------------------------------------------
class CameraDirsConstant : public EffectConstantValue
{
public:
	bool operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle)
	{
		BW_GUARD;

		const Moo::Camera& c = Moo::rc().camera();
		float scale = 1.0f / c.nearPlanePoint(0.0f, 0.0f).length();

		const Matrix& iv = Moo::rc().invView();

		Matrix dirs(
			Vector4(iv.applyVector(scale * c.nearPlanePoint(-1.f,  1.f)), 0.f), // left  - top
			Vector4(iv.applyVector(scale * c.nearPlanePoint(-1.f, -1.f)), 0.f), // left  - buttom
			Vector4(iv.applyVector(scale * c.nearPlanePoint( 1.f,  1.f)), 0.f), // right - top
			Vector4(iv.applyVector(scale * c.nearPlanePoint( 1.f, -1.f)), 0.f)  // right - buttom
			);

		pEffect->SetMatrix(constantHandle, &dirs);

		return true;
	}
};

//--------------------------------------------------------------------------------------------------
class NoiseMapConstant : public EffectConstantValue
{
public:
	bool operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle)
	{
		BW_GUARD;
		pEffect->SetTexture( constantHandle, rc().effectVisualContext().pNoiseMap() );
		return true;
	}
};

//--------------------------------------------------------------------------------------------------
class BitwiseLUTMapConstant : public EffectConstantValue
{
public:
	bool operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle)
	{
		BW_GUARD;
		pEffect->SetTexture( constantHandle, rc().effectVisualContext().pBitwiseLUT() );
		return true;
	}
};

//--------------------------------------------------------------------------------------------------
class Atan2LUTMapConstant : public EffectConstantValue
{
public:
	bool operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle)
	{
		BW_GUARD;
		pEffect->SetTexture( constantHandle, rc().effectVisualContext().pAtan2LUT() );
		return true;
	}
};

//--------------------------------------------------------------------------------------------------
class FogParamsConstant : public EffectConstantValue
{
public:
	bool operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle)
	{
		BW_GUARD;
		FogParams fog = FogHelper::pInstance()->fogParams();

		pEffect->SetValue(constantHandle, &fog, sizeof(FogParams));
		return true;
	}
};

//--------------------------------------------------------------------------------------------------
class SpecularParamsConstant : public EffectConstantValue
{
public:
	bool operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle)
	{
		BW_GUARD;
		pEffect->SetVector(constantHandle, &g_specularParams);
		return true;
	}
};


/**
 *	The automatically generated normalisation cube map constant.
 */
class NormalisationMapConstant : public EffectConstantValue
{
public:
	bool operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle)
	{
		BW_GUARD;
		pEffect->SetTexture( constantHandle,  rc().effectVisualContext().pNormalisationMap() );
		return true;
	}
	NormalisationMapConstant()
	{

	}
private:
};


/**
 *	Time.
 */
class EffectVisualContext::TimeConstant : public EffectConstantValue
{
public:
	TimeConstant(): time_( 0.0f ) {}

	void tick( float dTime )
	{
		time_ += dTime;
	}
	bool operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle)
	{
		BW_GUARD;
		pEffect->SetFloat( constantHandle, time_ );

		return true;
	}
private:
	float time_;

};

/**
 *	Near plane
 */
class NearPlaneConstant : public EffectConstantValue
{
public:
	NearPlaneConstant( )
	{
	}

	bool operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle)
	{
		BW_GUARD;
		pEffect->SetFloat( constantHandle, rc().camera().nearPlane() );

		return true;
	}
};


/**
 *	Object ID
 */
class ObjectIDConstant : public EffectConstantValue
{
public:
	ObjectIDConstant( )
	{
	}

	bool operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle)
	{
		BW_GUARD;
		pEffect->SetFloat( constantHandle, rc().effectVisualContext().currentObjectID() );

		return true;
	}
};

/**
 *	Sets the "MipFilter" constant. Shaders can read this constant to 
 *	allow users to define the mip mapping filter to be used via the 
 *	TEXTURE_FILTER graphics setting.
 */
class MipFilterConstant : public EffectConstantValue
{
public:
	bool operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle)
	{
		BW_GUARD;
		pEffect->SetInt( constantHandle, (INT)TextureManager::instance()->configurableMipFilter() );

		return true;
	}
private:
};

/**
 *	Sets the "MinMagFilter" constant. Shaders can read this constant to 
 *	allow users to define the minification/magnification mapping filters
 *	to be used via the TEXTURE_FILTER graphics setting.
 */
class MinMagFilterConstant : public EffectConstantValue
{
public:
	bool operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle)
	{
		BW_GUARD;
		pEffect->SetInt( constantHandle, (INT)TextureManager::instance()->configurableMinMagFilter() );

		return true;
	}
private:
};

/**
 *	Sets the "MaxAnisotropy" constant. Shaders can read this constant 
 *	to allow users to define the anisotropy value to be used via the 
 *	TEXTURE_FILTER graphics setting.
 */
class MaxAnisotropyConstant : public EffectConstantValue
{
public:
	bool operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle)
	{
		BW_GUARD;
		pEffect->SetInt( constantHandle, (INT)TextureManager::instance()->configurableMaxAnisotropy() );

		return true;
	}
private:
};

//--------------------------------------------------------------------------------------------------
class DebugVisualizerConstant : public EffectConstantValue
{
public:
	bool operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle)
	{
		BW_GUARD;

		Vector4 params(g_showAmbientTerm, g_showDiffuseTerm, g_showSpecularTerm, 1);
		pEffect->SetVector(constantHandle, &params);
		return true;
	}
};

//--------------------------------------------------------------------------------------------------
class SunLightConstant : public EffectConstantValue
{
public:
	bool operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle)
	{
		BW_GUARD;
		SunLight sun = rc().effectVisualContext().sunLight();
		pEffect->SetValue(constantHandle, &sun, sizeof(SunLight));
		return true;
	}
};

/**
 *	Constructor.
 *	This is where he common effect constants are all bound to their
 *	corresponding semantics.
 */
EffectVisualContext::EffectVisualContext() : 
	worldMatrix_( &Matrix::identity ),
	invWorldMatrix_( &invWorldCached_),
	worldMatrixPalette_( NULL ),
	worldMatrixPaletteSize_( 0 ),
	invWorldScale_( 1.f ),
	isOutside_( true ),
	overrideConstants_( false ),
	currentObjectID_( 0.f )
{
	BW_GUARD;

	//-- ToDo (b_sviglo) : delete unnecessary auto-constants and divide constants related only for specific
	//--		  render pipeline.
	//-- ToDo (b_sviglo): optimize. Maybe use BW::vector instead BW::map to make more predictable memory
	//--		  access pattern.	

#undef  DEFINE_AUTO_CONST
#define DEFINE_AUTO_CONST(type, name) constantMappings_[AUTO_CONSTANT_TYPE_##type][this->getMapping(name)]

	//-- per object auto-constants.
	DEFINE_AUTO_CONST(PER_OBJECT,	"WorldPalette")				= new WorldPaletteConstant;
	DEFINE_AUTO_CONST(PER_OBJECT,	"World")					= new WorldConstant;
	DEFINE_AUTO_CONST(PER_OBJECT,	"WorldIT")					= new InverseWorldConstant;
	DEFINE_AUTO_CONST(PER_OBJECT,	"WorldView")				= new WorldViewConstant;
	DEFINE_AUTO_CONST(PER_OBJECT,	"WorldViewProjection")		= new WorldViewProjectionConstant;
	DEFINE_AUTO_CONST(PER_OBJECT,	"ObjectID" )				= new ObjectIDConstant;	

	//-- shared per-frame constants.
	timeConstant_= new TimeConstant;
	DEFINE_AUTO_CONST(PER_FRAME,	"Time")						= timeConstant_;
	DEFINE_AUTO_CONST(PER_FRAME,	"SunLight")					= new SunLightConstant();
	DEFINE_AUTO_CONST(PER_FRAME,	"DebugVisualizer")			= new DebugVisualizerConstant();
	DEFINE_AUTO_CONST(PER_FRAME,	"MipFilter")				= new MipFilterConstant;
	DEFINE_AUTO_CONST(PER_FRAME,	"MinMagFilter")				= new MinMagFilterConstant;
	DEFINE_AUTO_CONST(PER_FRAME,	"MaxAnisotropy")			= new MaxAnisotropyConstant;
	DEFINE_AUTO_CONST(PER_FRAME,	"NoiseMap")					= new NoiseMapConstant();
	DEFINE_AUTO_CONST(PER_FRAME,	"BitwiseLUTMap")			= new BitwiseLUTMapConstant();
	DEFINE_AUTO_CONST(PER_FRAME,	"Atan2LUTMap")				= new Atan2LUTMapConstant();
	DEFINE_AUTO_CONST(PER_FRAME,	"FogParams")				= new FogParamsConstant();
	DEFINE_AUTO_CONST(PER_FRAME,	"SpecularParams")			= new SpecularParamsConstant();

	//-- shared per-screen constants.
	DEFINE_AUTO_CONST(PER_SCREEN,	"Screen")					= new ScreenConstant;
	DEFINE_AUTO_CONST(PER_SCREEN,	"InvScreen")				= new InvScreenConstant;

	//-- shared per-view constants.
	DEFINE_AUTO_CONST(PER_VIEW,		"ViewProjection")			= new ViewProjectionConstant;
	DEFINE_AUTO_CONST(PER_VIEW,		"InvViewProjection")		= new InvViewProjectionConstant;
	DEFINE_AUTO_CONST(PER_VIEW,		"LastViewProjection")		= new LastViewProjectionConstant;
	DEFINE_AUTO_CONST(PER_VIEW,		"View")						= new ViewConstant;
	DEFINE_AUTO_CONST(PER_VIEW,		"InvView")					= new InvViewConstant;
	DEFINE_AUTO_CONST(PER_VIEW,		"Projection")				= new ProjectionConstant;
	DEFINE_AUTO_CONST(PER_VIEW,		"EnvironmentTransform")		= new EnvironmentTransformConstant;
	DEFINE_AUTO_CONST(PER_VIEW,		"FarPlane")					= new FarPlaneConstant;
	DEFINE_AUTO_CONST(PER_VIEW,		"FarNearPlanes")			= new FarNearPlanesConstant;
	DEFINE_AUTO_CONST(PER_VIEW,		"CameraPos")				= new CameraPosConstant;
	DEFINE_AUTO_CONST(PER_VIEW,		"CameraDir")				= new CameraDirConstant;
	DEFINE_AUTO_CONST(PER_VIEW,		"LodCameraPos")				= new LodCameraPosConstant;
	DEFINE_AUTO_CONST(PER_VIEW,		"CameraDirs")				= new CameraDirsConstant();

#undef DEFINE_AUTO_CONST
	
	//-- init all constants.
	for (uint i = 0; i < AUTO_CONSTANT_TYPE_COUNT; ++i)
	{
		initConstantsInternal((EAutoConstantType)i);
	}

	//-- register watchers.
	{
		MF_WATCH("Render/Debug lighting visualization/show ambient",
			g_showAmbientTerm, Watcher::WT_READ_WRITE, "Shows ambient term of the lighting equation."
			);

		MF_WATCH("Render/Debug lighting visualization/show diffuse",
			g_showDiffuseTerm, Watcher::WT_READ_WRITE, "Shows diffuse term of the lighting equation."
			);

		MF_WATCH("Render/Debug lighting visualization/show specular",
			g_showSpecularTerm, Watcher::WT_READ_WRITE, "Shows specular term of the lighting equation."
			);

		MF_WATCH("Render/specular params/amount multiplier",
			g_specularParams.x, Watcher::WT_READ_WRITE, "Specular amount multiplier."
			);

		MF_WATCH("Render/specular params/power",
			g_specularParams.y, Watcher::WT_READ_WRITE, "Specular power. Affects size of the specular spot."
			);
	}
}

//--------------------------------------------------------------------------------------------------
void EffectVisualContext::init()
{
	BW_GUARD;
	EffectManager& em = EffectManager::instance();
	bool success = true;

	//-- create effect pool.
	success &= SUCCEEDED(D3DXCreateEffectPool(&pEffectPoolHandle_));
	success &= (pEffectPools_[AUTO_CONSTANT_TYPE_PER_FRAME]  = em.createEffectPool("shaders/std_effects/per_frame_effect_pool.fx")).exists();
	success &= (pEffectPools_[AUTO_CONSTANT_TYPE_PER_SCREEN] = em.createEffectPool("shaders/std_effects/per_screen_effect_pool.fx")).exists();
	success &= (pEffectPools_[AUTO_CONSTANT_TYPE_PER_VIEW]   = em.createEffectPool("shaders/std_effects/per_view_effect_pool.fx")).exists();

	success &= createNoiseMap();
	success &= createBitwiseLUTMap();
	success &= createAtan2LUTMap();
}

/**
 *	Destructor.
 */
EffectVisualContext::~EffectVisualContext()
{
	initConstants();

	pNormalisationMap_ = NULL;
#if 0
	constantMappings_.clear();
#else
	for (uint i = 0; i < AUTO_CONSTANT_TYPE_COUNT; ++i)
		constantMappings_[i].clear();
#endif

	for (Mappings::iterator it = mappings_.begin(), end = mappings_.end();
		it != end; ++it)
	{
		if (it->second->getObject())
		{
			if (it->second->getObject()->refCount() > 1)
				WARNING_MSG( "EffectConstantValue not deleted properly : %s\n",
				it->first.to_string().c_str() );

			*(it->second) = NULL;
		}
		bw_safe_delete(it->second);
	}

	mappings_.clear();

	//-- unregister watchers.
#if ENABLE_WATCHERS
	if (Watcher::hasRootWatcher())
	{
		Watcher::rootWatcher().removeChild("Render/Debug lighting visualization");
	}
#endif //-- ENABLE_WATCHERS
}

EffectConstantValuePtr * EffectVisualContext::getMapping(
	const BW::StringRef & identifier, bool createIfMissing )
{
	BW_GUARD;
	Mappings::iterator it = mappings_.find( identifier );
	if (it == mappings_.end())
	{
		if (createIfMissing)
		{
			// TODO: make this managed, don't like allocating four byte blocks.
			EffectConstantValuePtr * ppProxy = new EffectConstantValuePtr;
			mappings_.insert( std::make_pair( identifier, ppProxy ) );
			it = mappings_.find( identifier );
		}
		else
		{
			return NULL;
		}
	}

	return it->second;
}

/**
 *	Setup the world matrix
 *
 *	@param pRenderSet		The new render set.
 */
void EffectVisualContext::worldMatrix( const Matrix* worldMatrix )
{
	if (worldMatrix == NULL)
	{
		worldMatrix_ = &Matrix::identity;
		invWorldMatrix_ = &Matrix::identity;
		invWorldScale_ = 1.f;
	}
	else
	{
		worldMatrix_ = worldMatrix;
		invWorldCached_.invert( *worldMatrix_ );
		// alexanderr:
		// invWorld_ isn't uniformly scaled and result value
		// might not be entirely correct
		// our code doesn't use it, but it's possible that
		// customers are using object space lighting in shaders
		// which will use this scale
		// current decision is to leave it as is
		// to keep backwards compatibility
		invWorldScale_ = invWorldCached_.uniformScale();
		invWorldMatrix_ = &invWorldCached_;
	}
	worldMatrixPalette_ = NULL;
	worldMatrixPaletteSize_ = 0;
}

void	EffectVisualContext::worldMatrixPalette( const Vector4* worldPalette, uint32 worldPaletteSize )
{
	worldMatrixPalette_ = worldPalette;
	worldMatrixPaletteSize_ = worldPaletteSize;

	worldMatrix_ = &Matrix::identity;
	invWorldMatrix_ = &Matrix::identity;
	invWorldScale_ = 1.f;
}


/**
 *	Tick the constants that need it.
 *
 *	@param dTime		Elapsed time since last frame.
 */
void EffectVisualContext::tick( float dTime )
{
	BW_GUARD;
	timeConstant_->tick( dTime );	
}


/**
 *	This method creates the noise map.
 */
bool EffectVisualContext::createNoiseMap()
{
	BW_GUARD;
	pNoiseMap_ = Moo::TextureManager::instance()->get("system/maps/post_processing/noise.dds");
	return pNoiseMap_.exists();
}

/**
 *	This method creates the bitwise map.
 */
bool EffectVisualContext::createBitwiseLUTMap()
{
	BW_GUARD;

	pBitwiseLUT_ = new ImageTexture8(256, 8);
	if (!pBitwiseLUT_->pTexture())
		return false;

	pBitwiseLUT_->lock();
	for (uint w = 0; w < 256; ++w)
	{
		for (uint h = 0; h < 8; ++h)
		{
			pBitwiseLUT_->image().set(w, h, (getBit(w, h) ? 0xFF : 0x00));
		}
	}
	pBitwiseLUT_->unlock();

	return true;
}

/**
 *	This method creates the atan2 lookup map.
 */
bool EffectVisualContext::createAtan2LUTMap()
{
	BW_GUARD;

	pAtan2LUT_ = new ImageTexture8(256, 256);
	if (!pAtan2LUT_->pTexture())
		return false;

	pAtan2LUT_->lock();
	for (uint w = 0; w < 256; ++w)
	{
		for (uint h = 0; h < 256; ++h)
		{
			float xval  = (h / 256.0f) * 2.0f - 1.0f;
			float yval  = (w / 256.0f) * 2.0f - 1.0f;
			float atan2 = (atan2f(yval, xval) + MATH_PI) / MATH_2PI;

			pAtan2LUT_->image().set(w, h, static_cast<uint8>(atan2 * 255.0f + 0.5f));
		}
	}
	pAtan2LUT_->unlock();

	return true;
}

const uint32 CUBEMAP_SIZE = 128;
const uint32 CUBEMAP_SHIFT = 7;

/**
 * This method helps with creating the normalisation cubemap.
 *
 * @param buffer pointer to the side of the cubemap we are filling in.
 * @param xMask the direction of the x-axis on this cubemap side.
 * @param yMask the direction of the y-axis on this cubemap side.
 * @param zMask the direction of the z-axis on this cubemap side.
 */
namespace
{
static void fillNormMapSurface( CubeTextureLockWrapper& textureLock, D3DCUBEMAP_FACES face, 
								const Vector3& xMask,const Vector3& yMask, const Vector3& zMask )
{
	BW_GUARD;

	D3DLOCKED_RECT lockedRect;
	HRESULT hr = textureLock.lockRect( face, 0, &lockedRect, 0 );
	if ( FAILED( hr ) )
	{
		ERROR_MSG( "fillNormMapSurface - couldn't get %d cube surface.  error code %lx\n", face, hr );
		return;
	}
	uint32* buffer = (uint32*)lockedRect.pBits;

	for (int i = 0; i < int(CUBEMAP_SIZE*CUBEMAP_SIZE); i++)
	{
		int xx = i & (CUBEMAP_SIZE - 1);
		int yy = i >> CUBEMAP_SHIFT;
		float x = (( float(xx) / float(CUBEMAP_SIZE - 1) ) * 2.f ) - 1.f;
		float y = (( float(yy) / float(CUBEMAP_SIZE - 1) ) * 2.f ) - 1.f;
		Vector3 out = ( xMask * x ) + (yMask * y) + zMask;
		out.normalise();
		out += Vector3(1, 1, 1);
		out *= 255.5f * 0.5f;
		*(buffer++) = 0xff000000 |
			(uint32(out.x)<<16) |
			(uint32(out.y)<<8) |
			uint32(out.z);
	}

	textureLock.unlockRect( face, 0 );
}

}

/**
 *	This method creates the normalisation cube map.
 */
void EffectVisualContext::createNormalisationMap()
{
	BW_GUARD;

	pNormalisationMap_ = rc().createCubeTexture( CUBEMAP_SIZE, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED );

	if (!pNormalisationMap_.hasComObject())
	{
		ERROR_MSG( "EffectVisualContext::createNormalisationMap - couldn't create cube texture\n" );
		return;
	}

	// fill in all six cubemap sides.
	CubeTextureLockWrapper textureLock( pNormalisationMap_ );

	fillNormMapSurface( textureLock, D3DCUBEMAP_FACE_POSITIVE_X,
			Vector3( 0, 0, -1 ), Vector3( 0, -1, 0), Vector3(  1, 0, 0 ) );

	fillNormMapSurface( textureLock, D3DCUBEMAP_FACE_NEGATIVE_X,
			Vector3( 0, 0, 1 ), Vector3( 0, -1, 0), Vector3( -1, 0, 0 )  );

	fillNormMapSurface( textureLock, D3DCUBEMAP_FACE_POSITIVE_Y,
			Vector3( 1, 0, 0 ), Vector3( 0, 0,  1), Vector3( 0,  1, 0 ) );

	fillNormMapSurface( textureLock, D3DCUBEMAP_FACE_NEGATIVE_Y,
			Vector3( 1, 0, 0 ), Vector3( 0, 0, -1), Vector3( 0, -1, 0 ) );

	fillNormMapSurface( textureLock, D3DCUBEMAP_FACE_POSITIVE_Z,
			Vector3(  1, 0, 0 ), Vector3( 0, -1, 0), Vector3( 0, 0,  1 ) );

	fillNormMapSurface( textureLock, D3DCUBEMAP_FACE_NEGATIVE_Z,
			Vector3( -1, 0, 0 ), Vector3( 0, -1, 0), Vector3( 0, 0, -1 ) );

}

}	// namespace Moo

BW_END_NAMESPACE

// effect_visual_context.cpp
