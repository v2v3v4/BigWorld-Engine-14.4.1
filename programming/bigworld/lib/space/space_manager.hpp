#ifndef SPACE_MANAGER_HPP
#define SPACE_MANAGER_HPP

#include "forward_declarations.hpp"
#include "space_dll.hpp"

#include "cstdmf/event.hpp"
#include "resmgr/datasection.hpp"

BW_BEGIN_NAMESPACE

class AssetClient;
class SpaceDataMappings;
class ScriptObject;

class SPACE_API SpaceManager
{
public:
	static const SpaceID LOCAL_SPACE_START = (1 << 30);

	SpaceManager();
	~SpaceManager();

	static SpaceManager & instance();

	void init();
	void fini();
	bool isReady();

	void factory( IClientSpaceFactory * pFactory );
	IClientSpaceFactory * factory();

	void tick( float dTime );
	void updateAnimations( float dTime );

	void addSpace( ClientSpace * pSpace );
	void delSpace( ClientSpace * pSpace );
	ClientSpace * space( SpaceID spaceID );
	ClientSpace * createSpace( SpaceID spaceID );
	ClientSpace * findOrCreateSpace( SpaceID spaceID );

	void clearSpaces();
	void clearLocalSpaces();

	bool isLocalSpace( SpaceID spaceID );

	IEntityEmbodimentPtr createEntityEmbodiment( const ScriptObject& object );
	IOmniLightEmbodiment * createOmniLightEmbodiment(
		const PyOmniLight & pyOmniLight );
	ISpotLightEmbodiment * createSpotLightEmbodiment(
		const PySpotLight & pySpotLight );

	typedef Event< SpaceManager, ClientSpace * > SpaceCreatedEvent;
	typedef Event< SpaceManager, ClientSpace * > SpaceDestroyedEvent;
	SpaceCreatedEvent::EventDelegateList& spaceCreated();
	SpaceDestroyedEvent::EventDelegateList& spaceDestroyed();

private:

	std::auto_ptr<IClientSpaceFactory> pFactory_;

	typedef BW::map< SpaceID, ClientSpace * > SpaceMap;
	SpaceMap spaces_;
	SpaceMap localSpaces_;

	SpaceCreatedEvent onSpaceCreated;
	SpaceDestroyedEvent onSpaceDestroyed;

	void clearSpaceMap( SpaceMap & map );
};

BW_END_NAMESPACE

#endif // SPACE_MANAGER_HPP
