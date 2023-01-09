#ifndef UMBRA_CHUNK_ITEM_HPP
#define UMBRA_CHUNK_ITEM_HPP

#include "umbra_config.hpp"

#if UMBRA_ENABLE

#include "umbra_draw_item.hpp"
#include "chunk_item.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class implements the UmbraDrawItem interface for single chunk item.
 *	It handles the drawing of chunk items.
 */
class UmbraChunkItem : public UmbraDrawItem
{
public:
	UmbraChunkItem();
	~UmbraChunkItem();
	virtual Chunk* draw( Moo::DrawContext& drawContext, Chunk* pChunkContext );
	virtual Chunk* drawDepth( Moo::DrawContext& drawContext, Chunk* pChunkContext );

	void init( ChunkItem* pItem, const BoundingBox& bb, const Matrix& transform, Umbra::OB::Cell* pCell );
	void init( ChunkItem* pItem, const Vector3* pVertices, uint32 nVertices, const Matrix& transform, Umbra::OB::Cell* pCell );
	void init( ChunkItem* pItem, UmbraModelProxyPtr pModel, const Matrix& transform, Umbra::OB::Cell* pCell );
	void init( ChunkItem* pItem, UmbraObjectProxyPtr pObject, const Matrix& transform, Umbra::OB::Cell* pCell );
private:
	Chunk*					updateChunk( Moo::DrawContext& drawContext, Chunk* pChunkContext );

	ChunkItem*				pItem_;
};

/**
 *  This object simulates the projection of shadows.
 */
class UmbraChunkItemShadowCaster : public UmbraDrawShadowItem
{
public:
	UmbraChunkItemShadowCaster();
	~UmbraChunkItemShadowCaster();

public:

	virtual Chunk* draw( Moo::DrawContext& drawContext, Chunk* pChunkContext );
	virtual Chunk* drawDepth( Moo::DrawContext& drawContext, Chunk* pChunkContext );

	void init( ChunkItem* pItem, const BoundingBox& bb, const Matrix& transform, Umbra::OB::Cell* pCell, bool isDynamicObject = false );
	void updateTransform( const Matrix& newTransform );
	void update();

	// UmbraDrawShadowItem

private:
	ChunkItem*  pItem_;
	
	BoundingBox shadowBB_;
	Matrix      shadowTr_;

	float		distVisible_;
	bool		isUpdated_;
};

BW_END_NAMESPACE

#endif // UMBRA_ENABLE

#endif // UMBRA_CHUNK_ITEM_HPP
