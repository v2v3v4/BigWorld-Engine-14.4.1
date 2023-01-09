#include "pch.hpp"
#include "ccombobox_auto_complete.hpp"

BW_BEGIN_NAMESPACE
namespace controls
{

IMPLEMENT_DYNAMIC(CComboBoxAutoComplete, CComboBox)

BEGIN_MESSAGE_MAP(CComboBoxAutoComplete, CComboBox)
    ON_CONTROL_REFLECT(CBN_EDITUPDATE, OnCbnEditupdate)
END_MESSAGE_MAP()

CComboBoxAutoComplete::CComboBoxAutoComplete(): 
	CComboBox(),
	previousText_(""), 
	previousCurSel_(-1), 
	restrictToListBoxItems_(true)
{
}

CComboBoxAutoComplete::~CComboBoxAutoComplete()
{
}

void CComboBoxAutoComplete::restrictToListBoxItems(bool option) 
{ 
    restrictToListBoxItems_ = option; 
}

BOOL CComboBoxAutoComplete::PreTranslateMessage(MSG* pMsg)
{
	BW_GUARD;

    if (pMsg->message == WM_KEYDOWN)
    {
        int nVirtKey = (int)pMsg->wParam;
        if (nVirtKey == VK_RETURN)
        {
            // Set current selected if possible
            CString windowText;
            GetWindowText(windowText);
            
            if (windowText.IsEmpty())
            {
                SetCurSel(previousCurSel_);
            }
            else
            {
                const int index = FindStringExact( -1, windowText);
                if (index != -1)
                {
                    previousText_ = L"";
                    SetWindowText(L"");
                    SetCurSel(index);
                    previousCurSel_ = index;
                    GetParent()->SendMessage
                    (
                        WM_COMMAND, 
                        MAKEWPARAM(GetDlgCtrlID(), CBN_SELCHANGE), 
                        (LPARAM)m_hWnd
                    );
                }
            }
        }
    }
    return CComboBox::PreTranslateMessage(pMsg);
}

void CComboBoxAutoComplete::OnCbnEditupdate()
{
	BW_GUARD;

    if (previousCurSel_ == -1)
        previousCurSel_ = GetCurSel();

    CString windowText;
    GetWindowText(windowText);
    if (windowText.IsEmpty())
    {
        int index = GetCurSel();
        if (index != -1)
            previousCurSel_ = GetCurSel();

        previousText_ = "";
        return;
    }

    DWORD dwSel         = GetEditSel();
    int   wHi           = static_cast<int>(HIWORD(dwSel));
    int   wLo           = static_cast<int>(LOWORD(dwSel));
    bool  cursorCorrect = (wHi == wLo) && ((int)wHi == windowText.GetLength());
    int   notEqual      = previousText_.CompareNoCase(windowText);
    bool  stringGrowing = (previousText_.GetLength() <= windowText.GetLength());

    if (restrictToListBoxItems_)
    {
        int index = FindString(-1, windowText);
        if (index == -1)
        {
            // if a character is entered that is not found, disallow it
            windowText = previousText_;
            SetWindowText(windowText);
        }
    }

    if (cursorCorrect && notEqual && stringGrowing)
    {
        int index = FindString( -1, windowText);
        if (index >= 0)
        {
            // auto complete
            CString correspondingString;
            GetLBText(index,correspondingString);
            SetWindowText(correspondingString);
            SetEditSel(windowText.GetLength(), -1);
        }
    }
    else
    {
        // remove the auto complete
        SetWindowText(windowText);
        SetEditSel(windowText.GetLength(), -1);
    }

    previousText_ = windowText;
}

} // namespace controls
BW_END_NAMESPACE

