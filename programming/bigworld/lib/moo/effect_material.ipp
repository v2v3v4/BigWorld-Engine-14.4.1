// effect_material.ipp

#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif

namespace Moo {

INLINE uint32 EffectMaterial::numPasses() const
{
	BW_GUARD;
	return pManagedEffect_->numPasses();
}

INLINE void EffectMaterial::identifier( const BW::StringRef & id )
{
	identifier_.assign( id.data(), id.size() );
}

INLINE const BW::string& EffectMaterial::identifier() const
{
	return identifier_;
}

} // namespace Moo

// effect_material.ipp
