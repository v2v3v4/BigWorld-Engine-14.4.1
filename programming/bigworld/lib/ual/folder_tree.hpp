/**
 *	FolderTree: Inherits from CTreeCtrl to make a folder tree control with
 *	drag & drop support.
 */

#ifndef FOLDER_TREE_HPP
#define FOLDER_TREE_HPP

#include "cstdmf/smartpointer.hpp"
#include "asset_info.hpp"
#include "xml_item_list.hpp"
#include "filter_holder.hpp"
#include "thumbnail_manager.hpp"

BW_BEGIN_NAMESPACE

// Forward Declarations
namespace controls
{
	class DragImage;
}


// Datatypes
class VFolder;
typedef SmartPointer<VFolder> VFolderPtr;

class VFolderItemData;
typedef SmartPointer<VFolderItemData> VFolderItemDataPtr;

class VFolderProvider;
typedef SmartPointer<VFolderProvider> VFolderProviderPtr;

class FolderTree;

class ListProvider;
typedef SmartPointer<ListProvider> ListProviderPtr;


/**
 *	This abstract class serves as the base class for concrete tree view asset
 *	provider implementations.  It works diferently than a list.  Items are
 *	looked up on demand, depending on whether the user has expanded the
 *	corresponding node in the tree or not.
 */
class VFolderProvider : public ReferenceCount
{
public:
	VFolderProvider() :
		folderTree_(),
		filterHolder_( 0 ),
		listProvider_( 0 )
	{}

	virtual ~VFolderProvider()
	{}

	/**
	 *	This abstract method is called to tell the provider that enumeration of
	 *	the children of the input "parent" folder is about to start.
	 *
	 *	@param parent	Parent virtual folder to start enumerating.
	 *	@return		True if it all is OK and the folder is not empty.
	 */
	virtual bool startEnumChildren( const VFolderItemDataPtr parent ) = 0;


	/**
	 *	This abstract method is called repeatedly, after the call to
	 *	startEnumChildren, to retrieve the items one by one.
	 *
	 *	@param thumbnailManager	Thumbnail generator.
	 *	@param img				Thumbnail image corresponding to the item.
	 *	@return		Item data, be it a subfolder or an asset.
	 */
	virtual VFolderItemDataPtr getNextChild(
		ThumbnailManager& thumbnailManager, CImage& img ) = 0;


	/**
	 *	This method simply associates a provider with its owner FolderTree
	 *	tree control.
	 *
	 *	@param folderTree	FolderTree that is the owner of this provider.
	 */
	virtual void setFolderTree( FolderTree* folderTree )
		{ folderTree_ = folderTree; }


	/**
	 *	This method tells the provider which set of filters to use when
	 *	retrieving items.
	 *
	 *	@param filterHolder	Object holding search text and set of filters.
	 */
	virtual void setFilterHolder( FilterHolder* filterHolder )
		{ filterHolder_ = filterHolder; }


	/**
	 *	This method tells this folder provider which list provider is 
	 *	associated with it, which is useful for updating the contents of the
	 *	list when the user clicks on a folder/subfolder.
	 *
	 *	@param listProvider		Corresponding list provider.
	 */
	virtual void setListProvider( ListProviderPtr listProvider )
		{ listProvider_ = listProvider; }


	/**
	 *	This method returns list provider is associated with this folder
	 *	provider, which is useful for updating the contents of the list when
	 *	the user clicks on a folder/subfolder.
	 *
	 *	@return		Corresponding list provider.
	 */
	virtual ListProviderPtr getListProvider()
		{ return listProvider_; }


	/**
	 *	This abstract method lets the provider generate and return the
	 *	appropriate thumbnail for an item.
	 *
	 *	@param thumbnailManager	Thumbnail generator.
	 *	@param data				Item data.
	 *	@param img				Thumbnail image corresponding to the item.
	 */
	virtual void getThumbnail( ThumbnailManager& thumbnailManager,
		VFolderItemDataPtr data, CImage& img ) = 0;


	/**
	 *	This abstract method lets the provider return a description suitable
	 *	for the status bar for an item.
	 *
	 *	@param data				Item data.
	 *	@param numItems			Number of items currently displayed, to be
	 *							appended/included in the text.
	 *	@param finished			If False, the text should mention that the
	 *							provider is still loading other items.
	 *	@return		Item description suitable for the status bar for an item.
	 */
	virtual const BW::wstring getDescriptiveText( VFolderItemDataPtr data,
		int numItems, bool finished ) = 0;


	/**
	 *	This abstract method lets returns the associated list provider so the
	 *	list can be updated with the same criteria as this tree folde provider.
	 *	An example of when this is useful is, when the user clicks on a folder
	 *	in the tree control, it will call this method on the vfolder provider.
	 *
	 *	@param data				Item data.
	 *	@param retInitIdString	Input and returns string ID to identify this
	 *							group of items, and to avoid refreshing the
	 *							list if we are still in the same path.
	 *	@param retListProvider	Input and returns the associated list provider.
	 *	@param retItemClicked	True if an item as opposed to a folder was
	 *							clicked. If so, the list needs to be refreshed.
	 *	@return		Item description suitable for the status bar for an item.
	 */
	virtual bool getListProviderInfo(
		VFolderItemDataPtr data,
		BW::wstring& retInitIdString,
		ListProviderPtr& retListProvider,
		bool& retItemClicked ) = 0;

protected:
	// Item type
	enum ItemGroup
	{
		GROUP_FOLDER,
		GROUP_ITEM
	};

	FolderTree* folderTree_;
	FilterHolder* filterHolder_;
	ListProviderPtr listProvider_;
};


/**
 *	This class implements a virtual folder, which contains subfolders and items
 *	as specified by its VFolderProvider.
 */
class VFolder : public ReferenceCount
{
public:
	/**
	 *	Constructor.
	 *
	 *	@param parent	Parent virtual folder, if any.
	 *	@param name		Name of the virtual folder, to be displayed.
	 *	@param item		HTREEITEM in the tree control
	 *	@param provider	Virtual folder provider which will provide the folder
	 *					with its items (see VFolderProvider).
	 *	@param expandable	Whether the tree item can be expanded or not.
	 *	@param sortSubFolders	Whether or not its subfolders should be sorted.
	 *	@param customItems	Additional application items not from the provider.
	 *	@param data		Custom application data.
	 *	@param subVFolders	Whether it allows virtual sub folders or not,
	 *						useful when loading nested virtual folders.
	 */
	VFolder( VFolderPtr parent,
				BW::wstring name, HTREEITEM item,
				VFolderProviderPtr provider,
				bool expandable,
				bool sortSubFolders,
				XmlItemVec* customItems,
				void* data,
				bool subVFolders ) :
		parent_( parent ),
		name_( name ),
		item_( item ),
		provider_( provider ),
		expandable_( expandable ),
		sortSubFolders_( sortSubFolders ),
		customItems_( customItems ),
		data_( data ),
		subVFolders_( subVFolders )
	{
	}

	// getters and setters
	BW::wstring getName() { return name_; }
	HTREEITEM getItem() { return item_; }
	VFolderProviderPtr getProvider() { return provider_; }
	bool isExpandable() { return expandable_; }
	XmlItemVec* getCustomItems() { return customItems_; }
	void* getData() { return data_; }
	void setSortSubFolders( bool sortSubFolders ) { sortSubFolders_ = sortSubFolders; }
	bool getSortSubFolders() { return sortSubFolders_; }
	bool subVFolders() { return subVFolders_; }

private:
	VFolderPtr parent_;
	BW::wstring name_;
	HTREEITEM item_;
	VFolderProviderPtr provider_;
	bool expandable_;
	bool sortSubFolders_;
	XmlItemVec* customItems_;
	void* data_;
	bool subVFolders_;
};


/**
 *	This class handles additional data per item, This defaul VFolderItemData
 *	implements a virtual folder.
 */
class VFolderItemData : public ReferenceCount
{
public:
	/**
	 *	Constructor.
	 *
	 *	@param provider	Virtual folder provider which will provide the folder
	 *					with its items (see VFolderProvider).
	 */
	VFolderItemData( VFolderProviderPtr provider,
						const AssetInfo& assetInfo,
						int group,
						bool expandable ) :
		provider_( provider ),
		assetInfo_( assetInfo ),
		group_( group ),
		expandable_( expandable ),
		custom_( false ),
		item_( 0 ),
		vfolder_( 0 )
	{
	}


	/**
	 *	Destructor.
	 */
	virtual ~VFolderItemData() {}


	/**
	 *	This method checks the passed in "data" and returns True if it's a
	 *	duplicate of "this" and should not be added to the tree.  For a VFolder
	 *	the default behaviour is to return false.
	 *
	 *	@param data		Other item to check.
	 *	@return		True to prevent the item to be added to the tree, false to
	 *				allow.  By default, a VFolder allows duplicates.
	 */
	virtual bool handleDuplicate( VFolderItemDataPtr data )
		{ return false; } // accepts duplicates


	// getters and setters
	virtual VFolderProviderPtr getProvider() { return provider_; }
	virtual AssetInfo& assetInfo() { return assetInfo_; }
	virtual int getGroup() { return group_; }
	virtual bool getExpandable() { return expandable_; }
	virtual bool isCustomItem() { return custom_; }
	virtual void isCustomItem( bool custom ) { custom_ = custom; }
	virtual bool isVFolder() const { return !!vfolder_; } // derived classes should return false here
	virtual HTREEITEM getTreeItem() { return item_; }
	virtual void setTreeItem( HTREEITEM item ) { item_ = item; }
	virtual VFolderPtr getVFolder() { return vfolder_; }
	virtual void setVFolder( VFolderPtr vfolder ) { vfolder_ = vfolder; }

private:
	VFolderProviderPtr provider_;
	AssetInfo assetInfo_;
	HTREEITEM item_;
	int group_;
	bool expandable_;
	bool custom_;
	VFolderPtr vfolder_;
};


/**
 *	This interface class is inherited by classes wanting to handle events sent
 *	by the FolterTree tree control.
 */
class FolderTreeEventHandler
{
public:
	virtual void folderTreeSelect( VFolderItemData* data ) = 0;
	virtual void folderTreeStartDrag( VFolderItemData* data ) = 0;
	virtual void folderTreeItemDelete( VFolderItemData* data ) = 0;
	virtual void folderTreeRightClick( VFolderItemData* data ) = 0;
	virtual void folderTreeDoubleClick( VFolderItemData* data ) = 0;
};



/**
 *	This class implements the FolderTree tree view control for the asset
 *	browser.
 */
class FolderTree : public CTreeCtrl, public ThumbnailUpdater
{
public:
	explicit FolderTree( ThumbnailManagerPtr thumbnailManager );
	virtual ~FolderTree();

	int addIcon( HICON image );
	int addBitmap( HBITMAP image );
	void removeImage( int index );

	void init();
	VFolderPtr addVFolder(
		const BW::wstring& displayName,
		VFolderProviderPtr provider,
		VFolderPtr parent, HICON icon, HICON iconSel,
		bool show,
		bool expandable,
		XmlItemVec* customItems,
		void* data,
		bool subVFolders );
	void removeVFolder( const BW::wstring& displayName, HTREEITEM curItem = TVI_ROOT );
	void removeVFolder( HTREEITEM item, HTREEITEM curItem = TVI_ROOT );
	void clear();

	void setSortVFolders( bool sort );
	void setSortSubFolders( bool sort );

	void setEventHandler( FolderTreeEventHandler* eventHandler );

	void setGroupIcons( int group, HICON icon, HICON iconSel );
	void setExtensionsIcons( BW::wstring extensions, HICON icon, HICON iconSel );

	VFolderPtr getVFolder( VFolderItemData* data );

	HTREEITEM getItem( VFolderItemData* data, HTREEITEM item = TVI_ROOT );

	BW::wstring getVFolderOrder( const BW::wstring orderStr = L"", HTREEITEM item = TVI_ROOT );
	void setVFolderOrder( const BW::wstring orderStr );
	void moveVFolder( VFolderPtr vf1, VFolderPtr vf2 );

	int getLevelCount( HTREEITEM item = TVI_ROOT );

	void freeSubtreeData( HTREEITEM item );
	void refreshVFolder( VFolderPtr vfolder );
	void refreshVFolders( VFolderProviderPtr provider = 0, HTREEITEM item = TVI_ROOT );
	VFolderPtr getVFolder( const BW::wstring& name, bool strict = true, HTREEITEM item = TVI_ROOT );
	void getVFolders( BW::vector<HTREEITEM>& items, HTREEITEM item = TVI_ROOT );
	void selectVFolder( const BW::wstring& name );
	typedef bool (*ItemTestCB)( HTREEITEM item, void* testData );
	VFolderPtr getVFolderCustom( ItemTestCB test, void* testData, bool strict = true, HTREEITEM item = TVI_ROOT );
	void selectVFolderCustom( ItemTestCB test, void* testData );

	bool isDragging();
	void updateDrag( int x, int y );
	void endDrag();

	bool updateItem( const AssetInfo& assetInfo, HTREEITEM item = TVI_ROOT );

	// ThumbnailUpdater interface implementation
	void thumbManagerUpdate( const BW::wstring& longText );

private:
	static const int IMAGE_SIZE = 16;

	/**
	 *	This struct stores indices to the icons corresponding to a group.
	 */
	struct GroupIcons
	{
		int group;
		int icon;
		int iconSel;
	};

	/**
	 *	This struct stores indices to the icons corresponding to a set of
	 *	filename extensions.
	 */
	struct ExtensionsIcons
	{
		BW::vector<BW::wstring> extensions;
		int icon;
		int iconSel;
	};

	bool initialised_;
	bool sortVFolders_;
	bool sortSubFolders_;
	ThumbnailManagerPtr thumbnailManager_;
	CImageList imgList_;
	int vfolderIcon_;
	int vfolderIconSel_;
	int itemIcon_;
	int itemIconSel_;
	int firstImageIndex_;
	BW::vector<VFolderItemDataPtr> itemDataHeap_;
	BW::vector<int> unusedImages_;
	BW::vector<ExtensionsIcons> extensionsIcons_;
	BW::vector<GroupIcons> groupIcons_;
	FolderTreeEventHandler* eventHandler_;
	CImageList* dragImgList_;
	controls::DragImage * dragImage_;
	bool dragging_;

	void setItemData( HTREEITEM item, VFolderItemDataPtr data );

	void buildTree( HTREEITEM parentItem, VFolderProviderPtr provider );

	bool isStockIcon( int icon );
	void getGroupIcons( int group, int& icon, int& iconSel, bool expandable );
	void getExtensionIcons( const BW::wstring& name, int& icon, int& iconSel );
	void sortSubTree( HTREEITEM item );
	bool expandItem( HTREEITEM item );

	afx_msg BOOL OnEraseBkgnd( CDC* pDC );
	afx_msg void OnPaint();
	afx_msg void OnKeyDown( UINT nChar, UINT nRepCnt, UINT nFlags );
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnSelChanged( NMHDR * pNotifyStruct, LRESULT* result );
	afx_msg void OnRightClick( NMHDR * pNotifyStruct, LRESULT* result );
	afx_msg void OnItemExpanding( NMHDR * pNotifyStruct, LRESULT* result );
	afx_msg void OnBeginDrag(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnLButtonDblClk( UINT nFlags, CPoint point );
	DECLARE_MESSAGE_MAP()
};

BW_END_NAMESPACE

#endif // FOLDER_TREE_HPP
