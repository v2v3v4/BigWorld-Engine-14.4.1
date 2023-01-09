#include "pch.hpp"

#include "model_animation.hpp"

#include "math/matrix.hpp"
#include "moo/animation.hpp"


DECLARE_DEBUG_COMPONENT2( "Model", 0 )


BW_BEGIN_NAMESPACE

/**
 *	Constructor for Model's Animation
 */
ModelAnimation::ModelAnimation() : duration_( 0.f ), looped_( false )
{
	BW_GUARD;
}


/**
 *	Destructor for Model's Animation
 */
ModelAnimation::~ModelAnimation()
{
	BW_GUARD;
}


/**
 *	This function returns true if the Model::Animation is in a valid state.
 *	This function will not raise a critical error or assert if called but may
 *	commit ERROR_MSGs to the log.
 *
 *	@return	Returns true if the animation is in a valid state, otherwise false.
 *
 *	@see	NodefullModel::Animation::valid()
 */
bool ModelAnimation::valid() const
{
	return true;
}


/**
 *	@todo
 */
void ModelAnimation::tick( float dtime, float otime, float ntime )
{
	BW_GUARD;
}


/**
 *	@todo
 */
void ModelAnimation::flagFactor( int flags, Matrix & mOut ) const
{
	mOut.setIdentity();
}
const Matrix & ModelAnimation::flagFactorBit( int bit ) const
{
	return Matrix::identity;
}


/**
 *	@todo
 */
uint32 ModelAnimation::sizeInBytes() const
{
	return sizeof(*this);
}


/**
 *	@todo
 */
Moo::AnimationPtr ModelAnimation::getMooAnim()
{
	return NULL;
}

BW_END_NAMESPACE


// model_animation.cpp

