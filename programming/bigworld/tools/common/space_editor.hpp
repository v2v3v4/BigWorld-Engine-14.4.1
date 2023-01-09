#ifndef SPACE_EDITOR_HPP_
#define SPACE_EDITOR_HPP_


#include "cstdmf/debug.hpp"
#include "chunk/invalidate_flags.hpp"

#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_set.hpp"

BW_BEGIN_NAMESPACE

class Chunk;
class ChunkItem;
class EditorChunkItem;


class SpaceEditor
{
public:
	virtual ~SpaceEditor() {}

	virtual void	changedChunk( Chunk* pPrimaryChunk, 
		InvalidateFlags flags = InvalidateFlags::FLAG_THUMBNAIL )	{}
	virtual void	changedChunks( BW::set<Chunk*>& primaryChunks,
		InvalidateFlags flags = InvalidateFlags::FLAG_THUMBNAIL )	{}

	virtual void	changedChunk( Chunk* pPrimaryChunk, 
		EditorChunkItem& changedItem )	{}
	virtual void	changedChunks( BW::set<Chunk*>& primaryChunks,
		EditorChunkItem& changedItem )	{}

	virtual void	changedChunk( Chunk* pPrimaryChunk,
		InvalidateFlags flags,
		EditorChunkItem& changedItem )	{}
	virtual void	changedChunks( BW::set<Chunk*>& primaryChunks, 
		InvalidateFlags flags,
		EditorChunkItem& changedItem )	{}


	virtual void addError( Chunk* chunk, ChunkItem* item, const char * format, ... )	{};

	virtual void onDeleteVLO( const BW::string& id )	{};

	virtual bool isChunkWritable( Chunk* chunk ) const	{ return true;};

	static SpaceEditor& instance( SpaceEditor* editor = NULL )
	{
		static SpaceEditor* s_editor = NULL;

		if (editor)
		{
			// Used in WE passed with WorldManager instance
			s_editor = editor;
		}
		
		MF_ASSERT( s_editor );
		return *s_editor;
	}
};

BW_END_NAMESPACE

#endif //SPACE_EDITOR_HPP_
