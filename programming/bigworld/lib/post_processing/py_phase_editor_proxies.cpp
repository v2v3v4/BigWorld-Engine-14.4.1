
#include "pch.hpp"
#include "py_phase_editor_proxies.hpp"
#include "py_phase_editor.hpp"
#include "py_phase.hpp"
#include "moo/resource_load_context.hpp"


#ifdef EDITOR_ENABLED


BW_BEGIN_NAMESPACE

namespace PostProcessing
{

///////////////////////////////////////////////////////////////////////////////
//	Section: PyPhaseTextureProxy
///////////////////////////////////////////////////////////////////////////////

PyPhaseTextureProxy::PyPhaseTextureProxy( BaseMaterialProxyPtr proxy,
		MaterialPropertiesUser::GetFnTex getFn, MaterialPropertiesUser::SetFnTex setFn,
		const BW::string& descName, PyPhasePtr pPhase, Moo::EffectMaterialPtr pEffectMaterial ):
	proxyHolder_( proxy ),
	getFn_(getFn),
	setFn_(setFn),
	descName_(descName),
	pPhase_( pPhase ),
	pMaterial_( pEffectMaterial )
{
}


BW::string PyPhaseTextureProxy::get() const
{
	PyObjectPtr pyMaterial( PyObject_GetAttrString( pPhase_.get(), "material" ), PyObjectPtr::STEAL_REFERENCE );
	if (pyMaterial)
	{
		PyObjectPtr pyTexture( PyObject_GetAttrString( pyMaterial.get(), descName_.c_str() ), PyObjectPtr::STEAL_REFERENCE );
		if (pyTexture)
		{
			PyTextureProviderPtr textureProvider;
			if (Script::setData( pyTexture.get(), textureProvider ) == 0 &&
				textureProvider && textureProvider->texture())
			{
				return textureProvider->texture()->resourceID();
			}
		}
	}

	return (*getFn_)();
}


void PyPhaseTextureProxy::setTransient( BW::string v )
{
	PyObjectPtr pyName( PyObject_GetAttrString( pPhase_.get(), "name" ), PyObjectPtr::STEAL_REFERENCE );

	BW::string phaseName( "post-processing phase " );

	if (pyName)
	{
		phaseName.append( PyString_AS_STRING( pyName.get() ) );
	}

	Moo::ScopedResourceLoadContext resLoadCtx( phaseName );

#if MANAGED_CUBEMAPS
	// Make sure there isn't a mismatch between the supplied texture and the expected
	// texture type
	//

	// Get the managed effect
	Moo::ManagedEffectPtr pManagedEffect = pMaterial_->pEffect();

	// Get a handle to parameter descName_
	D3DXHANDLE param = pManagedEffect->pEffect()->GetParameterByName( descName_.c_str(), 0 );

	// Get the annotation, if any
	const char* UIWidget = 0;
	D3DXHANDLE hAnnot = pManagedEffect->pEffect()->GetAnnotationByName( param, "UIWidget" );
	if (hAnnot)
		pManagedEffect->pEffect()->GetString( hAnnot, &UIWidget );
	BW::string widgetType;
	if (UIWidget)
		widgetType = BW::string(UIWidget);
	else
		widgetType = "";


	// Check the type of the new texture
	bool isCubeMap = false;
	BW::string             newResID = BWResolver::dissolveFilename( v );
	Moo::BaseTexturePtr		newBaseTex = Moo::TextureManager::instance()->get( newResID ).getObject();

	// Is this texture a cube map
	if ( newBaseTex )
		isCubeMap = newBaseTex->isCubeMap();

	// Check if the types match up
	if ( newBaseTex && widgetType == "CubeMap" && !isCubeMap )
	{
		WARNING_MSG( "Warning - You have attempted to assign a non-cube map texture to a\n"
					 "cube map texture slot!  This is not permitted.");
		return;
	}
	if ( newBaseTex && widgetType != "CubeMap" && isCubeMap )
	{
		WARNING_MSG( "Warning - You have attempted to assign a cube map texture to a\n"
					 "non-cube map texture slot!  This is not permitted.");
		return;
	}
#endif
	BW::string textureName = v;

	// We need this postfix so the properties list understands this is a texture.
	BW::string renderTargetPostfix = ".rt";
	BW::string::size_type postfixPos = textureName.rfind( renderTargetPostfix );

	if (postfixPos != BW::string::npos)
	{
		// It's a render target, remove the postfix so we get the name right.
		textureName = textureName.substr( 0, postfixPos );
	}

	// We bypass the normal proxy in favour of PyMaterial // (*setFn_)( v );
	PyObjectPtr pyMaterial( PyObject_GetAttrString( pPhase_.get(), "material" ), PyObjectPtr::STEAL_REFERENCE );
	if (pyMaterial)
	{
		PyObject_SetAttrString( pyMaterial.get(), descName_.c_str(), Script::getData( textureName ) );
	}
}


bool PyPhaseTextureProxy::setPermanent( BW::string v )
{
	setTransient( v );
	pPhase_->callCallback( false );
	return true;
}


///////////////////////////////////////////////////////////////////////////////
//	Section: PyPhaseBoolProxy
///////////////////////////////////////////////////////////////////////////////

PyPhaseBoolProxy::PyPhaseBoolProxy( BaseMaterialProxyPtr proxy,
		MaterialPropertiesUser::GetFnBool getFn, MaterialPropertiesUser::SetFnBool setFn,
		const BW::string& descName, PyPhasePtr pPhase ):
	proxyHolder_( proxy ),
	getFn_(getFn),
	setFn_(setFn),
	descName_(descName),
	pPhase_( pPhase )
{}

bool PyPhaseBoolProxy::get() const
{
	return (*getFn_)();
}


void PyPhaseBoolProxy::setTransient( bool v )
{
	// We bypass the normal proxy in favour of PyMaterial // (*setFn_)( v );
	PyObjectPtr pyMaterial( PyObject_GetAttrString( pPhase_.get(), "material" ), PyObjectPtr::STEAL_REFERENCE );
	if (pyMaterial)
	{
		PyObject_SetAttrString( pyMaterial.get(), descName_.c_str(), Script::getData( v ) );
	}
}


bool PyPhaseBoolProxy::setPermanent( bool v )
{
	setTransient( v );
	pPhase_->callCallback( false );
	return true;
}


///////////////////////////////////////////////////////////////////////////////
//	Section: PyPhaseIntProxy
///////////////////////////////////////////////////////////////////////////////

PyPhaseIntProxy::PyPhaseIntProxy( BaseMaterialProxyPtr proxy,
		MaterialPropertiesUser::GetFnInt getFn, MaterialPropertiesUser::SetFnInt setFn,
		MaterialPropertiesUser::RangeFnInt rangeFn, const BW::string& descName, PyPhasePtr pPhase ):
	proxyHolder_( proxy ),
	getFn_(getFn),
	setFn_(setFn),
	rangeFn_(rangeFn),
	descName_(descName),
	pPhase_( pPhase )
{
}


int32 PyPhaseIntProxy::get() const
{
	return (*getFn_)();
}


void PyPhaseIntProxy::setTransient( int32 v )
{
	// We bypass the normal proxy in favour of PyMaterial // (*setFn_)( v );
	PyObjectPtr pyMaterial( PyObject_GetAttrString( pPhase_.get(), "material" ), PyObjectPtr::STEAL_REFERENCE );
	if (pyMaterial)
	{
		PyObject_SetAttrString( pyMaterial.get(), descName_.c_str(), Script::getData( v ) );
	}
}


bool PyPhaseIntProxy::setPermanent( int32 v )
{
	setTransient( v );
	pPhase_->callCallback( false );
	return true;
}


bool PyPhaseIntProxy::getRange( int32& min, int32& max ) const
{
	int mind, maxd;
	if( rangeFn_ )
	{
		bool result = (*rangeFn_)( mind, maxd );
		min = mind;
		max = maxd;
		return result;
	}
	return false;
}


///////////////////////////////////////////////////////////////////////////////
//	Section: PyPhaseEnumProxy
///////////////////////////////////////////////////////////////////////////////

PyPhaseEnumProxy::PyPhaseEnumProxy( BaseMaterialProxyPtr proxy,
		MaterialPropertiesUser::GetFnEnum getFn, MaterialPropertiesUser::SetFnEnum setFn,
		const BW::string& descName, PyPhasePtr pPhase ):
	proxyHolder_( proxy ),
	getFn_(getFn),
	setFn_(setFn),
	descName_(descName),
	pPhase_( pPhase )
{
}


int32 PyPhaseEnumProxy::get() const
{
	return (*getFn_)();
}


void PyPhaseEnumProxy::setTransient( int32 v )
{
	// We bypass the normal proxy in favour of PyMaterial // (*setFn_)( v );
	PyObjectPtr pyMaterial( PyObject_GetAttrString( pPhase_.get(), "material" ), PyObjectPtr::STEAL_REFERENCE );
	if (pyMaterial)
	{
		PyObject_SetAttrString( pyMaterial.get(), descName_.c_str(), Script::getData( v ) );
	}
}


bool PyPhaseEnumProxy::setPermanent( int32 v )
{
	setTransient( v );
	pPhase_->callCallback( false );
	return true;
}


///////////////////////////////////////////////////////////////////////////////
//	Section: PyPhaseFloatProxy
///////////////////////////////////////////////////////////////////////////////

PyPhaseFloatProxy::PyPhaseFloatProxy( BaseMaterialProxyPtr proxy,
		MaterialPropertiesUser::GetFnFloat getFn, MaterialPropertiesUser::SetFnFloat setFn,
		MaterialPropertiesUser::RangeFnFloat rangeFn, const BW::string& descName, PyPhasePtr pPhase ):
	proxyHolder_( proxy ),
	getFn_(getFn),
	setFn_(setFn),
	rangeFn_(rangeFn),
	descName_(descName),
	pPhase_( pPhase )
{
}


float PyPhaseFloatProxy::get() const
{
	return (*getFn_)();
}


void PyPhaseFloatProxy::setTransient( float v )
{
	// We bypass the normal proxy in favour of PyMaterial // (*setFn_)( v );
	PyObjectPtr pyMaterial( PyObject_GetAttrString( pPhase_.get(), "material" ), PyObjectPtr::STEAL_REFERENCE );
	if (pyMaterial)
	{
		PyObject_SetAttrString( pyMaterial.get(), descName_.c_str(), Script::getData( v ) );
	}
}


bool PyPhaseFloatProxy::setPermanent( float v )
{
	setTransient( v );
	pPhase_->callCallback( false );
	return true;
}


bool PyPhaseFloatProxy::getRange( float& min, float& max, int& digits ) const
{
	float mind, maxd;
	if( rangeFn_ )
	{
		bool result =  (*rangeFn_)( mind, maxd, digits );
		min = mind;
		max = maxd;
		return result;
	}
	return false;
}


///////////////////////////////////////////////////////////////////////////////
//	Section: PyPhaseVector4Proxy
///////////////////////////////////////////////////////////////////////////////

PyPhaseVector4Proxy::PyPhaseVector4Proxy( BaseMaterialProxyPtr proxy,
		MaterialPropertiesUser::GetFnVec4 getFn, MaterialPropertiesUser::SetFnVec4 setFn,
		const BW::string& descName, PyPhasePtr pPhase ):
	proxyHolder_( proxy ),
	getFn_(getFn),
	setFn_(setFn),
	descName_(descName),
	pPhase_( pPhase )
{
}


Vector4 PyPhaseVector4Proxy::get() const
{
	return (*getFn_)();
}


void PyPhaseVector4Proxy::setTransient( Vector4 v )
{
	// We bypass the normal proxy in favour of PyMaterial // (*setFn_)( v );
	PyObjectPtr pyMaterial( PyObject_GetAttrString( pPhase_.get(), "material" ), PyObjectPtr::STEAL_REFERENCE );
	if (pyMaterial)
	{
		PyObject_SetAttrString( pyMaterial.get(), descName_.c_str(), Script::getData( v ) );
	}
}


bool PyPhaseVector4Proxy::setPermanent( Vector4 v )
{
	setTransient( v );
	pPhase_->callCallback( false );
	return true;
}


///////////////////////////////////////////////////////////////////////////////
//	Section: PyPhaseFilterQuadTypeProxy
///////////////////////////////////////////////////////////////////////////////

PyPhaseFilterQuadTypeProxy::PyPhaseFilterQuadTypeProxy( PyPhaseEditorPtr pItem ) :
	pItem_( pItem )
{
}


IntProxy::Data PyPhaseFilterQuadTypeProxy::get() const
{
	return pItem_->getFilterQuadType();
}


void PyPhaseFilterQuadTypeProxy::set( IntProxy::Data v, bool transient, bool addBarrier /*= true*/ )
{
	if (!transient)
	{
		pItem_->setFilterQuadType( v, addBarrier );
	}
}


} // namespace PostProcessing

BW_END_NAMESPACE

#endif // EDITOR_ENABLED
