#include "pch.hpp"

#include "bw_util.hpp"

#include "bw_namespace.hpp"
#include "bw_vector.hpp"
#include "guard.hpp"
#include "string_utils.hpp"

BW_BEGIN_NAMESPACE

namespace BWUtil
{

BW::string formatPath( const BW::StringRef & pathToFormat )
{
	BW_GUARD;
	if (pathToFormat.empty())
	{
		return BW::string();
	}

	BW::string tmpPath( pathToFormat.data(), pathToFormat.length() );
	BWUtil::sanitisePath( tmpPath );

	if (tmpPath[tmpPath.size() - 1] != '/')
	{
		tmpPath += '/';
	}

	return tmpPath;
}

BW::string	normalisePath( const BW::StringRef& path )
{
	BW_GUARD;
	typedef BW::vector< BW::StringRef > StringArray;
	StringArray pathParts;
	bw_tokenise( path, "/\\", pathParts );

	BW::string ret;
	size_t i = 0;
	while (i < pathParts.size())
	{
		if (pathParts[i] == "." || pathParts[i] == "" )
		{
			pathParts.erase( pathParts.begin() + i );
		}
		else if (pathParts[i] == ".." && i > 0 && pathParts[i - 1] != "..")
		{
			pathParts.erase( pathParts.begin() + i );
			pathParts.erase( pathParts.begin() + (i-1) );
			--i;
		}
		else
		{
			++i;
		}
	}

	for (StringArray::const_iterator itr = pathParts.begin(), end = pathParts.end(); itr != end; ++itr)
	{
		if (!ret.empty())
		{
			ret += "/";
		}
		ret.append( itr->data(), itr->size() );
	}

	if (ret.size() == 2 && ret[1] == ':')
	{
		ret += "/";
	}
	else if (!path.empty() && 
		(path[0] == '/' || path[0] == '\\'))
	{
		ret.insert( 0, "/" );
	}

	return ret.empty() ? "." : ret;
}

bool isAbsolutePath( const BW::StringRef & path )
{
	BW_GUARD;
	if (path.empty())
	{
		return false;
	}
	else if (path[0] == '/' || path[0] == '\\')
	{
		return true;
	}
	else if (path.size() > 1 && path[1] == ':')
	{
		return true;
	}
	else
	{
		return false;
	}
}

BW::string executableDirectory()
{
	BW_GUARD;
	return BWUtil::getFilePath( BWUtil::executablePath() );
}


BW::string executablePath()
{
	char path[BW_MAX_PATH + 1];
	if (!BWUtil::getExecutablePath( path, sizeof(path) ))
	{
		return "";
	}
	return path;
}

BW::string executableBasename()
{
	BW::string exePath = BWUtil::executablePath();
	BW::StringRef filename( exePath );

	BW::StringRef::size_type startPos = filename.find_last_of( "\\/" );

	if (startPos != BW::StringRef::npos)
	{
		filename = filename.substr( startPos + 1, filename.length() );
	}

	BW::StringRef::size_type endPos = filename.find_first_of( "." );

	if (endPos != BW::string::npos)
	{
		filename = filename.substr( 0, endPos );
	}

	return BW::string( filename.data(), filename.size() );
}




void breakPath( char * path, char ** ppDirectory, 
	char ** ppBasename, char ** ppExtension)
{
	char * dir = path;
	char * base = path;
	char * ext = NULL;

	sanitisePathT( path );
	// Walk along the directory until the last slash
	while (*path)
	{
		if (*path == '/')
		{
			base = path;
		}

		if (*path == '.')
		{
			ext = path;
		}

		++path;
	}

	if (dir != base || *base == '/')
	{
		// We found a basename, so lets null terminate it
		*(base++) = 0;
	}

	if (ext > base)
	{
		// We found an extension, so lets null terminate it
		*(ext++) = 0;
	}

	if (ppDirectory)
	{
		// If valid directory store it, otherwise point to end (NULL)
		*ppDirectory = (dir != base ? dir : path);
	}

	if (ppBasename)
	{
		*ppBasename = base;
	}

	if (ppExtension)
	{
		// If valid extension store it, otherwise point to end (NULL)
		*ppExtension = (ext > base ? ext : path);
	}
}


void compressArgs( int & argc, const char * argv[] )
{
	BW_GUARD;
	const char ** ppDest = argv;
	int newArgc = 0;

	for (int i = 0; i < argc; ++i)
	{
		if (argv[i])
		{
			*ppDest = argv[i];
			++ppDest;
			++newArgc;
		}
	}

	argc = newArgc;
}


#ifdef _WIN32
// get the last windows error as a string
bool getLastErrorMsg( char * errMsg, size_t errMsgSize )
{
	DWORD lastError = GetLastError();
	if (lastError != ERROR_SUCCESS)
	{
		MF_ASSERT( errMsgSize <= UINT_MAX );
		DWORD result = FormatMessageA( FORMAT_MESSAGE_FROM_SYSTEM,
			0, lastError, 0, errMsg, ( DWORD )errMsgSize, 0 );

		if (result != 0)
		{
			return( true );
		}
	}
	return( false );
}
#endif // _WIN32

} // namespace BWUtil

#if !defined( _WIN32 )
/**
 *	This class is a wrapper for the getUsername implementation. An instance
 *	will call the method and initialise the username, thus getting any
 *	delay associated with an LDAP call out of the way before it can cause
 *	problems.
 */
class UsernameGetter
{
public:
	UsernameGetter() : name_()
	{
		struct passwd * pPasswd = getpwuid( geteuid() );
		char * pUsername = pPasswd ? pPasswd->pw_name : NULL;

		if (pUsername)
		{
			name_ = pUsername;
		}
	}

	const char * get() const
	{
		return name_.c_str();
	}

private:
	BW::string name_;
};

UsernameGetter g_usernameGetter;


/**
 *	This function returns the username the process is running under.
 *
 *	@return A string representation of the username on success, an empty string
 *	  on error.
 */
const char * getUsername()
{
	return g_usernameGetter.get();
}
#endif // !_WIN32

BW::string 
	formatNumber
	(
	uint64			number, 
	size_t			width, 
	char			fill ,
	char			thousands
	)
{
	uint64 n = number;
	size_t numchars = 0;
	do { n /= 10; ++numchars; } while (n != 0);

	if (thousands != '\0')
		numchars += (numchars - 1)/3;

	size_t len = std::max(width, numchars);
	BW::string result(len, fill);

	size_t pos = len - 1;
	size_t ths = 3;
	do
	{
		result[pos] = '0' + (char)(number%10);
		number /= 10;
		--pos;
		--ths;
		if (thousands != '\0' && ths == 0 && number != 0)
		{
			result[pos] = thousands;
			ths = 3;
			--pos;
		}
	}
	while (number != 0);

	return result;
}


BW::string sanitiseNonPrintableChars( const BW::StringRef & str )
{
	BW::string result;
	// Reserving for the worst case -- assuming every character is non-printable
	result.reserve( str.size() * 4 );
	for (BW::string::size_type i = 0; i < str.size(); ++i)
	{
		const char ch = str[i];
		if (::isprint( ch ))
		{
			result += ch;
		}
		else
		{
			static const char s_hexChars[] = "0123456789ABCDEF";
			const char hexRepr[] = {
				'\\', 'x', s_hexChars[( ch >> 4 ) & 0x0f], s_hexChars[ch & 0x0f]
			};
			result.append( hexRepr, sizeof( hexRepr ) );
		}
	}
	return result;
}


BW_END_NAMESPACE

// bw_util.cpp
