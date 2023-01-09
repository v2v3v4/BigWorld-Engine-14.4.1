#include "pch.hpp"
#include "vertex_format.hpp"
#include "resmgr/bwresource.hpp"
#include "vertices.hpp"
#include "cstdmf/profiler.hpp"

BW_BEGIN_NAMESPACE

namespace Moo
{


// Implementation is here since it depends on vertex element type traits classes
bool fillStorageInfo( VertexElement::StorageType::Value storageType, 
					 size_t* size, uint32* components)
{

#define FILL_INFO( type ) case VertexElement::StorageType::type: \
	if (size) \
	{ \
		*size = VertexElementStorageTypeTraits< VertexElement::StorageType::type >::s_size; \
	} \
	if (components) \
	{ \
		*components = VertexElementStorageTypeTraits< VertexElement::StorageType::type >::s_components; \
	} \
	return true

	switch (storageType)
	{
		FILL_INFO( FLOAT1 );
		FILL_INFO( FLOAT2 );
		FILL_INFO( FLOAT3 );
		FILL_INFO( FLOAT4 );
		FILL_INFO( COLOR );
		FILL_INFO( UBYTE1 );
		FILL_INFO( UBYTE2 );
		FILL_INFO( UBYTE3 );
		FILL_INFO( UBYTE4 );
		FILL_INFO( UBYTE4_NORMAL_8_8_8 );
		FILL_INFO( SHORT1 );
		FILL_INFO( SHORT2 );
		FILL_INFO( SHORT3 );
		FILL_INFO( SHORT4 );
		FILL_INFO( UBYTE4N );
		FILL_INFO( SHORT2N );
		FILL_INFO( SHORT4N );
		FILL_INFO( USHORT2N );
		FILL_INFO( USHORT4N );
		FILL_INFO( UDEC3 );
		FILL_INFO( DEC3N );
		FILL_INFO( HALF2 );
		FILL_INFO( HALF4 );

		FILL_INFO( SC_DIV3_III );
		FILL_INFO( SC_REVERSE_III );
		FILL_INFO( SC_REVERSE_PADDED_III_ );
		FILL_INFO( SC_REVERSE_PADDED__WW_ );
		FILL_INFO( SC_REVERSE_PADDED_WW_ );

	default:
		if (size)
		{
			*size = 0;
		}
		if (components)
		{
			*components = 0; 
		}
		return false;
	}
#undef FILL_INFO
}

uint32 VertexElement::componentCount( StorageType::Value storageType )
{
	uint32 storageComponents;
	fillStorageInfo( storageType, NULL, &storageComponents );
	return storageComponents;
}

size_t VertexElement::typeSize( StorageType::Value storageType )
{
	size_t storageSize;
	fillStorageInfo( storageType, &storageSize, NULL );
	return storageSize;
}


// -----------------------------------------------------------------------------
// Section: VertexFormat
// -----------------------------------------------------------------------------
VertexFormat::VertexFormat()
{

}


VertexFormat::~VertexFormat()
{

}

VertexFormat::VertexFormat( const BW::StringRef & name )
	: name_( name.to_string() )
{
}

bool VertexFormat::equals( const VertexFormat & other ) const
{
	if (this == &other)
	{
		return true;
	}

	if (this->streamCount() != other.streamCount())
	{
		return false;
	}

	for (uint32 streamIndex = 0; streamIndex < this->streamCount(); ++streamIndex)
	{
		// compare streams
		const StreamDefinition & myStream = streams_[streamIndex];
		const StreamDefinition & otherStream = other.streams_[streamIndex];
		if (myStream.stride_ != otherStream.stride_ || 
			myStream.elements_.size() != otherStream.elements_.size())
		{
			return false;
		}

		for (uint32 elementIndex = 0; elementIndex < myStream.elements_.size(); ++elementIndex)
		{
			// compare stream elements
			const ElementDefinition & myElement = myStream.elements_[elementIndex];
			const ElementDefinition & otherElement = otherStream.elements_[elementIndex];
			if (memcmp( &myElement, &otherElement, sizeof(ElementDefinition) ) != 0)
			{
				return false;
			}
		}
	}

	return true;
}

bool VertexFormat::operator==( const VertexFormat & other ) const
{
	return this->equals( other );
}


const BW::StringRef VertexFormat::name() const
{
	return name_;
}

uint32 VertexFormat::addStream()
{
	streams_.push_back( StreamDefinition() );
	return (uint32)(streams_.size() - 1);
}


void VertexFormat::addElement( uint32 streamIndex, 
	SemanticType::Value semantic, 
	StorageType::Value storageType, 
	uint32 semanticIndex, 
	size_t offset )
{
	MF_ASSERT( streamIndex < streams_.size() );
	StreamDefinition& stream = streams_[streamIndex];

	if (offset == DEFAULT)
	{
		offset = stream.stride_;
	}
	if (semanticIndex == DEFAULT)
	{
		semanticIndex = 0;
		while (containsElement( semantic, semanticIndex ))
		{
			++semanticIndex;
		}
	}

	MF_ASSERT( !containsElement( semantic, semanticIndex ) );

	ElementDefinition element( semantic, semanticIndex, storageType, offset );
	stream.elements_.push_back( element );

	stream.stride_ = max( stream.stride_, (uint32)offset + typeSize( storageType ));
}


bool VertexFormat::findElement( uint32 index, SemanticType::Value* semantic, 
	uint32* streamIndex, uint32* semanticIndex, size_t* offset, 
	StorageType::Value* storageType) const
{
	uint32 curStreamIndex = 0;
	uint32 streamCount = countElements( curStreamIndex );
	while (index >= streamCount && 
		curStreamIndex < streams_.size())
	{
		index -= streamCount;
		curStreamIndex++;
		streamCount = countElements( curStreamIndex );
	}

	if (curStreamIndex >= streams_.size())
	{
		return false;
	}

	if (streamIndex)
	{
		*streamIndex = curStreamIndex;
	}
	
	return findElement( curStreamIndex, index, semantic, semanticIndex, offset, storageType );
}


bool VertexFormat::findElement( uint32 streamIndex, uint32 index, 
	SemanticType::Value* semantic, uint32* semanticIndex,
	size_t * offset, StorageType::Value * storageType ) const
{
	MF_ASSERT( streamIndex < streams_.size() );
	const StreamDefinition& stream = streams_[streamIndex];

	size_t elementCount = stream.elements_.size();
	uint32 count = 0;
	for (uint32 curElement = 0; curElement < elementCount; ++curElement)
	{
		const ElementDefinition& element = stream.elements_[curElement];

		if (count != index)
		{
			count++;
			continue;
		}

		// Found the element we're looking for
		if (offset)
		{
			*offset = element.offset_;
		}
		if (storageType)
		{
			*storageType = element.storage_;
		}
		if (semantic)
		{
			*semantic = element.semantic_;
		}
		if (semanticIndex)
		{
			*semanticIndex = element.semanticIndex_;
		}

		return true;
	}

	// Failed to find element
	return false;
}


bool VertexFormat::findElement( SemanticType::Value semantic, uint32 semanticIndex,
	uint32 * streamIndex, size_t * offset, StorageType::Value * storageType ) const
{
	for (uint32 curStreamIndex = 0; curStreamIndex < streams_.size(); ++curStreamIndex)
	{
		bool success = findElement( curStreamIndex, semantic, semanticIndex, offset, storageType );
		if (success)
		{
			if (streamIndex)
			{
				*streamIndex = curStreamIndex;
			}

			return true;
		}
	}
	return false;
}


bool VertexFormat::findElement( uint32 streamIndex, SemanticType::Value semantic, uint32 semanticIndex,
	size_t * offset, StorageType::Value * storageType ) const
{
	MF_ASSERT( streamIndex < streams_.size() );
	const StreamDefinition& stream = streams_[streamIndex];

	size_t elementCount = stream.elements_.size();
	for (uint32 curElement = 0; curElement < elementCount; ++curElement)
	{
		const ElementDefinition& element = stream.elements_[curElement];

		if (element.semantic_ != semantic)
		{
			continue;
		}
		if (element.semanticIndex_ != semanticIndex)
		{
			continue;
		}

		// Found the element we're looking for
		if (offset)
		{
			*offset = element.offset_;
		}
		if (storageType)
		{
			*storageType = element.storage_;
		}
		
		return true;
	}

	// Failed to find element
	return false;
}


bool VertexFormat::containsElement( SemanticType::Value semantic, uint32 semanticIndex ) const
{
	return findElement( semantic, semanticIndex, NULL, NULL, NULL );
}


uint32 VertexFormat::countElements( ) const
{
	uint32 count = 0;
	for (uint32 index = 0; index < streams_.size(); ++index)
	{
		count += countElements( index );
	}
	return count;
}


uint32 VertexFormat::countElements( uint32 streamIndex ) const
{
	if (streamIndex >= streams_.size())
	{
		return 0;
	}

	const StreamDefinition& stream = streams_[streamIndex];
	return (uint32)stream.elements_.size();
}


uint32 VertexFormat::countElements( SemanticType::Value semantic ) const
{
	uint32 count = 0;
	for (uint32 index = 0; index < streams_.size(); ++index)
	{
		count += countElements( index, semantic );
	}
	return count;
}


uint32 VertexFormat::countElements( uint32 streamIndex, SemanticType::Value semantic ) const
{
	if (streamIndex >= streams_.size())
	{
		return 0;
	}

	const StreamDefinition& stream = streams_[streamIndex];

	size_t elementCount = stream.elements_.size();
	uint32 count = 0;
	for (uint32 index = 0; index < elementCount; ++index)
	{
		const ElementDefinition& element = stream.elements_[index];

		if (element.semantic_ == semantic)
		{
			count++;
		}
	}

	return count;
}

uint32 VertexFormat::streamStride( uint32 streamIndex ) const
{
	if (streamIndex < streams_.size())
	{
		return streams_[streamIndex].stride_;
	}
	else
	{
		return 0;
	}
}


uint32 VertexFormat::streamCount() const
{
	return (uint32)streams_.size();
}


uint32 VertexFormat::typeSize( StorageType::Value type )
{
	return (uint32)VertexElement::typeSize( type );
}

bool VertexFormat::containsSemantics( const SemanticType::Value * semantics, 
	size_t numSemantics, uint32 semanticIndex, uint32 streamIndex ) const
{
	MF_ASSERT( semantics );
	if (numSemantics == 0 || streamCount() == 0)
	{
		return false;
	}

	// Determine stream range to check
	uint32 beginStreamIndex = streamIndex;
	uint32 endStreamIndex = streamIndex + 1;
	if (streamIndex == DEFAULT)
	{
		// No stream provided: use all available streams
		beginStreamIndex = 0;
		endStreamIndex = std::max( beginStreamIndex + 1, streamCount() );
	}
	else if (streamIndex >= streamCount())
	{
		return false;
	}

	bool semanticFound;
	for (uint32 i = 0; i < numSemantics; ++i)
	{
		semanticFound = false;
		for (streamIndex = beginStreamIndex; 
			(streamIndex < endStreamIndex && !semanticFound); ++streamIndex)
		{
			// per stream in stream range
			const StreamDefinition& stream = streams_[streamIndex];

			size_t elementCount = stream.elements_.size();
			for (uint32 elementIndex = 0; elementIndex < elementCount; ++elementIndex)
			{
				// per element in stream, test against semantic
				const ElementDefinition& element = stream.elements_[elementIndex];

				if (element.semantic_ == semantics[i] &&
						(semanticIndex == DEFAULT || 
						semanticIndex == element.semanticIndex_))
				{
					// found semantic, start search for next semantic
					semanticFound = true;
					break;
				}
			}
		}
		if (!semanticFound)
		{
			// couldn't find semantic in stream range
			return false;
		}
	}

	return true;
}

bool VertexFormat::fillElements( void* dstData, size_t dstSize, uint32 streamIndex, 
	uint32 index, VertexConversions::DefaultValueFunc defaultValFunc ) const
{
	MF_ASSERT( dstData );
	MF_ASSERT( defaultValFunc );

	// Find the element
	SemanticType::Value semanticType;
	StorageType::Value storageType;
	uint32 semanticIndex;
	size_t offset;
	bool result = findElement( streamIndex, index, &semanticType, &semanticIndex, 
		&offset, &storageType );
	if (!result)
	{
		return false;
	}

	// Check the size
	uint32 elementSize = typeSize( storageType );
	if(elementSize == 0)
	{
		return false;
	}
	
	// Setup a raw accessor
	RawElementAccessor<uint8> accessor = createRawElementAccessor<uint8>(
		dstData, dstSize, streamIndex, offset );

	if (!accessor.isValid())
	{
		return false;
	}
	
	// Store for the first vert
	if (accessor.size() > 0)
	{
		uint8 * pFirstVal = &accessor[0];
		(*defaultValFunc)( pFirstVal, elementSize );

		// Write remaining verts.
		for (uint32 i = 1; i < accessor.size(); ++i)
		{
			memcpy( &accessor[i], pFirstVal, elementSize );
		}
	}

	return true;
}


bool VertexFormat::canConvert(
	const BufferSet & dstBuffers, const BufferSet & srcBuffers, 
	bool createFailureMessages )
{
	const VertexFormat& srcFormat = srcBuffers.format_;
	const VertexFormat& dstFormat = dstBuffers.format_;

	uint32 dstElementCount = dstFormat.countElements();
	for (uint32 elementIndex = 0; elementIndex != dstElementCount; ++elementIndex)
	{
		// Grab the element definitions
		SemanticType::Value dstSemantic;
		StorageType::Value dstStorage;
		uint32 dstStreamIndex;
		uint32 dstSemanticIndex;
		bool result = dstFormat.findElement(
			elementIndex, &dstSemantic, &dstStreamIndex, &dstSemanticIndex, 
			NULL, &dstStorage );
		if (!result)
		{
			if (createFailureMessages)
			{
				ASSET_MSG( "VertexFormat::canConvert: Failure finding destination element %d\n", elementIndex );
			}
			return false;
		}

		// Now attempt to find the corresponding element from the source stream
		StorageType::Value srcStorage;
		uint32 srcStreamIndex;
		result = srcFormat.findElement( dstSemantic, dstSemanticIndex, &srcStreamIndex, NULL, &srcStorage);
		if (!result)
		{
			// This part of the stream can't be copied from the source
			continue;
		}

		// Make sure a conversion exists between the types:
		VertexConversions::ConversionFunc convert = 
			VertexConversions::fetchConvertFunction( dstSemantic, dstStorage, srcStorage );

		if (convert == NULL)
		{
			if (createFailureMessages)
			{
				WARNING_MSG( "VertexFormat::canConvert: Could not find conversion function "
					"for converting storage type '%s' to '%s' with semantic '%s'\n",
					VertexElement::StringFromStorage( srcStorage ).to_string().c_str(),
					VertexElement::StringFromStorage( dstStorage ).to_string().c_str(),
					VertexElement::StringFromSemantic( dstSemantic ).to_string().c_str() );
			}
			return false;
		}

		// Make sure the sizes of the streams are compatible:
		if (dstStreamIndex >= dstBuffers.buffers_.size())
		{
			if (createFailureMessages)
			{
				WARNING_MSG( "VertexFormat::canConvert: Error iterating over destination format\n" );
			}
			return false;
		}
		if (srcStreamIndex >= srcBuffers.buffers_.size())
		{
			if (createFailureMessages)
			{
				WARNING_MSG( "VertexFormat::canConvert: Could not find an appropriate source "
					"element for semantic='%s', index=%d\n",
					VertexElement::StringFromSemantic( dstSemantic ).to_string().c_str(), dstSemanticIndex );
			}
			return false;
		}
		size_t dstNumVerts = dstBuffers.buffers_[dstStreamIndex].size_ / dstFormat.streamStride( dstStreamIndex );
		size_t srcNumVerts = srcBuffers.buffers_[srcStreamIndex].size_ / srcFormat.streamStride( srcStreamIndex );

		if (srcNumVerts == 0)
		{
			if (createFailureMessages)
			{
				WARNING_MSG( "VertexFormat::canConvert: Destination format stream (index=%d)"
					" requires data from empty source stream (index=%d)."
					" Please ensure that the relevant source stream data is being supplied.\n",
					dstStreamIndex, srcStreamIndex );
			}
			return false;
		}

		if (dstNumVerts != srcNumVerts)
		{
			if (createFailureMessages)
			{
				WARNING_MSG( "VertexFormat::canConvert: Vertex count mismatch in "
					" source=(stream %d, count=%d),"
					" target=(stream %d, count=%d)\n",
					srcStreamIndex, srcNumVerts, dstStreamIndex, dstNumVerts );
			}
			return false;
		}
	}

	return true;
}


bool VertexFormat::convert( const BufferSet& dstBuffers, 
	const BufferSet& srcBuffers, bool initialiseMissing )
{
	// Validate the conversion
	const VertexFormat& srcFormat = srcBuffers.format_;
	const VertexFormat& dstFormat = dstBuffers.format_;

	if (!canConvert( dstBuffers, srcBuffers ))
	{
		return false;
	}

	// Perform conversion
	uint32 dstElementCount = dstFormat.countElements();
	for (uint32 elementIndex = 0; elementIndex != dstElementCount; ++elementIndex)
	{
		// Grab the element definitions
		SemanticType::Value dstSemantic;
		StorageType::Value dstStorage;
		uint32 dstStreamIndex;
		size_t dstStreamOffset;
		uint32 dstSemanticIndex;
		bool result = dstFormat.findElement(
			elementIndex, &dstSemantic, &dstStreamIndex, &dstSemanticIndex, 
			&dstStreamOffset, &dstStorage );
		if (!result)
		{
			return false;
		}

		// Now attempt to find the corresponding element from the source stream
		StorageType::Value srcStorage;
		uint32 srcStreamIndex;
		size_t srcStreamOffset;
		result = srcFormat.findElement( dstSemantic, dstSemanticIndex, 
			&srcStreamIndex, &srcStreamOffset, &srcStorage );
		if (!result)
		{
			// There is no element to copy from (this is a new element)
			if(!initialiseMissing)
			{
				continue;
			}

			// Use the default value function
			VertexConversions::DefaultValueFunc defaultValFunc = 
				VertexConversions::fetchDefaultValueFunction( dstSemantic, dstStorage );
			MF_ASSERT( defaultValFunc );

			// Batch fill this element
			const Buffer& buffer = dstBuffers.buffers_[dstStreamIndex];
			bool filled = dstFormat.fillElements( buffer.data_, buffer.size_, 
				dstStreamIndex, elementIndex, defaultValFunc );
			MF_ASSERT( filled );
		} 
		else
		{
			// There is an element to copy from. Find the right conversion
			VertexConversions::ConversionFunc convertFunc = 
				VertexConversions::fetchConvertFunction( dstSemantic, dstStorage, srcStorage );

			if (convertFunc == NULL)
			{
				return false;
			}

			// Perform batch conversion of this element
			{
				RawElementAccessor<uint8> srcAccessor = srcFormat.createRawElementAccessor<uint8>(
					srcBuffers.buffers_[srcStreamIndex].data_, 
					srcBuffers.buffers_[srcStreamIndex].size_, 
					srcStreamIndex,
					srcStreamOffset);

				RawElementAccessor<uint8> dstAccessor = dstFormat.createRawElementAccessor<uint8>(
					dstBuffers.buffers_[dstStreamIndex].data_, 
					dstBuffers.buffers_[dstStreamIndex].size_, 
					dstStreamIndex,
					dstStreamOffset);

				MF_ASSERT( srcAccessor.size() == dstAccessor.size() );

				size_t elementCount = dstAccessor.size();
				uint32 dstTypeSize = typeSize( dstStorage );
				for (uint32 index = 0; index < elementCount; ++index)
				{
					void* srcData = &srcAccessor[index];
					void* dstData = &dstAccessor[index];

					(*convertFunc)( dstData, srcData, dstTypeSize );
				}
			}
			// End batch conversion of this element
		}
	}

	return true;
}

bool VertexFormat::convertBuffer( 
	const VertexFormat & srcFormat, const void * srcData, size_t srcSize,
	const VertexFormat & dstFormat, void * dstData, size_t dstSize,
	bool createFailureMessages )
{
	return convertBufferStream(
		srcFormat, srcData, srcSize, 0,
		dstFormat, dstData, dstSize, 0, createFailureMessages );
}


bool VertexFormat::convertBufferStream( 
	const VertexFormat & srcFormat, const void * srcData, size_t srcSize, uint32 srcStreamIndex,
	const VertexFormat & dstFormat, void * dstData, size_t dstSize, uint32 dstStreamIndex,
	bool createFailureMessages )
{
	PROFILER_SCOPED( VertexFormat_convertBufferStream );

	// Pad buffer list with required number of empty entries preceding
	// the single buffer of interest.
	
	// source buffer set
	VertexFormat::BufferSet sourceBufferSet( srcFormat );
	sourceBufferSet.buffers_.reserve( srcStreamIndex + 1 );
	for (uint32 i = 0; i < srcStreamIndex; ++i)
	{
		sourceBufferSet.buffers_.push_back( 
			VertexFormat::Buffer( NULL, 0 ) );
	}
	sourceBufferSet.buffers_.push_back( 
		VertexFormat::Buffer( const_cast<void*>(srcData), srcSize ) );
	
	// target buffer set
	VertexFormat::BufferSet targetBufferSet( dstFormat );
	targetBufferSet.buffers_.reserve( dstStreamIndex + 1 );
	for (uint32 i = 0; i < dstStreamIndex; ++i)
	{
		targetBufferSet.buffers_.push_back( 
			VertexFormat::Buffer( NULL, 0 ) );
	}
	targetBufferSet.buffers_.push_back( 
		VertexFormat::Buffer( dstData, dstSize) );
	
	// If the conversion process requires multiple streams then convert
	// will fail
	bool converted = convert( targetBufferSet, sourceBufferSet );
	if (!converted && createFailureMessages)
	{
		// On conversion error, enable print message flag so
		// it is verbose about why it couldn't convert
		canConvert( targetBufferSet, sourceBufferSet, true );
	}
	return converted;
}


bool VertexFormat::isSourceDataValid( 
	const VertexFormat & dstFormat, const BufferSet & srcBuffers,
	bool failOnFirstInvalid, bool failOnFirstInvalidPerElement )
{
	const VertexFormat & srcFormat = srcBuffers.format_;

	bool isValid = true;

	uint32 dstElementCount = dstFormat.countElements();
	for (uint32 elementIndex = 0; elementIndex != dstElementCount; ++elementIndex)
	{
		// Grab the element definitions
		SemanticType::Value dstSemantic;
		StorageType::Value dstStorage;
		uint32 dstStreamIndex;
		uint32 dstSemanticIndex;
		bool result = dstFormat.findElement(
			elementIndex, &dstSemantic, &dstStreamIndex, &dstSemanticIndex, 
			NULL, &dstStorage );
		if (!result)
		{
			continue;
		}

		// Now attempt to find the corresponding element from the source stream
		StorageType::Value srcStorage;
		uint32 srcStreamIndex;
		size_t srcStreamOffset;
		result = srcFormat.findElement( dstSemantic, dstSemanticIndex, 
			&srcStreamIndex, &srcStreamOffset, &srcStorage );
		if (!result)
		{
			// This part of the stream can't be copied from the source
			continue;
		}

		// Make sure a validation exists between the types:
		VertexConversions::ValidationFunc validate = 
			VertexConversions::fetchValidationFunction( dstSemantic, dstStorage, srcStorage );

		if (validate == NULL)
		{
			continue;
		}

		if (srcBuffers.buffers_.size() < srcStreamIndex)
		{
			WARNING_MSG( "VertexFormat::isSourceDataValid: Missing source buffer "
				"for source stream index %d. Cannot validate data.\n", srcStreamIndex );
			return false;
		}
		
		RawElementAccessor<uint8> srcAccessor = srcFormat.createRawElementAccessor<uint8>(
			srcBuffers.buffers_[srcStreamIndex].data_, 
			srcBuffers.buffers_[srcStreamIndex].size_, 
			srcStreamIndex,
			srcStreamOffset);
		
		// Perform batch validation of this element, respecting 
		// the early out parameters
		size_t elementCount = srcAccessor.size();
		for (uint32 index = 0; index < elementCount; ++index)
		{
			void* srcData = &srcAccessor[index];
			if (!(*validate)( srcData ))
			{
				isValid = false;
				if (failOnFirstInvalid)
				{
					return false;
				}
				else if (failOnFirstInvalidPerElement)
				{
					break;
				}
			}
		}
	}

	return isValid;
}

bool VertexFormat::load( VertexFormat& format, DataSectionPtr pSection,
						const BW::StringRef & name )
{
	BW_GUARD;
	bool ret = false;
	
	format.name_ = name.to_string();

	DataSectionIterator it = pSection->begin();
	DataSectionIterator end = pSection->end();
	while( it != end )
	{
		DataSectionPtr pDS = *it;
		const BW::string& key = pDS->sectionName();

		if (key == "elements")
		{
			DataSectionIterator elementsIter = pDS->begin();
			DataSectionIterator elementsEnd = pDS->end();
			while( elementsIter != elementsEnd )
			{
				DataSectionPtr elementIter = *elementsIter;
				const BW::string& elementKey = elementIter->sectionName();

				SemanticType::Value semantic = 
					VertexElement::SemanticFromString( elementKey );

				BW::string storageName = elementIter->readString( "type", "FLOAT3" );
				StorageType::Value storage = VertexElement::StorageFromString( storageName );
				uint32 semanticIndex = elementIter->readInt( 
					"semanticIndex", DEFAULT );
				uint32 stream = elementIter->readInt( "stream", 
					(format.streamCount()) ? format.streamCount() - 1 : 0 );
				uint32 offset = elementIter->readInt( "offset", DEFAULT );

				if (storage == StorageType::UNKNOWN)
				{
					WARNING_MSG( "VertexFormat::Load: unknown storage type: %s\n" , storageName.c_str() );
				}

				if (semantic != SemanticType::UNKNOWN)
				{
					while (stream >= format.streamCount())
					{
						format.addStream();
					}
					format.addElement( stream, semantic, storage, 
						semanticIndex, offset );
					ret = true;
				}

				++elementsIter;
			}
		}
		else if (key == "targets")
		{
			DataSectionIterator targetsIter = pDS->begin();
			DataSectionIterator targetsEnd = pDS->end();
			while( targetsIter != targetsEnd )
			{
				DataSectionPtr targetIter = *targetsIter;

				BW::string formatTarget = targetIter->readString( "format", "" );
				if (!formatTarget.empty())
				{
					BW::string targetName = targetIter->readString( "name", "" );
					format.addFormatTarget( targetName, formatTarget );
				}
				++targetsIter;
			}
		}
		it++;
	}

	return ret;
}


bool VertexFormat::merge( VertexFormat& format, const VertexFormat& extra )
{
	// Iterate over the extra's streams and add it's elements
	uint32 numStreams = extra.streamCount();
	for (uint32 streamIndex = 0; streamIndex < numStreams; ++streamIndex)
	{
		// Grab each element of this stream and add it to the same stream 
		// in the merged format.
		uint32 numElements = extra.countElements( streamIndex );
		if (numElements == 0)
		{
			continue;
		}

		// Make sure that this extra stream that we are adding to the format
		// does not already exist in the original format, otherwise
		// we would need to define some kind of append or interleave
		// behavior in the merge. Not going to define this behavior now
		// as no use cases require it.
		MF_ASSERT( format.countElements( streamIndex ) == 0 );

		while (format.streamCount() <= streamIndex)
		{
			format.addStream();
		}

		for (uint32 elementIndex = 0; elementIndex < numElements; ++elementIndex)
		{
			VertexElement::StorageType::Value storageType;
			VertexElement::SemanticType::Value semantic;
			uint32 semanticIndex;
			size_t offset;
			if (!extra.findElement(streamIndex, elementIndex, &semantic, &semanticIndex, &offset, &storageType))
			{
				return false;
			}
			
			format.addElement(streamIndex, semantic, storageType, semanticIndex, offset);
		}
	}

	return true;
}


BW::StringRef VertexFormat::getTargetFormatName( 
	const BW::StringRef & targetName ) const
{
	TargetFormatMapping::const_iterator targetIter = 
		targets_.find( targetName );
	if (targetIter != targets_.end())
	{
		return targetIter->second;
	}

	return BW::StringRef();
}

void VertexFormat::addFormatTarget( const BW::StringRef & targetName, 
	const BW::StringRef & formatName )
{
	targets_[targetName] = formatName.to_string();
}


VertexFormat::ElementDefinition::ElementDefinition(
	SemanticType::Value semantic, 
	uint32 semanticIndex,
	StorageType::Value storage,
	size_t offset)
	: semantic_( semantic )
	, semanticIndex_( semanticIndex )
	, storage_( storage )
	, offset_( offset )
{

}


VertexFormat::StreamDefinition::StreamDefinition()
	: stride_( 0 )
{

}


VertexFormat::Buffer::Buffer( void* data, size_t size )
	: data_( data )
	, size_( size )
{

}


VertexFormat::BufferSet::BufferSet( const VertexFormat& format )
	: format_( format )
{

}

VertexFormat::ConversionContext::ConversionContext()
: dstFormat_(NULL)
, srcFormat_(NULL)
{
}

VertexFormat::ConversionContext::ConversionContext( const VertexFormat * dstFormat, const VertexFormat * srcFormat )
: dstFormat_(dstFormat)
, srcFormat_(srcFormat)
{
	if (!dstFormat_ && srcFormat_)
	{
		dstFormat_ = srcFormat_;
	}
}

// returns src format if only initialised with src format 
const VertexFormat * VertexFormat::ConversionContext::dstFormat() const
{
	return dstFormat_;
}

const VertexFormat * VertexFormat::ConversionContext::srcFormat() const
{
	return srcFormat_;
}


bool VertexFormat::ConversionContext::isValid() const
{
	// check non null pointers
	if (!srcFormat_ || !dstFormat_)	
	{
		return false;
	}

	// check at least 1 stream available
	if ( srcFormat_->streamCount() < 1
		|| dstFormat_->streamCount() < 1)
	{
		return false;
	}

	return true;
}


const uint32 VertexFormat::ConversionContext::srcVertexSize( uint32 streamIndex ) const
{
	// use stride of first stream if available. 0 on error.
	return srcFormat_? srcFormat_->streamStride( streamIndex ) : 0;
}

const uint32 VertexFormat::ConversionContext::dstVertexSize( uint32 streamIndex ) const
{
	return dstFormat_? dstFormat_->streamStride( streamIndex ) : 0;
}

bool VertexFormat::ConversionContext::isConversionRequired() const
{
	if (!isValid() || 
		srcFormat_->equals(*dstFormat_))
	{
		return false;
	}
	return true;
}


bool VertexFormat::ConversionContext::convertSingleStream( void * dstData, const void * srcData,  
						 size_t numVertices ) const
{
	return this->convertSingleStream( dstData, 0, srcData, 0, numVertices );
}

bool VertexFormat::ConversionContext::convertSingleStream( void * dstData, uint32 dstStreamIndex, 
														  const void * srcData, uint32 srcStreamIndex, 
														  size_t numVertices ) const
{
	if (!isValid())
	{
		WARNING_MSG("ConversionContext::convertSingleStream: tried to convert between invalid formats\n");
		return false;
	}

	size_t srcBufferSize = this->srcVertexSize( srcStreamIndex ) * numVertices;
	size_t dstBufferSize = this->dstVertexSize( dstStreamIndex ) * numVertices;

	if (!srcBufferSize || !dstBufferSize)
	{
		WARNING_MSG("ConversionContext::convertSingleStream: empty or invalid stream indexes\n");
		return false;
	}

	if (isConversionRequired())
	{
		return convertBufferStream( 
			*srcFormat_, srcData, srcBufferSize, srcStreamIndex, 
			*dstFormat_, dstData, dstBufferSize, dstStreamIndex, true );
	}
	else
	{
		MF_ASSERT( dstBufferSize == srcBufferSize );
		memcpy( dstData, srcData, dstBufferSize );
		return true;
	}
}

} // namespace Moo

BW_END_NAMESPACE

// vertex_declaration.cpp
