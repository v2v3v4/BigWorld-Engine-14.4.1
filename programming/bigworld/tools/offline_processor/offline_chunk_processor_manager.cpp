#include "pch.hpp"
#include "offline_chunk_processor_manager.hpp"

#include "chunk/chunk.hpp"
#include "chunk/chunk_format.hpp"
#include "chunk/chunk_manager.hpp"
#include "chunk/chunk_item_amortise_delete.hpp"
#include "chunk/geometry_mapping.hpp"

#include "cstdmf/memory_load.hpp"

#include "resmgr/data_section_cache.hpp"
#include "resmgr/data_section_census.hpp"

#include "terrain/terrain_settings.hpp"

BW_BEGIN_NAMESPACE


namespace
{

unsigned int bw_hash( const char* str )
{
	BW_GUARD;

	static unsigned char hash[ 256 ];
	static bool inithash = true;
	if( inithash )
	{
		inithash = false;
		for( int i = 0; i < 256; ++i )
			hash[ i ] = i;
		int k = 7;
		for( int j = 0; j < 4; ++j )
			for( int i = 0; i < 256; ++i )
			{
				unsigned char s = hash[ i ];
				k = ( k + s ) % 256;
				hash[ i ] = hash[ k ];
				hash[ k ] = s;
			}
	}
	unsigned char result = ( 123 + strlen( str ) ) % 256;
	for( unsigned int i = 0; i < strlen( str ); ++i )
	{
		result = ( result + str[ i ] ) % 256;
		result = hash[ result ];
	}
	return result;
}


OfflineChunkProcessorManager* gProcessorManager_ = NULL;


BOOL WINAPI ConsoleHandlerRoutine( DWORD dwCtrlType )
{
	gProcessorManager_->terminate();

	return FALSE;
}

}


/**
 *	Check if the memory is low.
 *	@param testNow test now or wait.
 */
bool OfflineChunkProcessorManager::isMemoryLow( bool testNow ) const
{
	if ( Memory::memoryLoad() > 90.0f )
	{
		return true;
	}
	return false;
}


/**
 *	Reset chunk marks and then unload the loaded chunks that are not marked.
 *	Does not unload pending or currently loading chunks.
 */
void OfflineChunkProcessorManager::unloadChunks()
{
	const ChunkMap& chunkMap = mapping_->pSpace()->chunks();

	for (ChunkMap::const_iterator iter = chunkMap.begin();
		iter != chunkMap.end(); ++iter)
	{
		for (BW::vector<Chunk*>::const_iterator cit = iter->second.begin();
			cit != iter->second.end(); ++cit)
		{
			(*cit)->removable( true );
		}
	}

	mark();

	this->unloadRemovableChunks();
}




bool OfflineChunkProcessorManager::tick()
{
	MSG msg;

	while (PeekMessage( &msg, NULL, 0, 0, PM_REMOVE))
	{
		if (msg.message == WM_QUIT)
		{
			return false;
		}

		TranslateMessage( &msg );
		DispatchMessage( &msg );
	}

	return !terminated_ && ChunkProcessorManager::tick();
}


bool OfflineChunkProcessorManager::isChunkEditable( const BW::string& chunk ) const
{
	return bw_hash( chunk.c_str() ) % clusterSize_ == clusterIndex_;
}


OfflineChunkProcessorManager::OfflineChunkProcessorManager(
	int numThread, unsigned int clusterSize, unsigned int clusterIndex )
	: ChunkProcessorManager( numThread ), mapping_ ( NULL ),
	clusterSize_( clusterSize ), clusterIndex_( clusterIndex ),
	terminated_( false )
{
	MF_ASSERT( !gProcessorManager_ );

	gProcessorManager_ = this;
	SetConsoleCtrlHandler( ConsoleHandlerRoutine, TRUE );

	SpaceEditor::instance( this );

}


void OfflineChunkProcessorManager::init( const BW::string& spaceDir )
{
	fini();

	ChunkSpacePtr chunkSpace = ChunkManager::instance().space( 1 );
	const Matrix& identityMtx = Matrix::identity;

	mapping_ = chunkSpace->addMapping( SpaceEntryID(), (float*)&identityMtx, spaceDir );
	chunkSpace->terrainSettings()->defaultHeightMapLod( 0 );
	ChunkManager::instance().camera( Matrix::identity, chunkSpace );
	this->ChunkProcessorManager::onChangeSpace();
}


void OfflineChunkProcessorManager::fini()
{
}
BW_END_NAMESPACE

