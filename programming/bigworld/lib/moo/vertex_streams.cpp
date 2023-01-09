
#include "pch.hpp"
#include "vertex_streams.hpp"

#include "cstdmf/debug.hpp"
#include "render_context.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/quick_file_writer.hpp"
#include "vertex_formats.hpp"

DECLARE_DEBUG_COMPONENT2( "Moo", 0 )


BW_BEGIN_NAMESPACE

namespace Moo
{

// static stream names
const char * InstancingStream::TYPE_NAME = "instancing";
const char * UV2Stream::TYPE_NAME = "uv2";
const char * ColourStream::TYPE_NAME = "colour";

VertexStreamDesc::VertexStreamDesc()
: streamIndex_( 0 )
, defaultFormat_( NULL )
{}

const VertexFormat * VertexStreamDesc::defaultVertexFormat() const
{
	if (!defaultFormat_ && defaultFormatName_.length())
	{
		defaultFormat_ = VertexFormatCache::get( defaultFormatName_ );
	}
	return defaultFormat_;
}

const VertexStreamDesc::StreamDescs & VertexStreamDesc::getKnownStreams()
{
	static StreamDescs s_sectionDescs;
	static bool firstTime = true;
	if (firstTime)
	{
		// Add all known vertex stream file section types
		s_sectionDescs.push_back( VertexStreamDesc::create<UV2Stream>() );
		s_sectionDescs.push_back( VertexStreamDesc::create<ColourStream>() );

		// Not a stream loaded from disk, so commented out
		//s_sectionDescs.push_back( VertexStreamDesc::create<InstancingStream>() );

		firstTime = false;
	}
	return s_sectionDescs;
}

void VertexStream::setFormat( 
	const VertexFormat & vertexFormat, const uint32 streamIndex )
{
	MF_ASSERT( vertexFormat.streamCount() > streamIndex );
	stride_ = vertexFormat.streamStride( streamIndex );
	formatName_ = vertexFormat.name().to_string();
	stream_ = streamIndex;
}

VertexStream VertexStream::create( 
	const VertexFormat & vertexFormat, const uint32 streamIndex )
{
	VertexStream vs;
	vs.setFormat( vertexFormat, streamIndex );
	return vs;
}


/**
 * rearrange the order of the stream to match the given new->old remapping.
 */

void VertexStream::remapVertices( uint32 offset, uint32 nVerticesBeforeMapping, const BW::vector< uint32 >& mappingNewToOld )
{
	// get format of current verts
	const VertexFormat * format = VertexFormatCache::get( formatName_ );
	MF_ASSERT( format );
	const uint32 vertexSize = format->streamStride( stream_ );
	MF_ASSERT( vertexSize );

	const uint32 remapCount = static_cast<uint32>(mappingNewToOld.size());

	// data before + remapping count + after
	// (offset) + (remapCount) + (count_ - (offset +_ nVerticesBeforeMapping))
	const uint32 totalVerts = remapCount + count_ - nVerticesBeforeMapping;
	const uint32 totalSize = totalVerts * vertexSize;

	// setup src and dest pointers
	const uint8 * pOrigData = reinterpret_cast<const uint8*>(pData_->data());
	uint8 * pDestData = reinterpret_cast<uint8*>(bw_malloc( totalSize ));

	// copy the data before this
	memcpy( pDestData, pOrigData, vertexSize * offset);

	// copy/remap the actual data meant to be remapped.
	const RawElementAccessor<uint8> origData( (void*)pOrigData, totalSize, 0, vertexSize );
	RawElementAccessor<uint8> destData( pDestData, totalSize, 0, vertexSize );
	for (uint32 i = 0; i < remapCount; ++i)
	{	
		uint8 * dstVertex = &destData[i + offset];
		const uint8 * srcVertex = &origData[ offset + mappingNewToOld[i] ];
		memcpy( dstVertex, srcVertex, vertexSize );
	}

	// now copy the last bit of data after our target range.
	{
		uint8 * dst = pDestData + vertexSize * (offset + remapCount);
		const uint8 * src = pOrigData + vertexSize * (offset + nVerticesBeforeMapping);
		const uint32 count = count_ - (offset + nVerticesBeforeMapping);
		memcpy( dst, src, count * vertexSize );
	}

	// finally, rebuild the binary.
	QuickFileWriter qfw;
	qfw.write( pDestData, totalSize );
	pData_ = qfw.output();
	count_ = static_cast<uint32>(totalVerts);

	bw_free( pDestData );
}


/**
 * load the data from the data block and create the vertex buffer.
 */

void VertexStream::load( const void* data, uint32 nVertices, const VertexFormat * sourceFormat )
{
	// create the buffer
	DWORD usageFlag = rc().mixedVertexProcessing() ? D3DUSAGE_SOFTWAREPROCESSING : 0;

	if (!sourceFormat)
	{
		// Use the default format for this stream type.
		sourceFormat = VertexFormatCache::get( formatName_ );
	}
	MF_ASSERT( sourceFormat );

	// get required vertex formats and set up a vertex format conversion
	const BW::StringRef targetDevice( VertexDeclaration::getTargetDevice() );
	const VertexFormat * targetFormat = 
		VertexFormatCache::getTarget( *sourceFormat, targetDevice );
	VertexFormat::ConversionContext vfConversion( targetFormat, sourceFormat );

	// if conversion is invalid, vertex format definitions are not
	// correctly set up for these types.
	MF_ASSERT(vfConversion.isValid());

	uint32 vertexSize = vfConversion.dstVertexSize( stream_ );
	Moo::VertexBuffer pVertexBuffer;
	HRESULT res = E_FAIL;

	// Create the vertex buffer
	if (SUCCEEDED( res = pVertexBuffer.create( 
		nVertices * vertexSize, usageFlag, 0,
		D3DPOOL_MANAGED ) ))
	{
		// Set up the smart pointer to the vertex buffer
		vertexBuffer_ = pVertexBuffer;

		// Perform conversion to vertex buffer
		SimpleVertexLock vertexLock( vertexBuffer_, 0, nVertices * vertexSize, 0 );
		if (vertexLock)
		{
			bool converted = vfConversion.convertSingleStream( 
				vertexLock, stream_, data, stream_, nVertices );
			MF_ASSERT( converted );
		}
	}

	this->setFormat( *targetFormat, stream_ );
	count_ = nVertices;
}


/**
 * 
 */
StreamContainer::StreamContainer()
{
	pSoftwareDecl_ = VertexDeclaration::get( 
		VertexFormatCache::getFormatName<SoftwareVertType>() );

	pSoftwareDeclTB_ = VertexDeclaration::get( 
		VertexFormatCache::getFormatName<SoftwareVertTBType>() );
}


/**
 * Create the required declarations (combining where necessary).
 */
void StreamContainer::updateDeclarations( VertexDeclaration* pDecl )
{
	if (pSoftwareDecl_)
	{
		pSoftwareDecl_ = VertexDeclaration::combine( pSoftwareDecl_, pDecl );
	}

	if (pSoftwareDeclTB_)
	{
		pSoftwareDeclTB_ = VertexDeclaration::combine( pSoftwareDeclTB_, pDecl );
	}
}

} //namespace Moo

BW_END_NAMESPACE

// vertex_streams.cpp

