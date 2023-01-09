#ifndef EDIT_SPACE_DLG_HPP
#define EDIT_SPACE_DLG_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "worldeditor/resource.h"
#include "worldeditor/gui/controls/terrain_texture_lod_control.hpp"
#include "worldeditor/gui/dialogs/expand_space_dlg.hpp"
#include "controls/auto_tooltip.hpp"
#include "controls/image_control.hpp"
#include "controls/edit_numeric.hpp"

#include <afxwin.h>

BW_BEGIN_NAMESPACE

class SpaceSettings;

/**
 *
 * UI for editing space.settings files.
 *
 */
class EditSpaceDlg
	: public CDialog
	, public IValidationChangeHandler
	, public ITerrainTextureLodValueChangedListener
{
	DECLARE_DYNAMIC(EditSpaceDlg)

	DECLARE_AUTO_TOOLTIP( EditSpaceDlg, CDialog )

public:
	EditSpaceDlg(
		SpaceSettings & spaceSettings,
		CWnd* pParent = NULL);   // standard constructor
	virtual ~EditSpaceDlg();

	// Dialog Data
	enum { IDD = IDD_EDITSPACE };

	virtual void validationChange( bool validationResult );
	virtual void valueChanged();

private:
	void initIntEdit(
		controls::EditNumeric& edit, int minVal, int maxVal, int val );

	float currentChunkSize();
	void formatPerChunkToPerMetre( CString &str );
	void formatPerChunkToMetresPer( CString &str );
	void editSpace(
		const BW::wstring & spaceName,
		float chunkSize,
		uint32 heightMapEditorSize,
		uint32 heightMapSize, uint32 normalMapSize,
		uint32 shadowMapSize, uint32 holeMapSize,
		uint32 blendMapSize );
	void getSizeValues( float & chunkSize,
		uint32 & heightMapEditorSize,
		uint32 & heightMapSize, uint32 & normalMapSize,
		uint32 & shadowMapSize, uint32 & holeMapSize,
		uint32 & blendMapSize );
	void validateSizeValues();
	void updateValidation();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	controls::EditNumeric chunkSize_;
	CComboBox			  heightMapSize_;
	CComboBox			  heightMapEditorSize_;
	CComboBox			  normalMapSize_;
	CComboBox			  holeMapSize_;
	CComboBox			  shadowMapSize_;
	CComboBox			  blendMapSize_;
	CStatic heightMapRes_;
	CStatic heightMapEditorRes_;
	CStatic normalMapRes_;
	CStatic holeMapRes_;
	CStatic blendMapRes_;
	CStatic shadowMapRes_;
	bool busy_;
	int lastChunkSize_;

	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedOk();
	afx_msg void OnEnChangeChunkSize();
	afx_msg BOOL OnSetCursor( CWnd*, UINT, UINT );
	afx_msg void OnCbnSelChangeHeightmapSize();
	afx_msg void OnCbnSelChangeEditorHeightmapSize();
	afx_msg void OnCbnSelChangeNormalmapSize();
	afx_msg void OnCbnSelChangeHolemapSize();
	afx_msg void OnCbnSelChangeBlendmapSize();
	afx_msg void OnCbnSelChangeShadowmapSize();
	afx_msg void OnKickIdle();
	virtual void OnCancel();

	BW::string spacePath_;
	CButton buttonCancel_;
	CButton buttonApply_;

	CButton createLegacyTerrain_;

	BW::string space_;
	BW::string spaceName_;
	SpaceSettings * spaceSettings_;
	size_t filterChanged_;
	TerrainTextureLodControl terrainTextureLod_;
	ExpandSpaceControl expandSpaceDlg_;
	DWORD tickCount_;

	bool expandSpaceValid_;
	bool modifySizeValid_;
	bool modifyTextureLodValid_;
	bool initialized_;
};

BW_END_NAMESPACE

#endif // EDIT_SPACE_DLG_HPP
