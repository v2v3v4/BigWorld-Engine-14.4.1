#include "primitive_processor.hpp"

#include "asset_pipeline/compiler/compiler.hpp"
#include "asset_pipeline/discovery/conversion_rule.hpp"
#include "asset_pipeline/dependency/dependency_list.hpp"
#include "cstdmf/bw_hash64.hpp"
#include "cstdmf/bw_string_ref.hpp"
#include "cstdmf/guard.hpp"
#include "cstdmf/string_builder.hpp"
#include "cstdmf/command_line.hpp"
#include "cstdmf/fourcc.hpp"
#include "moo/visual_common.hpp"
#include "moo/visual.hpp"
#include "moo/visual_manager.hpp"
#include "moo/primitive.hpp"
#include "moo/primitive_manager.hpp"
#include "moo/vertices.hpp"
#include "moo/vertices_manager.hpp"
#include "moo/vertex_format_cache.hpp"
#include "moo/vertex_streams.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/bin_section.hpp"
#include "resmgr/binary_block.hpp"
#include "resmgr/quick_file_writer.hpp"


BW_BEGIN_NAMESPACE

const uint64 PrimitiveProcessor::s_TypeId = Hash64::compute( PrimitiveProcessor::getTypeName() );
const char * PrimitiveProcessor::s_Version = "2.8.3";

namespace
{
	void copySectionToOutput( const DataSectionPtr& pOrigSection, 
		const DataSectionPtr& pOutput )
	{
		pOutput->writeBinary( pOrigSection->sectionName(),
			pOrigSection->asBinary() );
	}

	bool sectionNameEndsWith( const BW::string & sectionName, const BW::StringRef & endMatch )
	{
		if (sectionName.length() < endMatch.length())
		{
			return false;
		}
		if (sectionName.compare( sectionName.length() - endMatch.length(),
			endMatch.length(), endMatch.to_string() ) == 0)
		{
			return true;
		}
		return false;
	}

	// Validate source data to make sure it is valid for the conversion
	bool validateVerticesSingleStream( 	const BW::string& filename,	const DataSectionPtr& pOrigSection, 
		const BW::StringRef & srcFormatName, const BW::StringRef & dstFormatName, 
		const void * pSrcData, uint32 numVerts, uint32 streamIndex )
	{
		// get formats using name
		const Moo::VertexFormat * pSrcFormat = Moo::VertexFormatCache::get( srcFormatName ); 
		const Moo::VertexFormat * pDstFormat = Moo::VertexFormatCache::get( dstFormatName ); 
		MF_ASSERT( pSrcFormat && pDstFormat && pSrcData );
		MF_ASSERT( streamIndex < pSrcFormat->streamCount() );

		// prepare source buffers
		Moo::VertexFormat::BufferSet srcBuffer( *pSrcFormat );
		srcBuffer.buffers_.reserve( streamIndex + 1 );
		for (uint32 i = 0; i < streamIndex; ++i)
		{
			srcBuffer.buffers_.push_back( Moo::VertexFormat::Buffer( NULL, 0 ) );
		}
		uint32 srcBufferSize = numVerts * pSrcFormat->streamStride( streamIndex );
		srcBuffer.buffers_.push_back( 
			Moo::VertexFormat::Buffer( (void*)pSrcData, srcBufferSize ) );

		// perform the actual check
		if (!Moo::VertexFormat::isSourceDataValid( *pDstFormat, srcBuffer, false, true ))
		{
			WARNING_MSG(
				"Source data for primitive '%s' in section '%s' contains potentially problematic"
				" vertex values for conversion from format '%s' to '%s'.\n", 
				filename.c_str(), pOrigSection->sectionName().c_str(),
				srcFormatName.to_string().c_str(), dstFormatName.to_string().c_str() );
			return false;
		}

		return true;
	}

	// If numVerts is non zero, check buffer length, otherwise 
	// use buffer length to determine vertex count.
	bool convertVertexStream( 
		const Moo::VertexStreamDesc & streamDesc, 
		const BW::string & filename, uint32 numVerts, 
		const DataSectionPtr & pOrigSection, 
		const DataSectionPtr & pOutput, const BW::string & targetDevice )
	{
		BinaryPtr streamBinary = pOrigSection->asBinary();
		const BW::string streamName = filename + "/" + pOrigSection->sectionName();
		const void * pSrcBuffer = streamBinary->cdata();

		// Set up a vertex format conversion
		const Moo::VertexFormat * defaultSrcFormat = streamDesc.defaultVertexFormat();
		MF_ASSERT( defaultSrcFormat );
		const Moo::VertexFormat & srcFormat = *defaultSrcFormat;
		const Moo::VertexFormat & dstFormat = 
			*Moo::VertexFormatCache::getTarget( srcFormat, targetDevice, true );
		
		Moo::VertexFormat::ConversionContext vfConversion( &dstFormat, &srcFormat );
		if( !vfConversion.isValid() )
		{
			ERROR_MSG( "No valid conversion was found for vertex stream in file: %s", 
				streamName.c_str() );
			return false;
		}

		const uint32 streamIndex = streamDesc.streamIndex_;

		// Check length of source buffer. no header for vertex streams
		const uint32 srcLength = streamBinary->len();
		const uint32 srcVertexSize = vfConversion.srcVertexSize( streamIndex );
		MF_ASSERT( srcVertexSize );

		if (numVerts == 0)
		{
			// Determine number of verts from length of the source buffer
			numVerts = srcLength/srcVertexSize;
		}
		else
		{
			// We were provided a verts count. verify against this.
			const uint32 expectedSrcLength = numVerts * srcVertexSize;
			if (srcLength < expectedSrcLength)
			{
				ERROR_MSG( "Vertex stream length in file: %s\n"
					"shorter than expected: %d, got: %d", 
					streamName.c_str(), expectedSrcLength, srcLength );
				return false;
			}
			else if (srcLength > expectedSrcLength)
			{
				WARNING_MSG( "Vertex stream length in file: %s\n"
					"longer than expected: %d, got: %d", 
					streamName.c_str(), expectedSrcLength, srcLength );
				return false;
			}
		}

		// Create destination buffer.
		uint32 dstVertexSize = vfConversion.dstVertexSize( streamIndex );
		uint32 dstBufferSize = dstVertexSize * numVerts;
		if ( !dstBufferSize )
		{
			ERROR_MSG( 
				"No valid destination buffer can be created with (numVerts=%d, vertSize=%d) "
				"for vertex stream in file: %s", 
				numVerts, dstVertexSize, streamName.c_str() );
			return false;
		}

		// Validate source data to make sure it is valid for the conversion
		validateVerticesSingleStream( filename, pOrigSection, srcFormat.name(), dstFormat.name(), 
			pSrcBuffer, numVerts, streamIndex );

		// Perform the conversion
		void * pDstBuffer = bw_malloc( dstBufferSize );
		bool converted = vfConversion.convertSingleStream( 
			pDstBuffer, streamIndex, pSrcBuffer, streamIndex, numVerts );

		if (!converted)
		{
			ERROR_MSG( "Vertex format conversion failed. %s",
				streamName.c_str() );
			bw_free( pDstBuffer );
			return false;
		}

		// Prepare header
		Moo::ProcessedVertexHeader procVertHeader;
		memset( &procVertHeader, 0, sizeof(Moo::ProcessedVertexHeader) );
		procVertHeader.magic_ = Moo::HeaderMagic::ProcessedVertexStream;
		strcpy( procVertHeader.originalVertexFormat_, 
			srcFormat.name().to_string().c_str() );
		strcpy( procVertHeader.vertexHeader_.vertexFormat_, 
			dstFormat.name().to_string().c_str() );
		procVertHeader.vertexHeader_.nVertices_ = numVerts;

		// Output processed data to disk
		QuickFileWriter f;
		f << procVertHeader;
		f.write( pDstBuffer, dstBufferSize );
		pOutput->writeBinary( pOrigSection->sectionName(), f.output() );
		bw_free( pDstBuffer );

		return true;
	}

	bool convertVertices( const BW::string& filename,
		const DataSectionPtr& pOrigSection, 
		const DataSectionPtr& pOutput, const BW::string& targetDevice )
	{
		// Open the original section and cause a conversion
		BW::string verticesName = filename + "/" + pOrigSection->sectionName();
		
		BinaryPtr vertBinary = pOrigSection->asBinary();
		const Moo::VertexHeader* pVH = 
			reinterpret_cast< const Moo::VertexHeader* >( vertBinary->data() );
		const void* pSrcVertices = reinterpret_cast<const void*>( pVH + 1 );
		
		uint32 maxFormatLength = 
			ARRAY_SIZE( pVH->vertexFormat_ ) - 1;
		if (strnlen(pVH->vertexFormat_, maxFormatLength + 1) >= maxFormatLength )
		{
			ERROR_MSG( "Invalid format string name in file: %s", 
				verticesName.c_str() );
			return false;
		}

		// Attempt to fetch the relevant vertex format
		BW::string srcFormatName( pVH->vertexFormat_ );
		const Moo::VertexFormat* pSrcFormat = 
			Moo::VertexFormatCache::get( srcFormatName );
		if (!pSrcFormat)
		{
			ERROR_MSG( "Unable to find source format definition: %s in file: %s", 
				pVH->vertexFormat_,
				verticesName.c_str() );
			return false;
		}

		// Attempt to fetch the target vertex format
		BW::StringRef dstFormatName = 
			pSrcFormat->getTargetFormatName( targetDevice );
		if (dstFormatName.empty())
		{
			// No conversion required, but save with a consistent format
			dstFormatName = pVH->vertexFormat_;
		}
		const Moo::VertexFormat* pDstFormat = 
			Moo::VertexFormatCache::get( dstFormatName );
		if (!pDstFormat)
		{
			ERROR_MSG( "Unable to find target format definition for format:"
				" %s, target %s in file: %s", 
				pVH->vertexFormat_,
				targetDevice.c_str(),
				verticesName.c_str() );
			return false;
		}

		if (dstFormatName.size() > maxFormatLength)
		{
			ERROR_MSG( "Format name is greater than supported length"
				", max length is %d. sourceFormat: %s. destFormat: %d. "
				"vertices in: %s", 
				maxFormatLength,
				pVH->vertexFormat_,
				dstFormatName.to_string().c_str(),
				verticesName.c_str() );
			return false;
		}

		// Now check the length of the source stream
		const uint32 streamIndex = 0;
		uint32 srcVertBufferLength = 
			vertBinary->len() - sizeof( Moo::VertexHeader );
		uint32 srcStreamLength = pVH->nVertices_ * pSrcFormat->streamStride( streamIndex );
		if (srcVertBufferLength != srcStreamLength )
		{
			ERROR_MSG( "Unexpected vertex stream length in file: %s\n"
				"expected: %d, got: %d", 
				verticesName.c_str(),
				srcStreamLength,
				srcVertBufferLength );
			return false;
		}


		// Validate source data to make sure it is valid for the conversion
		validateVerticesSingleStream( filename, pOrigSection, srcFormatName, dstFormatName, 
			pSrcVertices, pVH->nVertices_, streamIndex );
		
		// Now setup a conversion between the two streams
		uint32 dstStreamLength = pVH->nVertices_ * pDstFormat->streamStride( streamIndex );
		void* pDstBuffer = bw_malloc( dstStreamLength );

		if (!Moo::VertexFormat::convertBuffer( *pSrcFormat, pSrcVertices, 
			srcStreamLength, *pDstFormat, pDstBuffer, dstStreamLength,
			true))
		{
			ERROR_MSG( "Vertex format conversion failed. %s",
				verticesName.c_str() );
			bw_free( pDstBuffer );
			return false;
		}

		// Now output the data to disk again
		Moo::ProcessedVertexHeader procVertHeader;
		memset( &procVertHeader, 0, sizeof(Moo::ProcessedVertexHeader) );
		procVertHeader.magic_ = Moo::HeaderMagic::ProcessedVertices;
		
		strcpy( procVertHeader.originalVertexFormat_, pVH->vertexFormat_);
		strcpy( procVertHeader.vertexHeader_.vertexFormat_, 
			dstFormatName.to_string().c_str());
		procVertHeader.vertexHeader_.nVertices_ = pVH->nVertices_;

		QuickFileWriter f;
		f << procVertHeader;
		f.write( pDstBuffer, dstStreamLength );

		pOutput->writeBinary( pOrigSection->sectionName(), f.output() );

		bw_free( pDstBuffer );

		return true;
	}

	bool convertPrimitives( const BW::string& filename, 
		const BW::string& outputFile, const BW::string& targetDevice )
	{
		DataSectionPtr pPrimFile = BWResource::openSection( filename );
		if (!pPrimFile)
		{
			ERROR_MSG( "Unable to open primitives file %s", filename.c_str() );
			return false;
		}

		// Create output file
		DataSectionPtr pOutput = new BinSection( outputFile, 
			new BinaryBlock( 0, 0, "BinaryBlock/PrimitiveProcessor" ) );
		
		// Iterate over each section in the primitives file looking for
		// convertible sections by matching endings.
		const BW::StringRef morphVerticesEnding = "mvertices";
		const BW::StringRef verticesEnding = 
			morphVerticesEnding.to_string().c_str() + 1;

		DataSectionIterator sit = pPrimFile->begin();
		DataSectionIterator send = pPrimFile->end();
		while (sit != send)
		{
			DataSectionPtr pSection = *sit;
			++sit;

			BW::string name = pSection->sectionName();
			std::transform( name.begin(), name.end(), name.begin(),
				[]( char i ){ return std::tolower(i); } );

			bool foundStream = false;

			// try matching known vertex stream sections
			typedef Moo::VertexStreamDesc VSDesc;
			const VSDesc::StreamDescs & streamDescs = VSDesc::getKnownStreams();
			for (uint32 sdIndex = 0; sdIndex < streamDescs.size(); ++sdIndex)
			{
				const VSDesc & streamDesc = streamDescs[sdIndex];
				if (sectionNameEndsWith( name, streamDesc.fileSectionName_ ))
				{
					// Found a vertex stream, perform conversion
					foundStream = true;
					if (!convertVertexStream( streamDesc, filename, 
						0, pSection, pOutput, targetDevice ))
					{
						return false;
					}
					continue;
				}
			}

			if (foundStream)
			{
				// already converted a vertex stream
				continue;
			}

			// simply copy any remaining cases that don't match [m]vertices ending
			if (!sectionNameEndsWith( name, verticesEnding ))
			{
				copySectionToOutput( pSection, pOutput );
			}

			// Morph vertices are not supported currently! 
			// shares same suffix, so check and skip explicitly
			else if (sectionNameEndsWith( name, morphVerticesEnding ))
			{
				copySectionToOutput( pSection, pOutput );
			}
			else
			{
				// This must be a vertices section, execute conversion
				if (!convertVertices( filename, pSection, pOutput, targetDevice ))
				{
					return false;
				}
			}
		}
		
		pOutput->save( outputFile );

		return true;
	}

	bool huntVertexDependencies(
		const BW::string& filename, 
		const DataSectionPtr& pOrigSection, 
		const BW::string& targetDevice,
		BW::vector<BW::string>& dependencies )
	{
		BW::string verticesName = filename + "/" + pOrigSection->sectionName();

		BinaryPtr vertBinary = pOrigSection->asBinary();
		const Moo::VertexHeader* pVH = 
			reinterpret_cast< const Moo::VertexHeader* >( vertBinary->data() );
		const void* pSrcVertices = reinterpret_cast<const void*>( pVH + 1 );

		uint32 maxFormatLength = 
			ARRAY_SIZE( pVH->vertexFormat_ ) - 1;
		if (strnlen(pVH->vertexFormat_, maxFormatLength + 1) >= maxFormatLength )
		{
			ERROR_MSG( "Invalid format string name in file: %s", 
				verticesName.c_str() );
			return false;
		}

		// Attempt to fetch the relevant vertex format
		const BW::string srcFormatFilename = 
			Moo::VertexFormatCache::getResourcePath( pVH->vertexFormat_ );
		dependencies.push_back( srcFormatFilename );

		const Moo::VertexFormat* pSrcFormat = 
			Moo::VertexFormatCache::get( pVH->vertexFormat_ );
		if (!pSrcFormat)
		{
			ERROR_MSG( "Unable to find source format definition: %s in file: %s", 
				pVH->vertexFormat_,
				verticesName.c_str() );
			return false;
		}

		// Attempt to fetch the target vertex format
		BW::StringRef dstFormatName = 
			pSrcFormat->getTargetFormatName( targetDevice );
		if (dstFormatName.empty())
		{
			// No conversion required, but save with a consistent format
			dstFormatName = pVH->vertexFormat_;
		}
		const BW::string dstFormatFilename = 
			Moo::VertexFormatCache::getResourcePath( dstFormatName );
		dependencies.push_back( dstFormatFilename );

		return true;
	}

	bool huntPrimitiveDependencies( const BW::string& filename, 
		const BW::string& targetDevice,
		BW::vector<BW::string>& dependencies )
	{
		DataSectionPtr pPrimFile = BWResource::openSection( filename );
		if (!pPrimFile)
		{
			ERROR_MSG( "Unable to open primitives file %s", filename.c_str() );
			return false;
		}

		// Iterate over each section in the primitives file looking for
		// sections ending in "vertices or mvertices". 
		const char* matchEnding = "vertices";
		size_t matchEndingLength = strlen( matchEnding );

		DataSectionIterator sit = pPrimFile->begin();
		DataSectionIterator send = pPrimFile->end();
		while (sit != send)
		{
			DataSectionPtr pSection = *sit;
			++sit;

			BW::string name = pSection->sectionName();
			std::transform( name.begin(), name.end(), name.begin(),
				[]( char i ){ return std::tolower(i); } );
			if (name.length() < matchEndingLength)
			{
				continue;
			}

			if (name.compare( name.length() - matchEndingLength,
				matchEndingLength, matchEnding ) != 0)
			{
				// Did not match the ending
				continue;
			}

			if ((name.length() >= matchEndingLength + 1) &&
				name.compare( name.length() - (matchEndingLength + 1),
				matchEndingLength + 1, "mvertices" ) == 0)
			{
				// Morph vertices not supported currently!
				continue;
			}

			// Found a vertices section, so execute conversion
			huntVertexDependencies( filename, 
				pSection, targetDevice, dependencies );
		}

		return true;
	}
}

/* construct a converter with parameters. */
PrimitiveProcessor::PrimitiveProcessor( const BW::string & params ) :
	Converter( params )
{
	BW_GUARD;
	
	CommandLine cmd( params.c_str() );
	BW::string targets = cmd.getParam( "target" );
	for (char* token = strtok( const_cast<char*>( targets.c_str() ), ";" );
		token != NULL;
		token = strtok( NULL, ";" ) )
	{
		targets_.push_back( token );
	}
	if (targets_.empty())
	{
		targets_.push_back( "D3D9" );
	}

	MF_ASSERT( targets_.size() >= 1 );
}

PrimitiveProcessor::~PrimitiveProcessor()
{

}

/* builds the dependency list for a source file. */
bool PrimitiveProcessor::createDependencies( const BW::string & sourcefile, 
	const Compiler & compiler,
	DependencyList & dependencies )
{
	BW_GUARD;

	MF_ASSERT( !targets_.empty() );
	if (targets_.size() > 1)
	{
		// Different output files would need to be configured if there are
		// multiple targets. Currently there isn't a well defined
		// strategy to deal with such a situation, since there is also
		// the case where DX9 and DX10 want to share data. In that case
		// we wouldn't want to have different files for them since that would
		// duplicate data.
		ERROR_MSG( "Only a single target device is currently supported" );
		return false;
	}

	BW::vector<BW::string> dependencyFilenames;

	bool success = true;
	for (BW::vector<BW::string>::const_iterator iter = targets_.begin();
		iter != targets_.end(); ++iter)
	{
		const BW::string& targetName = *iter;

		success &= huntPrimitiveDependencies( sourcefile, 
			targetName, dependencyFilenames );
	}

	for (BW::vector<BW::string>::iterator iter = dependencyFilenames.begin();
		iter != dependencyFilenames.end(); ++iter)
	{
		BW::string& dependency = *iter;
		compiler.resolveRelativePath( dependency );
		dependencies.addSecondarySourceFileDependency( dependency, true );
	}

	return success;
}

/* convert a source file. */
/* work out the output filename & insert it into the converted files vector */
bool PrimitiveProcessor::convert( const BW::string & sourcefile,
	const Compiler & compiler,
	BW::vector< BW::string > & intermediateFiles,
	BW::vector< BW::string > & outputFiles )
{
	BW_GUARD;

	MF_ASSERT( !targets_.empty() );
	if (targets_.size() > 1)
	{
		// Different output files would need to be configured if there are
		// multiple targets. Currently there isn't a well defined
		// strategy to deal with such a situation, since there is also
		// the case where DX9 and DX10 want to share data. In that case
		// we wouldn't want to have different files for them since that would
		// duplicate data.
		ERROR_MSG( "Only a single target device is currently supported" );
		return false;
	}

	bool success = true;
	for (BW::vector<BW::string>::const_iterator iter = targets_.begin();
	iter != targets_.end(); ++iter)
	{
		const BW::string& targetName = *iter;

		BW::string outputFilename = sourcefile + ".processed";
		compiler.resolveOutputPath( outputFilename );
		BW::BWResource::ensurePathExists( outputFilename );

		success &= convertPrimitives( sourcefile, outputFilename, 
			targetName );

		outputFiles.push_back( outputFilename );
	}

	return success;
}
BW_END_NAMESPACE
