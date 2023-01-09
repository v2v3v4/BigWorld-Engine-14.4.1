#include "pch.hpp"

#include "effect_property.hpp"
#include "base_texture.hpp"
#include "texture_manager.hpp"

BW_BEGIN_NAMESPACE

namespace Moo {

/**
 *	Vector4 effect property wrapper.
 */
class Vector4Property : public EffectProperty
{
public:
	Vector4Property( const BW::StringRef & name ) : EffectProperty( name ) {}
	bool apply( ID3DXEffect* pEffect, D3DXHANDLE hProperty )
	{
		return SUCCEEDED( pEffect->SetVector( hProperty, &value_ ) );
	}
	bool be( const Vector4 & v ) { value_ = v; return true; }

	bool getVector( Vector4 & v ) const { v = value_; return true; };

	void value( const Vector4& value ) { value_ = value; }
	const Vector4& value() const { return value_; }
	virtual void save( DataSectionPtr pDS )
	{
		pDS->writeVector4( "Vector4", value_ );
	}
	EffectProperty* clone() const
	{
		Vector4Property* pClone = new Vector4Property( this->propName() );
		pClone->value_ = value_;
		return pClone;
	}
protected:
	Vector4 value_;
};

/**
 *	Vector4 property functor.
 */
class Vector4PropertyFactory : public EffectPropertyFactory
{
public:
	virtual EffectPropertyPtr create(
		const BW::StringRef & name,
		DataSectionPtr pSection )
	{
		Vector4Property* prop = new Vector4Property( name );
		prop->value( pSection->asVector4() );
		return prop;
	}
	virtual EffectPropertyPtr create(
		const BW::StringRef & name,
		D3DXHANDLE hProperty,
		ID3DXEffect * pEffect )
	{
		Vector4Property* prop = new Vector4Property( name );
		Vector4 v;
        pEffect->GetVector( hProperty, &v );
		prop->value( v );
		return prop;
	}
	virtual bool check( D3DXPARAMETER_DESC* propertyDesc )
	{
		return (propertyDesc->Class == D3DXPC_VECTOR &&
			propertyDesc->Type == D3DXPT_FLOAT);
	}	
private:
};


/**
 *	Matrix effect property wrapper.
 */
class MatrixProperty : public EffectProperty
{
public:
	MatrixProperty( const BW::StringRef & name ) : EffectProperty( name ) {}
	bool apply( ID3DXEffect* pEffect, D3DXHANDLE hProperty )
	{
		return SUCCEEDED( pEffect->SetMatrix( hProperty, &value_ ) );
	}
	bool be( const Vector4 & v )
		{ value_.translation( Vector3( v.x, v.y, v.z ) ); /*hmmm*/ return true; }
	bool be( const Matrix & m )
		{ value_ = m; return true; }

	bool getMatrix( Matrix & m ) const { m = value_; return true; };

	void value( Matrix& value ) { value_ = value; };
	const Matrix& value() const { return value_; }
	virtual void save( DataSectionPtr pDS )
	{
		pDS->writeString( "Matrix", "NOT YET IMPLEMENTED" );
	}
	EffectProperty* clone() const
	{
		MatrixProperty* pClone = new MatrixProperty( this->propName() );
		pClone->value_ = value_;
		return pClone;
	}
protected:
	Matrix value_;
};


/**
 *	Matrix property functor.
 */
class MatrixPropertyFactory : public EffectPropertyFactory
{
public:
	virtual EffectPropertyPtr create(
		const BW::StringRef & name,
		DataSectionPtr pSection )
	{
		MatrixProperty* prop = new MatrixProperty( name );
		Matrix m;
		m.row(0, pSection->readVector4( "row0" ) );
		m.row(1, pSection->readVector4( "row1" ) );
		m.row(2, pSection->readVector4( "row2" ) );
		m.row(3, pSection->readVector4( "row3" ) );
		prop->value( m );
		return prop;
	}
	virtual EffectPropertyPtr create(
		const BW::StringRef & name,
		D3DXHANDLE hProperty,
		ID3DXEffect* pEffect )
	{
		MatrixProperty* prop = new MatrixProperty( name );
		Matrix m;
        pEffect->GetMatrix( hProperty, &m );
		prop->value( m );
		return prop;
	}
	virtual bool check( D3DXPARAMETER_DESC* propertyDesc )
	{
		return ((propertyDesc->Class == D3DXPC_MATRIX_ROWS ||
            propertyDesc->Class == D3DXPC_MATRIX_COLUMNS) &&
			propertyDesc->Type == D3DXPT_FLOAT);
	}
private:
};

/**
 *	Texture effect property wrapper.
 */
class TextureProperty : public EffectProperty
{
public:
	TextureProperty( const BW::StringRef & name ) : EffectProperty( name ) {}
	bool apply( ID3DXEffect* pEffect, D3DXHANDLE hProperty )
	{
		if (!value_.hasObject())
			return SUCCEEDED( pEffect->SetTexture( hProperty, NULL ) );
		return SUCCEEDED( pEffect->SetTexture( hProperty, value_->pTexture() ) );
	}

	bool be( const BaseTexturePtr pTex ) { value_ = pTex; return true; }
	bool be( const BW::StringRef & s )	
	{
		value_ = TextureManager::instance()->get(s); return true;
	}

	void value( BaseTexturePtr value ) { value_ = value; };
	const BaseTexturePtr value() const { return value_; }

	bool getResourceID( BW::string & s ) const
	{
		s = value_ ? value_->resourceID() : ""; return true;
	};

	bool getTexture( BaseTexturePtr & pTex ) 
	{
		pTex = value_;
		return true; 
	}

	virtual void save( DataSectionPtr pDS )
	{
		BW::string s;
		if ( getResourceID(s) )
		{
			pDS->writeString( "Texture", s );
		}
	}
	EffectProperty* clone() const
	{
		TextureProperty* pClone = new TextureProperty( this->propName() );
		pClone->value_ = value_;
		return pClone;
	}
protected:
	BaseTexturePtr value_;
};

/**
 *	Texture property functor.
 */
class TexturePropertyFactory : public EffectPropertyFactory
{
public:
	virtual EffectPropertyPtr create(
		const BW::StringRef & name,
		DataSectionPtr pSection )
	{
		TextureProperty* prop = new TextureProperty( name );
		BaseTexturePtr pTexture =
			TextureManager::instance()->get( pSection->asString() );
		prop->value( pTexture );
		return prop;
	}
	virtual EffectPropertyPtr create(
		const BW::StringRef & name,
		D3DXHANDLE hProperty,
		ID3DXEffect * pEffect )
	{
		TextureProperty* prop = new TextureProperty( name );
		return prop;
	}
	virtual bool check( D3DXPARAMETER_DESC* propertyDesc )
	{
		return (propertyDesc->Class == D3DXPC_OBJECT &&
			(propertyDesc->Type == D3DXPT_TEXTURE ||
			propertyDesc->Type == D3DXPT_TEXTURE1D ||
			propertyDesc->Type == D3DXPT_TEXTURE2D ||
			propertyDesc->Type == D3DXPT_TEXTURE3D ||
			propertyDesc->Type == D3DXPT_TEXTURECUBE ));
	}
private:
};


/**
 *	Float effect property wrapper.
 */
class FloatProperty : public EffectProperty
{
public:
	FloatProperty( const BW::StringRef & name ):
		EffectProperty( name ),
		value_()
	{}

	bool apply( ID3DXEffect* pEffect, D3DXHANDLE hProperty )
	{
		return SUCCEEDED( pEffect->SetFloat( hProperty, value_ ) );
	}
	bool be( const Vector4 & v ) { value_ = v.x; return true; }
	bool be( const float & f ) { value_ = f; return true; }

	bool getFloat( float & f ) const { f = value_; return true; };

	void value( float value ) { value_ = value; };
	float value() const { return value_; }
	virtual void save( DataSectionPtr pDS )
	{
		pDS->writeFloat( "Float", value_ );
	}
	EffectProperty* clone() const
	{
		FloatProperty* pClone = new FloatProperty( this->propName() );
		pClone->value_ = value_;
		return pClone;
	}
protected:
	float value_;
};


/**
 * Float property functor
 */
class FloatPropertyFactory : public EffectPropertyFactory
{
public:
	virtual EffectPropertyPtr create(
		const BW::StringRef & name,
		DataSectionPtr pSection )
	{
		FloatProperty* prop = new FloatProperty( name );
		prop->value( pSection->asFloat() );
		return prop;
	}
	virtual EffectPropertyPtr create(
		const BW::StringRef & name,
		D3DXHANDLE hProperty,
		ID3DXEffect * pEffect )
	{
		FloatProperty* prop = new FloatProperty( name );
		float v;
        pEffect->GetFloat( hProperty, &v );
		prop->value( v );
		return prop;
	}
	virtual bool check( D3DXPARAMETER_DESC* propertyDesc )
	{
		return (propertyDesc->Class == D3DXPC_SCALAR &&
			propertyDesc->Type == D3DXPT_FLOAT);
	}
private:
};

/**
 * Bool effect property wrapper.
 */
class BoolProperty : public EffectProperty
{
public:
	BoolProperty( const BW::StringRef & name ) :
		EffectProperty( name ),
		value_()
	{}

	bool apply( ID3DXEffect* pEffect, D3DXHANDLE hProperty )
	{
		return SUCCEEDED( pEffect->SetBool( hProperty, value_ ) );
	}
	bool be( const Vector4 & v ) { value_ = (v.x != 0); return true; }
	bool be( const bool & b ) { value_ = b; return true; }

	bool getBool( bool & b ) const { b = ( value_ != 0 ); return true; };

	void value( BOOL value ) { value_ = value; };
	BOOL value() const { return value_; }
	virtual void save( DataSectionPtr pDS )
	{
		pDS->writeBool( "Bool", value_ != 0 ? true : false );
	}
	EffectProperty* clone() const
	{
		BoolProperty* pClone = new BoolProperty( this->propName() );
		pClone->value_ = value_;
		return pClone;
	}
protected:
	BOOL value_;
};

/**
 *	Bool property functor.
 */
class BoolPropertyFactory : public EffectPropertyFactory
{
public:
	virtual EffectPropertyPtr create(
		const BW::StringRef & name,
		DataSectionPtr pSection )
	{
		BoolProperty* prop = new BoolProperty( name );
		prop->value( pSection->asBool() );
		return prop;
	}
	virtual EffectPropertyPtr create(
		const BW::StringRef & name,
		D3DXHANDLE hProperty,
		ID3DXEffect* pEffect )
	{
		BoolProperty* prop = new BoolProperty( name );
		BOOL v;
        pEffect->GetBool( hProperty, &v );
		prop->value( v );
		return prop;
	}
	virtual bool check( D3DXPARAMETER_DESC* propertyDesc )
	{
		return (propertyDesc->Class == D3DXPC_SCALAR &&
			propertyDesc->Type == D3DXPT_BOOL);
	}
private:
};


/**
 *	Int effect property wrapper.
 */
class IntProperty : public EffectProperty
{
public:
	IntProperty( const BW::StringRef & name ) :
		EffectProperty( name ),
		value_()
	{}

	bool apply( ID3DXEffect* pEffect, D3DXHANDLE hProperty )
	{
		return SUCCEEDED( pEffect->SetInt( hProperty, value_ ) );
	}
	bool be( const Vector4 & v ) { value_ = int(v.x); return true; }
	bool be( const int & i ) { value_ = i; return true; }

	bool getInt( int & i ) const { i = value_; return true; };

	void value( int value ) { value_ = value; };
	int value() const { return value_; }
	virtual void save( DataSectionPtr pDS )
	{
		pDS->writeInt( "Int", value_ );
	}
	EffectProperty* clone() const
	{
		IntProperty* pClone = new IntProperty( this->propName() );
		pClone->value_ = value_;
		return pClone;
	}
protected:
	int value_;
};


/**
 *	Int property functor.
 */
class IntPropertyFactory : public EffectPropertyFactory
{
public:
	virtual EffectPropertyPtr create(
		const BW::StringRef & name,
		DataSectionPtr pSection )
	{
		IntProperty* prop = new IntProperty( name );
		prop->value( pSection->asInt() );
		return prop;
	}
	virtual EffectPropertyPtr create(
		const BW::StringRef & name,
		D3DXHANDLE hProperty,
		ID3DXEffect * pEffect )
	{
		IntProperty* prop = new IntProperty( name );
		int v;
        pEffect->GetInt( hProperty, &v );
		prop->value( v );
		return prop;
	}
	virtual bool check( D3DXPARAMETER_DESC* propertyDesc )
	{
		return (propertyDesc->Class == D3DXPC_SCALAR &&
			propertyDesc->Type == D3DXPT_INT );
	}
private:
};

// Keys are static char* so don't need to be copied
typedef BW::map< StringRef, EffectPropertyFactoryPtr >
		EffectPropertyFactories;

static EffectPropertyFactories s_effectPropertyProcessors;

void EffectPropertyFactory::fini()
{
	s_effectPropertyProcessors.clear();
}

static EffectPropertyFactories& effectPropertyFactories()
{

#define EFFECT_PROPERTY_TYPE(x) \
	s_effectPropertyProcessors[#x] = new x##PropertyFactory;

	if (s_effectPropertyProcessors.empty())
	{
		EFFECT_PROPERTY_TYPE( Vector4 );
		EFFECT_PROPERTY_TYPE( Matrix );
		EFFECT_PROPERTY_TYPE( Float );
		EFFECT_PROPERTY_TYPE( Bool );
		EFFECT_PROPERTY_TYPE( Texture );
		EFFECT_PROPERTY_TYPE( Int );
	}

	return s_effectPropertyProcessors;
}

/**
 *	Adds a new type to the property functor factory.
 */
//static 
void EffectPropertyFactory::setFactory( const StringRef & typeName,
					   const EffectPropertyFactoryPtr& functor )
{
	effectPropertyFactories()[typeName] = functor;
}

/**
 *	Factory for effect property functors.
 */
//static 
const EffectPropertyFactoryPtr& EffectPropertyFactory::findFactory( 
	const StringRef & typeName )
{
	BW_GUARD;	
	EffectPropertyFactories::iterator it = 
		effectPropertyFactories().find( typeName );

	if (it != effectPropertyFactories().end())
	{
		return it->second;
	}
	else
	{
		static EffectPropertyFactoryPtr s_NULL;
		return s_NULL;
	}
}

/**
 *	Factory for effect property functors.
 */
//static 
const EffectPropertyFactoryPtr& EffectPropertyFactory::findFactory(
	D3DXPARAMETER_DESC* paramDesc )
{
	BW_GUARD;

	EffectPropertyFactories::iterator it = 
		effectPropertyFactories().begin();
	EffectPropertyFactories::iterator end = 
		effectPropertyFactories().end();

	while (it != end)
	{
		if (it->second->check( paramDesc ))
		{
			return it->second;
		}
		else
		{
			++it;
		}
	}

	static EffectPropertyFactoryPtr s_NULL;
	return s_NULL;
}

BW_END_NAMESPACE

} // namespace Moo
