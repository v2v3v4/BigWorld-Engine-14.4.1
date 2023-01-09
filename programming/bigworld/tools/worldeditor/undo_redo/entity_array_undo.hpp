#ifndef ENTITY_ARRAY_UNDO_HPP
#define ENTITY_ARRAY_UNDO_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "gizmo/undoredo.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class handles undoing array operations
 */
class EntityArrayUndo : public UndoRedo::Operation
{
public:
    explicit EntityArrayUndo( BasePropertiesHelper* props, int index );

	/*virtual*/ void undo();

    /*virtual*/ bool iseq( UndoRedo::Operation const &other ) const;

protected:
    BasePropertiesHelper* props_;
	int index_;
	DataSectionPtr undoData_;

private:
	EntityArrayUndo( const EntityArrayUndo& );
	EntityArrayUndo &operator=( const EntityArrayUndo& );
};

BW_END_NAMESPACE

#endif // ENTITY_ARRAY_UNDO_HPP
