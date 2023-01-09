#ifndef PAGE_OPTIONS_ENVIRONMENT_HPP
#define PAGE_OPTIONS_ENVIRONMENT_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "worldeditor/resource.h"
#include "worldeditor/gui/controls/limit_slider.hpp"
#include "worldeditor/gui/controls/terrain_texture_lod_control.hpp"
#include "guitabs/guitabs_content.hpp"
#include "controls/color_picker.hpp"
#include "controls/color_timeline.hpp"
#include "controls/edit_numeric.hpp"
#include "controls/auto_tooltip.hpp"
#include "controls/dialog_toolbar.hpp"
#include "controls/image_button.hpp"

BW_BEGIN_NAMESPACE

class SkyDomeListBox : public CListBox
{
	public:
		virtual void DrawItem( LPDRAWITEMSTRUCT lpDrawItemStruct );
		virtual void MeasureItem(LPMEASUREITEMSTRUCT);
};


class PageOptionsEnvironment : public CFormView, public GUITABS::Content
{
    IMPLEMENT_BASIC_CONTENT
    (
        Localise(L"WORLDEDITOR/GUI/PAGE_OPTIONS_ENVIRONMENT/SHORT_NAME"),              // short name of page
        Localise(L"WORLDEDITOR/GUI/PAGE_OPTIONS_ENVIRONMENT/LONG_NAME"),      // long name of page
        290,                        // default width
        500,                        // default height
        NULL                        // icon
    )

	static bool wasSlider( CWnd const & testScrollBar, CWnd const & scrollBar );
public:
    enum { IDD = IDD_PAGE_OPTIONS_ENVIRONMENT };

    PageOptionsEnvironment();

    /*virtual*/ ~PageOptionsEnvironment();

    static PageOptionsEnvironment *instance();
    void reinitialise();
    float getSelTime() const;
    void setSelTime(float time);
    static EnviroMinder &getEnviroMinder();
    static TimeOfDay &getTimeOfDay();

protected:
    enum SliderMovementState
    {
        SMS_STARTED,
        SMS_MIDDLE,
        SMS_DONE
    };

    void InitPage();
	/*virtual*/ void DoDataExchange(CDataExchange *dx);

    afx_msg LRESULT OnUpdateControls(WPARAM /*wParam*/, LPARAM /*lParam*/);
    afx_msg LRESULT OnNewSpace(WPARAM /*wParam*/, LPARAM /*lParam*/);
    afx_msg LRESULT OnBeginSave(WPARAM /*wParam*/, LPARAM /*lParam*/);
    afx_msg LRESULT OnEndSave(WPARAM /*wParam*/, LPARAM /*lParam*/);
    afx_msg void OnBrowseSkyFile();
    afx_msg void OnCopySkyFile();
    afx_msg void OnBrowseTODFile();
    afx_msg void OnCopyTODFile();
    afx_msg void OnAddSkyDome();
    afx_msg void OnClearSkyDomes();
    afx_msg void OnBrowseSkyGradBtn();
    afx_msg void OnSkyboxUpEnable  (CCmdUI *cmdui);
    afx_msg void OnSkyboxDownEnable(CCmdUI *cmdui);
    afx_msg void OnSkyboxDelEnable (CCmdUI *cmdui);
    afx_msg void OnSkyboxUp();
    afx_msg void OnSkyboxDown();
    afx_msg void OnSkyboxDel();
    afx_msg void OnHourLengthEdit();
    afx_msg void OnStartTimeEdit();
    afx_msg void OnHScroll(UINT /*sbcode*/, UINT /*pos*/, CScrollBar *scrollBar);

    afx_msg LRESULT OnCTSelTime(WPARAM /*wParam*/, LPARAM /*lParam*/);

    void OnSunAngleSlider(SliderMovementState sms);
    afx_msg void OnSunAngleEdit();

    void OnMoonAngleSlider(SliderMovementState sms);
    afx_msg void OnMoonAngleEdit();

    void OnTimeOfDaySlider(SliderMovementState sms);

    afx_msg void OnSunAnimBtn();
    afx_msg void OnAmbAnimBtn();
    afx_msg void OnResetBtn();
    afx_msg void OnAddClrBtn();
    afx_msg void OnDelClrBtn();
    afx_msg void OnEditClrText();

    afx_msg LRESULT OnTimelineBegin (WPARAM wparam, LPARAM lparam);
    afx_msg LRESULT OnTimelineMiddle(WPARAM wparam, LPARAM lparam);
    afx_msg LRESULT OnTimelineDone  (WPARAM wparam, LPARAM lparam);
    afx_msg LRESULT OnTimelineAdd   (WPARAM wparam, LPARAM lparam);
    afx_msg LRESULT OnTimelineNewSel(WPARAM wparam, LPARAM lparam);

    afx_msg LRESULT OnPickerDown(WPARAM wparam, LPARAM lparam);
    afx_msg LRESULT OnPickerMove(WPARAM wparam, LPARAM lparam);
    afx_msg LRESULT OnPickerUp  (WPARAM wparam, LPARAM lparam);

    afx_msg void OnEditEnvironText();

    void OnMIESlider(SliderMovementState sms);

    void OnTurbOffsSlider    (SliderMovementState sms);
    void OnTurbFactSlider    (SliderMovementState sms);
    void OnVertEffSlider     (SliderMovementState sms);
    void OnSunHeightEffSlider(SliderMovementState sms);
    void OnPowerSlider       (SliderMovementState sms);
	void OnTexLodSlider      (SliderMovementState sms);

    afx_msg BOOL OnToolTipText(UINT, NMHDR* pNMHDR, LRESULT *result);
	afx_msg HBRUSH OnCtlColor( CDC* pDC, CWnd* pWnd, UINT nCtlColor );

	afx_msg void OnSunAngleEditKillFocus();
	afx_msg void OnMoonAngleEditKillFocus();	
	afx_msg void OnMieAmountEditKillFocus();	
	afx_msg void OnTurbOffEditKillFocus();
	afx_msg void OnTurbFactEditKillFocus();
	afx_msg void OnVertEffEditKillFocus();
	afx_msg void OnSunEffEditKillFocus();
	afx_msg void OnPowerEditKillFocus();

	afx_msg void OnBnClickedSunColorButton();
	afx_msg void OnBnClickedMoonColorButton();
	afx_msg void OnEnChangeSuncolorEdit();
	afx_msg void OnEnChangeMooncolorEdit();

    DECLARE_MESSAGE_MAP()

    DECLARE_AUTO_TOOLTIP(PageOptionsEnvironment, CFormView)

    void onModeChanged();
    void rebuildAnimation();
    void saveUndoState(BW::string const &description);
    void timelineChanged();
    void pickerChanged();
    void rebuildSkydomeList();

private:
    enum Mode
    {
        MODE_SUN,
        MODE_AMB
    };

    CEdit                           skyFileEdit_;
    controls::ImageButton           skyBrowseFileBtn_;
    controls::ImageButton           skyCopyFileBtn_;
    CEdit                           todFileEdit_;
    controls::ImageButton           todBrowseFileBtn_;
    controls::ImageButton           todCopyFileBtn_;
    SkyDomeListBox					skyDomesList_;
    CButton                         skyDomesAddBtn_;
    CButton                         skyDomesClearBtn_;
    controls::DialogToolbar         skyDomesTB_;
    CEdit                           skyBoxVisEdit_;
    CButton                         skyBoxVisBtn_;
    CEdit                           skyBoxGradEdit_;
    CButton                         skyBoxGradBtn_;
    controls::EditNumeric           hourLength_;
    controls::EditNumeric           startTime_;

	//Sun
    LimitSlider                     sunAngleSlider_;
    controls::EditNumeric           sunAngleEdit_;
	controls::EditNumeric           sunColorREdit_;
	controls::EditNumeric           sunColorGEdit_;
	controls::EditNumeric           sunColorBEdit_;
	//Moon
    LimitSlider                     moonAngleSlider_;
    controls::EditNumeric           moonAngleEdit_; 
	controls::EditNumeric           moonColorREdit_;
	controls::EditNumeric           moonColorGEdit_;
	controls::EditNumeric           moonColorBEdit_;

    CSliderCtrl                     timeOfDaySlider_;
    controls::EditNumeric           timeOfDayEdit_;
    CButton                         sunAnimBtn_;
    CButton                         ambAnimBtn_;
    CButton                         resetBtn_;
    controls::ColorTimeline         *colourTimeline_;
    controls::ColorPicker           *colourPicker_;
    CButton                         addClrBtn_;
    CButton                         delClrBtn_;
    controls::EditNumeric           rEdit_;
    controls::EditNumeric           gEdit_;
    controls::EditNumeric           bEdit_;
    controls::EditNumeric           mieEdit_;
    LimitSlider                     mieSlider_;
    controls::EditNumeric           turbOffsEdit_;
    LimitSlider                     turbOffsSlider_;
    controls::EditNumeric           turbFactorEdit_;
    LimitSlider                     turbFactorSlider_;
    controls::EditNumeric           vertexHeightEffectEdit_;
    LimitSlider                     vertexHeightEffectSlider_;
    controls::EditNumeric           sunHeightEffectEdit_;
    LimitSlider                     sunHeightEffectSlider_;
    controls::EditNumeric           powerEdit_;
    LimitSlider                     powerSlider_;
	TerrainTextureLodControl		terrainTextureLod_;
    bool                            initialised_;
    size_t                          filterChange_;
    Mode                            mode_;
    float                           initialValue_;
    Vector4                         initialColor_;
    bool                            sliding_;

    static PageOptionsEnvironment   *s_instance_;
};

IMPLEMENT_BASIC_CONTENT_FACTORY(PageOptionsEnvironment)

BW_END_NAMESPACE

#endif // PAGE_OPTIONS_ENVIRONMENT_HPP
