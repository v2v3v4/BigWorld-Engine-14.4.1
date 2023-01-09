#ifndef PAGE_FLORA_SETTING_HPP
#define PAGE_FLORA_SETTING_HPP

#include "list_cache.hpp"
#include "smart_list_ctrl.hpp"
#include "list_xml_provider.hpp"
#include "controls/auto_tooltip.hpp"
#include "controls/search_field.hpp"
#include "guitabs/guitabs_content.hpp"
#include "common/user_messages.hpp"
#include "guitabs/nice_splitter_wnd.hpp"
#include "page_flora_setting_tree.hpp"
#include "page_flora_setting_secondary_wnd.hpp"
#include "guimanager/gui_updater_maker.hpp"

BW_BEGIN_NAMESPACE

/**
 *  This class is the flora editor panel.
 */
class PageFloraSetting : 
	public CFormView, 
	public GUITABS::Content,
	public GUI::ActionMaker<PageFloraSetting>,
	public GUI::UpdaterMaker<PageFloraSetting>,
	public SmartListCtrlEventHandler
{
	IMPLEMENT_BASIC_CONTENT
		(
		Localise(L"WORLDEDITOR/GUI/PAGE_FLORA_SETTING/SHORT_NAME"),
		Localise(L"WORLDEDITOR/GUI/PAGE_FLORA_SETTING/LONG_NAME"),
		290,          // default width
		500,          // default height
		NULL          // icon
		)

public:
	PageFloraSetting();
	virtual ~PageFloraSetting();

	enum { IDD = IDD_PAGE_FLORA_SETTING };

	FloraSettingSecondaryWnd& secondaryWnd();
	FloraSettingTree& tree();
	SmartListCtrl&	assetList();
	SmartListCtrl&	existingResourceList();
	ListXmlSectionProviderPtr listXmlSectionProvider();
	ListFileProviderPtr fileScanProvider();
	FixedListFileProviderPtr existingResourceProvider();
	bool toggle3DView();
	uint32 floraVerticesPerBlock();

	DataSectionPtr refreshFloraXMLDataSection( 
					HTREEITEM hHighLightItem = NULL, bool saveToDisk = true );

	void selectTextureItem( const BW::string& textureName );
	void postChange();
	void preChange();
	bool isReady();
	void layOut();


protected:
	virtual void listLoadingUpdate(){}
	virtual void listLoadingFinished(){}
	virtual void listItemSelect(){}
	virtual void listDoubleClick( int index ){}
	virtual void listItemRightClick( int index ){}
	virtual void listStartDrag( int index );
	virtual void listItemToolTip( int index, BW::wstring& info );
	virtual void listItemDelete();

	
	unsigned int toggle3DView( GUI::ItemPtr item );
	bool handleGUIAction( GUI::ItemPtr item );
	bool actionRefresh( GUI::ItemPtr item );
	bool actionNewEcotypy( GUI::ItemPtr item );
	bool actionMoveUpItem( GUI::ItemPtr item );
	bool actionMoveDownItem( GUI::ItemPtr item );
	bool actionDeleteItem( GUI::ItemPtr item );
	bool actionTurnOn3DView( GUI::ItemPtr item );
	bool actionTurnOff3DView( GUI::ItemPtr item );

	virtual BOOL PreTranslateMessage( MSG * pMsg );
	virtual void DoDataExchange( CDataExchange* pDX );
	afx_msg LRESULT OnUpdateControls( WPARAM wParam, LPARAM lParam );
	afx_msg LRESULT OnNewSpace(WPARAM wParam, LPARAM lParam );
	afx_msg void OnSize( UINT nType, int cx, int cy );
	afx_msg int OnCreate( LPCREATESTRUCT lpCreateStruct );
	afx_msg void OnGUIManagerCommand( UINT nID );
	DECLARE_MESSAGE_MAP()

	DECLARE_AUTO_TOOLTIP_EX( PageFloraSetting );

private:
	bool                            initialised_;
	CToolBarCtrl					toolbar_;

	FloraSettingTree				floraSettingTree_;
	NiceSplitterWnd					splitterBar_;
	bool							layoutVertical_;
	FloraSettingSecondaryWnd		secondaryWnd_;

	ListFileProviderPtr				fileScanProvider_;
	FixedListFileProviderPtr		existingResourceProvider_;
	ListXmlSectionProviderPtr		listXmlSectionProvider_;

	bool							toggle3DView_;
	bool							changed_;
	BW::string						floraXML_;


	void loadToolbar( DataSectionPtr section );
	void initPage();

	// drag and drop:
	void stopDrag();
	void cancelDrag();
	void resetDragDropTargets();

	void fillSelectedItemsIntoAssetsInfo( SmartListCtrl& listWnd, 
			BW::vector<AssetInfo>& assetsInfo );

	void dragLoop( BW::vector<AssetInfo>& assetsInfo );

	void handleDragMouseMove( BW::vector<AssetInfo>& assetsInfo,
			const CPoint& srcPt, bool isScreenCoords = false );

	bool updateDrag( BW::vector<AssetInfo>& assetsInfo, 
			const CPoint& srcPt, bool endDrag );

	void updateFloraSettingTreeDrag( BW::vector<AssetInfo>& assetsInfo, 
			const CPoint& srcPt, bool endDrag );

	void updateExistingResourceListDrag( BW::vector<AssetInfo>& assetsInfo, 
			const CPoint& srcPt, bool endDrag );


	HTREEITEM currentFocusItem();
	void deleteSelectedExistingResource();
	void reload();

};
IMPLEMENT_BASIC_CONTENT_FACTORY(PageFloraSetting)

BW_END_NAMESPACE

#ifdef CODE_INLINE
#include "page_flora_setting.ipp"
#endif//CODE_INLINE

#endif // PAGE_FLORA_SETTING_HPP


