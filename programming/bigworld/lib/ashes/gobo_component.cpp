#include "pch.hpp"

#include "gobo_component.hpp"
#include "cstdmf/debug.hpp"
#include "moo/render_context.hpp"
#include "moo/effect_material.hpp"
#include "moo/vertex_formats.hpp"
#include "resmgr/auto_config.hpp"
#include "moo/custom_mesh.hpp"
#include "pyscript/script.hpp"

DECLARE_DEBUG_COMPONENT2( "2DComponents", 0 )


BW_BEGIN_NAMESPACE

/**
 *	This global method specifies the resources required by this file
 */
static AutoConfigString s_mfmName( "system/goboMaterial" );

// -----------------------------------------------------------------------------
// Section: GoboComponent
// -----------------------------------------------------------------------------

#undef PY_ATTR_SCOPE
#define PY_ATTR_SCOPE GoboComponent::

PY_TYPEOBJECT( GoboComponent )

PY_BEGIN_METHODS( GoboComponent )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( GoboComponent )
	/*~ attribute GoboComponent.secondaryTexture
	 *	@components{ client, tools }
	 *
	 *	This attribute provides the second texture that is blended with
	 *	the component's texture attribute.
	 */
	PY_ATTRIBUTE( secondaryTexture )
	/*~ attribute GoboComponent.freeze
	 *	@components{ client, tools }
	 *
	 *	This attribute, when set to a PyRenderTarget, stows the current
	 *	state of the secondary TextureProvider and displays it; essentially
	 *	freezing the current state.  To unfreeze, set the render target
	 *	to None.
	 */
	PY_ATTRIBUTE( freeze )
PY_END_ATTRIBUTES()

COMPONENT_FACTORY( GoboComponent )

/*~ function GUI.Gobo
 *	@components{ client, tools }
 *
 *	This function creates a new Gobo component.
 *	The GoboComponent acts like other gui components, but blends the gui
 *	component texture with another specified PyTextureProvider, using
 *	the alpha channel of the component's texture.
 *
 *	If you associate a render target with the gobo component, you can also
 *	freeze and unfreeze the secondary PyTextureProvider.
 *
 *	A recommended use of the gobo component is to create a post-processing
 *	effect, and use the output of that effect as the second texture stage
 *	in the gobo component.
 *
 *	This is for example perfect for binoculars and sniper scopes.
 *
 *	For example, in the following code:
 *
 *	@{
 *	comp = GUI.Gobo( "gui/maps/gobo_binoculars.dds" )
 *	comp.materialFX="SOLID"
 *	comp.secondTexture=myBlurredRenderTarget.texture
 *	GUI.addRoot( comp )
 *	@}
 *
 *	This example will display a binocular gobo, and where the alpha channel is
 *	relatively opaque in the binocular texture map, a blurred version of the scene
 *	is drawn. 
 *
 *	@param	textureName				The filename of the gobo texture.  The alpha
 *									channel determines how much of the blurred
 *									screen should be blended in.
 */
PY_FACTORY_NAMED( GoboComponent, "Gobo", GUI )


GoboComponent::GoboComponent( const BW::string& textureName, PyTypeObject * pType ):
	SimpleGUIComponent( textureName, pType ),
	freezeRenderTarget_( NULL ),
	secondaryTexture_( NULL )
{	
	BW_GUARD;
	material_ = NULL;
	buildMaterial();
}


GoboComponent::~GoboComponent()
{
	BW_GUARD;	
}


/**
 *	Section - SimpleGUIComponent methods
 */
void GoboComponent::setConstants()
{
	BW_GUARD;
	Moo::rc().setFVF( Moo::VertexXYZDUV2::fvf() );

	if ( material_->pEffect() && material_->pEffect()->pEffect() )
	{
		ComObjectWrap<ID3DXEffect> pEffect = material_->pEffect()->pEffect();
		D3DXHANDLE hDiffuseMap = pEffect->GetParameterByName(NULL,"diffuseMap");
		D3DXHANDLE hSecondMap = pEffect->GetParameterByName(NULL,"secondMap");
		pEffect->SetTexture(hDiffuseMap, texture_->pTexture());
		if (freezeRenderTarget_.hasObject())
		{
			pEffect->SetTexture(hSecondMap, freezeRenderTarget_->pTexture()->texture()->pTexture());
		}
		else if (secondaryTexture_.hasObject())
		{
			pEffect->SetTexture(hSecondMap, secondaryTexture_->texture()->pTexture());
		}
		else
		{
			pEffect->SetTexture(hSecondMap, NULL);
		}
	}
}

/**
 *	This method implements the PyAttachment::draw interface.  Since
 *	this gui component draws in the world, this is where we do our
 *	actual drawing.
 */
void GoboComponent::draw( Moo::DrawContext& drawContext, bool reallyDraw, bool overlay )
{
	BW_GUARD;
	this->setConstants();
	SimpleGUIComponent::draw( drawContext, overlay );
	return;

	CustomMesh<Moo::VertexXYZDUV2> mesh( D3DPT_TRIANGLEFAN );
	float w = 1.f;
	float h = 1.f;

	Moo::VertexXYZDUV2 v;
	v.colour_ = 0xffffffff;

	Vector3 fixup( -0.5f / SimpleGUI::instance().halfScreenWidth(), 0.5f / SimpleGUI::instance().halfScreenHeight(), 0.f );

	v.pos_.set(-1.f,-1.f,0.1f);
	v.uv_.set(0.f,0.f);
	v.uv2_.set(0.f,h);
	mesh.push_back(v);

	v.pos_.set(-1.f,1.f,0.1f);
	v.uv_.set(0.f,1.f);
	v.uv2_.set(0.f,0.f);
	mesh.push_back(v);

	v.pos_.set(1.f,1.f,0.1f);
	v.uv_.set(1.f,1.f);
	v.uv2_.set(w,0.f);
	mesh.push_back(v);

	v.pos_.set(1.f,-1.f,0.1f);
	v.uv_.set(1.f,0.f);
	v.uv2_.set(w,h);
	mesh.push_back(v);

	for (size_t i=0; i<4; i++)
	{
		mesh[i].pos_ = mesh[i].pos_ + fixup;
	}

	//Use a custom mesh to blend the linear texture onto the screen
	if ( visible() )
	{
		Moo::rc().push();
		Moo::rc().preMultiply( runTimeTransform() );
		Moo::rc().device()->SetTransform( D3DTS_WORLD, &Moo::rc().world() );

		if ( !momentarilyInvisible() && reallyDraw )
		{		
			SimpleGUI::instance().setConstants(runTimeColour(), pixelSnap());
			this->setConstants();
			material_->begin();
			for ( uint32 i=0; i<material_->numPasses(); i++ )
			{
				material_->beginPass(i);
				if ( !overlay )
				{
					Moo::rc().setRenderState( D3DRS_ZENABLE, TRUE );
					Moo::rc().setRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
					Moo::rc().setRenderState( D3DRS_ZFUNC, D3DCMP_LESS );
				}				
				mesh.drawEffect();				
				material_->endPass();
			}
			material_->end();
			Moo::rc().setVertexShader( NULL );
			Moo::rc().setFVF( GUIVertex::fvf() );
		}

		SimpleGUIComponent::drawChildren( drawContext, reallyDraw, overlay );

		Moo::rc().pop();
		Moo::rc().device()->SetTransform( D3DTS_WORLD, &Moo::rc().world() );
	}

	momentarilyInvisible( false );
}


PyObject *GoboComponent::pyGet_freeze()
{
	BW_GUARD;
	if ( freezeRenderTarget_.hasObject() )
	{
		PyObject *pObj = freezeRenderTarget_.getObject();
		Py_INCREF( pObj );
		return pObj;
	}
	Py_RETURN_NONE;
}


/**
 *	This method stows the current state of the secondary buffer in
 *	another texture, and displays it.
 */
void GoboComponent::freeze( PyRenderTargetPtr pRT )
{
	BW_GUARD;
	freezeRenderTarget_ = NULL;

	if (!secondaryTexture_.hasObject())
	{
		PyErr_SetString( PyExc_ValueError, "Gobo.freeze: "
			"Cannot freeze when the secondary texture is None" );
		return;
	}

	if (!pRT.hasObject())
	{
		//unsetting the freeze target, thus unfreezing.
		return;
	}

	if ( pRT->pRenderTarget()->copyTexture( secondaryTexture_->texture() ) )
	{
		freezeRenderTarget_ = pRT;
	}
}


/**
 *	This method overrides SimpleGUIComponent's method and makes sure the
 *	linear background texture is set into the second texture stage.
 *
 *	@returns	true if successful
 */
bool GoboComponent::buildMaterial( void )
{
	BW_GUARD;
	bool ret = false;

	if ( !material_ )
	{
		material_ = new Moo::EffectMaterial();
		material_->load( BWResource::openSection( s_mfmName ) );
	}

	if ( material_->pEffect() && material_->pEffect()->pEffect() )
	{
		ComObjectWrap<ID3DXEffect> pEffect = material_->pEffect()->pEffect();

		uint32 i = materialFX()-FX_ADD;
		D3DXHANDLE handle = pEffect->GetTechnique( i );
		material_->hTechnique( handle );

		ret = true;
	}
	else
	{
		ERROR_MSG(" GoboComponent::buildMaterial - material is invalid.");
	}

	return ret;
}


/**
 *	Factory method
 */
PyObject * GoboComponent::pyNew( PyObject * args )
{
	BW_GUARD;
	char * textureName;
	if (!PyArg_ParseTuple( args, "s", &textureName ))
	{
		PyErr_SetString( PyExc_TypeError, "GUI.Gobo: "
			"Argument parsing error: Expected a texture name" );
		return NULL;
	}

	return new GoboComponent( textureName );
}

BW_END_NAMESPACE

// gobo_component.cpp
