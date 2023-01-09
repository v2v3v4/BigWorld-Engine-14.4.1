#include "pch.hpp"

#include "static_geometry_writer.hpp"
#include "binary_format_writer.hpp"
#include "string_table_writer.hpp"
#include "asset_list_writer.hpp"

#include "resmgr/quick_file_writer.hpp"
#include "moo/vertices.hpp"
#include "moo/vertices_manager.hpp"
#include "moo/visual.hpp"
#include "moo/visual_common.hpp"

#include <limits>

namespace BW {
namespace CompiledSpace {

namespace {

	bool stringEndsWith( const BW::string& value, 
		const BW::string& endsWith )
	{
		if (value.length() < endsWith.length())
		{
			return false;
		}
		else
		{
			return value.compare( value.length() - endsWith.length(),
				endsWith.length(), endsWith ) == 0;	
		}
	}

	bool isGeometryAsset( const BW::string& filename )
	{
		// Geometry based assets are models and visuals.
		return stringEndsWith( filename, ".visual") ||
			stringEndsWith( filename, ".model");
	}

	bool includeAsStaticGeometry( const BW::string& filename )
	{
		// Insert checks here to determine if an asset should be included
		// as static geometry. To begin with, we are going to include 
		// everything. Add to here to create rules for things that shouldn't
		// be baked.
		return true;
	}

	bool isGeometrySupported( const BW::string& filename )
	{
		const char* matchEnding = "vertices";
		size_t matchEndingLength = strlen( matchEnding );

		BW::string name;
		std::transform( filename.begin(), filename.end(), 
			name.begin(), []( char i ){ return std::tolower(i); } );
		if (name.length() < matchEndingLength)
		{
			return false;
		}

		if (name.compare( name.length() - matchEndingLength,
			matchEndingLength, matchEnding ) != 0)
		{
			// Did not match the ending
			return false;
		}

		if ((name.length() >= matchEndingLength + 1) &&
			name.compare( name.length() - (matchEndingLength + 1),
			matchEndingLength + 1, "mvertices" ) == 0)
		{
			// Morph vertices not supported currently!
			return false;
		}

		return true;
	}

	void extractVisualPrimitiveReferences( const BW::string& filename,
		BW::vector<BW::string>& primitiveReferences )
	{
		BW::string baseName = 
			filename.substr( 0, filename.find_last_of( '.' ) );
		Moo::VisualLoader< Moo::Visual > loader( baseName, false );

		// open the resource
		DataSectionPtr orig = loader.getRootDataSection();
		if (!orig)
		{
			ERROR_MSG( "Couldn't open visual %s\n", filename.c_str() );
			return;
		}

		// Load our primitives
		const BW::string primitivesFileName = loader.getPrimitivesResID() + '/';

		// iterate through the rendersets and create them
		BW::vector< DataSectionPtr > renderSets;
		orig->openSections( "renderSet", renderSets );

		BW::vector< DataSectionPtr >::iterator rsit = renderSets.begin();
		BW::vector< DataSectionPtr >::iterator rsend = renderSets.end();
		while (rsit != rsend)
		{
			DataSectionPtr renderSetSection = *rsit;
			++rsit;

			// Open the geometry sections
			BW::vector< DataSectionPtr > geometries;
			renderSetSection->openSections( "geometry", geometries );

			BW::vector< DataSectionPtr >::iterator geit = geometries.begin();
			BW::vector< DataSectionPtr >::iterator geend = geometries.end();

			// iterate through the geometry sections
			while (geit != geend)
			{
				DataSectionPtr geometrySection = *geit;
				++geit;

				// Get a reference to the vertices used by this geometry
				BW::string verticesName = geometrySection->readString( "vertices" );
				if (verticesName.find_first_of( '/' ) == BW::string::npos)
					verticesName = primitivesFileName + verticesName;

				primitiveReferences.push_back( verticesName );
			}
		}
	}

	void extractModelPrimitiveReferences( const BW::string& filename,
		BW::vector<BW::string>& primitiveReferences )
	{
		DataSectionPtr pRoot = BWResource::openSection( filename );
		if (!pRoot)
		{
			ERROR_MSG( "Couldn't open model %s\n", filename.c_str() );
			return;
		}

		BW::string parentModel = pRoot->readString( "parent" );
		if (!parentModel.empty())
		{
			extractModelPrimitiveReferences( parentModel + ".model", 
				primitiveReferences );
		}

		BW::string nodelessVisual = pRoot->readString( "nodelessVisual" );
		if (!nodelessVisual.empty())
		{
			extractVisualPrimitiveReferences( nodelessVisual + ".visual", 
				primitiveReferences );
		}

		BW::string nodefullVisual = pRoot->readString( "nodefullVisual" );
		if (!nodefullVisual.empty())
		{
			extractVisualPrimitiveReferences( nodefullVisual + ".visual", 
				primitiveReferences );
		}
	}

	void extractPrimitiveReferences( const BW::string& filename,
		BW::vector<BW::string>& primitiveReferences )
	{
		if (stringEndsWith( filename, ".visual"))
		{
			extractVisualPrimitiveReferences( filename, primitiveReferences );
		}
		else if (stringEndsWith( filename, ".model"))
		{
			extractModelPrimitiveReferences( filename, primitiveReferences );
		}
	}

	const size_t MAX_BUFFER_SIZE = (1 << 16) * 64; // 2 ^ 16 - max value indexable by short
	const size_t BUFFER_ALIGNMENT = 16;

	struct MergedBuffer
	{
		void* buffer_;
		size_t usedBuffer_;
		size_t size_;
	};

	size_t alignSize( size_t bufferSize )
	{
		size_t alignedBufferSize = (bufferSize + BUFFER_ALIGNMENT - 1);
		alignedBufferSize &= ~(BUFFER_ALIGNMENT - 1);
		return alignedBufferSize;
	}

	size_t assignBuffer( BW::vector<MergedBuffer>* buffers, 
		size_t bufferSize )
	{
		if ( bufferSize > MAX_BUFFER_SIZE )
		{
			return (uint32)-1;
		}

		// Iterate over all existing buffers looking for a buffer with enough
		// space to fit this one:
		size_t index = 0;
		for (;index < buffers->size(); ++index)
		{
			const MergedBuffer& buffer = buffers->at( index );
			const size_t availableBytes = buffer.size_ - buffer.usedBuffer_;
			if (availableBytes >= bufferSize)
			{
				break;
			}
		}

		if (index == buffers->size())
		{
			// We need to make a new buffer to house this data
			MergedBuffer buffer;
			buffer.buffer_ = bw_malloc( MAX_BUFFER_SIZE );
			buffer.size_ = MAX_BUFFER_SIZE;
			buffer.usedBuffer_ = 0;
			buffers->push_back( buffer );
		}

		MF_ASSERT( index < buffers->size() );

		return index;
	}

	bool extractStream( 
		Moo::VertexBuffer vb,
		StaticGeometryTypes::StreamEntry& streamEntry,
		BW::vector<MergedBuffer>* buffers )
	{
		// Find a buffer that is large enough
		size_t bufferSize = vb.size();
		size_t alignedBufferSize = alignSize( bufferSize );
		
		streamEntry.hostBufferID_ = 
			(uint32)assignBuffer( buffers, alignedBufferSize );

		if (streamEntry.hostBufferID_ == (uint32)-1)
		{
			return false;
		}

		// Lock the VB for read
		Moo::SimpleVertexLock vertexLock( vb, 0, (uint32)bufferSize, 0 );
		if (!vertexLock)
		{
			return false;
		}
		
		MergedBuffer& buffer = (*buffers)[ streamEntry.hostBufferID_ ];
		
		streamEntry.hostBufferOffset_ = (uint32)buffer.usedBuffer_;
		
		// Copy the data into the target buffer
		memcpy( (char*)buffer.buffer_ + buffer.usedBuffer_, vertexLock, bufferSize ); 
		streamEntry.size_ = (uint32)bufferSize;

		// Modify buffer size so that it always allocates alligned blocks
		// that way the next buffer to come in here will be aligned too
		buffer.usedBuffer_ += alignedBufferSize;
		MF_ASSERT( buffer.usedBuffer_ <= buffer.size_ );

		return true;
	}

	void rollbackStream( 
		StaticGeometryTypes::StreamEntry& streamEntry,
		BW::vector<MergedBuffer>* buffers )
	{
		MF_ASSERT( streamEntry.hostBufferID_ < buffers->size() );

		// Modify buffer size so that it always allocates aligned blocks
		// that way the next buffer to come in here will be aligned too
		size_t alignedBufferSize = alignSize( streamEntry.size_ );

		// Fetch the buffer
		MergedBuffer& buffer = (*buffers)[ streamEntry.hostBufferID_ ];
		MF_ASSERT( buffer.usedBuffer_ >= alignedBufferSize );
		buffer.usedBuffer_ -= alignedBufferSize;

		if (buffer.usedBuffer_ == 0)
		{
			// remove the buffer from the end of buffers vector:
			MF_ASSERT( buffers->size() - 1 == streamEntry.hostBufferID_ );
			MF_ASSERT( 0 == streamEntry.hostBufferOffset_ );
			bw_free( buffer.buffer_ );

			buffers->pop_back();
		}
	}

	bool extractStreams( StaticGeometryTypes::Entry& entry,
		Moo::VerticesPtr pVerts,
		BW::vector<StaticGeometryTypes::StreamEntry>* streams,
		BW::vector<MergedBuffer>* buffers )
	{
		uint32 numStreams = pVerts->numVertexBuffers();

		entry.beginStreamIndex_ = (uint32)streams->size();
		entry.endStreamIndex_ = entry.beginStreamIndex_ + numStreams;

		// Extract all the data from the streams
		bool success = true;
		uint32 index = 0;
		for (; index < numStreams; ++index )
		{
			Moo::VertexBuffer vb = pVerts->vertexBuffer( index );

			StaticGeometryTypes::StreamEntry streamEntry;
			if (!extractStream( vb, streamEntry, buffers ))
			{
				success = false;
				break;
			}

			streamEntry.streamIndex_ = pVerts->vertexStreamIndex( index );
			streamEntry.stride_ = pVerts->vertexStride( index );

			MF_ASSERT( streamEntry.streamIndex_ == 0 ||
				streamEntry.streamIndex_ == Moo::UV2Stream::STREAM_NUMBER ||
				streamEntry.streamIndex_ == Moo::ColourStream::STREAM_NUMBER );

			streams->push_back( streamEntry );
		}

		if (!success)
		{
			// Roll back the allocations of this entry
			// The current index stream was never actually allocated
			// so decrement now before rolling back others.
			--index;
			for (; index < numStreams; --index )
			{
				// Pop off the stream definition
				StaticGeometryTypes::StreamEntry streamEntry = streams->back();
				streams->pop_back();

				rollbackStream( streamEntry, buffers );
			}
		}

		return success;
	}

	void generateEntry(const BW::string& primitives, 
		BW::vector<StaticGeometryTypes::Entry>* entries, 
		BW::vector<StaticGeometryTypes::StreamEntry>* streams, 
		BW::vector<Moo::VerticesPtr>* loadedData,
		BW::vector<MergedBuffer>* buffers,
		StringTableWriter* stringTable )
	{
		// Load the primitive data.
		Moo::VerticesPtr pVerts = Moo::VerticesManager::instance()->get( 
			primitives, 0 );
		if (!pVerts)
		{
			WARNING_MSG( "Failed to load primitives file '%s'. This data will"
				" not be available in the static geometry buffers.\n",
				primitives.c_str() );
			return;
		}

		loadedData->push_back( pVerts );

		StaticGeometryTypes::Entry entry;
		memset( &entry, 0, sizeof(entry) );

		// Copy out all the streams
		if (!extractStreams( entry, pVerts, streams, buffers ))
		{
			// Skip this one, it won't go into the final list of entries
			return;
		}

		// Make sure the right number of streams were created as expected
		MF_ASSERT( streams->size() == entry.endStreamIndex_ );

		// Extract the information required to rebuild it 
		entry.resourceID_ = stringTable->addString( primitives );

		// Copy in the original source format name
		entry.originalVertexFormat_ = stringTable->addString( 
			pVerts->sourceFormat().c_str() );

		entry.numVertices_ = pVerts->nVertices();

		entries->push_back( entry );
	}

	void generateEntries( const BW::vector<BW::string>& primitiveReferences, 
		BW::vector<StaticGeometryTypes::Entry>* entries, 
		BW::vector<StaticGeometryTypes::StreamEntry>* streams, 
		BW::vector<Moo::VerticesPtr>* loadedData,
		BW::vector<MergedBuffer>* buffers,
		StringTableWriter* stringTable )
	{
		for (size_t index = 0; index < primitiveReferences.size(); ++index)
		{
			const BW::string& primitiveName = primitiveReferences[index];
			generateEntry( primitiveName, entries, streams, loadedData, 
				buffers, stringTable );
		}
	}

	void generateBufferEntries(
		BW::vector<MergedBuffer>& buffers,
		BW::vector<StaticGeometryTypes::BufferEntry>* bufferEntries )
	{
		for (size_t index = 0; index < buffers.size(); ++index)
		{
			MergedBuffer& buffer = buffers[index];

			StaticGeometryTypes::BufferEntry entry;
			entry.size_ = (uint32)buffer.usedBuffer_;
			bufferEntries->push_back( entry );
		}
	}

	void writeBuffers( BinaryFormatWriter::Stream* pBufferStream,
		BW::vector<MergedBuffer>& buffers )
	{
		for (size_t index = 0; index < buffers.size(); ++index)
		{
			MergedBuffer& buffer = buffers[index];

			pBufferStream->writeRaw( buffer.buffer_, buffer.usedBuffer_ );
		}
	}

	void freeBuffers( BW::vector<MergedBuffer>& buffers )
	{
		for (size_t index = 0; index < buffers.size(); ++index)
		{
			MergedBuffer& buffer = buffers[index];
			bw_free( buffer.buffer_ );
		}
	}

} // End anonymous namespace

// ----------------------------------------------------------------------------
StaticGeometryWriter::StaticGeometryWriter()
{

}


// ----------------------------------------------------------------------------
StaticGeometryWriter::~StaticGeometryWriter()
{

}


// ----------------------------------------------------------------------------
bool StaticGeometryWriter::initialize( const DataSectionPtr& pSpaceSettings,
	const CommandLine& commandLine )
{
	// Nothing required
	return true;
}


// ----------------------------------------------------------------------------
void StaticGeometryWriter::postProcess()
{
	extractCandidates( strings(), assets() );

	uint32 primCount = (uint32)primitiveReferences_.size();
	TRACE_MSG( "%d static geometry primitive candidates.\n", 
		primCount );
}


// ----------------------------------------------------------------------------
void StaticGeometryWriter::extractCandidates( 
	const StringTableWriter& stringTable,
	const AssetListWriter& assetList )
{
	// Iterate all assets in the list, and determine what files we will 
	// bake into static geometry
	for (size_t index = 0; index < assetList.size(); ++index)
	{
		size_t stringIndex = assetList.assetNameIndex( index );
		const BW::string& assetName = stringTable.entry( stringIndex );

		// Check if the asset is relevant
		if (!isGeometryAsset( assetName ) ||
			!includeAsStaticGeometry( assetName ))
		{
			continue;
		}

		// Extract all the primitive files that this asset references
		extractPrimitiveReferences( assetName, primitiveReferences_ );
	}
}


// ----------------------------------------------------------------------------
bool StaticGeometryWriter::write( BinaryFormatWriter& writer )
{
	StringTableWriter internalStringTable;
	BW::vector<Moo::VerticesPtr> loadedData;
	BW::vector<StaticGeometryTypes::Entry> entries;
	BW::vector<StaticGeometryTypes::StreamEntry> streams;
	BW::vector<MergedBuffer> buffers;
	generateEntries( primitiveReferences_, &entries, &streams, &loadedData,
		&buffers, &internalStringTable );

	BW::vector<StaticGeometryTypes::BufferEntry> bufferEntries;
	generateBufferEntries( buffers, &bufferEntries );

	BinaryFormatWriter::Stream* pStream =
		writer.appendSection( 
		StaticGeometryTypes::FORMAT_MAGIC, 
		StaticGeometryTypes::FORMAT_VERSION );
	MF_ASSERT( pStream != NULL );

	internalStringTable.writeToStream( pStream );
	pStream->write( entries );
	pStream->write( streams );
	pStream->write( bufferEntries );
	writeBuffers( pStream, buffers );

	freeBuffers( buffers );

	return true;
}


} // namespace CompiledSpace
} // namespace BW
