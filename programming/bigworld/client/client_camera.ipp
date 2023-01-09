// client_camera.ipp

#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif


INLINE Entity* Targeting::hasAnAutoAimTarget( float& autoAimTargetDistance,
	float& autoAimTargetAngle,
	bool useFriction,
	bool wantHorizontalAngle )
{
	BW_GUARD;
	return entityPicker_.hasAnAutoAimTarget( autoAimTargetDistance,
		autoAimTargetAngle,
		useFriction,
		wantHorizontalAngle );
}

// client_camera.ipp
