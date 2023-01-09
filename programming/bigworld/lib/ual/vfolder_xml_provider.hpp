
#ifndef VFOLDER_XML_PROVIDER_HPP
#define VFOLDER_XML_PROVIDER_HPP

#include "folder_tree.hpp"
#include "resmgr/datasection.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class derives from VFolderItemData and keeps information about an XML
 *	VFolder item.
 */
class VFolderXmlItemData : public VFolderItemData
{
public:
	VFolderXmlItemData(
		VFolderProviderPtr provider,
		const AssetInfo& assetInfo,
		int group,
		bool expandable,
		const BW::wstring& thumb ) :
		VFolderItemData( provider, assetInfo, group, expandable ),
		thumb_( thumb )
	{}

	virtual bool isVFolder() const { return false; }
	virtual BW::wstring thumb() { return thumb_; }

private:
	BW::wstring thumb_;
};


/**
 *	This class derives from VFolderProvider to implement a provider that gets
 *	its items from an XML list.
 */
class VFolderXmlProvider : public VFolderProvider
{
public:
	// XML VFolder item group, only one at the moment.
	enum FileGroup
	{
		XMLGROUP_ITEM = VFolderProvider::GROUP_ITEM
	};

	VFolderXmlProvider();
	VFolderXmlProvider( const BW::wstring& path );
	virtual ~VFolderXmlProvider();

	virtual void init( const BW::wstring& path );

	virtual bool startEnumChildren( const VFolderItemDataPtr parent );
	virtual VFolderItemDataPtr getNextChild( ThumbnailManager& thumbnailManager, CImage& img );
	virtual void getThumbnail( ThumbnailManager& thumbnailManager, VFolderItemDataPtr data, CImage& img );

	virtual const BW::wstring getDescriptiveText( VFolderItemDataPtr data, int numItems, bool finished );
	virtual bool getListProviderInfo(
		VFolderItemDataPtr data,
		BW::wstring& retInitIdString,
		ListProviderPtr& retListProvider,
		bool& retItemClicked );

	// additional interface
	virtual BW::wstring getPath();
	virtual bool getSort();
private:
	BW::wstring path_;
	bool sort_;
	typedef SmartPointer<AssetInfo> ItemPtr;
	BW::vector<ItemPtr> items_;
	BW::vector<ItemPtr>::iterator itemsItr_;
};


BW_END_NAMESPACE

#endif // VFOLDER_XML_PROVIDER_HPP
