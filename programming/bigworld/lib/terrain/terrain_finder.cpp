#include "pch.hpp"
#include "terrain_finder.hpp"

BW_BEGIN_NAMESPACE

using namespace Terrain;

TerrainFinder::Details::Details() :
    pBlock_(NULL),
    pMatrix_(NULL),
    pInvMatrix_(NULL)
{
}

/*virtual*/ TerrainFinder::~TerrainFinder()
{
}

BW_END_NAMESPACE

// terrain_finder.cpp

