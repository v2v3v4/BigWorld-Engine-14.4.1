#ifndef SCRIPT_PLAYER_HPP
#define SCRIPT_PLAYER_HPP

#include "entity.hpp"
#include "physics.hpp"
#include "particle/py_meta_particle_system.hpp"


BW_BEGIN_NAMESPACE

class MatrixProvider;
typedef SmartPointer<MatrixProvider> MatrixProviderPtr;
class EntityPhotonOccluder;

namespace Moo {
typedef SmartPointer< class RenderTarget > RenderTargetPtr;
class DrawContext;
};

/**
 *	This class stores a pointer to the player entity, and other
 *	player specific objects such as the current physics controller.
 */
class ScriptPlayer : public InitSingleton<ScriptPlayer>
{
public:
	ScriptPlayer();
	virtual ~ScriptPlayer();

	virtual bool doInit();
	virtual bool doFini();

	bool setPlayer( Entity * newPlayer );

	Entity *	i_entity()		{ return pEntity_; }

	static MatrixProviderPtr camTarget();

	void updateWeatherParticleSystems( class PlayerAttachments & pa );

	static Entity *		entity()
	{
		return pInstance() ? instance().i_entity() : NULL;
	}

	Chunk * findChunk();

	bool drawPlayer( Moo::DrawContext& drawContext );

private:

	struct AttachedPS
	{
		SmartPointer<PyModelNode> pNode_;
		PyMetaParticleSystem* pAttachment_;
	};

	BW::vector< AttachedPS > attachedPSs_;

	Entity		* pEntity_;

	EntityPhotonOccluder * playerFlareCollider_;
};

BW_END_NAMESPACE

#endif // SCRIPT_PLAYER_HPP
