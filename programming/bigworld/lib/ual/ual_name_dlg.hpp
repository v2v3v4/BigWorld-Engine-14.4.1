#pragma once

#include "cstdmf/bw_namespace.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class implements a simple dialog used to change the name of an
 *	Asset Browser dialog.
 */
class UalNameDlg : public CDialog
{
public:
	UalNameDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~UalNameDlg();

// Dialog Data
	enum { IDD = IDD_UALNAME };

	void getNames( BW::wstring& shortName, BW::wstring& longName );
	void setNames( const BW::wstring& shortName, const BW::wstring& longName );

protected:
	CString longName_;
	CString shortName_;

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	void OnOK();

	DECLARE_MESSAGE_MAP()
};

BW_END_NAMESPACE

