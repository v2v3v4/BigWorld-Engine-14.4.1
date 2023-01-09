#ifndef AOI_UPDATE_SCHEMES_HPP
#define AOI_UPDATE_SCHEMES_HPP

#include "cstdmf/stdmf.hpp"

#include "cstdmf/bw_map.hpp"
#include "cstdmf/bw_string.hpp"


BW_BEGIN_NAMESPACE

typedef uint8 AoIUpdateSchemeID;


/**
 *	This class stores an AoI update scheme that can be used by EntityCache.
 */
class AoIUpdateScheme
{
public:
	AoIUpdateScheme();
	bool init( const BW::string & name, float minDelta, float maxDelta );

	/**
	 *  This function checks if this scheme should be treated as coincident.
	 *  @return True if scheme should be treated as coincident, false otherwise.
	 */
	bool shouldTreatAsCoincident() const
	{
		return isZero( weighting_ ) && isZero( distanceWeighting_ );
	}

	/**
	 *	This method applies the AoI update scheme, returning a priority delta
	 *	for the given distance.
	 *
	 *	@param distanceSquared 	The distance from the witness, squared.
	 *	@return 				The priority delta.
	 */
	double apply( float distanceSquared ) const
	{
		if (this->shouldTreatAsCoincident())
		{
			return 1.0;
		}

		const float distance = sqrtf( distanceSquared );
		return (distance * distanceWeighting_ + 1.f) * weighting_;
	}

private:
	float weighting_;
	float distanceWeighting_;
};


/**
 *	This class stores the AoI schemes that can be used by EntityCache.
 */
class AoIUpdateSchemes
{
public:
	typedef AoIUpdateSchemeID SchemeID;

	/**
	 *  This function checks if this scheme should be treated as coincident.
	 *  @param scheme Id of scheme.
	 *  @return True if scheme should be treated as coincident, false otherwise.
	 */
	static bool shouldTreatAsCoincident( SchemeID scheme )
	{
		return schemes_[ scheme ].shouldTreatAsCoincident();
	}

	static double apply( SchemeID scheme, float distance )
	{
		return schemes_[ scheme ].apply( distance );
	}

	static bool init();

	static bool getNameFromID( SchemeID id, BW::string & name );
	static bool getIDFromName( const BW::string & name, SchemeID & rID );

private:
	// Not using BW::vector just to remove one dereference.
	static AoIUpdateScheme schemes_[ 256 ];

	typedef BW::map< BW::string, SchemeID > NameToSchemeMap;
	static NameToSchemeMap nameToScheme_;

	typedef BW::map< SchemeID, BW::string > SchemeToNameMap;
	static SchemeToNameMap schemeToName_;
};

BW_END_NAMESPACE

#endif // AOI_UPDATE_SCHEMES_HPP
