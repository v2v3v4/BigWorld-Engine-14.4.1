#pragma once
#include "controls/color_picker.hpp"
#include "color_picker_dialog.hpp"

BW_BEGIN_NAMESPACE
// ColorPickerDialog dialog

class ColorPickerDialog : public CDialog
{
	DECLARE_DYNAMIC(ColorPickerDialog)

public:
	ColorPickerDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~ColorPickerDialog();

	virtual BOOL OnInitDialog();

	Vector4 colorSelected() { return colorPicker_.getRGBA(); }
	void selectColor(const Vector4 & color) { colorPicker_.setRGBA(color); }

	// Dialog Data
	enum { IDD = IDD_COLOR_PICKER };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	
    controls::ColorPicker colorPicker_;

	DECLARE_MESSAGE_MAP()
public:
};



class ColorPickerDialogThread : public CWinThread
{
	DECLARE_DYNCREATE(ColorPickerDialogThread)
	ColorPickerDialogThread()
		: colorDialog_(NULL)
	{};
	
	virtual BOOL InitInstance();

	ColorPickerDialog * colorDialog_;

public:
	ColorPickerDialog * getDialog() { return colorDialog_; }
};

BW_END_NAMESPACE

