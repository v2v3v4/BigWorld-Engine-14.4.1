#include "pch.hpp"
#include "bwversion.hpp"

#include "debug.hpp"
#include "watcher.hpp"

BW_BEGIN_NAMESPACE

namespace BWVersion
{
uint16 g_majorNumber = BW_VERSION_MAJOR;
uint16 g_minorNumber = BW_VERSION_MINOR;
uint16 g_patchNumber = BW_VERSION_PATCH;

BW::string g_versionString;


// -----------------------------------------------------------------------------
// Section: Get Accessors
// -----------------------------------------------------------------------------

/**
 *	This method returns the major version number.
 */
uint16 majorNumber()
{
	return g_majorNumber;
}


/**
 *	This method returns the minor version number.
 */
uint16 minorNumber()
{
	return g_minorNumber;
}


/**
 *	This method returns the patch version number. This is the number of the
 *	highest patch that has been applied.
 */
uint16 patchNumber()
{
	return g_patchNumber;
}


/**
 *	A string representing the version.
 */
const BW::string & versionString()
{
	return g_versionString;
}


// -----------------------------------------------------------------------------
// Section: Set Accessors
// -----------------------------------------------------------------------------

/**
 *	This method is used to update the version string if a version number is
 *	modified.
 */
void updateVersionString()
{
	char buf[ 128 ];
	bw_snprintf( buf, sizeof( buf ), "%d.%d.%d",
			g_majorNumber, g_minorNumber, g_patchNumber );
	g_versionString = buf;
}


/**
 *	This method sets the major version number.
 */
void majorNumber( uint16 number )
{
	if (number != g_majorNumber)
	{
		WARNING_MSG( "BWVersion::majorNumber: Changing from %d to %d\n",
				g_majorNumber, number );
		g_majorNumber = number;
		updateVersionString();
	}
}


/**
 *	This method sets the minor version number.
 */
void minorNumber( uint16 number )
{
	if (number != g_minorNumber)
	{
		WARNING_MSG( "BWVersion::minorNumber: Changing from %d to %d\n",
				g_minorNumber, number );
		g_minorNumber = number;
		updateVersionString();
	}
}


/**
 *	This method sets the patch version number.
 */
void patchNumber( uint16 number )
{
	if (number < g_patchNumber)
	{
		WARNING_MSG( "BWVersion::patchNumber: Changing from %d to %d\n",
				g_patchNumber, number );
	}

	g_patchNumber = number;
	updateVersionString();
}


// -----------------------------------------------------------------------------
// Section: Initialisation
// -----------------------------------------------------------------------------

/**
 *	This class is used to initialise the BWVersion data.
 */
class StaticIniter
{
public:
	StaticIniter()
	{
		MF_WATCH( "version/major",    g_majorNumber,    Watcher::WT_READ_ONLY );
		MF_WATCH( "version/minor",    g_minorNumber,    Watcher::WT_READ_ONLY );
		MF_WATCH( "version/patch",    g_patchNumber,    Watcher::WT_READ_ONLY );
		MF_WATCH( "version/string",   g_versionString,  Watcher::WT_READ_ONLY );

		updateVersionString();
	}
};

StaticIniter staticIniter;

} // BWVersion

BW_END_NAMESPACE

// bwversion.hpp
