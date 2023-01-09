#include "pch.hpp"
#include "material_properties.hpp"
#include "material_proxies.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/xml_section.hpp"
#include "moo/texture_manager.hpp"
#include "moo/effect_material.hpp"
#include "common/material_editor.hpp"
#include "common/material_utility.hpp"
#include "material_proxies.hpp"

DECLARE_DEBUG_COMPONENT2( "Common", 0 );

BW_BEGIN_NAMESPACE

namespace
{

typedef MaterialProxy< MaterialTextureProxy, BW::string > TextureProxyType;
typedef MaterialProxy< MaterialTextureFeedProxy, BW::string > TextureFeedProxyType;
typedef MaterialProxy< MaterialBoolProxy, bool > BoolProxyType;
typedef MaterialProxy< MaterialIntProxy, int32 > IntProxyType;
typedef MaterialProxy< MaterialEnumProxy, uint32 > EnumProxyType;
typedef MaterialProxy< MaterialFloatProxy, float > FloatProxyType;
typedef MaterialProxy< MaterialVector4Proxy, Vector4 > Vector4ProxyType;

// These maps help in identifying and join multiple instances of the same
// material property.
StringRefMap< SmartPointer< TextureProxyType > >		s_textureProxy;
StringRefMap< SmartPointer< TextureFeedProxyType > >	s_textureFeedProxy;
StringRefMap< SmartPointer< BoolProxyType > >			s_boolProxy;
StringRefMap< SmartPointer< IntProxyType > >			s_intProxy;
StringRefMap< SmartPointer< EnumProxyType > >			s_enumProxy;
StringRefMap< SmartPointer< FloatProxyType > >			s_floatProxy;
StringRefMap< SmartPointer< Vector4ProxyType > >		s_vector4Proxy;


///////////////////////////////////////////////////////////////////////////////
// Section : Proxy functors needed by MaterialProperties
///////////////////////////////////////////////////////////////////////////////

class BoolProxyFactory : public Moo::EffectPropertyFactory
{
public:
	virtual Moo::EffectPropertyPtr create(
		const BW::StringRef & name,
		DataSectionPtr pSection )
	{
		BW_GUARD;

		MaterialBoolProxy* prop = new MaterialBoolProxy( name );
		prop->set( pSection->asBool(), false );
		return prop;
	}

	virtual Moo::EffectPropertyPtr create(
		const BW::StringRef & name,
		D3DXHANDLE hProperty,
		ID3DXEffect * pEffect )
	{
		BW_GUARD;

		MaterialBoolProxy* prop =
			new MaterialBoolProxy( name, pEffect, hProperty );
		BOOL v;
        pEffect->GetBool( hProperty, &v );
		prop->set( !!v, false );
		return prop;
	}

	virtual bool check( D3DXPARAMETER_DESC* propertyDesc )
	{
		BW_GUARD;

		return (propertyDesc->Class == D3DXPC_SCALAR && propertyDesc->Type == D3DXPT_BOOL);
	}
};


class IntProxyFactory : public Moo::EffectPropertyFactory
{
public:
	virtual Moo::EffectPropertyPtr create(
		const BW::StringRef & name,
		DataSectionPtr pSection )
	{
		BW_GUARD;

		MaterialIntProxy* prop = new MaterialIntProxy( name );
		prop->set( pSection->asInt(), false );
		return prop;
	}

	virtual Moo::EffectPropertyPtr create(
		const BW::StringRef & name,
		D3DXHANDLE hProperty,
		ID3DXEffect * pEffect )
	{
		BW_GUARD;

		MaterialIntProxy* prop =
			new MaterialIntProxy( name, pEffect, hProperty );
		int32 v;
        pEffect->GetInt( hProperty, &v );
		prop->set( v, false );
		prop->attach( hProperty, pEffect );
		return prop;
	}

	virtual bool check( D3DXPARAMETER_DESC* propertyDesc )
	{
		BW_GUARD;

		return (propertyDesc->Class == D3DXPC_SCALAR && propertyDesc->Type == D3DXPT_INT );
	}
};


class FloatProxyFactory : public Moo::EffectPropertyFactory
{
public:
	virtual Moo::EffectPropertyPtr create(
		const BW::StringRef & name,
		DataSectionPtr pSection )
	{
		BW_GUARD;

		MaterialFloatProxy* proxy = new MaterialFloatProxy( name );
		proxy->set( pSection->asFloat(), false );
		return proxy;
	}

	virtual Moo::EffectPropertyPtr create(
		const BW::StringRef & name,
		D3DXHANDLE hProperty,
		ID3DXEffect * pEffect )
	{
		BW_GUARD;

		MaterialFloatProxy* proxy =
			new MaterialFloatProxy( name, pEffect, hProperty );
		float v;
        pEffect->GetFloat( hProperty, &v );
		proxy->set( v, false );
		proxy->attach( hProperty, pEffect );
		return proxy;
	}

	virtual bool check( D3DXPARAMETER_DESC* propertyDesc )
	{
		BW_GUARD;

		return (propertyDesc->Class == D3DXPC_SCALAR && propertyDesc->Type == D3DXPT_FLOAT);
	}
};


class Vector4ProxyFactory : public Moo::EffectPropertyFactory
{
public:
	virtual Moo::EffectPropertyPtr create(
		const BW::StringRef & name,
		DataSectionPtr pSection )
	{
		BW_GUARD;

		MaterialVector4Proxy* proxy = new MaterialVector4Proxy( name );
		proxy->set( pSection->asVector4(), false );
		return proxy;
	}

	virtual Moo::EffectPropertyPtr create(
		const BW::StringRef & name,
		D3DXHANDLE hProperty,
		ID3DXEffect * pEffect )
	{
		BW_GUARD;

		MaterialVector4Proxy* proxy =
			new MaterialVector4Proxy( name, pEffect, hProperty );
		Vector4 v;
        pEffect->GetVector( hProperty, &v );
		proxy->set( v, false );
		return proxy;
	}

	virtual bool check( D3DXPARAMETER_DESC* propertyDesc )
	{
		BW_GUARD;

		return (propertyDesc->Class == D3DXPC_VECTOR && propertyDesc->Type == D3DXPT_FLOAT);
	}
};


class MatrixProxyFactory : public Moo::EffectPropertyFactory
{
public:
	virtual Moo::EffectPropertyPtr create(
		const BW::StringRef & name,
		DataSectionPtr pSection )
	{
		BW_GUARD;

		MaterialMatrixProxy* prop = new MaterialMatrixProxy( name );
        Matrix m;
		m.row( 0, pSection->readVector4( "row0" ) );
		m.row( 1, pSection->readVector4( "row1" ) );
		m.row( 2, pSection->readVector4( "row2" ) );
		m.row( 3, pSection->readVector4( "row3" ) );
		prop->setMatrix( m );
		return prop;
	}

	virtual Moo::EffectPropertyPtr create(
		const BW::StringRef & name,
		D3DXHANDLE hProperty,
		ID3DXEffect * pEffect )
	{
		BW_GUARD;

		MaterialMatrixProxy* prop =
			new MaterialMatrixProxy( name, pEffect, hProperty );
		Matrix m;
        pEffect->GetMatrix( hProperty, &m );
		prop->setMatrix( m );
		return prop;
	}

	virtual bool check( D3DXPARAMETER_DESC* propertyDesc )
	{
		BW_GUARD;

		return ((propertyDesc->Class == D3DXPC_MATRIX_ROWS || propertyDesc->Class == D3DXPC_MATRIX_COLUMNS) &&
				propertyDesc->Type == D3DXPT_FLOAT);
	}
};


class ColourProxyFactory : public Moo::EffectPropertyFactory
{
public:
	virtual Moo::EffectPropertyPtr create(
		const BW::StringRef & name,
		DataSectionPtr pSection )
	{
		BW_GUARD;

		MaterialColourProxy* proxy = new MaterialColourProxy( name );
		proxy->setVector4( pSection->asVector4(), false );
		return proxy;
	}

	virtual Moo::EffectPropertyPtr create(
		const BW::StringRef & name,
		D3DXHANDLE hProperty,
		ID3DXEffect * pEffect )
	{
		BW_GUARD;

		MaterialColourProxy* proxy =
			new MaterialColourProxy( name, pEffect, hProperty );
		Vector4 v;
        pEffect->GetVector( hProperty, &v );
		proxy->setVector4( v, false );
		return proxy;
	}

	virtual bool check( D3DXPARAMETER_DESC* propertyDesc )
	{
		BW_GUARD;

		return (propertyDesc->Class == D3DXPC_VECTOR &&
			propertyDesc->Type == D3DXPT_FLOAT);
	}
private:
};


class TextureProxyFactory : public Moo::EffectPropertyFactory
{
public:
	virtual Moo::EffectPropertyPtr create(
		const BW::StringRef & name,
		DataSectionPtr pSection )
	{
		BW_GUARD;

		MaterialTextureProxy* prop = new MaterialTextureProxy( name );
		prop->set( pSection->asString(), false );
		return prop;
	}

	virtual Moo::EffectPropertyPtr create(
		const BW::StringRef & name,
		D3DXHANDLE hProperty,
		ID3DXEffect * pEffect )
	{
		BW_GUARD;

		MaterialTextureProxy* prop =
			new MaterialTextureProxy( name, pEffect, hProperty );
		return prop;
	}

	virtual bool check( D3DXPARAMETER_DESC* propertyDesc )
	{
		BW_GUARD;

		return (propertyDesc->Class == D3DXPC_OBJECT &&
			(propertyDesc->Type == D3DXPT_TEXTURE ||
			propertyDesc->Type == D3DXPT_TEXTURE1D ||
			propertyDesc->Type == D3DXPT_TEXTURE2D ||
			propertyDesc->Type == D3DXPT_TEXTURE3D ||
			propertyDesc->Type == D3DXPT_TEXTURECUBE ));
	}
};


class TextureFeedProxyFactory : public Moo::EffectPropertyFactory
{
public:
	virtual Moo::EffectPropertyPtr create(
		const BW::StringRef & name,
		DataSectionPtr pSection )
	{
		BW_GUARD;

		MaterialTextureFeedProxy* prop = new MaterialTextureFeedProxy( name );
		prop->set( pSection->readString( "default", "" ), false );
		prop->setTextureFeed( pSection->asString() );
		return prop;
	}

	virtual Moo::EffectPropertyPtr create(
		const BW::StringRef & name,
		D3DXHANDLE hProperty,
		ID3DXEffect * pEffect )
	{
		BW_GUARD;

		MaterialTextureFeedProxy* prop =
			new MaterialTextureFeedProxy( name, pEffect, hProperty );
		return prop;
	}

	virtual bool check( D3DXPARAMETER_DESC* propertyDesc )
	{
		BW_GUARD;

		return (propertyDesc->Class == D3DXPC_OBJECT &&
			(propertyDesc->Type == D3DXPT_TEXTURE ||
			propertyDesc->Type == D3DXPT_TEXTURE1D ||
			propertyDesc->Type == D3DXPT_TEXTURE2D ||
			propertyDesc->Type == D3DXPT_TEXTURE3D ||
			propertyDesc->Type == D3DXPT_TEXTURECUBE ));
	}
};


///////////////////////////////////////////////////////////////////////////////
// Section : Functions to add properties of different types
///////////////////////////////////////////////////////////////////////////////

void addBoolProperty(	const BW::string & descName,
						const BW::string & uiName, const BW::string & uiDesc,
						const Moo::EffectPropertyPtr & pProperty,
						bool canExposeToScript, bool canUseTextureFeeds, 
						MaterialPropertiesUser * callback, GeneralEditor * editor )
{
	BW_GUARD;

	MaterialBoolProxy* pBoolProxy = dynamic_cast<MaterialBoolProxy*>(pProperty.get());
	MF_ASSERT( pBoolProxy != NULL );

	if (s_boolProxy[ descName ])
	{
		s_boolProxy[ descName ]->addProperty( pBoolProxy );
	}
	else
	{
		s_boolProxy[ descName ] =
			new BoolProxyType( pBoolProxy, callback );

		BoolProxyPtr proxy = callback->boolProxy( s_boolProxy[ descName ],
				new BWFunctor0RC< BoolProxyType, bool >( s_boolProxy[ descName ].get(), &(BoolProxyType::get) ),
				new BWFunctor1< BoolProxyType, bool >( s_boolProxy[ descName ].get(), &(BoolProxyType::set) ),
				uiName, descName );

		BW::string exposedToScriptName = callback->exposedToScriptName( descName );
		
		GenBoolProperty* pProp = new GenBoolProperty( Name( uiName ), proxy );
		pProp->descName( Name( descName ) );
		pProp->UIDesc( Name( uiDesc ) );
		pProp->exposedToScriptName( Name( exposedToScriptName ) );
		pProp->canExposeToScript( canExposeToScript );
		editor->addProperty( pProp );
	}
}


void addEnumProperty(	const BW::string & descName,
						const BW::string & uiName, const BW::string & uiDesc,
						const Moo::EffectPropertyPtr & pProperty,
						bool canExposeToScript, bool canUseTextureFeeds, 
						MaterialPropertiesUser * callback, GeneralEditor * editor )
{
	BW_GUARD;

	MaterialIntProxy* pIntProxy = dynamic_cast<MaterialIntProxy*>(pProperty.get());
	MF_ASSERT( pIntProxy != NULL );

	BW::string enumType;
	if( pIntProxy->stringAnnotation( "EnumType", enumType ) )
	{
		if( DXEnum::instance().isEnum( enumType ) )
		{
			DataSectionPtr enumSec = new XMLSection( enumType );
			for( DXEnum::size_type i = 0;
					i != DXEnum::instance().size( enumType ); ++i)
			{
					BW::string name = DXEnum::instance().entry( enumType, i );
					enumSec->writeInt( name,
							DXEnum::instance().value( enumType, name ) );
			}
			
			if (s_enumProxy[ descName ])
			{
				s_enumProxy[ descName ]->addProperty( new MaterialEnumProxy( enumType, pIntProxy ) );
			}
			else
			{
				s_enumProxy[ descName ] =
					new EnumProxyType(
						new MaterialEnumProxy( enumType, pIntProxy ), callback );
		
				IntProxyPtr proxy = callback->enumProxy( s_enumProxy[ descName ],
						new BWFunctor0RC< EnumProxyType, uint32 >( s_enumProxy[ descName ].get(), &(EnumProxyType::get) ),
						new BWFunctor1< EnumProxyType, uint32 >( s_enumProxy[ descName ].get(), &(EnumProxyType::set) ),
						uiName, descName );
							
				ChoiceProperty* pProp = new ChoiceProperty( Name( uiName ), proxy, enumSec );
				pProp->descName( Name( descName ) );
				pProp->UIDesc( Name( uiDesc ) );
				editor->addProperty( pProp );
			}
		}
		else
		{
			enumType.clear();
		}
	}

	if( !enumType.size() )
	{
		if (s_intProxy[ descName ])
		{
			s_intProxy[ descName ]->addProperty( (MaterialIntProxy*)pProperty.getObject() );
		}
		else
		{
			s_intProxy[ descName ] =
				new IntProxyType( pIntProxy, callback );

			IntProxyPtr proxy = callback->intProxy( s_intProxy[descName],
					new BWFunctor0RC< IntProxyType, int >( s_intProxy[ descName ].get(), &(IntProxyType::get) ),
					new BWFunctor1< IntProxyType, int >( s_intProxy[ descName ].get(), &(IntProxyType::set) ),
					new BWFunctor2R< IntProxyType, int&, int&, bool >( s_intProxy[ descName ].get(), &(IntProxyType::getRange) ),
					uiName, descName );

			BW::string exposedToScriptName = callback->exposedToScriptName( descName );
			
			GenIntProperty* pProp = new GenIntProperty( Name( uiName ), proxy );
			pProp->descName( Name( descName ) );
			pProp->UIDesc( Name( uiDesc ) );
			pProp->exposedToScriptName( Name( exposedToScriptName ) );
			pProp->canExposeToScript( canExposeToScript );
			editor->addProperty( pProp );
		}
	}
}


void addFloatProperty(const BW::string & descName,
						const BW::string & uiName, const BW::string & uiDesc,
						const Moo::EffectPropertyPtr & pProperty,
						bool canExposeToScript, bool canUseTextureFeeds, 
						MaterialPropertiesUser * callback, GeneralEditor * editor )
{
	BW_GUARD;

	MaterialFloatProxy* pFloatProxy = dynamic_cast<MaterialFloatProxy*>( pProperty.get() );
	MF_ASSERT( pFloatProxy != NULL );

	if (s_floatProxy[ descName ])
	{
		s_floatProxy[ descName ]->addProperty( pFloatProxy );
	}
	else
	{
		s_floatProxy[ descName ] =
			new FloatProxyType( pFloatProxy, callback );

		FloatProxyPtr proxy = callback->floatProxy( s_floatProxy[ descName ],
				new BWFunctor0RC< FloatProxyType, float >( s_floatProxy[ descName ].get(), &(FloatProxyType::get) ),
				new BWFunctor1< FloatProxyType, float >( s_floatProxy[ descName ].get(), &(FloatProxyType::set) ),
				new BWFunctor3R< FloatProxyType, float&, float&, int&, bool >( s_floatProxy[ descName ].get(), &(FloatProxyType::getRange) ),
				uiName, descName );

		BW::string exposedToScriptName = callback->exposedToScriptName( descName );
		
		GenFloatProperty* pProp = new GenFloatProperty( Name( uiName ), proxy );
		pProp->descName( Name( descName ) );
		pProp->UIDesc( Name( uiDesc ) );
		pProp->exposedToScriptName( Name( exposedToScriptName ) );
		pProp->canExposeToScript( canExposeToScript );
		editor->addProperty( pProp );
	}
}


void addVector4Property(const BW::string & descName,
						const BW::string & uiName, const BW::string & uiDesc,
						const Moo::EffectPropertyPtr & pProperty,
						bool canExposeToScript, bool canUseTextureFeeds, 
						MaterialPropertiesUser * callback, GeneralEditor * editor )
{
	BW_GUARD;

	MaterialVector4Proxy* pVector4Proxy = dynamic_cast<MaterialVector4Proxy*>( pProperty.get() );
	MF_ASSERT( pVector4Proxy != NULL );

	if (s_vector4Proxy[ descName ])
	{
		s_vector4Proxy[ descName ]->addProperty( pVector4Proxy );
	}
	else
	{
		s_vector4Proxy[ descName ] =
			new Vector4ProxyType( pVector4Proxy, callback );

		Vector4ProxyPtr proxy = callback->vector4Proxy( s_vector4Proxy[ descName ],
				new BWFunctor0RC< Vector4ProxyType, Vector4 >( s_vector4Proxy[ descName ].get(), &(Vector4ProxyType::get) ),
				new BWFunctor1< Vector4ProxyType, Vector4 >( s_vector4Proxy[ descName ].get(), &(Vector4ProxyType::set) ),
				uiName, descName );

		BW::string exposedToScriptName = callback->exposedToScriptName( descName );
		
		BW::string UIWidget = MaterialUtility::UIWidget( pVector4Proxy );

		if ((UIWidget == "Color") || (UIWidget == "Colour"))
		{
			ColourProperty* pColourProp = new ColourProperty( Name( uiName ), (Vector4ProxyPtr)proxy );
			pColourProp->exposedToScriptName( Name( exposedToScriptName  ));
			pColourProp->descName( Name( descName ) );
			pColourProp->UIDesc( Name( uiDesc ) );
			pColourProp->exposedToScriptName( Name( exposedToScriptName ) );
			pColourProp->canExposeToScript( canExposeToScript );	
			editor->addProperty( pColourProp );
		}
		else // Must be a vector
		{
			Vector4Property* pVectorProp = new Vector4Property( Name( uiName ), proxy );
			pVectorProp->exposedToScriptName( Name( exposedToScriptName ) );
			pVectorProp->descName( Name( descName ) );
			pVectorProp->UIDesc( Name( uiDesc ) );	
			pVectorProp->exposedToScriptName( Name( exposedToScriptName ) );
			pVectorProp->canExposeToScript( canExposeToScript );
			editor->addProperty( pVectorProp );
		}
	}
}


void addTextureProperty( const BW::string & descName,
						const BW::string & uiName, const BW::string & uiDesc,
						const Moo::EffectPropertyPtr & pProperty,
						bool canExposeToScript, bool canUseTextureFeeds, 
						MaterialPropertiesUser * callback, GeneralEditor * editor )
{
	BW_GUARD;

	MaterialTextureProxy* pTextureProxy = dynamic_cast<MaterialTextureProxy*>( pProperty.get() );
	MaterialTextureFeedProxy* pTextureFeedProxy = dynamic_cast<MaterialTextureFeedProxy*>( pProperty.get() );
	MF_ASSERT( pTextureProxy != NULL || pTextureFeedProxy != NULL );

	if (s_textureProxy[ descName ] && pTextureProxy)
	{
		s_textureProxy[ descName ]->addProperty( pTextureProxy );
	}
	else if (s_textureFeedProxy[descName] && pTextureFeedProxy)
	{
		s_textureFeedProxy[ descName ]->addProperty( pTextureFeedProxy );
	}
	else
	{
		TextProperty* pProp = NULL;
		
		BW::string UIWidget;
		if (pTextureProxy)
		{
			s_textureProxy[ descName ] =
				new TextureProxyType( pTextureProxy, callback );
	
			StringProxyPtr proxy = callback->textureProxy( s_textureProxy[ descName ],
					new BWFunctor0RC< TextureProxyType, BW::string >( s_textureProxy[ descName ].get(), &(TextureProxyType::get) ),
					new BWFunctor1< TextureProxyType, BW::string >( s_textureProxy[ descName ].get(), &(TextureProxyType::set) ),
					uiName, descName );

			pProp = new TextProperty( Name( uiName ), proxy );
			UIWidget = MaterialUtility::UIWidget( pTextureProxy );
		}
		else
		{
			s_textureFeedProxy[ descName ] =
				new TextureFeedProxyType( pTextureFeedProxy, callback );
	
			StringProxyPtr proxy = callback->textureProxy( s_textureFeedProxy[ descName ],
					new BWFunctor0RC< TextureFeedProxyType, BW::string >( s_textureFeedProxy[ descName ].get(), &(TextureFeedProxyType::get) ),
					new BWFunctor1< TextureFeedProxyType, BW::string >( s_textureFeedProxy[ descName ].get(), &(TextureFeedProxyType::set) ),
					uiName, descName );

			pProp = new TextProperty( Name( uiName ), proxy );
			UIWidget = MaterialUtility::UIWidget( pTextureFeedProxy );
		}
	
		pProp->descName( Name( descName ) );
		if (UIWidget == "RenderTarget")
		{
			pProp->fileFilter( Name( "Render Targets (*.rt)|*.rt|Render Targets (*.rt)|*.rt||" ) );
		}
		else if (UIWidget == "CubeMap")
		{
			pProp->fileFilter( Name( "Cube maps (*.dds;*.texanim)|*.dds;*.texanim|DDS files(*.dds)|*.dds|"
				"Animated Textures (*.texanim)|*.texanim||" ) );
		}
		else
		{
			pProp->fileFilter( Name( "Texture files(*.bmp;*.tga;*.jpg;*.png;*.dds;*.texanim;*.rt)|*.bmp;*.tga;*.jpg;*.png;*.dds;*.texanim|"
				"Bitmap files(*.bmp)|*.bmp|"
				"Targa files(*.tga)|*.tga|"
				"Jpeg files(*.jpg)|*.jpg|"
				"Png files(*.png)|*.png|"
				"DDS files(*.dds)|*.dds|"
				"Animated Textures (*.texanim)|*.texanim|"
				"Render Target (*.rt)|*.rt||" ) );
		}
		pProp->defaultDir( bw_utf8tow( callback->defaultTextureDir() ) );
		pProp->UIDesc( Name( uiDesc ) );
		if (pTextureFeedProxy)
		{
			BW::string textureFeed = callback->textureFeed( descName );
			pProp->canTextureFeed( true );
			pProp->textureFeed( bw_utf8tow( textureFeed ) );
		}
		editor->addProperty( pProp );
	}
}


} // anonymous namespace


///////////////////////////////////////////////////////////////////////////////
// Section : MaterialProperties
///////////////////////////////////////////////////////////////////////////////

/**
 *	Important - this must be called at runtime, before you begin
 *	editing material properties.  The reason is that in
 *	moo/managed_effect the property processors are set up in the
 *	g_effectPropertyProcessors map at static initialisation time;
 *	our own processors are meant to override the default ones.
 */
/*static*/ bool MaterialProperties::runtimeInitMaterialProperties()
{
	BW_GUARD;

#define EP(x) Moo::EffectPropertyFactory::setFactory( #x, new x##ProxyFactory )
	EP(Vector4);
	EP(Matrix);
	EP(Float);
	EP(Bool);
	EP(Texture);
	EP(Int);
	EP(TextureFeed);

	return true;
}


/*static*/ void MaterialProperties::beginProperties()
{
	BW_GUARD;

	s_textureProxy.clear();
	s_textureFeedProxy.clear();
	s_boolProxy.clear();
	s_intProxy.clear();
	s_enumProxy.clear();
	s_floatProxy.clear();
	s_vector4Proxy.clear();
}


/*static*/ void MaterialProperties::endProperties()
{
	BW_GUARD;

	// Just clear maps again to be tidy.
	beginProperties();
}


/*static*/
bool MaterialProperties::addMaterialProperties( Moo::EffectMaterialPtr material, GeneralEditor * editor,
	bool canExposeToScript, bool canUseTextureFeeds, MaterialPropertiesUser * callback )
{
	BW_GUARD;

	if (!callback)
	{
		return false;
	}

	material->replaceDefaults();

	BW::vector<Moo::EffectPropertyPtr> existingProps;

	if ( material->pEffect() )
	{
		Moo::EffectMaterial::Properties& properties = material->properties();
		Moo::EffectMaterial::Properties::reverse_iterator it = properties.rbegin();
		Moo::EffectMaterial::Properties::reverse_iterator end = properties.rend();

		for (; it != end; ++it )
		{
			MF_ASSERT( it->second );
			D3DXHANDLE hParameter = it->first;
			EditorEffectProperty* pProperty = dynamic_cast<EditorEffectProperty*>(it->second.get());

			if (pProperty && 
				MaterialUtility::artistEditable( pProperty ))
			{
				BW::string uiName = MaterialUtility::UIName( pProperty );
				BW::string uiDesc = MaterialUtility::UIDesc( pProperty );
				
				// Use descName for the UI if uiName doesn't exist... (bug 6940)
				if (uiName.empty()) uiName = pProperty->name();
				
				if (MaterialTextureProxy* pTextureProperty = dynamic_cast<MaterialTextureProxy*>( pProperty ))
				{
					addTextureProperty( pProperty->name(), uiName, uiDesc,
						pProperty, canExposeToScript, canUseTextureFeeds, callback, editor );
				}
				else if (MaterialTextureFeedProxy* pTextureFeedProperty = dynamic_cast<MaterialTextureFeedProxy*>( pProperty ))
				{
					addTextureProperty( pProperty->name(), uiName, uiDesc,
						pProperty, canExposeToScript, canUseTextureFeeds, callback, editor );
				}
				else if (MaterialBoolProxy* pBoolProperty = dynamic_cast<MaterialBoolProxy*>( pProperty ))
				{
					addBoolProperty( pProperty->name(), uiName, uiDesc,
						pProperty, canExposeToScript, canUseTextureFeeds, callback, editor );
				}
				else if (MaterialIntProxy* pIntProperty = dynamic_cast<MaterialIntProxy*>( pProperty ))
				{
					addEnumProperty( pProperty->name(), uiName, uiDesc,
						pProperty, canExposeToScript, canUseTextureFeeds, callback, editor );
				}
				else if (MaterialFloatProxy* pFloatProperty = dynamic_cast<MaterialFloatProxy*>( pProperty ))
				{
					addFloatProperty( pProperty->name(), uiName, uiDesc,
						pProperty, canExposeToScript, canUseTextureFeeds, callback, editor );
				}
				else if (MaterialVector4Proxy* pVector4Property = dynamic_cast<MaterialVector4Proxy*>( pProperty ))
				{
					addVector4Property( pProperty->name(), uiName, uiDesc,
						pProperty, canExposeToScript, canUseTextureFeeds, callback, editor );
				}
			}
		}
	}

	return true;
}

BW_END_NAMESPACE

