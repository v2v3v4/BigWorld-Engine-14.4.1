// editor_chunk_item.ipp

#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif

// -----------------------------------------------------------------------------
// Section: Accessors
// -----------------------------------------------------------------------------

// editor_chunk_item.ipp

BW_BEGIN_NAMESPACE

INLINE void EditorChunkItem::edSelected( BW::vector<ChunkItemPtr>& selection )
{
	isSelected_ = true;
}

INLINE void EditorChunkItem::edDeselected()
{
	isSelected_ = false;
}

INLINE bool EditorChunkItem::edIsSelected() const
{
	return isSelected_;
}

BW_END_NAMESPACE

