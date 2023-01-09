#include "pch.hpp"
#include "resmgr/filename_case_checker.hpp"
#include "cstdmf/string_builder.hpp"
#include "cstdmf/string_utils.hpp"

#ifdef  _WIN32

BW_BEGIN_NAMESPACE

/**
 *	This is the FilenameCaseChecker constructor.
 */
FilenameCaseChecker::FilenameCaseChecker()
{
}


/**
 *	This checks that the file on disk matches the case of path.
 *
 *	@param path			The file to check.
 *	@param warnIfNotCorrect	Print a warning if the case is not correct the 
 *						first time that the file is checked.
 *	@returns			True if the case of path matches the file on disk, 
 *						false if they differ.  If the file does not exist
 *						then true is returned (it is assumed that this is a
 *						new file).
 */
bool FilenameCaseChecker::check
(
	BW::StringRef	const &path,
	bool			warnIfNotCorrect,	/*= true*/
	bool			warnIfNotFound		/*= false*/
)
{
	SimpleMutexHolder holder(mutex_);

	bool wasInCache = false;
	const FileNameCacheEntry & cachedEntry = cachePath(path, &wasInCache);

	if (!wasInCache)
	{
		if (warnIfNotFound && !cachedEntry.exists_)
		{
			WARNING_MSG
				(
					"Filename %s does not exist on disk\n",
					path.to_string().c_str()
				); 
			return true;
		}
		else if (warnIfNotCorrect && !cachedEntry.matchedCase_)
		{
			WARNING_MSG
				(
					"Case in filename %s does not match case on disk %s\n",
					path.to_string().c_str(),
					cachedEntry.resolvedName_.c_str()
				); 
			return false;
		}
	}
	
	if (cachedEntry.exists_)
	{
		return cachedEntry.matchedCase_;
	}

	return true;
}



/**
 *	This gets the name of a file as it is on disk.
 *
 *	@param path		The filename.
 *	@returns		The name of the file as it is on disk.  If the file does
 *					not exist then path is returned.
 */
void FilenameCaseChecker::filenameOnDisk(BW::StringRef const &path, BW::string& out)
{
	SimpleMutexHolder holder(mutex_);

	const FileNameCacheEntry & cachedEntry = cachePath( path, NULL );
	out.assign( cachedEntry.resolvedName_ );
}

// returns true if stripped. only checks last character.
bool FilenameCaseChecker::stripTrailingGlob( BW::string& path )
{
	if (path.size() && path[path.size()-1] == '*')
	{
		path.resize( path.size() - 1 );
		return true;
	}
	return false;
}


void FilenameCaseChecker::windowsSlashes( BW::string& path )
{
	std::replace( path.begin(), path.end(), '/', '\\' );
}


/**
 * This function returns the file system's case-correct file name given a 
 * case-insensitive input path. 
 *
 * @param path		The input case-insensitive path.
 * @param out		output param for filling case-correct path. 
 *					If path doesn't exist, is the input path
 * 
 * @param isCaseCorrect		True if the input path was case-correct. 
 *      					False if it was not case correct
 *
 * @return			True if the path exists (case-insensitive). 
 */
bool FilenameCaseChecker::caseCorrectFilenameOnDisk( 
	const BW::StringRef& path, BW::string& out, bool* isCorrectCase )
{
	// process slashes, glob
	BW::string spath( path.data(), path.size() );
	windowsSlashes( spath );
	bool hadTrailingGlob = stripTrailingGlob( spath );

	// convert to wide-string
	BW::wstring wpath(spath.begin(), spath.end());

	wchar_t shortName[MAX_PATH];
	wchar_t resolvedName[MAX_PATH];
	DWORD prevError = GetLastError();

	// get case-correct long name via the path's short name.
	DWORD shortres = GetShortPathName( wpath.c_str(), shortName, MAX_PATH);
	if (shortres)
	{
		DWORD longres = GetLongPathName( shortName, resolvedName, MAX_PATH);
		MF_ASSERT( longres != 0 );
	}

	// restore prev error. If the file doesn't exist, shortres is 0, 
	// and an error was set, but we handle this case and don't want to stomp
	// on a previous and possibly more meaningful error code that has been
	// set elsewhere unexpectedly.
	SetLastError( prevError );

	if (shortres)
	{
		// The path is a real path, write out case sensitive version:
		bw_wtoutf8( resolvedName, out );
	}
	else
	{
		// The path does not exist. 
		out = spath;
	}
	
	if ( isCorrectCase != NULL)
	{
		*isCorrectCase = compare( out, spath );
	}

	if (hadTrailingGlob)
	{
		out.append(1, '*');
	}

	return (shortres != 0);
}


/**
 *	This makes sure that path and the real filename on disk are cached.
 *
 *	@param path		The path to cache.
 *	@param wasInCache	If this is not NULL then it's set to true if the path
 *					was already in the cache and false if the path was not in
 *					the cache.
 *	@returns		An iterator to the cached value.
 */
const FilenameCaseChecker::FileNameCacheEntry& 
FilenameCaseChecker::cachePath
	(
	BW::StringRef	const &path,
	bool			*pWasInCache
	)
{
	// TODO: I'd like to search the hash by string ref, but can't see a way how
	BW::string spath(path.data(), path.size());
	FilenameCache::const_iterator it = cache_.find(spath);	
	bool wasInCache = it != cache_.end();
	if (!wasInCache)
	{
		// The name has not yet been cached.
		FileNameCacheEntry cacheEntry;

		// get the case-correct name.
		cacheEntry.exists_ = caseCorrectFilenameOnDisk( spath, 
			cacheEntry.resolvedName_, &cacheEntry.matchedCase_ );

		std::pair<FilenameCache::iterator, bool> res = 
			cache_.insert( std::make_pair( spath, cacheEntry ) );

		// check if insert succeeded
		MF_ASSERT( res.second );
		it = res.first;
	}

	if (pWasInCache != NULL)
	{
		*pWasInCache = wasInCache;
	}

	return it->second;
}


/**
 *	This function compares two paths to see if they are the same.  It
 *	ignores slashes and the first drive letter.
 *
 *	@param path1	The first path.
 *	@param path2	The second path.
 *	@returns		True if the paths are the same, excluding the drive
 *					letter and any slashes.
 */
bool FilenameCaseChecker::compare
(
	BW::StringRef		const &path1, 
	BW::StringRef		const &path2
)
{
	// Handle trivial cases:
	if (path1.empty() || path2.empty())
		return path1.empty() && path2.empty();

	const size_t bufferSize = 1024;
	MF_ASSERT( path1.length() < bufferSize );
	MF_ASSERT( path2.length() < bufferSize );
	char buffer1[bufferSize];
	char buffer2[bufferSize];
	BW::StringBuilder builder1( buffer1, bufferSize );
	BW::StringBuilder builder2( buffer2, bufferSize );
	builder1.append( path1 );
	builder2.append( path2 );

	// Make any drive letters lowercase:
	if (builder1.length() >= 2 && builder1[1] == ':') 
		builder1[0] = tolower(builder1[0]);
	if (builder2.length() >= 2 && builder2[1] == ':') 
		builder2[0] = tolower(builder2[0]);

	// Make back slashes forward slashes:
	builder1.replace( '/', '\\' );
	builder2.replace( '/', '\\' );

	BW::StringRef p1( builder1.string(), builder1.length() );
	BW::StringRef p2( builder2.string(), builder2.length() );

	// Remove any trailing slashes:
	if (p1[p1.length() - 1] == '\\')
	{
		p1 = p1.substr(0, p1.length() - 1);
	}
	if (p2[p2.length() - 1] == '\\')
	{
		p2 = p2.substr(0, p2.length() - 1);
	}

	return p1 == p2;
}

BW_END_NAMESPACE

#endif // _WIN32
