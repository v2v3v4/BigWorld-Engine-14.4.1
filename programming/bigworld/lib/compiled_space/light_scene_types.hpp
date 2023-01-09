#ifndef COMPILED_LIGHT_SCENE_TYPES_HPP
#define COMPILED_LIGHT_SCENE_TYPES_HPP

#include "cstdmf/stdmf.hpp"
#include "cstdmf/fourcc.hpp"

#include "string_table_types.hpp"

#include <moo/moo_math.hpp>

BW_BEGIN_NAMESPACE
class Matrix;
BW_END_NAMESPACE

namespace BW {

namespace CompiledSpace {

	namespace LightSceneTypes
	{
		const FourCC FORMAT_MAGIC = FourCC( "BWLC" );
		const uint32 FORMAT_VERSION = 0x00000001;

		struct Header
		{
		};

		// We've had to redefine the light type structures so that they're
		// standard layout.  These structures only represent the data necessary
		// to (de)serialise the light information.  Other structures in Moo
		// will be used as the runtime form of the lights.
		struct OmniLight
		{
			Vector3 position;
			float innerRadius;
			float outerRadius;
			Moo::Colour colour;
			uint32 priority;
			void transform( const Matrix & matrix );
		};

		struct SpotLight
		{
			Vector3 position;
			Vector3 direction;
			float innerRadius;
			float outerRadius;
			float cosConeAngle;
			Moo::Colour colour;
			uint32 priority;
			void transform( const Matrix & matrix );
		};

		// A PulseLight is an animated OmniLight
		struct PulseLight
		{
			OmniLight omniLight;
			StringTableTypes::Index animationNameIdx;
			uint32 firstFrameIdx;
			uint32 numFrames;
			float duration;
			void transform( const Matrix & matrix );
		};
	}

} // namespace CompiledSpace
} // namespace BW


#endif // COMPILED_LIGHT_SCENE_TYPES_HPP
