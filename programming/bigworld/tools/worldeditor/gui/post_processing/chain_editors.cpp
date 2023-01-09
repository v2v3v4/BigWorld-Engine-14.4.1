
#include "pch.hpp"
#include "chain_editors.hpp"
#include "post_processing/effect.hpp"
#include "post_processing/phase.hpp"

BW_BEGIN_NAMESPACE

//-----------------------------------------------------------------------------
//	Section: AddEffectEditor
//-----------------------------------------------------------------------------

/**
 *	Constructor.
 *
 *	@param effectName	Name of the effect to add.
 *	@param beforeNode	Node to insert the effect before it, or NULL to insert
 *						the effect last.
 */
AddEffectEditor::AddEffectEditor( const BW::wstring & effectName, EffectNodePtr beforeNode ) :
	beforeNode_( beforeNode )
{
	BW_GUARD;

	PyObject * pPP = PyImport_AddModule( "PostProcessing" );
	if (pPP)
	{
		BW::string effectFuncName = bw_wtoutf8( effectName );
		PyObject * pFactoryFn = PyObject_GetAttrString( pPP, "effectFactory" );
		if (pFactoryFn)
		{
			pyNewEffect_ = PyObjectPtr( Script::ask( pFactoryFn, Py_BuildValue("(s)", effectFuncName.c_str()) ),
										PyObjectPtr::STEAL_REFERENCE );
		}
	}

	if (PyErr_Occurred())
	{
		PyErr_Print();
	}
	
	if (!pyNewEffect_)
	{
		ERROR_MSG( "Couldn't add Post Processing Effect '%s'\n", bw_wtoutf8( effectName ).c_str() );
	}
}


/**
 *	Constructor.
 *
 *	@param pyEffect		Effect object.
 *	@param beforeNode	Node to insert the effect before it, or NULL to insert
 *						the effect last.
 */
AddEffectEditor::AddEffectEditor( PyObjectPtr pyEffect, EffectNodePtr beforeNode ) :
	pyNewEffect_( pyEffect ),
	beforeNode_( beforeNode )
{
	BW_GUARD;
}


/**
 *	This method return whether or not this editor is OK and ready.
 *
 *	@return	Whether or not this editor is OK and ready.
 */
bool AddEffectEditor::isOK() const
{
	BW_GUARD;

	return pyNewEffect_ != NULL;
}


/**
 *	This method adds the effect at the position specified during construction.
 *
 *	@param pChain	Original post-processing chain.
 *	@param i		Index of the effect currently being iterated.
 *	@param pNewChain	New, modified post-processing chain.
 */
void AddEffectEditor::modify( PyObjectPtr pChain, int i, PyObjectPtr pNewChain )
{
	BW_GUARD;

	PyObjectPtr pyEffect;
	
	if (i >=0 && i < PySequence_Size( pChain.get() ))
	{
		pyEffect = PyObjectPtr( PySequence_GetItem( pChain.get(), i ), PyObjectPtr::STEAL_REFERENCE );
	}

	if ((beforeNode_ && beforeNode_->pyEffect().get() == pyEffect.get()) ||
		(!beforeNode_ && !pyEffect))
	{
		PyList_Append( pNewChain.get(), pyNewEffect_.get() );
	}

	if (pyEffect)
	{
		PyList_Append( pNewChain.get(), pyEffect.get() );
	}
}


//-----------------------------------------------------------------------------
//	Section: DeleteEffectEditor
//-----------------------------------------------------------------------------

/**
 *	Constructor.
 *
 *	@param deleteNode	Effect to delete.
 */
DeleteEffectEditor::DeleteEffectEditor( EffectNodePtr deleteNode /* NULL for delete all */ ) :
	deleteNode_( deleteNode )
{
	BW_GUARD;
}


/**
 *	This method return whether or not this editor is OK and ready.
 *
 *	@return	Whether or not this editor is OK and ready.
 */
bool DeleteEffectEditor::isOK() const
{
	return true;
}


/**
 *	This method deletes the effect specified during construction.
 *
 *	@param pChain	Original post-processing chain.
 *	@param i		Index of the effect currently being iterated.
 *	@param pNewChain	New, modified post-processing chain.
 */
void DeleteEffectEditor::modify( PyObjectPtr pChain, int i, PyObjectPtr pNewChain )
{
	BW_GUARD;

	if (!deleteNode_)
	{
		// insert none into pNewChain = delete all
		return;
	}

	if (i >=0 && i < PySequence_Size( pChain.get() ))
	{
		PyObjectPtr pyEffect( PySequence_GetItem( pChain.get(), i ), PyObjectPtr::STEAL_REFERENCE );

		if (deleteNode_->pyEffect().get() != pyEffect.get())
		{
			PyList_Append( pNewChain.get(), pyEffect.get() );
		}
	}
}


//-----------------------------------------------------------------------------
//	Section: MoveEffectEditor
//-----------------------------------------------------------------------------

/**
 *	Constructor.
 *
 *	@param moveNode		Effect to move.
 *	@param beforeNode	Node to move the effect before it, or NULL to move
 *						the effect last.
 */
MoveEffectEditor::MoveEffectEditor( EffectNodePtr moveNode, EffectNodePtr beforeNode ) :
	moveNode_( moveNode ),
	beforeNode_( beforeNode )
{
	BW_GUARD;
}


/**
 *	This method return whether or not this editor is OK and ready.
 *
 *	@return	Whether or not this editor is OK and ready.
 */
bool MoveEffectEditor::isOK() const
{
	return true;
}


/**
 *	This method moves the effect to the position specified during construction.
 *
 *	@param pChain	Original post-processing chain.
 *	@param i		Index of the effect currently being iterated.
 *	@param pNewChain	New, modified post-processing chain.
 */
void MoveEffectEditor::modify( PyObjectPtr pChain, int i, PyObjectPtr pNewChain )
{
	BW_GUARD;

	PyObjectPtr pyEffect;

	if (i >=0 && i < PySequence_Size( pChain.get() ))
	{
		pyEffect = PyObjectPtr( PySequence_GetItem( pChain.get(), i ), PyObjectPtr::STEAL_REFERENCE );
	}

	if (moveNode_->pyEffect().get() != pyEffect.get())
	{
		if ((beforeNode_ && beforeNode_->pyEffect().get() == pyEffect.get()) ||
			(!beforeNode_ && !pyEffect))
		{
			PyList_Append( pNewChain.get(), moveNode_->pyEffect().get() );
		}

		if (pyEffect)
		{
			PyList_Append( pNewChain.get(), pyEffect.get() );
		}
	}
}


//-----------------------------------------------------------------------------
//	Section: AddPhaseEditor
//-----------------------------------------------------------------------------


/**
 *	Constructor.
 *
 *	@param phaseName	Name of the new phase to add.
 *	@param beforePhase	Node to move the phase before it, or NULL to move
 *						the phase last.
 *	@param pEditorPhasesModule	Phases module, where the phase factories are.
 */
AddPhaseEditor::AddPhaseEditor( const BW::wstring & phaseName, PhaseNodePtr beforePhase, PyObjectPtr pEditorPhasesModule ) :
	beforePhase_( beforePhase )
{
	BW_GUARD;

	if (pEditorPhasesModule)
	{
		BW::string phaseFuncName = bw_wtoutf8( phaseName );
		PyObject * pFactoryFn = PyObject_GetAttrString( pEditorPhasesModule.get(), "phaseFactory" );
		if (pFactoryFn)
		{
			pyNewPhase_ = PyObjectPtr( Script::ask( pFactoryFn, Py_BuildValue("(s)", phaseFuncName.c_str()) ),
										PyObjectPtr::STEAL_REFERENCE );
		}
	}

	if (PyErr_Occurred())
	{
		PyErr_Print();
	}
	
	if (!pyNewPhase_)
	{
		ERROR_MSG( "Couldn't add Post-Processing Phase '%s'\n", bw_wtoutf8( phaseName ).c_str() );
	}
}


/**
 *	Constructor.
 *
 *	@param pyPhase	Phase to add.
 *	@param beforePhase	Node to move the phase before it, or NULL to move
 *						the phase last.
 */
AddPhaseEditor::AddPhaseEditor( PyObjectPtr pyPhase, PhaseNodePtr beforePhase ) :
	pyNewPhase_( pyPhase ),
	beforePhase_( beforePhase )
{
	BW_GUARD;
}


/**
 *	This method return whether or not this editor is OK and ready.
 *
 *	@return	Whether or not this editor is OK and ready.
 */
bool AddPhaseEditor::isOK() const
{
	BW_GUARD;

	return pyNewPhase_ != NULL;
}


/**
 *	This method adds the phase at the position specified during construction.
 *
 *	@param pPhases	Original list of phases.
 *	@param i		Index of the phase currently being iterated.
 *	@param pNewPhases	New, modified list of phases.
 */
void AddPhaseEditor::modify( PyObjectPtr pPhases, int i, PyObjectPtr pNewPhases )
{
	BW_GUARD;

	PyObjectPtr pyPhase;
	
	if (i >=0 && i < PySequence_Size( pPhases.get() ))
	{
		pyPhase = PyObjectPtr( PySequence_GetItem( pPhases.get(), i ), PyObjectPtr::STEAL_REFERENCE );
	}

	if ((beforePhase_ && beforePhase_->pyPhase().get() == pyPhase.get()) ||
		(!beforePhase_ && !pyPhase))
	{
		PyList_Append( pNewPhases.get(), pyNewPhase_.get() );
	}

	if (pyPhase)
	{
		PyList_Append( pNewPhases.get(), pyPhase.get() );
	}
}


//-----------------------------------------------------------------------------
//	Section: DeletePhaseEditor
//-----------------------------------------------------------------------------

/**
 *	Constructor.
 *
 *	@param deleteNode	Phase node to delete.
 */
DeletePhaseEditor::DeletePhaseEditor( PhaseNodePtr deleteNode ) :
	deleteNode_( deleteNode )
{
	BW_GUARD;
}


/**
 *	This method return whether or not this editor is OK and ready.
 *
 *	@return	Whether or not this editor is OK and ready.
 */
bool DeletePhaseEditor::isOK() const
{
	return true;
}


/**
 *	This method deletes the phase specified during construction.
 *
 *	@param pPhases	Original list of phases.
 *	@param i		Index of the phase currently being iterated.
 *	@param pNewPhases	New, modified list of phases.
 */
void DeletePhaseEditor::modify( PyObjectPtr pPhases, int i, PyObjectPtr pNewPhases )
{
	BW_GUARD;

	if (i >=0 && i < PySequence_Size( pPhases.get() ))
	{
		PyObjectPtr pyPhase( PySequence_GetItem( pPhases.get(), i ), PyObjectPtr::STEAL_REFERENCE );

		if (deleteNode_->pyPhase().get() != pyPhase.get())
		{
			PyList_Append( pNewPhases.get(), pyPhase.get() );
		}
	}
}

BW_END_NAMESPACE

