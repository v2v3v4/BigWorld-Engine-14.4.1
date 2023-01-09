
#include "pch.hpp"
#include "vertices.hpp"

#include "cstdmf/fourcc.hpp"
#include "cstdmf/debug.hpp"
#include "dynamic_vertex_buffer.hpp"
#include "primitive_file_structs.hpp"
#include "render_context.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/primitive_file.hpp"
#include "software_skinner.hpp"
#include "vertex_formats.hpp"
#include "vertices_manager.hpp"
#include "vertex_format_cache.hpp"

#include "vertex_streams.hpp"

DECLARE_DEBUG_COMPONENT2( "Moo", 0 )


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "vertices.ipp"
#endif

// Vertex Format Refactor:
//
// TODO: Remove format string checks, once all remaining format string switches
//		 from related code have been replaced with VertexFormat logic.
// 
//		 For now, we check vertex formats are in expected lists. 
//		 This is just to mimic the "rejection" behaviour that existed prior 
//		 to the VertexFormat additions. However, once all VertexFormat 
//		 replacements are made, these checks will no longer be necessary, 
//		 since we will be able to operate on vertex formats generically.

namespace Moo
{

Vertices::Vertices( const BW::string& resourceID, int numNodes )
: resourceID_( resourceID ),
  nVertices_( 0 ),
  formatSupportsSoftwareSkinner_( false ),
  vbBumped_( false ),
  pDecl_( NULL ),
  pInstancedDecl_( NULL ),
  streams_( NULL ),
  vertexBufferOffset_( 0 ),
  vertexBufferSize_( 0 ),
#if ENABLE_RELOAD_MODEL
  isInVerticesManger_( true ),
#endif //ENABLE_RELOAD_MODEL
  numNodes_( numNodes * 3 /* indices are pre-multiplied by 3 */ )
{
	BW_GUARD;	
}

Vertices::~Vertices()
{
	BW_GUARD;

	if (streams_)
	{
		bw_safe_delete(streams_);
	}
}


void Vertices::destroy() const
{
	BW_GUARD;
#if ENABLE_RELOAD_MODEL
	bool isInManager = isInVerticesManger_;
#else
	bool isInManager = true;
#endif

	VerticesManager::tryDestroy( this, isInManager );
}

/// This function returns false if vertices have been loaded in the provided 
/// Vertices instance. More specifically, false means that the instance is ready
/// for format-based vertex operations, since the related members are available.
///
/// We don't make this a const member function for now since we don't want to
/// falsely advertise ".isEmpty()" functionality to other code or subclasses
/// For now it is just an implementation-specific helper that makes sense only
/// for the scope it is used in.
bool isEmpty( const Vertices & instance )
{
	return ( instance.nVertices() == 0 || 
		!instance.vertexBuffer().valid() || 
		instance.pDecl() == NULL );
}

#ifdef EDITOR_ENABLED


/// copies vertex normals as uint32 using VertexFormat
void Vertices::copyVertexNormals( const VertexFormat & vertexFormat, 
	VertexNormals & vertexNormals, const void * pVerts, uint32 nVertices )
{
	BW_GUARD;

	if (nVertices == 0)
	{
		return;
	}

	MF_ASSERT( vertexFormat.streamCount() > 0 );
	uint32 vertexSize = vertexFormat.streamStride( 0 );
	MF_ASSERT( vertexSize );

	VertexElement::StorageType::Value normalsStorageType = 
		VertexElementTypeTraits< VertexNormals::value_type >::s_storageType;

	// setup a target format for buffer conversion
	VertexFormat normalsFormat;
	normalsFormat.addStream();
	normalsFormat.addElement( 0, VertexElement::SemanticType::NORMAL, 
		normalsStorageType );
	size_t normalsVertexSize = normalsFormat.streamStride( 0 );
	
	vertexNormals.resize( nVertices );

	bool converted = VertexFormat::convertBuffer( 
		vertexFormat, pVerts, vertexSize * nVertices,
		normalsFormat, &vertexNormals[0], normalsVertexSize * nVertices, true );

	MF_ASSERT( converted );
}

/// copies vertex normals as uint32 using VertexFormat
void Vertices::copyVertexNormals( const VertexFormat& vertexFormat, 
	VertexPositions& vertexNormals, const void* pVerts, uint32 nVertices )
{
	BW_GUARD;
	
	if (nVertices == 0)
	{
		return;
	}

	MF_ASSERT( vertexFormat.streamCount() > 0 );
	uint32 vertexSize = vertexFormat.streamStride( 0 );
	MF_ASSERT( vertexSize );

	VertexElement::StorageType::Value normalsStorageType = 
		VertexElementTypeTraits< VertexPositions::value_type >::s_storageType;

	// setup a target format for buffer conversion
	VertexFormat normalsFormat;
	normalsFormat.addStream();
	normalsFormat.addElement( 0, VertexElement::SemanticType::NORMAL, 
		normalsStorageType );
	size_t normalsVertexSize = normalsFormat.streamStride( 0 );

	vertexNormals.resize( nVertices );

	bool converted = VertexFormat::convertBuffer( 
		vertexFormat, pVerts, vertexSize * nVertices,
		normalsFormat, &vertexNormals[0], normalsVertexSize * nVertices, true );

	MF_ASSERT( converted );
}

/// copies vertex normals, tangents, bi-normals as uint32 triplets 
void Vertices::copyTangentSpace( const VertexFormat& vertexFormat, 
	VertexNormals & vertexNormals, const void * pVerts, uint32 nVertices )
{
	BW_GUARD;

	if (nVertices == 0)
	{
		return;
	}

	MF_ASSERT( vertexFormat.streamCount() > 0 );
	uint32 vertexSize = vertexFormat.streamStride( 0 );
	MF_ASSERT( vertexSize );

	typedef VertexElement::SemanticType SemType;

	VertexElement::StorageType::Value normalsStorageType = 
		VertexElementTypeTraits< VertexNormals::value_type >::s_storageType;

	VertexFormat normalsFormat;
	normalsFormat.addStream();
	normalsFormat.addElement( 0, SemType::NORMAL, normalsStorageType );
	normalsFormat.addElement( 0, SemType::TANGENT, normalsStorageType );
	normalsFormat.addElement( 0, SemType::BINORMAL, normalsStorageType );
	size_t normalsVertexSize = normalsFormat.streamStride( 0 );

	vertexNormals.resize( nVertices * 3 );

	bool converted = VertexFormat::convertBuffer( 
		vertexFormat, pVerts, vertexSize * nVertices,
		normalsFormat, &vertexNormals[0], normalsVertexSize * nVertices, true );

	MF_ASSERT( converted );
}

bool Vertices::cacheVertexNormals()
{
	if ( isEmpty( *this ) )
	{
		ERROR_MSG( "Vertices::cacheVertexNormals: vertices aren't loaded.\n" );
		return false;
	}

	UINT bufferSize = vertexStride_ * nVertices_;
	Moo::SimpleVertexLock vertexLock( vertexBuffer_, 0, bufferSize, 0 );
	MF_ASSERT( vertexLock );

	const VertexFormat& vertexFormat = pDecl()->format();
	typedef VertexElement::SemanticType SemType;

	if (hasBumpedFormat())
	{
		// TODO: Remove (see vertex format TODO at top of file)
		if (!(sourceFormat() == "xyznuvtb" || 
			sourceFormat() == "xyznuv2tb" ||
			sourceFormat() == "xyznuviiiwwtb"))
		{
			WARNING_MSG( "Vertices::cacheVertexNormals: "
				"unexpected bumped format: '%s\n'", sourceFormat().c_str() );
		}

		copyTangentSpace( vertexFormat, vertexNormals_, vertexLock, 
			nVertices_ );
		return true;
	}
	else if (vertexFormat.containsElement( SemType::NORMAL, 0 ) && 
		vertexFormat.containsElement( SemType::BLENDINDICES, 0 ))
	{
		// TODO: Remove (see vertex format TODO at top of file)
		if (!(sourceFormat() == "xyznuviiiww"))
		{
			WARNING_MSG( "Vertices::cacheVertexNormals: "
				"unexpected skinned non-bumped normal format: '%s\n'", 
				sourceFormat().c_str() );
		}

		copyVertexNormals( vertexFormat, vertexNormals3_, vertexLock, 
			nVertices_ );
		return true;
	}
	else if (vertexFormat.containsElement( SemType::NORMAL, 0 ))
	{
		// TODO: Remove (see vertex format TODO at top of file)
		if (!(sourceFormat() == "xyznuv" ||
			sourceFormat() == "xyznduv"))
		{
			WARNING_MSG( "Vertices::cacheVertexNormals: "
				"unexpected non-bumped normal format: '%s\n'", 
				sourceFormat().c_str() );
		}

		copyVertexNormals( vertexFormat, vertexNormals2_, vertexLock, 
			nVertices_ );
		return true;
	}

	ASSET_MSG( "Vertices::cacheVertexNormals: normals not copied "
		"since vertex format '%s' does not have them.\n", sourceFormat().c_str() );
	return false;
}


const Vertices::VertexNormals & Vertices::vertexNormals() 
{ 
	// If there are vertices that normals can be copied from,
	// and if they havent been copied yet, do so.
	if (!isEmpty( *this ) && vertexNormals_.empty())
	{
		cacheVertexNormals();
	}
	return vertexNormals_; 
}


const Vertices::VertexPositions & Vertices::vertexNormals2() 
{ 
	if (!isEmpty( *this ) && vertexNormals2_.empty())
	{
		cacheVertexNormals();
	}
	return vertexNormals2_; 
}


const Vertices::VertexNormals & Vertices::vertexNormals3() 
{ 
	if (!isEmpty( *this ) && vertexNormals3_.empty())
	{
		cacheVertexNormals();
	}
	return vertexNormals3_; 
}

#endif //EDITOR_ENABLED

bool Vertices::verifyFixupBlendIndices( const VertexFormat& vertexFormat, 
	const void* vertices, uint32 nVertices, uint32 nIndices, 
	uint32 nBoneNodes, uint32 streamIndex, uint32 semanticIndex )
{
	BW_GUARD;

	if ( nBoneNodes == 0 || nIndices == 0 )
	{
		return true;
	}

	typedef VertexElement::Ubyte3 AccessType;
	MF_ASSERT( AccessType::component_count >= nIndices );
	MF_ASSERT( vertexFormat.streamCount() > streamIndex );

	typedef VertexElement::SemanticType SemType;

	// Create the Accessor
	uint32 vertexSize = vertexFormat.streamStride( streamIndex );
	ProxyElementAccessor<SemType::BLENDINDICES> blendIndicesAccessor =
		vertexFormat.createProxyElementAccessor<SemType::BLENDINDICES>( 
		vertices, vertexSize*nVertices, streamIndex, semanticIndex );
	MF_ASSERT( blendIndicesAccessor.isValid() );

	// Check/fix blend index data
	bool validData = true;
	bool badval = false;
	for (uint32 vertexIndex = 0; vertexIndex < nVertices; ++vertexIndex)
	{
		// read value
		AccessType val = blendIndicesAccessor[vertexIndex];

		// check/fix value's components
		for (uint32 blendIndex = 0; blendIndex < nIndices; ++blendIndex)
		{
			if ( val[blendIndex] < 0 || val[blendIndex] >= nBoneNodes )
			{
				val[blendIndex] = 0;
				badval = true;
			}
		}
		
		// write back fixed value
		if (badval)
		{
			blendIndicesAccessor[vertexIndex] = val;
			badval = false;
			validData = false;
		}
	}
	return validData;
}


namespace {

/**
 * This function returns true if the provided source format name
 * should require a verify / fix up of blend indices values.
 */
bool shouldFixBlendIndices( const BW::StringRef & sourceFormatName )
{
	return (sourceFormatName.equals("xyznuvitb") ||
			sourceFormatName.equals("xyznuviiiww") ||
			sourceFormatName.equals("xyznuviiiwwtb"));
}

}	// end anonymous namespace


bool Vertices::fixBlendIndices()
{
	if (isEmpty( *this ))
	{
		ERROR_MSG( "Vertices::fixBlendIndices: vertices aren't loaded.\n" );
		return false;
	}

#if !CONSUMER_CLIENT_BUILD

	UINT bufferSize = vertexStride_ * nVertices_;
	Moo::SimpleVertexLock vertexLock( vertexBuffer_, 0, bufferSize, 0 );
	MF_ASSERT( vertexLock );

	// get both the target and source vertex format
	const VertexFormat& vertexFormat = pDecl()->format();
	const VertexFormat* sourceVertexFormat = 
		VertexFormatCache::get( sourceFormat() );
	MF_ASSERT( sourceVertexFormat );

	typedef VertexElement::SemanticType SemType;
	typedef VertexElement::StorageType StorType;

	SemType::Value semanticType = SemType::BLENDINDICES;
	uint32 semanticIndex = 0;

	// target format query for blend indices (current buffer)
	uint32 streamIndex;
	StorType::Value storageType;
	if (!vertexFormat.findElement( semanticType, semanticIndex, 
		&streamIndex, NULL, &storageType ))
	{
		return false;
	}

	// source format query for blend indices
	StorType::Value sourceStorageType;
	if (!sourceVertexFormat->findElement( semanticType, semanticIndex,  
		NULL, NULL, &sourceStorageType ))
	{
		return false;
	}

	// number of indices to check is based on the source storage type
	uint32 nIndices = VertexElement::componentCount( sourceStorageType );

	// TODO: Remove (see vertex format TODO at top of file)
	if (sourceFormat() == "xyznuviiiww" || sourceFormat() == "xyznuviiiwwtb")
	{
		MF_ASSERT( nIndices == 3 );
	}
	else if (sourceFormat() == "xyznuvitb" || sourceFormat() == "xyznuvi")
	{
		MF_ASSERT( nIndices == 1 );
	}
	else
	{
		ASSET_MSG( "Vertices::fixBlendIndices: unexpected vertex format "
			"'%s', with %d blend indices.\n", 
			sourceFormat().c_str(), nIndices );
	}

	if ( nIndices == 0)
	{
		ERROR_MSG( "Vertices::fixBlendIndices: determined 0 blend indices, "
			"in vertex format '%s'.\n", sourceFormat().c_str() );
		return false;
	}

	// Check/fix up blend indices on copied/converted verts.
	if (!verifyFixupBlendIndices( vertexFormat, vertexLock, nVertices_, 
		nIndices, numNodes_, streamIndex, semanticIndex ))
	{
		ASSET_MSG( "Vertices::fixBlendIndices: Vertices in %s contain invalid "
			"bone indices\n", resourceID_.c_str() );
		return true;
	}

#endif //!CONSUMER_CLIENT_BUILD
	return false;
}

bool Vertices::isBumpedFormat( const VertexFormat & vertexFormat )
{
	// Check format contains all [normal, tangent, binormal] semantic elements
	typedef VertexElement::SemanticType SemType;

	SemType::Value requiredSemantics[] = 
	{ SemType::NORMAL, SemType::TANGENT, SemType::BINORMAL };
	uint32 requiredCount = ARRAYSIZE( requiredSemantics );

	// we test for the semantics with index 0 in first stream only
	return vertexFormat.containsSemantics( requiredSemantics, requiredCount, 0, 0 );
}

bool Vertices::hasBumpedFormat() const
{ 
	MF_ASSERT( pDecl() );

	bool res = isBumpedFormat( pDecl()->format() );
	if (res && !(sourceFormat() == "xyznuvtb" ||
		sourceFormat() == "xyznuviiiwwtb" ||
		sourceFormat() == "xyznuvitb"))
	{
		WARNING_MSG( "Vertices::hasBumpedFormat: "
			"unexpected bumped format: '%s\n'", 
			sourceFormat().c_str() );
	}
	return res;
}

namespace {
	bool determineIsSoftwareSkinnerFormat( 
		const VertexFormat & vertexFormat, const BW::StringRef & formatName )
	{
		if (vertexFormat.containsElement( 
			VertexElement::SemanticType::BLENDINDICES, 0 ))
		{
			if ( !(formatName.equals("xyznuviiiww") ||
				formatName.equals("xyznuviiiwwtb") ||
				formatName.equals("xyznuvitb") ||
				formatName.equals("xyznuvi")) )
			{
				WARNING_MSG( "Vertices::determineSoftwareSkinnerFormat: "
					"unexpected skinned format: '%s\n'", 
					formatName.to_string().c_str() );
			}
			return true;
		}
		return false;
	}
}	// anonymous namespace

#if ENABLE_RELOAD_MODEL
/**
 *	This function check if the load file is valid
 *	We are currently doing a fully load and check,
 *	but we might want to optimize it in the future 
 *	if it's needed.
 *	@param resourceID	the visual file path
 */
bool Vertices::validateFile( const BW::string& resourceID, int numNodes )
{
	BW_GUARD;
	DataSectionPtr pFile = BWResource::openSection( resourceID );
	if (!pFile)
	{
		ERROR_MSG( "Vertices::reload: "
			"%s is not a valid visual file\n",
			resourceID.c_str() );
		return false;
	}

	Vertices tempVertices( resourceID, numNodes );
	tempVertices.isInVerticeManger( false );
	return (tempVertices.load() == D3D_OK);
}


/**
 *	This function reload the current vertices from the disk.
 *	This function isn't thread safe. It will only be called
 *	For reload an externally-modified visual
 *	@param	doValidateCheck	do we check if the file is valid first?
 *	@return if succeed
*/
bool Vertices::reload( bool doValidateCheck )
{
	BW_GUARD;
	if (doValidateCheck && 
		!Vertices::validateFile( resourceID_, numNodes_/3 ))
	{
		return false;
	}

	this->load();

	this->onReloaded();
	return true;
}
#endif //ENABLE_RELOAD_MODEL


bool Vertices::inFormatNameList( const BW::StringRef & formatName, 
	const char ** formatNamesArray, uint32 nameCount, bool emitErrorMsgs )
{
	for (uint formatIndex = 0; formatIndex < nameCount; ++formatIndex)
	{
		if (formatName.equals_lower( formatNamesArray[formatIndex] ))
		{
			return true;
		}
	}

	if (emitErrorMsgs)
	{
		ERROR_MSG( "Vertices::inFormatNameList: "
			"vertex format not in whitelist: %s\n", 
			formatName.to_string().c_str() );
	}

	return false;
}


class Vertices::Detail
{
public:
/**
 * This helper class extracts the relevant available vertex header
 * and data start pointers given the start of a binary section of
 * vertex data.
 */
struct ExtractedVertexHeader 
{
	// processed header (if detected)
	const ProcessedVertexHeader * processedVertexHeader_;	

	// plain header (if detected)
	const VertexHeader * vertexHeader_;	

	// start of vertex data
	const void * data_;					

	// The vertex format to interpret this data (if detected)
	const VertexFormat * vertexFormat_;	

	// stream index of the vertex format stream  that the data 
	// corresponds to (default 0) actual storage of data
	uint32 dataStreamIndex_;

	// actual binary block storage of data
	BinaryPtr binaryPtr_;				

	ExtractedVertexHeader()
		: processedVertexHeader_( NULL )
		, vertexHeader_( NULL )
		, data_( NULL )
		, vertexFormat_( NULL )
		, dataStreamIndex_( 0 )
	{}

	ExtractedVertexHeader( 
		BinaryPtr binaryData, HeaderMagic::Value expectedMagic )
	{
		// default initialise
		*this = ExtractedVertexHeader();

		binaryPtr_ = binaryData;
		const void * data = binaryData->data();

		processedVertexHeader_ = 
			reinterpret_cast< const ProcessedVertexHeader* >( data );
		if (processedVertexHeader_->magic_ == expectedMagic)
		{
			// Processed data set
			vertexHeader_ = &processedVertexHeader_->vertexHeader_;
			data_ = reinterpret_cast<const void*>( processedVertexHeader_ + 1 );
		}
		else
		{
			// Not a processed data set
			processedVertexHeader_ = NULL;
			switch (expectedMagic)
			{
			case HeaderMagic::ProcessedVertices:
				// assume vertex header available
				vertexHeader_ = reinterpret_cast< const VertexHeader* >( data );
				data_ = reinterpret_cast< const void* >( vertexHeader_ + 1 );
				break;
			
			case HeaderMagic::ProcessedVertexStream:
				// assume no headers available
				vertexHeader_ = NULL;
				data_ = data;
				break;

			default:
				// unrecognized header type we dont handle yet
				ERROR_MSG( "Vertices::ExtractedVertexHeader: "
					"unrecognized header magic enum: %d\n", expectedMagic );
				break;
			}
		}

		if (vertexHeader_)
		{
			vertexFormat_ = VertexFormatCache::get( vertexHeader_->vertexFormat_ );
		}
	}

	/// does the extracted header represent readable vert data
	bool isValid() const 
	{ 
		return ( data_ != NULL && vertexFormat_ != NULL); 
	}

};	// ExtractedVertexHeader


/**
 * Helper function to try to read a stream type (supplied via streamDesc)
 * from a primitives data section. Success is if it finds data for the stream type.
 *
 * on success, returns a non-empty ExtractedVertexHeader, and updates streamBinary
 * on failure, the returned header and streamBinary will both be empty.
 */
static ExtractedVertexHeader readStream( 
	const VertexStreamDesc & streamDesc,
	DataSectionPtr pPrimFile, const BW::StringRef & streamRootPath, 
	BinaryPtr& streamBinary )
{
	// try to load the stream name.
	streamBinary = pPrimFile->readBinary( 
		streamRootPath + streamDesc.fileSectionName_ );
	if (!streamBinary)
	{
		return ExtractedVertexHeader();
	}

	// get the vertex format for this stream
	ExtractedVertexHeader header( streamBinary, 
		HeaderMagic::ProcessedVertexStream );
	if (!header.vertexFormat_)
	{
		// use default stream type's vertex format
		header.vertexFormat_ = streamDesc.defaultVertexFormat();
	}
	MF_ASSERT( header.vertexFormat_ );
	
	// set the stream index the data corresponds to
	header.dataStreamIndex_ = streamDesc.streamIndex_;

	return header;
}

// Vertices private methods

// This method adds a stream to its internal list of streams
static bool appendConfigureStream( 
	Vertices & vertices, VertexStream & stream, const VertexFormat& format )
{
	if (!vertices.streams_)
	{
		vertices.streams_ = new StreamContainer();
	}

	vertices.streams_->streamData_.push_back( stream );
	return configureStream( vertices, format );
}


// This method adds a stream
static void addStream(	
	Vertices & vertices, const VertexFormat& format, uint32 streamIndex, 
	const VertexBuffer& vb, uint32 offset, uint32 stride, uint32 size )
{
	VertexStream stream = VertexStream::create( format, streamIndex );
	stream.assign( vb, offset, stride, size );
	appendConfigureStream( vertices, stream, format );
}


// This method loads a stream and adds it.
static bool loadStream(	Vertices & vertices, const void * data, 
						const VertexFormat & format, uint32 streamIndex )
{
	if (!data)
	{
		return false;
	}

	VertexStream stream = VertexStream::create( format, streamIndex );
	stream.load( data, vertices.nVertices_, &format );
	appendConfigureStream( vertices, stream, format );
	return true;
}


// This method sets up and combines stream vertex declaration
static bool configureStream( 
	Vertices & vertices, const VertexFormat & sourceFormat )
{
	const BW::StringRef targetDevice( VertexDeclaration::getTargetDevice() );
	const VertexFormat & targetFormat = 
		*VertexFormatCache::getTarget( sourceFormat, targetDevice, true );

	// get the added stream declaration.
	BW::StringRef streamDeclName = targetFormat.name();
	Moo::VertexDeclaration* pStreamDecl = 
		VertexDeclaration::get( streamDeclName );

	if (pStreamDecl)
	{
		if (vertices.pDecl_)
		{
			// combine the stream declaration with the actual to 
			// create a new one.
			vertices.pDecl_ = VertexDeclaration::combine( 
				vertices.pDecl_, pStreamDecl );
		}

		if (vertices.pInstancedDecl_)
		{
			// combine the stream declaration with the actual to 
			// create a new one.
			vertices.pInstancedDecl_ = VertexDeclaration::combine( 
				vertices.pInstancedDecl_, pStreamDecl );
		}

		// and do the same with the software decls...
		vertices.streams_->updateDeclarations( pStreamDecl );
		return true;
	}
	return false;
}

};	// Vertices::Detail

HRESULT Vertices::load( )
{
	BW_GUARD;
	HRESULT res = E_FAIL;
	release();

	// Is there a valid device pointer?
	if (Moo::rc().device() == (void*)NULL)
	{
		return res;
	}

	// find our data
	BinaryPtr vertices;

	typedef Detail::ExtractedVertexHeader ExtVertHeader;
	BW::vector< ExtVertHeader > pendingStreamLoads;

	BW::string::size_type noff = resourceID_.find( ".primitives/" );
	if (noff < resourceID_.size())
	{
		// everything is normal
		noff += 11;
		BW::string primFilename = resourceID_.substr( 0, noff );

		// Intial attempt, try to get the processed primitives file
		DataSectionPtr pPrimFile = PrimitiveFile::get( primFilename );

		if (pPrimFile)
		{
			vertices = pPrimFile->readBinary(
				resourceID_.substr( noff+1 ) );

			// build the stream name
			BW::string stream = resourceID_.substr( noff+1 );
			BW::string::size_type dot = stream.find_last_of('.');
			if (dot == BW::string::npos)
			{
				stream = "";
			}
			else
			{
				stream = stream.substr(0, dot+1);
			}

			// try reading vertex streams.
			const VertexStreamDesc::StreamDescs & streamDescs = VertexStreamDesc::getKnownStreams();
			for (uint32 sdIndex = 0; sdIndex < streamDescs.size(); ++sdIndex)
			{
				// per possible stream section, try to read the relevant info
				// and queue it up for loading.
				const VertexStreamDesc & streamDesc = streamDescs[sdIndex];
				BinaryPtr streamBinary;
				ExtVertHeader streamInfo = Detail::readStream( 
					streamDesc, pPrimFile, stream, streamBinary );
				if (streamInfo.isValid() && streamBinary)
				{
					pendingStreamLoads.push_back( streamInfo );
				}
			}
		}
	}
	else
	{
		// find out where the data should really be stored
		BW::string fileName, partName;
		splitOldPrimitiveName( resourceID_, fileName, partName );

		// read it in from this file
		vertices = fetchOldPrimitivePart( fileName, partName );
	}

	// Open the binary resource for these vertices
	D3DPOOL pool = D3DPOOL_MANAGED;

	pool = RenderContext::patchD3DPool(pool);

	if (vertices)
	{
		DWORD usageFlag = rc().mixedVertexProcessing() ? D3DUSAGE_SOFTWAREPROCESSING : 0;

		// Get header information if there is any
		ExtVertHeader vertsInfo( vertices, HeaderMagic::ProcessedVertices );
		const void* pSrcVertices = vertsInfo.data_;

		// Get the vertex format name and vertex count from the vertex header
		nVertices_ = vertsInfo.vertexHeader_->nVertices_;
		const char* diskFormat = vertsInfo.vertexHeader_->vertexFormat_;

		// Save the disk format string to sourceFormat_, because 
		// there exist remaining format-string based switch sections in the code
		// and they still operate on what is now the "source format".
		sourceFormat_ = diskFormat;
		if (vertsInfo.processedVertexHeader_)
		{
			sourceFormat_ = vertsInfo.processedVertexHeader_->originalVertexFormat_;
		}
		
		std::transform( std::begin(sourceFormat_), std::end(sourceFormat_),
			std::begin(sourceFormat_), tolower );

		// TODO: Remove (see vertex format TODO at top of file)
		{
			const char * acceptedFormatNames[] = {"xyznuv", "xyznduv", 
				"xyznuvtb", "xyznuv2tb", "xyznuviiiww", "xyznuviiiwwtb", 
				"xyznuvitb", "xyznuvi"};
			uint nameCount = ARRAYSIZE( acceptedFormatNames );
			MF_ASSERT( inFormatNameList( sourceFormat_, 
				acceptedFormatNames, nameCount, true ) ); 
		}

		BW::StringRef targetDevice( VertexDeclaration::getTargetDevice() );

		// Retrieve Vertex formats
		const VertexFormat * pSourceVertexFormat = VertexFormatCache::get( diskFormat );
		MF_ASSERT( pSourceVertexFormat );
		const VertexFormat * pTargetVertexFormat = VertexFormatCache::getTarget( 
			*pSourceVertexFormat, targetDevice, true );

		// Retrieve vertex declaration
		const BW::StringRef targetFormatName = pTargetVertexFormat->name();
		pDecl_ = VertexDeclaration::get( targetFormatName );
		if (!pDecl_)
		{
			ERROR_MSG( "Vertices::load: Failed to load Vertex Declaration "
				"for format name '%s'\n", targetFormatName.to_string().c_str() );
			return E_FAIL;
		}

		// Setup Vertex conversion
		VertexFormat::ConversionContext vfConversion( pTargetVertexFormat, pSourceVertexFormat );
		MF_ASSERT( vfConversion.isValid() );

		// Vertex and buffer sizes
		const uint32 primaryStreamIndex = 0;
		vertexStride_ = vfConversion.dstVertexSize( primaryStreamIndex );
		vertexBufferSize_ = vertexStride_ * nVertices_;

		// initialise the vertex buffer for copying
		Moo::VertexBuffer pTargetVertexBuffer;

		// Create the vertex buffer
		if (SUCCEEDED( res = pTargetVertexBuffer.create( vertexBufferSize_, usageFlag, 0, pool ) ))
		{
			// Set up the smart pointer to the vertex buffer
			vertexBuffer_ = pTargetVertexBuffer;

			Moo::SimpleVertexLock vertexLock( vertexBuffer_, vertexBufferOffset_, vertexBufferSize_, 0 );
			if (vertexLock)
			{
				// Perform conversion
				bool converted = vfConversion.convertSingleStream( 
					vertexLock, primaryStreamIndex, pSrcVertices, primaryStreamIndex, nVertices_ );
				MF_ASSERT( converted );
			}
			else
			{
				// vertexLock invalid
				res = E_FAIL;
			}

		}
		else
		{
			// target buffer creation failed
			ERROR_MSG( "Vertices::load: Could not create target vertex buffer" );
		}
		
		// fix up any bad blend indices
		if (shouldFixBlendIndices( sourceFormat() ))
		{
			fixBlendIndices();
		}

		formatSupportsSoftwareSkinner_ = 
			determineIsSoftwareSkinnerFormat( *pTargetVertexFormat, sourceFormat_ );
		
		//-- combine instancing declaration with vertices format to create instanced declaration.
		BW::StringRef instancingDeclName = 
			VertexFormatCache::getTargetName<InstancingStream>( targetDevice, true );
		Moo::VertexDeclaration * instancing = VertexDeclaration::get( instancingDeclName );
		pInstancedDecl_ = VertexDeclaration::combine( pDecl_, instancing );
		
		// It's worth considering the usage of the declarations here.
		// Note that if the vertices are software skinned, they will use a 
		// different declaration when drawn. Is it the case that for each 
		// Vertices object? they will only use one declaration ever? If so, then
		// it could be detected that the software skinning will be used so that
		// declaration can be places in pDecl_ .... 

		// for now there are two extra declarations... pSoftwareDecl_ and pSoftwareDeclTB_
		// we would want to get rid of these ultimately.
		// perhaps we can just detect the change of materials and then re-create the
		// required declarations??? The only thing that will cause issues is if the 
		// same Vertices object is used multiple times with different declarations.

		// Loading each of the found secondary vertex streams.
		for (uint32 psIndex = 0; psIndex < pendingStreamLoads.size(); ++psIndex)
		{
			ExtVertHeader & streamInfo = pendingStreamLoads[psIndex];
			bool loadedStream = Detail::loadStream( 
				*this, streamInfo.data_, *streamInfo.vertexFormat_, 
				streamInfo.dataStreamIndex_ );
		}
	}
	else
	{
		ASSET_MSG( "Failed to read binary resource: %s\n", resourceID_.c_str() );
	}

	// Add the buffer to the preload list so that it can get uploaded
	// to video memory
	vertexBuffer_.addToPreloadList();

	if (streams_)
	{
		streams_->preload();
	}
	return res;
}


HRESULT Vertices::release( )
{
	BW_GUARD;
	vertexBuffer_.release();
	bw_safe_delete( streams_ );

	nVertices_ = 0;
	return S_OK;
}

BaseSoftwareSkinner* Vertices::softwareSkinner()
{
	BW_GUARD;
	if (pSoftwareSkinner_)
	{
		return pSoftwareSkinner_.getObject();
	}

	if (!formatSupportsSoftwareSkinner_ || nVertices_ == 0 || !vertexBuffer_.valid())
	{
		return NULL;
	}

	if (this->sourceFormat() == "xyznuviiiww")
	{
		VertexLock<Moo::VertexXYZNUVIIIWWPC> vl( vertexBuffer_, 
			vertexBufferOffset_, vertexBufferSize_, D3DLOCK_READONLY );
		if (vl.valid())
		{
			SoftwareSkinner< SoftSkinVertex >* pSkinner = new SoftwareSkinner< SoftSkinVertex >;
			pSkinner->init( vl.get(), nVertices_ );
			pSoftwareSkinner_ = pSkinner;
		}
	}
	else if (this->sourceFormat() == "xyznuviiiwwtb")
	{
		VertexLock<Moo::VertexXYZNUVIIIWWTBPC> vl( vertexBuffer_, 
			vertexBufferOffset_, vertexBufferSize_, D3DLOCK_READONLY );
		if (vl.valid())
		{
			SoftwareSkinner< SoftSkinBumpVertex >* pSkinner = new SoftwareSkinner< SoftSkinBumpVertex >;
			pSkinner->init( vl.get(), nVertices_ );
			pSoftwareSkinner_ = pSkinner;
		}
	}
	else if (this->sourceFormat() == "xyznuvitb")
	{
		VertexLock<Moo::VertexXYZNUVITBPC> vl( vertexBuffer_, 
			vertexBufferOffset_, vertexBufferSize_, D3DLOCK_READONLY );
		if (vl.valid())
		{
			SoftwareSkinner< RigidSkinBumpVertex >* pSkinner = new SoftwareSkinner< RigidSkinBumpVertex >;
			pSkinner->init( vl.get(), nVertices_ );
			pSoftwareSkinner_ = pSkinner;
		}
	}
	else if (this->sourceFormat() == "xyznuvi")
	{
		VertexLock<Moo::VertexXYZNUVIPC> vl( vertexBuffer_, 
			vertexBufferOffset_, vertexBufferSize_, D3DLOCK_READONLY );
		if (vl.valid())
		{
			SoftwareSkinner< RigidSkinVertex >* pSkinner = new SoftwareSkinner< RigidSkinVertex >;
			pSkinner->init( vl.get(), nVertices_ );
			pSoftwareSkinner_ = pSkinner;
		}
	}

	return pSoftwareSkinner_.getObject();
}

/**
  * Extract the vertex positions directly from the vertex buffer.
  */
void Vertices::vertexPositionsVB( VertexPositions& vPositions )
{
	vPositions.resize( nVertices_ );
	void *pData = NULL;
	if (vertexBuffer_.valid())
	{
		if (SUCCEEDED( vertexBuffer_.lock( vertexBufferOffset_, vertexBufferSize_, &pData, D3DLOCK_READONLY ) ))
		{
			for( size_t i = 0; i < nVertices_; i++ )
			{
				vPositions[i] = *(Vector3*)pData;
				pData = (char*)pData + vertexStride_;
			}

			vertexBuffer_.unlock();
		}
		else
		{
			ERROR_MSG( "Vertices::vertexPositionsVB: vertex buffer failed to lock\n" );
		}
	}
	else
	{
		ERROR_MSG( "Vertices::vertexPositionsVB: vertex buffer not valid\n" );
	}
}

/**
  * Get vertex positions, cached in system memory.
  */
const Vertices::VertexPositions & Vertices::vertexPositions()
{
	if (vertexPositions_.empty() && nVertices_ > 0)
	{
		const_cast< Vertices* >( this )->vertexPositionsVB( vertexPositions_ );
	}
	return vertexPositions_;
}

/**
  * Bind the vertex data and choose a vertex declaration to use.
  */
HRESULT Vertices::bindStreams(
	bool instanced,  bool softwareSkinned /*= false*/, bool bumpMapped /*= false*/ )
{
	if (streams_)
	{
		streams_->set();
	}

	HRESULT hr = E_FAIL;
	if (softwareSkinned)
	{
		if (bumpMapped)
		{
			if (streams_)
			{
				hr = Moo::rc().setVertexDeclaration( streams_->pSoftwareDeclTB_->declaration() );
			}
			else
			{
				static VertexDeclaration* pDecl = VertexDeclaration::get( "xyznuvtb" );
				hr = Moo::rc().setVertexDeclaration( pDecl->declaration() );
			}
		}
		else
		{
			if (streams_)
			{
				hr = Moo::rc().setVertexDeclaration( streams_->pSoftwareDecl_->declaration() );
			}
			else
			{
				static VertexDeclaration* pDecl = VertexDeclaration::get( "xyznuv" );
				hr = Moo::rc().setVertexDeclaration( pDecl->declaration() );
			}
		}
	}
	else
	{
		DX::VertexDeclaration * vd = pDecl_->declaration();

		if (instanced && pInstancedDecl_)
		{
			vd = pInstancedDecl_->declaration();
		}
		else
		{
			vd = pDecl_->declaration();
		}
		hr = Moo::rc().setVertexDeclaration( vd );
	}
	return hr;
}

/**
 * This method transforms vertices and prepares them for drawing.
 *
 */
HRESULT Vertices::setTransformedVertices( bool tb, const NodePtrVector& nodes )
{
	BW_GUARD;
	BaseSoftwareSkinner* pSoftwareSkinner = this->softwareSkinner();
	if (pSoftwareSkinner)
	{
		bindStreams( false, true, tb );

		if (tb)
		{
			typedef Moo::VertexXYZNUVTBPC VertexType;
			int vertexSize = sizeof(VertexType);

			if (!pSkinnerVertexBuffer_.valid() || vbBumped_ == false)
			{
				//DynamicVertexBuffer
				Moo::DynamicVertexBufferBase2 & vb = 
					Moo::DynamicVertexBufferBase2::instance( vertexSize );
				VertexType * pVerts = vb.lock<VertexType>( nVertices_ );
				if (pVerts == NULL)
				{
					ERROR_MSG( "Vertices::setTransformedVertices - lock failed\n" );
					return E_FAIL;
				}
				pSoftwareSkinner->transformVertices( pVerts, 0, nVertices_, nodes );
				vb.unlock();				
				pSkinnerVertexBuffer_ = vb.vertexBuffer();
				vbBumped_ = true;
			}
			return pSkinnerVertexBuffer_.set( 0, 0, vertexSize );
		}
		else
		{
			typedef Moo::VertexXYZNUV VertexType;
			int vertexSize = sizeof(VertexType);

			if (!pSkinnerVertexBuffer_.valid() || vbBumped_ == true)
			{
				//DynamicVertexBuffer
				Moo::DynamicVertexBufferBase2 & vb = 
					Moo::DynamicVertexBufferBase2::instance( vertexSize );
				VertexType * pVerts = vb.lock<VertexType>( nVertices_ );
				if (pVerts == NULL)
				{
					ERROR_MSG( "Vertices::setTransformedVertices - lock failed\n" );
					return E_FAIL;
				}
				pSoftwareSkinner->transformVertices( pVerts, 0, nVertices_, nodes );
				vb.unlock();				
				pSkinnerVertexBuffer_ = vb.vertexBuffer();
				vbBumped_ = false;
			}
			return pSkinnerVertexBuffer_.set( 0, 0, vertexSize );
		}
	}
	else
	{
		return setVertices( false, false );
	}
	return S_OK;
}

/**
 * This method prepares vertices for drawing.
 */
HRESULT Vertices::setVertices( bool software, bool instanced )
{
	BW_GUARD_PROFILER( Vertices_setVertices );

	// Does our vertexbuffer exist.
	HRESULT hr = E_FAIL;

	// Does our vertexbuffer exist?
	if (!vertexBuffer_.valid())
	{
		// If not load it
		hr = load();

		if (FAILED( hr ))
			return hr;
	}

	hr = bindStreams( instanced );

	// Set up the stream source(s).
	if ( SUCCEEDED( hr ) )
	{	
		hr = vertexBuffer_.set( 0, vertexBufferOffset_, vertexStride_ );
	}

	return hr;
}

void	Vertices::pullInternals( const bool instanced,
								  const VertexDeclaration*& outputVertexDecl,
								  const VertexBuffer*& outputDefaultVertexBuffer,
								  uint32&	outputDefaultVertexBufferOffset,
								  uint32& outputDefaultVertexStride,
								  const StreamContainer*& outputStreamContainer)
{
	// Does our vertexbuffer exist?
	if (!vertexBuffer_.valid())
	{
		// If not load it
		this->load();
	}

	MF_ASSERT( vertexBuffer_.valid() );

	outputVertexDecl = instanced ? pInstancedDecl_ : pDecl_;
	outputDefaultVertexBuffer = &vertexBuffer_;
	outputDefaultVertexBufferOffset = vertexBufferOffset_;
	outputDefaultVertexStride = vertexStride_;
	outputStreamContainer = streams_;
}


/**
 *	This method opens up the primitives file and vertices sub-file and
 *	returns them in the output parameters.
 *	@param	pPrimFile	[out] returned smartpointer to primitives DataSection.
 *	@param	vertices	[out] returned smartpointer to vertices BinSection.
 *	@param	partName	[out] name of vertices section in primitives file.
 *	@return	bool		if the primitives file and vertices file were opened.
 */
bool Vertices::openSourceFiles(
	DataSectionPtr& pPrimFile,
	BinaryPtr& vertices,
	BW::string& partName )
{
	BW_GUARD;
	// find our data		
	BW::string::size_type noff = resourceID_.find( ".primitives/" );
	if (noff < resourceID_.size())
	{		
		noff += 11;
		pPrimFile =	PrimitiveFile::get( resourceID_.substr( 0, noff ) );		
		partName = resourceID_.substr( noff+1 );
	}
	else
	{
		// find out where the data should really be stored
		BW::string fileName;		
		splitOldPrimitiveName( resourceID_, fileName, partName );		
		BW::string id = fileName + ".primitives";
		pPrimFile = PrimitiveFile::get( id );		
	}

	if (pPrimFile)
	{
		vertices = pPrimFile->readBinary( partName );
	}
	else
	{
		ASSET_MSG( "Could not open primitive file to find vertices: %s\n", resourceID_.c_str());
		return false;
	}

	if (!vertices)
	{
		ASSET_MSG( "Could not open vertices in file: %s\n", resourceID_.c_str());
		return false;
	}

	return true;
}


/**
 *	This method exists because hardskinned vertices have been deprecated in
 *	1.8, and removed in 1.9.  This method resaves hard-skinned vertex information
 *	in skinned format.
 *	@return	bool	Success or Failure.
 */
bool Vertices::resaveHardskinnedVertices()
{
	BW_GUARD;
	DataSectionPtr pPrimFile;
	BinaryPtr vertices;
	BW::string partName;

	if (this->openSourceFiles(pPrimFile,vertices,partName))
	{
		// Get the vertex header
		const VertexHeader* vh = reinterpret_cast< const VertexHeader* >( vertices->data() );
		nVertices_ = vh->nVertices_;
		bw_strlwr( const_cast< char* >( vh->vertexFormat_ ) );
		sourceFormat_ = vh->vertexFormat_;

		// create the right type of vertex
		int nVerts = vh->nVertices_;
		if (BW::string( vh->vertexFormat_ ) == "xyznuvitb")
		{				
			//Go from XYZNUVITB to XYZNUVIIIWWTBPC			
			const VertexXYZNUVITB * pSrcVerts = reinterpret_cast< const VertexXYZNUVITB* >( vh + 1 );
			//int dstSize = sizeof(VertexHeader) + sizeof(VertexXYZNUVIIIWWTBPC) * nVerts;
			int dstSize = sizeof(VertexHeader) + sizeof(VertexXYZNUVIIIWWTB) * nVerts;
			int srcSize = sizeof(VertexHeader) + sizeof(VertexXYZNUVITB) * nVerts;
			//extraSize accounts for any information that is in our data section that
			//we don't know about (for instance morph targets)
			int extraSize = vertices->len() - srcSize;
			char * output = new char[dstSize + extraSize];

			VertexHeader* vh2 = (VertexHeader*)output;
			ZeroMemory( vh2, sizeof( VertexHeader ) );
			vh2->nVertices_ = nVerts;
			//bw_snprintf( vh2->vertexFormat, sizeof( vh2->vertexFormat ), "xyznuviiiwwtbpc" );
			bw_snprintf( vh2->vertexFormat_, sizeof( vh2->vertexFormat_ ), "xyznuviiiwwtb" );

			//VertexXYZNUVIIIWWTBPC * outVerts = reinterpret_cast< VertexXYZNUVIIIWWTBPC * >(vh2 + 1);
			VertexXYZNUVIIIWWTB * outVerts = reinterpret_cast< VertexXYZNUVIIIWWTB * >(vh2 + 1);
			for (int i=0; i<nVerts; i++)
			{
				outVerts[i] = pSrcVerts[i];				
			}

			//copy trailing information from source data file
			memcpy( output + dstSize, vertices->cdata() + srcSize, extraSize );

			INFO_MSG( "Converted file %s\n", resourceID_.c_str() );			
			BinaryPtr pVerts = 
				new BinaryBlock(output, dstSize + extraSize, "BinaryBlock/vertices");
			pPrimFile->writeBinary( partName, pVerts );
			pPrimFile->save();
			bw_safe_delete_array(output);
			return true;
		}
		else if (BW::string( vh->vertexFormat_ ) == "xyznuvi")
		{
			//Go from XYZNUVI to XYZNUVIIIWWPC			
			const VertexXYZNUVI * pSrcVerts = reinterpret_cast< const VertexXYZNUVI* >( vh + 1 );
			//int dstSize = sizeof(VertexHeader) + sizeof(VertexXYZNUVIIIWWPC) * nVerts;
			int dstSize = sizeof(VertexHeader) + sizeof(VertexXYZNUVIIIWW) * nVerts;
			int srcSize = sizeof(VertexHeader) + sizeof(VertexXYZNUVI) * nVerts;
			int extraSize = vertices->len() - srcSize;
			char * output = new char[dstSize + extraSize];

			VertexHeader* vh2 = (VertexHeader*)output;
			ZeroMemory( vh2, sizeof( VertexHeader ) );
			vh2->nVertices_ = nVerts;
			//bw_snprintf( vh2->vertexFormat, sizeof( vh2->vertexFormat ), "xyznuviiiwwpc" );
			bw_snprintf( vh2->vertexFormat_, sizeof( vh2->vertexFormat_ ), "xyznuviiiww" );

			//VertexXYZNUVIIIWWPC * outVerts = reinterpret_cast< VertexXYZNUVIIIWWPC * >(vh2 + 1);
			VertexXYZNUVIIIWW * outVerts = reinterpret_cast< VertexXYZNUVIIIWW * >(vh2 + 1);
			for (int i=0; i<nVerts; i++)
			{
				outVerts[i] = pSrcVerts[i];				
			}

			//copy trailing information from source data file
			memcpy( output + dstSize, vertices->cdata() + srcSize, extraSize );

			INFO_MSG( "Converted file %s\n", resourceID_.c_str() );
			BinaryPtr pVerts = 
				new BinaryBlock(output, dstSize + extraSize, "BinaryBlock/vertices");
			pPrimFile->writeBinary( partName, pVerts );
			pPrimFile->save();
			bw_safe_delete_array(output);
			return true;
		}
		else
		{
			ERROR_MSG( "Cannot change from vertex format: %s (not yet implemented)\n", vh->vertexFormat_ );
			return false;
		}
	}
	else
	{		
		//openSourceFiles outputs an errormsg, so no need to do it here.
		return false;
	}

	return true;
}


std::ostream& operator<<(std::ostream& o, const Vertices& t)
{
	o << "Vertices\n";
	return o;
}

Moo::VertexBuffer Vertices::vertexBuffer( uint32 streamIndex ) const
{
	if (vertexBuffer_.valid())
	{
		if (streamIndex == 0)
		{
			return vertexBuffer_;
		}

		// This wasn't the stream we were interested in, fall through to the
		// stream container
		streamIndex--;
	}
	
	if (streams_ && streamIndex < streams_->streamData_.size())
	{
		return streams_->streamData_[streamIndex].vertexBuffer();
	}
	else
	{
		return Moo::VertexBuffer();
	}
}

uint32 Vertices::vertexBufferOffset( uint32 streamIndex ) const
{
	if (vertexBuffer_.valid())
	{
		if (streamIndex == 0)
		{
			return vertexBufferOffset_;
		}

		// This wasn't the stream we were interested in, fall through to the
		// stream container
		streamIndex--;
	}

	if (streams_ && streamIndex < streams_->streamData_.size())
	{
		return streams_->streamData_[streamIndex].offset();
	}
	else
	{
		return 0;
	}
}

uint32 Vertices::vertexStride( uint32 streamIndex ) const
{
	if (vertexBuffer_.valid())
	{
		if (streamIndex == 0)
		{
			return vertexStride_;
		}

		// This wasn't the stream we were interested in, fall through to the
		// stream container
		streamIndex--;
	}

	if (streams_ && streamIndex < streams_->streamData_.size())
	{
		return streams_->streamData_[streamIndex].stride();
	}
	else
	{
		return 0;
	}
}

uint32 Vertices::vertexStreamIndex( uint32 streamIndex ) const
{
	if (vertexBuffer_.valid())
	{
		if (streamIndex == 0)
		{
			return 0;
		}

		// This wasn't the stream we were interested in, fall through to the
		// stream container
		streamIndex--;
	}

	if (streams_ && streamIndex < streams_->streamData_.size())
	{
		return streams_->streamData_[streamIndex].stream();
	}
	else
	{
		return 0;
	}
}

uint32 Vertices::numVertexBuffers() const
{
	uint32 size = 0;
	if (vertexBuffer_.valid())
	{
		size++;
	}
	if (streams_)
	{
		size += (uint32)streams_->streamData_.size();
	}
	return size;
}

void Vertices::initFromValues( uint32 numVerts, const char* sourceFormat )
{
	nVertices_ = numVerts;
	sourceFormat_ = sourceFormat;

	// Load up formats according to the source format
	BW::StringRef targetDevice( VertexDeclaration::getTargetDevice() );

	const VertexFormat * pSourceFormat = VertexFormatCache::get( sourceFormat_ );
	MF_ASSERT( pSourceFormat );
	const BW::StringRef targetFormatName = 
		VertexFormatCache::getTarget( *pSourceFormat, targetDevice, true )->name();

	pDecl_ = VertexDeclaration::get( targetFormatName );
	MF_ASSERT( pDecl_ );
	const VertexFormat & targetFormat = pDecl_->format();

	formatSupportsSoftwareSkinner_ = 
		determineIsSoftwareSkinnerFormat( targetFormat, sourceFormat_ );

	const uint32 streamIndex = 0;
	MF_ASSERT( targetFormat.streamCount() > streamIndex );
	vertexStride_ = targetFormat.streamStride( streamIndex );

	//-- combine instancing declaration with vertices format to create instanced declaration.
	BW::StringRef instancingDeclName = 
		VertexFormatCache::getTargetName<InstancingStream>( targetDevice, true );
	Moo::VertexDeclaration* instancing = VertexDeclaration::get( instancingDeclName );
	pInstancedDecl_ = VertexDeclaration::combine( pDecl_, instancing );
}

void Vertices::addStream( uint32 streamIndex, const VertexBuffer& vb, 
	uint32 offset, uint32 stride, uint32 size )
{
	if (streamIndex == 0)
	{
		// Triggering this assert indicates a mismatch between the data
		// being loaded, and the current source format definition
		MF_ASSERT( vertexStride_ == stride );

		vertexBuffer_ = vb;
		vertexBufferOffset_ = offset;
		vertexStride_ = stride;
		vertexBufferSize_ = size;
	}
	else
	{
		bool added = false;
		const VertexStreamDesc::StreamDescs & streamDescs = VertexStreamDesc::getKnownStreams();
		for (uint32 sdIndex = 0; sdIndex < streamDescs.size(); ++sdIndex)
		{
			// per possible stream section, try to match the stream index
			// note that this relies on each stream type having a unique stream index
			const VertexStreamDesc & streamDesc = streamDescs[sdIndex];
			if (streamIndex == streamDesc.streamIndex_)
			{
				// add using the corresponding stream info.
				const VertexFormat * streamFormat = streamDesc.defaultVertexFormat();
				MF_ASSERT( streamFormat );
				Detail::addStream( *this, *streamFormat, 
					streamIndex, vb, offset, stride, size );
				added = true;
				break;
			}
		}
		if (!added)
		{
			ERROR_MSG( "Invalid stream type received. %s\n",
				resourceID_.c_str() );
		}
	}
}


} // namespace Moo

BW_END_NAMESPACE

// vertices.cpp

