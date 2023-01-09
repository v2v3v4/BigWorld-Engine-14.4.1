#ifndef SPEEDTREE_VERTEX_TYPES_HPP
#define SPEEDTREE_VERTEX_TYPES_HPP

#include "moo/index_buffer_wrapper.hpp"
#include "moo/vertex_buffer_wrapper.hpp"
#include "moo/moo_math.hpp"
#include "moo/texture_manager.hpp"
#include "moo/texture_streaming_manager.hpp"
#include "moo/texture_usage.hpp"

#include <d3dx9math.h>


BW_BEGIN_NAMESPACE

using Moo::IndexBufferWrapper;
using Moo::VertexBufferWrapper;
using Moo::BaseTexturePtr;

namespace speedtree
{

/**
 * This class represents a single allocation slot in the vertex/index 
 * buffer used by the speedtree.
 *
 * This is used to allow dynamic streaming (in/out) of different tree types.
 */
class BufferSlot : public SafeReferenceCount
{
public:
	BufferSlot(): start_(0), count_(0), next_(NULL), prev_(NULL) {}

	~BufferSlot() 
	{
		// Slot should not be active when destroyed.
		MF_ASSERT( !next_ );
		MF_ASSERT( !prev_ );
	}

	void init( int start, int count )
	{
		start_ = start;
		count_ = count;
	}

	void erased()
	{
		if ( next_ )
		{
			next_->updateStart( *this );
			next_->setPrev( prev_ );
		}

		if ( prev_ )
			prev_->setNext( next_ );

		clear();
	}

	void setPrev(const SmartPointer<BufferSlot>& prev )
	{ 
		MF_ASSERT_DEBUG( prev.getObject() != this );
		prev_ = prev;
	}
	void setNext(const SmartPointer<BufferSlot>& next )
	{
		MF_ASSERT_DEBUG( next.getObject() != this );
		next_ = next;
	}
	SmartPointer<BufferSlot> next() const { return next_; }
	SmartPointer<BufferSlot> prev() const { return prev_; }
	int start() const { return start_; }
	int count() const { return count_; }	
	void count(int count) { count_ = count; }
private:
	void updateStart( BufferSlot& deletedSlot )
	{
		start_ -= deletedSlot.count();
		if (next_)
			next_->updateStart( deletedSlot );			
	}

	void clear()
	{
		start_=0;
		count_=0;
		next_ = NULL;
		prev_ = NULL;
	}

	int start_;
	int count_;
	SmartPointer<BufferSlot> next_, prev_;
};

/**
 * This class is used to store the vertex/index buffer for the various
 * speedtree renderable components.
 *
 * The usage is monitored via the BufferSlot class to allow streaming.
 */
template < class T >
class STVector : public BW::vector< T >
{
public:
	STVector() : dirty_(true), first_(0), last_(0) {}
	//TODO: consider using the STL for this allocation list management
	void eraseSlot(const SmartPointer<BufferSlot>& slot )
	{
		if (slot->count())
		{
			typename BW::vector< T >::iterator it =
				this->begin() + slot->start();
			this->erase( it, it + slot->count() );
		}
		
		if (last_ == slot)
		{
			last_ = slot->prev();
			if (last_)
				last_->setNext( NULL );
		}

		if (first_ == slot)
		{
			first_ = slot->next();
			if (first_)
				first_->setPrev( NULL );
		}

		if (first_ == last_)
		{
			last_ = NULL;
		}
		slot->erased();
		dirty(true);
	}

	SmartPointer<BufferSlot> newSlot( int size = 0 )
	{
		SmartPointer<BufferSlot> newSlot = new BufferSlot();
		MF_ASSERT( this->size() <= INT_MAX );
		newSlot->init( ( int )this->size(), size );
		addSlot( newSlot );
		return newSlot;
	}

	void dirty(bool val) { dirty_ = val; }
	bool dirty() const { return dirty_; }

private:
	void addSlot(const SmartPointer<BufferSlot>& slot )
	{
		if (first_ == NULL)
		{
			first_ = slot;
			last_ = NULL;

			first_->setNext( NULL );
			first_->setPrev( NULL );
		}			
		else
		{
			if (last_ == NULL)
			{
				last_ = slot;
				first_->setNext( last_ );
				last_->setPrev( first_ );
			}
			else
			{
				slot->setPrev( last_ );
				last_->setNext( slot );
				last_ = slot;
			}				
			slot->setNext( NULL );
		}
		dirty(true);
	}

	bool dirty_;

	SmartPointer<BufferSlot> first_, last_;
};

//-- To guaranty one byte aligned packing.
#pragma pack(push, 1)

//-- The vertex type used to render branches and fronds.
//--------------------------------------------------------------------------------------------------
struct BranchVertex
{
	Vector3 pos_;		 //-- POSITION
	Vector3 normal_;	 //-- NORMAL
	Vector4 tcWindInfo_; //-- TEXCOORD0
	Vector3 tangent_;	 //-- TANGENT

	static DWORD fvf()	{ return 0; }

	static bool getVertexFormat( Moo::VertexFormat & format );
};

//-- The vertex type used to render leaves. 
//--------------------------------------------------------------------------------------------------
struct LeafVertex
{
	Vector3 pos_;			//-- POSITION
	Vector3 normal_;		//-- NORMAL
	Vector4 tcWindInfo_;	//-- TEXCOORD0
	Vector4 rotGeomInfo_;	//-- TEXCOORD1
	Vector3 extraInfo_;		//-- TEXCOORD2
	Vector3 tangent_;		//-- TANGENT 
	Vector2 pivotInfo_;		//-- BINORMAL

	static DWORD fvf()	{ return 0; }

	static bool getVertexFormat( Moo::VertexFormat & format );
};

//-- The vertex type used to render normal billboards (non optimized).
//--------------------------------------------------------------------------------------------------
struct BillboardVertex
{
	Vector3 pos_;			//-- POSITION
	Vector3 lightNormal_;	//-- NORMAL
	Vector3 alphaNormal_;	//-- TEXCOORD0
	Vector2 tc_;			//-- TEXCOORD1
	Vector3 binormal_;		//-- BINORMAL
	Vector3 tangent_;		//-- TANGENT

	static DWORD fvf()	{ return 0; }

	static bool getVertexFormat( Moo::VertexFormat & format );

	static float s_minAlpha_;
	static float s_maxAlpha_;
};

#pragma pack(pop)

//--------------------------------------------------------------------------------------------------
typedef Moo::VertexBufferWrapper< BillboardVertex > BBVertexBuffer;

template< typename VertexType >
struct TreePartRenderData
{
	TreePartRenderData() :
		uvSpaceDensity_( Moo::ModelTextureUsage::INVALID_DENSITY )
	{
		verts_ = NULL;

		RESOURCE_COUNTER_ADD( ResourceCounters::DescriptionPool("Speedtree/TreePartRenderData", (uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER),
			sizeInBytes(), 0 )
	}
	~TreePartRenderData()
	{
		verts_ = NULL;
		RESOURCE_COUNTER_SUB( ResourceCounters::DescriptionPool("Speedtree/TreePartRenderData", (uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER),
			sizeInBytes(), 0 )
	}
	typedef VertexBufferWrapper< VertexType >   VertexBufferWrapper;
	typedef SmartPointer< VertexBufferWrapper > VertexBufferPtr;
	typedef SmartPointer< IndexBufferWrapper >  IndexBufferPtr;
	typedef BW::vector< IndexBufferPtr >       IndexBufferVector;
	typedef STVector< uint32 >					IndexVector;
	typedef STVector< VertexType >				VertexVector;

	void unload( VertexVector& verts, IndexVector& indices );

	//-- holds offsets in the index and vertex buffer for each unique LOD of the tree part.
	//----------------------------------------------------------------------------------------------
	struct LodData
	{
		SmartPointer<BufferSlot> index_;
	};

	/**
	 *	return the bytes used by this structure excluding vertex/index buffer
	 */
	size_t sizeInBytes() const
	{
		return (sizeof( *this ) + lod_.capacity() * sizeof( lod_[0] ));
	}
	typedef BW::vector< LodData > LodDataVector;

	/**
	 *	Checks there is some vertex data in the LODs.
	 */
	bool checkVerts() const
	{
		return (verts_ && verts_->count()>0);
	}

	BaseTexturePtr  diffuseMap_;
#if SPT_ENABLE_NORMAL_MAPS
	BaseTexturePtr  normalMap_;
#endif // SPT_ENABLE_NORMAL_MAPS
	LodDataVector   lod_;
	float uvSpaceDensity_;

	SmartPointer<BufferSlot> verts_;

	// serialisation
	int			size() const;
	char*		write(char * pdata, const VertexVector& verts, const IndexVector& indices) const;
	const char*	read(const char * data, VertexVector& verts, IndexVector& indices);
};

// Render Data;
typedef TreePartRenderData< BranchVertex >    BranchRenderData;
typedef TreePartRenderData< LeafVertex >      LeafRenderData;
typedef TreePartRenderData< BillboardVertex > BillboardRenderData;

} // namespace speedtree

BW_END_NAMESPACE

#endif //SPEEDTREE_VERTEX_TYPES_HPP
