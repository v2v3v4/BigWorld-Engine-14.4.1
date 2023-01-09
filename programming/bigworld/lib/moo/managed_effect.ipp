// managed_effect.ipp

#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif

namespace Moo
{

/**
 *	This method returns a pointer to the underlying ID3DXEffect interface.
 */
INLINE ID3DXEffect* ManagedEffect::pEffect() 
{
	return pEffect_.pComObject();
}

/**
 *	This method gets the resource ID that the effect was loaded from.
 */
INLINE const BW::string& ManagedEffect::resourceID() const
{
	return resourceID_; 
}

/**
 *	This method returns the number of passes required to draw the effect with
 *	the current technique. The begin() method should be called before this
 *	value is valid.
 */
INLINE uint32 ManagedEffect::numPasses() const
{
	return numPasses_;
}

/**
 *	This method returns the compile mark for this effect, which is incremented
 *	every time the effect gets reloaded.
 */
INLINE ManagedEffect::CompileMark ManagedEffect::compileMark() const
{
	return compileMark_;
}

/**
 *	This method returns the default property/value map.
 */
INLINE EffectPropertyMappings& ManagedEffect::defaultProperties()
{
	return defaultProperties_; 
}

/**
 *	This method provides access to the cached technique information.
 */
INLINE const ManagedEffect::TechniqueInfoCache& ManagedEffect::techniques()
{
	return techniques_;
}

/**
 * This method returns the handle of the currently used technique.
 */
INLINE D3DXHANDLE ManagedEffect::currentTechnique() const
{
	return currentTechnique_;
}

/**
 * This method finds the technique with the given handle, returns NULL if not
 * found.
 */
INLINE TechniqueInfo* 
	ManagedEffect::findTechniqueInfo( D3DXHANDLE handle )
{	
	BW_GUARD;
	
	// look up technique using "magic" handle which could be a string or an int
	for (	TechniqueInfoCache::iterator it = techniques_.begin();
			it != techniques_.end(); 
			it++ )
	{
		if ( handle == it->handle_ )
			return &(*it);
	}
	
	// its a string, do it the slow way
	D3DXTECHNIQUE_DESC techniqueDesc;
	if ( SUCCEEDED( pEffect_->GetTechniqueDesc( handle , &techniqueDesc ) ) )
	{	
		for (	TechniqueInfoCache::iterator it = techniques_.begin();
				it != techniques_.end(); 
				it++ )
		{
			if ( bw_stricmp( techniqueDesc.Name, it->name_.c_str() ) == 0 )
			{
				return &(*it);
			}
		}
	}
	
	return NULL;
}

/**
 * Returns a handle to the given named parameter.
 */
INLINE
ManagedEffect::Handle ManagedEffect::getParameterByName( const BW::string& name )
{
	BW_GUARD;
	return pEffect_->GetParameterByName( NULL, name.c_str() );
}

/**
 * Returns a handle to the parameter with the given semantic name.
 */
INLINE
ManagedEffect::Handle ManagedEffect::getParameterBySematic( const BW::string& semantic )
{
	BW_GUARD;
	return pEffect_->GetParameterBySemantic( NULL, semantic.c_str() );
}

/**
 * Sets the value of the given shader parameter.
 */
INLINE
void ManagedEffect::setBool( ManagedEffect::Handle h, bool b )
{
	BW_GUARD;
	pEffect_->SetBool( h, b );
}

/**
 * Sets the value of the given shader parameter.
 */
INLINE
void ManagedEffect::setInt( ManagedEffect::Handle h, int i )
{
	BW_GUARD;
	pEffect_->SetInt( h, i );
}

/**
 * Sets the value of the given shader parameter.
 */
INLINE
void ManagedEffect::setFloat( ManagedEffect::Handle h, float f )
{
	BW_GUARD;
	pEffect_->SetFloat( h, f );
}

/**
 * Sets the value of the given shader parameter.
 */
INLINE
void ManagedEffect::setVector( ManagedEffect::Handle h, const Vector2& vec )
{
	BW_GUARD;
	D3DXVECTOR4 d3dvec( vec.x, vec.y, 0, 0 );
	pEffect_->SetVector( h, &d3dvec );
}

/**
 * Sets the value of the given shader parameter.
 */
INLINE
void ManagedEffect::setVector( ManagedEffect::Handle h, const Vector3& vec )
{
	BW_GUARD;
	D3DXVECTOR4 d3dvec( vec.x, vec.y, vec.z, 0 );
	pEffect_->SetVector( h, &d3dvec );
}

/**
 * Sets the value of the given shader parameter.
 */
INLINE
void ManagedEffect::setVector( ManagedEffect::Handle h, const Vector4& vec )
{
	BW_GUARD;
	pEffect_->SetVector( h, (D3DXVECTOR4*)&vec );
}

/**
 * Sets the value of the given shader parameter.
 */
INLINE
void ManagedEffect::setMatrix( ManagedEffect::Handle h, const Matrix& mat )
{
	BW_GUARD;
	pEffect_->SetMatrix( h, &mat );
}

/**
 * Sets the value of the given shader parameter.
 */
INLINE
void ManagedEffect::setTexture( Handle h, const BaseTexturePtr& tex )
{
	BW_GUARD;
	if (tex)
	{
		pEffect_->SetTexture( h, tex->pTexture() );
	}
	else
	{
		pEffect_->SetTexture( h, NULL );
	}
}

} // namespace Moo

// managed_effect.ipp
