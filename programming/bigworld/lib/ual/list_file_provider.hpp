
#ifndef LIST_FILE_PROVIDER_HPP
#define LIST_FILE_PROVIDER_HPP

#include "list_cache.hpp"
#include "filter_spec.hpp"
#include "smart_list_ctrl.hpp"
#include "atlimage.h"
#include "cstdmf/concurrency.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class derives from ListProvider to implement a virtual list provider
 *	that scans for files.
 */
class ListFileProvider : public ListProvider
{
public:
	enum ListFileProvFlags
	{
		// Default behaviout
		LISTFILEPROV_DEFAULT = 0,
		
		// Don't recurse into subfolders
		LISTFILEPROV_DONTRECURSE = 1,

		// Don't filter out DDSs if a matching TGA/JPG/PNG/BMP file exists.
		LISTFILEPROV_DONTFILTERDDS = 2,

		// Use only DDSs if matching TGA/JPG/PNG/BMP file exists,opposite to LISTFILEPROV_DONTFILTERDDS
		LISTFILEPROV_FILTERNONDDS = 4
	};

	explicit ListFileProvider( const BW::wstring& thumbnailPostfix );
	virtual ~ListFileProvider();

	virtual void init(
		const BW::wstring& type,
		const BW::wstring& paths,
		const BW::wstring& extensions,
		const BW::wstring& includeFolders,
		const BW::wstring& excludeFolders,
		int flags = LISTFILEPROV_DEFAULT );

	virtual void init(
		const BW::wstring& type,
		const BW::vector<BW::wstring>& paths,
		const BW::vector<BW::wstring>& extensions,
		const BW::vector<BW::wstring>& includeFolders,
		const BW::vector<BW::wstring>& excludeFolders,
		int flags = LISTFILEPROV_DEFAULT );

	virtual void refresh();

	virtual bool finished();

	virtual int getNumItems();

	virtual	const AssetInfo getAssetInfo( int index );
	virtual void getThumbnail( ThumbnailManager& manager,
								int index, CImage& img, int w, int h,
								ThumbnailUpdater* updater );

	virtual void filterItems();

	// additional interface
	virtual void setExtensionsIcon( BW::wstring extensions, HICON icon );

	// makes the thread sleep for 50 msecs each 'msec' it uses
	virtual void setThreadYieldMsec( int msec );
	virtual int getThreadYieldMsec();

	// if greater than 0, thread priority will be above normal
	// if less than 0, thread priority will be below normal
	virtual void setThreadPriority( int priority );
	virtual int getThreadPriority();

protected:
	/**
	 *	This internal class holds information about a file found during
	 *	scanning.
	 */
	class ListItem : public SafeReferenceCount
	{
	public:
		BW::wstring fileName;		// actually the full path
		BW::wstring dissolved;		// dissolved filename
		BW::wstring title;			// file name only, no path
	};

	typedef SmartPointer<ListItem> ListItemPtr;
	BW::vector<ListItemPtr> items_;

private:
	static const int VECTOR_BLOCK_SIZE = 1000;


	/**
	 *	This simple struct just associates a list of extensions to an image.
	 */
	struct ExtensionsIcons
	{
		BW::vector<BW::wstring> extensions;
		HICON icon;
	};




	bool hasImages_;
	BW::vector<ListItemPtr> searchResults_;
	typedef BW::vector<ListItemPtr>::iterator ItemsItr;
	BW::vector<BW::wstring> paths_;
	BW::vector<BW::wstring> extensions_;
	BW::vector<BW::wstring> includeFolders_;
	BW::vector<BW::wstring> excludeFolders_;
	BW::vector<ExtensionsIcons> extensionsIcons_;
	BW::wstring thumbnailPostfix_;
	int flags_;
	BW::wstring type_;

	void clearItems();
	void clearThreadItems();

	HICON getExtensionIcons( const BW::wstring& name );

	// load thread stuff
	SimpleThread* thread_;
	SimpleMutex mutex_;
	SimpleMutex threadMutex_;
	bool threadWorking_;
	BW::vector<ListItemPtr> threadItems_;
	BW::vector<ListItemPtr> tempItems_;
	clock_t flushClock_;
	int threadFlushMsec_;
	clock_t yieldClock_;
	int threadYieldMsec_;
	int threadPriority_;

	static bool s_comparator( const ListItemPtr& a, const ListItemPtr& b );
	static void s_startThread( void* provider );
	void startThread();
	void stopThread();
	void setThreadWorking( bool working );
	bool getThreadWorking();

	void fillFiles( const CString& path );
	void removeDuplicateFileNames();
	void removeRedundantDdsFiles();
	void removeRedundantNonDdsFiles();
	void flushThreadBuf();
	bool isFiltering();
};

typedef SmartPointer<ListFileProvider> ListFileProviderPtr;


/**
 *  This class provide a fixed list of files specified in initialization.
 */
class FixedListFileProvider : public ListFileProvider
{
public:
	explicit FixedListFileProvider( const BW::wstring& thumbnailPostfix );
	void init( const BW::vector< BW::wstring >& paths );

};
typedef SmartPointer<FixedListFileProvider> FixedListFileProviderPtr;

BW_END_NAMESPACE

#endif // LIST_FILE_PROVIDER_HPP
