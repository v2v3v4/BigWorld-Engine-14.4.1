#ifndef MATERIAL_PROXIES_HPP
#define MATERIAL_PROXIES_HPP


#include "moo/managed_effect.hpp"
#include "gizmo/general_properties.hpp"

BW_BEGIN_NAMESPACE

class MaterialPropertiesUser;



/**
 *	The editor effect property simply adds a save interface to 
 *	the base class.
 */
class EditorEffectProperty : public Moo::EffectProperty
{
public:
	EditorEffectProperty(
		const BW::StringRef & name,
		ID3DXEffect * pEffect,
		D3DXHANDLE hProperty );
	virtual void save( DataSectionPtr pSection ) = 0;

	bool boolAnnotation( const BW::string& annotation, bool& retVal ) const;
	bool intAnnotation( const BW::string& annotation, int32& retVal ) const;
	bool stringAnnotation( const BW::string& annotation, BW::string& retVal ) const;
	bool floatAnnotation( const BW::string& annotation, float& retVal ) const;
	const BW::string& name() const;
	void setParent( const Moo::EffectProperty* pParent );
protected:

	typedef BW::map<BW::string, bool> BoolAnnotations;
	typedef BW::map<BW::string, int32> IntAnnotations;
	typedef BW::map<BW::string, BW::string> StringAnnotations;
	typedef BW::map<BW::string, float> FloatAnnotations;

	class AnnotationData : public ReferenceCount
	{
	public:
		BoolAnnotations boolAnnotations_;
		IntAnnotations intAnnotations_;
		StringAnnotations stringAnnotations_;
		FloatAnnotations floatAnnotations_;
		BW::string name_;

		static BW::string s_emptyString_;
	};

	SmartPointer<AnnotationData> pAnnotations_;
};


/**
 *	Base class for all material proxies
 */
class BaseMaterialProxy : public ReferenceCount
{
public:
	BaseMaterialProxy() {}
	virtual ~BaseMaterialProxy() {}
};


/**
 *	Wrapper class for all material proxies
 */
template <class CL, class DT> class MaterialProxy : public BaseMaterialProxy
{
public:
	MaterialProxy( SmartPointer<CL> proxy, MaterialPropertiesUser * callback ) :
		callback_( callback )
	{
		BW_GUARD;

		proxies_.push_back( proxy );
		if (callback_)
		{
			callback_->proxySetCallback();
		}
	}

	void addProperty( SmartPointer<CL> proxy )
	{
		BW_GUARD;

		proxies_.push_back( proxy );
	}

	DT get() const
	{
		BW_GUARD;

		return proxies_[0]->get();
	}

	void set( DT val )
	{
		BW_GUARD;

		for ( BW::vector< SmartPointer<CL> >::iterator it = proxies_.begin(); it != proxies_.end(); it++ )
			if (*it)
				(*it)->set( val, true );
		if (callback_)
		{
			callback_->proxySetCallback();
		}
	}

	bool getRange( int& min, int& max )
	{
		BW_GUARD;

		return proxies_[0]->getRange( min, max );
	}

	bool getRange( float& min, float& max, int& digits )
	{
		BW_GUARD;

		return proxies_[0]->getRange( min, max, digits );
	}

private:
	MaterialPropertiesUser * callback_;
	BW::vector< SmartPointer<CL> > proxies_;
};


/**
 *	This class is used to avoid multiple-inheritance ambiguity of incRef/decRef
 *	in material proxies, since they inherit from EditorEffectProperty and from
 *	a proxy class, both classes ref-counted.
 */
template<typename Outer, typename Parent> class ProxyHolder
{
public:
	class InnerProxy : public Parent
	{
		Outer& outer_;
	public:
		InnerProxy( Outer& outer ) : outer_( outer )
		{
		}

		void set( typename Parent::Data value, bool transient, bool addBarrier = true )
		{
			BW_GUARD;

			outer_.set( value, transient, addBarrier );
		}

		typename Parent::Data get() const
		{
			BW_GUARD;

			return outer_.get();
		}

		void getMatrix( Matrix & m, bool world = true )
		{
			BW_GUARD;

			outer_.getMatrix( m, world );
		}

		void getMatrixContext( Matrix & m )
		{
			BW_GUARD;

			outer_.getMatrix( m );
		}

		void getMatrixContextInverse( Matrix & m )
		{
			BW_GUARD;

			outer_.getMatrixContextInverse( m );
		}

		bool setMatrix( const Matrix & m )
		{
			BW_GUARD;

			return outer_.setMatrix( m );
		}

		void recordState()
		{
			BW_GUARD;

			outer_.recordState();
		}

		bool commitState( bool revertToRecord = false, bool addUndoBarrier = true )
		{
			BW_GUARD;

			return outer_.commitState( revertToRecord, addUndoBarrier );
		}

		bool hasChanged()
		{
			BW_GUARD;

			return outer_.hasChanged();
		}

        bool getRange( float& min, float& max, int& digits ) const
        {
			BW_GUARD;

            return outer_.getRange( min, max, digits );
        }

        bool getRange( typename Parent::Data& min, typename Parent::Data& max ) const
        {
			BW_GUARD;

            return outer_.getRange( min, max );
        }

		typedef SmartPointer<InnerProxy> SP;
	};

	typename InnerProxy::SP proxy()
	{
		BW_GUARD;

		if( !ptr_ )
			ptr_ = new InnerProxy( *static_cast< Outer* >( this ) );
		return ptr_;
	}

private:
	typename InnerProxy::SP ptr_;
};


/**
 *	Material Bool Proxy
 */
class MaterialBoolProxy : public EditorEffectProperty, public ProxyHolder< MaterialBoolProxy,BoolProxy >
{
public:
	MaterialBoolProxy(
		const BW::StringRef & name,
		ID3DXEffect * pEffect = NULL,
		D3DXHANDLE hProperty = NULL ) :
		EditorEffectProperty( name, pEffect, hProperty ),
		value_()
	{
	}

	bool apply( ID3DXEffect* pEffect, D3DXHANDLE hProperty );

	void set( bool value, bool transient, bool addBarrier = true );
	bool get() const;

	bool be( const Vector4 & v );
	void asVector4( Vector4 & v ) const;
	bool be( const bool & b );
	bool getBool( bool & b ) const;

	void save( DataSectionPtr pSection );

	Moo::EffectProperty* clone() const;
protected:
	bool value_;
};


/**
 *	Material Int Proxy
 */
class MaterialIntProxy : public EditorEffectProperty, public ProxyHolder< MaterialIntProxy,IntProxy >
{
public:
	MaterialIntProxy(
		const BW::StringRef & name,
		ID3DXEffect * pEffect = NULL,
		D3DXHANDLE hProperty = NULL );
	bool apply( ID3DXEffect* pEffect, D3DXHANDLE hProperty );
	void set( int32 value, bool transient,
											bool addBarrier = true  );
	int32 get() const { return value_; }
	bool be( const Vector4 & v ) { value_ = int(v.x); return true; }
	void asVector4( Vector4 & v ) const { v.x = float( value_ ); }
	bool be( const int & i ) { value_ = i; return true; }
	bool getInt( int & i ) const { i = value_; return true; }
	void save( DataSectionPtr pSection );
    bool getRange( int32& min, int32& max ) const;
    void setRange( int32 min, int32 max );
	virtual void attach( D3DXHANDLE hProperty, ID3DXEffect* pEffect );
	void setParent( const Moo::EffectProperty* pParent );
	Moo::EffectProperty* clone() const;
protected:
	int32 value_;
    bool ranged_;
    int32 min_;
    int32 max_;
};


/**
 *	Material Float Proxy
 */
class MaterialFloatProxy : public EditorEffectProperty, public ProxyHolder< MaterialFloatProxy,FloatProxy >
{
public:
	MaterialFloatProxy(
		const BW::StringRef & name,
		ID3DXEffect * pEffect = NULL,
		D3DXHANDLE hProperty = NULL ) :
		EditorEffectProperty( name, pEffect, hProperty ),
		value_( 0.f ),
		ranged_( false ),
		min_(),
		max_(),
		digits_()
	{
	}

	bool apply( ID3DXEffect* pEffect, D3DXHANDLE hProperty )
	{
		BW_GUARD;

		return SUCCEEDED( pEffect->SetFloat( hProperty, value_ ) );
	}

	float get() const { return value_; }

	void set( float f, bool transient, bool addBarrier = true ) { value_ = f; }

	bool be( const Vector4 & v ) { value_ = v.x; return true; }
	void asVector4( Vector4 & v ) const { v.x = value_; }
	bool be( const float & f ) { value_ = f; return true; }
	bool getFloat( float & f ) const { f = value_; return true; }

	void save( DataSectionPtr pSection );

    bool getRange( float& min, float& max, int& digits ) const;
    void setRange( float min, float max, int digits );

	virtual void attach( D3DXHANDLE hProperty, ID3DXEffect* pEffect );

	void setParent( const Moo::EffectProperty* pParent );
	Moo::EffectProperty* clone() const;
protected:
	float value_;
    bool ranged_;
    float min_;
    float max_;
    int digits_;
};


/**
 *	Material Vector4 Proxy
 */
class MaterialVector4Proxy : public EditorEffectProperty, public ProxyHolder< MaterialVector4Proxy,Vector4Proxy >
{
public:
	MaterialVector4Proxy(
		const BW::StringRef & name,
		ID3DXEffect * pEffect = NULL,
		D3DXHANDLE hProperty = NULL ) : 
		EditorEffectProperty( name, pEffect, hProperty ),
		value_(0.f,0.f,0.f,0.f) 
	{}

	bool apply( ID3DXEffect* pEffect, D3DXHANDLE hProperty );

	bool be( const Vector4 & v );
	bool getVector( Vector4 & v ) const;
	
	Vector4 get() const;
	void set( Vector4 f, bool transient, bool addBarrier = true );

	void save( DataSectionPtr pSection );

	Moo::EffectProperty* clone() const;
protected:
	Vector4 value_;
};


/**
 *	Material Matrix Proxy
 */
class MaterialMatrixProxy : public EditorEffectProperty, public ProxyHolder< MaterialMatrixProxy,MatrixProxy >
{
public:
	MaterialMatrixProxy(
		const BW::StringRef & name,
		ID3DXEffect * pEffect = NULL,
		D3DXHANDLE hProperty = NULL ) :
		EditorEffectProperty( name, pEffect, hProperty )
	{}

	bool apply( ID3DXEffect* pEffect, D3DXHANDLE hProperty );
	bool setMatrix( const Matrix & m );
	void getMatrix( Matrix& m, bool world = true ) { m = value_; }
    void getMatrixContext( Matrix& m )   {};
    void getMatrixContextInverse( Matrix& m )   {};
	void save( DataSectionPtr pSection );
    void recordState()   {};
	bool commitState( bool revertToRecord = false, bool addUndoBarrier = true );
    bool hasChanged()    { return true; }
	bool be( const Vector4 & v );
	void asVector4( Vector4 & v ) const;
	bool be( const Matrix & m );
	bool getMatrix( Matrix & m ) const;

	Moo::EffectProperty* clone() const;
protected:
	Matrix value_;
};


/**
 *	Material Colour Proxy
 */
class MaterialColourProxy : public EditorEffectProperty, public ProxyHolder< MaterialColourProxy,ColourProxy >
{
public:
	MaterialColourProxy(
		const BW::StringRef & name,
		ID3DXEffect* pEffect = NULL,
		D3DXHANDLE hProperty = NULL ) :
		EditorEffectProperty( name, pEffect, hProperty ), 
		value_(0.f,0.f,0.f,0.f)
	{}

	bool apply( ID3DXEffect* pEffect, D3DXHANDLE hProperty );
	
	Moo::Colour get() const;
	void set( Moo::Colour f, bool transient, bool addBarrier = true );
    
    Vector4 getVector4() const;
    void setVector4( Vector4 f, bool transient );

	void save( DataSectionPtr pSection );

	Moo::EffectProperty* clone() const;
protected:
	Vector4 value_;
};


/**
 *	Material Texture Proxy
 */
class MaterialTextureProxy : public EditorEffectProperty, public ProxyHolder< MaterialTextureProxy,StringProxy >
{
public:
	MaterialTextureProxy(
		const BW::StringRef & name,
		ID3DXEffect * pEffect = NULL,
		D3DXHANDLE hProperty = NULL ) :
		EditorEffectProperty( name, pEffect, hProperty )
	{
	}
	bool apply( ID3DXEffect* pEffect, D3DXHANDLE hProperty );
	void set( BW::string value, bool transient,
											bool addBarrier = true  );
	BW::string get() const { return resourceID_; };
	bool be( const Moo::BaseTexturePtr pTex );
	bool be( const BW::StringRef & s );
	void save( DataSectionPtr pSection );
	bool getResourceID( BW::string & s ) const;
	Moo::EffectProperty* clone() const;
protected:
	BW::string		resourceID_;
	Moo::BaseTexturePtr value_;
};


/**
 *	Material Texture Feed Proxy
 */
class MaterialTextureFeedProxy : public EditorEffectProperty, public ProxyHolder< MaterialTextureFeedProxy, StringProxy >
{
public:
	MaterialTextureFeedProxy(
		const BW::StringRef & name,
		ID3DXEffect * pEffect = NULL,
		D3DXHANDLE hProperty = NULL ) :
		EditorEffectProperty( name, pEffect, hProperty )
	{
	}
	bool apply( ID3DXEffect* pEffect, D3DXHANDLE hProperty );
	void set( BW::string value, bool transient,
												bool addBarrier = true );
	BW::string get() const { return resourceID_; };
	void save( DataSectionPtr pSection );
	void setTextureFeed( BW::string value );

	bool be( const Moo::BaseTexturePtr pTex ) { value_ = pTex; return true; }
	bool be( const BW::StringRef & s )		 { value_ = Moo::TextureManager::instance()->get(s); return true; }
	bool getResourceID( BW::string & s ) const { s = value_ ? value_->resourceID() : ""; return true; }
	Moo::EffectProperty* clone() const;
protected:
	BW::string		resourceID_;
	BW::string		textureFeed_;
	Moo::BaseTexturePtr value_;
};


BW_END_NAMESPACE
#endif // MATERIAL_PROXIES_HPP
