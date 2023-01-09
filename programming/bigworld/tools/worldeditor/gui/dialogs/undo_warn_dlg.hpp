#pragma once


#include "worldeditor/resource.h"

BW_BEGIN_NAMESPACE

/**
 *	This dialog warns the user that Process Data might clear the Undo/Redo
 *	stack.
 */
class UndoWarnDlg : public CDialog
{
	DECLARE_DYNAMIC( UndoWarnDlg )

public:
	enum { IDD = IDD_UNDOWARN };

	UndoWarnDlg( CWnd * pParent = NULL );
	virtual ~UndoWarnDlg();

	bool dontRepeat() const { return dontRepeat_; }

protected:
	virtual void DoDataExchange( CDataExchange * pDX );

	BOOL OnInitDialog();
	void OnOK();

	DECLARE_MESSAGE_MAP()

	bool dontRepeat_;
	CButton dontRepeatBtn_;
};
BW_END_NAMESPACE

