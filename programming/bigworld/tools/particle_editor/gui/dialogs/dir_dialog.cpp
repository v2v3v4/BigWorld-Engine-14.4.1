#error File no longer in use.  Allan 04/03/03


#include "stdafx.h"
#include "dir_dialog.hpp"
#include "shlobj.h"


// Callback function called by SHBrowseForFolder's browse control after initialisation and when selection changes
int __stdcall DirDialog::browseCtrlCallback(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
	DirDialog* pDirDialogObj = (DirDialog*)lpData;
	if (uMsg == BFFM_INITIALIZED )
	{
		if( ! pDirDialogObj->startDirectory_.IsEmpty() )
			SendMessage(hwnd, BFFM_SETSELECTION, TRUE, (LPARAM)(LPCTSTR)(pDirDialogObj->startDirectory_));
		if( ! pDirDialogObj->windowTitle_.IsEmpty() )
			SetWindowText(hwnd, (LPCTSTR) pDirDialogObj->windowTitle_);
	}
	else if( uMsg == BFFM_SELCHANGED )
	{
		LPITEMIDLIST pidl = (LPITEMIDLIST) lParam;
		char selection[MAX_PATH];
		if( ! ::SHGetPathFromIDList(pidl, selection) )
			selection[0] = '\0';

		SendMessage(hwnd, BFFM_ENABLEOK, 0, TRUE);
	}
	
	return 0;
}


DirDialog::DirDialog()
{
}

DirDialog::~DirDialog()
{
}

BOOL DirDialog::doBrowse(CWnd *pwndParent)
{
	if( ! startDirectory_.IsEmpty() )
	{
		// correct the format
		startDirectory_.Replace('/', '\\');

		startDirectory_.TrimRight();
		if (startDirectory_.Right(1) == "\\")
			startDirectory_ = startDirectory_.Left(startDirectory_.GetLength() - 1);
	}

	LPMALLOC pMalloc;
	if (SHGetMalloc(&pMalloc) != NOERROR)
		return FALSE;

	BROWSEINFO bInfo;
	LPITEMIDLIST pidl;
	ZeroMemory((PVOID)(&bInfo), sizeof(BROWSEINFO));

	if (!fakeRootDirectory_.IsEmpty())
	{
		OLECHAR olePath[MAX_PATH];
		HRESULT hr;
		LPSHELLFOLDER pDesktopFolder;

		// the desktop's IShellFolder interface.
		if (SUCCEEDED(SHGetDesktopFolder(&pDesktopFolder)))
		{
			// correct the format
			fakeRootDirectory_.Replace('/', '\\');

			// need unicode
			MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, fakeRootDirectory_.GetBuffer(MAX_PATH), -1, olePath, MAX_PATH);
            
			// Convert the path to an ITEMIDLIST.
			hr = pDesktopFolder->ParseDisplayName(NULL, NULL, olePath, NULL, &pidl, NULL);

			if (FAILED(hr))
			{
				pMalloc->Free(pidl);
				pMalloc->Release();
				return FALSE;
			}
			bInfo.pidlRoot = pidl;
		}
	}
    
	bInfo.hwndOwner = pwndParent == NULL ? NULL : pwndParent->GetSafeHwnd();
	bInfo.pszDisplayName = userSelectedDirectory_.GetBuffer(MAX_PATH);
	bInfo.lpszTitle = (promptText_.IsEmpty()) ? "Open" : promptText_;
	bInfo.ulFlags = BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
	bInfo.lpfn = browseCtrlCallback;
	bInfo.lParam = (LPARAM)this;

	// open the dialog!
	if ((pidl = SHBrowseForFolder(&bInfo)) == NULL)
	{
		return FALSE;
	}
    
	// get the selected directory
	userSelectedDirectory_.ReleaseBuffer();
	if (SHGetPathFromIDList(pidl, userSelectedDirectory_.GetBuffer(MAX_PATH)) == FALSE)
	{
		pMalloc->Free(pidl);
		pMalloc->Release();
		return FALSE;
	}

	userSelectedDirectory_.ReleaseBuffer();
	userSelectedDirectory_.Replace('\\', '/');

	pMalloc->Free(pidl);
	pMalloc->Release();
	return TRUE;
}
