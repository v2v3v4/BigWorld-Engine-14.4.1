#ifndef CHUNK_EDITOR_HPP
#define CHUNK_EDITOR_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "gizmo/general_editor.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class facilitates and controls the editing of a chunk itself,
 *	as opposed to the items within it.
 */
class ChunkEditor : public GeneralEditor
{
	Py_Header( ChunkEditor, GeneralEditor )

public:
	ChunkEditor( Chunk * pChunk, PyTypeObject * pType = &s_type_ );
	~ChunkEditor();

	PyObject * pyGet_contents();
	PY_RO_ATTRIBUTE_SET( contents )

	PyObject * pyGet_isWriteable();
	PY_RO_ATTRIBUTE_SET( isWriteable )

	PY_FACTORY_DECLARE()

private:
	ChunkEditor( const ChunkEditor& );
	ChunkEditor& operator=( const ChunkEditor& );

	Chunk * pChunk_;
};

BW_END_NAMESPACE

#endif // CHUNK_EDITOR_HPP
