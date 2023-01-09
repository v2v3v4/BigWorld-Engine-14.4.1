#ifndef POST_PROC_UNDO_HPP
#define POST_PROC_UNDO_HPP

#include "gizmo/undoredo.hpp"
#include "post_processing/effect.hpp"

BW_BEGIN_NAMESPACE

/**
 *	Interface with the outside world
 */
class PostProcUndo
{
public:
	static bool undoRedoDone();
	static void undoRedoDone( bool val );
};


/**
 *	Chain-level undo-redo
 */
class ChainUndoOp : public UndoRedo::Operation
{
public:
	ChainUndoOp();

	virtual void undo();

	virtual bool iseq( const UndoRedo::Operation & oth ) const;

private:
	BW::vector< PyObjectPtr > savedChain_;
};


/**
 *	Effect-level phases list undo-redo
 */
class PhasesUndoOp : public UndoRedo::Operation
{
public:
	PhasesUndoOp( PostProcessing::EffectPtr effect );

	virtual void undo();

	virtual bool iseq( const UndoRedo::Operation & oth ) const;

private:
	PostProcessing::EffectPtr effect_;
	BW::vector< PyObjectPtr > savedPhases_;
};

BW_END_NAMESPACE

#endif POST_PROC_UNDO_HPP
