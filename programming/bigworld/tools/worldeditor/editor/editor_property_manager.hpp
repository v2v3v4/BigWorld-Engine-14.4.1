#ifndef EDITOR_PROPERTY_MANAGER_HPP
#define EDITOR_PROPERTY_MANAGER_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"

BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: EditorPropertyManagerOperation
// -----------------------------------------------------------------------------

template< class PropertyItemPtr >
class EditorPropertyManagerOperation : public UndoRedo::Operation
{
public:
	EditorPropertyManagerOperation( PropertyItemPtr item, const BW::string & propName ) :
		UndoRedo::Operation( int(typeid(EditorPropertyManagerOperation).name()) ),
		item_( item ),
		propName_( propName )
	{
		BW_GUARD;

		DataSectionPtr section = item_->pOwnSect();
		DataSectionPtr propertiesSection = section->openSection( "properties", true );
		DataSectionPtr propSection = propertiesSection->openSection( propName_ );
		if (!propSection)
		{
			oldData_ = NULL;
		}
		else
		{
			oldData_ = new XMLSection( "temp" );
			oldData_->copy( propSection );
		}
		addChunk( item->chunk() );
	}

private:

	virtual void undo()
	{
		BW_GUARD;

		// first add the current state of this block to the undo/redo list
		UndoRedo::instance().add( new EditorPropertyManagerOperation(
			item_, propName_) );
		UndoRedo::instance().barrier( Localise(L"WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_PROPERTY_MANAGER/ADD_PROPERTY_BARRIER",
			item_->edDescription() ), false );

		// then return to the old data
		DataSectionPtr section = item_->pOwnSect();
		DataSectionPtr propertiesSection = section->openSection( "properties", true );
		DataSectionPtr propSection = propertiesSection->openSection( propName_ );
		MF_ASSERT( propSection );
		if (oldData_)
		{
			propSection->delChildren();
			propSection->copy( oldData_ );
		}
		else
		{
			propertiesSection->delChild( propSection );
		}

		// mark as dirty (for saving)
		WorldManager::instance().changedChunk( item_->chunk() );

		// reload the section
		item_->propHelper()->resetSelUpdate();
	}

	virtual bool iseq( const UndoRedo::Operation & oth ) const
	{
		BW_GUARD;

		return (oldData_ ==
			static_cast<const EditorPropertyManagerOperation&>( oth ).oldData_) &&
			(item_ == 
			static_cast<const EditorPropertyManagerOperation&>( oth ).item_);
	}

	PropertyItemPtr item_;
	BW::string propName_;
	DataSectionPtr oldData_;
};




// -----------------------------------------------------------------------------
// Section: EditorPropertyManager
// -----------------------------------------------------------------------------


// each item property that needs to be managed (added to or removed) gets one of these
template< class PropertyItemPtr >
class EditorPropertyManager : public PropertyManager
{
public:
	EditorPropertyManager( PropertyItemPtr item, 
			const BW::string & propName, const BW::string & defaultItemName)
		: item_( item )
		, propName_( propName )
		, defaultItemName_( defaultItemName )
		, listIndex_( -1 )
	{
	}

	EditorPropertyManager( PropertyItemPtr item, 
			const BW::string & propName, int listIndex )
		: item_( item )
		, propName_( propName )
		, defaultItemName_( "" )
		, listIndex_( listIndex )
	{
	}

	virtual bool canAddItem()
	{
		return !defaultItemName_.empty();
	}

	virtual void addItem()
	{
		BW_GUARD;

		if (defaultItemName_.empty())
			return;

		UndoRedo::instance().add( new EditorPropertyManagerOperation<PropertyItemPtr>( item_, propName_ ) );
		UndoRedo::instance().barrier( Localise(L"WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_PROPERTY_MANAGER/ADD_PROPERTY_BARRIER",
			item_->edDescription() ), false );

		DataSectionPtr section = item_->pOwnSect();
		DataSectionPtr propertiesSection = section->openSection( "properties", true );
		DataSectionPtr propSection = propertiesSection->openSection( propName_, true );
		DataSectionPtr actionSection = propSection->newSection( "item" );
		actionSection->setString( defaultItemName_ );

		// mark as dirty (for saving)
		WorldManager::instance().changedChunk( item_->chunk() );
		item_->edPostModify();

		// reload the section
		item_->propHelper()->resetSelUpdate();
	}

	virtual bool canRemoveItem()
	{
		return listIndex_ != -1;
	}

	virtual void removeItem()
	{
		BW_GUARD;

		if (listIndex_ == -1)
			return;

		UndoRedo::instance().add( new EditorPropertyManagerOperation<PropertyItemPtr>( item_, propName_ ) );
		UndoRedo::instance().barrier(
			Localise(L"WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_PROPERTY_MANAGER/REMOVE_PROPERTY", item_->edDescription() ), false );

		DataSectionPtr section = item_->pOwnSect();
		DataSectionPtr propertiesSection = section->openSection( "properties", true );
		DataSectionPtr propSection = propertiesSection->openSection( propName_ );
		MF_ASSERT( propSection );
		DataSectionPtr childToDelete = propSection->openChild( listIndex_ );
		MF_ASSERT( childToDelete );
		propSection->delChild( childToDelete );

		// mark as dirty (for saving)
		WorldManager::instance().changedChunk( item_->chunk() );
		item_->edPostModify();

		// reload the section
		item_->propHelper()->resetSelUpdate();
	}

private:
	PropertyItemPtr item_;
	BW::string propName_;
	int listIndex_;
	BW::string defaultItemName_;
};

BW_END_NAMESPACE

#endif // EDITOR_PROPERTY_MANAGER_HPP
