#ifndef ITEM_EDITOR_HPP
#define ITEM_EDITOR_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "gizmo/general_editor.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class controls and defines the editing operations which
 *	can be performed on a chunk item.
 */
class ChunkItemEditor : public GeneralEditor
{
	Py_Header( ChunkItemEditor, GeneralEditor )
public:
	ChunkItemEditor( ChunkItemPtr pItem, PyTypeObject * pType = &s_type_ );
	~ChunkItemEditor();

	PY_DEFERRED_ATTRIBUTE_DECLARE( className )
	PY_DEFERRED_ATTRIBUTE_DECLARE( description )
	PY_DEFERRED_ATTRIBUTE_DECLARE( transform )

	PY_FACTORY_DECLARE()

private:
	ChunkItemEditor( const ChunkItemEditor& );
	ChunkItemEditor& operator=( const ChunkItemEditor& );

	ChunkItemPtr	pItem_;

};

BW_END_NAMESPACE

#endif // ITEM_EDITOR_HPP
