#pragma once

BW_BEGIN_NAMESPACE

/**
 *	This class implements a genering single-string input dialog.
 */
class StringInputDlg : public CDialog
{
public:
	StringInputDlg( CWnd* pParent = NULL );   // standard constructor
	virtual ~StringInputDlg();

// Dialog Data
	enum { IDD = IDD_STRING_INPUT };

	void init( const BW::wstring & caption, const BW::wstring & label,
										int maxLen, const BW::string & str );

	const BW::string result() const { return bw_wtoutf8( (LPCTSTR)str_ ); }

protected:
	CString caption_;
	CString label_;
	CString str_;
	int maxLen_;

	virtual BOOL OnInitDialog();
	virtual void DoDataExchange( CDataExchange* pDX );    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
};
BW_END_NAMESPACE

