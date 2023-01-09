#include "pch.hpp"

#include "chunk_grid_size.hpp"

BW_BEGIN_NAMESPACE

/* This is here so that binary-only server objects that
 * reference this value can get it at link-time rather
 * than directly importing the constant provided in
 * chunk_grid_size.hpp
*/
float g_defaultGridResolution = DEFAULT_GRID_RESOLUTION;

BW_END_NAMESPACE

// chunk_grid_size.cpp
