#ifndef UNDOREDO_OP_HPP
#define UNDOREDO_OP_HPP

#include "fwd.hpp"
#include "gizmo/undoredo.hpp"
 
BW_BEGIN_NAMESPACE

class UndoRedoOp : public UndoRedo::Operation
{
public:
    enum ActionKind
    {
        AK_PARAMETER,
        AK_NPARAMETER
    };

    //
    // Constructor.
    //
    // @param actionKind    The kind of undo/redo action.
    // @param data          The saved state.
    //
	UndoRedoOp
    ( 
        ActionKind          actionKind, 
        DataSectionPtr      data
    );

    //
    // Destructor.
    //
	~UndoRedoOp();

    //
    // Undo operation.
    //
	/*virtual*/ void undo();

    //
    // Are two undo operations the same?
    //
    // @param other         The other undo operation.
    // @returns             True if the undo operations are the same.
    //
	/*virtual*/ bool iseq(Operation const &other) const;

    //
    // Get the saved state.
    //
    // @returns             The saved state.
    //
	DataSectionPtr data() const;

protected:
    //
    // Are the data sections equal?
    //
    // @param one           DataSection one.
    // @param two           DataSection two.
    // @returns             True if data section as binary data are bit for bit
    //                      the same.
    //
	bool XmlSectionsAreEq
    (
        DataSectionPtr      const &one, 
        DataSectionPtr      const &two 
    ) const;

private:
	DataSectionPtr	        data_;
};

BW_END_NAMESPACE

#endif // UNDOREDO_OP_HPP
