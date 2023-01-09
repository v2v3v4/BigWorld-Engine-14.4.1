#include "pch.hpp"

#include "deprecated_space_helpers.hpp"

#include <chunk/chunk_manager.hpp>
#include <chunk/chunk_space.hpp>

#include <space/space_manager.hpp>
#include <space/client_space.hpp>

namespace BW
{

// WOO!
static ClientSpacePtr s_cameraSpace( NULL );

void DeprecatedSpaceHelpers::cameraSpace( const ClientSpacePtr& space )
{
	s_cameraSpace = space;
}


ClientSpacePtr DeprecatedSpaceHelpers::cameraSpace()
{
	return s_cameraSpace;
}


} // namespace BW