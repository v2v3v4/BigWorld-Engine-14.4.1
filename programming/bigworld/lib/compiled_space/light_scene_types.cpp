#include "pch.hpp"
#include "light_scene_types.hpp"

#include <math/matrix.hpp>

namespace BW {
namespace CompiledSpace {

	namespace LightSceneTypes
	{
		void OmniLight::transform( const Matrix & matrix )
		{
			position = matrix.applyPoint(position);
			MF_ASSERT(matrix.isUniformlyScaled());
			float scale = matrix.uniformScale();
			innerRadius *= scale;
			outerRadius *= scale;
		}

		void SpotLight::transform( const Matrix & matrix )
		{
			position = matrix.applyPoint(position);
			MF_ASSERT(matrix.isUniformlyScaled());
			float scale = matrix.uniformScale();
			innerRadius *= scale;
			outerRadius *= scale;
			{
				Matrix transNormal(matrix);
				if (transNormal.invert())
				{
					transNormal.transpose();
				}
				direction = transNormal.applyVector(direction);
			}
		}

		void PulseLight::transform( const Matrix & matrix )
		{
			omniLight.transform(matrix);
		}

	}

} // namespace CompiledSpace
} // namespace BW
