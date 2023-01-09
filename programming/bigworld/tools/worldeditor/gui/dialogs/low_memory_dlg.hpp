#ifndef LOW_MEMORY_DLG_HPP
#define LOW_MEMORY_DLG_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "controls/show_cursor_helper.hpp"
#include <afxwin.h>

BW_BEGIN_NAMESPACE

class LowMemoryDlg : public CDialog
{
	DECLARE_DYNAMIC(LowMemoryDlg)

public:
	LowMemoryDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~LowMemoryDlg();

// Dialog Data
	enum { IDD = IDD_OUT_OF_MEMORY };

protected:
	ShowCursorHelper scopedShowCursor_;

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CButton mOK;
public:
	afx_msg void OnBnClickedContinue();
	afx_msg void OnBnClickedSave();
	afx_msg void OnBnClickedOk();
public:
	virtual void OnCancel();
};

BW_END_NAMESPACE

#endif // LOW_MEMORY_DLG_HPP
