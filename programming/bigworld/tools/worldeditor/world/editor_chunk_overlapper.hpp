#ifndef EDITOR_CHUNK_OVERLAPPER_HPP
#define EDITOR_CHUNK_OVERLAPPER_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "chunk/chunk_item.hpp"
#include "chunk/chunk_overlapper.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class is a chunk item that records another chunk overlapping
 *	the one it is in.
 */
class EditorChunkOverlapper : public ChunkOverlapper
{
	DECLARE_EDITOR_CHUNK_ITEM( EditorChunkOverlapper )

public:
	EditorChunkOverlapper();
	~EditorChunkOverlapper();

	bool load( DataSectionPtr pSection, Chunk * pChunk, BW::string* errorString = NULL );

	virtual void toss( Chunk * pChunk );
	virtual void draw( Moo::DrawContext& drawContext );
	virtual void lend( Chunk * pLender );

	DataSectionPtr	pOwnSect()				{ return pOwnSect_; }

	/**
	 * Chunks which should be drawn; must be cleared every frame
	 */
	static BW::vector<Chunk*> drawList;

private:
	EditorChunkOverlapper( const EditorChunkOverlapper& );
	EditorChunkOverlapper& operator=( const EditorChunkOverlapper& );

	void bindStuff();

	DataSectionPtr	pOwnSect_;
	bool			bound_;

	static bool		s_drawAlways_;		// at options: render/scenery/shells/gameVisibility
	static uint32	s_settingsMark_;
};

typedef SmartPointer<EditorChunkOverlapper> EditorChunkOverlapperPtr;

/**
 *	This class keeps track of all the overlappers in a chunk,
 *	and can form and cut them when a chunk is moved.
 */
class EditorChunkOverlappers : public ChunkCache
{
public:
	EditorChunkOverlappers( Chunk & chunk );
	~EditorChunkOverlappers();

	static Instance<EditorChunkOverlappers> instance;

	void add( EditorChunkOverlapperPtr pOverlapper );
	void del( EditorChunkOverlapperPtr pOverlapper );

	void form( Chunk * pOverlapper );
	void cut( Chunk * pOverlapper );

	typedef BW::vector< EditorChunkOverlapperPtr > Items;
	const Items& overlappers() const
	{
		return items_;
	}
private:
	Chunk *				pChunk_;
	Items				items_;
};

BW_END_NAMESPACE

#endif // EDITOR_CHUNK_OVERLAPPER_HPP
