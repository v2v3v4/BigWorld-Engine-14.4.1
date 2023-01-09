#ifndef PANEL_MANAGER_HPP
#define PANEL_MANAGER_HPP


#include "cstdmf/singleton.hpp"
#include "ual/ual_manager.hpp"
#include "guitabs/manager.hpp"
#include "guimanager/gui_action_maker.hpp"
#include "guimanager/gui_updater_maker.hpp"
#include "tools/common/base_panel_manager.hpp"

BW_BEGIN_NAMESPACE

class UalDialog;
class UalItemInfo;


class PanelManager :
	public Singleton<PanelManager>,
	public GUI::ActionMaker<PanelManager>  ,	// show panels
	public GUI::ActionMaker<PanelManager,1>,	// hide panels
	public GUI::ActionMaker<PanelManager, 2>,		// Language selection
	public GUI::UpdaterMaker<PanelManager>,		// update show/hide side panel
	public GUI::UpdaterMaker<PanelManager, 1>,	// update show/hide ual panel
	public GUI::UpdaterMaker<PanelManager, 2>,	// update show/hide messages panel
	public GUI::UpdaterMaker<PanelManager, 3>,		// Language selection
	public BasePanelManager
{
public:
    ~PanelManager();

	static bool init( CFrameWnd* mainFrame, CWnd* mainView, const wchar_t* defaultLayoutFilename );
	static void fini();

	// panel stuff
    bool ready();
    void showPanel(BW::wstring const &pyID, int show);
    bool isPanelVisible(BW::wstring const &pyID);
    bool showSidePanel(GUI::ItemPtr item);
    bool hideSidePanel(GUI::ItemPtr item);
    bool setLanguage(GUI::ItemPtr item);
    unsigned int updateSidePanel(GUI::ItemPtr item);
	unsigned int updateUalPanel(GUI::ItemPtr item);
	unsigned int updateMsgsPanel(GUI::ItemPtr item);
	unsigned int updateLanguage(GUI::ItemPtr item);
    void updateControls();
    void onClose();
    void ualAddItemToHistory(BW::string filePath);
	bool loadDefaultPanels(GUI::ItemPtr item);
	bool loadLastPanels(GUI::ItemPtr item, const wchar_t* defaultLayoutFilename );

	GUITABS::Manager& panels() { return panels_; }

private:
    PanelManager();

    void finishLoad();
    bool initPanels(const wchar_t* defaultLayoutFilename );

	// UAL callbacks
	void ualItemDblClick(UalItemInfo* ii);
	void ualStartDrag(UalItemInfo* ii);
	void ualUpdateDrag(UalItemInfo* ii);
	void ualEndDrag(UalItemInfo* ii);

private:
	UalManager ualManager_;

	GUITABS::Manager panels_;

    BW::map<BW::wstring, BW::wstring>  m_contentID;
    int                                 m_currentTool;
    CFrameWnd                           *m_mainFrame;
    CWnd                                *m_mainView;
    bool                                m_ready;
    bool                                m_dropable;
	BW::string							currentLanguageName_;
	BW::string							currentCountryName_;
};

BW_END_NAMESPACE

#endif // PANEL_MANAGER_HPP
