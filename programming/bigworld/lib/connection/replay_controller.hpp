#ifndef REPLAY_CONTROLLER_HPP
#define REPLAY_CONTROLLER_HPP

#include "cstdmf/bw_string.hpp"
#include "cstdmf/checksum_stream.hpp"
#include "cstdmf/memory_stream.hpp"
#include "cstdmf/background_file_writer.hpp"
#include "cstdmf/smartpointer.hpp"

#include "entity_def_constants.hpp"
#include "server_message_handler.hpp"
#include "replay_data_file_reader.hpp"
#include "replay_tick_loader.hpp"

#include <memory>


BW_BEGIN_NAMESPACE

// Uncomment this to test if prepend and append are adding
// the correct ticks 
//#define DEBUG_TICK_DATA

class ClientServerProtocolVersion;
class ReplayController;
class ReplayHeader;
class EntityVolatileCache;

// -----------------------------------------------------------------------------
// Section: ReplayTickData
// -----------------------------------------------------------------------------
/**
 *	This class holds buffered tick data in memory as a linked list.
 */
class ReplayTickData
{
public:
	ReplayTickData( BinaryIStream & data ) :
#if defined( DEBUG_TICK_DATA )
		tick_( 0 ),
#endif // defined( DEBUG_TICK_DATA )
		data_( data.remainingLength() ),
		pNext_( NULL )
	{
		data_.transfer( data, data.remainingLength() );
	}

	~ReplayTickData()
	{
	}

	/**
	 *	This method returns the tick data.
	 */
	MemoryOStream & data()
	{
		return data_;
	}

	/**
	 *	This method returns the next tick data node in the list.
	 */
	ReplayTickData * pNext() const
	{
		return pNext_;
	}

	/**
	 *	This method sets the next data node.
	 */
	void pNext( ReplayTickData * newNext )
	{
		pNext_ = newNext;
	}

#if defined( DEBUG_TICK_DATA )

	/**
	 *	This method returns the associated tick time for this tick data node.
	 */
	GameTime tick() const
	{
		return tick_;
	}

	/**
	 *	This method sets the associated tick time for this tick data node.
	 *
	 *	@param newTick 	The tick time.
	 */
	void tick( GameTime newTick )
	{
		tick_ = newTick;
	}

#endif // defined( DEBUG_TICK_DATA )

private:

#if defined( DEBUG_TICK_DATA )
	GameTime tick_;
#endif // defined( DEBUG_TICK_DATA )

	MemoryOStream data_;
	ReplayTickData * pNext_;
};


/**
 *	This is the interface that ReplayController calls when playing back a
 *	replay file. Note that implementors must also implement the
 *	ServerMessageHandler interface.
 */
class ReplayControllerHandler : public ServerMessageHandler
{
public:
	/**
	 *	Destructor.
	 */
	virtual ~ReplayControllerHandler() {}


	/**
	 *	This method is called when the replay data file is of an unsupported
	 *	version.
	 *
	 *	@param controller 	The ReplayController instance.
	 *	@param version 		The version the replay file is marked as.
	 */
	virtual void onReplayControllerGotBadVersion( ReplayController & controller,
		const ClientServerProtocolVersion & version ) = 0;

	/**
	 *	This method is called when the header of the replay data file has been
	 *	parsed.
	 *
	 *	This method is primarily used by ReplayController to query whether the
	 *	parsed defs digest is valid.
	 *
	 *	@param controller 	The ReplayController instance.
	 *	@param header 		The header data.
	 *
	 *	@return 			True if replay should proceed, false otherwise.
	 *						This is typically based on the defs digest in the
	 *						header.
	 */
	virtual bool onReplayControllerReadHeader(
		ReplayController & controller, const ReplayHeader & header ) = 0;


	/**
	 *	This method is called when corrupted tick data was read from the
	 *	replay data file.
	 *
	 *	@param controller 	The ReplayController instance.
	 */
	virtual void onReplayControllerGotCorruptedData() = 0;


	/**
	 *	This method is called when the meta-data block of the replay data file
	 *	has been parsed.
	 *
	 *	The callback can elect to reject the replay data file based on the
	 *	contents of the meta-data.
	 *
	 *	@param controller 	The ReplayController instance.
	 *	@param metaData 	The meta-data.
	 *
	 *	@return 			True if replay should proceed, false otherwise.
	 */
	virtual bool onReplayControllerReadMetaData(
		ReplayController & controller, const ReplayMetaData & metaData ) = 0;


	/**
	 *	This method is called when a replayed entity transitions between being
	 *	a non-player entity to a player-entity, or vice versa.
	 *
	 *	When an entity becomes a player, it is taken as implicit that entities
	 *	within the AoI radius will be in the AoI of the player. Any exceptions
	 *	will have onReplayControllerAoIChange() called after the notification
	 *	that an entity has become a player, for example in the case of withheld
	 *	entities and entities with a non-zero AppealRadius outside the AoI
	 *	range.
	 *
	 *	@param controller 		The replay controller.
	 *	@param playerID 		The player entity.
	 *	@param hasBecomePlayer 	If true, the player has just become a player,
	 *							otherwise it is now a non-player.
	 */
	virtual void onReplayControllerEntityPlayerStateChange(
		ReplayController & controller, EntityID playerID,
		bool hasBecomePlayer )
	{}


	/**
	 *	This method is called when an entity moves into or out of a player's
	 *	AoI.
	 *
	 *	@param controller 		The ReplayController instance.
	 *	@param playerID 		The player ID.
	 *	@param entityID 		The entity ID entering or leaving the AoI.
	 *	@param hasEnteredAoI	If true, then the entity has entered the
	 *							player's AoI, if false, the entity has left the
	 *							player's AoI.
	 */
	virtual void onReplayControllerAoIChange(
			ReplayController & controller, EntityID playerID,
			EntityID entityID, bool hasEnteredAoI )
	{}


	/**
	 *	This method is called when the replay controller has an error.
	 *
	 *	@param controller 	The ReplayController instance.
	 *	@param error	 	The error description.
	 */
	virtual void onReplayControllerError(
		ReplayController & controller, const BW::string & error ) = 0;


	/**
	 *	This method is called when the replay controller has finished playing
	 *	back the file.
	 *
	 *	@param controller 	The ReplayController instance.
	 */
	virtual void onReplayControllerFinish( ReplayController & controller ) = 0;


	/**
	 *	This method is called when the replay controller is being destroyed.
	 *
	 *	@param controller 	The ReplayController instance that will be
	 *						destroyed.
	 */
	virtual void onReplayControllerDestroyed(
		ReplayController & controller ) = 0;


	/**
	 *	This method is called when the replay controller has processed a tick
	 *	of the replay data.
	 *
	 *	@param controller 	The ReplayController instance.
	 *	@param tickTime 	The tick time.
	 */
	virtual void onReplayControllerPostTick( ReplayController & controller,
		GameTime tickTime ) = 0;


	/**
	 *	This method is called when the replay controller
	 *	needs to change total game time.
	 *
	 *	@param controller 	The ReplayController instance.
	 *	@param dTime 	The amount of time to be changed,
	 *		this can be a negative value, which means that
	 *		the current game time needs to be decreased.
	 */
	virtual void onReplayControllerIncreaseTotalTime(
		ReplayController & controller,
		double dTime ) = 0;
};


/**
 *	This class plays back replay data to a ReplayControllerHandler instance.
 */
class ReplayController : public ReferenceCount,
		private IReplayDataFileReaderListener,
		private IReplayTickLoaderListener,
		private BackgroundFileWriterListener
{
public:
	static const GameTime DEFAULT_VOLATILE_INJECTION_PERIOD_TICKS = 5;

	enum ReplayFileType
	{
		// Remove when completed
		FILE_REMOVE,
		// Keep when completed
		FILE_KEEP,
		// File contains the data already, just read from it
		FILE_ALREADY_DOWNLOADED
	};

	ReplayController( TaskManager & taskManager, SpaceID spaceID,
		const EntityDefConstants & entityDefConstants,
		const BW::string & verifyingKey,
		ReplayControllerHandler & handler,
		const BW::string & replayFilePath,
		ReplayFileType replayFileType,
		GameTime volatileInjectionPeriod =
			DEFAULT_VOLATILE_INJECTION_PERIOD_TICKS );

	const BW::string & errorMessage() const { return errorMessage_; }
	void destroy();

	bool isSeekingToTick() const;
	void pausePlayback();
	void resumePlayback();

	void tick( double timeDelta );

	bool addReplayData( BinaryIStream & data );
	bool isReceivingData() const { return isReceiving_; }
	void replayDataFinalise();

	/**
	 *	This method returns the update frequency.
	 */
	int updateFrequency() const
	{
		return gameUpdateFrequency_;
	}

	/**
	 *	This method returns true if the replay file has been loaded.
	 */
	bool isLoaded() const
	{
		return isStreamLoaded_;
	}

	/**
	 *	This method returns the current tick index being played.
	 */
	GameTime getCurrentTick() const
	{
		return currentTick_;
	}

	GameTime getCurrentTickTime() const;
	double getCurrentTickTimeInSeconds() const;

	bool setCurrentTick( GameTime tick );


	GameTime numTicksTotal() const;

	/**
	 *	This method returns the number of ticks loaded.
	 */
	GameTime numTicksLoaded() const
	{
		return numTicksLoaded_;
	}

	/**
	 *	This method returns true if playback is proceeding.
	 */
	bool isPlaying() const
	{
		return isPlayingStream_;
	}

	/**
	 *	This method returns the playback speed scale.
	 */
	float speedScale() const
	{
		return speedScale_;
	}

	/**
	 *	This method sets the playback speed scale.
	 */
	void speedScale( float speedScale )
	{
		speedScale_ = speedScale;
	}


	/**
	 *	This method gets the recording filename
	 */
	const BW::string & replayFilePath() const
	{
		return replayFilePath_;
	}

	void resetEntities();

private:
	virtual ~ReplayController();

	bool processTick( BinaryIStream & tickData );

	void handleEntityCreate( BinaryIStream & tickData );
	void handleEntityVolatile( BinaryIStream & tickData );
	void handleEntityVolatileOnGround( BinaryIStream & tickData );
	void handleEntityMethod( BinaryIStream & tickData );
	void handleEntityPropertyChange( BinaryIStream & tickData );
	void handleEntityPlayerStateChange( BinaryIStream & tickData );
	void handleEntityAoIChange( BinaryIStream & tickData );
	void handleEntityDelete( BinaryIStream & tickData );
	void handleSpaceData( BinaryIStream & tickData );
	void handleError( const BW::string & errorString );
	void handleFinal();
	void moveEntity( EntityID id, SpaceID spaceID, EntityID vehicleID,
		const Position3D & pos, const Vector3 & posError,
		const Direction3D & direction );

	// Overrides from BackgroundFileWriterListener
	void onBackgroundFileWritingError( IBackgroundFileWriter & writer ) /* override */;
	void onBackgroundFileWritingComplete(
		IBackgroundFileWriter & writer, long filePosition,
		int userData ) /* override */;

	// Overrides from IReplayTickLoaderListener
	void onReplayTickLoaderReadHeader( const ReplayHeader & header,
		GameTime firstGameTime ) /* override */;
	void onReplayTickLoaderPrependTicks( ReplayTickData * pStart,
		ReplayTickData * pEnd, uint totalTicks ) /* override */;
	void onReplayTickLoaderAppendTicks( ReplayTickData * pStart,
		ReplayTickData * pEnd, uint totalTicks ) /* override */;
	void onReplayTickLoaderError( ReplayTickLoader::ErrorType errorType,
		const BW::string & errorString ) /* override */;


	// Overrides from IReplayDataFileListener
	// These are used for the pTickCounter_. The tick loader will do the actual
	// tick reading and verifying.

	virtual bool onReplayDataFileReaderHeader(
			ReplayDataFileReader & reader,
			const ReplayHeader & header,
			BW::string & errorString )  /* override */
	{ return true; }

	virtual bool onReplayDataFileReaderMetaData( ReplayDataFileReader & reader,
		const ReplayMetaData & metaData, BW::string & errorString ) /* override */;

	virtual bool onReplayDataFileReaderTickData( ReplayDataFileReader & reader,
		GameTime time, bool isCompressed, BinaryIStream & stream,
		BW::string & errorString ) /* override */;

	virtual void onReplayDataFileReaderError( ReplayDataFileReader & reader,
		ErrorType errorType, const BW::string & errorDescription ) /* override */;

	virtual void onReplayDataFileReaderDestroyed(
		ReplayDataFileReader & reader )  /* override */
	{}


	EntityDefConstants 		entityDefConstants_;

	ReplayControllerHandler & handler_;
	BW::string				errorMessage_;
	BackgroundFileWriterPtr pBufferFileWriter_;
	TaskManager &			taskManager_;

	ReplayDataFileReader *	pTickCounter_;
	ReplayTickLoaderPtr 	pTickLoader_;
	bool 					haveReadHeader_;

	ReplayTickData * 		pNextRead_;
	ReplayTickData * 		pLastRead_;

	SpaceID 				spaceID_;
	uint 					gameUpdateFrequency_;
	double 					timeSinceLastTick_;
	GameTime				firstGameTime_;
	GameTime 				currentTick_;
	int 					seekToTick_;
	GameTime 				numTicksLoaded_;
	GameTime 				numTicksReported_;
	float 					speedScale_;
	bool					isPlayingStream_;
	bool					isStreamLoaded_;
	bool					isReceiving_;
	bool					shouldDelayVolatileInjection_;
	bool					isLive_;
	bool					isDestroyed_;
	ReplayFileType 			replayFileType_;
	BW::string 				replayFilePath_;

	std::auto_ptr< EntityVolatileCache >
							pVolatileCache_;
};

typedef SmartPointer<ReplayController> ReplayControllerPtr;


BW_END_NAMESPACE


#endif // REPLAY_CONTROLLER_HPP

