#ifndef NEW_SPACE_DLG_HPP
#define NEW_SPACE_DLG_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "worldeditor/resource.h"
#include "controls/auto_tooltip.hpp"
#include "controls/image_control.hpp"
#include "controls/edit_numeric.hpp"
#include <afxwin.h>

BW_BEGIN_NAMESPACE

class NewSpaceDlg : public CDialog
{
	DECLARE_DYNAMIC(NewSpaceDlg)

	DECLARE_AUTO_TOOLTIP( NewSpaceDlg, CDialog )

public:
	NewSpaceDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~NewSpaceDlg();

// Dialog Data
	enum { IDD = IDD_NEWSPACE };

private:
	CString createdSpace_;

	void initIntEdit(
		controls::EditNumeric& edit, int minVal, int maxVal, int val );

	void validateFile( CEdit& ctrl, bool isPath );
	bool validateDefaultTexture();
	float currentChunkSize();
	void formatChunkToKms( CString& str, const CString& chunkSizeStr );
	void formatPerChunkToPerMetre( CString &str );
	void formatPerChunkToMetresPer( CString &str );

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CEdit space_;
	controls::EditNumeric chunkSize_;
	controls::EditNumeric width_;
	controls::EditNumeric height_;
	controls::EditNumeric floraVB_;
	controls::EditNumeric floraTextureWidth_;
	controls::EditNumeric floraTextureHeight_;
	CComboBox			  heightMapSize_;
	CComboBox			  heightMapEditorSize_;
	CComboBox			  normalMapSize_;
	CComboBox			  holeMapSize_;
	CComboBox			  shadowMapSize_;
	CComboBox			  blendMapSize_;
	CStatic heightMapRes_;
	CStatic normalMapRes_;
	CStatic holeMapRes_;
	CStatic blendMapRes_;
	CStatic shadowMapRes_;
	CEdit defaultTexture_;
	controls::ImageControl32 textureImage_;
	CProgressCtrl progress_;
	CString defaultSpace_;
	BW::string defaultSpacePath_;
	bool busy_;
	CStatic widthKms_;
	CStatic heightKms_;
	int lastChunkSize_;

	CString createdSpace() { return createdSpace_; }

	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedOk();
	afx_msg void OnEnChangeSpace();
	afx_msg void OnEnChangeSpacePath();
	afx_msg void OnBnClickedNewspaceBrowse();
	afx_msg void OnEnTextureChange();
	afx_msg void OnBnTextureBrowse();
	afx_msg void OnEnChangeChunkSize();
	afx_msg void OnEnChangeWidth();
	afx_msg void OnEnChangeHeight();
	afx_msg BOOL OnSetCursor( CWnd*, UINT, UINT );
	afx_msg void OnCbnSelchangeHeightmapEditorSize();
	afx_msg void OnCbnSelChangeHeightmapSize();
	afx_msg void OnCbnSelChangeNormalmapSize();
	afx_msg void OnCbnSelChangeHolemapSize();
	afx_msg void OnCbnSelChangeBlendmapSize();
	afx_msg void OnCbnSelChangeShadowmapSize();

	CEdit spacePath_;
	CButton buttonCancel_;
	CButton buttonCreate_;
};

BW_END_NAMESPACE

#endif // NEW_SPACE_DLG_HPP
