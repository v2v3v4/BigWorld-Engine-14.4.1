#ifndef SCENE_BROWSER_DLG_HPP
#define SCENE_BROWSER_DLG_HPP


#include "resource.h"
#include "scene_browser_list.hpp"
#include "guitabs/guitabs_content.hpp"
#include "controls/search_field.hpp"
#include "controls/auto_tooltip.hpp"
#include "controls/wait_anim.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class implements the Scene Browser dialog as a dockable/floatable
 *	panel.
 */
class SceneBrowserDlg : public CDialog, public GUITABS::Content
{
	IMPLEMENT_LOADABLE_CONTENT( Localise( L"SCENEBROWSER/SHORT_NAME" ),
					Localise( L"SCENEBROWSER/LONG_NAME" ), 300, 400, NULL );

public:
	enum { IDD = IDD_SCENE_BROWSER };

	SceneBrowserDlg( CWnd* pParent = NULL );

	bool load( DataSectionPtr section );
	bool save( DataSectionPtr section );

	void currentItems( BW::vector< ChunkItemPtr > & chunkItems ) const;

protected:
	DECLARE_AUTO_TOOLTIP( SceneBrowserDlg, CDialog );

	// MFC related methods
	virtual void DoDataExchange( CDataExchange* pDX );

	afx_msg BOOL OnInitDialog();
	afx_msg void OnSize( UINT nType, int cx, int cy );
	afx_msg void OnGroupBy();
	afx_msg void OnGUIManagerCommand( UINT nID );
	afx_msg LRESULT OnSearchFieldChanged( WPARAM wParam, LPARAM lParam );
	afx_msg LRESULT OnSearchFieldFilters( WPARAM wParam, LPARAM lParam );
	afx_msg LRESULT OnUpdateControls( WPARAM wParam, LPARAM lParam );
	afx_msg LRESULT OnListGroupByChanged( WPARAM wParam, LPARAM lParam );
	afx_msg LRESULT OnListScrollTo( WPARAM wParam, LPARAM lParam );
	DECLARE_MESSAGE_MAP()

private:
	// MFC controls
	SearchField search_;

	CStatic groupByLabel_;
	CComboBox groupBy_;

	CToolBarCtrl toolbar_;

	SceneBrowserList list_;

	CStatic statusBar_;

	controls::WaitAnim workingAnim_;

	// members
	uint64 lastUpdate_;
	BW::vector<ChunkItemPtr> lastSelection_;
	ListGroups groups_;
	bool ignoreGroupByMessages_;

	// Control positioning offsets, used on resize.
	int groupByLabelLeft_;
	int groupByLabelTop_;

	int groupByLeft_;
	int groupByMinRight_;
	int groupByTop_;
	int groupByHeight_;

	int toolbarLeft_;
	int toolbarMinRight_;

	int listLeft_;
	int listRight_;
	int listTop_;
	int listBottom_;

	int statusBarLeft_;
	int statusBarRight_;
	int statusBarHeight_;
	int statusBarBottom_;

	bool inited_;

	// Private methods
	void updateStatusBar();
};


IMPLEMENT_CDIALOG_CONTENT_FACTORY( SceneBrowserDlg, SceneBrowserDlg::IDD );

BW_END_NAMESPACE

#endif // SCENE_BROWSER_DLG_HPP
