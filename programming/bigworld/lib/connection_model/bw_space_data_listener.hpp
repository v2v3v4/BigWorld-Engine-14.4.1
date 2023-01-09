#ifndef BW_SPACE_DATA_LISTENER_HPP
#define BW_SPACE_DATA_LISTENER_HPP

#include "cstdmf/bw_string.hpp"

#include "math/matrix.hpp"

#include "network/basictypes.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class is the interface for space data listeners.
 */
class BWSpaceDataListener
{
public:
	virtual ~BWSpaceDataListener() {}

	/**
	 *	This method is called when a new geometry mapping is added to a space.
	 *
	 *	@param spaceID			The SpaceID for the new mapping
	 *	@param mappingMatrix	The translation of the new mapping
	 *	@param mappingName		The resource path for the new mapping
	 */
	virtual void onGeometryMapping( SpaceID spaceID,
						Matrix mappingMatrix,
						const BW::string & mappingName )
	{}

	/**
	 *	This method is called when a geometry mapping is deleted from a space.
	 *
	 *	@param spaceID			The SpaceID for the new mapping
	 *	@param mappingMatrix	The translation of the new mapping
	 *	@param mappingName		The resource path for the new mapping
	 */
	virtual void onGeometryMappingDeleted( SpaceID spaceID,
						Matrix mappingMatrix,
						const BW::string & mappingName )
	{}

	/**
	 *	This method is called when the time of day in a space is changed.
	 *
	 *	@param spaceID				The SpaceID for the changed time of day
	 *	@param initialTime			The time of day for GameTime 0
	 *	@param gameSecondsPerSecond	The number of game seconds per real second
	 */
	virtual void onTimeOfDay( SpaceID spaceID, 
						float initialTime,
						float gameSecondsPerSecond )
	{} 

	/**
	 *	This method is called when user-defined spaceData is added or deleted
	 *
	 *	@param spaceID		The SpaceID for the user-defined spaceData
	 *	@param key			The key for the user-defined spaceData
	 *	@param isInsertion	True if the data was added to this key, false if
	 *						it was deleted
	 *	@param data			The piece of data added to or delete from this key
	 *						in this space
	 */
	virtual void onUserSpaceData( SpaceID spaceID, 
						uint16 key,
						bool isInsertion,
						const BW::string & data )
	{}

};

BW_END_NAMESPACE

#endif // BW_SPACE_DATA_LISTENER_HPP
