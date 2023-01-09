#ifndef UNSAVED_TERRAIN_BLOCKS_HPP__
#define UNSAVED_TERRAIN_BLOCKS_HPP__


#include "terrain/editor_base_terrain_block.hpp"
#include "cstdmf/bw_map.hpp"


BW_BEGIN_NAMESPACE

class Progress;


class UnsavedTerrainBlocks
{
	typedef BW::map<BW::string, Terrain::EditorBaseTerrainBlockPtr> Blocks;
	Blocks blocks_;
	Blocks savedBlocks_;

public:
	void add( Terrain::EditorBaseTerrainBlockPtr block );
	void clear();
	void save( Progress* progress = NULL );

	bool empty() const;

	void filter( const UnsavedTerrainBlocks& utb );
};

BW_END_NAMESPACE

#endif//UNSAVED_TERRAIN_BLOCKS_HPP__
