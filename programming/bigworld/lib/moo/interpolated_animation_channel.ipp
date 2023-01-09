
#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif

namespace Moo
{
	INLINE
	void InterpolatedAnimationChannel::reduceKeyFrames( float angleError, float scaleError, float positionError )
	{
		reduceRotationKeys( angleError );
		reduceScaleKeys( scaleError );
		reducePositionKeys( positionError );
	}

}

/*interpolated_animation_channel.ipp*/
