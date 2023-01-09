#pragma warning (disable : 4530 )

#include "polyCont.hpp"

BW_BEGIN_NAMESPACE

PolyContainer::PolyContainer()
{
	v1_=v2_=v3_=tv1_=tv2_=tv3_=0;
}

PolyContainer::~PolyContainer()
{
}

PolyContainer::PolyContainer(const PolyContainer & pc)
{
	*this = pc;
}
PolyContainer& PolyContainer::operator=(const PolyContainer & pc)
{
	v1_ = pc.v1_;
	v2_ = pc.v2_;
	v3_ = pc.v3_;

	tv1_ = pc.tv1_;
	tv2_ = pc.tv2_;
	tv3_ = pc.tv3_;

	return *this;
}

void PolyContainer::setVIndices( int a, int b, int c )
{
	v1_ = a;
	v2_ = b;
	v3_ = c;
}
void PolyContainer::setTCIndices( int a, int b, int c )
{
	tv1_ = a;
	tv2_ = b;
	tv3_ = c;
}

int PolyContainer::getV1( void )
{
	return v1_;
}
int PolyContainer::getV2( void )
{
	return v2_;

}
int PolyContainer::getV3( void )
{
	return v3_;
}

int PolyContainer::getTV1( void )
{
	return tv1_;
}
int PolyContainer::getTV2( void )
{
	return tv2_;
}
int PolyContainer::getTV3( void )
{
	return tv3_;
}

BW_END_NAMESPACE

// polycont.cpp
