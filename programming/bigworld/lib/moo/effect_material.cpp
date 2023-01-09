#include "pch.hpp"
#include "effect_material.hpp"
#include "managed_effect.hpp"
#include "resmgr/bwresource.hpp"
#include "effect_state_manager.hpp"
#include "texture_usage.hpp"

#include "resmgr/xml_section.hpp"
#include "cstdmf/profiler.hpp"
#include "cstdmf/string_builder.hpp"
#include "moo/texture_usage.hpp"

DECLARE_DEBUG_COMPONENT2( "Moo", 0 );


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "effect_material.ipp"
#endif

namespace Moo
{

// -----------------------------------------------------------------------------
// Section: EffectMaterial
// -----------------------------------------------------------------------------

namespace 
{
const size_t MAX_PARAM_NAME_SIZE = 1024;
} // anonymous namespace

/**
 *	Constructor.
 */
EffectMaterial::EffectMaterial() :
#ifdef EDITOR_ENABLED
	bspModified_( false ),
#endif
	compileMark_( 0 ),
	properties_(),
	hOverriddenTechnique_( 0 ),
	materialKind_( 0 ),
	collisionFlags_( 0 )
{
}

/**
 *	Copy constructor.
 */
EffectMaterial::EffectMaterial( const EffectMaterial & other )
{
	*this = other;
}

/**
 *	Destructor.
 */
EffectMaterial::~EffectMaterial()
{	
}

/**
 *	Checks to see if the underlying effect has been recompiled since we last
 *	used it. If it has, it will rebind property effect handles and return true.
 */
bool EffectMaterial::checkEffectRecompiled()
{
	BW_GUARD;
	if (pManagedEffect_ &&
		pManagedEffect_->compileMark() != compileMark_)
	{
		this->reloadHandles();
		compileMark_ = pManagedEffect_->compileMark();
		return true;
	}
	else
	{
		return false;
	}
}

/**
 *	This method sets up the initial constants and render states for rendering.
 *	@return true if successful.
 */
bool EffectMaterial::begin() const
{
	BW_GUARD;

	if (!pManagedEffect_)
	{
		return false;
	}

	const_cast<EffectMaterial*>(this)->checkEffectRecompiled();

	ManagedEffect::Handle hTechnique = this->hTechnique();
	if (!hTechnique)
	{
		return false;
	}

	bool explicitlySet = (hTechnique == hOverriddenTechnique_);

	pManagedEffect_->currentTechnique( hTechnique, explicitlySet );
	this->setProperties();
	return pManagedEffect_->begin( true );
}


void EffectMaterial::setProperties() const
{
	BW_GUARD;

	ID3DXEffect*				pEffect = pManagedEffect_->pEffect();
	Properties::const_iterator	it		= properties_.begin();
	Properties::const_iterator	end		= properties_.end();

	while (it != end)
	{
		it->second->apply( pEffect, it->first );
		it++;
	}
}


/**
 * This method commits state changes on the material's effect to Direct3D.
 */
bool EffectMaterial::commitChanges() const
{
	BW_GUARD_PROFILER( EffectMaterial_commit );
	return pManagedEffect_->commitChanges();
}

/**
 *	This method cleans up after rendering with this material.
 *	@return true if successful
 */
bool EffectMaterial::end() const
{
	BW_GUARD_PROFILER( EffectMaterial_end );
	return pManagedEffect_->end();
}

/**
 *	This method sets up rendering for a specific pass
 *
 *	@param pass the pass to render
 *	@return true if successful
 */
bool EffectMaterial::beginPass( uint32 pass ) const
{
	BW_GUARD_PROFILER( EffectMaterial_beginPass );
	return pManagedEffect_->beginPass( pass );
}

/**
 *	This method ends the current pass
 *
 *	@return true if successful
 */
bool EffectMaterial::endPass() const
{
	BW_GUARD_PROFILER( EffectMaterial_endPass );
	return pManagedEffect_->endPass();
}

/**
 *	This method loads an EffectMaterial from a DataSection 
 *	and sets up the tweakables.
 *
 *	@param pSection the DataSection to load the material from
 *	@param addDefault	Boolean indicating whether to add default mappings
 *                      from properties to shader parameters.
 *
 *	@return true if successful
 */
bool EffectMaterial::load(
	DataSectionPtr pSection, bool addDefault /*= true*/,
	bool suppressPropertyWarnings /*= false*/ )
{
	BW_GUARD;
	bool ret = false;
	IF_NOT_MF_ASSERT_DEV( pSection.hasObject() )
	{
		return false;
	}

	EffectPropertyMappings effectProperties;

	pManagedEffect_ = NULL;
	properties_.clear();

	// Do internal loading of material
	ret = this->loadInternal( pSection, effectProperties );

	if ( ret && pManagedEffect_ )
	{
		// Load properties
		loadProperties( effectProperties, addDefault );
		loadPropertyHandles( effectProperties, suppressPropertyWarnings );
	}

	return ret;
}

/**
 *	This method saves the given material's tweakable properties
 *	to the given datasection.
 *
 *	Material saving does not support recursion / inherited properties.
 *
 *	@param	m			The material to save
 *	@param	pSection	The data section to save to.
 *
 *	@return Success or Failure.
 */
void EffectMaterial::save( DataSectionPtr pSection )
{
	ComObjectWrap<ID3DXEffect> pEffect = this->pEffect()->pEffect();
	if ( !pEffect )
		return;

    pSection->deleteSections( "property" );
    pSection->deleteSections( "fx" );

    pSection->writeString( "fx", this->pEffect()->resourceID() );

	EffectMaterial::Properties& properties = this->properties();
    EffectMaterial::Properties::iterator it = properties.begin();
    EffectMaterial::Properties::iterator end = properties.end();

    while ( it != end )
    {
		D3DXHANDLE hParameter = it->first;
        EffectPropertyPtr& pProperty = it->second;

        D3DXPARAMETER_DESC desc;
        HRESULT hr = pEffect->GetParameterDesc( hParameter, &desc );
        if ( SUCCEEDED(hr) )
        {
            EffectProperty* pProp = pProperty.getObject();
            //If the following assertion is hit, then
            //runtimeInitMaterialProperties() was not called before this effect
            //material was created.
            BW::string name(desc.Name);
            DataSectionPtr pChild = pSection->newSection( "property" );
            pChild->setString( name );
            pProp->save( pChild );
        }
		it++;
	}

	//TODO : probably want to do save these too.
	//pSection->writeInt( "collisionFlags", this->collisionFlags() );
	//pSection->writeInt( "materialKind", this->materialKind() );
}


/*
 * This method is the internal load method from a data section.
 */
bool EffectMaterial::loadInternal(	DataSectionPtr pSection, 
									EffectPropertyMappings& outProperties )
{
	BW_GUARD;
	bool ret = true;

	IF_NOT_MF_ASSERT_DEV( pSection.hasObject() )
	{
		return false;
	}

	// get the identifier if we don't have one already.
	if (!identifier_.length())
	{
		identifier_ = pSection->readString( "identifier" );
	}

	// open the effect itself
	BW::vector< BW::string > fxNames;
	pSection->readStrings( "fx", fxNames );

	// complain if there are too many effects.
	if (fxNames.size() > 1)
	{
		WARNING_MSG( "EffectMaterial::loadInternal - "
					 "found multiple .fx files in %s\n", 
			pSection->sectionName().c_str() );
	}

	if (pManagedEffect_ == NULL && fxNames.size() > 0)
	{
		properties_.clear();

		// load the managed effect
		pManagedEffect_ = EffectManager::instance().get( fxNames[0] );

		if (!pManagedEffect_)
		{
			return false;
		}

		compileMark_ = pManagedEffect_->compileMark();
	}

	// load another material file if we are inheriting
	DataSectionPtr pMFMSect = pSection->openSection( "mfm" );
	if( pMFMSect )
	{
		DataSectionPtr pBaseSection = 
			BWResource::instance().openSection( pMFMSect->asString() );
		if (pBaseSection)
		{
			ret = loadInternal( pBaseSection, outProperties );
		}
	}

	// grab the mfm saved tweakable properties
	BW::vector<DataSectionPtr> dataSections;
	pSection->openSections( "property", dataSections );
	while (dataSections.size())
	{
		DataSectionPtr pSect = dataSections.back();
		dataSections.pop_back();
		DataSectionPtr pChild = pSect->openChild( 0 );
		if (pChild)
		{
			const EffectPropertyFactoryPtr& propertyFactory =
				EffectPropertyFactory::findFactory(
					pChild->sectionName().c_str() );

			if (propertyFactory)
			{
				outProperties[pSect->asString()] =
					propertyFactory->create( pSect->asString(), pChild );
			}
            else
            {
                DEBUG_MSG( "Could not find property processor "
							"for mfm-specified property %s\n", 
							pChild->sectionName().c_str() );
            }
		}
	}

	// grab the editor only properties
	materialKind_ = pSection->readInt( "materialKind", materialKind_ );
	collisionFlags_ = pSection->readInt( "collisionFlags", collisionFlags_ );

	return ret;
}

/**
 *	Create property list.
 *	@param effectProperties list of properties to create.
 *	@param addDefault 
 */
void EffectMaterial::loadProperties(
	EffectPropertyMappings& effectProperties,
	bool addDefault ) const
{
	BW_GUARD;
	const EffectPropertyMappings& defaultProperties =
		pManagedEffect_->defaultProperties();

	// Iterate over the default properties
	EffectPropertyMappings::const_iterator it;
	for ( it = defaultProperties.begin(); it != defaultProperties.end(); ++it )
	{
		// Find property in our property list
		EffectPropertyMappings::iterator propertyOverride =
			effectProperties.find( it->first );

		// Found it, override
		if ( propertyOverride != effectProperties.end() )
		{
			// If a default property exists
			// set it as the overridden properties parent
			propertyOverride->second->setParent( it->second.getObject() );
		}
		// Otherwise, add it to our list
		else if ( addDefault )
		{
			// Add the default property to this materials property list
			effectProperties.insert( *it );
		}
	}
}

/**
 *	Map the effect property names to the right d3dx handles.
 *	Takes a list of properties and adds the property and handle to
 *	the properties_<handle, property> map.
 *	@param effectProperties list of properties to map to handles.
 */
void EffectMaterial::loadPropertyHandles(
	const EffectPropertyMappings& effectProperties,
	bool suppressPropertyWarnings )
{
	BW_GUARD;

	// Check there is an effect to get handles from.
	if (!pManagedEffect_)
	{
		return;
	}

	// Calculate handles and insert in properties map
	ID3DXEffect* pEffect = pManagedEffect_->pEffect();
	D3DXHANDLE h;

	EffectPropertyMappings::const_iterator it;
	for ( it = effectProperties.begin(); it != effectProperties.end(); ++it )
	{
		char buffer[MAX_PARAM_NAME_SIZE];
		BW::StringBuilder builder( buffer, ARRAY_SIZE( buffer ) );
		builder.append( it->first );
		h = pEffect->GetParameterByName( NULL, builder.string() );

		// Got handle for property
		if ( h != NULL )
		{
			// Set handle in properties map
			properties_[ h ] = it->second;
		}

		// No handle for property
		else if (!suppressPropertyWarnings)
		{
			NOTICE_MSG( 
				"EffectMaterial::load - no parameter "
				"\"%s\" found in .fx file \"%s\"\n",
				builder.string(),
				pManagedEffect_->resourceID().c_str() );
		}
	}
}

void EffectMaterial::reloadHandles()
{
	BW_GUARD;
	this->reloadTechniqueHandles();
	this->reloadPropertyHandles();
}

void EffectMaterial::reloadTechniqueHandles()
{
	BW_GUARD;
	hOverriddenTechnique_ = NULL;
}

/**
 *	Map the effect property names to the right d3dx handles.
 *	Reloads the handles in the properties_<handle, property> map.
 */
void EffectMaterial::reloadPropertyHandles()
{
	BW_GUARD;

	// Check there is an effect to get handles fro,.
	if (!pManagedEffect_)
	{
		return;
	}

	// Copy properties
	Properties oldProperties;
	oldProperties.insert( properties_.begin(), properties_.end() );

	// Clear old handles
	properties_.clear();

	// Calculate new handles
	ID3DXEffect* pEffect = pManagedEffect_->pEffect();

	Properties::const_iterator it;
	for ( it = oldProperties.begin(); it != oldProperties.end(); ++it )
	{
		const BW::string& name = it->second->propName();
		D3DXHANDLE h = pEffect->GetParameterByName( NULL, name.c_str() );

		// Got handle for property
		if ( h != NULL )
		{
			// Insert handle in properties map
			properties_[ h ] = it->second;
		}

		// No handle for property
		else
		{
			NOTICE_MSG( 
				"EffectMaterial::load - no parameter "
					"\"%s\" found in .fx file \"%s\"\n",
				name.c_str(),
				pManagedEffect_->resourceID().c_str() );
		}
	}
}

/**
 *	This method returns the current technique handle. If a specific technique
 *	has been selected then that will take precedence over the default one.
 *	A graphics settings change can change the default technique, but a caller
 *	selected technique still overrides that.
 */
D3DXHANDLE EffectMaterial::hTechnique() const 
{ 
	BW_GUARD_PROFILER( EffectMaterial_hTechnique );

	D3DXHANDLE ret = 0;

	if (pManagedEffect_)
	{
		if (hOverriddenTechnique_)
		{
			ret = hOverriddenTechnique_;
		}
		else
		{
			ret = pManagedEffect_->currentTechnique();
		}
	}

	return ret; 
}

/**
 *	This method sets the current technique and selects the appropriate visual 
 *	channel for the material.
 *
 *	@param hTec handle to the technique to use.
 *	@return true if successful
 */
bool EffectMaterial::hTechnique( ManagedEffect::Handle hTec )
{
	BW_GUARD;
	hOverriddenTechnique_ = hTec;

	bool ret = pManagedEffect_->currentTechnique( hTec, true );

	return ret;
}

/**
 *	This method sets the current technique and selects the appropriate visual 
 *	channel for the material.
 *
 *	@param techniqueName name of the technique to use.
 *	@return true if successful
 */
bool EffectMaterial::setTechnique( const BW::StringRef & techniqueName )
{
	BW_GUARD;

	// Grab the techniques
	const Moo::ManagedEffect::TechniqueInfoCache& techniques =
		pManagedEffect_->techniques();

	// Find handle that matches given name
	D3DXHANDLE handle = 0;
	for (Moo::ManagedEffect::TechniqueInfoCache::const_iterator 
		itr = techniques.begin(), end = techniques.end(); itr != end; ++itr)
	{
		if (itr->name_ == techniqueName)
		{
			handle = itr->handle_;
		}
	}

	// Could not find handle
	if (!handle)
	{
		ERROR_MSG( "EffectMaterial::hTechnique: could not find valid " 
			"technique '%s'\n",
			techniqueName.to_string().c_str() );
		return false;
	}

	// Try and set handle
	return this->hTechnique( handle );
}

#ifdef D3DXFX_LARGEADDRESS_HANDLE
/**
 *	This method sets the current technique and selects the appropriate visual 
 *	channel for the material.
 *
 *	@param hTec name of the technique to use.
 *	@return true if successful
 */
bool EffectMaterial::hTechnique( LPCSTR hTecName )
{
	BW_GUARD;
	D3DXHANDLE hTec = pManagedEffect_->pEffect()->GetTechniqueByName( hTecName );
	if (hTec == NULL)
		return false;
	return hTechnique( hTec );
}
#endif // D3DXFX_LARGEADDRESS_HANDLE

bool EffectMaterial::initFromEffect( const BW::StringRef & effect, 
	const BW::StringRef & diffuseMap /* = "" */, 
	int doubleSided /* = -1 */ )
{
	BW_GUARD;
	DataSectionPtr pSection = new XMLSection( "material" );
	if (pSection)
	{
		pSection->writeString( "fx", effect );

		if (!diffuseMap.empty())
		{
			DataSectionPtr pSect = pSection->newSection( "property" );
			pSect->setString( "diffuseMap" );
			pSect->writeString( "Texture", diffuseMap );
		}

		if (doubleSided != -1)
		{
			DataSectionPtr pSect = pSection->newSection( "property" );
			pSect->setString( "doubleSided" );
			pSect->writeBool( "Bool", !!doubleSided );			
		}

		// Label this as an "initFromEffect" effect
		// As we are not loading from a material file,
		// there is no "identifier" section to read
		this->identifier_ = "initFromEffect " + effect;

		return this->load( pSection );
	}

	return false;
}

/**
 *	Assignment operator.
 */
EffectMaterial & EffectMaterial::operator=( const EffectMaterial & other )
{
	BW_GUARD;

#ifdef EDITOR_ENABLED
	bspModified_			= other.bspModified_;
#endif
	compileMark_			= other.compileMark_;
	pManagedEffect_         = other.pManagedEffect_;
	properties_             = other.properties_;
	hOverriddenTechnique_	= other.hOverriddenTechnique_;
	identifier_             = other.identifier_;
	materialKind_			= other.materialKind_;
	collisionFlags_			= other.collisionFlags_;

	return *this;
}


/**
 *	Retrieves value of the first property with the specified name.
 *	This is not the value in the DirectX effect but the  Moo::EffectProperty
 *	If the property is a integer the value is copied into result.
 *
 *	@param result The int into which the result will be placed
 *	@param name The name of the property to read.
 *	@return Returns true if the property's value was successfully retrieved.
 */
bool EffectMaterial::boolProperty( bool & result, const BW::StringRef & name ) const
{
	BW_GUARD;
	ConstEffectPropertyPtr effectProperty = this->getProperty( name );
	if( effectProperty )
	{
		return effectProperty->getBool( result );
	}
	return false;
}

/**
 *	Retrieves value of the first property with the specified name.
 *	This is not the value in the DirectX effect but the  Moo::EffectProperty
 *	If the property is a integer the value is copied into result.
 *
 *	@param result The int into which the result will be placed
 *	@param name The name of the property to read.
 *	@return Returns true if the property's value was successfully retrieved.
 */
bool EffectMaterial::intProperty(
	int & result,
	const BW::StringRef & name ) const
{
	BW_GUARD;
	ConstEffectPropertyPtr effectProperty = this->getProperty( name );
	if( effectProperty )
	{
		return effectProperty->getInt( result );
	}
	return false;
}

/**
 *	Retrieves value of the first property with the specified name.
 *	This is not the value in the DirectX effect but the  Moo::EffectProperty
 *	If the property is a float the value is copied into result.
 *
 *	@param result The float into which the result will be placed
 *	@param name The name of the property to read.
 *	@return Returns true if the property's value was successfully retrieved.
 */
bool EffectMaterial::floatProperty( float & result, const BW::StringRef & name )
	const
{
	BW_GUARD;
	ConstEffectPropertyPtr effectProperty = this->getProperty( name );
	if( effectProperty )
	{
		return effectProperty->getFloat( result );
	}
	return false;
}

/**
 *	Retrieves value of the first property with the specified name.
 *	This is not the value in the DirectX effect but the  Moo::EffectProperty
 *	If the property is a texture the name of the texture is copied into result.
 *
 *	@param result The resource ID into which the result will be placed
 *	@param name The name of the property to read.
 *	@return Returns true if the property's value was successfully retrieved.
 */
bool EffectMaterial::textureProperty(
	BW::string & result,
	const BW::StringRef & name ) const
{
	BW_GUARD;
	ConstEffectPropertyPtr effectProperty = this->getProperty( name );
	if( effectProperty )
	{
		return effectProperty->getResourceID( result );
	}
	return false;
}

/**
 *	Retrieves value of the first property with the specified name.
 *	This is not the value in the DirectX effect but the  Moo::EffectProperty
 *	If the property is a vector the value is copied into result.
 *
 *	@param result The vector in which the result will be placed
 *	@param name The name of the property to read.
 *	@return Returns true if the property's value was successfully retrieved.
 */
bool EffectMaterial::vectorProperty(
	Vector4 & result,
	const BW::StringRef & name ) const
{
	BW_GUARD;
	ConstEffectPropertyPtr effectProperty = this->getProperty( name );
	if( effectProperty )
	{
		return effectProperty->getVector( result );
	}
	return false;
}

/**
 *	Retrieves value of the first property with the specified name.
 *	This is not the value in the DirectX effect but the  Moo::EffectProperty
 *	If the property is a matrix the value is copied into result.
 *
 *	@param result The matrix in which the result will be placed
 *	@param name The name of the property to read.
 *	@return Returns true if the property's value was successfully retrieved.
 */
bool EffectMaterial::matrixProperty(
	Matrix & result,
	const BW::StringRef & name ) const
{
	BW_GUARD;
	ConstEffectPropertyPtr effectProperty = this->getProperty( name );
	if( effectProperty )
	{
		return effectProperty->getMatrix( result );
	}
	return false;
}

/**
 *	Retrieves the first Moo::EffectProperty with the specified name.
 *
 *	@param name The name of the property to read.
 *	@return SmartPointer to the property. NULL if not found.
 */
EffectPropertyPtr EffectMaterial::getProperty( const BW::StringRef & name )
{	
	BW_GUARD;
	if ( pManagedEffect_ )
	{
		ID3DXEffect* dxEffect = pManagedEffect_->pEffect();
		if( !dxEffect )
			return NULL;

		for( Properties::iterator iProperty = properties_.begin();
			iProperty != properties_.end(); ++iProperty )
		{
			if ( name == iProperty->second->propName() )
			{
				return iProperty->second;
			}
		}
	}

	return NULL;
}

/**
 *	Retrieves the first Moo::EffectProperty with the specified name.
 *
 *	@param name The name of the property to read.
 *	@return ConstSmartPointer to the property. NULL if not found.
 */
ConstEffectPropertyPtr EffectMaterial::getProperty( const BW::StringRef & name )
	const
{
	BW_GUARD;
	return const_cast<EffectMaterial*>( this )->getProperty( name );
}

/** 
 *  This function replaces an existing effect property with the one given. 
 *
 *  @param name             The name of the property to replace. 
 *  @param effectProperty   SmartPointer to the property to replace the existing. 
 *  @return                 Returns true if the replace succeeded. 
 */ 
bool EffectMaterial::replaceProperty( const BW::StringRef & name, 
	EffectPropertyPtr effectProperty ) 
{
	BW_GUARD;
	if ( pManagedEffect_ )
	{
		ID3DXEffect* dxEffect = pManagedEffect_->pEffect();
		if( !dxEffect )
			return NULL;

		for( Properties::iterator iProperty = properties_.begin();
			iProperty != properties_.end(); ++iProperty )
		{
			D3DXPARAMETER_DESC description;

			if( !SUCCEEDED( dxEffect->GetParameterDesc( iProperty->first, 
				&description ) ) )
			{
				continue;
			}

			if( name == description.Name )
			{
                iProperty->second = effectProperty; 
                return true; 
			}
		}
	}

    return false;
} 

/** 
 * 	This method returns the physics flags for this material.
 */
WorldTriangle::Flags EffectMaterial::getFlags( int objectMaterialKind ) const
{
	return ( materialKind_ != 0 ) ?
		WorldTriangle::packFlags( collisionFlags_, materialKind_ ) :
		WorldTriangle::packFlags( collisionFlags_, objectMaterialKind );
}

/**
 * This method returns the skinned status of the current technique of the effect.
 */
bool EffectMaterial::skinned() const 
{ 
	BW_GUARD;
	return pManagedEffect_ ? 
		pManagedEffect_->skinned( hOverriddenTechnique_ ) : false; 
}

/**
 * This method returns the bumped status of the current technique of the effect.
 */
bool EffectMaterial::bumpMapped() const 
{ 
	BW_GUARD;
	return pManagedEffect_ ? 
		pManagedEffect_->bumpMapped( hOverriddenTechnique_ ) : false; 
}

/**
 * This method returns the dualUV status of the current technique of the effect.
 */
bool EffectMaterial::dualUV() const 
{ 
	BW_GUARD;
	return pManagedEffect_ ? 
		pManagedEffect_->dualUV( hOverriddenTechnique_ ) : false; 
}

/**
*	This method returns the channel for this material. If channel is overridden,
*	that is returned first. Otherwise the channel for the current technique is
*	returned.
*/
ChannelType EffectMaterial::channelType() const
{
	BW_GUARD;
	ChannelType ret = NULL_CHANNEL;

	if (pManagedEffect_)
	{
		ret = pManagedEffect_->getChannelType( hOverriddenTechnique_ );
	}

	return ret;
}


/**
 *  This method returns true if the given property is a default property.
 */
bool EffectMaterial::isDefault( EffectPropertyPtr pProperty )
{
	BW_GUARD;
	if ( pManagedEffect_ )
	{
		const EffectPropertyMappings& defaultProperties = pManagedEffect_->defaultProperties();
		EffectPropertyMappings::const_iterator dit = defaultProperties.begin();
		EffectPropertyMappings::const_iterator dend = defaultProperties.end();

		while (dit != dend)
		{
			if ( dit->second == pProperty )
				return true;	
			dit++;
		}
	}

	return false;
}


/**
 *  This method finds all shared default properties in the material, and
 *  replaces them with instanced properties.  This allows editing of
 *  properties without affecting other materials that use the same effect.
 */
void EffectMaterial::replaceDefaults()
{
	BW_GUARD;
	if ( pManagedEffect_ )
	{
		BW::vector< std::pair<D3DXHANDLE,EffectPropertyPtr> > props;

		// Iterate over properties and create property instances for them
		Properties::iterator it = properties_.begin();
		Properties::iterator end = properties_.end();
		while (it != end)
		{
			if ( isDefault( it->second ))
			{
				EffectPropertyPtr pProp = it->second->clone();
				it->second = pProp;
			}
			++it;
		}
	}
}

/**
 *	Set property in the effect material.
 *	Must be a bool, float, int, Matrix, Vector4, BaseTexturePtr or BW::string.
 *	@param name the name of the property to set.
 *	@param value the property's new value.
 *	@param create create the property if it did not exist in the material.
 *	@param typeName the type of property that your are trying to create.
 *		Possible values: "Bool", "Int", "Float", "Texture", "Vector4", "Matrix"
 *	@return true if the property was successfully set.
 */
template<class T>
bool EffectMaterial::setPropertyType(
	const BW::StringRef & name,
	const T & value,
	bool create,
	const BW::StringRef & typeName )
{
	BW_GUARD;
	EffectPropertyPtr effectProperty = this->getProperty( name );
	if( effectProperty )
	{
		return effectProperty->be( value );
	}
	else if ( create )
	{
		// Find property creator
		const EffectPropertyFactoryPtr& propertyFactory = 
			EffectPropertyFactory::findFactory( typeName );

		if (propertyFactory)
		{
			// Look up handle for property in effect
			char buffer[MAX_PARAM_NAME_SIZE];
			BW::StringBuilder builder( buffer, ARRAY_SIZE( buffer ));
			builder.append( name );
			D3DXHANDLE h = pManagedEffect_->pEffect()->
				GetParameterByName( NULL, builder.string() );

			// Found handle
			if (h != NULL)
			{
				// Create property
				EffectPropertyPtr prop =
					propertyFactory->create( name, h, 
							pManagedEffect_->pEffect() );
				prop->be( value );

				// Set handle in properties map
				properties_[ h ] = prop;
				return true;
			}
			else
			{
				ERROR_MSG( "EffectMaterial::setProperty "
					"Could not find handle for property %s, type %s\n", 
					name.to_string().c_str(),
					typeName.to_string().c_str() );
				return false;
			}
		}
        else
        {
            ERROR_MSG( "EffectMaterial::setProperty "
				"Could not find property processor for property %s, type %s\n", 
				name.to_string().c_str(),
				typeName.to_string().c_str() );
			return false;
        }
	}
	else
	{
		ASSET_MSG( "EffectMaterial::setProperty: "
			"Effect %s does not have a property called \"%s\"\n",
			identifier_.c_str(),
			name.to_string().c_str() );
		return false;
	}

	ASSET_MSG( "EffectMaterial::setProperty: "
			"Effect %s cannot set property \"%s\"\n",
			identifier_.c_str(),
			name.to_string().c_str() );
	return false;
}

bool EffectMaterial::setProperty(
	const BW::StringRef & name,
	const bool value,
	bool create )
{
	BW_GUARD;
	return this->setPropertyType< bool >(
		name, value, create, "Bool" );
}

bool EffectMaterial::setProperty(
	const BW::StringRef & name,
	const int value,
	bool create )
{
	BW_GUARD;
	return this->setPropertyType< int >(
		name, value, create, "Int" );
}

bool EffectMaterial::setProperty(
	const BW::StringRef & name,
	const float value,
	bool create )
{
	BW_GUARD;
	return this->setPropertyType< float >(
		name, value, create, "Float" );
}

bool EffectMaterial::setProperty(
	const BW::StringRef & name,
	const Moo::BaseTexturePtr & value,
	bool create )
{
	BW_GUARD;
	return this->setPropertyType< Moo::BaseTexturePtr >(
		name, value, create, "Texture" );
}

bool EffectMaterial::setProperty(
	const BW::StringRef & name,
	const Vector4 & value,
	bool create )
{
	BW_GUARD;
	return this->setPropertyType< Vector4 >(
		name, value, create, "Vector4" );
}

bool EffectMaterial::setProperty(
	const BW::StringRef & name,
	const Matrix & value,
	bool create )
{
	BW_GUARD;
	return this->setPropertyType < Matrix >(
		name, value, create, "Matrix" );
}

void EffectMaterial::generateTextureUsage( 
	Moo::ModelTextureUsageGroup & usageGroup, float texelDensity )
{
	const Moo::EffectMaterial::Properties & props = properties();
	for (Moo::EffectMaterial::Properties::const_iterator propIter = props.begin(),
		end = props.end(); propIter != end;
		++propIter)
	{
		const Moo::EffectPropertyPtr & prop = propIter->second;
		Moo::BaseTexturePtr texture = NULL;
		if (!prop->getTexture( texture ))
		{
			continue;
		}

		usageGroup.setTextureUsage( texture.get(), texelDensity );
	}
}

} // namespace Moo

BW_END_NAMESPACE

// effect_material.cpp
