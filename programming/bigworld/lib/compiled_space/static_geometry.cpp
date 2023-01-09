#include "pch.hpp"

#include "static_geometry.hpp"
#include "string_table.hpp"

#include "cstdmf/string_builder.hpp"
#include "cstdmf/profiler.hpp"

#include "resmgr/bwresource.hpp"
#include "model/model.hpp"
#include "moo/texture_manager.hpp"
#include "moo/render_context.hpp"
#include "moo/vertices_manager.hpp"
#include "moo/vertices.hpp"

namespace BW {
namespace CompiledSpace {

// ----------------------------------------------------------------------------
class StaticGeometry::Detail
{
public:

	static void readBuffers( StaticGeometry* pThis,
		BinaryFormat::Stream* pStream,
		BW::vector<Moo::VertexBuffer>& buffers )
	{
		// Open the binary resource for these vertices
		D3DPOOL pool = D3DPOOL_MANAGED;
		pool = Moo::RenderContext::patchD3DPool(pool);

		// Iterate over all buffers defined
		for (size_t index = 0; index < pThis->bufferEntries_.size(); ++index)
		{
			pThis->numEntriesPopulated_++;

			StaticGeometryTypes::BufferEntry& entry = 
				pThis->bufferEntries_[index];	

			void* pData = pStream->readRaw( entry.size_ );

			// Now create a vertex buffer that we can fill with this data:
			Moo::VertexBuffer vb;
			vb.create( entry.size_, 0, 0, pool, "vertexBuffer/StaticGeometry");
			
			Moo::SimpleVertexLock vertexLock( vb, 0, entry.size_, 0 );
			if (!vertexLock)
			{
				continue;
			}

			memcpy( vertexLock, pData, entry.size_ );

			vb.addToPreloadList();

			buffers.push_back( vb );
		}
	}

	static void createResource( StaticGeometry* pThis,
		StaticGeometryTypes::Entry& entry, 
		BW::vector<Moo::VertexBuffer>& buffers,
		BW::vector<Moo::VerticesPtr>& loadedResources )
	{
		const char* resourceID =
			pThis->stringTable_.entry( entry.resourceID_ );
		Moo::VerticesPtr pVerts = new Moo::Vertices( resourceID, 0 );

		const char* sourceFormat = 
			pThis->stringTable_.entry( entry.originalVertexFormat_ );
		pVerts->initFromValues( entry.numVertices_, sourceFormat );

		for (uint32 index = entry.beginStreamIndex_; 
			index < entry.endStreamIndex_; ++index )
		{
			MF_ASSERT( index < pThis->streams_.size() );
			StaticGeometryTypes::StreamEntry& streamEntry =
				pThis->streams_[index];

			Moo::VertexBuffer vb = buffers[streamEntry.hostBufferID_];
			pVerts->addStream( 
				streamEntry.streamIndex_,
				vb, 
				streamEntry.hostBufferOffset_, 
				streamEntry.stride_,
				streamEntry.size_ );
		}

		// Now push this resource to VerticesManager
		Moo::VerticesManager::instance()->populateResource( 
			resourceID, pVerts );

		loadedResources.push_back( pVerts );
	}

	static void assignBuffers( StaticGeometry* pThis,
		BW::vector<Moo::VertexBuffer>& buffers,
		BW::vector<Moo::VerticesPtr>& loadedResources)
	{
		// Iterate over all the resources and build up their information
		// based on the baked information
		for (size_t index = 0; index < pThis->entries_.size(); ++index)
		{
			StaticGeometryTypes::Entry& entry = pThis->entries_[index];
			
			createResource( pThis, entry, buffers, loadedResources );

			pThis->numEntriesPopulated_++;
		}
	}
};

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
StaticGeometry::StaticGeometry() 
	: pReader_( NULL )
	, pStream_( NULL )
	, numEntriesPopulated_( 0 )
{

}


// ----------------------------------------------------------------------------
StaticGeometry::~StaticGeometry()
{
	this->close();
}


// ----------------------------------------------------------------------------
bool StaticGeometry::read( BinaryFormat& reader )
{
	PROFILER_SCOPED( StaticGeometry_ReadFromSpace );

	typedef StaticGeometryTypes::Entry Entry;

	MF_ASSERT( pReader_ == NULL );
	pReader_ = &reader;

	pStream_ = pReader_->findAndOpenSection(
		StaticGeometryTypes::FORMAT_MAGIC,
		StaticGeometryTypes::FORMAT_VERSION,
		"StaticGeometry" );
	if (!pStream_)
	{
		ASSET_MSG( "Failed to map static geometry '%s' into memory.\n",
			pReader_->resourceID() );
		this->close();
		return false;
	}

	if (!stringTable_.readFromStream( pStream_ ))
	{
		ASSET_MSG( "'%s' is malformed.\n", pReader_->resourceID() );
		this->close();
		return false;
	}

	pStream_->read( entries_ );
	pStream_->read( streams_ );
	pStream_->read( bufferEntries_ );

	loadGeometry();

	return true;
}


// ----------------------------------------------------------------------------
void StaticGeometry::loadGeometry()
{
	PROFILER_SCOPED( StaticGeometry_loadGeometry );

	BW::vector<Moo::VertexBuffer> buffers;
	Detail::readBuffers( this, pStream_, buffers );
	Detail::assignBuffers( this, buffers, loadedResources_ );
}


// ----------------------------------------------------------------------------
void StaticGeometry::close()
{
	entries_.reset();

	if (pStream_ && pReader_)
	{
		pReader_->closeSection( pStream_ );
	}
	pReader_ = NULL;
	pStream_ = NULL;
}


// ----------------------------------------------------------------------------
bool StaticGeometry::isValid() const
{
	return entries_.valid();
}


// ----------------------------------------------------------------------------
size_t StaticGeometry::size() const
{
	MF_ASSERT( this->isValid() );
	return entries_.size();
}


// ----------------------------------------------------------------------------
bool StaticGeometry::empty() const
{
	return entries_.empty();
}


// ----------------------------------------------------------------------------
float StaticGeometry::percentLoaded() const
{
	if (!this->isValid())
	{
		return 0.f;
	}
	else if (this->empty())
	{
		return 1.f;
	}
	else
	{
		return static_cast<float>( numEntriesPopulated_) /
			static_cast<float>( entries_.size() + bufferEntries_.size() );
	}
}

} // namespace CompiledSpace
} // namespace BW