#ifndef REPLAY_DATA_COLLECTOR_HPP
#define REPLAY_DATA_COLLECTOR_HPP

#include "cstdmf/bw_map.hpp"
#include "cstdmf/bw_set.hpp"
#include "cstdmf/bw_string.hpp"
#include "cstdmf/memory_stream.hpp"
#include "cstdmf/smartpointer.hpp"
#include "network/basictypes.hpp"

BW_BEGIN_NAMESPACE

class Entity;
class DataDescription;
class MethodDescription;
class PropertyChange;

typedef ConstSmartPointer< Entity > EntityConstPtr;


/**
 *	This class is used to collect replay data each tick.
 */
class ReplayDataCollector
{
public:
	ReplayDataCollector( const BW::string & name, bool shouldRecordAoIEvents );

	~ReplayDataCollector();


	void addEntityState( const Entity & entity );

	void queueEntityVolatile( const Entity & entity );
	void addVolatileDataForQueuedEntities();

	void addEntityMethod( const Entity & entity, 
		const MethodDescription & methodDescription,
		BinaryIStream & data );

	void addEntityProperty( const Entity & entity, 
		const DataDescription & dataDescription, 
		PropertyChange & change );

	void addEntityProperties( const Entity & entity, ScriptDict properties );

	void addEntityPlayerStateChange( const Entity & entity );
	void addEntityAoIChange( EntityID playerID, const Entity & entity, 
		bool hasEnteredAoI );

	void deleteEntity( EntityID entityID );

	void addSpaceData( const SpaceEntryID & spaceEntryID, uint16 key, 
		const BW::string & data );

	void addFinish();

	void clear();

	BinaryIStream & data() { return data_; }

	const BW::string & name() const { return name_; }

	const SpaceEntryID & recordingSpaceEntryID() const
	{ 
		return recordingSpaceEntryID_;
	}

	void recordingSpaceEntryID( const SpaceEntryID & entryID )
	{
		recordingSpaceEntryID_ = entryID;
	}

private:
	BinaryOStream * getStreamForEntity( const Entity & entity );

	BW::string 			name_;
	bool				shouldRecordAoIEvents_;

	MemoryOStream 		data_;
	SpaceEntryID 		recordingSpaceEntryID_;

	typedef BW::map< EntityID, EntityConstPtr > 	MovedEntities;
	MovedEntities movedEntities_;

	typedef BW::map< EntityID, MemoryOStream * > BufferedEntityData;
	BufferedEntityData bufferedEntityData_;
};


BW_END_NAMESPACE

#endif // REPLAY_DATA_COLLECTOR_HPP
