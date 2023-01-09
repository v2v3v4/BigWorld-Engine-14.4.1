#include "pch.hpp"
#include "embeddable_cdialog.hpp"

BW_BEGIN_NAMESPACE
namespace controls
{

/**
 * Constructor
 * @param nIdTemplate the resource ID to look up the template control
 * @param pParentWnd the parent CWnd
 */
EmbeddableCDialog::EmbeddableCDialog(
	UINT nIDTemplate, CWnd* pParentWnd /*= NULL */)
	: CDialog( nIDTemplate, pParentWnd )
	, templateId_( nIDTemplate )
{
}


EmbeddableCDialog::~EmbeddableCDialog()
{
	DestroyWindow();
}

/**
 * Passes control of specified dialog item to this control
 * @param nId resource ID for dummy control to replace. 
 * @param pParent the parent control
 */
void EmbeddableCDialog::SubclassDlgItem( UINT nID, CWnd * pParent )
{
	RECT rect;
	CWnd * origDlgItem = pParent->GetDlgItem( nID );
	origDlgItem->ShowWindow( SW_HIDE );
	origDlgItem->GetWindowRect( &rect );
	pParent->ScreenToClient( &rect );
	BOOL created = Create( templateId_, pParent ); 
	MF_ASSERT( created );
	SetWindowPos(
		NULL, rect.left, rect.top,
		abs( rect.left - rect.right ), abs( rect.top - rect.bottom ),
		SWP_NOREPOSITION );
	SetDlgCtrlID( nID );
	ShowWindow( SW_SHOWNORMAL );
	UpdateData( FALSE );
}


/*virtual */void EmbeddableCDialog::OnOK()
{
	return;
}


/*virtual */void EmbeddableCDialog::OnCancel()
{
	return;
}

} // namespace controls
BW_END_NAMESPACE

