#include "pch.hpp"
#include "romp/romp_collider.hpp"

BW_BEGIN_NAMESPACE

/*static*/ float RompCollider::NO_GROUND_COLLISION = -1000000.0f;
/*static*/ RompCollider::CreationFunction RompCollider::s_defaultCreate = NULL;

RompColliderPtr RompCollider::createDefault( FilterType filter )
{
	MF_ASSERT( s_defaultCreate );
	return (*s_defaultCreate)( filter );
}

void RompCollider::setDefaultCreationFuction( 
	CreationFunction func )
{
	s_defaultCreate = func;
}

BW_END_NAMESPACE


// romp_collider.cpp

