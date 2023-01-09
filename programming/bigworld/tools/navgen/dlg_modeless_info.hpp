#pragma once

#include "resource.h"


BW_BEGIN_NAMESPACE

class ModelessInfoDialog
{
public:
	ModelessInfoDialog( HWND hwnd, const BW::wstring& title, const BW::wstring& msg, bool okButton = true );
	~ModelessInfoDialog();

// Dialog Data
	enum { IDD = IDD_MODELESS_INFO };
};

BW_END_NAMESPACE
