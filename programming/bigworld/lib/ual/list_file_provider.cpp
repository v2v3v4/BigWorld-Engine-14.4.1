
#include "pch.hpp"
#include <algorithm>
#include "cstdmf/bw_vector.hpp"
#include "cstdmf/bw_string.hpp"
#include <time.h>
#include "list_file_provider.hpp"
#include "thumbnail_manager.hpp"

#include "common/string_utils.hpp"
#include "resmgr/bwresource.hpp"
#include "cstdmf/string_utils.hpp"
#include "cstdmf/debug.hpp"

DECLARE_DEBUG_COMPONENT( 0 );

BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 *
 *	@param thumbnailPostfix	Posfix used for thumbnail image files.
 */
ListFileProvider::ListFileProvider( const BW::wstring& thumbnailPostfix ) :
	hasImages_( false ),
	thread_( 0 ),
	threadWorking_( false ),
	threadFlushMsec_( 200 ),
	threadYieldMsec_( 0 ),
	threadPriority_( 0 ),
	thumbnailPostfix_( thumbnailPostfix ),
	flags_( LISTFILEPROV_DEFAULT )
{
	BW_GUARD;

	init( L"", L"", L"", L"", L"", flags_ );
}


/**
 *	Destructor.
 */
ListFileProvider::~ListFileProvider()
{
	BW_GUARD;

	stopThread();

	clearItems();
	clearThreadItems();
}


/**
 *	This static function extracts paths separated by comma or semicolons and
 *	returns them in a vector.
 *
 *	@param paths	Comma or semicolon separated list of paths.
 *	@param subPaths	Return vector with the paths.
 */
void extractVector( const BW::wstring& paths, BW::vector< BW::wstring > & subPaths )
{
	BW_GUARD;

	BW::wstring pathsL = paths;
	std::replace( pathsL.begin(), pathsL.end(), L'/', L'\\' );
	bw_tokenise( pathsL, L",;", subPaths );
}


/**
 *	This method initialises the provider.
 *
 *	@param type		Type string from the config file.
 *	@param paths	Comma or semicolon separated list of paths to scan.
 *	@param extensions	File extensions to include.
 *	@param includeFolders	Folders to include.
 *	@param excludeFolders	Folders to exclude.
 *	@param flags	Flags, see the "ListFileProvFlags" in the header file.	
 */
void ListFileProvider::init(
	const BW::wstring& type,
	const BW::vector<BW::wstring>& paths,
	const BW::vector<BW::wstring>& extensions,
	const BW::vector<BW::wstring>& includeFolders,
	const BW::vector<BW::wstring>& excludeFolders,
	int flags )
{
	BW_GUARD;

	stopThread();

	// member variables
	type_ = type;

	flags_ = flags;

	paths_.clear();
	extensions_.clear();
	includeFolders_.clear();
	excludeFolders_.clear();

	paths_ = paths;
	extensions_ = extensions;
	hasImages_ = false;
	for( BW::vector<BW::wstring>::iterator i = extensions_.begin();
		i != extensions_.end(); ++i )
	{
		if ( (*i) == L"dds" )
		{
			hasImages_ = true;
			break;
		}
	}
	includeFolders_ = includeFolders;
	excludeFolders_ = excludeFolders;

	StringUtils::filterSpecVectorT( paths_, excludeFolders_ );

	// clear items and start file-seeking thread
	clearItems();

	if ( !paths_.empty() ) 
		startThread();
}


/**
 *	This method initialises the provider.
 *
 *	@param type		Type string from the config file.
 *	@param paths	Comma or semicolon separated list of paths to scan.
 *	@param extensions	File extensions to include.
 *	@param includeFolders	Folders to include.
 *	@param excludeFolders	Folders to exclude.
 *	@param flags	Flags, see the "ListFileProvFlags" in the header file.	
 */
void ListFileProvider::init(
	const BW::wstring& type,
	const BW::wstring& paths,
	const BW::wstring& extensions,
	const BW::wstring& includeFolders,
	const BW::wstring& excludeFolders,
	int flags )
{
	BW_GUARD;

	stopThread();

	// member variables
	type_ = type;

	flags_ = flags;

	paths_.clear();
	extensions_.clear();
	includeFolders_.clear();
	excludeFolders_.clear();

	extractVector( paths, paths_ );

	BW::wstring extL = extensions;
	StringUtils::toLowerCaseT( extL );
	extractVector( extL, extensions_ );
	hasImages_ = false;
	for( BW::vector<BW::wstring>::iterator i = extensions_.begin();
		i != extensions_.end(); ++i )
	{
		if ( (*i) == L"dds" )
		{
			hasImages_ = true;
			break;
		}
	}

	extractVector( includeFolders, includeFolders_ );
	extractVector( excludeFolders, excludeFolders_ );

	StringUtils::filterSpecVectorT( paths_, excludeFolders_ );

	// clear items and start file-seeking thread
	clearItems();

	if ( !paths_.empty() ) 
		startThread();
}


/**
 *	This method starts a rescan for files.
 */
void ListFileProvider::refresh()
{
	BW_GUARD;

	stopThread();

	clearItems();

	if ( !paths_.empty() ) 
		startThread();
}


/**
 *	This method clears all items off the list.
 */
void ListFileProvider::clearItems()
{
	BW_GUARD;

	mutex_.grab();
	items_.clear();
	searchResults_.clear();
	mutex_.give();
}


/**
 *	This method clears items pending in the scan thread.
 */
void ListFileProvider::clearThreadItems()
{
	BW_GUARD;

	threadMutex_.grab();
	threadItems_.clear();
	tempItems_.clear();
	threadMutex_.give();
}


/**
 *	This method returns true if the scan thread has finished scanning.
 *
 *	@return		True if the scan thread has finished scanning.
 */
bool ListFileProvider::finished()
{
	return !getThreadWorking();
}


/**
 *	This method returns the number of items found during scanning.
 *
 *	@return		Number of items currently in found during scanning.
 */
int ListFileProvider::getNumItems()
{
	BW_GUARD;

	mutex_.grab();
	int ret;
	if ( isFiltering() )
		ret = (int)searchResults_.size();
	else
		ret = (int)items_.size();
	mutex_.give();

	return ret;
}


/**
 *	This method returns info object for the item at the given position.
 *
 *	@param index	Index of the item in the list.
 *	@return		Asset info object corresponding to the item.
 */
const AssetInfo ListFileProvider::getAssetInfo( int index )
{
	BW_GUARD;

	if ( index < 0 || getNumItems() <= index )
		return AssetInfo();

	SimpleMutexHolder smh( mutex_ );
	ListItemPtr item;
	if ( isFiltering() )
		item = searchResults_[ index ];
	else
		item = items_[ index ];

	return AssetInfo(
		type_,
		item->title,
		item->fileName );
}


/**
 *	This method returns the icon corresponding to the filename extension of the
 *	item, if any.
 *
 *	@param name		Asset filename.
 *	@return		Icon corresponding to the asset's filename extension.
 */
HICON ListFileProvider::getExtensionIcons( const BW::wstring& name )
{
	BW_GUARD;

	BW::string nname;
	bw_wtoutf8( name, nname );
	BW::string ext = BWResource::getExtension( nname ).to_string();

	if ( !ext.length() )
		return 0;

	HICON icon = 0;

	for( BW::vector<ExtensionsIcons>::iterator i = extensionsIcons_.begin(); i != extensionsIcons_.end(); ++i )
	{
		for( BW::vector<BW::wstring>::iterator j = (*i).extensions.begin(); j != (*i).extensions.end(); ++j )
		{
			if ( bw_MW_stricmp( (*j).c_str(), ext.c_str() ) == 0 )
			{
				icon = (*i).icon;
				break;
			}
		}
	}

	return icon;
}


/**
 *	This method returns the appropriate thumbnail for the item.
 *
 *	@param manager	Reference to the thumbnail manager object in use.
 *	@param index	Index of the item in the list.
 *	@param img		Returns here the thumbnail for the item.
 *	@param w		Desired width for the thumbnail.
 *	@param h		Desired height for the thumbnail.
 *	@param updater	Thumbnail creation callback object.
 */
void ListFileProvider::getThumbnail( ThumbnailManager& manager,
									int index, CImage& img, int w, int h,
									ThumbnailUpdater* updater )
{
	BW_GUARD;

	if ( index < 0 || getNumItems() <= index )
		return;

	mutex_.grab();
	BW::wstring item;
	if ( isFiltering() )
		item = searchResults_[ index ]->fileName;
	else
		item = items_[ index ]->fileName;
	mutex_.give();

	// find thumbnail

	HICON extIcon = getExtensionIcons( item );
	if ( extIcon )
	{
		CBrush back;
		back.CreateSolidBrush( GetSysColor( COLOR_WINDOW ) );
		img.Create( w, h, 32 );
		CDC* pDC = CDC::FromHandle( img.GetDC() );
		DrawIconEx( pDC->m_hDC, 0, 0, extIcon, w, h, 0, (HBRUSH)back, DI_NORMAL );
		img.ReleaseDC();
	}

	manager.create( item, img, w, h, updater );
}


/**
 *	This method filters the list of items found during scanning by the filters
 *	if the filters and/or search text are active.
 */
void ListFileProvider::filterItems()
{
	BW_GUARD;

	mutex_.grab();
	if ( !isFiltering() )
	{
		mutex_.give();
		return;
	}

	// start
	searchResults_.clear();

	// start filtering
	for( ItemsItr i = items_.begin(); i != items_.end(); ++i )
	{
		if ( filterHolder_->filter( (*i)->title, (*i)->fileName ) )
		{
			if ( (searchResults_.size() % VECTOR_BLOCK_SIZE) == 0 )
				searchResults_.reserve( searchResults_.size() + VECTOR_BLOCK_SIZE );
			searchResults_.push_back( (*i) );
		}
	}
	mutex_.give();
}


/**
 *	This method returns whether or not items are being filtered.
 *
 *	@return True if the items where filtered.
 */
bool ListFileProvider::isFiltering()
{
	BW_GUARD;

	return filterHolder_ && filterHolder_->isFiltering();
}


/**
 *	This method sets the desired icon for one or more filename extensions.
 *
 *	@param extensions	Comma or semicolon separated list of filename
 *						extensions to associate with the desired icon.
 *	@param icon			Icon for the extensions.
 */
void ListFileProvider::setExtensionsIcon( BW::wstring extensions, HICON icon )
{
	BW_GUARD;

	if ( !icon )
		return;

	ExtensionsIcons extIcons;

	bw_tokenise( extensions, L",;", extIcons.extensions );
	
	extIcons.icon = icon;

	extensionsIcons_.push_back( extIcons );
}


/**
 *	This method allows for tweaking the interval for the scanning thread to
 *	yield in order to avoid using to much CPU and thrashing the disk too much.
 *	NOTE: After heaps of testing, the recommended value for this is 125ms with
 *	a thread priority of +1 which is "above normal".  This gives very
 *	responsive file scanning without interfering too much with the tool.
 *
 *	@param msec		Interval in milliseconds in between yield periods.
 */
void ListFileProvider::setThreadYieldMsec( int msec )
{
	BW_GUARD;

	threadYieldMsec_ = max( msec, 0 );
}

/**
 *	This method returns the interval for the scanning thread to yield, used in
 *	order to avoid using to much CPU and thrashing the disk too much.
 *
 *	@return		Interval in milliseconds in between yield periods.
 */
int ListFileProvider::getThreadYieldMsec()
{
	BW_GUARD;

	return threadYieldMsec_;
}


/**
 *	This method allows for tweaking the scanning thread's priority to avoid
 *	using to much CPU and thrashing the disk too much.
 *	NOTE: After heaps of testing, the recommended value for this is +1 which is
 *	"above normal" with 125ms of yield interval.  This gives very responsive
 *	file scanning without interfering too much with the tool.
 *
 *	@param priority	Thread priority, -1 for bellow normal, 0 for normal, 1 for
 *					above normal priority.
 */
void ListFileProvider::setThreadPriority( int priority )
{
	BW_GUARD;

	threadPriority_ = priority;
}


/**
 *	This method allows for tweaking the scanning thread's priority to avoid
 *	using to much CPU and thrashing the disk too much.
 *
 *	@return	Thread priority, -1 for bellow normal, 0 for normal, 1 for above
 *			normal priority.
 */
int ListFileProvider::getThreadPriority()
{
	BW_GUARD;

	return threadPriority_;
}


/**
 *	This method allows for flagging the scanning thread for working or stopping
 *	when set to true or false respectively.
 *
 *	@param working	True to flag that the thread is working, false to stop it.
 */
void ListFileProvider::setThreadWorking( bool working )
{
	BW_GUARD;

	mutex_.grab();
	threadWorking_ = working;
	mutex_.give();
}


/**
 *	This method returns whether the scanning thread is working or stopped.
 *
 *	@return		True if the scanning thread is working, false if it is stopped.
 */
bool ListFileProvider::getThreadWorking()
{
	BW_GUARD;

	mutex_.grab();
	bool ret = threadWorking_; 
	mutex_.give();
	return ret;
}


/**
 *	This static method serves as the main loop of the scanning thread.
 *
 *	@param provider	User-defined thread param, in this case it's a list file
 *					provider.
 */
void ListFileProvider::s_startThread( void* provider )
{
	BW_GUARD;

	ListFileProvider* p = (ListFileProvider*)provider;

	p->clearThreadItems();

	p->flushClock_ = clock();

	int lastFlushMsec = p->threadFlushMsec_; // save original flush Msecs
	if ( p->threadYieldMsec_ > 0 )
		p->yieldClock_ = clock();

	for ( BW::vector<BW::wstring>::iterator i = p->paths_.begin() ;
		p->getThreadWorking() && i != p->paths_.end() ;
		++i )
		p->fillFiles( (*i).c_str() );

	p->flushThreadBuf();

	p->threadFlushMsec_ = lastFlushMsec; // restore original flush Msecs

	p->setThreadWorking( false );
}


/**
 *	This method starts the file scanning thread.
 */
void ListFileProvider::startThread()
{
	BW_GUARD;

	stopThread();

	setThreadWorking( true );

	thread_ = new SimpleThread( s_startThread, this, "List File Provider" );
	if ( threadPriority_ > 0 )
	{
		// the user wants a lot of priority for the thread
		SetThreadPriority( thread_->handle(), THREAD_PRIORITY_ABOVE_NORMAL );
	}
	else if ( threadPriority_ < 0 )
	{
		// the user wants the thread to be highly cooperative
		SetThreadPriority( thread_->handle(), THREAD_PRIORITY_BELOW_NORMAL );
	}
}


/**
 *	This method stops the file scanning thread.
 */
void ListFileProvider::stopThread()
{
	BW_GUARD;

	if ( !thread_ )
		return;

	setThreadWorking( false );
	bw_safe_delete(thread_);

	clearThreadItems();
}


/**
 *	This method is called by the scanning thread in order to look for files
 *	that match this provider's paths and filename extensions.
 *
 *	@param path		File path to look for files.
 */
void ListFileProvider::fillFiles( const CString& path )
{
	BW_GUARD;

	CFileFind finder;
	
	CString p = path + L"\\*.*";
	if ( !finder.FindFile( p ) )
		return;

	// hack to avoid reading 
	BW::wstring legacyThumbnailPostfix = L".thumbnail.bmp";
	int legacyThumbSize = (int)legacyThumbnailPostfix.length();
	int thumbSize = (int)thumbnailPostfix_.length();

	bool ignoreFiles = false;
	if ( !includeFolders_.empty()
		&& !StringUtils::matchSpecT( BW::wstring( (LPCTSTR)path ), includeFolders_ ) )
		ignoreFiles = true;

	BOOL notEOF = TRUE;
	while( notEOF && getThreadWorking() )
	{
		notEOF = finder.FindNextFile();
		if ( !finder.IsDirectory() )
		{
			if ( !ignoreFiles )
			{
				BW::wstring fname( (LPCTSTR)finder.GetFileName() );
				if ( StringUtils::matchExtensionT( fname, extensions_ )
					&& ( (int)fname.length() <= thumbSize
						|| fname.substr( fname.length() - thumbSize ) != thumbnailPostfix_ )
					&& ( (int)fname.length() <= legacyThumbSize
						|| fname.substr( fname.length() - legacyThumbSize ) != legacyThumbnailPostfix )
					)
				{
					ListItemPtr item = new ListItem();
					item->fileName = (LPCTSTR)finder.GetFilePath();
					BW::string nfileName;
					bw_wtoutf8( item->fileName, nfileName );
					bw_utf8tow( BWResource::dissolveFilename( nfileName ), item->dissolved );
					item->title = (LPCTSTR)finder.GetFileName();
					threadMutex_.grab();
					if ( (threadItems_.size() % VECTOR_BLOCK_SIZE) == 0 )
						threadItems_.reserve( threadItems_.size() + VECTOR_BLOCK_SIZE );
					threadItems_.push_back( item );
					threadMutex_.give();
				}
			}
		}
		else if ( !finder.IsDots()
			&& !( flags_ & LISTFILEPROV_DONTRECURSE )
			&& ( excludeFolders_.empty()
			|| !StringUtils::matchSpecT( BW::wstring( (LPCTSTR)finder.GetFilePath() ), excludeFolders_ ) )
			)
		{
			fillFiles( finder.GetFilePath() );
		}

		if ( threadYieldMsec_ > 0 )
		{
			if ( ( clock() - yieldClock_ ) * 1000 / CLOCKS_PER_SEC > threadYieldMsec_ )
			{
				Sleep( 50 ); // yield
				yieldClock_ = clock();
			}
		}

		if ( ( clock() - flushClock_ ) * 1000 / CLOCKS_PER_SEC >= threadFlushMsec_  )
		{
			flushThreadBuf();
			flushClock_ = clock();
		}
	}
}


/**
 *	This static method is used for sorting items in the list, which is mainly
 *	done in the scanning thread.
 *
 *	@param a	First item to compare.
 *	@param b	The other item to compare against.
 *	@return		True if a is less than b, false otherwise.
 */
bool ListFileProvider::s_comparator( const ListFileProvider::ListItemPtr& a, const ListFileProvider::ListItemPtr& b )
{
	BW_GUARD;

	return wcsicmp( a->title.c_str(), b->title.c_str() ) < 0;
}


/**
 *	This method finishes processing items found in the scanning thread, and it
 *	is called from the scanning thread itself as well.
 */
void ListFileProvider::flushThreadBuf()
{
	BW_GUARD;

	threadMutex_.grab();
	if ( !threadItems_.empty() )
	{
		tempItems_.reserve( tempItems_.size() + threadItems_.size() );
		for( ItemsItr i = threadItems_.begin(); i != threadItems_.end(); ++i )
			tempItems_.push_back( *i );

		threadItems_.clear();

		std::sort< ItemsItr >( tempItems_.begin(), tempItems_.end(), s_comparator );

		if ( paths_.size() > 1 )
			removeDuplicateFileNames();

		if ( !(flags_ & LISTFILEPROV_DONTFILTERDDS) && hasImages_ )
		{
			MF_ASSERT( !(flags_ & LISTFILEPROV_FILTERNONDDS) );
			removeRedundantDdsFiles();
		}

		if ( (flags_ & LISTFILEPROV_FILTERNONDDS) && hasImages_ )
		{
			MF_ASSERT( (flags_ & LISTFILEPROV_DONTFILTERDDS) );
			removeRedundantNonDdsFiles();
		}

		threadFlushMsec_ = 1000;	// after the first flush, update every second

		// copy the actual items in tempItems_ to the items_ vector
		mutex_.grab();
		items_.clear();
		items_.reserve( tempItems_.size() );
		items_ = tempItems_;
		mutex_.give();

		// finished doing stuff with thread-only data, so release mutex
		threadMutex_.give();

		// and filter if filtering is on
		filterItems();
	}
	else
	{
		threadMutex_.give();
	}
}


/**
 *	This method ensures that when the same filename is found in more than one
 *	path, only the one belonging to the topmost path in the paths.xml is taken,
 *	as the BW resource manager does.
 */
void ListFileProvider::removeDuplicateFileNames()
{
	BW_GUARD;

	for( ItemsItr i = tempItems_.begin(); i != tempItems_.end(); ++i )
	{
		// remove items marked as duplicates in the inner loop
		while ( i != tempItems_.end() && !(*i) )
			i = tempItems_.erase( i );
		if ( i == tempItems_.end() )
			break;

		ItemsItr next = i;
		++next;
		if ( next != tempItems_.end() )
		{
			// check all items named the same as i ( and not marked for erasure! )
			for( ItemsItr j = next; j != tempItems_.end(); ++j )
			{
				if ( (*j) )
				{
					// not marked for erasure, so process

					// files are sorted, so check if the next is duplicate of the current
					if ( (*i)->title != (*j)->title )
						break; // no more duplicate names, get out!

					// handle files that are duplicated
					if ( (*i)->dissolved == (*j)->dissolved )
					{
						// make sure that the file surviving has the correct path
						BW::string ndissolved;
						bw_wtoutf8( (*i)->dissolved, ndissolved );
						bw_utf8tow( BWResource::resolveFilename( ndissolved ), (*i)->fileName );
						// keep using windows-style slashes
						std::replace( (*i)->fileName.begin(), (*i)->fileName.end(), L'/', L'\\' );
						// mark as duplicate, so it gets erased later
						*j = 0;
					}
				}
			}
		}
	}
}


/**
 *	This method removes DDS files if there is an equally named TGA/PNG/JPG/BMP
 *	file next to it, to avoid artists from directly using the automatically
 *	generated DDS file.
 */
void ListFileProvider::removeRedundantDdsFiles()
{
	BW_GUARD;

	for( ItemsItr i = tempItems_.begin(); i != tempItems_.end(); )
	{
		// remove DDS files if a corresponding source image file exists
		BW::string nFilename;
		bw_wtoutf8( (*i)->fileName, nFilename );
		if ( BWResource::getExtension( nFilename ) == "dds" )
		{
			BW::wstring wbmp; bw_utf8tow( BWResource::changeExtension( nFilename, ".bmp" ), wbmp );
			BW::wstring wpng; bw_utf8tow( BWResource::changeExtension( nFilename, ".png" ), wpng );
			BW::wstring wtga; bw_utf8tow( BWResource::changeExtension( nFilename, ".tga" ), wtga );

			if ((PathFileExists( wbmp.c_str() ) ||
				PathFileExists( wpng.c_str() ) ||
				PathFileExists( wtga.c_str() ) ) )
			{
				// the DDS already has a source image, so don't show the DDS file
				i = tempItems_.erase( i );
				continue;
			}
		}

		++i;
	}
}


/**
 *	This method removes TGA/PNG/JPG/BMP if there is an equally named DDS
 *	file next to it, to sometimes we need use only dds file (ie. flora.xml)
 */
void ListFileProvider::removeRedundantNonDdsFiles()
{
	BW_GUARD;

	for( ItemsItr i = tempItems_.begin(); i != tempItems_.end(); )
	{
		// remove non DDS files if a corresponding dds file exists
		wchar_t ext[10];
		BWResource::getExtensionT( (*i)->fileName.c_str(), ext, ARRAY_SIZE( ext ) );
		if ( bw_str_equal( ext, L"bmp", 3 ) ||
			bw_str_equal( ext, L"png", 3 ) ||
			bw_str_equal( ext, L"jpg", 3 ) ||
			bw_str_equal( ext, L"tga", 3 ))
		{
			wchar_t wdds[256];
			bw_str_copy( wdds, 256, (*i)->fileName );
			BWResource::changeExtensionT( wdds, L".dds" );

			if (PathFileExists( wdds))
			{
				// there is already a dds file, so don't show this image.
				i = tempItems_.erase( i );
				continue;
			}
		}

		++i;
	}
}

/**
 *	Constructor.
 *
 *	@param thumbnailPostfix	Posfix used for thumbnail image files.
 */
FixedListFileProvider::FixedListFileProvider( const BW::wstring& thumbnailPostfix ) :
	ListFileProvider( thumbnailPostfix )
{
	BW_GUARD;
}


void FixedListFileProvider::init( const BW::vector< BW::wstring >& paths )
{
	BW_GUARD;
	items_.clear();

	BW::vector< BW::wstring >::const_iterator it = paths.begin();
	for (; it != paths.end(); ++it)
	{
		ListItemPtr item = new ListItem();
		item->dissolved = *it;
		item->fileName = BWResource::resolveFilenameW( item->dissolved );

		BW::wstring::size_type pos = item->dissolved.find_last_of( L'/' );
		if (pos != BW::wstring::npos)
		{
			item->title = item->dissolved.substr( pos + 1 );
		}
		items_.push_back( item );
	}

	filterItems();

}

BW_END_NAMESPACE

