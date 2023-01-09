#pragma once
#include "../resource.h"

#include "mutant.hpp"

BW_BEGIN_NAMESPACE

typedef std::pair < BW::string , BW::string > StringPair;

// TreeListDlg

class TreeListDlg: public CDialog
{
	DECLARE_DYNCREATE(TreeListDlg)

public:
	//This is the default constructor required for DYNCREATE
	TreeListDlg(): CDialog(IDD_EMPTY) {}
	
	TreeListDlg( UINT dialogID, TreeRoot* tree_root_, const BW::string& what, const BW::string& currentModel = "" );
	virtual ~TreeListDlg();

private:
	HTREEITEM selItem_ ;
	StringPair selID_;
	BW::string search_str_;

	TreeRoot* treeRoot_;
	BW::string what_;
	BW::string currentModel_;

protected:

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

public:

	void OnUpdateTreeList();
	
	afx_msg void OnEnSetFocusSearch();
	afx_msg void OnEnChangeSearch();
	afx_msg void OnStnClickedCancelSearch();
	afx_msg void OnTvnSelChangedTree(NMHDR *pNMHDR, LRESULT *pResult);

	virtual void selChange( const StringPair& itemID ) {};

	StringPair& selID() { return selID_; };
	HTREEITEM& selItem() { return selItem_; };
	CTreeCtrl& tree() { return tree_; };

private:

	bool ignoreSelChange_;

	CStatic search_bkg_;
	CEdit search_;
	CWnd search_button_;
	CWnd search_cancel_;
	CTreeCtrl tree_;

	BW::vector<BW::string*> pathData_;
};
BW_END_NAMESPACE

