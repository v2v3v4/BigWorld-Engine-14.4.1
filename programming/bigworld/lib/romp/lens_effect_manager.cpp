#include "pch.hpp"

#include "lens_effect_manager.hpp"
#include "moo/render_context.hpp"
#include "cstdmf/debug.hpp"
#include "photon_occluder.hpp"
#include "resmgr/bwresource.hpp"

DECLARE_DEBUG_COMPONENT2( "Romp", 2 )


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "lens_effect_manager.ipp"
#endif

BW_SINGLETON_STORAGE( LensEffectManager )

///static initialiser
uint32	LensEffectManager::s_drawCounter_ = 0;

#if ENABLE_WATCHERS
bool LensEffectManager::isVisible = true;
#endif // ENABLE_WATCHERS

///constructor
LensEffectManager::LensEffectManager()
{
	BW_GUARD;
#if ENABLE_WATCHERS
	MF_WATCH("Visibility/Lens effects", &(IsVisible), 
		&(SetVisible), "Are lens effects visible." );
#endif
}


///destructor
LensEffectManager::~LensEffectManager()
{
	BW_GUARD;
	materials_.clear();
}


/**
 *	This method updates all current lens effects.
 *
 *	@param dTime the change in time since the last frame
 */
void LensEffectManager::tick( float dTime )
{
	BW_GUARD_PROFILER( LensManager_Tick );

	//Start occlusion testing
	PhotonOccluders::iterator pit = photonOccluders_.begin();
	PhotonOccluders::iterator pend = photonOccluders_.end();

	while( pit != pend )
	{
		(*pit++)->beginOcclusionTests();
	}

	//Tick flares type 1
	LensEffectsMap::iterator it = lensEffects_.begin();
	LensEffectsMap::iterator end = lensEffects_.end();

	uint64 startTime = timestamp();
	uint64 maxExecutionTime = stampsPerSecond() / 1000;

	while( it != end )
	{
		LensEffect & l = it->second;
		float visibility = flareVisible( l );
		l.tick( dTime, visibility );
		++it;
	}

	//End occlusion testing
	pit = photonOccluders_.begin();
	pend = photonOccluders_.end();

	while( pit != pend )
	{
		(*pit++)->endOcclusionTests();
	}

	//Tick and draw flares type 2
	zAttenuationHelper_.update( lensEffects2_ );

	//We shouldn't need to do this here but due to the usage of materials
	//by photon occluders, we can't assume anything about the device state
	//Therefore we should reset any global states.  Color writes are the
	//only one that is managed globally by the engine.
	Moo::rc().setRenderState( D3DRS_COLORWRITEENABLE,
		D3DCOLORWRITEENABLE_RED | 
		D3DCOLORWRITEENABLE_GREEN | 
		D3DCOLORWRITEENABLE_BLUE );

	this->killOld();
}


/**
 *	This method draws all current lens effects.
 */
void LensEffectManager::draw()
{
	BW_GUARD_PROFILER( LensManager_Draw );

#if ENABLE_WATCHERS
	if(!isVisible)
		return;
#endif

	//Setup rendering state for lens flares
	Moo::rc().setVertexShader( NULL );
	Moo::rc().setFVF( Moo::VertexTLUV::fvf() );
	Moo::rc().setRenderState( D3DRS_LIGHTING, FALSE );

	//Draw lens flares type 1
	LensEffectsMap::iterator lit = lensEffects_.begin();
	LensEffectsMap::iterator lend = lensEffects_.end();

	while( lit != lend )
	{
		LensEffect & l = lit->second;
		l.draw();
		++lit;
	}

	//Restore rendering state
	Moo::rc().setRenderState( D3DRS_LIGHTING, TRUE );

	//Increment draw counter
	s_drawCounter_++;
}


/**
 *	This method performs visibility analysis on a single
 *	lens effect.  All of the plug-in photon occluders
 *	are checked against.
 *
 *	@param l	the lensEffect to check.
 *	@return The visibility of the flare
 */
float LensEffectManager::flareVisible( LensEffect & l )
{
	BW_GUARD;
	float visibility = 1.f;

	//First, check if the lens effect is too far away
	float dist = Vector3( l.position()- Moo::rc().invView().applyToOrigin() ).lengthSquared();
	if ( dist > l.maxDistance() * l.maxDistance() )
		return 0.f;

	//Then, check the lens effect projects onto the screen
	Vector4 projPos;
	Vector4 in( l.position().x, l.position().y, l.position().z, 1.f );
	Moo::rc().viewProjection().applyPoint( projPos, in );

	//For small area lens flares, as soon as they go off the screen,
	//we can ignore them.  This test is disabled for larger area flares
	//(like the sun) as we don't want to occlude the sun as soon as its
	//centre point is off-screen.
	if ( l.area() < 5.f )
	{
		if (( projPos.x < -projPos.w ) ||
			( projPos.x >  projPos.w ) ||
			( projPos.y < -projPos.w ) ||
			( projPos.y >  projPos.w ) ||
			( projPos.w <=       0.f ))
			return 0.f;
	}

	// Check to see if nobody cares for this flare anymore
	//if ( l.added() != s_drawCounter_ )
	//	return 0.f;

	// Run the Occlusion tests
	Vector3 cameraPosition = Moo::rc().invView().applyToOrigin();

	//Move flare position towards camera by 50cm., to account for
	//geometry the lens flare may be residing within.
	Vector3 dir( l.position() - cameraPosition  );
	dir.normalise();
	dir *= 0.5f;
	Vector3 testPos = l.position() - dir;

	//Check our plug-in photon occluders
	cameraPosition += Moo::rc().invView().applyToUnitAxisVector( 2 ) *
		( Moo::rc().camera().nearPlane() * 1.01f );

	PhotonOccluders::iterator it = photonOccluders_.begin();
	PhotonOccluders::iterator end = photonOccluders_.end();

	while( ( it != end ) && ( visibility != 0.f ) )
	{
		PhotonOccluder * occluder = *it++;

		float result = occluder->collides( testPos, cameraPosition, l );

		visibility = std::min( visibility, result );
	}

	return visibility;
}


/**
 *	This method adds a lens effect for a single frame to the manager.
 *	If the lens effect is already in existence( if the id matches
 *	one already in the list ), then it is simply refreshed.
 *
 *	@param	id		a unique id for the lens flare.  This can be anything
 *					that is persistent between frames.
 *	@param	worldPosition the world position of the lens effect
 *	@param	le		contains the details about the lens effect to add.
 *	@param	leList	lens effect list to add to.
 */
void LensEffectManager::addInternal( size_t id,
					const Vector3 & worldPosition,
					const LensEffect & le,
					LensEffectsMap& leMap )
{
	BW_GUARD;
	//See if this lens effect is already there
	LensEffectsMap::iterator it = leMap.find(id);

	if( it != leMap.end() )
	{
		LensEffect & l = it->second;

		l.position( worldPosition );
		l.colour( le.colour() );
		l.added( s_drawCounter_ );
		l.maxDistance( le.maxDistance() );
		l.area( le.area() );
		l.fadeSpeed( le.fadeSpeed() );

		//if flare is in it's "grace period" of 1 sec, but now it is
		//being added again, then give it a valid age.
		//i.e. don't allow it a grace period of 1 sec before being drawn again.

		float a = l.age();
		if ( a > OLDEST_LENS_EFFECT)
			l.ageBy( OLDEST_LENS_EFFECT - a );

		return;
	}


	//This is a new lens effect.  add to our list
	LensEffect newEffect( le );
	newEffect.id( id );
	newEffect.position( worldPosition );
	newEffect.added( s_drawCounter_ );

	leMap[id] = newEffect;
}



/**
 *	This method adds a lens effect for a single frame to the manager.
 *	If the lens effect is already in existence( if the id matches
 *	one already in the list ), then it is simply refreshed.
 *
 *	For simple flares, it will use 2nd implementation designed for
 *	large numbers of simple flares.
 *
 *	@param	id		a unique id for the lens flare.  This can be anything
 *					that is persistent between frames.
 *	@param	worldPosition the world position of the lens effect
 *	@param	le		contains the details about the lens effect to add.
 */
void LensEffectManager::add( size_t id,
					const Vector3 & worldPosition,
					const LensEffect & le )
{
	BW_GUARD;
	LensEffectsMap& lem = le.visibilityType2() ? lensEffects2_ : lensEffects_;

	this->addInternal( id, worldPosition, le, lem );
}


/**
 *	This method causes the lens effect manager to forget about the
 *	given lens effect id. It will accept no more updates for the existing
 *	lens effect with that id. This is only necessary if you want to reuse
 *	the id for a different lens effect.
 *
 *	@return	bool	true if the id was found in the list.
 */
bool LensEffectManager::forgetInternal( size_t id, LensEffectsMap& leMap )
{
	BW_GUARD;
	LensEffectsMap::iterator it = leMap.find( id );

	if( it != leMap.end() )
	{
		leMap.erase( it );
		return true;
	}
	return false;
}


/**
 *	This method causes the lens effect manager to forget about the
 *	given lens effect id. It will accept no more updates for the existing
 *	lens effect with that id. This is only necessary if you want to reuse
 *	the id for a different lens effect.
 */
void LensEffectManager::forget( size_t id )
{
	BW_GUARD;
	if ( !forgetInternal( id, lensEffects_ ) )
	{
		forgetInternal( id, lensEffects2_ );
	}
}


/**
 *	This method kills all lens effects from the list, with 
 * 	exception of the sun.
 */
void LensEffectManager::clear()
{
	BW_GUARD;
	lensEffects_.clear();
	lensEffects2_.clear();
}


/**
 *	This method culls lens effects from the list corresponding to the supplied
 *	id list.
 *
 *	@param	ids the id's of the lens effects to cull from the list.
 */
void LensEffectManager::killFlaresInternal( const BW::set<size_t> & ids, LensEffectsMap& leMap )
{
	BW_GUARD;
	for ( BW::set<size_t>::const_iterator it = ids.begin(); it != ids.end(); it++ )
	{
		LensEffectsMap::iterator lt = leMap.find(*it);
		if( lt != leMap.end() )
		{
			leMap.erase( lt );
		}
	}
}


/**
 *	This method culls lens effects from the list corresponding to the supplied
 *	id list.
 *
 *	@param	ids the id's of the lens effects to cull from the list.
 */
void LensEffectManager::killFlares( const BW::set<size_t> & ids )
{
	BW_GUARD;
	killFlaresInternal( ids, lensEffects_ );
	killFlaresInternal( ids, lensEffects2_ );
}


/**
 *	This method culls all dead lens effects from the list.
 */
void LensEffectManager::killOldInternal( LensEffectsMap& leMap )
{
	BW_GUARD;
	static BW::vector<size_t> eraseIDs;

	LensEffectsMap::iterator it = leMap.begin();
	while( it != leMap.end() )
	{
		LensEffect & l = it->second;
		if ( l.age() >= KILL_LENS_EFFECT )
		{
			eraseIDs.push_back( it->first );
		}
		++it;
	}

	BW::vector<size_t>::iterator eit = eraseIDs.begin();
	BW::vector<size_t>::iterator een = eraseIDs.end();
	while ( eit != een )
	{
		leMap.erase(*eit);
		++eit;
	}

	eraseIDs.clear();
}


/**
 *	This method culls all dead lens effects from the list.
 */
void LensEffectManager::killOld()
{
	BW_GUARD;
	killOldInternal( lensEffects_ );
	killOldInternal( lensEffects2_ );
}


/**
 *	This method sets the appropriate material for the
 *	given lens effect
 *
 *	@param material	Name of the material to retrieve.
 *
 *	@returns A pointer to the Moo::EffectMaterial on success, NULL on error.
 */
Moo::EffectMaterialPtr LensEffectManager::getMaterial( const BW::string& material )
{
	BW_GUARD;
	if (material != "" )
	{
		Materials::iterator it = materials_.get( material );
		if (it != materials_.end())
		{
			return it->second;
		}
	}

	return NULL;
}


/**
 *	This method preloads the material used by this lens effect
 */
void LensEffectManager::preload( const BW::string& material )
{
	BW_GUARD;
	if (material != "")
		materials_.get( material, true );
}


/**
 *	Method to remove a photon occluder
 */
void LensEffectManager::delPhotonOccluder( PhotonOccluder * occluder )
{
	BW_GUARD;
	PhotonOccluders::iterator it = std::find(
		photonOccluders_.begin(), photonOccluders_.end(), occluder );
	if (it != photonOccluders_.end()) photonOccluders_.erase( it );
}

void LensEffectManager::setScale( size_t id, bool visibilityType2, float scale )
{
	LensEffectsMap& lem = visibilityType2 ? lensEffects2_ : lensEffects_;
	LensEffectsMap::iterator it = lem.find(id);
	if(it != lem.end())
	{
		it->second.size(scale);
	}
}

float prevVisibility = 1.0f;

float LensEffectManager::sunVisibility()
{
	LensEffectsMap::iterator it = lensEffects_.find(SUN_VISIBILITY_TEST);
	if(it != lensEffects_.end())
	{
		prevVisibility = it->second.visibility();
		return prevVisibility;
	}
	return prevVisibility;
}
/**
 *	This method deletes this vector properly
 */
LensEffectManager::Materials::~Materials()
{
	BW_GUARD;
	this->clear();
}


void LensEffectManager::Materials::clear()
{
	BW_GUARD;
	this->BW::map< BW::string, Moo::EffectMaterialPtr >::clear();
}


/**
 *	This method gets the named material from this material map,
 *	creating it if it does not exist. If the material can't be found,
 *	then a blank (unfogged) material is used instead.
 */
LensEffectManager::Materials::iterator LensEffectManager::Materials::get(
	const BW::string & resourceID, bool reportError )
{
	BW_GUARD;
	iterator it = this->find( resourceID );
	if (it == this->end())
	{
		value_type v( resourceID, new Moo::EffectMaterial() );
		DataSectionPtr pSection = BWResource::openSection(resourceID);
		if (pSection.getObject()!=NULL)
		{
			v.second->load( pSection );
			it = this->insert( v ).first;
		}
		else
		{
			if (reportError)
			{
				ASSET_MSG( "Could not load %s\n", resourceID.c_str() );
			}
			return this->end();
		}
	}
	return it;
}

BW_END_NAMESPACE

/*lens_effect_manager.cpp*/
