#pragma once

#include "ual_resource.h"
#include "ual_vfolder_loader.hpp"
#include "cstdmf/smartpointer.hpp"
#include "resmgr/datasection.hpp"
#include "guitabs/nice_splitter_wnd.hpp"
#include "guitabs/guitabs_content.hpp"
#include "folder_tree.hpp"
#include "list_cache.hpp"
#include "filter_spec.hpp"
#include "smart_list_ctrl.hpp"
#include "list_file_provider.hpp"
#include "list_xml_provider.hpp"
#include "vfolder_file_provider.hpp"
#include "vfolder_xml_provider.hpp"
#include "filters_ctrl.hpp"
#include "xml_item_list.hpp"
#include "ual_manager.hpp"
#include "common/popup_menu.hpp"
#include "controls/search_field.hpp"

BW_BEGIN_NAMESPACE

// Forward declarations
class FilterHolder;


/**
 *	This class just serves as container of information about an Asset Browser
 *	folder.
 */
class UalFolderData : public ReferenceCount
{
public:
	BW::wstring internalTag_;	// used to refer to vFolders from within the code
	int thumbSize_;
	int originalThumbSize_;
	bool showInList_;
	bool multiItemDrag_;
	BW::vector<BW::wstring> disabledFilters_;
	XmlItemVec customItems_;
	BW::wstring idleText_;
};
typedef SmartPointer<UalFolderData> UalFolderDataPtr;



/**
 *	This class implements the Asset Browser dialog.  It implements a GUITABS
 *	panel, and it also implements event handlers for the tree view, list and
 *	filters controls it contains.  This dialog depends on including the
 *	appropriate resources.  Open world_editor_app.rc in a text editor and
 *	search for ual_resource.h and ual.rc to see how to include it in a new
 *	tool.
 */
class UalDialog :
	public CDialog,
	public GUITABS::Content,
	public FolderTreeEventHandler,
	public SmartListCtrlEventHandler,
	public FiltersCtrlEventHandler
{
public:
	// GUITABS Content Methods
	static const BW::wstring contentID;
	BW::wstring getContentID() { return contentID; }
	BW::wstring getDisplayString() { return dlgLongCaption_; }
	BW::wstring getTabDisplayString() { return dlgShortCaption_; }
	HICON getIcon() { return hicon_; }
	CWnd* getCWnd() { return this; }

	void getPreferredSize( int& w, int& h )
	{
		w = preferredWidth_;
		h = preferredHeight_;
	}

	bool isClonable() { return true; }
	GUITABS::ContentPtr clone();
	void handleRightClick( int x, int y );
	void OnOK() { }
	void OnCancel() { }
	void PostNcDestroy() { }
	bool load( DataSectionPtr section );
	bool save( DataSectionPtr section );
	OnCloseAction onClose( bool isLastContent ) { return isLastContent?CONTENT_HIDE:CONTENT_DESTROY; }

	// Normal public methods
	UalDialog( const BW::wstring& configFile = L"" );   // standard constructor
	virtual ~UalDialog();

	static void registerVFolderLoader( UalVFolderLoaderPtr loader );
	static void fini();

	bool loadConfig( const BW::wstring fname = L"" );
	void saveConfig();

	void setListStyle( SmartListCtrl::ViewStyle style );
	void setLayout( bool vertical, bool resetLastSize = false );

	void setShortCaption( BW::wstring caption ) { dlgShortCaption_ = caption; }
	void setLongCaption( BW::wstring caption ) { dlgLongCaption_ = caption; }
	void setIcon( HICON hicon ) { hicon_ = hicon; }

	void updateItem( const BW::wstring& longText );
	static bool vfolderFindByTag( HTREEITEM item, void* testData );
	void showItem( const BW::wstring& vfolder, const BW::wstring& longText );

	// Dialog Data
	enum { IDD = IDD_UAL };

	// Dialog's controls event handling
	void favouritesChanged();
	void historyChanged();
	void folderTreeSelect( VFolderItemData* data );
	void folderTreeStartDrag( VFolderItemData* data );
	void folderTreeItemDelete( VFolderItemData* data );
	void folderTreeRightClick( VFolderItemData* data );
	void folderTreeDoubleClick( VFolderItemData* data );
	void listLoadingUpdate();
	void listLoadingFinished();
	void listItemSelect();
	void listItemDelete();
	void listDoubleClick( int index );
	void listStartDrag( int index );
	void listItemRightClick( int index );
	void listItemToolTip( int index, BW::wstring& info );
	void filterClicked( const wchar_t* name, bool pushed, void* data );

	// Methods used by the loaders
	ListFileProviderPtr fileListProvider() { return fileListProvider_; }
	ListXmlProviderPtr xmlListProvider() { return xmlListProvider_; }
	ListXmlProviderPtr historyListProvider() { return historyListProvider_; }
	ListXmlProviderPtr favouritesListProvider() { return favouritesListProvider_; }
	VFolderXmlProvider* historyFolderProvider() { return historyFolderProvider_.getObject(); }
	VFolderXmlProvider* favouritesFolderProvider() { return favouritesFolderProvider_.getObject(); }
	void historyFolderProvider( VFolderXmlProvider* prov ) { historyFolderProvider_ = prov; }
	void favouritesFolderProvider( VFolderXmlProvider* prov ) { favouritesFolderProvider_ = prov; }

	BW::vector<BW::wstring> getSelectedAssetPaths();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	BOOL OnInitDialog();
	BOOL PreTranslateMessage( MSG* msg );

	afx_msg void OnSetFocus( CWnd* pOldWnd );
	afx_msg void OnKillFocus( CWnd* pNewWnd );
	afx_msg void OnDestroy();
	afx_msg void OnSize( UINT nType, int cx, int cy );
	afx_msg void OnGUIManagerCommand(UINT nID);
	afx_msg LRESULT OnSearchFieldChanged( WPARAM wParam, LPARAM lParam );
	afx_msg LRESULT OnSearchFieldFilters( WPARAM wParam, LPARAM lParam );
	DECLARE_MESSAGE_MAP()

private:
	friend class UalManager;
	friend class UalVFolderLoader;

	BW::wstring configFile_;
	BW::wstring lastLanguage_;
	BW::wstring dlgShortCaption_;
	BW::wstring dlgLongCaption_;
	HICON hicon_;
	int preferredWidth_;
	int preferredHeight_;
	bool layoutVertical_;
	int layoutLastRowSize_;
	int layoutLastColSize_;
	int defaultSize_;
	CToolBarCtrl toolbar_;
	FolderTree folderTree_;
	SmartListCtrl smartList_;
	SearchField search_;
	CStatic statusBar_;

	// need to have the list providers declared per-ual
	ListFileProviderPtr fileListProvider_;
	ListXmlProviderPtr xmlListProvider_;
	ListXmlProviderPtr historyListProvider_;
	ListXmlProviderPtr favouritesListProvider_;
	
	// have to have folder providers for history and favourites, for refreshing
	typedef SmartPointer<VFolderXmlProvider> VFolderXmlProviderPtr;
	VFolderXmlProviderPtr historyFolderProvider_;
	VFolderXmlProviderPtr favouritesFolderProvider_;
	BW::vector<UalFolderDataPtr> folderData_;

	// Use CSplitterWnd if NiceSplitterWnd (in GUITABS) not available (will need to rewrite code)
	typedef NiceSplitterWnd SplitterBarType; 
	SplitterBarType* splitterBar_;
	FiltersCtrl filtersCtrl_;
	FilterHolder filterHolder_;
	bool showFilters_;
	HWND lastFocus_;

	// override UAL's config file for this folders: don't load them
	BW::vector<BW::wstring> excludeVFolders_;

	// section containing customVFolders data, only used when cloning
	DataSectionPtr customVFolders_; 

	// stores data from the last drag and drop operation when cloning using drag&drop
	UalItemInfo lastItemInfo_;

	// used to avoid flickering in the list when clicking items in the tree
	BW::wstring lastListInit_;

	CToolTipCtrl toolTip_;
	BW::wstring delayedListShowItem_;

	HICON iconFromXml( DataSectionPtr section, BW::string item );
	void loadMain( DataSectionPtr section );
	void loadToolbar( DataSectionPtr section );
	void loadFilters( DataSectionPtr section );
	void loadVFolders( DataSectionPtr section, const BW::wstring& loadOneName = L"", VFolderPtr parent = 0 );
	VFolderPtr loadVFolder( DataSectionPtr section, const BW::wstring& loadOneName, VFolderPtr parent, DataSectionPtr customData = 0 );
	void loadVFolderExcludeInfo( DataSectionPtr section );
	void loadCustomVFolders( DataSectionPtr section, const BW::wstring& loadOneName = L"" );
	VFolderPtr loadFromBaseVFolder( DataSectionPtr section, const BW::wstring& baseName, DataSectionPtr customData, VFolderPtr parent = 0 );

	void buildSmartListFilters();

	void buildFiltersCtrl();
	void buildFolderTree();
	void buildSmartList();

	bool guiActionRefresh();
	bool guiActionLayout();

	int createContextMenu( UalPopupMenuItems& menuItems );
	void showItemContextMenu( UalItemInfo* ii );
	void showContextMenu( VFolderItemData* data );
	void fillAssetsVectorFromList( BW::vector<AssetInfo>& assets );

	void updateFiltersImage();
	void adjustSplitterSize( int width, int height );
	void adjustSearchSize( int width, int height );
	void adjustFiltersSize( int width, int height );

	void callbackVFolderSelect( VFolderItemData* data );
	void refreshStatusBar();
	void setFolderTreeStatusBar( VFolderItemData* data );

	void getItemInfo( int index, AssetInfo& assetInfo );
	void scrollWindow( CWnd* wnd, CPoint pt );
	void handleDragMouseMove( UalItemInfo& ii, const CPoint& srcPt, bool isScreenCoords = false );
	void dragLoop( BW::vector<AssetInfo>& assetsInfo, bool isFolder = false, void* folderExtraData = 0 );
	void stopDrag();
	void cancelDrag();
	void updateSmartListDrag( const UalItemInfo& itemInfo, bool endDrag );
	void updateFolderTreeDrag( const UalItemInfo& itemInfo, bool endDrag );
	bool updateDrag( const UalItemInfo& itemInfo, bool endDrag );
	void resetDragDropTargets();

	void setStatusText( const BW::wstring& text );

	void error( const BW::string& msg );
};


/**
 *	This class implements a GUITABS panel factory for the UalDialog.
 */
class UalDialogFactory : public GUITABS::ContentFactory
{
public:
	UalDialogFactory() {}

	GUITABS::ContentPtr create()
	{
		BW_GUARD;

		return createUal( UalManager::instance().getConfigFile() );
	}

	UalDialog* createUal( const BW::wstring& configFile )
	{
		BW_GUARD;

		UalDialog* newUal = new UalDialog( configFile );
		newUal->Create( UalDialog::IDD );
		return newUal;
	}

	BW::wstring getContentID() { return UalDialog::contentID; }
};

BW_END_NAMESPACE

