#pragma once

#include "../resource.h"
#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_namespace.hpp"

BW_BEGIN_NAMESPACE

// CModelessInfoDialog

class CLoadingDialog : public CDialog
{
public:
	CLoadingDialog( const BW::wstring& fileName );
	~CLoadingDialog();

	void setRange( int num );

	void step();

// Dialog Data
	enum { IDD = IDD_LOADING };

protected:
	void DoDataExchange( CDataExchange* pDX );
	BOOL OnInitDialog();

	 BW::wstring fileName_;

	 CProgressCtrl bar_;

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

BW_END_NAMESPACE

