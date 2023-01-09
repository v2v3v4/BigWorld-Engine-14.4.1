
#ifndef VFOLDER_MULTI_PROVIDER_HPP
#define VFOLDER_MULTI_PROVIDER_HPP

#include "folder_tree.hpp"
#include "resmgr/datasection.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class derives from VFolderProvider to implement a VFolder provider
 *	that manages one or more sub-providers, allowing multiple asset sources to
 *	be shown under one UAL folder.
 */
class VFolderMultiProvider : public VFolderProvider
{
public:
	VFolderMultiProvider();
	virtual ~VFolderMultiProvider();

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
	void addProvider( VFolderProviderPtr provider );

private:
	typedef BW::vector<VFolderProviderPtr> ProvVec;
	ProvVec providers_;
	ProvVec::iterator iter_;
	VFolderItemData* parent_;
};

BW_END_NAMESPACE

#endif // VFOLDER_MULTI_PROVIDER_HPP
