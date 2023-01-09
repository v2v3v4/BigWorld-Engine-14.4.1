#include "pch.hpp"

#include "resmgr/xml_section.hpp"

#include "App/me_app.hpp"

#include "undo_redo.hpp"

#include "modeleditor_core/Models/mutant.hpp"

BW_BEGIN_NAMESPACE

/**
 *	Constructor
 */
UndoRedoOp::UndoRedoOp( int kind, DataSectionPtr data, DataSectionPtr parent /*= NULL*/, bool materialFlagChange /*= false*/, StringPair item /*= StringPair()*/ ):
	UndoRedo::Operation(kind),
	kind_(kind),
	data_(data),
	parent_(parent),
	state_(NULL),
	materialFlagChange_(materialFlagChange),
	item_(item)
{
	BW_GUARD;

	if (!UndoRedo::instance().isUndoing())
	{
		MeApp::instance().mutant()->updateModificationInfo();
	}

	if (parent_)
	{
		MeApp::instance().mutant()->dirty( parent_ );
	}
	
	if (data_)
	{
		state_ = new XMLSection("undo_state");
		state_->copy( data_ );
	}
}


/**
 *	Destructor
 */
UndoRedoOp::~UndoRedoOp()
{
	BW_GUARD;

	data_ = NULL;
}


/**
 *	This method is called by the underlying undo system.
 *	Here we load our stored XML data into the model.
 */
void UndoRedoOp::undo()
{
	BW_GUARD;

	// first add the current state of this block to the undo/redo list
	UndoRedo::instance().add( new UndoRedoOp( kind_, data_, parent_, materialFlagChange_, item_));

	if (data_ != NULL)
	{
		//Make sure we have marked the parent as dirty since it will have changed
		if (parent_)
		{
			MeApp::instance().mutant()->dirty( parent_ );
		}

		//Remove the current data
		data_->delChildren();
				
		//Copy the old state back
		data_->copy( state_ );

		if ( item_ != StringPair() )
		{
			MeApp::instance().mutant()->cleanAnim( item_ );
		}

		MeApp::instance().mutant()->reloadAllLists();
		
		if (materialFlagChange_)
		{
			MeApp::instance().mutant()->reloadBSP();
		}
	}
}


/**
 *	This method checks whether two UndoRedo operations are the same,
 *	so duplicate entries can be discarded.  It simply checks if the
 *	data in the XML sections are the same.  It ignores the state of
 *	the gui, since that is for user convenience, but not so the user
 *	can undo/redo changes in list boxes etc.
 */
bool UndoRedoOp::iseq( const UndoRedo::Operation & oth ) const
{
	return false;
}

/***
 * class UndoRedoMatterName
 */
UndoRedoMatterName::UndoRedoMatterName( const BW::string & oldName, const BW::string & newName ):
	UndoRedo::Operation(1),
	oldName_( oldName ),
	newName_( newName )
{
}

UndoRedoMatterName::~UndoRedoMatterName()
{
}

void UndoRedoMatterName::undo()
{
	BW_GUARD;

	UndoRedo::instance().add( new UndoRedoMatterName( newName_, oldName_ ) );
	MeApp::instance().mutant()->matterName( oldName_, newName_, false );
}

bool UndoRedoMatterName::iseq( const UndoRedo::Operation & oth ) const
{
	return false;
}


/***
 * class UndoRedoTintName
 */
UndoRedoTintName::UndoRedoTintName( const BW::string & matterName, const BW::string & oldName, const BW::string & newName ):
	UndoRedo::Operation(2),
	matterName_( matterName ),
	oldName_( oldName ),
	newName_( newName )
{
}

UndoRedoTintName::~UndoRedoTintName()
{
}

void UndoRedoTintName::undo()
{
	BW_GUARD;

	UndoRedo::instance().add( new UndoRedoTintName( matterName_, newName_, oldName_ ) );
	MeApp::instance().mutant()->tintName( matterName_, oldName_, newName_, false );
}

bool UndoRedoTintName::iseq( const UndoRedo::Operation & oth ) const
{
	return false;
}
BW_END_NAMESPACE

