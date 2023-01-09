#ifndef GRID_VERTEX_BUFFER_HPP
#define GRID_VERTEX_BUFFER_HPP

#include "moo/vertex_buffer.hpp"


BW_BEGIN_NAMESPACE

namespace Terrain
{

/**
 *	This class implements the grid vertex buffer.
 *	The grid vertex buffer contains vertices containing of two
 *	elements laid out on a grid, the values range from 0 to 1 on
 *	each axis spread over the dimensions of the grid.
 */
class GridVertexBuffer : public SafeReferenceCount
{
public:
	~GridVertexBuffer();

	Moo::VertexBuffer pBuffer() const { return pVertexBuffer_; };

	static GridVertexBuffer* get( uint16 resolutionX, uint16 resolutionZ );

private:

	static uint32 token( uint16 resolutionX, uint16 resolutionZ );
	static void del( GridVertexBuffer* pBuf );

	GridVertexBuffer();
	bool init( uint16 resolutionX, uint16 resolutionZ );

	Moo::VertexBuffer pVertexBuffer_;
	uint16 resolutionX_;
	uint16 resolutionZ_;

	GridVertexBuffer( const GridVertexBuffer& );
	GridVertexBuffer& operator=( const GridVertexBuffer& );

	static BW::map< uint32, GridVertexBuffer* > s_gridVertexBuffers_;
	static SimpleMutex s_mutex_;
};

typedef SmartPointer<GridVertexBuffer> GridVertexBufferPtr;

} // namespace Terrain

BW_END_NAMESPACE

#endif // GRID_VERTEX_BUFFER_HPP
