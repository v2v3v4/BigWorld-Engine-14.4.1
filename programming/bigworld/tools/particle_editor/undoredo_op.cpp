#include "pch.hpp"
#include "undoredo_op.hpp"
#include "main_frame.hpp"

BW_BEGIN_NAMESPACE

UndoRedoOp::UndoRedoOp(ActionKind actionKind, DataSectionPtr pDS)
:
UndoRedo::Operation((int)actionKind),
data_(pDS)
{
}

UndoRedoOp::~UndoRedoOp()
{
    data_ = NULL;
}

void UndoRedoOp::undo()
{
	BW_GUARD;

    MainFrame::instance()->CopyFromDataSection
    ( 
        this->kind_, 
        this->data_ 
    );
    MainFrame::instance()->RefreshGUI(this->kind_);
}

bool UndoRedoOp::iseq(Operation const &oth) const
{
	BW_GUARD;

    const UndoRedoOp& oth2 =
        static_cast<const UndoRedoOp&>(oth);
    return XmlSectionsAreEq(data_, oth2.data());
}

DataSectionPtr UndoRedoOp::data() const 
{ 
    return data_; 
}

bool UndoRedoOp::XmlSectionsAreEq
( 
    DataSectionPtr const &one, 
    DataSectionPtr const &two 
) const
{
	BW_GUARD;

    const BinaryPtr s1 = one->asBinary();
    const BinaryPtr s2 = two->asBinary();

    if (s1->len() != s2->len())
        return false;

    if (strncmp( (char*)s1->data(), (char*)s2->data(), s1->len() ))
        return false;

    return true;
}

BW_END_NAMESPACE

