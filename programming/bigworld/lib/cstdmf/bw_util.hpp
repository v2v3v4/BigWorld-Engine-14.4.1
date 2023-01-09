#ifndef BW_UTIL_HPP
#define BW_UTIL_HPP

#include "bw_namespace.hpp"
#include "bw_string.hpp"
#include "bw_string_ref.hpp"

#include <algorithm>

#if !defined( _WIN32 )
#include <unistd.h>
#endif // defined( _WIN32 )

BW_BEGIN_NAMESPACE

namespace BWUtil
{

/**
 *	This method converts a path into a path that is useable by BigWorld.
 *	
 *	Primarily this is used to convert windows paths:
 *	Example
 *		c:\\bigworld\\fantasydemo -> c:/bigworld/fantasydemo
 *	
 *	@param path The path to sanatise.
 */
CSTDMF_DLL void sanitisePath( BW::string & path );

template< typename T >
void sanitisePathT( T * p )
{
	while (*p)
	{
		if (*p == T('\\') )
		{
			*p = T('/');
		}
		++p;
	}
}

/**
 *	This method makes certain that the path contains a trailing folder
 *	separator.
 *
 *	@param pathToFormat 	The path string to be formatted.
 *
 *	@return The path with a trailing folder separator.
 */
CSTDMF_DLL BW::string formatPath( const BW::StringRef & pathToFormat );

/**
 *	This method normalises the given path, collapsing down any relative
 *	folders (i.e. "." and ".." get collapsed). The path types will be
 *	sanitised so that \\ is converted to /. Any leading '..' tokens will
 *	be preserved.
 *	Example
 *		c:\\bigworld\\fantasydemo\\..\\res ->
 *		c:/bigworld/fantasydemo/res
 *
 *	@param path 	The path string to be normalised.
 *
 *	@return The path with a trailing folder separator.
 */
CSTDMF_DLL BW::string normalisePath( const BW::StringRef & path );

/**
 *	Determines if the given path is absolute or relative.
 *
 *	@param path		The path to check.
 *
 *	@return path string
 *
 */
CSTDMF_DLL bool isAbsolutePath( const BW::StringRef & path );

/**
 *	Retrieves the path from a file.
 *
 *	@param pathToFile file to get path of.
 *
 *	@return path string
 *
 */
CSTDMF_DLL BW::string getFilePath( const BW::StringRef & pathToFile );


/**
 *	This method returns the directory component of the current
 *	process' executable file location.
 */
CSTDMF_DLL BW::string executableDirectory();


/**
 *	This method returns the full path (including executable filename) of the
 *	current process' executable file.
 */
CSTDMF_DLL BW::string executablePath();


/**
 *	This method gets the executable filename
 *
 *	This filename is the basename of the executable
 *
 *	@return the executable basename
 */
CSTDMF_DLL BW::string executableBasename();

/**
 *	This method retreives the full path (including executable filename) of the
 *	current process' executable file.
 */
CSTDMF_DLL bool getExecutablePath( char * pathBuffer, size_t pathBufferSize );

/**
 *	This method breaks a path into its different paths, in the specified string,
 *	the directory will not have a trailing slash, and the path will be 
 *	sanitised.
 *
 *	The original path will point to the beginning of the string, however there
 *	will be null terminators at each section.
 */
CSTDMF_DLL void breakPath( char * path, char ** ppDirectory,
	char ** ppBasename, char ** ppExtension );

/**
 *	This method removes NULL args from argv and adjusts argc appropriately.
 */
CSTDMF_DLL void compressArgs( int & argc, const char * argv[] );


/**
 *	This method returns if the item is inside the container
 */
template<typename Container, typename Item>
bool contains( const Container& c, const Item& i )
{
	return std::find( c.begin(), c.end(), i ) != c.end();
}

#ifdef _WIN32
/**
 * get the last windows error as a string
 */ 
CSTDMF_DLL bool  getLastErrorMsg( char * errMsg, size_t errMsgSize );
#endif

} // namespace BWUtil


#if defined( _WIN32 )
/**
 *	This function returns the username the process is running under.
 *
 *	@return A string representation of the username on success, an empty string
 *	  on error.
 */
inline const char * getUsername()
{
	return "";
}
#else
const char * getUsername(); // Implemented in cpp.
#endif


#if defined( _WIN32 )
CSTDMF_DLL FILE* bw_fopen( const char* filename, const char* mode );
#define fopen bw_fopen

CSTDMF_DLL long bw_fileSize( FILE* file );
BW::wstring getTempFilePathName();

CSTDMF_DLL int bw_unlink( const char* filename ); // remove a file
CSTDMF_DLL int bw_rename( const char *oldname, const char *newname );// rename a file

#else
CSTDMF_DLL BW::string getTempFilePathName();
#define bw_fopen fopen //bw_fopen might be used directly

#define bw_unlink unlink
#define bw_rename rename
#endif

	/**
	 *	This nicely formats a number.
	 *
	 *	@param number	The number to format.
	 *	@param width	The width of the resultant string.  This may be 
	 *					expanded if this number is too small and the number 
	 *					large.
	 *	@param fill		The character to use for spacing.
	 *	@param thousands	The character to use for a separators between
	 *					thousand units.  If this is NULL then no separator is
	 *					is used.
	 */
	BW::string 
	formatNumber
	(
		uint64			number, 
		size_t			width, 
		char			fill			= ' ', 
		char			thousands		= ','
	);

/**
 *	This function replaces non-printable characters (@see ::isprint) in the
 *	provided string with escaped hexadecimal representation (\xXX) to be used in
 *	diagnostic messages.
 */
CSTDMF_DLL BW::string sanitiseNonPrintableChars( const BW::StringRef & str );

BW_END_NAMESPACE

#endif // BW_UTIL_HPP
