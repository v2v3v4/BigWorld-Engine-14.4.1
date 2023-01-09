#include "pch.hpp"
#include "worldeditor/undo_redo/entity_array_undo.hpp"
#include "common/base_properties_helper.hpp"

BW_BEGIN_NAMESPACE

EntityArrayUndo::EntityArrayUndo( BasePropertiesHelper* props, int index ) :
	UndoRedo::Operation(size_t(typeid(EntityArrayUndo).name())),
    props_( props ),
	index_( index )
{
	BW_GUARD;

	undoData_ = props_->propGet( index_ );
}


/*virtual*/ void EntityArrayUndo::undo()
{
	BW_GUARD;

    UndoRedo::instance().add(new EntityArrayUndo( props_, index_ ));
	props_->propSet( index_, undoData_ );
	WorldManager::instance().changedChunk( props_->pItem()->chunk() );
}


/*virtual*/ bool EntityArrayUndo::iseq( const UndoRedo::Operation& other ) const
{
	return false;
}
BW_END_NAMESPACE

