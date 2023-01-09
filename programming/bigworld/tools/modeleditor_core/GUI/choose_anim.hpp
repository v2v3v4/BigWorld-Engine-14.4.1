#ifndef CHOOSE_ANIM_HPP
#define CHOOSE_ANIM_HPP

#include "tree_list_dlg.hpp"

BW_BEGIN_NAMESPACE

typedef std::pair < BW::string , BW::string > StringPair;

class CChooseAnim : public TreeListDlg
{
public:
	CChooseAnim( int dialogID, bool withName, const BW::string& currentModel = "" );
	virtual BOOL OnInitDialog();

	virtual void selChange( const StringPair& animId );
	afx_msg void OnEnChangeActName();

	BW::string& actName() { return actName_; };
	BW::string& animName() { return animName_; }

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
private:
	CEdit name_;
	CButton ok_;

	BW::string actName_;
	BW::string animName_;

	bool withName_;

	bool defaultName_;
	bool defaultNameChange_;
	
public:
	afx_msg void OnNMDblclkSearchTree(NMHDR *pNMHDR, LRESULT *pResult);
};
BW_END_NAMESPACE

#endif // CHOOSE_ANIM_HPP
