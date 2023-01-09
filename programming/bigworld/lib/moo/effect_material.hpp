#ifndef EFFECT_MATERIAL_HPP
#define EFFECT_MATERIAL_HPP

#include "forward_declarations.hpp"
#include "moo_dx.hpp"
#include "managed_effect.hpp"
#include "effect_state_manager.hpp"
#include "physics2/worldtri.hpp"
#include "effect_manager.hpp"

BW_BEGIN_NAMESPACE

namespace Moo
{

class ModelTextureUsageGroup;

#define SAFE_SET( pTheEffect, type, name, value ) \
{\
	D3DXHANDLE h = pTheEffect->GetParameterByName( NULL, name );\
	if (h)\
	{\
	pTheEffect->Set##type( h, value );\
	}\
}

#define SAFE_SET_ARRAY( pTheEffect, type, name, value, count ) \
{\
	D3DXHANDLE h = pTheEffect->GetParameterByName( NULL, name );\
	if (h)\
	{\
	pTheEffect->Set##type##Array( h, value, count );\
	}\
}

#define SAFE_GET( handle, pTheEffect, type, name, result ) \
	(((handle = pTheEffect->GetParameterByName( NULL, name )) != NULL) \
		&& SUCCEEDED(pTheEffect->Get##type( handle, result )))

class EffectMaterial : public SafeReferenceCount
{
public:
	typedef BW::map< D3DXHANDLE, EffectPropertyPtr > Properties;

	EffectMaterial();
	explicit EffectMaterial( const EffectMaterial & other );
	EffectMaterial & operator=( const EffectMaterial & other );

	~EffectMaterial();

	bool load( DataSectionPtr pSection, bool addDefault = true, 
		bool suppressPropertyWarnings = false );
	void save( DataSectionPtr pSection );

	bool initFromEffect(
		const BW::StringRef & effect,
		const BW::StringRef & diffuseMap = "",
		int doubleSided = -1 );

	void identifier( const BW::StringRef & id );
	const BW::string& identifier() const;

	bool checkEffectRecompiled();

	bool begin() const;
	bool end() const;

	bool beginPass( uint32 pass ) const;
	bool endPass() const;
	uint32 numPasses() const;

	void setProperties() const;
	bool commitChanges() const;

	D3DXHANDLE hTechnique() const;
	bool hTechnique( D3DXHANDLE hTec );
	bool setTechnique( const BW::StringRef & techniqueName );
#ifdef D3DXFX_LARGEADDRESS_HANDLE
	bool hTechnique( LPCSTR hTecName );
#endif // D3DXFX_LARGEADDRESS_HANDLE

	const ManagedEffectPtr& pEffect() { return pManagedEffect_; }

	bool boolProperty( bool & result, const BW::StringRef & name ) const;
	bool intProperty( int & result, const BW::StringRef & name ) const;
	bool floatProperty( float & result, const BW::StringRef & name ) const;

	bool textureProperty(
		BW::string & result,
		const BW::StringRef & name ) const;

	void generateTextureUsage( Moo::ModelTextureUsageGroup & usageGroup, float texelDensity );

	bool vectorProperty( Vector4 & result, const BW::StringRef & name ) const;
	bool matrixProperty( Matrix & result, const BW::StringRef & name ) const;


	bool setProperty(
		const BW::StringRef & name,
		const bool value,
		bool create );

	bool setProperty(
		const BW::StringRef & name,
		const int value,
		bool create );

	bool setProperty(
		const BW::StringRef & name,
		const float value,
		bool create );

	bool setProperty(
		const BW::StringRef & name,
		const Moo::BaseTexturePtr & value,
		bool create );

	bool setProperty(
		const BW::StringRef & name,
		const Vector4 & value,
		bool create );

	bool setProperty(
		const BW::StringRef & name,
		const Matrix & value,
		bool create );

	EffectPropertyPtr getProperty( const BW::StringRef & name );
	ConstEffectPropertyPtr getProperty( const BW::StringRef & name ) const;
	bool replaceProperty( const BW::StringRef & name, 
		EffectPropertyPtr effectProperty );

	WorldTriangle::Flags getFlags( int objectMaterialKind ) const;

	const Properties& properties() const	{ return properties_; }
	Properties& properties()	{ return properties_; }

	bool skinned() const;
	bool bumpMapped() const;
	bool dualUV() const;

	ChannelType channelType() const;

	bool isDefault( EffectPropertyPtr pProp );
	void replaceDefaults();

#ifdef EDITOR_ENABLED
	int materialKind() const { return materialKind_; }
	void materialKind( int mk ) { materialKind_ = mk; }

	int collisionFlags() const { return collisionFlags_; }
	void collisionFlags( int cf ) { collisionFlags_ = cf; }

	bool bspModified_;
#endif

private:

	template<class T>
	bool setPropertyType(
		const BW::StringRef & name,
		const T & value,
		bool create,
		const BW::StringRef & typeName );

	bool loadInternal(
		DataSectionPtr pSection,
		EffectPropertyMappings & outProperties );

	void loadProperties(
		EffectPropertyMappings & effectProperties,
		bool addDefault ) const;

	void loadPropertyHandles(
		const EffectPropertyMappings & effectProperties,
		bool suppressPropertyWarnings );

	void reloadHandles();
	void reloadTechniqueHandles();
	void reloadPropertyHandles();

	bool handlesValid() const;

	void loadDepthParent( DataSectionPtr pMaterialSection );

	ManagedEffect::CompileMark compileMark_;

	ManagedEffectPtr	pManagedEffect_;
	Properties			properties_;

	D3DXHANDLE			hOverriddenTechnique_; // if 0, then no override

	BW::string			identifier_;

	// physics data
	int					materialKind_;
	int					collisionFlags_;
};

} // namespace Moo

#ifdef CODE_INLINE
#include "effect_material.ipp"
#endif

BW_END_NAMESPACE

#endif // EFFECT_MATERIAL_HPP
