#ifndef TRIGGER_LIST_HPP
#define TRIGGER_LIST_HPP

#include "resource.h"       // main symbols

#include "cstdmf/bw_set.hpp"
#include "resmgr/datasection.hpp"

BW_BEGIN_NAMESPACE

/*
 *	This class is used to manage the currently set action caps
 */
class CheckList: public CTreeCtrl
{
protected:
	DECLARE_MESSAGE_MAP()

private:
	BW::set< int > capsSet_;

public:
	void capsStr( const BW::string& caps );
	BW::string caps();
	void updateList();
	void redrawList();

};

/*
 * This class is used for managing the action cap window
 */
class CTriggerList : public CDialog
{
public:
	CTriggerList( const BW::string& capsName, BW::vector< DataSectionPtr >& capsList, const BW::string& capsString = "" );
	~CTriggerList();

	virtual BOOL OnInitDialog();

	BW::string caps();

// Dialog Data
	enum { IDD = IDD_ACT_TRIGGER };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
	
private:
	BW::vector< DataSectionPtr > capsList_;
	BW::vector< int* > capsData_;

	CheckList checkList_;
	BW::string capsName_;
	BW::string capsStr_;

public:
	afx_msg void OnPaint();
	void OnOK();
};
BW_END_NAMESPACE

#endif // TRIGGER_LIST_HPP
