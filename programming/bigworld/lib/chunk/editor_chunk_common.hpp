#ifndef EDITOR_CHUNK_COMMON_HPP
#define EDITOR_CHUNK_COMMON_HPP

#include "cstdmf/bw_namespace.hpp"

BW_BEGIN_NAMESPACE

class EditorChunkCommonLoadSave
{
public:
#ifdef EDITOR_ENABLED
	EditorChunkCommonLoadSave() : hidden_(false), frozen_(false), selectable_(true) {}
	virtual ~EditorChunkCommonLoadSave(){}

	virtual bool edCommonLoad( DataSectionPtr pSection );
	virtual bool edCommonSave( DataSectionPtr pSection );

	virtual void edCommonChanged() = 0;

	/**
	 * Hide or reveal this item.
	 */
	virtual void edHidden( bool value );
	virtual bool edHidden( ) const;

	/**
	 * Freeze or unfreeze this item.
	 */
	virtual void edFrozen( bool value );
	virtual bool edFrozen( ) const;

	void edSelectable( bool enableSelect ) { selectable_ = enableSelect; }
	bool edSelectable() const { return selectable_; }

private:
	bool hidden_, frozen_, selectable_;
#endif
};

BW_END_NAMESPACE

#endif // EDITOR_CHUNK_COMMON_HPP
