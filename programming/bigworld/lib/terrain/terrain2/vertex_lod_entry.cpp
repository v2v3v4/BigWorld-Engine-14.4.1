#include "pch.hpp"
#include "vertex_lod_entry.hpp"

#include "../terrain_data.hpp"
#include "terrain_index_buffer.hpp"
#include "terrain_vertex_buffer.hpp"

#include "resmgr/bin_section.hpp"

DECLARE_DEBUG_COMPONENT2( "Moo", 0 )


BW_BEGIN_NAMESPACE

using namespace Terrain;

#ifdef WATCH_TERRAIN_VERTEX_LODS
float VertexLodEntry::s_totalMB_ = 0;
#endif


VertexLodEntry::VertexLodEntry()
{

}


VertexLodEntry::~VertexLodEntry()
{
	PROFILER_SCOPED( VertexLodEntry_destruct );

#ifdef WATCH_TERRAIN_VERTEX_LODS
	if (pVertices_ && pVertices_->getBuffer().valid())
	{
		s_totalMB_ -= float( sizeInBytes_ ) / ( 1024.0f * 1024.0f );
	}
#endif

	if (pIndexes_)
	{
		TerrainIndexBuffer::release( pIndexes_ );
	}
	pVertices_= NULL;
}

bool VertexLodEntry::init(	const AliasedHeightMap* baseMap, 
							const AliasedHeightMap* previousMap,
							uint32					gridSize )
{
	BW_GUARD;
	uint32 vGridSize = gridSize + 1;

	// Create index buffer
	pIndexes_ = TerrainIndexBuffer::get(gridSize, gridSize );

	// Create this lod level data in vertexHolder
	BW::vector< Vector2 > vertexHolder;
	TerrainVertexBuffer::generate( baseMap, previousMap,  vGridSize, vGridSize, 
		vertexHolder);

	// Create vertex buffer
	uint32 usage = 
#ifdef EDITOR_ENABLED
		0;
#else
		D3DUSAGE_WRITEONLY;
#endif //EDITOR_ENABLED 

	pVertices_= new TerrainVertexBuffer;
	if ( pVertices_->init( (Vector2*)(&vertexHolder.front()), 
			vGridSize, vGridSize, usage ) )
	{
#ifdef WATCH_TERRAIN_VERTEX_LODS
		sizeInBytes_=	sizeof( VertexLodEntry ) +
						sizeof( TerrainVertexBuffer ) + 
						sizeof( Vector2 ) * vGridSize * vGridSize;
		s_totalMB_	+=	float( sizeInBytes_ ) / ( 1024.0f * 1024.0f );
#endif
		return true;
	}

	return false;
}

bool VertexLodEntry::load( DataSectionPtr pTerrain, uint32 level, BinaryPtr& pVertices, uint32& gridSize )
{
	BW_GUARD;
	// Open numbered section for lod
	BW::string sectionName; getSectionName( level, sectionName );
	DataSectionPtr pVertexLOD = pTerrain->openSection( sectionName );
	if ( !pVertexLOD ) 
	{
		WARNING_MSG( "Missing VertexLOD section (%d) in terrain file - skipping.\n", level );
		return false;
	}

	BinaryPtr pBin = pVertexLOD->asBinary();
	if (!pBin)
	{
		WARNING_MSG( "VertexLOD section in terrain file is not binary - skipping.\n" );
		return false;
	}

	// check header
	VertexLODHeader * header = (VertexLODHeader*)pBin->data();

	if ( header->magic_ != VertexLODHeader::MAGIC )
	{
		WARNING_MSG( "VertexLOD header has wrong magic number - skipping.\n" );
		return false;
	}

	// check version
	if ( header->version_ != VertexLODHeader::VERSION_RAW_VERTICES && 
		header->version_ != VertexLODHeader::VERSION_ZIP_VERTICES )
	{
		WARNING_MSG( "VertexLOD header has wrong version - skipping.\n" );
		return false;
	}

	// return grid size
	gridSize = header->gridSize_;

	// Make a copy of the stream after the header
	pVertices = new BinaryBlock( header + 1, 
								pBin->len()  - sizeof(VertexLODHeader),
								"BinaryBlock/TerrainLodEntry" );

	// decompress vertices if necessary
	if ( header->version_ == VertexLODHeader::VERSION_ZIP_VERTICES )
	{
		// decompress vertices
		if ( !pVertices->isCompressed() )
		{
			WARNING_MSG( "VertexLOD claimed to be compressed, but wasn't.\n" );
			return false;
		}

		pVertices = pVertices->decompress();
	}

	// check vertex length - vertex grid has an extra row and column
	if ( pVertices->len() != (gridSize + 1) * (gridSize + 1) * sizeof(Vector2) )
	{
		WARNING_MSG( "VertexLOD data has an incorrect length.\n" );
		return false;
	}

	return true;
}

#ifdef EDITOR_ENABLED
/**
* Save vertices to a data section.
*/
bool VertexLodEntry::save(	DataSectionPtr pSection, uint32 level, uint32 gridSize )
{
	BW_GUARD;
	// Make a new section in terrain section
	BW::string sectionName; getSectionName( level, sectionName );
	DataSectionPtr pLODSection = pSection->openSection( sectionName, true,
												 BinSection::creator() );

	Moo::VertexBuffer vb = pVertices_->getBuffer();
	// Get pointer to vertices
	D3DVERTEXBUFFER_DESC			desc;
	Moo::SimpleVertexLock vl( vb, 0, 0, D3DLOCK_READONLY );

	if ( !vl ||
		FAILED( vb.getDesc(&desc)) )
	{
		ERROR_MSG( "TerrainVertexBuffer::save() - Unable to lock vertex buffer." );
		return false;
	}

	// Copy to a binary block
	BinaryPtr pBinary = 
		new BinaryBlock( vl, desc.Size, "BinaryBlock/TerrainLodEntry" );

	// Compress
	BinaryPtr pCompressed = pBinary->compress();

	// Use only if compressed size was smaller.
	uint32 version = VertexLODHeader::VERSION_RAW_VERTICES;
	if ( pCompressed->len() < pBinary->len() )
	{
		version = VertexLODHeader::VERSION_ZIP_VERTICES;
		pBinary = pCompressed;
	}

	// Allocate memory for header + vertices
	BW::vector<uint8> data( sizeof( VertexLODHeader ) + pBinary->len() );
	VertexLODHeader * header = (VertexLODHeader*)(&data.front());

	// Write header
	header->magic_		= VertexLODHeader::MAGIC;
	header->version_	= version;
	header->gridSize_	= gridSize;

	// Copy vertices after header
	memcpy( header+1, pBinary->data(), pBinary->len() );

	// Make binary section
	BinaryPtr binaryBlock = 
		new BinaryBlock(&data.front(), data.size(), "BinaryBlock/TerrainLodEntry" );
	pLODSection->setBinary( binaryBlock );

	return true;
}

#endif // EDITOR_ENABLED

#ifndef MF_SERVER

// Draw a single lod entry.
bool VertexLodEntry::draw(	const Moo::EffectMaterialPtr& pMaterial,	
							const Vector2&			morphRanges,
							const NeighbourMasks&	neighbourMasks,
							uint8					subBlockMask )
{
	BW_GUARD;
	if ( pIndexes_->setIndices() && pVertices_->set() )
	{
		// do draw
		pIndexes_->draw(pMaterial, morphRanges, neighbourMasks, subBlockMask );
		return true;
	}

	return false;
}

void VertexLodEntry::getVerticesMemoryMap( BW::map<void*, int>& verts )
{
	if(pVertices_.getObject())
		verts.insert(std::make_pair(pVertices_.get(), sizeInBytes_));
}

#endif // MF_SERVER

void VertexLodEntry::getSectionName( uint32 level, BW::string & sectionName )
{
	char buffer[32];
	bw_snprintf( buffer, ARRAY_SIZE(buffer), "VertexLod%d", level );
	sectionName = buffer;
}
BW_END_NAMESPACE

// vertex_lod_entry.cpp

