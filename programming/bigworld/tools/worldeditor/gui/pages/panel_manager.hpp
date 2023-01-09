#ifndef PANEL_MANAGER_HPP
#define PANEL_MANAGER_HPP


#include "cstdmf/singleton.hpp"
#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "ual/ual_manager.hpp"
#include "guitabs/manager.hpp"
#include "guimanager/gui_functor_cpp.hpp"
#include "tools/common/base_panel_manager.hpp"
#include "cstdmf/bw_set.hpp"

BW_BEGIN_NAMESPACE

class PanelManager :
	public Singleton<PanelManager>,
	public GUI::ActionMaker<PanelManager>,	// load default panels
	public GUI::ActionMaker<PanelManager,1>,// show panels
	public GUI::ActionMaker<PanelManager,2>,// hide panels
	public GUI::ActionMaker<PanelManager,3>,// load last panels
	public GUI::UpdaterMaker<PanelManager>,	// update show/hide panels
	public GUI::UpdaterMaker<PanelManager,1>,// disable/enable show/hide panels
	public BasePanelManager
{
public:
	~PanelManager() {};

	static bool init( CFrameWnd* mainFrame, CWnd* mainView );
	static void fini();

	bool ready();
	void updateUIToolMode( const BW::wstring& pyID );
	void setToolMode( const BW::wstring pyID, bool canUndo = true );
	void setDefaultToolMode( bool canUndo = true );
	void showPanel( const BW::wstring pyID, int show );
	int isPanelVisible( const BW::wstring pyID );
	const BW::wstring getContentID( const BW::wstring pyID );
	const BW::wstring getPythonID( const BW::wstring contentID );
	const BW::wstring currentTool();
	bool isCurrentTool( const BW::wstring& id );
	bool showSidePanel( GUI::ItemPtr item );
	bool hideSidePanel( GUI::ItemPtr item );
	unsigned int updateSidePanel( GUI::ItemPtr item );
	unsigned int disableEnablePanels( GUI::ItemPtr item );
	void updateControls();
	void onClose();
    void onNewSpace(unsigned int width, unsigned int height);
	void onChangedChunkState(int x, int z);
	void onNewWorkingChunk();
    void onBeginSave();
    void onEndSave();
	void showItemInUal( const BW::wstring& vfolder, const BW::wstring& longText );

	// UAL callbacks
	void ualAddItemToHistory( const BW::wstring & str, const BW::wstring & type );

	GUITABS::Manager& panels() { return panels_; }

private:
	PanelManager();

	BW::map<BW::wstring,BW::wstring> panelNames_; // pair( python ID, GUITABS contentID )
	typedef std::pair<BW::wstring,BW::wstring> StrPair;
	BW::wstring currentTool_;
	CFrameWnd* mainFrame_;
	CWnd* mainView_;
	bool ready_;
	BW::set<BW::wstring> ignoredObjectTypes_;

	UalManager ualManager_;

	GUITABS::Manager panels_;

	// panel stuff
	void finishLoad();
	bool initPanels();
	bool allPanelsLoaded();
	bool loadDefaultPanels( GUI::ItemPtr item );
	bool loadLastPanels( GUI::ItemPtr item );

	// UAL callbacks
	void ualItemClick( UalItemInfo* ii );
	void ualDblItemClick( UalItemInfo* ii );
	void ualStartPopupMenu( UalItemInfo* ii, UalPopupMenuItems& menuItems );
	void ualEndPopupMenu( UalItemInfo* ii, int result );

	void ualStartDrag( UalItemInfo* ii );
	void ualUpdateDrag( UalItemInfo* ii );
	void ualEndDrag( UalItemInfo* ii );

	// other
	void addSimpleError( const BW::string& msg );
};

BW_END_NAMESPACE

#endif // PANEL_MANAGER_HPP
