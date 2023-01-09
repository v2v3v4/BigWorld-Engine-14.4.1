#include "pch.hpp"

#include "chunk_flora.hpp"
#include "chunk_manager.hpp"
#include "geometry_mapping.hpp"

#include "cstdmf/debug.hpp"
#include "resmgr/bin_section.hpp"

DECLARE_DEBUG_COMPONENT2( "Chunk", 0 )


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "chunk_flora.ipp"
#endif

//-----------------------------------------------------------------------------
//	Section : ChunkFlora
//-----------------------------------------------------------------------------

///Constructor.
ChunkFlora::ChunkFlora():
	pData_(NULL),
	ecotypeIDs_(NULL),
	width_(0),
	height_(0),
	spacing_(0.f,0.f)
{
}


///Destructor.
ChunkFlora::~ChunkFlora()
{
}


/**
 *	This method loads and initalises a ChunkFlora item. 
 */
bool ChunkFlora::load( DataSectionPtr pSection, Chunk * pChunk )
{
	BW_GUARD;
	BW::string resName = pSection->readString( "resource" );

	DataSectionPtr pData = BWResource::openSection( pChunk->mapping()->path() + resName );

	const float gridSize = pChunk->space()->gridSize();
	if (pData)
	{
		pData_ = pData->asBinary();
		if (pData_->len() < sizeof(Header))
		{
			ERROR_MSG( "ChunkFlora data for chunk %s is smaller than the "
				"required header size.\n", pChunk->resourceID().c_str() );
			return false;
		}

		Header* header = (Header*)(pData_->cdata());
		if (header->version_ != ChunkFlora::VERSION)
		{
			WARNING_MSG( "ChunkFlora data for chunk %s contains the wrong "
				"version number (is %d, required %d).  Ignoring data.\n",
				pChunk->resourceID().c_str(), header->version_,
				ChunkFlora::VERSION );
			return false;
		}

		uint32 dataLen = pData_->len() - sizeof(Header);
		uint32 nIDs = pData_->len() / sizeof(Ecotype::ID);
		width_ = header->width_;
		height_ = header->height_;
		uint32 nBytesReqd = width_ * height_ * sizeof(Ecotype::ID) + sizeof(Header);
		if (nBytesReqd != pData_->len())
		{
			ERROR_MSG( "ChunkFlora data for chunk %s does not contain the "
				"correct amount of data (wanted %d, got %d)\n",
				pChunk->resourceID().c_str(), nBytesReqd, pData_->len() );
			return false;
		}

		char* cData = pData_->cdata();
		cData += sizeof(Header);
		ecotypeIDs_ = (Ecotype::ID*)(cData);
		spacing_ = Vector2(gridSize / (float)width_, gridSize / (float)height_);
		return true;
	}
	//create debug data
	/*else
	{
		int n = 6;
		Ecotype::ID* ids = new Ecotype::ID[n*n];
		memset( ids, 1, n*n );
		Header h;
		h.height_ = n;
		h.width_ = n;
		h.magic_ = Header::MAGIC;
		h.version_ = ChunkFlora::VERSION;
		uint32 len = sizeof(h) + sizeof(Ecotype::ID)*n*n;
		char *data = new char[len];
		memcpy(data, &h, sizeof(h));
		memcpy(data+sizeof(h), ids, sizeof(Ecotype::ID)*n*n);
		pData_ = new BinaryBlock( data, len );
		BinSection* pData = new BinSection( "float", pData_ );		
		pData->save(pChunk->mapping()->path() + resName);
		pData_ = NULL;		
	}*/

	return false;
}


/**
 *	This method is called when a chunk item is added or removed from a chunk.
 */
void ChunkFlora::toss( Chunk * pChunk )
{
	BW_GUARD;
	if (pChunk_ != NULL)
	{
		ChunkFloraManager::instance().del( this );
	}

	this->ChunkItem::toss( pChunk );

	if (pChunk_ != NULL)
	{	
		ChunkFloraManager::instance().add( this );
	}
}


/**
 *	This static method creates a flora from the input section and adds
 *	it to the given chunk.
 */
ChunkItemFactory::Result ChunkFlora::create( Chunk * pChunk, DataSectionPtr pSection )
{
	BW_GUARD;
	ChunkFlora * pFlora = new ChunkFlora();

	if (!pFlora->load( pSection, pChunk ))
	{
		bw_safe_delete(pFlora);
		return ChunkItemFactory::Result( NULL );
	}
	else
	{
		pChunk->addStaticItem( pFlora );
		return ChunkItemFactory::Result( pFlora );
	}
}


/// Static factory initialiser
ChunkItemFactory ChunkFlora::factory_( "flora", 0, ChunkFlora::create );


//-----------------------------------------------------------------------------
//	Section : ChunkFloraManager
//-----------------------------------------------------------------------------
ChunkFloraManager ChunkFloraManager::s_instance;

/**
 *	This method adds a Chunk Flora object to the manager.  From here on in
 *	it can provide custom flora information to the Flora system.
 *
 *	@param	pChunkFlora	pointer to the ChunkFlora object.
 */
void ChunkFloraManager::add( ChunkFlora* pChunkFlora )
{
	BW_GUARD;
	Chunk* pChunk = pChunkFlora->chunk();
	IF_NOT_MF_ASSERT_DEV( pChunk )
	{
		return;
	}

	int32 gridX, gridZ;
	this->chunkToGrid( pChunk, gridX, gridZ );

	IntMap::iterator it = items_.find(gridX);
	if (it != items_.end())
	{
		ChunkFloraMap& cfm = *it->second;
		cfm.insert( std::make_pair(gridZ,pChunkFlora) );
	}
	else
	{
		items_.insert( std::make_pair(gridX,new ChunkFloraMap) );
		it = items_.find(gridX);
		MF_ASSERT_DEV(it != items_.end());

		if( it != items_.end() )
		{
			ChunkFloraMap& cfm = *it->second;
			cfm.insert( std::make_pair(gridZ,pChunkFlora) );
		}
	}
}


/**
 *	This method removes a Chunk Flora object from the manager.
 *	The manager cleans up its own allocated structures in this
 *	method when it can.  This means that if you remove all
 *	ChunkFlora objects that were added, the ChunkFloraManager
 *	will have cleaned up all memory - thus nothing needs to be
 *	done in the destructor.
 *
 *	@param	pChunkFlora	pointer to the ChunkFlora object.
 */
void ChunkFloraManager::del( ChunkFlora* pChunkFlora )
{
	BW_GUARD;
	Chunk* pChunk = pChunkFlora->chunk();
	IF_NOT_MF_ASSERT_DEV( pChunk )
	{
		return;
	}

	int32 gridX, gridZ;
	this->chunkToGrid( pChunk, gridX, gridZ );

	IntMap::iterator it = items_.find(gridX);
	if (it != items_.end())
	{
		ChunkFloraMap& cfm = *it->second;
		ChunkFloraMap::iterator cit = cfm.find(gridZ);
		if (cit != cfm.end())
		{
			cfm.erase(cit);
			if (cfm.empty())
			{
				bw_safe_delete(it->second);
				items_.erase(it);
			}
		}
	}
}


/**
 *	Destructor.
 *	By this time, all ChunkFlora items should have been removed from
 *	the manager, and this means all our internal data structures will
 *	have been freed too.
 */
ChunkFloraManager::~ChunkFloraManager()
{
	BW_GUARD;
	if (!items_.empty())
	{
		WARNING_MSG( "ChunkFloraManager - items are not empty.  This means the "
			"ChunkFloraItems have not been tossed out, meaning perhaps the "
			"space has not been cleaned up properly.\n" );
	}
}


/**
 *	This method returns the ecotype ID at the given position.
 *	If there is custom IDs at the position, it looks it up and
 *	uses that information.  Otherwise, it returns ECOTYPE_ID_AUTO
 *	meaning, make up your own ecotype ID.
 */
Ecotype::ID ChunkFloraManager::ecotypeAt( const Vector2 & worldPos )
{
	BW_GUARD;
	Vector3 pos( worldPos.x, 0.f, worldPos.y );

	// TODO: At the moment, assuming the space the camera is in.
	ChunkSpacePtr pSpace = ChunkManager::instance().cameraSpace();

	//find the chunk
	if (pSpace)
	{
		ChunkSpace::Column* pColumn = pSpace->column( pos, false );

		if ( pColumn != NULL )
		{
			Chunk * pChunk = pColumn->pOutsideChunk();

			if (pChunk != NULL)
			{
				int32 gridX, gridZ;
				this->chunkToGrid( pChunk, gridX, gridZ );

				IntMap::iterator it = items_.find(gridX);
				if (it != items_.end())
				{
					ChunkFloraMap& cfm = *it->second;
					ChunkFloraMap::iterator cit = cfm.find(gridZ);
					if (cit != cfm.end())
					{
						ChunkFlora* cf = cit->second;
						Vector2 clPos;
						this->chunkLocalPosition( pos, gridX, gridZ, clPos );
						return cf->ecotypeAt(clPos);
					}
				}
			}
		}
	}

	return Ecotype::ID_AUTO;
}

BW_END_NAMESPACE

// chunk_flora.cpp
