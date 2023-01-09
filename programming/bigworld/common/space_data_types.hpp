#ifndef SPACE_DATA_TYPES_HPP
#define SPACE_DATA_TYPES_HPP

#include "cstdmf/stdmf.hpp"
#include "math/rectt.hpp"


BW_BEGIN_NAMESPACE

const uint16 SPACE_DATA_TOD_KEY = 0;
/**
 *	This structure is used to send time-of-day data via the SpaceData mechanism.
 */
struct SpaceData_ToDData
{
	float	initialTimeOfDay;
	float	gameSecondsPerSecond;
};


/// This constant is the index for SpaceData concerning what geometry data is
/// mapped into the space. It influences both the client and the server.
const uint16 SPACE_DATA_MAPPING_KEY_CLIENT_SERVER = 1;

/// This constant is the index for SpaceData concerning what geometry data is
/// mapped into the space. It only influences the client. The server will not
/// load this geometry.
const uint16 SPACE_DATA_MAPPING_KEY_CLIENT_ONLY = 2;

/**
 *	This structure is used to send information about what geometry data is
 *	mapped into a space and where it is mapped.
 */
struct SpaceData_MappingData
{
	float	matrix[4][4];	// like this since unaligned
	//char	path[];
};


const uint16 SPACE_DATA_FIRST_USER_KEY = 256;
const uint16 SPACE_DATA_FINAL_USER_KEY = 32767;

const uint16 SPACE_DATA_FIRST_CELL_ONLY_KEY = 16384;

const uint16 SPACE_DATA_FIRST_NON_USER_CELL_ONLY_KEY =
		SPACE_DATA_FINAL_USER_KEY + 1;


/// This constant is the index for the recording name.
const uint16 SPACE_DATA_RECORDING =
		SPACE_DATA_FIRST_NON_USER_CELL_ONLY_KEY;

/// This constant is the index for the artificial load data for space.
const uint16 SPACE_DATA_ARTIFICIAL_LOAD =
		SPACE_DATA_FIRST_NON_USER_CELL_ONLY_KEY + 1;
/**
 * This structure is used to send artificial load data for space
 */
struct SpaceData_ArtificialLoad
{
	float	artificialMinLoad;
};



const uint16 SPACE_DATA_SERVER_LOAD_BOUNDS =
		SPACE_DATA_FIRST_NON_USER_CELL_ONLY_KEY + 2;


/**
 *  This structure is used to send load bounds via the SpaceData mechanism.
 */
struct SpaceData_LoadBounds
{
	BW::Rect loadRect;

	/**
	 *  This function converts a string buffer to a SpaceData_LoadBounds representation
	 *  @param input string that acts as a buffer
	 *  @param loadBounds reference to a pointer that receives the casted value
	 */
	static bool castFromString( const BW::string & input, SpaceData_LoadBounds * & loadBounds )
	{
		IF_NOT_MF_ASSERT_DEV( input.length() == sizeof(SpaceData_LoadBounds) )
		{
			return false;
		}
		loadBounds = (SpaceData_LoadBounds*)input.data();
		return true;
	}

	/**
	 *  This function copies SpaceData_LoadBounds structure to a string buffer
	 *  @param loadBounds reference to a source structure
	 *  @param output reference to a target buffer string
	 */
	static void copyToString( const SpaceData_LoadBounds & loadBounds, BW::string & output )
	{
		output.assign( (char*)&loadBounds, sizeof(SpaceData_LoadBounds) );
	}
};




BW_END_NAMESPACE

#endif // SPACE_DATA_TYPES_HPP
