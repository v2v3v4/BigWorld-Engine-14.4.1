#include "pch.hpp"

#include "discrete_animation_channel.hpp"

#ifndef CODE_INLINE
#include "discrete_animation_channel.ipp"
#endif

#include "cstdmf/binaryfile.hpp"


BW_BEGIN_NAMESPACE

namespace Moo
{

DiscreteAnimationChannel::DiscreteAnimationChannel()
{
}

DiscreteAnimationChannel::~DiscreteAnimationChannel()
{
	BW_GUARD;
}

Matrix DiscreteAnimationChannel::result( float time ) const
{
	BW_GUARD;
	Matrix m;
	DiscreteKeyframes::const_iterator it = keyframes_.upper_bound( time );
	if (it != keyframes_.begin())
		--it;
	if (it != keyframes_.end())
		m = (const Matrix&)it->second;
	else
		XPMatrixIdentity( &m );

	return m;
}

void DiscreteAnimationChannel::addKey( float time, const Matrix& key )
{
	keyframes_[ time ] = key;
}

void DiscreteAnimationChannel::result( float time, Matrix& out ) const
{
	BW_GUARD;
	DiscreteKeyframes::const_iterator it = keyframes_.upper_bound( time );
	if (it != keyframes_.begin())
		--it;
	if (it != keyframes_.end())
		out = (const Matrix&)it->second;
}

/**
 *	Call this method when you have finished adding keys.
 *	Currently it is only necessary for memory accounting balancing.
 */
void DiscreteAnimationChannel::finalise()
{
	BW_GUARD;
}

void DiscreteAnimationChannel::preCombine( const AnimationChannel & rOther )
{
	BW_GUARD;
	Matrix m;

	for (DiscreteKeyframes::iterator it = keyframes_.begin();
		it != keyframes_.end();
		it++)
	{
		rOther.result( it->first, m );
		//it->second.preMultiply( m );
		Matrix our( (Matrix&)it->second );	// copy since it->second unaligned
		our.preMultiply( m );
		it->second = our;
	}
}

void DiscreteAnimationChannel::postCombine( const AnimationChannel & rOther )
{
	BW_GUARD;
	Matrix m;

	for (DiscreteKeyframes::iterator it = keyframes_.begin();
		it != keyframes_.end();
		it++)
	{
		rOther.result( it->first, m );
		//it->second.postMultiply( m );
		//it->second.preMultiply( m );
		Matrix our( (Matrix&)it->second );	// copy since it->second unaligned
		our.postMultiply( m );
		it->second = our;
	}
}



bool DiscreteAnimationChannel::load( BinaryFile & bf )
{
	BW_GUARD;
	if( !this->AnimationChannel::load( bf ) )
		return false;

	bf.readMap( keyframes_ );

	this->finalise();

	return !!bf;
}


bool DiscreteAnimationChannel::save( BinaryFile & bf ) const
{
	BW_GUARD;
	if( !this->AnimationChannel::save( bf ) )
		return false;

	bf.writeMap( keyframes_ );

	return !!bf;
}

DiscreteAnimationChannel::TypeRegisterer
	DiscreteAnimationChannel::s_rego_( DISCRETE_ANIMATION_CHANNEL, New );


std::ostream& operator<<(std::ostream& o, const DiscreteAnimationChannel& t)
{
	o << "DiscreteAnimationChannel\n";
	return o;
}

} // namespace Moo

BW_END_NAMESPACE

// discrete_animation_channel.cpp
