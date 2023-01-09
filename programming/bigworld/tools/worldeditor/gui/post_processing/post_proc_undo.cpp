

#include "pch.hpp"
#include "post_proc_undo.hpp"

BW_BEGIN_NAMESPACE

namespace
{
	// Flag to know if an undo has occurred => recreate the chain.
	bool s_undoRedoDone = false;

} // anonymous namespace


//-----------------------------------------------------------------------------
//	Section: PostProcUndo
//-----------------------------------------------------------------------------

/**
 *	This static method returns whether or not an undo was performed.
 *
 *	@return Whether or not an undo was performed.
 */
/*static*/ bool PostProcUndo::undoRedoDone()
{
	return s_undoRedoDone;
}


/**
 *	This static method sets whether or not an undo operation was performed.
 *
 *	@param val	 Whether or not an undo operation was performed.
 */
/*static*/ void PostProcUndo::undoRedoDone( bool val )
{
	s_undoRedoDone = val;
}


//-----------------------------------------------------------------------------
//	Section: ChainUndoOp
//-----------------------------------------------------------------------------


/**
 *	Constructor.
 */
ChainUndoOp::ChainUndoOp() :
  UndoRedo::Operation( size_t(typeid(ChainUndoOp).name()) )
{
	BW_GUARD;

	PyObject * pPP = PyImport_AddModule( "PostProcessing" );
	if (pPP)
	{
		PyObject * pChainAttr = PyObject_GetAttrString( pPP, "chain" );
		if (pChainAttr)
		{
			PyObjectPtr pChain( Script::ask( pChainAttr, PyTuple_New(0) ),
								PyObjectPtr::STEAL_REFERENCE );

			if (pChain && PySequence_Check( pChain.get() ))
			{
				for (int i = 0; i < PySequence_Size( pChain.get() ); ++i)
				{
					savedChain_.push_back(
						PyObjectPtr( PySequence_GetItem( pChain.get(), i ), PyObjectPtr::STEAL_REFERENCE ) );
				}
			}
		}
	}
	
	if (PyErr_Occurred())
	{
		PyErr_Print();
	}
}


/**
 *	This method performs the undo on the whole chain.
 */
void ChainUndoOp::undo()
{
	BW_GUARD;

	UndoRedo::instance().add( new ChainUndoOp() );

	PyObject * pPP = PyImport_AddModule( "PostProcessing" );
	if (pPP)
	{
		PyObjectPtr pChain( PyList_New( 0 ), PyObjectPtr::STEAL_REFERENCE );

		for (BW::vector< PyObjectPtr >::iterator it = savedChain_.begin();
			it != savedChain_.end(); ++it)
		{
			PyList_Append( pChain.get(), (*it).get() );
		}

		// Set this to false so we can change the chain.
		WorldManager::instance().userEditingPostProcessing( false );

		PyObject * pChainAttr = PyObject_GetAttrString( pPP, "chain" );
		if (pChainAttr)
		{
			Script::call( pChainAttr, Py_BuildValue( "(O)", pChain.get() ) );
		}
		else
		{
			ERROR_MSG( "Failed to undo/redo the post-processing chain\n" );
		}

		// User is editing the panel... don't allow changes underneath it
		WorldManager::instance().userEditingPostProcessing( true );

		// Restore the changed state of the chain. This state is used for
		// when someone else (Weather, Graphic Settings) changes it, but
		// here we changed it ourselves.
		WorldManager::instance().changedPostProcessing( false );

		// Make sure the panel knows an undo or redo was done.
		s_undoRedoDone = true;
	}
	
	if (PyErr_Occurred())
	{
		PyErr_Print();
	}
}


/**
 *	This method simply returns false always to indicate that all undo
 *	operations of this type are unique.
 */
bool ChainUndoOp::iseq( const UndoRedo::Operation & oth ) const
{
	return false;
}


//-----------------------------------------------------------------------------
//	Section: PhasesUndoOp
//-----------------------------------------------------------------------------

/**
 *	Constructor.
 */
PhasesUndoOp::PhasesUndoOp( PostProcessing::EffectPtr effect ) :
  UndoRedo::Operation( size_t(typeid(PhasesUndoOp).name()) ),
  effect_( effect )
{
	BW_GUARD;

	PyObjectPtr pPhases( PyObject_GetAttrString( effect_.get(), "phases" ), PyObjectPtr::STEAL_REFERENCE );
	if (pPhases && PySequence_Check( pPhases.get() ))
	{
		for (int i = 0; i < PySequence_Size( pPhases.get() ); ++i)
		{
			savedPhases_.push_back(
				PyObjectPtr( PySequence_GetItem( pPhases.get(), i ), PyObjectPtr::STEAL_REFERENCE ) );
		}
	}
}


/**
 *	This method performs the undo on the list of phases of an Effect.
 */
void PhasesUndoOp::undo()
{
	BW_GUARD;

	UndoRedo::instance().add( new PhasesUndoOp( effect_ ) );

	PyObjectPtr pPhases( PyList_New( 0 ), PyObjectPtr::STEAL_REFERENCE );

	for (BW::vector< PyObjectPtr >::iterator it = savedPhases_.begin();
		it != savedPhases_.end(); ++it)
	{
		PyList_Append( pPhases.get(), (*it).get() );
	}

	PyObject_SetAttrString( effect_.get(), "phases", pPhases.get() );

	// User is undoing or redoing, which is a form of editing.
	WorldManager::instance().userEditingPostProcessing( true );

	// Make sure the panel knows an undo or redo was done.
	s_undoRedoDone = true;
}


/**
 *	This method simply returns false always to indicate that all undo
 *	operations of this type are unique.
 */
bool PhasesUndoOp::iseq( const UndoRedo::Operation & oth ) const
{
	return false;
}

BW_END_NAMESPACE

