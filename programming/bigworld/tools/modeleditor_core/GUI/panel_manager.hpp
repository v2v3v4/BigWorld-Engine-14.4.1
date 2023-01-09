#ifndef PANEL_MANAGER_HPP
#define PANEL_MANAGER_HPP

#include "cstdmf/singleton.hpp"
#include "ual/ual_manager.hpp"
#include "guitabs/manager.hpp"
#include "guimanager/gui_updater_maker.hpp"
#include "modeleditor_core/i_model_editor_app.hpp"
#include "tools/common/base_panel_manager.hpp"

class CFrameWnd;

BW_BEGIN_NAMESPACE

class UalDialog;
class UalItemInfo;

class PanelManager :
	public Singleton<PanelManager>,
	public GUI::ActionMaker<PanelManager>,	 // load default panels
	public GUI::UpdaterMaker<PanelManager>,	 // update show/hide panels
	public BasePanelManager
{
public:
	~PanelManager();

	static bool init(
		CFrameWnd* mainFrameWnd, CWnd* mainView,
		IModelEditorApp * editorApp, IMainFrame * mainFrame );
	static void fini();

	// panel stuff
	bool ready();
	void showPanel( const BW::string& pyID, int show = 1 );
	int isPanelVisible( const BW::string& pyID );
	BW::wstring getPanelID( const BW::string& pyID );
	int currentTool();
	bool handelGUIAction( GUI::ItemPtr item );
	bool showSidePanel( GUI::ItemPtr item );
	bool hideSidePanel( GUI::ItemPtr item );
	unsigned int handleGUIUpdate( GUI::ItemPtr item );
	unsigned int updateSidePanel( GUI::ItemPtr item );
	void updateControls();
	void onClose();
	void ualAddItemToHistory( BW::string filePath );

	GUITABS::Manager& panels() { return panels_; }
	
	IModelEditorApp * getEditorApp() const { return editorApp_; }

private:
	PanelManager();

	BW::map< BW::string, BW::wstring > contentID_;

	int currentTool_;
	CFrameWnd * mainFrameWnd_;
	IMainFrame * mainFrame_;
	CWnd* mainView_;
	bool ready_;
	HCURSOR addCursor_;

	UalManager ualManager_;

	GUITABS::Manager panels_;

	BW::string currentLanguageName_; 
	BW::string currentCountryName_; 
	IModelEditorApp * editorApp_;

	// guimanager stuff
	bool recent_models( GUI::ItemPtr item );
	bool recent_lights( GUI::ItemPtr item );
	bool setLanguage( GUI::ItemPtr item );
	unsigned int updateLanguage( GUI::ItemPtr item );
	bool OnAppAbout( GUI::ItemPtr item );
	bool OnShortcuts( GUI::ItemPtr item );
	

	// panel stuff
	void finishLoad();
	bool initPanels();
	bool loadDefaultPanels( GUI::ItemPtr item );
	bool loadLastPanels( GUI::ItemPtr item );
	BW::string getContentID( BW::string& pyID );

	// UAL callbacks
	void ualItemDblClick( UalItemInfo* ii );
	void ualStartDrag( UalItemInfo* ii );
	void ualUpdateDrag( UalItemInfo* ii );
	void ualEndDrag( UalItemInfo* ii );
	void ualStartPopupMenu( UalItemInfo* ii, UalPopupMenuItems& menuItems );
	void ualEndPopupMenu( UalItemInfo* ii, int result );

	struct Impl;
	Impl* pImpl;
	friend Impl;
};

BW_END_NAMESPACE

#endif // PANEL_MANAGER_HPP
