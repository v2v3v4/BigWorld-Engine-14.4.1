#include "pch.hpp"

#include "pyscript/pyobject_plus.hpp"
#include "particle/particle_serialisation.hpp"
#include "particle/particle_system.hpp"
#include "particle_system_action.hpp"
#include "source_psa.hpp"
#include "sink_psa.hpp"
#include "barrier_psa.hpp"
#include "force_psa.hpp"
#include "stream_psa.hpp"
#include "jitter_psa.hpp"
#include "scaler_psa.hpp"
#include "tint_shader_psa.hpp"
#include "node_clamp_psa.hpp"
#include "orbitor_psa.hpp"
#include "flare_psa.hpp"
#include "collide_psa.hpp"
#include "matrix_swarm_psa.hpp"
#include "magnet_psa.hpp"
#include "splat_psa.hpp"
#include "resmgr/xml_section.hpp"

DECLARE_DEBUG_COMPONENT2( "Particle", 0 )


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "particle_system_action.ipp"
#endif

// -----------------------------------------------------------------------------
// Section: Static Variable for Particle System Actions.
// -----------------------------------------------------------------------------

int SourcePSA::typeID_		= PSA_SOURCE_TYPE_ID;
int SinkPSA::typeID_		= PSA_SINK_TYPE_ID;
int BarrierPSA::typeID_		= PSA_BARRIER_TYPE_ID;
int ForcePSA::typeID_		= PSA_FORCE_TYPE_ID;
int StreamPSA::typeID_		= PSA_STREAM_TYPE_ID;
int JitterPSA::typeID_		= PSA_JITTER_TYPE_ID;
int ScalerPSA::typeID_		= PSA_SCALAR_TYPE_ID;
int TintShaderPSA::typeID_	= PSA_TINT_SHADER_TYPE_ID;
int NodeClampPSA::typeID_	= PSA_NODE_CLAMP_TYPE_ID;
int OrbitorPSA::typeID_		= PSA_ORBITOR_TYPE_ID;
int FlarePSA::typeID_		= PSA_FLARE_TYPE_ID;
int CollidePSA::typeID_		= PSA_COLLIDE_TYPE_ID;
int MatrixSwarmPSA::typeID_	= PSA_MATRIX_SWARM_TYPE_ID;
int MagnetPSA::typeID_		= PSA_MAGNET_TYPE_ID;
int SplatPSA::typeID_		= PSA_SPLAT_TYPE_ID;

const BW::string SourcePSA::nameID_		= "Source";
const BW::string SinkPSA::nameID_			= "Sink";
const BW::string BarrierPSA::nameID_		= "Barrier";
const BW::string ForcePSA::nameID_			= "Force";
const BW::string StreamPSA::nameID_		= "Stream";
const BW::string JitterPSA::nameID_		= "Jitter";
const BW::string ScalerPSA::nameID_		= "Scaler";
const BW::string TintShaderPSA::nameID_	= "TintShader";
const BW::string NodeClampPSA::nameID_		= "NodeClamp";
const BW::string OrbitorPSA::nameID_		= "Orbitor";
const BW::string FlarePSA::nameID_			= "Flare";
const BW::string CollidePSA::nameID_		= "Collide";
const BW::string MatrixSwarmPSA::nameID_	= "MatrixSwarm";
const BW::string MagnetPSA::nameID_		= "Magnet";
const BW::string SplatPSA::nameID_			= "Splat";


/*static*/ ParticleSystemAction::UpdateLaterQueue
	ParticleSystemAction::s_updateLaterQueue_;


/**
 *	Put an action into a queue to be updated later.
 *	Make sure the queue is flushed using flushLateUpdates().
 *	@param updateLaterAction action to defer to later.
 */
/*static*/ void ParticleSystemAction::pushLateUpdate(
	ParticleSystemAction& action,
	ParticleSystem& particleSystem,
	float dTime )
{
	ParticleSystemAction::s_updateLaterQueue_.push_back( UpdateLaterAction( action,
		particleSystem,
		dTime ) );
}


/**
 *	Update all of the matrix swarm actions that have been queued for this frame.
 *	Then clear the queue ready for next frame.
 */
/*static*/ void ParticleSystemAction::flushLateUpdates()
{
	BW_GUARD;

	for (UpdateLaterQueue::iterator itr =
		s_updateLaterQueue_.begin();
		itr != s_updateLaterQueue_.end();
		++itr)
	{
		UpdateLaterAction& tracker = (*itr);
		MF_ASSERT( tracker.action_ != NULL );
		MF_ASSERT( tracker.particleSystem_ != NULL );
		tracker.action_->lateExecute( (*tracker.particleSystem_),
			tracker.dTime_ );
	}
	s_updateLaterQueue_.clear();
}

/**
 *	Remove all entries for this action. (from the action's destructor)
 */
/*static*/ void ParticleSystemAction::removeLateUpdates(
	const ParticleSystemAction* action )
{
	BW_GUARD;

	for (size_t i = 0; i < s_updateLaterQueue_.size();)
	{
		if (s_updateLaterQueue_[i].action_ == action)
		{
			s_updateLaterQueue_.erase_swap_last( i );
		}
		else
		{
			++i;
		}
	}
}

/**
 *	Remove all entries for this ParticleSystem. (from ParticleSystem destructor)
 */
/*static*/ void ParticleSystemAction::removeLateUpdates(
	const ParticleSystem* system )
{
	BW_GUARD;

	for (size_t i = 0; i < s_updateLaterQueue_.size();)
	{
		if (s_updateLaterQueue_[i].particleSystem_ == system)
		{
			s_updateLaterQueue_.erase_swap_last( i );
		}
		else
		{
			++i;
		}
	}
}

/**
 *	Clear all entries.
 */
/*static*/ void ParticleSystemAction::clearLateUpdates()
{
	BW_GUARD;
	s_updateLaterQueue_.clear();
}

const BW::string & ParticleSystemAction::typeToName(int type)
{
	switch(type)
	{
	case PSA_SOURCE_TYPE_ID:
		{
			return SourcePSA::nameID_;
		}
	case PSA_SINK_TYPE_ID:
		{
			return SinkPSA::nameID_;
		}
	case PSA_BARRIER_TYPE_ID:
		{
			return BarrierPSA::nameID_;
		}
	case PSA_FORCE_TYPE_ID:
		{
			return ForcePSA::nameID_;
		}
	case PSA_STREAM_TYPE_ID:
		{
			return StreamPSA::nameID_;
		}
	case PSA_JITTER_TYPE_ID:
		{
			return JitterPSA::nameID_;
		}
	case PSA_SCALAR_TYPE_ID:
		{
			return ScalerPSA::nameID_;
		}
	case PSA_TINT_SHADER_TYPE_ID:
		{
			return TintShaderPSA::nameID_;
		}
	case PSA_NODE_CLAMP_TYPE_ID:
		{
			return NodeClampPSA::nameID_;
		}
	case PSA_ORBITOR_TYPE_ID:
		{
			return OrbitorPSA::nameID_;
		}
	case PSA_FLARE_TYPE_ID:
		{
			return FlarePSA::nameID_;
		}
	case PSA_COLLIDE_TYPE_ID:
		{
			return CollidePSA::nameID_;
		}
	case PSA_MATRIX_SWARM_TYPE_ID:
		{
			return MatrixSwarmPSA::nameID_;
		}
	case PSA_MAGNET_TYPE_ID:
		{
			return MagnetPSA::nameID_;
		}
	case PSA_SPLAT_TYPE_ID:
		{
			return SplatPSA::nameID_;
		}
	default:
		{
			static BW::string errorName("Unknown");
			return errorName;
		}
	}
}


int ParticleSystemAction::nameToType(const BW::string &name)
{
	if (name == SourcePSA::nameID_)
	{
		return PSA_SOURCE_TYPE_ID;
	}
	else if (name == SinkPSA::nameID_)	
	{
		return PSA_SINK_TYPE_ID;
	}
	else if (name == BarrierPSA::nameID_)
	{
		return PSA_BARRIER_TYPE_ID;
	}
	else if (name == ForcePSA::nameID_)
	{
		return PSA_FORCE_TYPE_ID;
	}
	else if (name == StreamPSA::nameID_)
	{
		return PSA_STREAM_TYPE_ID;
	}
	else if (name == JitterPSA::nameID_)
	{
		return PSA_JITTER_TYPE_ID;
	}
	else if (name == ScalerPSA::nameID_)
	{
		return PSA_SCALAR_TYPE_ID;
	}
	else if (name == TintShaderPSA::nameID_)
	{
		return PSA_TINT_SHADER_TYPE_ID;
	}
	else if (name == NodeClampPSA::nameID_)
	{
		return PSA_NODE_CLAMP_TYPE_ID;
	}
	else if (name == OrbitorPSA::nameID_)
	{
		return PSA_ORBITOR_TYPE_ID;
	}
	else if (name == FlarePSA::nameID_)
	{
		return PSA_FLARE_TYPE_ID;
	}
	else if (name == CollidePSA::nameID_)
	{
		return PSA_COLLIDE_TYPE_ID;
	}
	else if (name == MatrixSwarmPSA::nameID_)
	{
		return PSA_MATRIX_SWARM_TYPE_ID;
	}
	else if (name == MagnetPSA::nameID_)
	{
		return PSA_MAGNET_TYPE_ID;
	}
	else if (name == SplatPSA::nameID_)
	{
		return PSA_SPLAT_TYPE_ID;
	}
	else
	{
		return PSA_TYPE_ID_MAX;
	}
}


ParticleSystemActionPtr ParticleSystemAction::createActionOfType(int type)
{
	switch(type)
	{
	case PSA_SOURCE_TYPE_ID:
		{
			return new SourcePSA();
		}
	case PSA_SINK_TYPE_ID:
		{
			return new SinkPSA();
		}
	case PSA_BARRIER_TYPE_ID:
		{
			return new BarrierPSA();
		}
	case PSA_FORCE_TYPE_ID:
		{
			return new ForcePSA();
		}
	case PSA_STREAM_TYPE_ID:
		{
			return new StreamPSA();
		}
	case PSA_JITTER_TYPE_ID:
		{
			return new JitterPSA();
		}
	case PSA_SCALAR_TYPE_ID:
		{
			return new ScalerPSA();
		}
	case PSA_TINT_SHADER_TYPE_ID:
		{
			return new TintShaderPSA();
		}
	case PSA_NODE_CLAMP_TYPE_ID:
		{
			return new NodeClampPSA();
		}
	case PSA_ORBITOR_TYPE_ID:
		{
			return new OrbitorPSA();
		}
	case PSA_FLARE_TYPE_ID:
		{
			return new FlarePSA();
		}
	case PSA_COLLIDE_TYPE_ID:
		{
			return new CollidePSA();
		}
	case PSA_MATRIX_SWARM_TYPE_ID:
		{
			return new MatrixSwarmPSA();
		}
	case PSA_MAGNET_TYPE_ID:
		{
			return new MagnetPSA();
		}
	case PSA_SPLAT_TYPE_ID:
		{
			return new SplatPSA();
		}
	default:
		{
			return NULL;
		}
	}
}


/**
 *	This static method returns all of the resources that will be required to
 *	create the particle system action.  It is designed so that the resources
 *	can be loaded in the loading thread before constructing the particle system
 */
/*static*/ void ParticleSystemAction::prerequisitesOfType(DataSectionPtr pSect,
												BW::set<BW::string>& output)
{
	int type = nameToType( pSect->sectionName() );

	switch(type)
	{
	case PSA_SOURCE_TYPE_ID:
		{
			SourcePSA::prerequisites(pSect, output);
			break;
		}
	case PSA_SINK_TYPE_ID:
		{
			SinkPSA::prerequisites(pSect, output);
			break;
		}
	case PSA_BARRIER_TYPE_ID:
		{
			BarrierPSA::prerequisites(pSect, output);
			break;
		}
	case PSA_FORCE_TYPE_ID:
		{
			ForcePSA::prerequisites(pSect, output);
			break;
		}
	case PSA_STREAM_TYPE_ID:
		{
			StreamPSA::prerequisites(pSect, output);
			break;
		}
	case PSA_JITTER_TYPE_ID:
		{
			JitterPSA::prerequisites(pSect, output);
			break;
		}
	case PSA_SCALAR_TYPE_ID:
		{
			ScalerPSA::prerequisites(pSect, output);
			break;
		}
	case PSA_TINT_SHADER_TYPE_ID:
		{
			TintShaderPSA::prerequisites(pSect, output);
			break;
		}
	case PSA_NODE_CLAMP_TYPE_ID:
		{
			NodeClampPSA::prerequisites(pSect, output);
			break;
		}
	case PSA_ORBITOR_TYPE_ID:
		{
			OrbitorPSA::prerequisites(pSect, output);
			break;
		}
	case PSA_FLARE_TYPE_ID:
		{			
			FlarePSA::prerequisites(pSect, output);
			break;
		}
	case PSA_COLLIDE_TYPE_ID:
		{
			CollidePSA::prerequisites(pSect, output);
			break;
		}
	case PSA_MATRIX_SWARM_TYPE_ID:
		{
			MatrixSwarmPSA::prerequisites(pSect, output);
			break;
		}
	case PSA_MAGNET_TYPE_ID:
		{
			MagnetPSA::prerequisites(pSect, output);
			break;
		}
	case PSA_SPLAT_TYPE_ID:
		{
			SplatPSA::prerequisites(pSect, output);
			break;
		}
	default:
		{			
		}
	}
}


template < typename Serialiser >
void ParticleSystemAction::serialise( const Serialiser & s ) const
{
	BW_PARITCLE_SERIALISE( s, ParticleSystemAction, delay );
	BW_PARITCLE_SERIALISE( s, ParticleSystemAction, minimumAge );
	BW_PARITCLE_SERIALISE( s, ParticleSystemAction, name );
}


void ParticleSystemAction::load( DataSectionPtr pSect )
{
	BW_GUARD;
	serialise( LoadFromDataSection< ParticleSystemAction >( pSect, this ) );
	loadInternal( pSect );
}


void ParticleSystemAction::save( DataSectionPtr pSect ) const
{
	BW_GUARD;
	pSect = pSect->newSection( nameID() );
	serialise( SaveToDataSection< ParticleSystemAction >( pSect, this ) );
	saveInternal( pSect );
}


void ParticleSystemAction::clone( ParticleSystemAction * dest ) const
{
	BW_GUARD;
	serialise( CloneObject< ParticleSystemAction >( this, dest ) );
}


// -----------------------------------------------------------------------------
// Section: Script Convertors implementation.
//
// These convert between ParticleSystemActions and PyParticleSystemActions.  We
// need these so PyParticleSystem can use an ActionsHolder that directly refers
// to its ParticleSystem's actions.
// -----------------------------------------------------------------------------

/**
 *	This method converts from a C++ action to a PyAction.  It constructs
 *	a new wrapper around the C++ action and returns it.
 */
PyObject * Script::getData( const ParticleSystemAction * pDerived )					
{
	BW_GUARD;
	PyParticleSystemActionPtr pyAction =
		PyParticleSystemAction::createPyAction(
			const_cast<ParticleSystemAction *>(pDerived) );

	return Script::getData(												
		static_cast< const PyObject * >( &*pyAction ) );					
}																		


/**
 *	This method retrieves converts a PyAction to a C++ action, and sets
 *	the rpDerived smart pointer reference.
 */
int Script::setData( PyObject * pObject,								
	SmartPointer<ParticleSystemAction> & rpDerived, const char * varName )				
{																		
	BW_GUARD;
	if (pObject == Py_None)												
	{																	
		rpDerived = NULL;												
	}																	
	else if (PyParticleSystemAction::Check( pObject ))									
	{																	
		rpDerived = static_cast<PyParticleSystemAction *>(pObject)->pAction();
	}																	
	else																
	{																	
		PyErr_Format( PyExc_TypeError,									
			"%s must be set to a PyParticleSystemAction or None", varName );		
		return -1;														
	}																	

	return 0;															
}																		


/**
 *	This method converts from a C++ action to a PyAction.  It constructs
 *	a new wrapper around the C++ action and returns it.
 */
PyObject * Script::getData( ConstSmartPointer<ParticleSystemAction> pDerived )			
{																		
	BW_GUARD;
	PyParticleSystemActionPtr pyAction =
		PyParticleSystemAction::createPyAction(
			const_cast<ParticleSystemAction*>(pDerived.getObject()) );

	return Script::getData(												
		static_cast< const PyObject * >( &*pyAction ) );
}																	


// -----------------------------------------------------------------------------
// Section: The Python Interface to the PyParticleSystemAction.
// -----------------------------------------------------------------------------

PY_TYPEOBJECT( PyParticleSystemAction )

PY_BEGIN_ATTRIBUTES( PyParticleSystemAction )
	/*~ attribute PyParticleSystemAction.delay
	 *	The delay in time from creation of the PyParticleSystemAction before its
	 *	ParticleSystemAction affects the particle system. Default value is 0.
	 *	@type Float.
	 */
	PY_ATTRIBUTE( delay )
	/*~ attribute PyParticleSystemAction.minimumAge
	 *	The minimum time a particle must have existed before being affected by
	 *	the ParticleSystemAction. Default value is 0.
	 *	@type Float.
	 */
	PY_ATTRIBUTE( minimumAge )
	/*~ attribute PyParticleSystemAction.typeID
	 *	The type ID of the ParticleSystemAction, PSA_FLARE_TYPE_ID (see
	 *	ParticleSystemAction).
	 *	@type Integer as enumerated value.
	 */
	PY_ATTRIBUTE( typeID )
PY_END_ATTRIBUTES()

PY_BEGIN_METHODS( PyParticleSystemAction )
PY_END_METHODS()

PyParticleSystemActionPtr PyParticleSystemAction::createPyAction(ParticleSystemAction * pAction)
{
	BW_GUARD;
	if (!pAction)
	{
		return NULL;
	}

	int type = pAction->typeID();

	switch(type)
	{
	case PSA_SOURCE_TYPE_ID:
		{
			SourcePSA* source = static_cast<SourcePSA*>(&*pAction);
			PyParticleSystemActionPtr pAction( new PySourcePSA(source), true );
			return pAction;
		}
	case PSA_SINK_TYPE_ID:
		{
			SinkPSA* sink = static_cast<SinkPSA*>(&*pAction);
			PyParticleSystemActionPtr pAction(new PySinkPSA(sink), true);
			return pAction;
		}
	case PSA_BARRIER_TYPE_ID:
		{
			BarrierPSA* barrier = static_cast<BarrierPSA*>(&*pAction);
			PyParticleSystemActionPtr pAction(new PyBarrierPSA(barrier), true);
			return pAction;
		}
	case PSA_FORCE_TYPE_ID:
		{
			ForcePSA* force = static_cast<ForcePSA*>(&*pAction);
			PyParticleSystemActionPtr pAction(new PyForcePSA(force), true);
			return pAction;
		}
	case PSA_STREAM_TYPE_ID:
		{
			StreamPSA* stream = static_cast<StreamPSA*>(&*pAction);
			PyParticleSystemActionPtr pAction(new PyStreamPSA(stream), true);
			return pAction;
		}
	case PSA_JITTER_TYPE_ID:
		{
			JitterPSA* jitter = static_cast<JitterPSA*>(&*pAction);
			PyParticleSystemActionPtr pAction(new PyJitterPSA(jitter), true);
			return pAction;
		}
	case PSA_SCALAR_TYPE_ID:
		{
			ScalerPSA* scaler = static_cast<ScalerPSA*>(&*pAction);
			PyParticleSystemActionPtr pAction(new PyScalerPSA(scaler), true);
			return pAction;
		}
	case PSA_TINT_SHADER_TYPE_ID:
		{
			TintShaderPSA* tintShader = static_cast<TintShaderPSA*>(&*pAction);
			PyParticleSystemActionPtr pAction(new PyTintShaderPSA(tintShader), true);
			return pAction;
		}
	case PSA_NODE_CLAMP_TYPE_ID:
		{
			NodeClampPSA* nodeClamp = static_cast<NodeClampPSA*>(&*pAction);
			PyParticleSystemActionPtr pAction(new PyNodeClampPSA(nodeClamp), true);
			return pAction;
		}
	case PSA_ORBITOR_TYPE_ID:
		{
			OrbitorPSA* orbitor = static_cast<OrbitorPSA*>(&*pAction);
			PyParticleSystemActionPtr pAction(new PyOrbitorPSA(orbitor), true);
			return pAction;
		}
	case PSA_FLARE_TYPE_ID:
		{
			FlarePSA* flare = static_cast<FlarePSA*>(&*pAction);
			PyParticleSystemActionPtr pAction(new PyFlarePSA(flare), true);
			return pAction;
		}
	case PSA_COLLIDE_TYPE_ID:
		{
			CollidePSA* collide = static_cast<CollidePSA*>(&*pAction);
			PyParticleSystemActionPtr pAction(new PyCollidePSA(collide), true);
			return pAction;
		}
	case PSA_MATRIX_SWARM_TYPE_ID:
		{
			MatrixSwarmPSA* matrixSwarm = static_cast<MatrixSwarmPSA*>(&*pAction);
			PyParticleSystemActionPtr pAction(new PyMatrixSwarmPSA(matrixSwarm), true);
			return pAction;
		}
	case PSA_MAGNET_TYPE_ID:
		{
			MagnetPSA* magnet = static_cast<MagnetPSA*>(&*pAction);
			PyParticleSystemActionPtr pAction(new PyMagnetPSA(magnet), true);
			return pAction;
		}
	case PSA_SPLAT_TYPE_ID:
		{
			SplatPSA* splat = static_cast<SplatPSA*>(&*pAction);
			PyParticleSystemActionPtr pAction(new PySplatPSA(splat), true);
			return pAction;
		}
	default:
		{
			return NULL;
		}
	}
}

PY_SCRIPT_CONVERTERS( PyParticleSystemAction )

BW_END_NAMESPACE

// particle_system_action.cpp
