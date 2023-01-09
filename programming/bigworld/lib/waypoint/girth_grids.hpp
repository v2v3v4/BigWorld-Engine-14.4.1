#ifndef GIRTH_GRIDS_HPP
#define GIRTH_GRIDS_HPP

#include "cstdmf/bw_vector.hpp"


BW_BEGIN_NAMESPACE

class GirthGridList;

struct GirthGrid
{
	float				girth;
	GirthGridList * 	grid;
};

typedef BW::vector< GirthGrid > GirthGrids;

BW_END_NAMESPACE

#endif // GIRTH_GRIDS

