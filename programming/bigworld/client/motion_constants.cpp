#include "pch.hpp"

#include "entity.hpp"
#include "motion_constants.hpp"
#include "physics.hpp"


BW_BEGIN_NAMESPACE

/// MotionConstants constructor
MotionConstants::MotionConstants( const Physics & physics )
{
	BW_GUARD;
	modelYaw = 0.f;
	modelPitch = 0.f;
	modelWidth = physics.modelWidth_;
	modelHeight = physics.modelHeight_;
	modelDepth = physics.modelDepth_;
	scrambleHeight = physics.scrambleHeight_;

	IEntityEmbodiment * pEmb = physics.pSlave_->pPrimaryEmbodiment();
	if (pEmb != NULL)
	{
		const Matrix& modelTransform = pEmb->worldTransform();
		modelYaw = atan2f( modelTransform.m[2][0], modelTransform.m[2][2] );

		const AABB & bb = pEmb->localBoundingBox();
		Vector3 bbSize( bb.maxBounds() - bb.minBounds() );
		if (!modelWidth) modelWidth  = bbSize[0];
		if (!modelHeight) modelHeight = bbSize[1];
		if (!modelDepth) modelDepth	= bbSize[2];
	}

	if (!modelWidth) modelWidth = 0.4f;
	if (!modelHeight) modelHeight = 1.8f;
	if (!modelDepth) modelDepth = 1.8f;
	if (!scrambleHeight) scrambleHeight = modelHeight/3.f;
}

BW_END_NAMESPACE

// motion_constants.cpp
