#include "pch.hpp"
#include "cstdmf/progress.hpp"
#include "unsaved_terrain_blocks.hpp"


BW_BEGIN_NAMESPACE

void UnsavedTerrainBlocks::add( Terrain::EditorBaseTerrainBlockPtr block )
{
	BW_GUARD;
	blocks_[block->resourceName()] = block;
}


void UnsavedTerrainBlocks::clear()
{
	BW_GUARD;
	blocks_.clear();
	savedBlocks_.clear();
}


void UnsavedTerrainBlocks::save( Progress* progress )
{
	BW_GUARD;

	if (progress)
	{
		progress->length( float( blocks_.size() ) );
	}

	for (Blocks::iterator iter = blocks_.begin();
		iter != blocks_.end(); ++iter)
	{
		Terrain::EditorBaseTerrainBlockPtr block = iter->second;
		BW::string filename = block->resourceName();
		BW::string::size_type posDot = filename.find_last_of( '.' );

		filename = filename.substr( 0, posDot + 6 );
		block->save( filename );
		savedBlocks_[block->resourceName()] = block;

		if (progress)
		{
			progress->step();;
		}
	}
}


bool UnsavedTerrainBlocks::empty() const
{
	return blocks_.empty();
}


void UnsavedTerrainBlocks::filter( const UnsavedTerrainBlocks& utb )
{
	BW_GUARD;

	for (Blocks::iterator iter = blocks_.begin();
		iter != blocks_.end();)
	{
		if (utb.savedBlocks_.find( iter->first ) != utb.savedBlocks_.end())
		{
			iter = blocks_.erase( iter );
		}
		else
		{
			++iter;
		}
	}
}

BW_END_NAMESPACE

// unsaved_terrain_blocks.cpp
