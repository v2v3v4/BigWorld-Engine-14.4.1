#include "pch.hpp"

#include "compiled_space_factory.hpp"
#include "compiled_space.hpp"
#include "omni_light_embodiment.hpp"
#include "spot_light_embodiment.hpp"
#include "py_attachment_entity_embodiment.hpp"
#include "py_model_obstacle_entity_embodiment.hpp"

namespace BW {
namespace CompiledSpace {

// ----------------------------------------------------------------------------
CompiledSpaceFactory::CompiledSpaceFactory( AssetClient* pAssetClient ) : 
	pAssetClient_( pAssetClient ),
	mappingCreator_( NULL )
{

}


// ----------------------------------------------------------------------------
ClientSpace * CompiledSpaceFactory::createSpace( SpaceID spaceID ) const
{
	CompiledSpace* pSpace = new CompiledSpace( spaceID, pAssetClient_ );

	if (mappingCreator_)
	{
		pSpace->mappingCreator( mappingCreator_ );
	}

	return pSpace;
}


// ----------------------------------------------------------------------------
IEntityEmbodimentPtr CompiledSpaceFactory::createEntityEmbodiment(
	const ScriptObject& object ) const
{
	PyAttachmentPtr pAttachment = PyAttachmentPtr::create( object );
	if (pAttachment)
	{
		if (pAttachment->isAttached())
		{
			PyErr_Format( PyExc_TypeError, 
				"Embodiment must be set to an Attachment "
				"that is not attached elsewhere" );
			return NULL;
		}

		return IEntityEmbodimentPtr(
			new PyAttachmentEntityEmbodiment( pAttachment ) );
	}


	PyModelObstaclePtr pObstacle = PyModelObstaclePtr::create( object );
	if (pObstacle)
	{
		if (pObstacle->attached())
		{
			PyErr_Format( PyExc_TypeError,
				"Embodiment must be set to a PyModelObstacle "
				"that is not attached elsewhere" );
			return NULL;
		}

		return IEntityEmbodimentPtr(
			new PyModelObstacleEntityEmbodiment( pObstacle ) );
	}

	return NULL;
}


// ----------------------------------------------------------------------------
IOmniLightEmbodiment * CompiledSpaceFactory::createOmniLightEmbodiment(
	const PyOmniLight & pyOmniLight) const
{
	return new OmniLightEmbodiment( pyOmniLight );
}


// ----------------------------------------------------------------------------
ISpotLightEmbodiment * CompiledSpaceFactory::createSpotLightEmbodiment(
	const PySpotLight & pySpotLight) const
{
	return new SpotLightEmbodiment( pySpotLight );
}


// ----------------------------------------------------------------------------
void CompiledSpaceFactory::mappingCreator( CompiledSpace::MappingCreator func )
{
	mappingCreator_ = func;
}


} // namespace CompiledSpace
} // namespace BW
