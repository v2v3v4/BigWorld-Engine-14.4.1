

#include "pch.hpp"
#include "material_proxies.hpp"
#include "resmgr/auto_config.hpp"

BW_BEGIN_NAMESPACE

namespace
{
	AutoConfigString s_notFoundBmp( "system/notFoundBmp" );

	const char * RANGE_MIN = "UIMin";
	const char * RANGE_MAX = "UIMax";
	const char * RANGE_DIGITS = "UIDigits";

} // anonymous namespace

///////////////////////////////////////////////////////////////////////////////
// Section : EditorEffectProperty
///////////////////////////////////////////////////////////////////////////////

/**
 *	EditorEffectProperty constructor.
 *	@param name the property's name.
 *	@param pEffect the ID3DXEffect this property exists in (can be NULL)
 *	@param hProperty the property handle for this property
 */
EditorEffectProperty::EditorEffectProperty(
	const BW::StringRef & name,
	ID3DXEffect * pEffect,
	D3DXHANDLE hProperty ) : EffectProperty( name )
{
	BW_GUARD;

	// Try to cache all annoations for this property
	D3DXPARAMETER_DESC propertyDesc;
	if (pEffect && hProperty && 
		S_OK == pEffect->GetParameterDesc( hProperty, &propertyDesc) )
	{
		pAnnotations_ = new AnnotationData;

		// Cache the name of the property
		pAnnotations_->name_ = propertyDesc.Name;

		// Iterate over property annotations 
		for (UINT annotationIndex = 0; annotationIndex < propertyDesc.Annotations; annotationIndex++)
		{
			D3DXHANDLE hAnnotation = pEffect->GetAnnotation( hProperty, annotationIndex );
			D3DXPARAMETER_DESC desc;
			if (hAnnotation &&
				S_OK == pEffect->GetParameterDesc( hAnnotation, &desc))
			{
				// Check annotation types, we only store string, float, int and bool annotations
				if (desc.Type == D3DXPT_STRING && desc.Class == D3DXPC_OBJECT)
				{
					LPCSTR pString = NULL;
					HRESULT hr = pEffect->GetString( hAnnotation, &pString );
					if (pString && hr == S_OK)
					{
						pAnnotations_->stringAnnotations_[desc.Name] = pString;
					}
				}
				else if (desc.Type == D3DXPT_FLOAT && desc.Class == D3DXPC_SCALAR)
				{
					float value = 0;
					HRESULT hr = pEffect->GetFloat( hAnnotation, &value );
					if (hr == S_OK)
					{
						pAnnotations_->floatAnnotations_[desc.Name] = value;
					}
				}
				else if (desc.Type == D3DXPT_INT && desc.Class == D3DXPC_SCALAR)
				{
					int32 value = 0;
					HRESULT hr = pEffect->GetInt( hAnnotation, &value );
					if (hr == S_OK)
					{
						pAnnotations_->intAnnotations_[desc.Name] = value;
					}
				}
				else if (desc.Type == D3DXPT_BOOL && desc.Class == D3DXPC_SCALAR)
				{
					BOOL value = FALSE;
					HRESULT hr = pEffect->GetBool( hAnnotation, &value );
					if (hr == S_OK)
					{
						pAnnotations_->boolAnnotations_[desc.Name] = value == TRUE;
					}
				}
				else
				{
					INFO_MSG( "EditorEffectProperty::EditorEffectProperty - Unknown annotation type %s\n", desc.Name );
				}
			}
		}
	}
}


/**
 *	Get a bool annotation for this property
 *	@param annotation the name of the annotation
 *	@param retVal the value of the annotation, if the annotation does not exist
 *			or is not of bool type this is unchanged
 *	@return true if the bool annotation was found
 */
bool EditorEffectProperty::boolAnnotation( const BW::string& annotation, bool& retVal ) const
{
	BW_GUARD;

	if (pAnnotations_.exists())
	{
		BoolAnnotations::const_iterator it = pAnnotations_->boolAnnotations_.find( annotation );
		if (it != pAnnotations_->boolAnnotations_.end())
		{
			retVal = it->second;

			return true;
		}
	}
	return false;
}


/**
 *	Get a int annotation for this property
 *	@param annotation the name of the annotation
 *	@param retVal the value of the annotation, if the annotation does not exist
 *			or is not of int type this is unchanged
 *	@return true if the int annotation was found
 */
bool EditorEffectProperty::intAnnotation( const BW::string& annotation, int32& retVal ) const
{
	BW_GUARD;

	if (pAnnotations_.exists())
	{
		IntAnnotations::const_iterator it = pAnnotations_->intAnnotations_.find( annotation );
		if (it != pAnnotations_->intAnnotations_.end())
		{
			retVal = it->second;

			return true;
		}
	}

	return false;
}


/**
 *	Get a int annotation for this property
 *	@param annotation the name of the annotation
 *	@param retVal the value of the annotation, if the annotation does not exist
 *			or is not of string type this is unchanged
 *	@return true if the string annotation was found
 */
bool EditorEffectProperty::stringAnnotation( const BW::string& annotation, BW::string& retVal ) const
{
	BW_GUARD;
	if (pAnnotations_.exists())
	{
		StringAnnotations::const_iterator it = pAnnotations_->stringAnnotations_.find( annotation );
		if (it != pAnnotations_->stringAnnotations_.end())
		{
			retVal = it->second;
			return true;
		}
	}


	return false;
}


/**
 *	Get a float annotation for this property
 *	@param annotation the name of the annotation
 *	@param retVal the value of the annotation, if the annotation does not exist
 *			or is not of float type this is unchanged
 *	@return true if the float annotation was found
 */
bool EditorEffectProperty::floatAnnotation( const BW::string& annotation, float& retVal ) const
{
	BW_GUARD;

	if (pAnnotations_.exists())
	{
		FloatAnnotations::const_iterator it = pAnnotations_->floatAnnotations_.find( annotation );
		if (it != pAnnotations_->floatAnnotations_.end())
		{
			retVal = it->second;
			return true;
		}

	}

	return false;
}


/**
 *	Get the name of this property
 *	@return the name of the property or an empty string if the name is unknown
 */
const BW::string& EditorEffectProperty::name() const
{
	if (pAnnotations_.exists())
	{
		return pAnnotations_->name_;
	}

	return AnnotationData::s_emptyString_;
}


/** 
 *	Set the parent for this property, this sets the annotations for the property
 *	to be the same as the parent property
 *	@param pParent the parent property
 */
void EditorEffectProperty::setParent( const Moo::EffectProperty* pParent )
{
	const EditorEffectProperty* pEPParent = dynamic_cast<const EditorEffectProperty*>( pParent );
	if (pEPParent)
	{
		pAnnotations_ = pEPParent->pAnnotations_;
	}
}

BW::string EditorEffectProperty::AnnotationData::s_emptyString_;

///////////////////////////////////////////////////////////////////////////////
// Section : MaterialBoolProxy
///////////////////////////////////////////////////////////////////////////////

bool MaterialBoolProxy::apply( ID3DXEffect* pEffect, D3DXHANDLE hProperty )
{
	BW_GUARD;

	return SUCCEEDED( pEffect->SetBool( hProperty, (BOOL)!!value_ ) );
}


void MaterialBoolProxy::set( bool value, bool transient, bool addBarrier )
{
	value_ = value;
};


bool MaterialBoolProxy::get() const
{
	return value_;
}


bool MaterialBoolProxy::be( const Vector4 & v )
{
	value_ = (v.x != 0);
	return true;
}


void MaterialBoolProxy::asVector4( Vector4 & v ) const
{
	if (v.x != 0.f && !value_)
	{
		v.x = 0.f;
	}
	else if (v.x == 0.f && value_)
	{
		v.x = 1.f;
	}
}


bool MaterialBoolProxy::be( const bool & b )
{
	value_ = b;
	return true;
}


bool MaterialBoolProxy::getBool( bool & b ) const
{
	b = value_;
	return true;
}


void MaterialBoolProxy::save( DataSectionPtr pSection )
{
	BW_GUARD;

	pSection->writeBool( "Bool", value_ );
}

Moo::EffectProperty* MaterialBoolProxy::clone() const
{
	MaterialBoolProxy* pClone = new MaterialBoolProxy( this->propName() );
	pClone->value_ = value_;
	pClone->setParent( this );
	return pClone;
}


///////////////////////////////////////////////////////////////////////////////
// Section : MaterialIntProxy
///////////////////////////////////////////////////////////////////////////////


MaterialIntProxy::MaterialIntProxy(
	const BW::StringRef & name, ID3DXEffect * pEffect, D3DXHANDLE hProperty ) :
	EditorEffectProperty( name, pEffect, hProperty ),
	value_(),
	ranged_( false ),
	min_(),
    max_()
{
}

bool MaterialIntProxy::apply( ID3DXEffect* pEffect, D3DXHANDLE hProperty )
{
	BW_GUARD;

	return SUCCEEDED( pEffect->SetInt( hProperty, value_ ) );
}

void MaterialIntProxy::set( int32 value, bool transient,
													bool addBarrier )
{
    value_ = value;
}

void MaterialIntProxy::save( DataSectionPtr pSection )
{
	BW_GUARD;

	pSection->writeInt( "Int", value_ );
}

bool MaterialIntProxy::getRange( int32& min, int32& max ) const
{
    min = min_;
    max = max_;
    return ranged_;
}

void MaterialIntProxy::setRange( int32 min, int32 max )
{
	ranged_ = true;
    min_ = min;
    max_ = max;
}

void MaterialIntProxy::attach( D3DXHANDLE hProperty, ID3DXEffect* pEffect )
{
	BW_GUARD;

	D3DXHANDLE minHandle = pEffect->GetAnnotationByName( hProperty, RANGE_MIN );
	D3DXHANDLE maxHandle = pEffect->GetAnnotationByName( hProperty, RANGE_MAX );
	if( minHandle && maxHandle )
	{
		D3DXPARAMETER_DESC minPara, maxPara;
		if( SUCCEEDED( pEffect->GetParameterDesc( minHandle, &minPara ) ) &&
			SUCCEEDED( pEffect->GetParameterDesc( maxHandle, &maxPara ) ) &&
			minPara.Type == D3DXPT_INT &&
			maxPara.Type == D3DXPT_INT )
		{
			int min, max;
			if( SUCCEEDED( pEffect->GetInt( minHandle, &min ) ) &&
				SUCCEEDED( pEffect->GetInt( maxHandle, &max ) ) )
			{
				setRange( min, max );
			}
		}
	}
}


void MaterialIntProxy::setParent( const EffectProperty* pParent ) 
{
	EditorEffectProperty::setParent( pParent );
	const MaterialIntProxy* pIntParent = dynamic_cast<const MaterialIntProxy*>( pParent );
	if (pIntParent)
	{
		ranged_ = pIntParent->ranged_;
		min_ = pIntParent->min_;
		max_ = pIntParent->max_;
	}
	else
	{
		INFO_MSG( "MaterialIntProxy - setParent - incorrect parent type\n" );
	}
}


Moo::EffectProperty* MaterialIntProxy::clone() const
{
	MaterialIntProxy* pClone = new MaterialIntProxy( this->propName() );
	pClone->value_ = value_;
	pClone->setParent( this );
	return pClone;
}

///////////////////////////////////////////////////////////////////////////////
// Section : MaterialFloatProxy
///////////////////////////////////////////////////////////////////////////////

void MaterialFloatProxy::save( DataSectionPtr pSection )
{
	BW_GUARD;

	pSection->writeFloat( "Float", value_ );
}


bool MaterialFloatProxy::getRange( float& min, float& max, int& digits ) const
{
    min = min_;
    max = max_;
    digits = digits_;
	return ranged_;
}


void MaterialFloatProxy::setRange( float min, float max, int digits )
{
    ranged_ = true;
    min_ = min;
    max_ = max;
    digits_ = digits;
}


void MaterialFloatProxy::attach( D3DXHANDLE hProperty, ID3DXEffect* pEffect )
{
	BW_GUARD;

	D3DXHANDLE minHandle = pEffect->GetAnnotationByName( hProperty, RANGE_MIN );
	D3DXHANDLE maxHandle = pEffect->GetAnnotationByName( hProperty, RANGE_MAX );
	if( minHandle && maxHandle )
	{
		D3DXPARAMETER_DESC minPara, maxPara;
		if( SUCCEEDED( pEffect->GetParameterDesc( minHandle, &minPara ) ) &&
			SUCCEEDED( pEffect->GetParameterDesc( maxHandle, &maxPara ) ) &&
			minPara.Type == D3DXPT_FLOAT &&
			maxPara.Type == D3DXPT_FLOAT )
		{
			float min, max;
			if( SUCCEEDED( pEffect->GetFloat( minHandle, &min ) ) &&
				SUCCEEDED( pEffect->GetFloat( maxHandle, &max ) ) )
			{
				int digits = 0;
				D3DXPARAMETER_DESC digitsPara;
				D3DXHANDLE digitsHandle = pEffect->GetAnnotationByName( hProperty, RANGE_DIGITS );

				if( digitsHandle &&
					SUCCEEDED( pEffect->GetParameterDesc( digitsHandle, &digitsPara ) ) &&
					digitsPara.Type == D3DXPT_INT &&
					SUCCEEDED( pEffect->GetInt( digitsHandle, &digits ) ) )
					setRange( min, max, digits );
				else
				{
					float range = max - min;
					if( range < 0.0 )
						range = -range;
					while( range <= 99.9999 )// for range, normally include 2 valid digits
					{
						range *= 10;
						++digits;
						setRange( min, max, digits );
					}
				}
			}
		}
	}
}


void MaterialFloatProxy::setParent( const EffectProperty* pParent ) 
{
	EditorEffectProperty::setParent( pParent );
	const MaterialFloatProxy* pFloatParent = dynamic_cast<const MaterialFloatProxy*>( pParent );
	if (pFloatParent)
	{
		ranged_ = pFloatParent->ranged_;
		min_ = pFloatParent->min_;
		max_ = pFloatParent->max_;
		digits_ = pFloatParent->digits_;
	}
	else
	{
		INFO_MSG( "MaterialIntProxy - setParent - incorrect parent type\n" );
	}
}


Moo::EffectProperty* MaterialFloatProxy::clone() const
{
	MaterialFloatProxy* pClone = new MaterialFloatProxy( this->propName() );
	pClone->value_ = value_;
	pClone->setParent( this );
	return pClone;
}

///////////////////////////////////////////////////////////////////////////////
// Section : MaterialVector4Proxy
///////////////////////////////////////////////////////////////////////////////

bool MaterialVector4Proxy::apply( ID3DXEffect* pEffect, D3DXHANDLE hProperty )
{
	BW_GUARD;

	return SUCCEEDED( pEffect->SetVector( hProperty, &value_ ) );
}


bool MaterialVector4Proxy::be( const Vector4 & v )
{
	value_ = v;
	return true;
}


bool MaterialVector4Proxy::getVector( Vector4 & v ) const
{
	v = value_;
	return true;
}


Vector4 MaterialVector4Proxy::get() const
{
	return value_;
}


void MaterialVector4Proxy::set( Vector4 f, bool transient, bool addBarrier )
{
	value_ = f;
}


void MaterialVector4Proxy::save( DataSectionPtr pSection )
{
	BW_GUARD;

	pSection->writeVector4( "Vector4", value_ );
}


Moo::EffectProperty* MaterialVector4Proxy::clone() const
{
	MaterialVector4Proxy* pClone = new MaterialVector4Proxy( this->propName() );
	pClone->value_ = value_;
	pClone->setParent( this );
	return pClone;
}


///////////////////////////////////////////////////////////////////////////////
// Section : MaterialMatrixProxy
///////////////////////////////////////////////////////////////////////////////

bool MaterialMatrixProxy::apply( ID3DXEffect* pEffect, D3DXHANDLE hProperty )
{
	BW_GUARD;

	return SUCCEEDED( pEffect->SetMatrix( hProperty, &value_ ) );
}

bool MaterialMatrixProxy::setMatrix( const Matrix & m )
{
    value_ = m;
	return true;
}

void MaterialMatrixProxy::save( DataSectionPtr pSection )
{
	BW_GUARD;

    pSection = pSection->openSection( "Matrix", true );
    char buf[6] = "row0\0";
    for ( int i=0; i<4; i++ )
    {
        pSection->writeVector4(buf,value_.row(i));
        buf[3]++;
    }
}


bool MaterialMatrixProxy::commitState( bool revertToRecord, bool addUndoBarrier )
{
    return true;
}


bool MaterialMatrixProxy::be( const Vector4 & v )
{
	value_.translation( Vector3( v.x, v.y, v.z ) );
	return true;
}


void MaterialMatrixProxy::asVector4( Vector4 & v ) const
{
	v = value_.row( 3 );
}


bool MaterialMatrixProxy::be( const Matrix & m )
{
	value_ = m;
	return true;
}


bool MaterialMatrixProxy::getMatrix( Matrix & m ) const
{
	m = value_;
	return true;
}

	
Moo::EffectProperty* MaterialMatrixProxy::clone() const
{
	MaterialMatrixProxy* pClone = new MaterialMatrixProxy( this->propName() );
	pClone->value_ = value_;
	pClone->setParent( this );
	return pClone;
}


///////////////////////////////////////////////////////////////////////////////
// Section : MaterialColourProxy
///////////////////////////////////////////////////////////////////////////////

bool MaterialColourProxy::apply( ID3DXEffect* pEffect, D3DXHANDLE hProperty )
{
	BW_GUARD;

	return SUCCEEDED( pEffect->SetVector( hProperty, &value_ ) );
}


Moo::Colour MaterialColourProxy::get() const
{
	return Moo::Colour(value_) / 255.f;
}


void MaterialColourProxy::set( Moo::Colour f, bool transient, bool addBarrier )
{
	value_ = Vector4( static_cast<float*>(f) ) * 255.f;
}


Vector4 MaterialColourProxy::getVector4() const
{
	return value_;
}


void MaterialColourProxy::setVector4( Vector4 f, bool transient )
{
	value_ = f;
}


void MaterialColourProxy::save( DataSectionPtr pSection )
{
	BW_GUARD;

	pSection->writeVector4( "Colour", value_ );
}


Moo::EffectProperty* MaterialColourProxy::clone() const
{
	MaterialColourProxy* pClone = new MaterialColourProxy( this->propName() );
	pClone->value_ = value_;
	pClone->setParent( this );
	return pClone;
}

	
///////////////////////////////////////////////////////////////////////////////
// Section : MaterialTextureProxy
///////////////////////////////////////////////////////////////////////////////

bool MaterialTextureProxy::apply( ID3DXEffect* pEffect, D3DXHANDLE hProperty )
{
	BW_GUARD;

	if (!value_.hasObject())
		return SUCCEEDED( pEffect->SetTexture( hProperty, NULL ) );
	return SUCCEEDED( pEffect->SetTexture( hProperty, value_->pTexture() ) );
}


bool MaterialTextureProxy::be( const Moo::BaseTexturePtr pTex )
{
	BW_GUARD;

	value_ = pTex;
	resourceID_ = value_->resourceID();
	return true;
}


bool MaterialTextureProxy::be( const BW::StringRef & s )
{
	BW_GUARD;

	value_ = Moo::TextureManager::instance()->get(s);
	resourceID_ = value_.hasObject() ? value_->resourceID() : "";
	return true;
}


bool MaterialTextureProxy::getResourceID( BW::string & s ) const
{
	BW_GUARD;

	s = value_ ? value_->resourceID() : resourceID_;
	return true;
};


void MaterialTextureProxy::set( BW::string value, bool transient,
													bool addBarrier )
{
	BW_GUARD;

	resourceID_ = BWResolver::dissolveFilename( value );
	value_ = Moo::TextureManager::instance()->get(resourceID_, true, true, true, "texture/material");
}


void MaterialTextureProxy::save( DataSectionPtr pSection )
{
	BW_GUARD;

	if (!resourceID_.empty())
	{
		pSection->writeString( "Texture", resourceID_ );
	}
	else if (value_.hasObject())
	{
		pSection->writeString("Texture", value_->resourceID() );
	}
	else
	{
		pSection->writeString( "Texture", "" );
	}
}


Moo::EffectProperty* MaterialTextureProxy::clone() const
{
	MaterialTextureProxy* pClone = new MaterialTextureProxy( this->propName() );
	pClone->resourceID_ = resourceID_;
	pClone->value_ = value_;
	pClone->setParent( this );
	return pClone;
}

	
///////////////////////////////////////////////////////////////////////////////
// Section : MaterialTextureFeedProxy
///////////////////////////////////////////////////////////////////////////////


bool MaterialTextureFeedProxy::apply( ID3DXEffect* pEffect, D3DXHANDLE hProperty )
{
	BW_GUARD;

	if (!value_.hasObject())
		return SUCCEEDED( pEffect->SetTexture( hProperty, NULL ) );
	return SUCCEEDED( pEffect->SetTexture( hProperty, value_->pTexture() ) );
}

void MaterialTextureFeedProxy::set( BW::string value, bool transient,
															bool addBarrier )
{
	BW_GUARD;

	resourceID_ = value;
	if( resourceID_.size() )
		value_ = Moo::TextureManager::instance()->get( resourceID_, true, true, true, "texture/material" );
	else
		value_ = Moo::TextureManager::instance()->get( s_notFoundBmp, true, true, true, "texture/material" );
}

void MaterialTextureFeedProxy::save( DataSectionPtr pSection )
{
	BW_GUARD;

	pSection = pSection->newSection( "TextureFeed" );
	pSection->setString( textureFeed_ );
	if( ! resourceID_.empty() )
		pSection->writeString( "default", resourceID_ );
}

void MaterialTextureFeedProxy::setTextureFeed( BW::string value )
{
	textureFeed_ = value;
}


Moo::EffectProperty* MaterialTextureFeedProxy::clone() const
{
	MaterialTextureFeedProxy* pClone = new MaterialTextureFeedProxy( this->propName() );
	pClone->resourceID_ = resourceID_;
	pClone->textureFeed_ = textureFeed_;
	pClone->value_ = value_;
	pClone->setParent( this );
	return pClone;
}
BW_END_NAMESPACE

