#pragma once
#include "resource.h"

#include "mutant.hpp"

BW_BEGIN_NAMESPACE

typedef std::pair < BW::string , BW::string > StringPair;

// TreeList

class TreeList: public CFormView
{
	DECLARE_DYNCREATE(TreeList)

public:
	//This is the default constructor required for DYNCREATE
	TreeList(): CFormView(IDD_EMPTY) {}
	
	TreeList( UINT dialogID, TreeRoot* tree_root_, const BW::string& what );
	virtual ~TreeList();

	static const wchar_t LOCKED_TAG[];

private:
	HTREEITEM selItem_ ;
	StringPair selID_;
	BW::string search_str_;

	TreeRoot* treeRoot_;
	BW::string what_;

protected:

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	void OnSize(UINT nType, int cx, int cy);

	DECLARE_MESSAGE_MAP()

	Mutant* mutant() const {	return MeApp::instance().mutant();	}

public:

	void OnUpdateTreeList();
	
	afx_msg void OnEnSetFocusSearch();
	afx_msg void OnEnChangeSearch();
	afx_msg void OnStnClickedCancelSearch();
	afx_msg void OnTvnSelChangedTree(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMClickSearchTree(NMHDR *pNMHDR, LRESULT *pResult);

	void selectItem( const StringPair& sp );
	virtual void selChange( const StringPair& itemID ) {}
	virtual void selClick( const StringPair& itemID ) {}

	const StringPair& selID() { return selID_; }
	void selID( const StringPair& id ) { selID_ = id; }
	HTREEITEM& selItem() { return selItem_; }
	CTreeCtrl& tree() { return tree_; }

	bool locked() { return locked_; }
	void locked( bool locked ) { locked_ = locked; OnUpdateTreeList(); }

	bool sameClicked() const { return sameClicked_; }
	void sameClicked( bool same ) { sameClicked_ = same; }

private:

	bool ignoreSelChange_;
	bool sameClicked_;

	CStatic search_bkg_;
	CEdit search_;
	CWnd search_button_;
	CWnd search_cancel_;
	CTreeCtrl tree_;

	BW::vector<BW::string*> pathData_;

protected:

	bool locked_;
};
BW_END_NAMESPACE

