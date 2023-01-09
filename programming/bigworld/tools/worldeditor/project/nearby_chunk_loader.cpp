#include "pch.hpp"
#include "worldeditor/project/nearby_chunk_loader.hpp"
#include "worldeditor/project/space_helpers.hpp"
#include "worldeditor/world/world_manager.hpp"

BW_BEGIN_NAMESPACE

NearbyChunkLoader::NearbyChunkLoader( int32 minLGridX, int32 maxLGridX, int32 minLGridY, int32 maxLGridY ):
maxLGridY_(maxLGridY),
	minLGridY_(minLGridY),
	maxLGridX_(maxLGridX),
	minLGridX_(minLGridX)
{

}

NearbyChunkLoader::~NearbyChunkLoader()
{
	//If it fails here, it means that you forget to call NearbyChunkLoader::releaseUnusedChunks
	MF_ASSERT( xToRelease_.size() == 0 && yToRelease_.size() == 0 ); 
}

void NearbyChunkLoader::loadChunks( int16 x, int16 z )
{
	BW_GUARD;

	for ( int xPos = x - 1; xPos <= x+1; xPos++ )
	{
		for ( int yPos = z - 1; yPos <= z+1; yPos++ )
		{
			if (xPos < minLGridX_ ||
				xPos > maxLGridX_ ||
				yPos < minLGridY_ ||
				yPos > maxLGridY_)
			{
				continue;
			}
			bool loadedByCaller = false;
			BW::string chunkName = "";
			chunkID( chunkName, xPos, yPos );
			Chunk *pChunk = WorldManager::instance().getChunk( chunkName, true, &loadedByCaller );

			//record chunks to be released
			if ( x == maxLGridX_ && xPos == x )
			{
				yToRelease_.push_back( yPos );
				xToRelease_.push_back( xPos );
			}
			else if ( xPos == x - 1)
			{
				yToRelease_.push_back( yPos );
				xToRelease_.push_back( xPos );
			}

			if ( loadedByCaller )
			{
				xLoaded_.push_back( xPos );
				yLoaded_.push_back( yPos );
			}
		}
	}
}

bool NearbyChunkLoader::isChunkLoadedByMe( int16 x, int16 y )
{
	BW_GUARD;
	for ( BW::vector<uint16>::iterator i = xLoaded_.begin(); i != xLoaded_.end(); ++i )
	{
		int16 lX = (*i);
		if ( lX == x )
		{
			for ( BW::vector<uint16>::iterator i2 = yLoaded_.begin(); i2 != yLoaded_.end(); ++i2 )
			{
				int16 lY = (*i2);

				if ( lY == y )
				{
					xLoaded_.erase( i );
					yLoaded_.erase( i2 );
					return true;
				}
			}
		}
	}

	return false;
}

void NearbyChunkLoader::releaseUnusedChunks( BW::set<Chunk*>& unloadedChunks )
{
	BW_GUARD;
	BW::string chunkName = "";
	for ( int i = 0; i < (int)xToRelease_.size(); i++ )
	{
		int16 gridX = xToRelease_[i];
		int16 gridZ = yToRelease_[i];
		if ( !this->isChunkLoadedByMe( gridX, gridZ ) )
		{
			continue;
		}
		chunkID( chunkName, gridX, gridZ );
		if ( !chunkName.empty() )
		{
			Chunk *pChunk = WorldManager::instance().getChunk( chunkName, false );
			if ( pChunk )
			{
				unloadedChunks.insert( pChunk );
				if (pChunk->isBound())
				{
					pChunk->unbind( false );
				}
				pChunk->unload();
			}
		}
	}

	xToRelease_.clear();
	yToRelease_.clear();

}

BW_END_NAMESPACE

