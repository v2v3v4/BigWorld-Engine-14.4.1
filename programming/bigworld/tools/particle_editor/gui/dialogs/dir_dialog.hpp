#error File no longer in use.  Allan 04/03/03

#pragma once


class DirDialog
{
public:
	DirDialog();
	virtual ~DirDialog();

	
	CString windowTitle_;				// window title text
	CString promptText_;				// inner window text

	CString fakeRootDirectory_;			// restrict choice to directories below
	CString startDirectory_;			// start off in directory


	BOOL doBrowse(CWnd *pwndParent = NULL);		// spawn the window


	CString userSelectedDirectory_;		// directory returned


private:
	static int __stdcall DirDialog::browseCtrlCallback(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData);
};

