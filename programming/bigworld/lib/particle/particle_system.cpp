#include "pch.hpp"

#include "particle_system.hpp"
#include "particle_serialisation.hpp"
#include "particle_system_manager.hpp"
#include "particle/actions/particle_system_action.hpp"
#include "particle/actions/source_psa.hpp"
#include "particle/actions/sink_psa.hpp"
#include "particle/renderers/particle_system_renderer.hpp"
#include "particle/renderers/mesh_particle_renderer.hpp"
#include "particle/renderers/visual_particle_renderer.hpp"
#include "particle/renderers/sprite_particle_renderer.hpp"
#include "model/super_model.hpp"	// for Model::s_blendCookie_
#include "romp/lens_effect_manager.hpp"
#include "moo/lod_settings.hpp"
#include "moo/geometrics.hpp"
#include "moo/render_context.hpp"
#include "chunk/chunk_manager.hpp"
#include "chunk/chunk_space.hpp"
#include "space/space_romp_collider.hpp"

#include "cstdmf/debug.hpp"
#include "cstdmf/string_builder.hpp"
#include "duplo/pymodelnode.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/xml_section.hpp"
#include "resmgr/win_file_system.hpp"

DECLARE_DEBUG_COMPONENT2( "Duplo", 0 )


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "particle_system.ipp"
#endif

namespace
{
// version number.  if any part of the particle system serialisation changes, bump this up
const int serialiseVersionCode = 2;
const BW::StringRef ActionsString( "Actions" );
const BW::StringRef RendererString( "Renderer" );
const BW::StringRef CapacityString( "capacity" );
const BW::StringRef BoundingBoxString( "boundingBox" );
const BW::StringRef MinString( "min" );
const BW::StringRef MaxString( "max" );
const int MAX_CAPACITY = 65536;
}


// -----------------------------------------------------------------------------
// Section: Static Variable for Particle System Actions.
// -----------------------------------------------------------------------------

const BW::StringRef ParticleSystem::VERSION_STRING( "serialiseVersionData" );

/// If the maxLod_ is set to this, then the particle system never lods out
/*static*/ const float ParticleSystem::LOD_INFINITE = 0.0f;

int ParticleSystem::s_counter_ = 0;

// -----------------------------------------------------------------------------
// Section: Constructor(s) and Destructor.
// -----------------------------------------------------------------------------

/*static*/ uint32 ParticleSystem::s_idCounter_ = 0;

/**
 *	This is the constructor for ParticleSystem.
 *
 *	@param initialCapacity	The initial limit to the amount of particles
 *							accepted by the system.
 */
ParticleSystem::ParticleSystem( int initialCapacity ) :	
	windFactor_( 0.f ),
	enabled_( true ),
	doUpdate_( false ),
	firstUpdate_( false ),
	pGS_( NULL ),
	pRenderer_( NULL ),
	particles_( new ContiguousParticles ),
	counter_( s_counter_++ % 10 ),
	boundingBox_( BoundingBox::s_insideOut_ ),
	vizBox_( BoundingBox::s_insideOut_ ),
	explicitPosition_( 0,0,0 ),
	explicitDirection_( 0,0,1 ),
	explicitTransform_( false ),
	localOffset_( 0.f, 0.f, 0.f ),
	maxLod_( 0.f ),	
	fixedFrameRate_( -1.f ),
	framesLeftOver_( 0.f ),
	static_( false ),	
	id_(s_idCounter_++),
	forcingSave_( false ),
	pOwnWorld_( NULL ),
	attached_( false ),
	inWorld_( false ),
	windEffect_( Vector3::zero() )
{	
	BW_GUARD;
	this->capacity( std::min( initialCapacity, MAX_CAPACITY ) );
}

/**
 *	This is the destructor for ParticleSystem.
 */
ParticleSystem::~ParticleSystem()
{
	BW_GUARD;
	ParticleSystemAction::removeLateUpdates( this );

	Actions::iterator iter = actions_.begin();

	while ( iter != actions_.end() )
	{
		ParticleSystemActionPtr pAction = *iter;
		iter = actions_.erase( iter );		
	}

	this->pRenderer( NULL );	
}


/**
 *	This method clones a particle system, but does not copy the current
 *	state of the particles.
 *
 *	@return ParticleSystem *	Pointer to the cloned particle system.
 */
ParticleSystem * ParticleSystem::clone() const
{
	ParticleSystem * ps = new ParticleSystem( this->capacity() );

	ps->name_ = name_;

	this->serialise( BW::CloneObject< ParticleSystem >( this, ps ) );

	// handle the actions
	ps->actions_.reserve( actions_.size() );
	for (Actions::const_iterator it = actions_.begin(), end = actions_.end();
		it != end; ++it)
	{
		ps->actions_.push_back( (*it)->clone() );
	}

	// handle the renderer
	if (pRenderer_)
	{
		ps->pRenderer( pRenderer_->clone() );
	}

	// set capacity after the renderer, because the particles
	// container has probably changed and we should now sets its capacity.
	ps->capacity( this->capacity() );

	// mimic save/load of bounding box
	{
		ps->vizBox_ = vizBox_;
#ifdef EDITOR_ENABLED
		ps->originalVizBox_ = originalVizBox_;
#endif//EDITOR_ENABLED
		ps->clearBoundingBox();
	}

	return ps;
}


/**
 *	This method loads a particle system, given a filename and base directory.
 *	
 *	@param	filename			The filename for the particle system.
 *	@param	directory			Base directory path, prepended to filename.
 *	@param	pError				out, if not NULL,append the error message into it.
 *
 *	@return bool				Success or failure of the load operation.
 */
bool ParticleSystem::load( const BW::StringRef & filename, 
						   const BW::StringRef & directory,
						   StringBuilder* pError/* = NULL*/ )
{
	BW_GUARD;
	char buffer[BW_MAX_PATH];
	BW::StringBuilder builder( buffer, ARRAY_SIZE( buffer ) );
	builder.append( directory );
	builder.append( filename );
	DataSectionPtr pDS = BWResource::openSection( builder.string() );
	return this->load( pDS, BW::StringRef(), false, pError );
}


/**
 *	This method loads a particle system, given a data section.
 *	
 *	@param	pDS					DataSection to load from. 
 *	@param	pError				out, if not NULL,append the error message into it.
 *
 *	@return bool				Success or failure of the load operation.
 */
bool ParticleSystem::load( DataSectionPtr pDS, const BW::StringRef & name,
	bool transient, StringBuilder* pError )
{
	BW_GUARD;
	if (pDS)
	{
		name_.assign( name.data(), name.size() );
		return this->loadInternal( pDS, transient, pError );
	}
	return false;
}


/**
 *	This method saves a particle system, to a given filename and base
 *	directory.
 *	
 *	@param	filename			The filename for the particle system.
 *	@param	directory			Base directory path, prepended to filename.
 *	@param	transient			Whether the save operation is transient. If
 *								set the false, the full visibility bounding
 *								box is recaculated for the save - this may
 *								take a few seconds.
 */
bool ParticleSystem::save( const BW::StringRef & filename,
	const BW::StringRef & directory, bool transient )
{
	BW_GUARD;

	char buffer[BW_MAX_PATH];
	BW::StringBuilder builder( buffer, ARRAY_SIZE( buffer ) );
	builder.append( directory );
	builder.append( filename );

	DataSectionPtr pDS = BWResource::openSection( builder.string() );

	if (this->save( pDS, name(), transient ))
	{
		// save cache to file
		return pDS->save( builder.string() );
	}

	return false;
}


bool ParticleSystem::save( DataSectionPtr pDS, const BW::StringRef & name,
	bool transient )
{
	if (!pDS)
	{
		return false;
	}

	// XMLSection might have sanitised the name
	name_.assign( name.data(), name.size() );

	// save the system properties
	return this->saveInternal( pDS, transient );
}


template < typename Serialiser >
void ParticleSystem::serialise( const Serialiser & s ) const
{
	BW_PARITCLE_SERIALISE( s, ParticleSystem, windFactor );
	BW_PARITCLE_SERIALISE( s, ParticleSystem, explicitPosition );
	BW_PARITCLE_SERIALISE( s, ParticleSystem, explicitDirection );
	// explicitTransform is modified by position and direction setter, so
	// serialise it last so it's loaded correctly.
	BW_PARITCLE_SERIALISE( s, ParticleSystem, explicitTransform );
	BW_PARITCLE_SERIALISE( s, ParticleSystem, localOffset );
	BW_PARITCLE_SERIALISE( s, ParticleSystem, maxLod );
	BW_PARITCLE_SERIALISE( s, ParticleSystem, fixedFrameRate );
}


bool ParticleSystem::loadInternal( DataSectionPtr pSect, bool transient,
								  StringBuilder* pError /*= NULL*/ )
{
	int serialiseVersionData = pSect->readInt( VERSION_STRING,
		serialiseVersionCode );
	if (serialiseVersionData != serialiseVersionCode)
	{
		if (pError)
		{
			pError->appendf( "ParticleSystem::serialiseInternal - version mismatch.  "
			"Code = %d, Data = %d.\n",
			serialiseVersionCode, serialiseVersionData );
		}
	}

	this->serialise( BW::LoadFromDataSection< ParticleSystem >( pSect, this ) );

	// For backwards compatibility try and read the wind half-life.  If it's
	// read then set the windFactor to 0.5.  If it's not read then do nothing.
	float windHL = pSect->readFloat( "windHalflife_", -1.0f );
	if (windHL != -1.0f)
	{
		// Scale so that a wind half-life of 5 seconds corresponds to
		// a wind factor of 0.5.
		float k = -0.2f*logf(0.5f);
		windFactor(expf(-k*fabsf(windHL)));
	}

	// handle the actions
	DataSectionPtr pActionSect = pSect->openSection( ActionsString );
	if (pActionSect)
	{
		actions_.clear();
		for (DataSectionIterator it = pActionSect->begin();
			it != pActionSect->end(); ++it)
		{
			const DataSectionPtr & pDS = *it;
			BW::string psaTypeStr = pDS->sectionName();
			int type = ParticleSystemAction::nameToType( psaTypeStr );
			ParticleSystemActionPtr newAction =
				ParticleSystemAction::createActionOfType( type ); 
			if (newAction)
			{
				newAction->load( pDS );
				actions_.push_back(newAction);
			}
			else if(pError)
			{
				pError->appendf( "Unknown Particle System Action type %s\n",
					psaTypeStr.c_str() );
			}
		}
	}

	// handle the renderer
	DataSectionPtr pRendererSect = pSect->openSection( RendererString );
	if (pRendererSect)
	{
		// create a new one, use iterators for convenience
		for (DataSectionIterator it = pRendererSect->begin();
			it != pRendererSect->end(); ++it)
		{
			const DataSectionPtr & pDS = *it;
			BW::string renderer = pDS->sectionName();

			ParticleSystemRendererPtr pr = 
				ParticleSystemRenderer::createRendererOfType( renderer, pDS );

			MF_ASSERT_DEV(pr);

			if (pr != NULL)
			{
				pr->load( pDS );

				// have to call through the accessor,
				// so the renderer's capacity is set.
				this->pRenderer( pr.getObject() );

				IF_NOT_MF_ASSERT_DEV(pRenderer())
				{
					break;
				}

				// TODO: can this go away?
				// if sprite renderer, let it know if the sprites can rotate
				if (pRenderer()->nameID() == SpriteParticleRenderer::nameID_)
				{
					bool canRotate = false;
					Actions & actions = actionSet();
					for (Actions::iterator it = actions.begin(),
						end = actions.end(); it != end; ++it)
					{
						ParticleSystemActionPtr act = *it;
						if (act->typeID() == PSA_SOURCE_TYPE_ID)
						{
							SourcePSA * source = (SourcePSA*)(&*act);
							if (source->initialRotation() != Vector2::zero() ||
								source->randomInitialRotation() != Vector2::zero() )
							{
								canRotate = true;
								break;
							}
						}
					}
					SpriteParticleRenderer * sp =
						(SpriteParticleRenderer*)(pRenderer().get());
					sp->rotated( canRotate );
				}
			}
		}
	}

	//Note : set capacity after the renderer, because the particles
	//container has probably changed and we should now sets its capacity.
	this->capacity( pSect->readInt( CapacityString, this->capacity() ) );

	// handle the bounding box
	DataSectionPtr pBB = pSect->openSection( BoundingBoxString );
	if (pBB)
	{
		vizBox_.setBounds( pBB->readVector3( MinString ),
			pBB->readVector3( MaxString ) );
#ifdef EDITOR_ENABLED
		originalVizBox_ = vizBox_;
#endif//EDITOR_ENABLED
	}
	else
	{
		vizBox_.setBounds( Vector3( -0.01f, -0.01f, -0.01f ),
			Vector3( 0.01f, 0.01f, 0.01f ) );
#ifdef EDITOR_ENABLED
		originalVizBox_ = vizBox_;
#endif//EDITOR_ENABLED
	}

	this->clearBoundingBox();

	return (pRenderer_ != NULL);
}


bool ParticleSystem::saveInternal( DataSectionPtr pSect, bool transient )
{
	pSect->writeInt( VERSION_STRING, serialiseVersionCode );

	this->serialise( BW::SaveToDataSection< ParticleSystem >( pSect, this ) );

	// handle the actions
	DataSectionPtr pActionSect = pSect->newSection( ActionsString );
	for (Actions::const_iterator iter = actions_.begin(),
		lastAction = actions_.end(); iter != lastAction; ++iter)
	{
		(*iter)->save( pActionSect );
	}

	// handle the renderer
	if (pRenderer_)
	{
		DataSectionPtr pRendererSect = pSect->newSection( RendererString );
		pRenderer_->save( pRendererSect );
	}

	// TODO: is this valid for save?
	pSect->writeInt( CapacityString, capacity() );

	// handle the bounding box
	if (!transient)
		this->forceFullBoundingBoxCalculation();
	else
		vizBox_ = boundingBox_;

	DataSectionPtr pBB = pSect->openSection( BoundingBoxString, true );
	if (pBB)
	{
		pBB->delChildren();
		if (!boundingBox_.insideOut())
		{
			pBB->writeVector3( MinString, boundingBox_.minBounds() );
			pBB->writeVector3( MaxString, boundingBox_.maxBounds() );
		}
		else
		{
			WARNING_MSG( "Bounding box was inside out during save\n" );
		}
	}

	return true;
}


/**
 *	This static method returns all of the resources that will be required to
 *	create the particle system.  It is designed so that the resources
 *	can be loaded in the loading thread before constructing the particle system.
 *
 *	@param	pSection	The data section containing the particle system.
 *	@param	output		The set of output resources that are prerequisites for
 *						creation of the particle system.
 */
/*static*/ void ParticleSystem::prerequisites( DataSectionPtr pSection, BW::set<BW::string>& output )
{
	BW_GUARD;
	const BW::StringRef ActionsString( "Actions" );

	DataSectionPtr pActionsSect = pSection->openSection(ActionsString);

	if (pActionsSect)
	{        
        for
        (
            DataSectionIterator it = pActionsSect->begin();
            it != pActionsSect->end();
            ++it
        )
        {
            DataSectionPtr pActionSect = *it;
			ParticleSystemAction::prerequisitesOfType(pActionSect, output);
		}
	}	


	const BW::StringRef RendererString = "Renderer";

	DataSectionPtr pRendererSect = pSection->openSection( RendererString );
	if (pRendererSect)
	{
		DataSection::iterator it;
		for(it = pRendererSect->begin(); it != pRendererSect->end(); ++it)
		{
			DataSectionPtr pDS = *it;
			ParticleSystemRenderer::prerequisitesOfType(pDS, output);
		}		
	}	
}


/**
 *	This method forces a bounding box calculation, by simulating
 *	the system for 30 seconds, and doing a full spawn.  It should
 *	only be used in non-time critical situations, for example during
 *	a save to xml.
 */
void ParticleSystem::forceFullBoundingBoxCalculation()
{
	BW_GUARD;
	forcingSave_ = true;

	BoundingBox bb( BoundingBox::s_insideOut_ );

	//calc bounding box. spawn all our particles at once,
	//then run the simulation for 30 seconds (the maximum particle age)
	//at 30fps.
	const float timeToRun = 30.f;
	const float tickTime = 1.f / 30.f;

	//spawn particles to capacity - this means non-time-triggered systems
	//also get bb's generated.
	this->clear();
	this->spawn( this->capacity() );

	for (float ft=0.f; ft<timeToRun; ft += tickTime)
	{
		//note - call update not tick, we want to avoid the runtime bb calculation
		//mechanism (instead just run the psystem, then we calculate bb explicitly)
		this->update(tickTime);
		ParticleSystemAction::flushLateUpdates();

		//need to update each frame, because particles may die in the meantime.  we
		//want the accumulated maximum.
		this->updateBoundingBox();
		if (!boundingBox_.insideOut())
		{
			bb.addBounds( boundingBox_ );
		}
	}
	// clear flares created during calculation
	if ( !flareIDs_.empty() )
		LensEffectManager::instance().killFlares( flareIDs_ );

	//bb has now accumulated the largest runtime size during 30 sec run.
	boundingBox_ = bb;
	vizBox_ = boundingBox_;

	forcingSave_ = false;
}


// -----------------------------------------------------------------------------
// Section: Accessors to Particle System Properties.
// -----------------------------------------------------------------------------

/**
 *	This is the set-accessor for the particle system's renderer. The old
 *	renderer, if any must have its reference count decremented before it is
 *	discarded by the particle system. Likewise, the renderer has its
 *	reference count incremented only if it is not NULL.
 *
 *	Note that assigning a new renderer recreates the particle container, as
 *	different renderers may have different containers.
 *
 *	@param	pNewRenderer		A pointer to the new renderer.
 */
void ParticleSystem::pRenderer( ParticleSystemRendererPtr pNewRenderer )
{
	BW_GUARD;
	if ( pRenderer_ != pNewRenderer )
	{
		size_t capacity = this->capacity();

		pRenderer_ = pNewRenderer;		

		if ( pRenderer_ != NULL )
		{			
			particles_ = pRenderer_->createParticleContainer();
			this->capacity( static_cast<int>(capacity) );

			//If you hit this assert, it means somebody has tried to marry MeshParticles
			//with a ContiguousContainer.  ContinguousContainers rely on particle.index() to exist,
			//and mesh particles don't have that in their data.
			MF_ASSERT( ! (dynamic_cast<ContiguousParticles*>(particles_.get()) && pRenderer_->isMeshStyle() ) );
		}
	}
}


/**
 *	This returns the worldTransform for the particle system. If the particle
 *	system is attached to a node, it uses that node's transform.
 *
 *	If there is no attached node, then the result is the identity matrix.
 *
 *	@return	The world transform matrix for the Particle System.
 */
Matrix ParticleSystem::worldTransform( void ) const
{
	BW_GUARD;
	Matrix retMatrix = Matrix::identity;
	if (explicitTransform_)
	{
		retMatrix.lookAt( explicitPosition_, explicitDirection_,
			fabsf(explicitDirection_.y) < 0.9f ? Vector3(0,1,0) : Vector3(0,0,1) );
		retMatrix.invert();
		retMatrix.preTranslateBy(localOffset_);
		return retMatrix;
	}
	else if (pOwnWorld_ != NULL)
	{
		retMatrix.setTranslate(localOffset_);
		retMatrix.postMultiply(pOwnWorld_->getMatrix());
		return retMatrix;
	}
	else
	{
		retMatrix.setTranslate(localOffset_);
		return retMatrix;
	}
}


/**
 *	This returns the worldTransform for the particle system, adjusted
 *	for options such a local/viewspace rendering. It modifies the results
 *	of the worldTransform call, and as such should be used as the
 *	definitive version of the world transform for rendering.
 *
 *	@return	The object to world transform matrix for the Particle System.
 */
Matrix ParticleSystem::objectToWorld( void ) const
{
	BW_GUARD;
	Matrix retMatrix;
	if ( pRenderer() != NULL )
	{
		if ( pRenderer()->local() )
		{
			retMatrix = Matrix::identity;
		}
		else
		{
			retMatrix = worldTransform();
			Matrix scaler;
			scaler.setScale( 1.f / retMatrix.applyToUnitAxisVector(0).length(), 
							 1.f / retMatrix.applyToUnitAxisVector(1).length(), 
							 1.f / retMatrix.applyToUnitAxisVector(2).length() );
			retMatrix.preMultiply( scaler );
		}

		if ( pRenderer()->viewDependent() )
		{
			// Adjust to view, if particle system is view dependent.
			retMatrix.postMultiply( Moo::rc().view() );
		}
	}
	else
	{
		retMatrix = worldTransform();
	}
	return retMatrix;
}


// -----------------------------------------------------------------------------
// Section: General Operations on Particle Systems.
// -----------------------------------------------------------------------------

/**
 *	This method adds an action to the particle system. An action may be
 *	added multiple times to the particle system to create a meta-action but
 *	it usually makes better sense to modify the attributes of an action
 *	to create the equivalent effect.
 *
 *	@param pAction		A pointer to the action to be added.
 */
void ParticleSystem::addAction( ParticleSystemActionPtr pAction )
{
	BW_GUARD;
	if ( pAction != NULL )
	{
		actions_.push_back( pAction );

		// If the action was a source action, pass any collision information
		// to it.
		if ( groundSpecifier() )
		{
			if ( pAction->typeID() == PSA_SOURCE_TYPE_ID )
			{
				SourcePSA *pSourcePSA = reinterpret_cast<SourcePSA *>(&*pAction);
				pSourcePSA->groundSpecifier( groundSpecifier() );
			}
		}		
	}
}

/**
 *	This method inserts an action to the particle system. An action may be
 *	added multiple times to the particle system to create a meta-action but
 *	it usually makes better sense to modify the attributes of an action
 *	to create the equivalent effect.
 *
 *	@param idx			The index at which to insert the action.
 *	@param pAction		A pointer to the action to be added.
 */
void ParticleSystem::insertAction( size_t idx, ParticleSystemActionPtr pAction )
{
	BW_GUARD;
	if ( pAction != NULL )
	{
		actions_.insert( actions_.begin() + idx, pAction );

		// If the action was a source action, pass any collision information
		// to it.
		if ( groundSpecifier() )
		{
			if ( pAction->typeID() == PSA_SOURCE_TYPE_ID )
			{
				SourcePSA *pSourcePSA = reinterpret_cast<SourcePSA *>(&*pAction);
				pSourcePSA->groundSpecifier( groundSpecifier() );
			}
		}		
	}
}

/**
 *	This method removes the first occurrence of an action of type actionID
 *	from the particle system. This is in order of first-created.
 *
 *	@param actionTypeID		The action type of the action to be removed.
 */
void ParticleSystem::removeAction( int actionTypeID )
{
	BW_GUARD;
	Actions::iterator iter = actions_.begin();
	Actions::iterator endOfVector = actions_.end();

	while ( iter != endOfVector )
	{
		ParticleSystemActionPtr pAction = *iter;
		if ( pAction->typeID() == actionTypeID )
		{
			// If the action was a source, remove any collision information
			// from it.
			if ( pAction->typeID() == PSA_SOURCE_TYPE_ID )
			{
				SourcePSA *pSourcePSA = reinterpret_cast<SourcePSA *>(&*pAction);
				pSourcePSA->groundSpecifier( NULL );
			}

			actions_.erase( iter );			
			break;
		}

		iter++;
	}
}

/**
 *	This method removes the action from the particle system.
 *
 *	@param pAction		A pointer to the action to be removed.
 */
void ParticleSystem::removeAction( ParticleSystemActionPtr pAction )
{
	BW_GUARD;
	if ( pAction != NULL )
	{
		Actions::iterator iter = actions_.begin();
		Actions::iterator endOfVector = actions_.end();

		while ( iter != endOfVector )
		{
			if ( *iter == pAction )
			{
				// If the action was a source, remove any collision information
				// from it.
				if ( pAction->typeID() == PSA_SOURCE_TYPE_ID )
				{
					SourcePSA *pSourcePSA = reinterpret_cast<SourcePSA *>
						(&*pAction);
					pSourcePSA->groundSpecifier( NULL );
				}

				actions_.erase( iter );				
				break;
			}

			iter++;
		}
	}
}

/**
 *	This method finds the first occurrence of an action of type actionID
 *	from the particle system. This is in order of first-created.
 *
 *	@param actionTypeID		The action type of the action to be accessed.
 */
ParticleSystemActionPtr ParticleSystem::pAction( int actionTypeID )
{
	BW_GUARD;
	Actions::iterator iter = actions_.begin();
	Actions::iterator endOfVector = actions_.end();

	while ( iter != endOfVector )
	{
		ParticleSystemActionPtr pAction = *iter;
		if ( pAction->typeID() == actionTypeID )
		{
			return *iter;
		}

		++iter;
	}

	return NULL;
}

/**
 *	This method finds the first occurrence of an action of type actionID
 *	from the particle system where the name is str. This is in order of 
 *  first-created.
 *
 *	@param actionTypeID		The action type of the action to be accessed.
 *  @param str              The name of the action to search for.  If this is 
 *                          empty then the first action of the appropriate id
 *                          is found.
 */
ParticleSystemActionPtr ParticleSystem::pAction( int actionTypeID,
    const BW::StringRef & str )
{
    BW_GUARD;
	if (str.empty())
    {
        return pAction(actionTypeID);
    }
    else
    {
	    Actions::iterator iter = actions_.begin();
	    Actions::iterator endOfVector = actions_.end();

	    while ( iter != endOfVector )
	    {
		    ParticleSystemActionPtr pAction = *iter;
		    if ( pAction->typeID() == actionTypeID && pAction->name() == str)
		    {
			    return *iter;
		    }

		    ++iter;
	    }

	    return NULL;
    }
}

/**
 *	This method updates the state of the particles for the time given. There
 *	is a distinction made between the drones and non-drones. These are update
 *	slightly differently.
 *
 *	@param dTime	Time since last update in seconds.
 */
bool ParticleSystem::tick( float dTime )
{
	BW_GUARD;

	if (!ParticleSystemManager::instance().active())
		return false;

	bool calcBoundingBox = false;

	// We only call the actual update on particle systems that are :
	// dynamic (meaning tick is only called by and controlled by parent)
	// static and we are told to.
	// When are static ps's told to update?
	// a) when they were visible last frame
	// b) for x seconds after loading (see ChunkParticles for implementation)
	if (!isStatic() || doUpdate_)
	{
		this->update( dTime );

		// counter is frames not calced for.
		if (++counter_ == 10)
		{
			counter_ = 0;
			calcBoundingBox = true;
		}

		// TODO : once the particle system has a good bounding box (whatever
		// that means), then we no longer need to update it every so often.	

		// OK, now actually calculate it if we ought
		if (calcBoundingBox)
		{
			updateBoundingBox();			
		}

		return true;
	}

	return false;
}


/**
 *	Update the particle systems for the given elapsed time.
 *
 *	@param	dTime	the delta frame time.
 */
void ParticleSystem::update( float dTime )
{
	BW_GUARD_PROFILER( ParticleSystem_update );
	PROFILER_SCOPED_DYNAMIC_STRING( name_.c_str() );

	if (!ParticleSystemManager::instance().active())
		return;

	dTime += framesLeftOver_;

	if ( dTime > 1.f )
		dTime = 1.f;

	// pre-calculate our wind vector.
	windEffect_ =  Math::clamp(0.0f, windFactor(), 1.0f) * 
		ParticleSystemManager::instance().windVelocity();

	while ( dTime > 0.f )
	{
		float dt = ( fixedFrameRate_ > 0.f ) ? ( 1.f / fixedFrameRate_ ) : dTime;

		//quantise the dt to be a fixed multiple of the particle's age increment.
		//this is to avoid inaccuracies in the particle's age calculation (very
		//important because that flows on to the overall lifetime of the system etc.)
		uint16 nAgeIncrements = Particle::nAgeIncrements( dt );
		if (nAgeIncrements == 0)
		{
			break;
		}

		dt = Particle::age( nAgeIncrements );

		if (dt > dTime)
		{
			if (almostEqual(dt, dTime)) // allow some inaccuracy..
			{
				dt = dTime;
			}
			else
			{
				break;
			}
		}

		dTime -= dt;

		// Execute aging pass for each particle. The actions needs to know
		// how much time has passed so they know how to adjust the properties
		// accordingly.
		if (particles_)
		{
			Particles::iterator pIter = particles_->begin();
			Particles::iterator endOfParticles = particles_->end();
			while ( pIter != endOfParticles )
			{
				if (pIter->isAlive())
				{
					uint16 age = pIter->ageAccurate();
					if ( (uint32)age + (uint32)nAgeIncrements > (uint32)Particle::ageMax() )
						pIter->ageAccurate( Particle::ageMax() );
					else
						pIter->ageAccurate( age + nAgeIncrements );
				}
				++pIter;
			}

			// Execute each of the actions.
			Actions::iterator aIter = actions_.begin();
			Actions::iterator endOfActions = actions_.end();
			while ( aIter != endOfActions )
			{
				if ( firstUpdate_ )
					(*aIter)->setFirstUpdate();
				if ( (*aIter)->enabled() )
					(*aIter)->execute( *this, dt );
				++aIter;
			}

			firstUpdate_ = false;
		}				

		// Apply movement pass for each particle. This pass applies the
		// velocities of the particle to the particle's position.
		if ( particles_ )
		{
			Particles::iterator pIter = particles_->begin();
			Particles::iterator endOfParticles = particles_->end();			
			while ( pIter != endOfParticles )
			{
				if (pIter->isAlive())
				{
					Particles::iterator particle = pIter;
					this->predictPosition( *particle, dt, particle->position() );

				}
				++pIter;
			}
		}

		if (pRenderer_)
			pRenderer_->update( begin(), end(), dt );
	}

	framesLeftOver_ = dTime;
	doUpdate_ = false;
}



/**
 *	This method moves a particle forward in time.
 *
 *	@param	particle		The particle to move
 *	@param	dt				The delta time to apply
 *	@param	retPos			[out] The new particle position.
 */
void ParticleSystem::predictPosition( const Particle & particle, float dt,
	Vector3 & retPos ) const
{	
	// Apply the effect of wind.
	Vector3 oldVelocity;
	particle.getVelocity( oldVelocity );
	Vector3 newVelocity = oldVelocity + windEffect_;
	retPos = particle.position() + dt * newVelocity;
}


/**
 *	Check if this if visible or lodded out.
 *	@param lod the lod of this particle system.
 *	@return true if this object is visible, false if lodded out.
 */
bool ParticleSystem::isLodVisible( float lod ) const
{
	BW_GUARD;

	if (!ParticleSystemManager::instance().active())
	{
		return false;
	}

	// Infinite lod
	if (maxLod_ <= ParticleSystem::LOD_INFINITE)
	{
		return true;
	}

	const float biasedMaxLod = LodSettings::instance().applyLodBias( maxLod_ );
	if (lod <= biasedMaxLod)
	{
		return true;
	}

	return false;
}


/**
 *	This methods tells the particle system to draw itself.
 *
 *	Particles are stored in world coordinates so we do not need
 *	the attachment transform which is passed in.
 *
 *	This method draws the particle system with the given transform,
 *	and at the desired level-of-detail.
 *
 *	@param	world	The world matrix to render with.
 *	@param	lod		The level-of-detail, or adjusted distance from the camera.
 */
void ParticleSystem::draw( Moo::DrawContext& drawContext, const Matrix &world, float lod )
{
	BW_GUARD_PROFILER( ParticleSystem_draw );
	PROFILER_SCOPED_DYNAMIC_STRING( name_.c_str() );
	// make sure particle systems have not been globally disabled,
	// and that we are close enough to be drawn
	if (!this->isLodVisible( lod ))
	{
		return;
	}

	// and now get the renderer to draw us
	if (pRenderer_ != NULL)
	{
		Matrix w( world );
		w.preTranslateBy( localOffset_ );
		pRenderer_->draw( drawContext, w, this->begin(), this->end(), this->boundingBox() );
	}

	/*Matrix previousWorld = Moo::rc().world();
	Moo::rc().world( Matrix::identity );
	BoundingBox drawBB = BoundingBox::s_insideOut_;
	this->localBoundingBox( drawBB );
	if ( !drawBB.insideOut() )
	{
		drawBB.transformBy( worldTransform() );
		Geometrics::wireBox( drawBB, 0x00ffff00 );
	}
	BoundingBox drawVBB = BoundingBox::s_insideOut_;
	this->worldVisibilityBoundingBox( drawVBB );
	if ( !drawVBB.insideOut() )
	{
		Geometrics::wireBox( drawVBB, 0x000000ff );
	}
	Moo::rc().world( previousWorld );*/

	// tick for at least the next frame.  this also means that visible
	// particle systems get ticked at all.
	doUpdate_ = true;	
}


/**
 *	This accumulates our bounding box into the given matrix.
 *
 *	@param	bb		The resultant bounding box. 
 */
void ParticleSystem::localBoundingBox( BoundingBox & bb ) const
{
	BW_GUARD;
	if (!enabled_)
		return;

	if (!isStatic() && particles_ && !particles_->size())
		return;

	if( boundingBox_.insideOut() )
		return;

	if (this->isLocal())
	{
		bb.addBounds( boundingBox_ );		
	}
	else
	{
		Matrix invWorld( worldTransform() );
		invWorld.invert();
		BoundingBox tbb( boundingBox_.minBounds(),
						 boundingBox_.maxBounds() );		
		tbb.transformBy( invWorld );
		bb.addBounds( tbb );
	}
}

/**
 *	This accumulates our visibility bounding box into the given variable.
 *
 *	@param	bb		The resultant bounding box. 
 */
void ParticleSystem::localVisibilityBoundingBox( BoundingBox & vbb ) const
{
	BW_GUARD;
	if (!enabled_)
		return;

	if (!isStatic() && particles_ && !particles_->size())
		return;

	if( vizBox_.insideOut() )
		return;

	if (this->isLocal())
	{
		vbb.addBounds( vizBox_ );
	}
	else
	{
		Matrix invWorld( worldTransform() );
		invWorld.invert();
		BoundingBox tbb( vizBox_.minBounds(),
						 vizBox_.maxBounds() );		
		tbb.transformBy( invWorld );
		vbb.addBounds( tbb );
	}

#ifdef EDITOR_ENABLED
	vbb.addBounds( originalVizBox_ );
#endif//EDITOR_ENABLED
}


/**
 *	This accumulates our bounding box into the given matrix.
 *
 *	@param	bb		The resultant bounding box. 
 *	@param	world	The world transform.
 */
void ParticleSystem::worldBoundingBox( BoundingBox & bb,
	const Matrix& world ) const
{
	BW_GUARD;
	if (!enabled_)
		return;

	if (!isStatic() && particles_ && !particles_->size())
		return;

	if( boundingBox_.insideOut() )
		return;

	if (this->isLocal())
	{
		const Matrix & world = worldTransform();
		BoundingBox tbb(	boundingBox_.minBounds(),
							boundingBox_.maxBounds() );
		tbb.transformBy( world );
		bb.addBounds( tbb );
	}
	else
	{
		bb.addBounds( boundingBox_ );
	}
}


/**
 *	This accumulates our visibility bounding box into the given variable.
 *
 *	@param	bb		The resultant bounding box. 
 */
void ParticleSystem::worldVisibilityBoundingBox( BoundingBox & wvbb ) const
{
	BW_GUARD;
	if (!enabled_)
		return;

	if (!isStatic() && particles_ && !particles_->size())
		return;

	if( vizBox_.insideOut() )
		return;

	if (this->isLocal())
	{
		BoundingBox tbb( vizBox_.minBounds(),
						 vizBox_.maxBounds() );
#ifdef EDITOR_ENABLED
		tbb.addBounds( originalVizBox_ );
#endif//EDITOR_ENABLED

		tbb.transformBy( worldTransform() );
		wvbb.addBounds( tbb );
	}
	else
	{
		wvbb.addBounds( vizBox_ );

#ifdef EDITOR_ENABLED
		BoundingBox wOriginalVizBox( originalVizBox_.minBounds(),
			originalVizBox_.maxBounds() );		
		wOriginalVizBox.transformBy( worldTransform() );
		wvbb.addBounds( wOriginalVizBox );
#endif//EDITOR_ENABLED

	}
}


/**
 *	This method attaches the particle system to the world via a
 *	matrix liason.  We give ourselves a ground specifier.
 *
 *	@param	pOwnWorld	The matrix liason that will provide our world matrix.
 *
 *	@returns True on success, false on error.
 */
bool ParticleSystem::attach( MatrixLiaison * pOwnWorld )
{
	BW_GUARD;
	if (attached_) return false;

	attached_ = true;
	pOwnWorld_ = pOwnWorld;	

	this->groundSpecifier( RompCollider::createDefault() );
	//before the particle system is put in the world, it has a local space
	//bounding box - so here we transform it into world space.
	this->clearBoundingBox();
	return true;
}


/**
 *	This method detaches the particle system from the existing
 *	matrix liason, undoing the attach operation.
 */
void ParticleSystem::detach()
{
	BW_GUARD;
	attached_ = false;
	pOwnWorld_ = NULL;
}


/**
 *	This method is called when we are tossed into the world.
 *
 *	@param	isOutside	Whether we are being placed inside or outside.
 */
void ParticleSystem::tossed( bool isOutside )
{
}


/**
 *	This method is called when the particle system is moved via a
 *	chunk item operation.
 *
 *	@param	dTime		Delta frame time.
 */
void ParticleSystem::move(float dTime)
{
}


/**
 *	The method is called when the particle system has first entered the world.
 *	Make sure to convert back to world coordinates when entering world.
 */
void ParticleSystem::enterWorld()
{
	BW_GUARD;
	SCOPED_DISABLE_FRAME_BEHIND_WARNING;
	if ( !this->isLocal() )
	{
		vizBox_.transformBy( worldTransform() );
	}
	this->clearBoundingBox();
}

/**
 *	The method is called when the particle system is leaving the world.
 *	Make sure to convert back to local coordinates when leaving world.
 */
void ParticleSystem::leaveWorld()
{
	BW_GUARD;
	if ( !this->boundingBox_.insideOut() && !this->isLocal() )
	{
		SCOPED_DISABLE_FRAME_BEHIND_WARNING;
		Matrix worldToLocal = worldTransform();
		worldToLocal.invert();
		this->boundingBox_.transformBy( worldToLocal );
		this->vizBox_.transformBy( worldToLocal );
	}
}


/**
 *	This method tells the particle system to remove all particles from itself.
 */
void ParticleSystem::clear( void )
{
	BW_GUARD;
	if ( !flareIDs_.empty() )
		LensEffectManager::instance().killFlares( flareIDs_ );
	if ( particles_ )
		particles_->clear();	
	this->clearBoundingBox();	
}


/**
 *	This method sets a new 'empty' bounding box, which for psystems is
 *	a small (1cm) box positioned in world space.  This is necessary because
 *	if psystems aren't seen, they aren't calculated (and thus won't ever get
 *	any particles).
 */
void ParticleSystem::clearBoundingBox()
{
	BW_GUARD;
	boundingBox_.setBounds( Vector3(-0.01f,-0.01f,-0.01f), Vector3(0.01f,0.01f,0.01f) );
	if (!this->isLocal())
	{
		SCOPED_DISABLE_FRAME_BEHIND_WARNING;
		this->boundingBox_.transformBy( pOwnWorld_->getMatrix() );
	}

#ifndef EDITOR_ENABLED
	if (this->isStatic())
	{
		// Not reset vizBox_ cause chunk based systems
		// should only ever grow in size.
	}
	else
#endif
	{
		vizBox_ = boundingBox_;
	}

}


/**
 *	This is the Set-Accessor for the ground specifier of the particle
 *	system.
 *
 *	@param pGS	A smart pointer to the new ground specifier.
 */
void ParticleSystem::groundSpecifier( RompColliderPtr pGS )
{
	BW_GUARD;
	pGS_ = pGS;

	// Need to inform all source actions of the ground specifier.
	if ( groundSpecifier() )
	{
		Actions::iterator iter = actions_.begin();
		Actions::iterator endOfVector = actions_.end();

		while ( iter != endOfVector )
		{
			ParticleSystemActionPtr pAction = *iter;
			if ( pAction->typeID() == PSA_SOURCE_TYPE_ID )
			{
				SourcePSA *pSourcePSA = reinterpret_cast<SourcePSA *>(&*pAction);
				pSourcePSA->groundSpecifier( groundSpecifier() );
			}

			++iter;
		}
	}
}


/**
 *	This method returns the memory used by the particle system.
 *
 *	@return size_t	Size, in bytes, used by the particle system.
 */
size_t ParticleSystem::sizeInBytes() const
{
	BW_GUARD;
	size_t footprint = sizeof(ParticleSystem);
	if (particles_)
		footprint += particles_->capacity() * Particle::sizeInBytes();
	if (pRenderer_)
		footprint += pRenderer_->sizeInBytes();

	Actions::const_iterator iter = actions_.begin();
	Actions::const_iterator endOfVector = actions_.end();

	while ( iter != endOfVector )
	{
		ParticleSystemActionPtr pAction = *iter;
		footprint += pAction->sizeInBytes();
		++iter;
	}

	return footprint;
}


/**
 *	This method spawns a number of particles on-demand.
 *
 *	@param	num		The number of particles to spawn.
 */
void ParticleSystem::spawn( int num )
{
	BW_GUARD;
	for (ParticleSystem::Actions::iterator actionIt = actions_.begin(),
			end = actions_.end(); actionIt != end; ++actionIt)
	{
		if ((*actionIt)->typeID() == PSA_SOURCE_TYPE_ID)
		{
			SourcePSA * source = (SourcePSA *)((*actionIt).getObject());
			source->force(num);
		}
	}
}


/**
 *	This is the Set-Accessor for the capacity property.
 *
 *	@param number	The new maximum number of particles allowed.
 */
void ParticleSystem::capacity( int number )
{
	BW_GUARD;
	//Code error - particle system storage should be constructed before
	//setting the capacity.  Usually this means having a renderer first.
	IF_NOT_MF_ASSERT_DEV( particles_.getObject() )
	{
		return;
	}

	if (number > MAX_CAPACITY)
	{
		number = MAX_CAPACITY;
		WARNING_MSG( "ParticleSystem::capacity : Maximum Capacity exceeded - setting capacity to %d\n", MAX_CAPACITY );
	}

	size_t currentCapacity = particles_->capacity();

	if (number != currentCapacity)
	{
		this->clear();
		particles_->reserve( number );
	}
}


/**
 *	This method returns the duration of the particle system.  The
 *	duration is only valid for particle systems containing at least
 *	one sink action that specifies a maximum particle age.
 *
 *	@return float	The duration of the particle system, or -1 if
 *					the particle system has no fixed duration.
 */
float ParticleSystem::duration() const
{
	BW_GUARD;
	float duration = -1;

	Actions::const_iterator it = actions_.begin();

	while ( it != actions_.end() )
	{
		if ( (*it)->typeID() == PSA_SINK_TYPE_ID )
		{
			const SinkPSA *pSinkPSA = reinterpret_cast<const SinkPSA *>((*it).getObject());
			duration = max(pSinkPSA->maximumAge(), duration);
		}

		++it;
	}

	return duration;
}


/**
 *	This method updates the bounding box of the system.
 */
void ParticleSystem::updateBoundingBox()
{
	BW_GUARD;
	this->clearBoundingBox();

    float sz = 0.01f; // Potential size of the largest particle

    // Iterate through the particles, expanding the bounding box by 
    // the positions.  In addition, get the size of the largest particle
    // if not a mesh render.
	bool meshRenderer = false;
	if (pRenderer())
	{
		meshRenderer = pRenderer()->isMeshStyle();
	}

	Particles::const_iterator iter = this->begin();
	while (iter != this->end())
	{
		const Particle & p = *iter;
		if (p.isAlive())
		{
			boundingBox_.addBounds( p.position() );
			if (!meshRenderer)
				sz = std::max(sz, p.size());
		}
		++iter;
	}

	// Query the renderer about further expansion of the bounding box.
	if (pRenderer() && pRenderer()->knowParticleRadius())
		sz = std::max(sz, pRenderer()->particleRadius());

	// Expand the bounding box by the potential size of the largest
	// particle.
	boundingBox_.expandSymmetrically(sz, sz, sz);

	// update so you know the maximum bounding box
	// Ignore static state in editor as the particle system can
	// be dragged around.
#ifndef EDITOR_ENABLED
	if (this->isStatic())
	{
		// static, i.e. Chunk based systems, should only ever grow
		// in size.
		vizBox_.addBounds(boundingBox_);
	}
	else
#endif
	{
		// dynamic, i.e. attached to PyModel systems, should shrink
		// when the bounding box is calculated.  This is because you
		// can feasibly drag a system all over the world, and you don't
		// want the bb to grow ever larger.
		// Also used in WE
		vizBox_ = boundingBox_;
	}
}


/**
 *	This method is called to set a flag on the particle system indicating
 *	it is static or not.  In reality this method only gets called with true.
 *	If true, then it means the particle system has been placed as a chunk
 *	item.
 *
 *	@param	s	static or not.
 */
void ParticleSystem::isStatic( bool s )
{
	static_ = s;
}

/**
 *	This is the hash function which gets a uniqueID for the particle
 *
 *	@param p	The particle to get the uniqueID for
 *	@param ps	The particle system that the particle belongs to
 */
/*static*/ uint32 ParticleSystem::getUniqueParticleID( Particles::iterator p, const ParticleSystem& ps )
{
	uint32 index = static_cast<uint32>(ps.particles()->index(p));
	return index + ps.id() * MAX_CAPACITY + MAX_CAPACITY;
}

BW_END_NAMESPACE

// particle_system.cpp
