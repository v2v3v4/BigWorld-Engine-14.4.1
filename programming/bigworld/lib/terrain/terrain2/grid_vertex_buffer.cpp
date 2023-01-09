#include "pch.hpp"
#include "grid_vertex_buffer.hpp"

BW_BEGIN_NAMESPACE

namespace Terrain
{

// -----------------------------------------------------------------------------
// Section: GridVertexBuffer
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
GridVertexBuffer::GridVertexBuffer() :
	resolutionX_( 0 ),
	resolutionZ_( 0 )
{
}


/**
 *	Destructor.
 */
GridVertexBuffer::~GridVertexBuffer()
{
	del( this );
}

/*
 *	This method inits the grid vertex buffer
 *	The buffer is filled with two heights per vertex, the x grid position
 *	and the z grid position, both values between 0 and 1
 *	@param resolutionX the x resolution
 *	@param resolutionZ the z resolution
 *	@return true if successful
 */
bool GridVertexBuffer::init( uint16 resolutionX, uint16 resolutionZ )
{
	BW_GUARD;
	bool res = false;
	if (Moo::rc().device())
	{
		uint32 nVertices = uint32(resolutionX) * uint32(resolutionZ);
		pVertexBuffer_.release();
		resolutionX_ = resolutionX;
		resolutionZ_ = resolutionZ;
		// Creeate the vertex buffer
		if (SUCCEEDED(pVertexBuffer_.create( sizeof( Vector2 ) * nVertices,
					D3DUSAGE_WRITEONLY, 0, D3DPOOL_MANAGED ) ) )
		{
			Moo::VertexLock<Vector2> vl( pVertexBuffer_ );
			if (vl)
			{
				float z = 0.f;
				float zi = 1.f / float(resolutionZ - 1);
				int index = 0;
				for (uint32 zind = 0; zind < resolutionZ; zind++)
				{
					float x = 0.f;
					float xi = 1.f / float(resolutionX - 1);
					for( uint32 xind = 0; xind < resolutionX; xind++)
					{
						vl[index].x = x;
						vl[index].y = z;
						x += xi;
						++index;
					}
					z += zi;
				}
				res = true;
			}
			// Add the buffer to the preload list so that it can get uploaded
			// to video memory
			pVertexBuffer_.addToPreloadList();
		}
	}
	return res;
}

/*
 *	This method creates a token for the map from the two grid resolution values.
 *	This token contains the x resolution in the upper 16 bits and the z resolution
 *	in the lower 16 bits.
 *	@param resolutionX the x resolution
 *	@param resolutionZ the Z resolution
 *	@return the token
 */
uint32 GridVertexBuffer::token( uint16 resolutionX, uint16 resolutionZ )
{
	return (uint32( resolutionX ) << 16) | uint32(resolutionZ );
}

/**
 *	This static method returns the grid vertex buffer for the supplied resolution.
 *	@param resolutionX the x resolution
 *	@param resolutionZ the z resolution
 *	
 */
GridVertexBuffer* GridVertexBuffer::get( uint16 resolutionX, uint16 resolutionZ )
{
	BW_GUARD;
	SimpleMutexHolder smh( s_mutex_ );
	BW::map< uint32, GridVertexBuffer* >::iterator it
		= s_gridVertexBuffers_.find( token( resolutionX, resolutionZ ) );

	GridVertexBuffer* pRes = NULL;

	if (it != s_gridVertexBuffers_.end() )
	{
		pRes = it->second;
	}
	else
	{
		GridVertexBuffer* pBuf = new GridVertexBuffer;
		if (pBuf->init( resolutionX, resolutionZ ))
		{
			s_gridVertexBuffers_.insert( 
				std::make_pair( token( resolutionX, resolutionZ ), pBuf ) );
			pRes = pBuf;
		}
	}
	return pRes;
}

void GridVertexBuffer::del( GridVertexBuffer* pBuf )
{
	BW_GUARD;
	SimpleMutexHolder smh( s_mutex_ );
	BW::map< uint32, GridVertexBuffer* >::iterator it = 
		s_gridVertexBuffers_.find( 
		token( pBuf->resolutionX_, pBuf->resolutionZ_ ) );
	if (it != s_gridVertexBuffers_.end())
	{
		s_gridVertexBuffers_.erase( it );
	}
}

BW::map< uint32, GridVertexBuffer* > GridVertexBuffer::s_gridVertexBuffers_;
SimpleMutex GridVertexBuffer::s_mutex_;

} // namespace Terrain

BW_END_NAMESPACE

// grid_vertex_buffer.cpp
