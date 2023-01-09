#ifndef DEPRECATED_SPACE_HELPERS_HPP
#define DEPRECATED_SPACE_HELPERS_HPP

#include "forward_declarations.hpp"

namespace BW
{

/*
	All helpers listed here are deprecated. They are listed here as a means of
	documenting all code that needs to be changed to work with the new space
	system properly.

	Some of these functions as a result may not function properly with the new
	system. But they should function properly against the old chunk system.

	DO NOT USE these functions when writing new code.
*/

namespace DeprecatedSpaceHelpers
{
	void cameraSpace( const ClientSpacePtr& space );
	ClientSpacePtr cameraSpace();
}

} // namespace BW

#endif // DEPRECATED_SPACE_HELPERS_HPP
