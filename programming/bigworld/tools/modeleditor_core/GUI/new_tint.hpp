#ifndef NEW_TINT_HPP
#define NEW_TINT_HPP

#include "resource.h"       // main symbols

BW_BEGIN_NAMESPACE

class CNewTint : public CDialog
{
public:
	CNewTint( BW::vector< BW::string >& otherTintNames );
	virtual BOOL OnInitDialog();

	const BW::string& tintName() { return tintName_; };
	const BW::string& fxFile() { return fxFile_; }
	const BW::string& mfmFile() { return mfmFile_; }

// Dialog Data
	enum { IDD = IDD_NEW_TINT };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
private:
	BW::vector< BW::string >& tintNames_;
	
	CEdit name_;

	CButton fxCheck_;
	CButton mfmCheck_;

	CComboBox fxList_;
	CComboBox mfmList_;

	CButton fxSel_;
	CButton mfmSel_;

	BW::string tintName_;
	BW::string fxFile_;
	BW::string mfmFile_;

	CButton ok_;

	void checkComplete();
	void redrawList( CComboBox& list, const BW::string& name );

	afx_msg void OnEnChangeNewTintName();
	afx_msg void OnBnClickedNewTintMfmCheck();
	afx_msg void OnBnClickedNewTintFxCheck();
	afx_msg void OnCbnSelchangeNewTintFxList();
	afx_msg void OnCbnSelchangeNewTintMfmList();
	afx_msg void OnBnClickedNewTintFxSel();
	afx_msg void OnBnClickedNewTintMfmSel();

	void OnOK();
};
BW_END_NAMESPACE

#endif // NEW_TINT_HPP
