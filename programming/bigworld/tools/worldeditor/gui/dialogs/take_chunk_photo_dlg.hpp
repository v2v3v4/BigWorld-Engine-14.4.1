#ifndef TAKE_CHUNK_PHOTO_DLG_HPP
#define TAKE_CHUNK_PHOTO_DLG_HPP
#pragma once

#include "controls/edit_numeric.hpp"
#include "worldeditor/resource.h"
#include <afxwin.h>
#include "afxcmn.h"
// take_chunk_photo_dlg dialog
BW_BEGIN_NAMESPACE

class TakePhotoDlg : public CDialog
{
	DECLARE_DYNAMIC(TakePhotoDlg)

public:
	TakePhotoDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~TakePhotoDlg();

// Dialog Data
	enum { IDD = IDD_TAKEPHOTOS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

public:
	enum SpaceImageMode
	{
		LEVEL_MAP,
		SINGLE_MAP,
		NO_IMAGE
	};

	struct PhotoInfo
	{
		PhotoInfo():
			pixelsPerMetre(0),
			spaceMapHeight(0),
			spaceMapWidth(0),
			mode(NO_IMAGE)
		{

		};
		float pixelsPerMetre;
		int spaceMapHeight;
		int spaceMapWidth;
		TakePhotoDlg::SpaceImageMode mode;
	};

public:
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnBnClickedRadioCreateLevelMap();
	afx_msg void OnBnClickedRadioCreateSingleMap();

	virtual BOOL OnInitDialog();
	void setSpaceInfo( const BW::string& spaceName, float minPixelsperMetre, float maxPixelsperMetre, int blockSize, int gridCountX, int gridCountY );
	PhotoInfo getPhotoInfo();

private:
	void enableSpaceMapCtrls( bool enable );
	void calcMapSizeRange();
	void OnPhotoOption();

	controls::EditNumeric editorPixelsperMetre_;
	controls::EditNumeric editorMapWidth_;
	controls::EditNumeric editorMapHeight_;
	CSliderCtrl sliderPixelsperMetre_;
	CButton radioCreateLevelMap_;
	CButton radioCreateSingleMap_;
	CStatic lableDesc;

	float pixelsPerMetre_;
	float minPixelsperMetre_;
	float maxPixelsperMetre_;
	int blockSize_;
	int gridCountX_;
	int gridCountY_;
	float mapMinWidth_;
	float mapMaxWidth_;
	bool createSingleMap_;
	bool createLevelMap_;
	BW::string spaceName_;
	

public:
	afx_msg void OnEnChangeEditMapWidth();
	
};
BW_END_NAMESPACE

#endif // TAKE_CHUNK_PHOTO_DLG_HPP