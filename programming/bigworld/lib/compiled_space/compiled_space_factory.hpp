#ifndef COMPILED_SPACE_FACTORY_HPP
#define COMPILED_SPACE_FACTORY_HPP

#include "compiled_space/forward_declarations.hpp"
#include "resmgr/datasection.hpp"

#include "space/client_space.hpp"
#include "space/client_space_factory.hpp"
#include "space/space_interfaces.hpp"

#include "compiled_space.hpp"

BW_BEGIN_NAMESPACE

class AssetClient;

class PyOmniLight;
class PySpotLight;

BW_END_NAMESPACE

namespace BW {

namespace CompiledSpace {

class COMPILED_SPACE_API CompiledSpaceFactory : public IClientSpaceFactory
{
public:
	CompiledSpaceFactory( AssetClient* pAssetClient );

	virtual ClientSpace * createSpace( SpaceID spaceID ) const;
	virtual IEntityEmbodimentPtr createEntityEmbodiment( 
		const ScriptObject& object ) const;
	virtual IOmniLightEmbodiment * createOmniLightEmbodiment(
		const PyOmniLight & pyOmniLight ) const;
	virtual ISpotLightEmbodiment * createSpotLightEmbodiment(
		const PySpotLight & pySpotLight ) const;

	void mappingCreator( CompiledSpace::MappingCreator func );

protected:
	AssetClient* pAssetClient_;
	CompiledSpace::MappingCreator mappingCreator_;
};

} // namespace CompiledSpace
} // namespace BW


#endif // COMPILED_SPACE_FACTORY_HPP
