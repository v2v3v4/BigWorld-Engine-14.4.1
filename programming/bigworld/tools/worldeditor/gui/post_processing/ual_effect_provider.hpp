
#ifndef UAL_EFFECT_PROVIDER_HPP
#define UAL_EFFECT_PROVIDER_HPP


#include "ual/ual_dialog.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class implements the effect VFolder provider
 */
class EffectVFolderProvider : public VFolderProvider
{
public:
	explicit EffectVFolderProvider( const BW::string& thumb );
	~EffectVFolderProvider();
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
 *	This class implements the effect list provider
 */
class EffectListProvider : public ListProvider
{
public:
	explicit EffectListProvider( const BW::string& thumb );
	~EffectListProvider();

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
 *	This class implements the effect loader class, that creates the VFolder and
 *	list providers from a given section of the UAL's config file.
 */
class UalEffectVFolderLoader : public UalVFolderLoader
{
public:
	bool test( const BW::string& sectionName )
	{ 
		BW_GUARD;

		return sectionName == "PostProcessingEffects";
	}
	VFolderPtr load( UalDialog* dlg,
		DataSectionPtr section, VFolderPtr parent, DataSectionPtr customData,
		bool addToFolderTree );
};

BW_END_NAMESPACE

#endif // UAL_EFFECT_PROVIDER_HPP
