
#ifndef UAL_PHASE_PROVIDER_HPP
#define UAL_PHASE_PROVIDER_HPP


#include "ual/ual_dialog.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class implements the phase VFolder provider
 */
class PhaseVFolderProvider : public VFolderProvider
{
public:
	explicit PhaseVFolderProvider( const BW::string& thumb );
	~PhaseVFolderProvider();
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
 *	This class implements the phase list provider
 */
class PhaseListProvider : public ListProvider
{
public:
	explicit PhaseListProvider( const BW::string& thumb );
	~PhaseListProvider();

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
 *	This class implements the phase loader class, that creates the VFolder and
 *	list providers from a given section of the UAL's config file.
 */
class UalPhaseVFolderLoader : public UalVFolderLoader
{
public:
	bool test( const BW::string& sectionName )
	{ 
		BW_GUARD;

		return sectionName == "PostProcessingPhases";
	}
	VFolderPtr load( UalDialog* dlg,
		DataSectionPtr section, VFolderPtr parent, DataSectionPtr customData,
		bool addToFolderTree );
};

BW_END_NAMESPACE

#endif // UAL_PHASE_PROVIDER_HPP
