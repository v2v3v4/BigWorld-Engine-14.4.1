#ifndef VLO_EXISTENCE_OPERATION_HPP
#define VLO_EXISTENCE_OPERATION_HPP

#include "gizmo/undoredo.hpp"

BW_BEGIN_NAMESPACE

class Chunk;
class ChunkVLO;

typedef SmartPointer<ChunkVLO> ChunkVLOPtr;

// -----------------------------------------------------------------------------
// Section: VLOExistenceOperation
// -----------------------------------------------------------------------------
class VLOExistenceOperation
	: public UndoRedo::Operation
{

public:
	VLOExistenceOperation( ChunkVLOPtr pItem, Chunk * pOldChunk );

protected:
	virtual void undo();
	virtual bool iseq( const UndoRedo::Operation & oth ) const;

	ChunkVLOPtr	pItem_;
	Chunk *		pOldChunk_;
};

BW_END_NAMESPACE

#endif // VLO_EXISTENCE_OPERATION_HPP
