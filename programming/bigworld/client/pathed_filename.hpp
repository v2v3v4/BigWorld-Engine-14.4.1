#ifndef PATHED_FILENAME__HPP
#define PATHED_FILENAME__HPP

#include "resmgr/datasection.hpp"

#include "cstdmf/bw_string.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This is a helper class which assembles an absolute system filename
 *	based on a static filename and a dynamically detemermined base name
 *	(e.g. executable path or application data path).
 */
class PathedFilename
{
public:
	enum PathBase
	{
		BASE_APP_DATA,			// The user's application data folder (as determined by the system).
		BASE_LOCAL_APP_DATA,	// The user's app data folder on the local machine.
		BASE_ROAMING_APP_DATA,	// The user's roaming (domain) app data folder.
		BASE_MY_DOCS,			// The user's My Documents folder.
		BASE_MY_PICTURES,		// The user's My Pictures folder.
		BASE_CWD,				// The current working directory.
		BASE_EXE_PATH,			// The directory containing the executable.
		BASE_RES_TREE			// The first res specified in paths.xml.
	};

public:
	PathedFilename();
	PathedFilename( const BW::string& filename, PathBase base );
	PathedFilename( DataSectionPtr ds, 
					const BW::string& defaultFilename, 
					PathBase defaultBase );

	void init( DataSectionPtr ds, 
				const BW::string& defaultFilename, 
				PathBase defaultBase );

	BW::string resolveName() const;

	static BW::string resolveBase( PathBase baseType );

private:
	BW::string filename_;
	PathBase base_;
};

BW_END_NAMESPACE

#endif // PATHED_FILENAME__HPP
