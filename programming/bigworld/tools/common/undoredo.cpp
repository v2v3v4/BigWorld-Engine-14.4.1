#include "pch.hpp"
#include "undoredo.hpp"

#ifndef CODE_INLINE
#include "undoredo.ipp"
#endif

#include "cstdmf/debug.hpp"

DECLARE_DEBUG_COMPONENT2( "EditorCommon", 0 )


// -----------------------------------------------------------------------------
// Section: UndoRedo
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
UndoRedo::UndoRedo() :
	undoing_( false )
{
	undoList_.push_back( new Barrier() );
}


/**
 *	Destructor.
 */
UndoRedo::~UndoRedo()
{
}


/**
 *	This method returns true if there is an entry in the undo list.
 */
bool UndoRedo::canUndo() const
{
	return (undoList_.size() > 1);
}

/**
 *	This method returns true if there is an entry in the redo list.
 */
bool UndoRedo::canRedo() const
{
	return (!redoList_.empty());
}


/**
 *	Add an operation to the current undo list
 */
void UndoRedo::addUndo( Operation * op )
{
	// see if we already have the same operation on this object
	//  (for operations that replace each other and don't compound)
	Operations & ops = undoList_.back()->ops_;
	for (Operations::iterator it = ops.begin(); it != ops.end(); it++)
	{
		if (*op == **it)
		{
			// keep the original undo, since that sets it to the correct
			// data. I acknowledge a possible problem here when we undo
			// if ops between **it and *op depend on the changes made by
			// by *op ... but this can be avoided by not mixing different
			// kinds of operations (when they don't compound) within the
			// same set ... which will be the normal usage of this class.
			delete op;
			return;
		}
	}

	// ok, record this one then
	ops.push_back( op );
};


/**
 *	Add an operation to the current redo list
 */
void UndoRedo::addRedo( Operation * op )
{
	// we assume that these will closely follow the original undo
	// list order and so do not bother checking for operations
	// on the same piece of data here.

	redoList_.back()->ops_.push_back( op );
}


/**
 *	Undo the most recent set of operations. There should have been
 *	no additions since the last barrier.
 */
void UndoRedo::undo()
{
	// make sure not in some weird state
	MF_ASSERT( !undoing_ );

	// make sure barrier is closed
	MF_ASSERT( undoList_.back()->ops_.empty() );

	// get out now if there's nothing to undo
	if (undoList_.size() == 1) return;

	// ok, apply away then
	undoing_ = true;

	// remove the empty last element of the undo list
	undoList_.pop_back();

	// set up an entry on the redo list
	redoList_.push_back( new Barrier() );
	redoList_.back()->what_ = undoList_.back()->what_;

	// now go through and apply each of the undo operations in turn
	Operations & ops = undoList_.back()->ops_;
	for (Operations::reverse_iterator rit = ops.rbegin(); rit != ops.rend(); rit++)
	{
		(*rit)->undo();
	}

	// then clear the last undo list element
	undoList_.back() = new Barrier();

	// and we're done
	undoing_ = false;
}


/**
 *	Redo the most recent set of operations undone. If there were any
 *	additions since the last undo, then this will do nothing since
 *	the redo list would have been cleared.
 */
void UndoRedo::redo()
{
	// make sure not in some weird state
	MF_ASSERT( !undoing_ );

	// make sure barrier is closed
	MF_ASSERT( undoList_.back()->ops_.empty() );

	// get out now if there's nothing to redo
	if (redoList_.empty()) return;

	// ok then, apply each of the redos in turn, which will add their
	//  undos to the undo list (like they were in the first place)
	Operations & ops = redoList_.back()->ops_;
	for (Operations::reverse_iterator rit = ops.rbegin(); rit != ops.rend(); rit++)
	{
		(*rit)->undo();
	}

	// set the barrier on the reconstructed undo list
	this->barrierInternal( redoList_.back()->what_ );

	// and forget about this redo element
	redoList_.pop_back();
}


/**
 *	Close the current set of undo operations, and move on to the next one.
 *	An error message results if the set was empty.
 */
void UndoRedo::barrier( const BW::string & what, bool skipIfNoChange )
{
	MF_ASSERT( !undoing_ );

	// get rid of any redos ... we're moving forward again
	this->clearRedos();

	if (undoList_.back()->ops_.empty())
	{
		if (skipIfNoChange) return;

		WARNING_MSG( "UndoRedo::barrier: Barrier closed for '%s'"
			"' but no intermediate operations added!\n", what.c_str() );
	}

	this->barrierInternal( what );
}

/**
 *	Internal method to close the barrier
 */
void UndoRedo::barrierInternal( const BW::string & what )
{
	undoList_.back()->what_ = what;
	undoList_.push_back( new Barrier() );
}


/**
 *	Clear any redo operations
 */
void UndoRedo::clearRedos()
{
	redoList_.clear();
}


static BW::string s_noSuchLevel = "";

/**
 *	This method gets info about the given undo level.
 *	It returns an empty string when there is no such level.
 */
const BW::string & UndoRedo::undoInfo( uint32 level ) const
{
	++level;
	if (level >= undoList_.size()) return s_noSuchLevel;
	return undoList_[ undoList_.size()-1 - level ]->what_;
}

/**
 *	This method gets info about the given undo level.
 *	It returns an empty string when there is no such level.
 */
const BW::string & UndoRedo::redoInfo( uint32 level ) const
{
	if (level >= redoList_.size()) return s_noSuchLevel;
	return redoList_[ redoList_.size()-1 - level ]->what_;
}

void UndoRedo::clear()
{
	undoList_.clear();
	redoList_.clear();
	undoList_.push_back( new Barrier() );
}

/**
 *	Instance accessor
 */
UndoRedo & UndoRedo::instance()
{
	static UndoRedo s_instance;
	return s_instance;
}


/**
 *	Barrier destructor ... deletes all operations in its ops_ list
 */
UndoRedo::Barrier::~Barrier()
{
	for (Operations::iterator it = ops_.begin(); it != ops_.end(); it++)
	{
		delete *it;
	};
	ops_.clear();	// for sanity
}

// undoredo.cpp
