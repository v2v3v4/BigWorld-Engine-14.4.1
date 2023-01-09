#ifndef TERRAIN_INDEX_BUFFER_HPP
#define TERRAIN_INDEX_BUFFER_HPP

#include "moo/forward_declarations.hpp"
#include "moo/device_callback.hpp"
#include "moo/index_buffer.hpp"
#include "cstdmf/static_array.hpp"


BW_BEGIN_NAMESPACE

namespace Moo
{
	class EffectMaterial;
}

namespace Terrain
{

// This structure holds morph ranges for the main block and for subblock (next
// lower lod).
struct MorphRanges
{
	MorphRanges()
	{
		main_.setZero();
		subblock_.setZero();
	}

	Vector2 main_;
	Vector2 subblock_;
};

// This array holds neighbour masks for main block at index 0, and the four sub 
// blocks after that ( five elements total ).
typedef StaticArray<uint8, 5>	 NeighbourMasks;


// forward declaration
class TerrainIndexBuffer;
typedef SmartPointer<TerrainIndexBuffer> TerrainIndexBufferPtr;


/**
 *	This class generates and handles the index buffer for 
 *	one detail level for the terrain.
 *	It also creates the degenerate triangles used between this
 *	lod level and the next lower lod level.
 *
 */
class TerrainIndexBuffer : public Moo::DeviceCallback, public SafeReferenceCount
{
public:
	// These are used to specify degenerate triangle usage
	enum 
	{
		DIRECTION_POSITIVEX = 0x01,
		DIRECTION_NEGATIVEX = 0x02,
		DIRECTION_POSITIVEZ = 0x04,
		DIRECTION_NEGATIVEZ = 0x08
	};

	~TerrainIndexBuffer();

	void createManagedObjects();
	void deleteManagedObjects();
	bool setIndices();

	void draw(
		const Moo::EffectMaterialPtr& pMaterial,	
					const Vector2&			morphRanges,
					const NeighbourMasks&	neighbourMasks,
		uint8 subBlockMask
		);

	static TerrainIndexBufferPtr get( uint32 quadCountX, uint32 quadCountZ );
	static void release( TerrainIndexBufferPtr & ptr );

private:
	

	// Ways to order the triangles
	enum TriangleListOrder
	{
		TLO_TILES_ROWS = 0,
		TLO_TILES_SWIZZLED
	};

	TerrainIndexBuffer( uint32 quadCountX, uint32 quadCountZ );

	template <class IndexType>
	void	generateIndices(TriangleListOrder order, IndexType* pOut, 
							uint32 bufferSize ) const;
	template <class IndexType>
	void	generateDegenerates(IndexType xStart, IndexType zStart, 
								IndexType xEnd,  IndexType zEnd,	
								IndexType rowSize, IndexType*& pBuffer ) const;

	uint32 quadCountX_;
	uint32 quadCountZ_;

	uint32 indexCount_;
	uint32 subBlockDegIndexCount_;

	Moo::IndexBuffer	pIndexBuffer_;

	typedef BW::map< uint64, TerrainIndexBufferPtr > IndexBufferMap;
	static  IndexBufferMap s_buffers_;
	static SimpleMutex s_mutex_;

	TerrainIndexBuffer( const TerrainIndexBuffer& );
	TerrainIndexBuffer& operator=( const TerrainIndexBuffer& );
};


} // namespace Terrain

BW_END_NAMESPACE

#endif // TERRAIN_INDEX_BUFFER_HPP
