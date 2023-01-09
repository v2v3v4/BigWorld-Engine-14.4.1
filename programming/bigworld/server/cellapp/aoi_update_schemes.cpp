#include "aoi_update_schemes.hpp"

#include "cellapp_config.hpp"

#include "server/bwconfig.hpp"


BW_BEGIN_NAMESPACE



// -----------------------------------------------------------------------------
// Section: AoIUpdateScheme
// -----------------------------------------------------------------------------

/**
 *	Default constructor.
 */
AoIUpdateScheme::AoIUpdateScheme() :
		weighting_( 1.f ),
		distanceWeighting_( 0.2f )
{
}


/**
 *	This method initialises an AoIUpdateScheme.
 *
 *	@param name 	The scheme name.
 *	@param minDelta The priority delta to use for an entity 0 metres away.
 *	@param maxDelta The priority delta to use for an entity maxAoIRadius metres
 *					away.
 */
bool AoIUpdateScheme::init( const BW::string & name, float minDelta,
		float maxDelta )
{
	if ((minDelta < 0.f) || (maxDelta < 0.f))
	{
		ERROR_MSG( "AoIUpdateScheme::init: minDelta (%.f) and maxDelta "
					"(%.f) must be non-negative.\n",
				minDelta, maxDelta );
		return false;
	}

	if (((isZero( minDelta )|| isZero( maxDelta )) &&
			!isZero( minDelta + maxDelta )))
	{
		ERROR_MSG( "AoIUpdateScheme::init: Scheme \"%s\": "
				"Both minDelta (%.f) and maxDelta (%.f) must either be "
				"both zero, or both positive.\n",
			name.c_str(), minDelta, maxDelta );
		return false;
	}

	if (isZero( minDelta ) && isZero( maxDelta ))
	{
		// This is a special case for updating at the maximum detail level, and
		// pretending that the entity is at the same position as the player for 
		// LoD calculations.
		weighting_ = distanceWeighting_ = 0.f;
	}
	else
	{
		weighting_ = minDelta;
		distanceWeighting_ = ((maxDelta / minDelta) - 1.f) /
			CellAppConfig::maxAoIRadius();
	}

	return true;
}


// -----------------------------------------------------------------------------
// Section: AoIUpdateSchemes
// -----------------------------------------------------------------------------

// Static data

AoIUpdateScheme AoIUpdateSchemes::schemes_[ 256 ];

BW::map< BW::string, AoIUpdateSchemes::SchemeID >
	AoIUpdateSchemes::nameToScheme_;

BW::map< AoIUpdateSchemes::SchemeID, BW::string >
	AoIUpdateSchemes::schemeToName_;

/**
 *	This method initialises the AoIUpdateScheme instances.
 */
bool AoIUpdateSchemes::init()
{
	const float DEFAULT_AOI_SCHEME_MIN_DELTA =
		CellAppConfig::witnessUpdateDefaultMinDelta();
	const float DEFAULT_AOI_SCHEME_MAX_DELTA =
		CellAppConfig::witnessUpdateDefaultMaxDelta();
	// Add the default scheme, aliased to empty string and "default".
	schemes_[ 0 ].init( "default", DEFAULT_AOI_SCHEME_MIN_DELTA,
		DEFAULT_AOI_SCHEME_MAX_DELTA );

	nameToScheme_[ "" ] = 0;
	nameToScheme_[ "default" ] = 0;
	schemeToName_[ 0 ] = "default";

	DataSectionPtr pTopSection =
		BWConfig::getSection( "cellApp/aoiUpdateSchemes" );

	DataSectionIterator iter = pTopSection->begin();

	// Custom AoI schemes start from 1.
	SchemeID currSchemeID = 1;

	while ((iter != pTopSection->end()) && (currSchemeID != 0))
	{
		DataSectionPtr pSection = *iter;

		if (pSection->sectionName() == "scheme")
		{
			const BW::string name = pSection->readString( "name" );
			const float minDelta = pSection->readFloat( "minDelta",
				DEFAULT_AOI_SCHEME_MIN_DELTA );
			const float maxDelta = pSection->readFloat( "maxDelta",
				DEFAULT_AOI_SCHEME_MAX_DELTA );

			if ((name == "default") &&
					isEqual( minDelta, DEFAULT_AOI_SCHEME_MIN_DELTA ) &&
					isEqual( maxDelta, DEFAULT_AOI_SCHEME_MAX_DELTA ))
			{
				CONFIG_NOTICE_MSG( "AoIUpdateSchemes::init: "
					"The default scheme is redefined, this is no longer "
					"required and is deprecated behaviour\n" );
				++iter;
				continue;
			}

			if (name.empty())
			{
				CONFIG_ERROR_MSG( "AoIUpdateSchemes::init: "
						"Scheme name cannot be empty (scheme #%d)\n",
					currSchemeID );
				return false;
			}

			if (nameToScheme_.find( name ) != nameToScheme_.end())
			{
				CONFIG_ERROR_MSG( "AoIUpdateSchemes::init: "
								"Duplicate scheme name '%s' (scheme #%d)\n",
							name.c_str(), currSchemeID );

				return false;
			}

			if (!schemes_[ currSchemeID ].init( name, minDelta, maxDelta ))
			{
				return false;
			}

			if (currSchemeID == 1)
			{
				CONFIG_INFO_MSG( "Custom AoI update schemes (from bw.xml "
					"cellApp/aoiUpdateSchemes) are:\n" );
			}

			CONFIG_INFO_MSG( "  Scheme #%d: %s\n",
				int( currSchemeID ), name.c_str() );

			nameToScheme_[ name ] = currSchemeID;
			schemeToName_[ currSchemeID ] = name;

			++currSchemeID;
		}
		else
		{
			CONFIG_WARNING_MSG( "AoIUpdateSchemes::init: "
					"Unexpected section '%s' in 'cellApp/aoiUpdateSchemes'\n",
				pSection->sectionName().c_str() );
		}

		++iter;
	}

	if (currSchemeID == 0)
	{
		// We wrapped around, which means we had too many schemes. Don't
		// overwrite any existing ones.

		CONFIG_ERROR_MSG( "AoIUpdateSchemes::init: Too many schemes defined, "
				"maximum of %" PRIzu " schemes allowed.\n",
			sizeof(schemes_) - 1 );
		return false;
	}

	return true;
}


/**
 *	This method finds the name associated with the given scheme id.
 *
 *	@param id The scheme id to find.
 *	@param rName A reference to a string that will be populated with the name.
 *
 *	@return true on success, otherwise false.
 */
bool AoIUpdateSchemes::getNameFromID( SchemeID id, BW::string & rName )
{
	SchemeToNameMap::const_iterator iter = schemeToName_.find( id );

	if (iter == schemeToName_.end())
	{
		return false;
	}

	rName = iter->second;

	return true;
}


/**
 *	This method finds the id associated with the given scheme name.
 *
 *	@param name The name of the scheme to find.
 *	@param rID A reference to an id that will be populated with the scheme's id.
 *
 *	@return true on success, otherwise false.
 */
bool AoIUpdateSchemes::getIDFromName( const BW::string & name, SchemeID & rID )
{
	NameToSchemeMap::const_iterator iter = nameToScheme_.find( name );

	if (iter == nameToScheme_.end())
	{
		return false;
	}

	rID = iter->second;

	return true;
}

BW_END_NAMESPACE

// aoi_update_schemes.cpp
