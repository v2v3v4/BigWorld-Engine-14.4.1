#ifndef TERRAIN_VERTEX_BUFFER_HPP
#define TERRAIN_VERTEX_BUFFER_HPP

#include "moo/vertex_buffer.hpp"


BW_BEGIN_NAMESPACE

namespace Terrain
{

class AliasedHeightMap;
class GridVertexBuffer;

/**
 *	This class generates and handles the vertex buffer for one 
 *	level in a lod chain. It generates two heights per vertex, one
 *	for the current lod level and one for the next lower lod level,
 *	this is to aid geo-morphing when doing geo mip-mapping
 *
 */
class TerrainVertexBuffer : public SafeReferenceCount
{
public:
	TerrainVertexBuffer();
	~TerrainVertexBuffer();

	static void generate( const AliasedHeightMap* hm, const AliasedHeightMap* previousLOD, 
							uint32 resolutionX, uint32 resolutionZ,
							BW::vector< Vector2 > & vertices );
	bool init( const Vector2 * vertices, uint32 resolutionX, uint32 resolutionZ, uint32 usage );
	bool set();

	Moo::VertexBuffer getBuffer() { return pVertexBuffer_; }

private:

	Moo::VertexBuffer				pVertexBuffer_;
	SmartPointer<GridVertexBuffer>	pGridBuffer_;

	TerrainVertexBuffer( const TerrainVertexBuffer& );
	TerrainVertexBuffer& operator=( const TerrainVertexBuffer& );
};

typedef SmartPointer<TerrainVertexBuffer> TerrainVertexBufferPtr;

} // namespace Terrain

BW_END_NAMESPACE

#endif // TERRAIN_VERTEX_BUFFER_HPP
