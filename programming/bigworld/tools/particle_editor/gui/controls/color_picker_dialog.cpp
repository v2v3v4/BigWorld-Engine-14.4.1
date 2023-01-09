#include "pch.hpp"
#include "main_frame.hpp"
#include "particle_editor.hpp"
#include "gui/controls/color_picker_dialog.hpp"
#include "gui/gui_utilities.hpp"
#include "resmgr/string_provider.hpp"

BW_BEGIN_NAMESPACE

IMPLEMENT_DYNAMIC(ColorPickerDialog, CDialog)

ColorPickerDialog::ColorPickerDialog(CWnd* pParent /*=NULL*/)
	: CDialog(ColorPickerDialog::IDD, pParent)
{
}

ColorPickerDialog::~ColorPickerDialog()
{
}

void ColorPickerDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BOOL ColorPickerDialog::OnInitDialog()
{
	BW_GUARD;

	CDialog::OnInitDialog();

	CWnd * pickerWnd = GetDlgItem(IDC_COLOR_PICKER_DIALOG_WND);
	CRect pickerRect;
	pickerWnd->GetWindowRect( &pickerRect );
	ScreenToClient(&pickerRect);

	colorPicker_.Create(WS_CHILD|WS_VISIBLE, pickerRect, this, false);

	return TRUE;  // return TRUE unless you set the focus to a control
}


BEGIN_MESSAGE_MAP(ColorPickerDialog, CDialog)
END_MESSAGE_MAP()


// ColorPickerDialog message handlers





///////////////////////////////////////////////////////////////////////////////


IMPLEMENT_DYNCREATE(ColorPickerDialogThread, CWinThread)
BOOL ColorPickerDialogThread::InitInstance() 
{
	BW_GUARD;

	WindowTextNotifier::instance();

	ColorPickerDialog dlg(m_pMainWnd);
	colorDialog_ = &dlg;
	m_pMainWnd = &dlg;

	// set initial color to be same as current background
	Moo::Colour currentColor = MainFrame::instance()->BgColour();
	Vector4 currentColorVector(currentColor.r, currentColor.g, currentColor.b, currentColor.a);
	colorDialog_->selectColor(currentColorVector);

	INT_PTR result = dlg.DoModal();	

	// tell mainframe the window is closing
	MainFrame::instance()->DereferenceColorDialogThread();

	// return false so windows does the cleanup
	return FALSE;
} 
BW_END_NAMESPACE

