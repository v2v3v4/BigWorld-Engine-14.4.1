#pragma once
#include "vertex_formats.hpp"

BW_BEGIN_NAMESPACE

void beginZBIASDraw(float bias);
void endZBIASDraw();

typedef Moo::VertexXYZNUV CPUSpotVertex;
typedef Moo::VertexXYZNUVPC GPUSpotVertex;


void calculateNormals(BW::vector< CPUSpotVertex > &vertices);

BW_END_NAMESPACE
