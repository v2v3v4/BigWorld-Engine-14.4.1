

#pragma warning (disable : 4530 )

#include "vertcont.hpp"

BW_BEGIN_NAMESPACE

VertexContainer::VertexContainer()
{
	v_ = tv_ = 0;
}

VertexContainer::VertexContainer(int v, int tv)
{
	v_ = v; tv_ = tv;
}


VertexContainer::~VertexContainer()
{
}

VertexContainer::VertexContainer(const VertexContainer &vc)
{
	*this = vc;
}
VertexContainer& VertexContainer::operator = (const VertexContainer &vc)
{
	v_ = vc.v_;
	tv_ = vc.tv_;

	return *this;
}
bool VertexContainer::operator == (const VertexContainer &vc) const
{
	return ( ( v_ == vc.v_ ) && ( tv_ == vc.tv_ ) );
}

int VertexContainer::getV(void)
{
	return v_;
}
int VertexContainer::getTV(void)
{
	return tv_;
}

void VertexContainer::setV( int i)
{
	v_ = i;
}
void VertexContainer::setTV( int i)
{
	tv_ = i;
}

BW_END_NAMESPACE

/*vertcont.cpp*/
