#ifndef SPACE_FORWARD_DECLARATIONS_HPP
#define SPACE_FORWARD_DECLARATIONS_HPP

//-----------------------------------------------------------------------------
// Standard inclusions
#include "cstdmf/stdmf.hpp"
#include "cstdmf/smartpointer.hpp"

//-----------------------------------------------------------------------------
// Forward declarations

namespace BW
{

typedef int32 SpaceID;
const SpaceID NULL_SPACE_ID = SpaceID( 0 );

// Deprecated. Use NULL_SPACE_ID instead.
const SpaceID NULL_SPACE = NULL_SPACE_ID;

typedef uint32 SpaceTypeID;

class ClientSpace;
class SpaceManager;
class IClientSpaceFactory; 
class IEntityEmbodiment;
class PyOmniLight;
class PySpotLight;
class IOmniLightEmbodiment;
class ISpotLightEmbodiment;

typedef SmartPointer< IEntityEmbodiment > IEntityEmbodimentPtr;
typedef SmartPointer< ClientSpace > ClientSpacePtr;

} // namespace BW

#endif // SPACE_FORWARD_DECLARATIONS_HPP
