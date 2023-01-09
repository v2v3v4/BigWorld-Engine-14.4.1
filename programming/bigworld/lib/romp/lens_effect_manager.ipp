
#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif


/**
 * This method adds a photon occluder to the lens effects visibility
 * determination.
 */
INLINE void LensEffectManager::addPhotonOccluder( PhotonOccluder * occluder )
{
	BW_GUARD;
	photonOccluders_.push_back( occluder );
}


/*lens_effect_manager.ipp*/
