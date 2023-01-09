#ifndef PLACEMENT_CTRLS_DLG_HPP
#define PLACEMENT_CTRLS_DLG_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "worldeditor/resource.h"
#include "worldeditor/gui/dialogs/new_placement_dlg.hpp"
#include "worldeditor/misc/placement_presets.hpp"
#include "controls/auto_tooltip.hpp"
#include "controls/range_slider_ctrl.hpp"
#include "controls/edit_numeric.hpp"
#include "controls/themed_image_button.hpp"
#include "guimanager/gui_functor_cpp.hpp"
#include "guitabs/guitabs_content.hpp"
#include <afxwin.h>

BW_BEGIN_NAMESPACE

class PlacementCtrlsDlg : public CDialog,
	public GUITABS::Content,
	public PlacementPresetsCallback,
	GUI::ActionMaker<PlacementCtrlsDlg>,
	GUI::ActionMaker<PlacementCtrlsDlg, 1>,
	GUI::UpdaterMaker<PlacementCtrlsDlg>,
	GUI::ActionMaker<PlacementCtrlsDlg, 2>
{
	IMPLEMENT_BASIC_CONTENT( Localise(L"WORLDEDITOR/GUI/PLACEMENT_CTRLS_DLG/SHORT_NAME"),
		Localise(L"WORLDEDITOR/GUI/PLACEMENT_CTRLS_DLG/LONG_NAME"), 290, 280, NULL )

public:
// Dialog Data
	enum { IDD = IDD_PLACEMENT_CTRLS };

	PlacementCtrlsDlg(CWnd* pParent = NULL);
	~PlacementCtrlsDlg();


protected:
	DECLARE_AUTO_TOOLTIP(PlacementCtrlsDlg, CDialog);

	enum SliderContainerGroup {
		GROUP_ROTATION,
		GROUP_SCALE
	};
	class SliderContainer : public ReferenceCount
	{
	public:
        SliderContainer
        ( 
            controls::RangeSliderCtrl       *slider, 
            controls::EditNumeric           *minEd, 
            controls::EditNumeric           *maxEd, 
            controls::ThemedImageButton     *reset, 
            int                             group 
        );

        controls::RangeSliderCtrl* slider_;
		controls::EditNumeric* minEd_;
		controls::EditNumeric* maxEd_;
		controls::ThemedImageButton* reset_;
		int group_;
	};
	typedef SmartPointer<SliderContainer> SliderContainerPtr;

	CStatic presetsLabel_;
	CStatic randRotGroup_;
	CStatic randScaGroup_;
	controls::ThemedImageButton linkRandRot_;
	CStatic linkRotUpIcon_;
	CStatic linkRotDownIcon_;
	controls::ThemedImageButton linkRandSca_;
	CStatic linkScaUpIcon_;
	CStatic linkScaDownIcon_;
    controls::RangeSliderCtrl sliderRandRotX_;
	controls::RangeSliderCtrl sliderRandRotY_;
	controls::RangeSliderCtrl sliderRandRotZ_;
	controls::RangeSliderCtrl sliderRandScaX_;
	controls::RangeSliderCtrl sliderRandScaY_;
	controls::RangeSliderCtrl sliderRandScaZ_;
	controls::EditNumeric minRandRotX_;
	controls::EditNumeric maxRandRotX_;
	controls::EditNumeric minRandRotY_;
	controls::EditNumeric maxRandRotY_;
	controls::EditNumeric minRandRotZ_;
	controls::EditNumeric maxRandRotZ_;
	controls::EditNumeric minRandScaX_;
	controls::EditNumeric maxRandScaX_;
	controls::EditNumeric minRandScaY_;
	controls::EditNumeric maxRandScaY_;
	controls::EditNumeric minRandScaZ_;
	controls::EditNumeric maxRandScaZ_;
	controls::ThemedImageButton resetRotX_;
	controls::ThemedImageButton resetRotY_;
	controls::ThemedImageButton resetRotZ_;
	controls::ThemedImageButton resetScaX_;
	controls::ThemedImageButton resetScaY_;
	controls::ThemedImageButton resetScaZ_;
	CButton uniformSca_;

	CToolBarCtrl toolbar_;
	
	NewPlacementDlg newPlacementDlg_;
	
	bool updateEdits_;
	DataSectionPtr section_;
	DataSectionPtr presetsSection_;
	BW::string placementXML_;

	BW::vector<SliderContainerPtr> sliders_;

	int rightEditDist_;
	int rightResetButDist_;

	HICON linkIcon_;
	HICON resetIcon_;

	SliderContainerPtr find( void* ptr );
	BOOL OnInitDialog();
	void DoDataExchange(CDataExchange* pDX);

	void setDefaults( SliderContainerPtr slider );
    void writeEditNum( controls::EditNumeric* edit, float num, int group );
	void sliderFromEdits( SliderContainerPtr slider );
	void syncBars( SliderContainerPtr slider );
	void updateVisibility();
	void updateLinks();

	void loadPreset();
	void savePreset();

	bool newPreset( GUI::ItemPtr item );
	bool delPreset( GUI::ItemPtr item );
	unsigned int presetEditableUpdater( GUI::ItemPtr item );
	bool renamePreset( GUI::ItemPtr item );

	void currentPresetChanged( const BW::string& presetName );

	afx_msg void OnGUIManagerCommand(UINT nID);
	afx_msg LRESULT OnRangeSliderChanged(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnRangeSliderTrack(WPARAM wParam, LPARAM lParam);
	afx_msg void OnBnClickedRandrotLink();
	afx_msg void OnBnClickedRandscaLink();
	afx_msg void OnEnChangeRandRotX();
	afx_msg void OnEnChangeRandRotY();
	afx_msg void OnEnChangeRandRotZ();
	afx_msg void OnEnChangeRandScaX();
	afx_msg void OnEnChangeRandScaY();
	afx_msg void OnEnChangeRandScaZ();
	afx_msg void OnBnClickedRandRotXReset();
	afx_msg void OnBnClickedRandRotYReset();
	afx_msg void OnBnClickedRandRotZReset();
	afx_msg void OnBnClickedRandScaXReset();
	afx_msg void OnBnClickedRandScaYReset();
	afx_msg void OnBnClickedRandScaZReset();
	afx_msg void OnBnClickedRandscaUni();
	afx_msg void OnSize( UINT nType, int cx, int cy );
	afx_msg HBRUSH OnCtlColor( CDC* pDC, CWnd* pWnd, UINT nCtlColor );
	DECLARE_MESSAGE_MAP()
};

IMPLEMENT_CDIALOG_CONTENT_FACTORY( PlacementCtrlsDlg, PlacementCtrlsDlg::IDD )

BW_END_NAMESPACE

#endif // PLACEMENT_CTRLS_DLG_HPP
