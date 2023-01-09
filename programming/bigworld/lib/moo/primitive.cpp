#include "pch.hpp"
#include "primitive.hpp"

#include "resmgr/bwresource.hpp"
#include "cstdmf/debug.hpp"
#include "render_context.hpp"
#include "resmgr/primitive_file.hpp"
#include "primitive_manager.hpp"
#include "primitive_file_structs.hpp"

#include "math/boundbox.hpp"

#include "vertices.hpp"

DECLARE_DEBUG_COMPONENT2( "Moo", 0 )


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "primitive.ipp"
#endif

namespace Moo
{

static bool s_primitiveEnableDrawPrim = true;
static bool s_primitiveFirstTime = true;

/**
 * Construct an empty object.
 */
Primitive::Primitive( const BW::string& resourceID )
: resourceID_( resourceID ),
  maxVertices_( 0 ),
  primType_( D3DPT_TRIANGLELIST ),
#if ENABLE_RELOAD_MODEL
  isInPrimitiveManger_( true ),
#endif //ENABLE_RELOAD_MODEL
  nIndices_( 0 )
{
	BW_GUARD;
	if (s_primitiveFirstTime)
	{
		MF_WATCH( "Render/Performance/DrawPrim Primitive", s_primitiveEnableDrawPrim,
			Watcher::WT_READ_WRITE,
			"Allow Primitive to call drawIndexedPrimitive()." );

		s_primitiveFirstTime = false;
	}
}

/**
 * Destruct this object, and tell manager.
 */
Primitive::~Primitive()
{
	BW_GUARD;
}


void Primitive::destroy() const
{
	BW_GUARD;

#if ENABLE_RELOAD_MODEL
	bool isManaged = isInPrimitiveManger_;
#else
	bool isManaged = true;
#endif //ENABLE_RELOAD_MODEL

	// try to safety destroy this primitive
	PrimitiveManager::tryDestroy( this, isManaged );
}

#if ENABLE_RELOAD_MODEL
/**
 *	This function check if the load file is valid
 *	We are currently doing a fully load and check,
 *	but we might want to optimize it in the future 
 *	if it's needed.
 *	@param resourceID	the visual file path
 */
bool Primitive::validateFile( const BW::string& resourceID )
{
	BW_GUARD;
	DataSectionPtr pFile = BWResource::openSection( resourceID );
	if (!pFile)
	{
		ERROR_MSG( "Primitive::reload: "
			"%s is not a valid visual file\n",
			resourceID.c_str() );
		return false;
	}

	Primitive tempPrimitive( resourceID );
	tempPrimitive.isInPrimitiveManger( false );
	return (tempPrimitive.load() == D3D_OK);
}



/**
 *	This function reload the current primitive from the disk.
 *	This function isn't thread safe. It will only be called
 *	For reload an externally-modified visual
 *	@param	doValidateCheck	do we check if the file is valid first?
 *	@return if succeed
*/
bool Primitive::reload( bool doValidateCheck )
{
	BW_GUARD;
	if (doValidateCheck && 
		!Primitive::validateFile( resourceID_ ))
	{
		return false;
	}

	this->load();

	this->onReloaded();

	return true;
}
#endif //ENABLE_RELOAD_MODEL


/**
 *	This method loads the primitive from a .primitive file.
 *
 *	@returns E_FAIL if unsuccessful.
 */
HRESULT Primitive::load( )
{
	BW_GUARD;
	release();
	HRESULT res = E_FAIL;

	// Is there a valid device pointer?
	if( Moo::rc().device() == (void*)NULL )
	{
		return res;
	}

	// find our data
	BinaryPtr indices;
	BW::string::size_type noff = resourceID_.find( ".primitives/" );
	if (noff < resourceID_.size())
	{
		// everything is normal
		noff += 11;
		DataSectionPtr sec = PrimitiveFile::get( resourceID_.substr( 0, noff ) );
		if ( sec )
			indices = sec->readBinary( resourceID_.substr( noff+1 ) );
	}
	else
	{
		// find out where the data should really be stored
		BW::string fileName, partName;
		splitOldPrimitiveName( resourceID_, fileName, partName );

		// read it in from this file
		indices = fetchOldPrimitivePart( fileName, partName );
	}

	// Open the binary resource for this primitive
	//BinaryPtr indices = BWResource::instance().rootSection()->readBinary( resourceID_ );
	if( indices )
	{
		// Get the index header
		const IndexHeader* ih = reinterpret_cast< const IndexHeader* >( indices->data() );

		// Create the right type of primitive
		if( BW::string( ih->indexFormat_ ) == "list" || BW::string( ih->indexFormat_ ) == "list32" )
		{
			D3DFORMAT format = BW::string( ih->indexFormat_ ) == "list" ? D3DFMT_INDEX16 : D3DFMT_INDEX32;
			primType_ = D3DPT_TRIANGLELIST;

			if (Moo::rc().maxVertexIndex() <= 0xffff && 
 				format == D3DFMT_INDEX32 ) 
 			{ 
 				ERROR_MSG( "Primitives::load - unable to create index buffer as 32 bit indices " 
 					"were requested and only 16 bit indices are supported\n" ); 
 				return res; 
 			} 

			DWORD usageFlag = rc().mixedVertexProcessing() ? D3DUSAGE_SOFTWAREPROCESSING : 0;

			// Create the index buffer
			if( SUCCEEDED( res = indexBuffer_.create( ih->nIndices_, format, usageFlag, D3DPOOL_MANAGED ) ) )
			{
				// store the number of elements in the index buffer
				nIndices_ = ih->nIndices_;

				// Get the indices
				const char* pSrc = reinterpret_cast< const char* >( ih + 1 );

				IndicesReference ind = indexBuffer_.lock();
				if (!ind.valid())
				{
					ERROR_MSG( "Primitives::load - lock failed\n" );
					return E_FAIL;
				}

				// Fill in the index buffer.
				ind.fill( pSrc, ih->nIndices_ );
				res = indexBuffer_.unlock( );

				// Add the buffer to the preload list so that it can get uploaded
				// to video memory
				indexBuffer_.addToPreloadList();

				pSrc += ih->nIndices_ * ind.entrySize();

				// Get the primitive groups
				const PrimitiveGroup* pGroup = reinterpret_cast< const PrimitiveGroup* >( pSrc );
				for( int i = 0; i < ih->nTriangleGroups_; i++ )
				{
					primGroups_.push_back( *(pGroup++) );
				}

				maxVertices_ = 0;

				// Go through the primitive groups to find the number of vertices referenced by the index buffer.
				PrimGroupVector::iterator it = primGroups_.begin();
				PrimGroupVector::iterator end = primGroups_.end();

				while( it != end )
				{
					maxVertices_ = max( (uint32)( it->startVertex_ + it->nVertices_ ), maxVertices_ );
					it++;
				}
			}
		}
	}
	else
	{
		ASSET_MSG( "Failed to read binary resource: %s\n", resourceID_.c_str() );
	}
	return res;
}

/**
 *	Gets the index buffer format (i.e. 16bit vs 32bit).
 */
D3DFORMAT Primitive::indicesFormat() const
{
	BW_GUARD;
	return indexBuffer_.valid() ? indexBuffer_.format() : D3DFMT_UNKNOWN;
}

/**
 *	Gets contents of the index buffer directly from the D3D resource.
 */
void Primitive::indicesIB( IndicesHolder& indices )
{
	BW_GUARD;
	IndicesReference ind = indexBuffer_.lock( D3DLOCK_READONLY );
	if (ind.valid())
	{
		indices.assign( ind.indices(), ind.size(), ind.format() );
		indexBuffer_.unlock();
	}
}

/**
 *	Gets the indices for the primitives.
 */
const IndicesHolder& Primitive::indices()
{
	BW_GUARD;
	if (!indices_.valid() && indexBuffer_.valid())
	{
		this->indicesIB( indices_ );
	}

	return indices_;
}

void Primitive::calcGroupOrigins( const VerticesPtr verts )
{
	BW_GUARD;
	Vertices::VertexPositions vertexPositions;
	verts->vertexPositionsVB( vertexPositions );

	PrimGroupVector::iterator it = primGroups_.begin();
	PrimGroupVector::iterator end = primGroups_.end();
	while ( it != end )
	{
		const PrimitiveGroup& group = (*it);
		BoundingBox bb;

		for ( int i = group.startVertex_; 
				i < group.startVertex_ + group.nVertices_ &&
					i < (int)vertexPositions.size(); 
				++i )
		{
			bb.addBounds( vertexPositions[i] );
		}
		if (!bb.insideOut())
		{
			groupOrigins_.push_back( bb.centre() );
		}

		it++;
	}
}

/*
 * Accept an array of precalculated origins. If they are invalid,
 * then they will need to be calculated
 */
bool Primitive::adoptGroupOrigins( BW::vector<Vector3>& origins )
{
	if (origins.size() != primGroups_.size())
	{
		return false;
	}

	groupOrigins_.swap( origins );

	return true;
}

/**
 * This method sets all primitive groups for drawing and loads if necessary.
 */
HRESULT Primitive::setPrimitives( )
{
	BW_GUARD;
	if( !indexBuffer_.valid() )
	{
		HRESULT hr = load();
		if( FAILED ( hr ) )
			return hr;
	}

	IF_NOT_MF_ASSERT_DEV( indexBuffer_.valid() )
	{
		MF_EXIT( "index buffer loaded ok, but not valid?" );
	}

	return indexBuffer_.set();
}

/**
 *	This method draws an individual primitive group, given by index.
 *
 *	@param groupIndex The index of the group to draw.
 *	@returns	E_FAIL if indexes have not been set or groupIndex is invalid, or
 *				if drawing fails for some other reason.
 */
HRESULT Primitive::drawPrimitiveGroup( uint32 groupIndex )
{
	BW_GUARD;	
#ifdef _DEBUG
	IF_NOT_MF_ASSERT_DEV( Moo::rc().device() != NULL )
	{
		return E_FAIL;
	}
	IF_NOT_MF_ASSERT_DEV( groupIndex < primGroups_.size() )
	{
		return E_FAIL;
	}
	IF_NOT_MF_ASSERT_DEV( indexBuffer_.isCurrent() )
	{
		return E_FAIL;
	}
#endif

	// Draw the primitive group
	PrimitiveGroup& pg = primGroups_[ groupIndex ];

	if( pg.nVertices_ && pg.nPrimitives_ && s_primitiveEnableDrawPrim )
	{
		return Moo::rc().drawIndexedPrimitive( primType_, 0, pg.startVertex_, pg.nVertices_, pg.startIndex_, pg.nPrimitives_ );
	}
	return S_OK;
}

/**
 *	This method draws an individual primitive group, given by index.
 *
 *	@param groupIndex The index of the group to draw.
 *	@returns	E_FAIL if indexes have not been set or groupIndex is invalid, or
 *				if drawing fails for some other reason.
 */
HRESULT	Primitive::drawInstancedPrimitiveGroup( uint32 groupIndex, uint32 instanceCount )
{
	BW_GUARD;	
#ifdef _DEBUG
	IF_NOT_MF_ASSERT_DEV( Moo::rc().device() != NULL )
	{
		return E_FAIL;
	}
	IF_NOT_MF_ASSERT_DEV( groupIndex < primGroups_.size() )
	{
		return E_FAIL;
	}
	IF_NOT_MF_ASSERT_DEV( indexBuffer_.isCurrent() )
	{
		return E_FAIL;
	}
#endif

	// Draw the primitive group
	PrimitiveGroup& pg = primGroups_[ groupIndex ];

	if( pg.nVertices_ && pg.nPrimitives_ && s_primitiveEnableDrawPrim )
	{
		return Moo::rc().drawIndexedInstancedPrimitive(
			primType_, 0, pg.startVertex_, pg.nVertices_, pg.startIndex_, pg.nPrimitives_, instanceCount
			);
	}
	return S_OK;
}

/**
 * This method releases all index data and low level resources.
 */
HRESULT Primitive::release( )
{
	BW_GUARD;
	nIndices_ = 0;
	maxVertices_ = 0;
	primGroups_.clear();
	groupOrigins_.clear();
	indexBuffer_.release();
	return S_OK;
}

} // namespace Moo

BW_END_NAMESPACE

// primitive.cpp
