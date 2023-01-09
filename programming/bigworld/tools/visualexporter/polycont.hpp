#ifndef POLYCONT_HPP
#define POLYCONT_HPP

#include "cstdmf/bw_namespace.hpp"
#include "cstdmf/bw_vector.hpp"

BW_BEGIN_NAMESPACE

typedef BW::vector< class PolyContainer > PCVector;

class PolyContainer
{
public:
	PolyContainer();
	~PolyContainer();
	PolyContainer(const PolyContainer&);
	PolyContainer& operator=(const PolyContainer&);

	void setVIndices( int a, int b, int c );
	void setTCIndices( int a, int b, int c );

	int getV1( void );
	int getV2( void );
	int getV3( void );

	int getTV1( void );
	int getTV2( void );
	int getTV3( void );

private:
	int v1_, v2_, v3_;
	int tv1_, tv2_, tv3_;
};


BW_END_NAMESPACE

#endif
/*polyCont.hpp*/
