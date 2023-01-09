#include "pch.hpp"

#include "linear_path.hpp"
#include "cstdmf/guard.hpp"


BW_BEGIN_NAMESPACE

LinearPath::LinearPath( const Vector3& start, const Vector3& end ) :
	start_( start ),
	end_( end ),
	dir_( end - start )
{
}


float LinearPath::length() const
{
	BW_GUARD;
	return dir_.length();
}


Vector3 LinearPath::getPointAt( float t ) const
{
	BW_GUARD;
	return start_ + dir_ * t;
}


Vector3 LinearPath::getDirection( float t ) const
{
	BW_GUARD;
	return dir_;
}


const Vector3& LinearPath::getStart() const
{
	BW_GUARD;
	return start_;
}


const Vector3& LinearPath::getEnd() const
{
	BW_GUARD;
	return end_;
}

BW_END_NAMESPACE

// linear_path.cpp
