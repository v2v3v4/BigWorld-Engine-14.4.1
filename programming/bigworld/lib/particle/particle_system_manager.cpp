#include "pch.hpp"
#include "particle_system_manager.hpp"
#include "cstdmf/guard.hpp"
#include "resmgr/auto_config.hpp"
#include "moo/effect_manager.hpp"
#include "moo/render_context.hpp"
#include "space/space_manager.hpp"


BW_BEGIN_NAMESPACE

// These need to be static as they have to be initialised before
// AutoConfig::configureAllFrom is called during app startup.
static AutoConfigString s_meshSolidEffect("system/meshParticles/solid");
static AutoConfigString s_meshAdditiveEffect("system/meshParticles/additive");
static AutoConfigString s_meshBlendedEffect("system/meshParticles/blended");

// Implementation of the singleton static pointer.
BW_INIT_SINGLETON_STORAGE( ParticleSystemManager );

/**
 *	Constructor
 */
ParticleSystemManager::ParticleSystemManager():
	pPointSpriteVertexShader_( NULL ),
	pPointSpriteVertexDeclaration_( NULL ),
	active_( true )
{
}


/**
 *	This method initialises any objects that require initialisation in the
 *	library.
 *
 *	@return		True if init went on without errors, false otherwise.
 */
bool ParticleSystemManager::doInit()
{
	BW_GUARD;
	// Load the ManagedEffects for MeshParticleRenderer
	meshSolidManagedEffect_ = Moo::EffectManager::instance().get( s_meshSolidEffect );
	meshAdditiveManagedEffect_ = Moo::EffectManager::instance().get( s_meshAdditiveEffect );
	meshBlendedManagedEffect_ = Moo::EffectManager::instance().get( s_meshBlendedEffect );

	// Load vertex shader stuff for SpriteParticleRenderer
	pPointSpriteVertexDeclaration_ = Moo::VertexDeclaration::get( "particle" );

	// Create any unmanaged resources
	this->createUnmanagedObjects();

	SpaceManager::instance().spaceDestroyed().add< ParticleSystemManager, 
		&ParticleSystemManager::onSpaceDestroyed >( this );

	return true;
}


/**
 *	This method finalises any objects that require finalises in the
 *	library, releasing resources.
 *
 *	@return		True if fini went on without errors, false otherwise.
 */
bool ParticleSystemManager::doFini()
{
	BW_GUARD;

	SpaceManager::instance().spaceDestroyed().remove< ParticleSystemManager, 
		&ParticleSystemManager::onSpaceDestroyed >( this );

	// Clear and disable further caching of particle systems
	cache_.shutdown();

	// Free the ManagedEffects for MeshParticleRenderer
	meshSolidManagedEffect_ = NULL;
	meshAdditiveManagedEffect_ = NULL;
	meshBlendedManagedEffect_ = NULL;

	// Free vertex shader stuff for SpriteParticleRenderer
	// There's no way to free Moo::VertexDeclarations...

	// Release any unmanaged resources
	this->deleteUnmanagedObjects();

	return true;
}

MetaParticleSystemCache & ParticleSystemManager::loader()
{
	return cache_;
}


void ParticleSystemManager::onSpaceDestroyed( SpaceManager *, ClientSpace * )
{
	// clear cache when space changes
	cache_.clear();
}


/**
 *  This method provides the name of the effect to be applied to
 *  solid MeshParticleRenderer materials
 *
 *  @return		const BW::string reference
 */
const BW::string & ParticleSystemManager::meshSolidEffect() const
{
	BW_GUARD;
	return s_meshSolidEffect;
}

/**
 *  This method provides the name of the effect to be applied to
 *  additive MeshParticleRenderer materials
 *
 *  @return		const BW::string reference
 */
const BW::string & ParticleSystemManager::meshAdditiveEffect() const
{
	BW_GUARD;
	return s_meshAdditiveEffect;
}

/**
 *  This method provides the name of the effect to be applied to
 *  blended MeshParticleRenderer materials
 *
 *  @return		const BW::string reference
 */
const BW::string & ParticleSystemManager::meshBlendedEffect() const
{
	BW_GUARD;
	return s_meshBlendedEffect;
}

/**
 *	This method creates any resources that need to be recreated when
 *  our device is recreated.
 */
void ParticleSystemManager::createUnmanagedObjects()
{
	BW_GUARD;
	if (pPointSpriteVertexShader_ != NULL)
		this->deleteUnmanagedObjects();

	const char* pPointSpriteVertexShaderName = "shaders/xyzdp/particles_pc/0d0o0s.vso";
	BinaryPtr pVertexShader = BWResource::instance().rootSection()->readBinary(
		pPointSpriteVertexShaderName );

	if( pVertexShader )
	{
		// Try to create the shader.
		HRESULT result = Moo::rc().device()->CreateVertexShader( (DWORD*)pVertexShader->data(), &pPointSpriteVertexShader_ );
		if ( FAILED( result ) )
		{
			CRITICAL_MSG( "ParticleSystemManager::createUnmanagedObjects: "
				"Couldn't create vertexshader from %s: 0x%08lx\n",
				pPointSpriteVertexShaderName, result );
		}
	}
}


/**
 *	This method frees any resources that need to be recreated when
 *  our device is recreated.
 */
void ParticleSystemManager::deleteUnmanagedObjects()
{
	BW_GUARD;
	if ( pPointSpriteVertexShader_ )
	{
		pPointSpriteVertexShader_->Release();
		pPointSpriteVertexShader_ = NULL;
	}
}

BW_END_NAMESPACE

// particle_system_manager.cpp
