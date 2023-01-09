#ifndef CHUNK_GRID_SIZE_HPP
#define CHUNK_GRID_SIZE_HPP

#include "cstdmf/bw_namespace.hpp"

BW_BEGIN_NAMESPACE

/**
 *	Legacy value for spaces without a chunkSize parameter set (i.e.
 *	spaces creted in an older version of BigWorld)
 *	Only needs to be changed if you are migrating spaces from an
 *	older version of BigWorld, and will use the AssetProcessor to
 *	insert the appropriate chunkSize record.
 */
const float DEFAULT_GRID_RESOLUTION = 100.f;
extern float g_defaultGridResolution; // exposed to binary-only server code

BW_END_NAMESPACE

#endif // CHUNK_GRID_SIZE_HPP
