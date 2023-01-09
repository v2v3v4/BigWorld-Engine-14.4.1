
#ifndef UAL_RENDER_TARGET_PROVIDER_HPP
#define UAL_RENDER_TARGET_PROVIDER_HPP


#include "ual/ual_dialog.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class implements the render target VFolder provider
 */
class RenderTargetVFolderProvider : public VFolderProvider
{
public:
	explicit RenderTargetVFolderProvider( const BW::string& thumb );
	~RenderTargetVFolderProvider();
	bool startEnumChildren( const VFolderItemDataPtr parent );
	VFolderItemDataPtr getNextChild( ThumbnailManager& manager, CImage& img );
	void getThumbnail( ThumbnailManager& manager, VFolderItemDataPtr data, CImage& img );
	const BW::wstring getDescriptiveText( VFolderItemDataPtr data, int numItems, bool finished );
	bool getListProviderInfo(
		VFolderItemDataPtr data,
		BW::wstring& retInitIdString,
		ListProviderPtr& retListProvider,
		bool& retItemClicked );

private:
	int index_;
	BW::string thumb_;
	CImage img_;
};


/**
 *	This class implements the render target list provider
 */
class RenderTargetListProvider : public ListProvider
{
public:
	explicit RenderTargetListProvider( const BW::string& thumb );
	~RenderTargetListProvider();

	void refresh();
	bool finished();
	int getNumItems();
	const AssetInfo getAssetInfo( int index );
	void getThumbnail( ThumbnailManager& manager,
		int index, CImage& img, int w, int h, ThumbnailUpdater* updater );
	void filterItems();

private:
	BW::vector<AssetInfo> searchResults_;
	BW::string thumb_;
	CImage img_;
};


/**
 *	This class implements the render target loader class, that creates the VFolder and
 *	list providers from a given section of the UAL's config file.
 */
class UalRenderTargetVFolderLoader : public UalVFolderLoader
{
public:
	bool test( const BW::string& sectionName )
	{ 
		BW_GUARD;

		return sectionName == "PostProcessingRenderTargets";
	}
	VFolderPtr load( UalDialog* dlg,
		DataSectionPtr section, VFolderPtr parent, DataSectionPtr customData,
		bool addToFolderTree );
};

BW_END_NAMESPACE
#endif // UAL_RENDER_TARGET_PROVIDER_HPP
