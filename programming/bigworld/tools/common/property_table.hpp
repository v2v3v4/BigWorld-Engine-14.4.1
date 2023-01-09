#pragma once

#include "base_property_table.hpp"
#include "common/property_list.hpp"

BW_BEGIN_NAMESPACE

class BaseView;


class PropertyTable: public CFormView, public BasePropertyTable
{
public:
	
	PropertyTable( UINT dialogID );

	~PropertyTable();
	
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

	void stretchToRight( CWnd& widget, int pageWidth, int border );
	void OnSize(UINT nType, int cx, int cy);

	BOOL PreTranslateMessage(MSG* pMsg);
};
BW_END_NAMESPACE

