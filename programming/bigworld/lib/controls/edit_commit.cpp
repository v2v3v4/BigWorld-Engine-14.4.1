#include "pch.hpp"
#include "edit_commit.hpp"

BW_BEGIN_NAMESPACE
namespace controls
{

EditCommit::EditCommit():
	autoSelect_ (false),
	commitOnFocusLoss_(true),
	dirty_ (false)
{
}

EditCommit::~EditCommit()
{
}

BEGIN_MESSAGE_MAP(EditCommit, CEdit)
	ON_WM_CHAR()
	ON_CONTROL_REFLECT_EX(EN_SETFOCUS, OnSetfocus)
	ON_CONTROL_REFLECT_EX(EN_KILLFOCUS, OnKillfocus)
END_MESSAGE_MAP()

void EditCommit::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	BW_GUARD;

	if (nChar == 13) // Catch enter press...
	{
		doCommit();	// Call the commit function
		return;     // Ignore the enter
	}
	CEdit::OnChar(nChar, nRepCnt, nFlags);
}

BOOL EditCommit::OnSetfocus()
{
	BW_GUARD;

	if (autoSelect_)
		SetSel(0,-1);

	return FALSE;
}

BOOL EditCommit::OnKillfocus()
{
	BW_GUARD;

	if (commitOnFocusLoss_)
	{
		doCommit();
	}

	return FALSE;
}

} // namespace controls
BW_END_NAMESPACE

