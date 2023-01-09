#ifndef LENS_EFFECT_MANAGER_HPP
#define LENS_EFFECT_MANAGER_HPP

#include "lens_effect.hpp"
#include "z_attenuation_occluder.hpp"
#include "moo/effect_material.hpp"
#include "cstdmf/singleton.hpp"

#include "cstdmf/bw_set.hpp"


BW_BEGIN_NAMESPACE

class PhotonOccluder;

enum LENS_EFFECT_TYPE
{
	SUN_EFFECT,
	SUN_VISIBILITY_TEST
};

/**
 *	This class manages and draws lens effects.
 *
 *	During the rendering pass, an object can
 *	register a lens effect with the LensEffectManager.
 *	If that lens effect is already alive, it will be
 *	refreshed.
 *	If this is a new lens effect, then it will fade in.
 *
 *	During LensEffectManager::tick, it will decay the
 *	age of current lens effects, and cull any lens
 *	effects that no longer exist.
 *
 *	During LensEffectManager::draw, it will perform
 *	line-of-sight checks on the scene, and ask
 *	individual lens effects to draw.
 *
 *	The lens effect manager uses plug-in photon occluder
 *	classes to perform line-of-sight checks.
 */
class LensEffectManager : public Singleton<LensEffectManager>
{
public:
	LensEffectManager();
	virtual ~LensEffectManager();

	virtual void tick( float dTime );
	virtual void draw();

	//Lens Effects use this method, because there is currently
	//no global material manager.  The LensEffectManager
	//manages the lens effect materials

	//the returned pointer should not be held onto.
	Moo::EffectMaterialPtr getMaterial( const BW::string& material );

	void	add( size_t id,
				 const Vector3 & worldPosition,
				 const LensEffect & le );

	void	forget( size_t id );

	void	clear( void );

	void	killFlaresInternal( const BW::set<size_t> & ids, LensEffectsMap& le );
	void	killFlares( const BW::set<size_t> & ids );

	void	addPhotonOccluder( PhotonOccluder * occluder );
	void	delPhotonOccluder( PhotonOccluder * occluder );

	void	preload( const BW::string& material );

	void	setScale ( size_t id, bool visibilityType2, float scale);

	float	sunVisibility();
	
	static bool s_partitcle_editor_;

	static uint32	s_drawCounter_;	

	float	flareVisible( LensEffect & l );

#if ENABLE_WATCHERS
	static void SetVisible(bool visible) { isVisible = visible; }
	static bool IsVisible() { return isVisible; }
#endif

private:
#if ENABLE_WATCHERS
	static bool isVisible;
#endif
	void	addInternal( size_t id,
				 const Vector3 & worldPosition,
				 const LensEffect & le,
				 LensEffectsMap& leMap );
	bool	forgetInternal ( size_t id, LensEffectsMap& le );
	void	killOldInternal( LensEffectsMap& le );
	void	killOld( void );

	//a map of id -> lens effects
	LensEffectsMap	lensEffects_;
	LensEffectsMap	lensEffects2_;

	//lens flare materials are managed.
	class Materials : public BW::map< BW::string, Moo::EffectMaterialPtr >
	{
	public:
		~Materials();
		void clear();
		iterator get( const BW::string & resourceID, bool reportError = false );
	};
	Materials materials_;

	//and a vector of photon occluders for lens flare style 1
	typedef BW::vector< PhotonOccluder * >	PhotonOccluders;
	PhotonOccluders		photonOccluders_;

	//and a helper class for lens flare style 2
	ZAttenuationOccluder zAttenuationHelper_;

	LensEffectManager(const LensEffectManager&);
	LensEffectManager& operator=(const LensEffectManager&);
};

#ifdef CODE_INLINE
#include "lens_effect_manager.ipp"
#endif

BW_END_NAMESPACE

#endif
/*lens_effect_manager.hpp*/
