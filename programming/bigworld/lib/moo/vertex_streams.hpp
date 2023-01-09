#ifndef VERTEX_STREAMS_HPP
#define VERTEX_STREAMS_HPP

#include "moo_math.hpp"
#include "moo_dx.hpp"
#include "vertex_buffer.hpp"
#include "cstdmf/smartpointer.hpp"
#include "vertex_declaration.hpp"
#include "vertex_formats.hpp"
#include "vertex_format_cache.hpp"


BW_BEGIN_NAMESPACE

namespace Moo
{

/**
 * This struct encapsulates the various section attributes for
 * a section type as a run time structure. 
 */
struct VertexStreamDesc
{
	BW::StringRef			fileSectionName_;
	BW::StringRef			defaultFormatName_;
	uint32					streamIndex_;

	VertexStreamDesc();
	const VertexFormat *	defaultVertexFormat() const;

	typedef BW::vector<VertexStreamDesc> StreamDescs;
	static const StreamDescs & getKnownStreams();

	/**
	 * This create function returns a VertexStreamDesc for the
	 * given type.
	 */
	template <typename StreamType>
	static const VertexStreamDesc create()
	{
		// default stream type
		VertexStreamDesc desc;
		desc.streamIndex_ = StreamType::STREAM_NUMBER;
		desc.fileSectionName_ = StreamType::TYPE_NAME;
		desc.defaultFormatName_ = VertexFormatCache::getFormatName<StreamType>();

		return desc;
	}

private:
	// cached on first query
	mutable const VertexFormat *	defaultFormat_;
};



//-- Stream used for instancing. It is not a stream which loaded from disc, it exist only in
//--	 run-time and we use it here just to clarify that we use appropriate stream number and this
//--	 stream can't be used on for other purposes.
struct InstancingStream
{
	static const char *				TYPE_NAME;
	static const uint32				STREAM_NUMBER = 9;
	static const uint32				ELEMENT_SIZE = 64;
};


/**
 * Stream details for the second UV data.
 */
struct UV2Stream
{
	static const char *				TYPE_NAME;
	static const uint32				STREAM_NUMBER = 10;
	typedef Vector2					TYPE;
};

/**
 * Stream details for the vertex colour data.
 */
struct ColourStream
{	
	static const char *				TYPE_NAME;
	static const uint32				STREAM_NUMBER = 11;
	typedef DWORD					TYPE;
};

// definition name mappings.
#if BW_GPU_VERTEX_PACKING_SET == 1
	DECLARE_VERTEX_TYPE_DEFINITION_NAME( UV2Stream, uv2 );
	DECLARE_VERTEX_TYPE_DEFINITION_NAME( ColourStream, colour );
	DECLARE_VERTEX_TYPE_DEFINITION_NAME( InstancingStream, instancing );

#else
	BW_STATIC_ASSERT( false, Vertex_stream_definitions_not_defined_for_selected_vertex_packing_set );

#endif


/**
 * Vertex stream base.
 */
class VertexStream
{
private:
	VertexStream() :
		stream_( 0 )
		, stride_( 0 )
		, offset_( 0 )
	{
	}

	// set members using a vertex format
	void setFormat( const VertexFormat & vertexFormat, const uint32 streamIndex );

public:
	static VertexStream create( const VertexFormat & vertexFormat, const uint32 streamIndex );

	template <class StreamType>
	static VertexStream create()
	{
		const VertexFormat * vertexFormat = VertexFormatCache::get<StreamType>();
		MF_ASSERT( vertexFormat );
		return create( *vertexFormat, StreamType::STREAM_NUMBER );
	}

	template <class StreamType>
	static VertexStream create( BinaryPtr pData, uint32 count )
	{
		VertexStream vs = create<StreamType>();
		vs.pData_ = pData;
		vs.count_ = count;
		return vs;
	}

	void remapVertices( uint32 offset, uint32 nVerticesBeforeMapping, const BW::vector< uint32 >& mappingNewToOld );
	void load( const void* data, uint32 nVertices, const VertexFormat * sourceFormat );

	void assign( const VertexBuffer& vb, uint32 offset, uint32 stride, 
		uint32 size )
	{
		vertexBuffer_ = vb;
		offset_ = offset;
		stride_ = stride;
		count_ = size / stride;
	}

	void set() const
	{
		if (vertexBuffer_.valid())
		{
			vertexBuffer_.set( stream_, offset_, stride_ );
		}
	}
	void release()
	{
		vertexBuffer_.release();
	}

	void preload()
	{
		if (vertexBuffer_.valid())
			vertexBuffer_.addToPreloadList();
	}
	
	BinaryPtr data() 
	{
		return pData_; 
	}

	const BinaryPtr data() const
	{
		return pData_; 
	}

	VertexBuffer vertexBuffer() const
	{
		return vertexBuffer_;
	}

	uint32 offset() const
	{
		return offset_;
	}

	uint32 stride() const
	{
		return stride_;
	}

	uint32 stream() const
	{
		return stream_;
	}

	uint32 size() const
	{
		return count_ * stride_;
	}

protected:
	BW::string formatName_;
	uint32 stream_;
	uint32 stride_;
	uint32 offset_;
	Moo::VertexBuffer  vertexBuffer_;

protected:
	template<typename T>
	void loadInternal( const void* data, uint32 nVertices, const VertexFormat * sourceFormat );

	template<typename T>
	void remap( uint32 offset, uint32 nVerticesBeforeMapping, const BW::vector< uint32 >& mappingNewToOld );


	uint32 count_;
	BinaryPtr pData_;
};


/**
 * Holder of all the streams for a vertex block.
 */
class StreamContainer
{
public:
	typedef VertexXYZNUV SoftwareVertType;
	typedef VertexXYZNUVTBPC SoftwareVertTBType;

	StreamContainer();

	void release()
	{
		for (uint i=0; i<streamData_.size(); ++i)
		{
			streamData_[i].release();
		}
	}
	void set() const
	{
		for (uint i=0; i<streamData_.size(); ++i)
		{
			streamData_[i].set();
		}
	}
	void preload()
	{
		for (uint i=0; i<streamData_.size(); ++i)
		{
			streamData_[i].preload();
		}
	}

	void updateDeclarations( VertexDeclaration* pDecl );

	size_t size() const
	{
		return streamData_.size();
	}

	BW::vector<VertexStream> streamData_;
	VertexDeclaration* pSoftwareDecl_;
	VertexDeclaration* pSoftwareDeclTB_;
};

} // namespace Moo

BW_END_NAMESPACE

#endif
/*vertex_streams.hpp*/
