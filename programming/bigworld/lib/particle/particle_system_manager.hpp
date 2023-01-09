#ifndef PARTICLE_SYSTEM_MANAGER_HPP
#define PARTICLE_SYSTEM_MANAGER_HPP

#include "cstdmf/init_singleton.hpp"
#include "moo/managed_effect.hpp"
#include "moo/vertex_declaration.hpp"
#include "particle/meta_particle_system.hpp" // needed for cache


BW_BEGIN_NAMESPACE

class SpaceManager;
class ClientSpace;

/**
 *	This class is the main singleton of the particle system library
 *  for now, it simply holds references to expensive assets known
 *  to the C++ code and the relevant configuration strings for them
 */
class ParticleSystemManager : public InitSingleton<ParticleSystemManager>, public Moo::DeviceCallback
{
public:
	ParticleSystemManager();
	~ParticleSystemManager() {}

	// Provide our own fini() method since both our parent classes have their own
	// fini() method, which is ambiguous
	static bool fini() { return InitSingleton<ParticleSystemManager>::fini(); }

	MetaParticleSystemCache & loader();

	// Resources for MeshParticleRenderer
	const BW::string & meshSolidEffect() const;
	const BW::string & meshAdditiveEffect() const;
	const BW::string & meshBlendedEffect() const;

	///	@name DeviceCallback.
	//@{
	void	createUnmanagedObjects();
	void	deleteUnmanagedObjects();
	//@}

	// Resources for PointSprintParticleRenderer
	/**
	 *	This is the Get-Accessor for PointSprintParticleRenderer's
	 *  vertex shader.
	 *
	 *	@return	A pointer to a VertexShader for PointSprintParticleRenderer
	 */
	DX::VertexShader* pPointSpriteVertexShader()
		{ return pPointSpriteVertexShader_; }

	/**
	 *	This is the Get-Accessor for PointSprintParticleRenderer's
	 *  vertex declaration
	 *
	 *	@return	A pointer to a VertexDeclaration for PointSprintParticleRenderer
	 */
	Moo::VertexDeclaration* pPointSpriteVertexDeclaration()
		{ return pPointSpriteVertexDeclaration_; }

	/**
	 *	This is the Get-Accessor for the variable active. If this is set to
	 *	false, the particle systems are no longer updated or drawn. This
	 *	effectively disables them.
	 *
	 *	@return	The flag indicating whether all particle systems are active or not.
	 */
	const bool active( void ) const { return active_; }

	/**
	 *	This is the Set-Accessor for the variable active.
	 *
	 *	@param flag		The new value for active/inactive particle systems.
	 */
	void active( bool flag ) { active_ = flag; }

	/**
	 *	This is the Get-Accessor for the variable windVelocity. This tells
	 *	all particle systems what the current wind velocity is for the frame.
	 *
	 *	@return	Current wind velocity as a 3D vector.
	 */
	const Vector3 & windVelocity( void ) const { return windVelocity_; }

	/**
	 *	This is the Set-Accessor for the variable windVelocity. All game
	 *	systems have to update this on their update methods in order to keep
	 *	every particle system updated on the wind velocity in the world.
	 *
	 *	@param newWindVelocity	The current wind velocity for the frame.
	 */
	void windVelocity( const Vector3 & newWindVelocity )
		{ windVelocity_ = newWindVelocity; }

private:

	void onSpaceDestroyed( SpaceManager *, ClientSpace * );

	// MeshParticleRenderer's ManagedEffects
	Moo::ManagedEffectPtr meshSolidManagedEffect_;
	Moo::ManagedEffectPtr meshAdditiveManagedEffect_;
	Moo::ManagedEffectPtr meshBlendedManagedEffect_;

	// PointSpriteRenderer's vertex resources
	// These are not managed, so they are handled by the DeviceCallback
	// subclassing
	DX::VertexShader* pPointSpriteVertexShader_;
	Moo::VertexDeclaration* pPointSpriteVertexDeclaration_;

	/// A windVelocity to apply to all particle systems
	Vector3 windVelocity_;

	/// Cache loaded systems
	MetaParticleSystemCache cache_;

	/// Activate or deactivate all particle systems
	bool active_;

protected:
	bool doInit();
	bool doFini();
};

BW_END_NAMESPACE

#endif // PARTICLE_SYSTEM_MANAGER_HPP
