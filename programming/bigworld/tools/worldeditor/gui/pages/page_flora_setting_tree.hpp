#ifndef PAGE_FLORA_SETTING_TREE_HPP
#define PAGE_FLORA_SETTING_TREE_HPP

#include "worldeditor/config.hpp"
#include "common/user_messages.hpp"
#include "asset_info.hpp"
#include "cstdmf/stringmap.hpp"
#include "list_file_provider.hpp"
#include "gizmo/undoredo.hpp"
#include "resmgr/datasection.hpp"


#define FLORA_SETTING_PAGE_XML	"resources/data/flora_setting_page.xml"

BW_BEGIN_NAMESPACE

class PageFloraSetting;
class FloraSettingSecondaryChildWnd;
class FloraSettingFloatBarWnd;
class FloraSettingTree;
class FloraSettingTreeSection;
class ListProvider;
typedef SmartPointer<ListProvider> ListProviderPtr;


#define FLORA_VISUAL_TEXTURE_FILTER_NAME  L"Valid Material"
/**
 *	This class search the flora visuals with specified texture.
 */
class ValidTextureFilterSpec : public FilterSpec
{
public:
	ValidTextureFilterSpec( const BW::wstring& name, bool active = false );
	bool setTextureName( const BW::string& textureName );
	virtual bool filter( const BW::wstring& str );
private:
	char validTextureName_[256];
};


/**
 *	Base class for the param of a tree view item control
 */
class FloraSettingTreeItemParam
{
public:
	FloraSettingTreeItemParam();
	FloraSettingTreeSection* pSection_;
	uint32 error_;
};

/**
 *	param of visual object item
 */
class VisualParam: public FloraSettingTreeItemParam
{
public:
	VisualParam();
	BW::string textureName_;
	int numVertices_;
};

/**
 *	param of visual generator item
 */
class VisualGeneratorParam: public FloraSettingTreeItemParam
{
public:
	VisualGeneratorParam();
	HTREEITEM	hDerivedVisualItem_;
};

/**
 *	param of ecotype item
 */
class  EcotypeParam: public FloraSettingTreeItemParam
{
public:
	EcotypeParam();
	HTREEITEM	hDerivedVisualGeneratorItem_;
};

/**
 *	param of ecotype item
 */
class  TextureParam: public FloraSettingTreeItemParam
{
public:
	TextureParam();
	void setExtension( const char* extention );

	#define MAX_EXTENSION 10
	char extension_[ MAX_EXTENSION ];
};


/**
 *	Base class for the section associated to a type of 
 *	item, it provides to a type of items the services: 
 *  read,write, edit. same type of items point to one section
 *  object.
 */
class FloraSettingTreeSection
{
public:
	enum FLORA_SETTING_ERROR
	{
		CORRECT = 0,
		MIN_ERROR = 1,
		VISUAL_TEXTURE_WRONG = 1,
		FILE_MISSING = 2,
		TERRAIN_USED_IN_OTHER = 4,
		MAX_ERROR = 4
	};


	HTREEITEM populate( DataSectionPtr pDataSection, HTREEITEM parentItem );

	HTREEITEM createDefaultItem( HTREEITEM parentItem, 
										HTREEITEM hInsertAfter = TVI_LAST );

	virtual void init( DataSectionPtr pConfigSection, 
										PageFloraSetting* pPageWnd );


	// for display UI 
	virtual ListProviderPtr prepareAssetListProvider( HTREEITEM hItem, 
								FilterHolder& filterHolder )	{ return NULL; }

	virtual ListProviderPtr prepareExistingResourceProvider( HTREEITEM hItem )	
								{ return NULL; }

	virtual bool			prepareCurDisplayImage( HTREEITEM hItem, 
								wchar_t* description, size_t szDescription,
								BW::string& imagePath )	{ return false; }

	virtual bool onDoubleClicked( HTREEITEM hItem )	{ return false; }
	virtual bool prepareEorrorMessage( HTREEITEM hItem, 
								BW::wstring& errorMessage );
	// This is only used by float bar window.
	virtual bool isFloatResource( HTREEITEM hItem )	{ return false; }

	virtual	bool isAssetConsumable( HTREEITEM hItem, 
							BW::vector<AssetInfo>& assets )	{ return false; }

	virtual	void consumeAssets( HTREEITEM hItem, BW::vector<AssetInfo>& assets ){}

	virtual void postChildAdded( HTREEITEM hParentItem,
					HTREEITEM hChildItem, bool moveOnly = false );

	virtual void postChildDeleted( HTREEITEM hParentItem, 
					FloraSettingTreeSection* pDeletedItemSection );

	virtual void preChildDeleted( HTREEITEM hParentItem, HTREEITEM hChildItem );


	virtual HTREEITEM createItem( HTREEITEM parentItem, 
						DataSectionPtr pDataSection = NULL, 
						HTREEITEM hInsertAfter = TVI_LAST );

	virtual bool isDeletable( HTREEITEM hItem );
	virtual bool isChildItemNameEditable( HTREEITEM hItem )	{ return false; }
	virtual bool validEditedChildItemName( HTREEITEM hParentItem, 
							const wchar_t* childItemName,
							wchar_t* error, size_t lenError )	{ return false;}

	virtual DataSectionPtr save( DataSectionPtr pParentDataSection, 
					HTREEITEM hItem, FloraSettingTreeSection* pParentSection, 
					BW::vector<HTREEITEM>* pHighLightVisuals = NULL );

	virtual FloraSettingTreeSection* childSection( DataSectionPtr pChildDataSection );

	bool	isEcotypeVisualDependent( bool includeEcotypeSection = false, 
					bool* pIndexPathDependent = NULL );


	// Section name:
	class SectionName
	{
	public:
		BW::string name_;
		//if true, then it's name is the data section name:
		//ie. "generator" in <generator> visual </generator>
		//if false, by section's value (== asString()). 
		//ie. "visual" in <generator> visual </generator>
		bool nameByDataSectionName_;
	};

	class ChildSection
	{
	public:
		BW::string& name()	{ return sectionName_.name_;}

		bool& nameByDataSectionName() 
				{ return sectionName_.nameByDataSectionName_; }

		bool required_;
		bool displayInPropertyWindow_;
	private:
		SectionName sectionName_;
	};

	BW::string& name()	{ return sectionName_.name_; }

	bool& nameByDataSectionName() 
			{ return sectionName_.nameByDataSectionName_; }

	const BW::vector< ChildSection >& childSections()	
			{ return childSections_; }

protected:
	PageFloraSetting* pPageWnd_;

	SectionName sectionName_;
	BW::vector< ChildSection > childSections_;

	bool isValidChildSection( const BW::string& name, bool* 
			pRetNameBySectionName = NULL, bool* pRetIsRequired =NULL );

	virtual void createItemParam( HTREEITEM hItem );
	void deleteItemParam( HTREEITEM hItem );
	FloraSettingTree& tree();


	friend class FloraSettingTree;
};


////////////////General function Sections/////////////////////////////
/**
 *	Base class for the sections that in the asset window
 *	gives a list of things to drag from.
 */
class ListProviderSection: public FloraSettingTreeSection
{
public:
	enum MouseDragAction
	{
		addToChildrenNoRepeat,
		addToChildrenRepeat,
		replaceChild
	};

	virtual	void consumeAssets( HTREEITEM hItem, BW::vector<AssetInfo>& assets );
	virtual void init( DataSectionPtr pConfigSection, PageFloraSetting* pPageWnd );

	virtual bool generateItemText( const AssetInfo& asset, wchar_t* text, 
					size_t maxText, bool forTooltip = false ) = 0;

	MouseDragAction mouseAction()	{ return mouseDragAction_; }

	virtual void postChildAdded( HTREEITEM hParentItem, HTREEITEM hChildItem, 
					bool moveOnly = false );

	virtual void postChildDeleted( HTREEITEM hParentItem, 
					FloraSettingTreeSection* pDeletedItemSection );


protected:
	virtual FloraSettingTreeSection* childSection( AssetInfo& asset ) = 0;

	ListProviderPtr assetListProvider_;
	ListProviderPtr existResourceListProvider_;
	MouseDragAction mouseDragAction_;
};


/**
 *  This class scans files for the list of things on the asset window
 */
class ListFileProviderSection: public ListProviderSection
{
public:
	virtual void init( DataSectionPtr pConfigSection, 
					PageFloraSetting* pPageWnd );

	virtual	bool isAssetConsumable( HTREEITEM hItem, 
					BW::vector<AssetInfo>& assetsInfo );

	virtual bool generateItemText( const AssetInfo& asset, 
					wchar_t* text, size_t maxText, bool forTooltip = false  );

	virtual ListProviderPtr prepareAssetListProvider( HTREEITEM hItem, 
					FilterHolder& filterHolder );

	virtual ListProviderPtr prepareExistingResourceProvider( HTREEITEM hItem );

protected:
	virtual FloraSettingTreeSection* childSection( AssetInfo& asset );

	BW::vector<BW::wstring> paths_;
	BW::vector<BW::wstring> extensions_;
	BW::vector<BW::wstring> includeFolders_;
	BW::vector<BW::wstring> excludeFolders_;

};

/**
 *  This class read the content from flora_setting_page.xml
 *  and list them in the asset window.
 */
class ListXmlProviderSection: public ListProviderSection
{
public:
	virtual void init( DataSectionPtr pConfigSection, 
					PageFloraSetting* pPageWnd );

	virtual	bool isAssetConsumable( HTREEITEM hItem, 
					BW::vector<AssetInfo>& assetsInfo );

	virtual bool generateItemText( const AssetInfo& asset, 
					wchar_t* text, size_t maxText, bool forTooltip = false  );

	virtual ListProviderPtr prepareAssetListProvider( HTREEITEM hItem, 
					FilterHolder& filterHolder );

protected:
	virtual FloraSettingTreeSection* childSection( AssetInfo& asset );

private:
	BW::wstring path_;
};


/**
 *  This class provide a float drag bar in the properties window.
 */
class FloatSection : public FloraSettingTreeSection
{
public:
	virtual void init( DataSectionPtr pConfigSection, 
					PageFloraSetting* pPageWnd );

	virtual HTREEITEM createItem( HTREEITEM parentItem, 
						DataSectionPtr pDataSection = NULL, 
						HTREEITEM hInsertAfter = TVI_LAST );

	virtual DataSectionPtr save( DataSectionPtr pParentDataSection,  
						HTREEITEM hItem, FloraSettingTreeSection* pParentSection,
						BW::vector<HTREEITEM>* pHighLightVisuals = NULL );

	void getNameAndValue( HTREEITEM hItem, wchar_t* pPropertyNameRet, size_t retSize, float& value);
	void setNameAndValue( HTREEITEM hItem, const BW::string& propertyName, float value );

	virtual bool isFloatResource( HTREEITEM hItem )	{ return true; }

	float min()	{ return min_; }
	float max()	{ return max_; }

private:
	
	float	min_;
	float	max_;
	float	default_;
};

/**
 *  This section can have only one type of child section, 
 *	and the child section's name inside flora.xml has to 
 *  be specified by the user(ie. ecotype like <grass>, <stone> )
 */
class AddNewItemSection: public FloraSettingTreeSection
{
public:
	static const size_t NAME_LENGTH = 32;

	virtual FloraSettingTreeSection* childSection( 
					DataSectionPtr pChildDataSection );

	virtual bool isChildItemNameEditable( HTREEITEM hItem )	{ return true; }
	virtual bool validEditedChildItemName( HTREEITEM hParentItem, 
											const wchar_t* childItemName,
											wchar_t* error, size_t lenError );
	HTREEITEM newDefaultChildItem( HTREEITEM parentItem );
};


/**
 *  This section's items are only for display, uneditable
 */
class UnEditableSection : public FloraSettingTreeSection
{
public:
	virtual HTREEITEM createItem( HTREEITEM parentItem, 
						DataSectionPtr pDataSection = NULL,
						HTREEITEM hInsertAfter = TVI_LAST );

	virtual FloraSettingTreeSection* childSection( 
						DataSectionPtr pChildDataSection );

	virtual bool isDeletable( HTREEITEM hItem );
};

///////////The following Sections are more properties specific//////////////////

/**
 *  This section handle visual object items.
 */
class VisualSection: public FloraSettingTreeSection
{
public:
	virtual void createItemParam( HTREEITEM hItem );

	virtual bool prepareCurDisplayImage( HTREEITEM hItem, wchar_t* description, 
					size_t szDescription, BW::string& imagePath );

	virtual DataSectionPtr save( DataSectionPtr pParentDataSection, 
								HTREEITEM hItem, 
								FloraSettingTreeSection* pParentSection, 
								BW::vector<HTREEITEM>* pHighLightVisuals = NULL );
};

/**
 *  This section handle visual generator items.
 */
class VisualGeneratorSection: public ListFileProviderSection
{
public:
	virtual void createItemParam( HTREEITEM hItem );

	virtual void postChildAdded( HTREEITEM hParentItem, 
					HTREEITEM hChildItem, bool moveOnly = false );

	virtual void postChildDeleted( HTREEITEM hParentItem, 
					FloraSettingTreeSection* pDeletedItemSection );

	virtual ListProviderPtr prepareAssetListProvider( HTREEITEM hItem, 
					FilterHolder& filterHolder );

	virtual bool prepareCurDisplayImage( HTREEITEM hItem, wchar_t* description, 
					size_t szDescription, BW::string& imagePath );

	void validateTexture(  HTREEITEM hItem, const BW::string& validTextureName );
	bool getVisualTexture( HTREEITEM hItem, BW::string& textureName );
	bool getEcotypeVisualTexture( HTREEITEM hItem,BW::string& textureName );
	void updateVisualTexture( HTREEITEM hItem, bool updateEcotype = true );
};


/**
 *  This section handle terrain texture items.
 */
class TextureSection: public FloraSettingTreeSection
{
public:
	virtual void createItemParam( HTREEITEM hItem );
	virtual bool prepareCurDisplayImage( HTREEITEM hItem, wchar_t* description, 
					size_t szDescription, BW::string& imagePath );

	virtual bool onDoubleClicked( HTREEITEM hItem );
};


/**
 *  This section handle ecotype items (ie. <grass>, <stone>).
 */
class EcotypeSection: public ListFileProviderSection
{
public:
	virtual void createItemParam( HTREEITEM hItem );

	virtual void postChildAdded( HTREEITEM hParentItem, 
					HTREEITEM hChildItem, bool moveOnly = false );

	virtual void preChildDeleted( HTREEITEM hParentItem, HTREEITEM hChildItem );

	virtual bool prepareCurDisplayImage( HTREEITEM hItem, wchar_t* description, 
					size_t szDescription, BW::string& imagePath );

	void updateVisualTexture( HTREEITEM hItem );
	bool getVisualTexture( HTREEITEM hItem, BW::string& textureName );

};


/**
 *  This class is the tree view window which
 *	lists the content of flora.xml and handle
 *	editing.
 */
class FloraSettingTree : public CTreeCtrl
{
	DECLARE_DYNAMIC(FloraSettingTree)

public:

	FloraSettingTree( PageFloraSetting* pPageWnd );
	virtual ~FloraSettingTree();

	void init( DataSectionPtr pDataSection );
	FloraSettingTreeSection* section( const BW::string& name, bool isSectionName );
	FloraSettingTreeSection* getItemSection( HTREEITEM hItem );
	void populate( DataSectionPtr pFloraXMLDataSect );

	void save( DataSectionPtr pFloraXMLDataSect, 
					BW::vector<HTREEITEM>* pHighLightVisuals = NULL );
	bool moveItem( HTREEITEM hItem, int step, bool addUndo = true );
	bool deleteItem( HTREEITEM hItem, bool popupAskDlg = false, bool addUndo = true );
	void expand( HTREEITEM hItem, bool recursive = false );

	void markItemWrong( HTREEITEM hItem, 
				FloraSettingTreeSection::FLORA_SETTING_ERROR error );

	void unmarkItemWrong( HTREEITEM hItem, 
				FloraSettingTreeSection::FLORA_SETTING_ERROR error );

	HTREEITEM findChildItem( HTREEITEM hItem, const wchar_t* itemName );

	HTREEITEM findFirstParentBySection( HTREEITEM hStartItem, 
				FloraSettingTreeSection* pSection );

	void findChildItemsBySection( HTREEITEM hStartItem, 
								FloraSettingTreeSection* pSection, 
								BW::vector<HTREEITEM>& retItems, 
								bool lookForChildOfFoundItem = false );
	bool createNewEcotype();
	void highLightVisuals( HTREEITEM hItem, bool highLight );

	HTREEITEM findTextureItem( const wchar_t* wTextureName, 
								HTREEITEM hStartEcotypeItem = NULL, 
								HTREEITEM hEndEcotypeItem = NULL, 
								bool reverseSearch = false );
	size_t getSiblingIndex( HTREEITEM hItem );
	bool getIndexPath( HTREEITEM hItem, size_t* indexPath, size_t& len );
	HTREEITEM findItem( size_t* indexPath, size_t len );
	size_t childCount( HTREEITEM hParentItem );

protected:
	afx_msg void OnSelChanged( NMHDR * pNotifyStruct, LRESULT* result );
	afx_msg void OnKeyDown( NMHDR * pNotifyStruct, LRESULT* result );
	afx_msg void OnBeginLabelEdit(NMHDR *nmhdr, LRESULT *result);
	afx_msg void OnEndLabelEdit(NMHDR* nmhdr, LRESULT* result);
	DECLARE_MESSAGE_MAP()

private:

	bool getHighlightVisualIndexPath( HTREEITEM hItem, BW::list<int>& indexPath );

	PageFloraSetting* pPageWnd_;
	OrderedStringMap<FloraSettingTreeSection*> sectionMap_;
	CImageList imgList_;
};

//////////////Undo/redo operations///////////

class TreeItemOperation : public UndoRedo::Operation
{
public:
	TreeItemOperation( FloraSettingTree* pTree, HTREEITEM hItem );
	HTREEITEM parentItem();
	HTREEITEM thisItem();
protected:
#define MAX_INDEX_PATH 50
	size_t indexPath_[ MAX_INDEX_PATH ];
	size_t indexPathLen_;
	FloraSettingTree* pTree_;

};


class ItemExistenceOperation : public TreeItemOperation
{
public:
	ItemExistenceOperation( bool deletion, FloraSettingTree* pTree, HTREEITEM hItem );
protected:
	virtual void undo();
	virtual bool iseq( const UndoRedo::Operation & oth ) const
	{
		BW_GUARD;
		// these operations never replace each other
		return false;
	}
private:
	bool deletion_;
	DataSectionPtr pData_;
	FloraSettingTreeSection* pSection_;

};


class MoveItemUpDownOperation : public TreeItemOperation
{
public:
	MoveItemUpDownOperation( FloraSettingTree* pTree, HTREEITEM hItem, 
														size_t oldLeaveIndex );
protected:
	virtual void undo();
	virtual bool iseq( const UndoRedo::Operation & oth ) const
	{
		BW_GUARD;
		// these operations never replace each other
		return false;
	}
private:
	size_t oldLeaveIndex_;
};


class FloatValueOperation : public TreeItemOperation
{
public:
	FloatValueOperation( FloraSettingTree* pTree, HTREEITEM hItem,
						float oldValue, FloraSettingFloatBarWnd* pFloatBarWnd );
protected:
	virtual void undo();
	virtual bool iseq( const UndoRedo::Operation & oth ) const
	{
		BW_GUARD;
		// these operations never replace each other
		return false;
	}
private:
	float oldValue_;
	FloraSettingFloatBarWnd* pFloatBarWnd_;
};


class ChangeNameOperation : public TreeItemOperation
{
public:
	ChangeNameOperation( FloraSettingTree* pTree, HTREEITEM hItem, LPCWSTR pOldName );
protected:
	virtual void undo();
	virtual bool iseq( const UndoRedo::Operation & oth ) const
	{
		BW_GUARD;
		// these operations never replace each other
		return false;
	}
private:
	wchar_t oldName_[ AddNewItemSection::NAME_LENGTH ];
};

BW_END_NAMESPACE

#endif // PAGE_FLORA_SETTING_TREE_HPP