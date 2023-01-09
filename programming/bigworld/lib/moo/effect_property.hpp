#ifndef EFFECT_PROPERTY_HPP
#define EFFECT_PROPERTY_HPP

#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_map.hpp"

#include "moo_dx.hpp"
#include "cstdmf/smartpointer.hpp"
#include "forward_declarations.hpp"


BW_BEGIN_NAMESPACE

namespace Moo {

/**
 * This class is the base class for the effect properties.
 */
class EffectProperty : public SafeReferenceCount
{
public:
	EffectProperty( const BW::StringRef & name ) : 
		propName_( name.data(), name.size() ) {}
	virtual EffectProperty* clone() const = 0;
	virtual bool apply( ID3DXEffect* pEffect, D3DXHANDLE hProperty ) = 0;
	virtual bool be( const bool & b ) { return false; }
	virtual bool be( const float & f ) { return false; }
	virtual bool be( const int & i ) { return false; }
	virtual bool be( const Matrix & m ) { return false; }
	virtual bool be( const Vector4 & v ) { return false; }
	virtual bool be( const BaseTexturePtr pTex ) { return false; }
	virtual bool be( const BW::StringRef & s ) { return false; }
	virtual bool getBool( bool & b ) const { return false; };
	virtual bool getInt( int & i ) const { return false; };
	virtual bool getFloat( float & f ) const { return false; };
	virtual bool getVector( Vector4 & v ) const { return false; };
	virtual bool getMatrix( Matrix & m ) const { return false; };
	virtual bool getTexture( BaseTexturePtr & pTex ) { return false; }
	virtual bool getResourceID( BW::string & s ) const { return false; };
	virtual void asVector4( Vector4 & v ) const { getVector( v ); } // never fails
	virtual void setParent( const EffectProperty* pParent ) {};
	virtual void save( DataSectionPtr pDS ) = 0;

	const BW::string& propName() const { return propName_; }
	void propName( const BW::StringRef & propName )
	{ 
		propName_.assign( propName.data(), propName.size()); 
	}
private:
	BW::string propName_;
};

typedef SmartPointer<EffectProperty> EffectPropertyPtr;
typedef ConstSmartPointer<EffectProperty> ConstEffectPropertyPtr;
typedef StringRefMap< EffectPropertyPtr > EffectPropertyMappings;


/**
 *	An interface for objects that require setup of properties
 *	including creation and validity checking. Generally used for
 *	GUI property fields.
 */
class EffectPropertyFactory;
typedef SmartPointer<EffectPropertyFactory> EffectPropertyFactoryPtr;

class EffectPropertyFactory : public ReferenceCount
{
public:
	virtual EffectPropertyPtr create(
		const BW::StringRef & name,
		DataSectionPtr pSection ) = 0;
	virtual EffectPropertyPtr create(
		const BW::StringRef & name,
		D3DXHANDLE hProperty,
		ID3DXEffect * pEffect ) = 0;
	virtual bool check( D3DXPARAMETER_DESC* propertyDesc ) = 0;

public:
	static void setFactory( const StringRef & typeName, 
		const EffectPropertyFactoryPtr & functor );

	static const EffectPropertyFactoryPtr& findFactory( 
		const StringRef & typeName );

	static const EffectPropertyFactoryPtr& findFactory(
		D3DXPARAMETER_DESC* paramDesc );

	static void fini();
};

} // namespace Moo

BW_END_NAMESPACE

#endif // EFFECT_PROPERTY_HPP
