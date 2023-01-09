#pragma once

#include "resource.h"
#include "base_property_table.hpp"
#include "common/property_list.hpp"

BW_BEGIN_NAMESPACE

class BaseView;


class CDialogPropertyTable: public CDialog, public BasePropertyTable
{
public:
	
	CDialogPropertyTable( UINT dialogID );
	~CDialogPropertyTable();
	
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

	void stretchToRight( CWnd& widget, int pageWidth, int border );
	void OnSize(UINT nType, int cx, int cy);

	BOOL PreTranslateMessage(MSG* pMsg);
};
BW_END_NAMESPACE

