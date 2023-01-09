#ifndef BW_REPLAY_EVENT_HANDLER
#define BW_REPLAY_EVENT_HANDLER

#include "network/basictypes.hpp"


BW_BEGIN_NAMESPACE

class BWEntity;
class ReplayHeader;
class ReplayMetaData;

/**
 *	This class is the interface for replay event handlers
 */
class BWReplayEventHandler
{
public:
	enum ReplayTerminatedReason
	{
		REPLAY_STOPPED_PLAYBACK,
		REPLAY_ABORTED_VERSION_MISMATCH,
		REPLAY_ABORTED_ENTITYDEF_MISMATCH,
		REPLAY_ABORTED_METADATA_REJECTED,
		REPLAY_ABORTED_CORRUPTED_DATA
	};

	virtual ~BWReplayEventHandler() {}

	/**
	 *	This method receives a notification when the replay header has been
	 *	read successfully. This is the first event in a successful replay
	 *	playback.
	 */
	virtual void onReplayHeaderRead( const ReplayHeader & header ) = 0;

	/**
	 *	This method receives the replay's metadata after the header is read.
	 *
	 *	@param	metaData	The ReplayMetaData contained in the replay.
	 *	@return	false if the metadata indicated that playback should be
	 *			aborted, true otherwise.
	 */
	virtual bool onReplayMetaData( const ReplayMetaData & metaData ) = 0;


	/**
	 *	This method receives a notification when the replay playback reaches
	 *	an error in its recording. No more events will be raised unless
	 *	the replay is rewound, but playback will not be aborted.
	 *
	 *	@param	error		The error description.
	 */
	virtual void onReplayError( const BW::string & error ) = 0;


	/**
	 *	This method receives a notification when the replay playback has
	 *	reached the end of its recording. No more events will be raised
	 *	unless the replay is rewound, but playback will not be aborted.
	 */
	virtual void onReplayReachedEnd() = 0;

	/**
	 *	This method is called when an entity in the currently playing
	 *	replay gains or loses a client.
	 *
	 *	@param	pEntity	The BWEntity which gained or lost a client.
	 */
	virtual void onReplayEntityClientChanged( const BWEntity * pEntity ) = 0;

	/**
	 *	This method is called when an entity enters or leaves the AoI of an
	 *	entity with a client attached.
	 *
	 *	@param	pWitness	The BWEntity whose AoI was entered or left
	 *	@param	pEntity		The BWEntity which entered or left AoI
	 *	@param	isEnter		true if pEntity entered pWitness's AoI, false if
	 *						pEntity left pWitness's AoI.
	 */
	virtual void onReplayEntityAoIChanged( const BWEntity * pWitness,
		const BWEntity * pEntity, bool isEnter ) = 0;

	/**
	 *	This method is called after each tick of the replay has been played
	 *	back.
	 *
	 *	@param	currentTick	The (0-based) replay tick that has just been played.
	 *						Tick 0 will contain all the data for the space as it
	 *						was known when the recording was started.
	 *	@param	totalTicks	The number of ticks currently known in the replay.
	 *						This value will increase when streaming a replay as
	 *						new data is received.
	 *
	 *	Note that a new replay will not advance past tick 0 until
	 *	BWReplayConnection::onInitialDataProcessed is called, usually after this
	 *	method has been called for the completion of tick 0.
	 */
	virtual void onReplayTicked( GameTime currentTick,
		GameTime totalTicks ) = 0;

	/**
	 *	This method is called when the replay is seeking, to advance the current
	 *	game time.
	 *
	 *	@param	dTime		The amount of time to skip forward. This should be
	 *						included in the next BWConnection::update() call's
	 *						input dTime.
	 */
	virtual void onReplaySeek( double dTime ) = 0;

	/**
	 *	This method is called when the replay is being terminated. It may be
	 *	called at any stage of the replay playback.
	 *
	 * 	@param	reason		The ReplayTerminatedReason for terminating the
	 * 						playback.
	 */
	virtual void onReplayTerminated( ReplayTerminatedReason reason ) = 0;
};

BW_END_NAMESPACE

#endif // BW_REPLAY_EVENT_HANDLER
