#ifndef CLIENT_SPACE_FACTORY_HPP
#define CLIENT_SPACE_FACTORY_HPP

#include "forward_declarations.hpp"

namespace BW
{

class ScriptObject;
class WaterSceneRenderer;

class IClientSpaceFactory
{
public:
	virtual ~IClientSpaceFactory();
	
	virtual ClientSpace * createSpace( SpaceID spaceID ) const = 0;
	virtual IEntityEmbodimentPtr createEntityEmbodiment( 
		const ScriptObject& object ) const = 0;
	virtual IOmniLightEmbodiment * createOmniLightEmbodiment(
		const PyOmniLight & pyOmniLight ) const = 0;
	virtual ISpotLightEmbodiment * createSpotLightEmbodiment(
		const PySpotLight & pySpotLight ) const = 0;
};

} // namespace BW

#endif // CLIENT_SPACE_FACTORY_HPP
