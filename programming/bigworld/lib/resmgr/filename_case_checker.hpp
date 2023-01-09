#ifndef FILENAME_CHECKER_HPP
#define FILENAME_CHECKER_HPP


#ifdef  _WIN32

#include "cstdmf/bw_string_ref.hpp"
#include "cstdmf/stringmap.hpp"
#include "cstdmf/concurrency.hpp"

BW_BEGIN_NAMESPACE

class FilenameCaseChecker
{
public:
	FilenameCaseChecker();

	bool check( BW::StringRef const & path, 
		bool warnIfNotCorrect = true, bool warnIfNotFound = false );

	// cached
	void filenameOnDisk(BW::StringRef const &path, BW::string& out);

public:
	// not cached
	static bool caseCorrectFilenameOnDisk( const BW::StringRef & path, 
		BW::string & out, bool * isCorrectCase );

	// path sanitizing helpers
	static bool stripTrailingGlob( BW::string & path );
	static void windowsSlashes( BW::string & path );

protected:
	struct FileNameCacheEntry
	{
		BW::string resolvedName_;	// case correct name, or input name if not found
		bool matchedCase_;
		bool exists_;
	};

	typedef StringHashMap<FileNameCacheEntry>	FilenameCache;

	const FileNameCacheEntry & cachePath( BW::StringRef const & path, bool * wasInCache );

	static bool compare( BW::StringRef const & path1, BW::StringRef const & path2 );

private:
	FilenameCaseChecker(FilenameCaseChecker const &);				// not allowed
	FilenameCaseChecker &operator=(FilenameCaseChecker const &);	// not allowed

private:
	FilenameCache	cache_;
	SimpleMutex		mutex_;
};

BW_END_NAMESPACE

#endif // _WIN32


#endif // FILENAME_CHECKER_HPP
