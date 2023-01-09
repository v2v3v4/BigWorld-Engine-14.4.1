#ifndef ABOUTBOX_HPP
#define ABOUTBOX_HPP

#pragma warning ( disable : 4530 )

#include <iostream>

#include "mfxexp.hpp"

BW_BEGIN_NAMESPACE

class AboutBox
{
public:
	AboutBox();
	~AboutBox();
	
	void display( HWND hWnd );

private:
	static INT_PTR CALLBACK dialogProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	AboutBox(const AboutBox&);
	AboutBox& operator=(const AboutBox&);

	friend std::ostream& operator<<(std::ostream&, const AboutBox&);
};

BW_END_NAMESPACE

#endif
/*aboutbox.hpp*/