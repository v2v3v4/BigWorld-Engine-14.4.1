#ifndef RECREATE_SPACE_DLG_HPP
#define RECREATE_SPACE_DLG_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "worldeditor/resource.h"
#include "controls/auto_tooltip.hpp"
#include "controls/edit_numeric.hpp"
#include <afxwin.h>

BW_BEGIN_NAMESPACE

class SpaceSettings;

class RecreateSpaceDlg : public CDialog
{
	DECLARE_DYNAMIC(RecreateSpaceDlg)

	DECLARE_AUTO_TOOLTIP( RecreateSpaceDlg, CDialog )

public:
	RecreateSpaceDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~RecreateSpaceDlg();

// Dialog Data
	enum { IDD = IDD_RECREATESPACE };

	void setSpaceSettings( SpaceSettings * spaceSettings );

private:
	CString createdSpace_;

	void initIntEdit(
		controls::EditNumeric& edit, int minVal, int maxVal, int val );

	void validateFile( CEdit& ctrl, bool isPath );

	float currentChunkSize();
	// These all read str as an integer number, and output back into str as a float
	void formatChunksToKms( CString& str );
	void formatPerChunkToPerMetre( CString &str );
	void formatPerChunkToMetresPer( CString &str );

	void refreshHeightmapRes();
	void refreshEditorHeightmapRes();
	void refreshHolemapRes();
	void refreshShadowmapRes();
	void refreshBlendmapRes();
	void refreshWarnings();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	/* Inputs */
	CEdit space_;
	CEdit spacePath_;
	controls::EditNumeric chunkSize_;
	controls::EditNumeric width_;
	controls::EditNumeric height_;
	CComboBox			  heightMapSize_;
	CComboBox			  heightMapEditorSize_;
	CComboBox			  normalMapSize_;
	CComboBox			  holeMapSize_;
	CComboBox			  shadowMapSize_;
	CComboBox			  blendMapSize_;
	CButton buttonCreate_;
	CButton buttonCancel_;

	/* Static but updated values */
	CStatic widthKms_;
	CStatic heightKms_;
	CStatic heightMapRes_;
	CStatic heightMapEditorRes_;
	CStatic normalMapRes_;
	CStatic holeMapRes_;
	CStatic shadowMapRes_;
	CStatic blendMapRes_;
	CListBox warnings_;

	CString defaultSpace_;
	BW::string defaultSpacePath_;
	SpaceSettings * spaceSettings_;

	CString createdSpace() { return createdSpace_; }

	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedOk();
	afx_msg void OnEnChangeSpace();
	afx_msg void OnEnChangeSpacePath();
	afx_msg void OnBnClickedRecreatespaceBrowse();
	afx_msg void OnEnChangeChunkSize();
	afx_msg void OnEnKillFocusChunkSize();
	afx_msg void OnEnChangeWidth();
	afx_msg void OnEnChangeHeight();
	afx_msg void OnCbnSelChangeHeightmapSize();
	afx_msg void OnCbnSelChangeEditorHeightmapSize();
	afx_msg void OnCbnSelChangeNormalmapSize();
	afx_msg void OnCbnSelChangeHolemapSize();
	afx_msg void OnCbnSelChangeBlendmapSize();
	afx_msg void OnCbnSelchangeShadowmapSize();
};

BW_END_NAMESPACE

#endif // RECREATE_SPACE_DLG_HPP
