#ifndef POST_PROCESSING_CHAINS_HPP
#define POST_PROCESSING_CHAINS_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "worldeditor/resource.h"
#include "controls/auto_tooltip.hpp"
#include <afxwin.h>
#include <afxcmn.h>

BW_BEGIN_NAMESPACE

/**
 *	This class implements the chain list subpanel of the post processing panel.
 */
class PostProcessingChains : public CDialog
{
public:
	enum { IDD = IDD_POST_PROC_CHAINS };

	PostProcessingChains();
	virtual ~PostProcessingChains();

	void setEventHandler( CWnd * handler ) { handler_ = handler; }

	void update();

	void chainChanged();

	void onExit();

protected:
	DECLARE_AUTO_TOOLTIP_EX( PostProcessingChains );

	virtual void DoDataExchange( CDataExchange * pDX );    // DDX/DDV support

	virtual BOOL OnInitDialog();

	virtual BOOL PreTranslateMessage( MSG * pMsg );

	void OnOK() {}
	void OnCancel() {}

	afx_msg void OnSize( UINT nType, int cx, int cy );
	afx_msg void OnChangeSelectedChain( NMHDR * pNotifyStruct, LRESULT * result );
	afx_msg void OnBtnNewChain();
	afx_msg void OnBtnRenameChain();
	afx_msg void OnBtnDuplicateChain();
	afx_msg void OnBtnDeleteChain();
	afx_msg void OnBtnSaveChanges();
	afx_msg void OnBtnDiscardChanges();
	afx_msg void OnCustomDrawList( NMHDR * pNMHDR, LRESULT * pResult );
	afx_msg void OnListBeginEditName( NMHDR * pNMHDR, LRESULT * pResult );
	afx_msg void OnListEndEditName( NMHDR * pNMHDR, LRESULT * pResult );
	afx_msg void OnListKeyDown( NMHDR * pNMHDR, LRESULT * pResult );
	DECLARE_MESSAGE_MAP()

private:
	CWnd * handler_;
	PyObject * pScriptObject_;
	CTreeCtrl list_;
	BW::string chainsFolder_;
	bool listNeedsUpdate_;
	bool ignoreSelection_;

	HTREEITEM defaultItem_;
	BW::map< BW::string, HTREEITEM > listMap_;

	void getPythonChain( BW::string & ret );
	void getListChain( BW::string & ret );

	void changeSelectedChain();

	void fillChainsList();
	void select( const BW::string & str );
};

BW_END_NAMESPACE

#endif // POST_PROCESSING_CHAINS_HPP
