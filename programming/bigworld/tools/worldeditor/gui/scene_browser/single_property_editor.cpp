
#include "pch.hpp"
#include "single_property_editor.hpp"
#include "worldeditor/editor/chunk_item_placer.hpp"
#include "worldeditor/world/world_manager.hpp"

BW_BEGIN_NAMESPACE

/**
 *  Constructor
 */
SinglePropertyEditor::SinglePropertyEditor( ChunkItemPtr pItem,
			const ItemInfoDB::Type & type, const BW::string & newVal ) :
	pItem_( pItem ),
	type_( type ),
	newVal_( newVal ),
	editHidden_( type ==
			ItemInfoDB::Type::builtinType( ItemInfoDB::Type::TYPEID_HIDDEN ) ),
	editFrozen_( type ==
			ItemInfoDB::Type::builtinType( ItemInfoDB::Type::TYPEID_FROZEN ) ),
	pProperty_( NULL )
{
	BW_GUARD;
}


/**
 *  Destructor
 */
SinglePropertyEditor::~SinglePropertyEditor()
{
	BW_GUARD;

	if (pProperty_)
	{
		if (pProperty_->valueType().desc() == ValueTypeDesc::BOOL)
		{
			PyObject * pVal = NULL;
			if (pProperty_->valueType().fromString( newVal_, pVal ))
			{
				pProperty_->pySet( pVal, false, false );
				Py_DECREF( pVal );
			}
			else
			{
				ERROR_MSG( "SceneBrowser error editing bool property '%s'\n",
							pProperty_->name() );
			}
		}
		else
		{
			ERROR_MSG(
					"SinglePropertyEditor can't edit non-bool properties.\n" );
		}
	}
	else if (editHidden_)
	{
		bool hidden = false;
		type_.valueType().fromString( newVal_, hidden );
		BW::vector<ChunkItemPtr> items;
		items.push_back( pItem_ );
		ChunkItemPlacer::hideChunkItems( items, hidden, true );
	}
	else if (editFrozen_)
	{
		bool frozen = false;
		type_.valueType().fromString( newVal_, frozen );
		BW::vector<ChunkItemPtr> items;
		items.push_back( pItem_ );
		ChunkItemPlacer::freezeChunkItems( items, frozen, true );
	}
}


/**
 *	This method gets the desired chunk item property and stores it for
 *	changing it later.
 */
void SinglePropertyEditor::addProperty( GeneralProperty * pProp )
{
	BW_GUARD;

	properties_.push_back( pProp );

	if (!pProperty_ && !editHidden_ && !editFrozen_)
	{
		if (pProp->name() == type_.name() &&
			type_.valueType() == pProp->valueType())
		{
			pProperty_ = pProp;
		}
	}
}
BW_END_NAMESPACE

