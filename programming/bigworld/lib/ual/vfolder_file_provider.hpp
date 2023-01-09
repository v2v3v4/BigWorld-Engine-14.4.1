
#ifndef VFOLDER_FILE_PROVIDER_HPP
#define VFOLDER_FILE_PROVIDER_HPP

#include <stack>
#include "folder_tree.hpp"

BW_BEGIN_NAMESPACE

/// Files VFolder provider initialisation flags.
enum FILETREE_FLAGS
{
	FILETREE_SHOWSUBFOLDERS = 1,	// Show subfolders
	FILETREE_SHOWFILES = 2,			// Show files
	FILETREE_DONTRECURSE = 4		// Just one recursion level
};


/**
 *	This class derives from VFolderItemData and keeps information about a files
 *	VFolder item.
 */
class VFolderFileItemData : public VFolderItemData
{
public:
	// Files VFolder item type, whether it's a file or a folder.
	enum ItemDataType
	{
		ITEMDATA_FOLDER,
		ITEMDATA_FILE
	};

	VFolderFileItemData(
		VFolderProviderPtr provider,
		ItemDataType type,
		const AssetInfo& assetInfo,
		int group,
		bool expandable );
	virtual ~VFolderFileItemData();

	virtual bool isVFolder() const { return false; }

	virtual bool handleDuplicate( VFolderItemDataPtr data );

	virtual ItemDataType getItemType() { return type_; }

private:
	ItemDataType type_;
};


/**
 *	This class derives from VFolderProvider to implement a provider that scans
 *	a series of paths for files.
 */
class VFolderFileProvider : public VFolderProvider
{
public:
	// Files VFolder item group, whether it's a file or a folder.
	enum FileGroup
	{
		FILEGROUP_FOLDER = VFolderProvider::GROUP_FOLDER,
		FILEGROUP_FILE = VFolderProvider::GROUP_ITEM
	};

	VFolderFileProvider();
	VFolderFileProvider(
		const BW::wstring& thumbnailPostfix,
		const BW::wstring& type,
		const BW::wstring& paths,
		const BW::wstring& extensions,
		const BW::wstring& includeFolders,
		const BW::wstring& excludeFolders,
		int flags );
	virtual ~VFolderFileProvider();

	virtual void init(
		const BW::wstring& type,
		const BW::wstring& paths,
		const BW::wstring& extensions,
		const BW::wstring& includeFolders,
		const BW::wstring& excludeFolders,
		int flags );

	virtual bool startEnumChildren( const VFolderItemDataPtr parent );
	virtual VFolderItemDataPtr getNextChild( ThumbnailManager& thumbnailManager, CImage& img );
	virtual void getThumbnail( ThumbnailManager& thumbnailManager, VFolderItemDataPtr data, CImage& img );
	virtual const BW::wstring getDescriptiveText( VFolderItemDataPtr data, int numItems, bool finished );
	virtual bool getListProviderInfo(
		VFolderItemDataPtr data,
		BW::wstring& retInitIdString,
		ListProviderPtr& retListProvider,
		bool& retItemClicked );

	virtual const BW::wstring getType();
	virtual int getFlags();
	virtual const BW::vector<BW::wstring>& getPaths();
	virtual const BW::vector<BW::wstring>& getExtensions();
	virtual const BW::vector<BW::wstring>& getIncludeFolders();
	virtual const BW::vector<BW::wstring>& getExcludeFolders();
	virtual const BW::wstring getPathsString();
	virtual const BW::wstring getExtensionsString();
	virtual const BW::wstring getIncludeFoldersString();
	virtual const BW::wstring getExcludeFoldersString();

private:
	/**
	 *	This internal class simply stores information for scanning a list of
	 *	paths for files and folders.  This is needed because of the nature of 
	 *	VFolder providers, that are asked for one item at a time, so we need to
	 *	store where we're at.
	 */
	class FileFinder : public ReferenceCount
	{
	public:
		CFileFind files;
		BW::vector<BW::wstring> paths;
		BW::vector<BW::wstring>::iterator path;
		BW::vector<BW::wstring>::iterator pathEnd;
		bool eof;
	};

	BW::wstring type_;
	int flags_;
	typedef SmartPointer<FileFinder> FileFinderPtr;
	std::stack<FileFinderPtr> finderStack_;
	BW::wstring thumbnailPostfix_;
	BW::vector<BW::wstring> paths_;
	BW::vector<BW::wstring> extensions_;
	BW::vector<BW::wstring> includeFolders_;
	BW::vector<BW::wstring> excludeFolders_;

	void popFinderStack();
	void clearFinderStack();
	FileFinderPtr topFinderStack();
};

BW_END_NAMESPACE

#endif // VFOLDER_FILE_PROVIDER_HPP
