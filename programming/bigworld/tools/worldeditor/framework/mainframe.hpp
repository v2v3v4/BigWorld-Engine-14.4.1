#ifndef ME_MAINFRAME_HPP
#define ME_MAINFRAME_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "guimanager/gui_functor_cpp.hpp"
#include "common/base_mainframe.hpp"
#include "editor_shared/gui/i_main_frame.hpp"

BW_BEGIN_NAMESPACE

// MainFrame
class MainFrame
	: public BaseMainFrame
	, public IMainFrame
	, GUI::ActionMaker<MainFrame>, // save prefab
	GUI::ActionMaker<MainFrame, 1>, // show toolbar
	GUI::ActionMaker<MainFrame, 2>, // hide toolbar
	GUI::ActionMaker<MainFrame, 3>, // show status bar
	GUI::ActionMaker<MainFrame, 4>, // hide status bar
	GUI::ActionMaker<MainFrame, 5>, // show player preview
	GUI::ActionMaker<MainFrame, 6>, // hide player preview
	GUI::UpdaterMaker<MainFrame>, // update show toolbar
	GUI::UpdaterMaker<MainFrame, 1>, // update show status bar
	GUI::UpdaterMaker<MainFrame, 2>, // update player preview
	GUI::UpdaterMaker<MainFrame, 3> // update tool mode
{
protected: 
	MainFrame();
	DECLARE_DYNCREATE(MainFrame)

public:
	virtual ~MainFrame();

	void updateStatusBar();
	void frameUpdate();
	bool resizing() const { return resizing_; }
	void enableCloseButton( bool enable );

	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg LRESULT OnEnterSizeMove (WPARAM, LPARAM);
	afx_msg LRESULT OnExitSizeMove (WPARAM, LPARAM);
	afx_msg void OnUpdateIndicatorTriangles(CCmdUI *pCmdUI);
	afx_msg void OnUpdateIndicatorSnaps(CCmdUI *pCmdUI);
	afx_msg void OnUpdateIndicatorPosition(CCmdUI *pCmdUI);
	afx_msg void OnUpdateIndicatorMemoryLoad(CCmdUI *pCmdUI);
	afx_msg void OnUpdateIndicatorFrameRate(CCmdUI *pCmdUI);
	afx_msg void OnUpdateIndicatorChunks(CCmdUI *pCmdUI);
	virtual void GetMessageString(UINT nID, CString& rMessage) const;
	afx_msg void OnPopupPropertyListAddItem();
	afx_msg void OnPopupPropertyListItemRemoveItem();
	afx_msg void OnClose();
	afx_msg void OnGUIManagerCommand(UINT nID);
	afx_msg void OnMenuSelect( UINT nItemID, UINT nFlags, HMENU hSysMenu );
	afx_msg void OnExitMenuLoop( BOOL bIsTrackPopupMenu );
	afx_msg void OnToolbarHotItemChange( UINT id, NMHDR * pNotifyStruct, LRESULT * result );
	afx_msg void OnSysColorChange();

	void setMessageText( const wchar_t * pText );
	void setStatusText( UINT id, const wchar_t * text );
	bool cursorOverGraphicsWnd() const { return false; }
	void updateGUI( bool force = false ) {}
	Vector2 currentCursorPosition() const { return Vector2::ZERO; }
	Vector3 getWorldRay(int x, int y) const { return Vector3::ZERO; }

	void grabFocus() { SetFocus(); }

protected:
	void onButtonClick();

	BW::string GetActionName(UINT nID);

	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();

	virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
	virtual BOOL OnWndMsg(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);

	virtual LRESULT DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()

	bool saveSelectionAsPrefab( GUI::ItemPtr item );

	bool hideSelection( GUI::ItemPtr item );
	bool unhideSelection( GUI::ItemPtr item );
	bool unhideAll( GUI::ItemPtr item );

	bool freezeSelection( GUI::ItemPtr item );
	bool unfreezeSelection( GUI::ItemPtr item );
	bool unfreezeAll( GUI::ItemPtr item );

	bool showToolbar( GUI::ItemPtr item );
	bool hideToolbar( GUI::ItemPtr item );
	unsigned int updateToolbar( GUI::ItemPtr item );

	bool showStatusBar( GUI::ItemPtr item );
	bool hideStatusBar( GUI::ItemPtr item );
	unsigned int updateStatusBar( GUI::ItemPtr item );

	bool showPlayerPreview( GUI::ItemPtr item );
	bool hidePlayerPreview( GUI::ItemPtr item );
	unsigned int updatePlayerPreview( GUI::ItemPtr item );
	unsigned int updateToolMode( GUI::ItemPtr item );

private:
	typedef struct _object PyObject;

	CStatusBar		m_wndStatusBar;
	PyObject		*pScriptObject_;
	bool			resizing_;
	CString			triangles_;	
	bool			initialised_;
};

BW_END_NAMESPACE

#endif // ME_MAINFRAME_HPP
