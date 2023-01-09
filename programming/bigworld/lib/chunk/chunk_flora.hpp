#ifndef FLORA_CHUNK_ITEM_HPP
#define FLORA_CHUNK_ITEM_HPP

#include "chunk_manager.hpp"
#include "chunk_item.hpp"
#include "chunk.hpp"
#include "chunk_space.hpp"
#include "romp/ecotype.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class is a chunk item that has customised flora data.
 *	Note the drawing is performed by the Flora class; this chunk item
 *	simply loads and provides ecotype information where necessary.
 *	Not all chunks will have ChunkFlora items, and if a chunk does
 *	not have one, then the Flora in the chunk will be auto-generated.
 */
class ChunkFlora : public ChunkItem
{
public:
	DECLARE_CHUNK_ITEM( ChunkFlora )

	static const uint32 VERSION = 2;

	struct Header
	{
		uint32					magic_;	  // Should be "chf\0"		
		uint32					version_; // format version
		uint32					width_;
		uint32					height_;
		static const uint32 MAGIC = '\0fhc';
	};	

	ChunkFlora();
	~ChunkFlora();

	bool load( DataSectionPtr pSection, Chunk * pChunk );
	void toss( Chunk * pChunk );

	Ecotype::ID	ecotypeAt( const Vector2& chunkLocalPosition ) const;
private:
	BinaryPtr	pData_;
	Ecotype::ID* ecotypeIDs_;
	uint32		width_;
	uint32		height_;
	Vector2		spacing_;
};


/**
 *	This class manages the current working set of ChunkFlora objects,
 *	and provides world coordinate access to them.
 */
class ChunkFloraManager
{
public:
	~ChunkFloraManager();
	static ChunkFloraManager& instance()
	{
		return s_instance;
	}

	Ecotype::ID ecotypeAt( const Vector2& worldPosition );

	void	add( ChunkFlora* );
	void	del( ChunkFlora* );
private:

	typedef BW::map<int32, ChunkFlora*> ChunkFloraMap;
	typedef BW::map<int32, ChunkFloraMap*> IntMap;

	void	chunkToGrid( Chunk* pChunk, int32& x, int32& z );
	void	chunkLocalPosition( const Vector3& pos, int32 gridX, int32 gridZ,
								Vector2& ret );

	static ChunkFloraManager s_instance;

	IntMap	items_;
};


#ifdef CODE_INLINE
#include "chunk_flora.ipp"
#endif

BW_END_NAMESPACE

#endif
