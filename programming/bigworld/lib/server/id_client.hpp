#ifndef ID_CLIENT_HPP
#define ID_CLIENT_HPP

#include <queue>
#include "network/channel_owner.hpp"


BW_BEGIN_NAMESPACE

// #define MF_ID_RECYCLING

/**
 * 	This class provides management of IDs.
 */
class IDClient : private Mercury::ShutdownSafeReplyMessageHandler
{
public:
	IDClient();
	virtual ~IDClient() {}

	bool init(
			Mercury::ChannelOwner * pChannelOwner,
			const Mercury::InterfaceElement& getMoreMethod,
			const Mercury::InterfaceElement& putBackMethod,
			size_t criticallyLowSize,
			size_t lowSize,
			size_t desiredSize,
			size_t highSize );

	// return a previously used ID to the pool
	void putUsedID( EntityID );

	// get a new ID
	EntityID getID();

	// we're done.. return all ID's to the upstream pool
	void returnIDs();

protected:
	typedef std::queue<EntityID> IDQueue;

	// this function places n ID's from a stack onto a binary stream
	static void placeIDsOntoStream( size_t n, IDQueue&, BinaryOStream& );
	// this function retrieves ID's from a stream and places them onto
	// a stack
	static void retrieveIDsFromStream( IDQueue&, BinaryIStream& );

	// this contains ID's that are ready to be used
	IDQueue readyIDs_;

	// network stuff
	Mercury::ChannelOwner * pChannelOwner_;

	// this function performs any updates that might be required when
	// this structure changes
	// can pass isEmergency==true if we really really need to get ID's into
	// readyID_'s
	void performUpdates( bool isEmergency );

	// after we have this many ID's, we should consider handing some back
	// to our parent
	size_t highSize_;

	size_t desiredSize_;

	// if we have this few ID's, we should ask our parent to give us some
	size_t lowSize_;
	// and if we get to this few ID's, our sizes are too low, so we
	// should increase them (and hence reduce the likelyhood of getting
	// this low again), and ask loudly for more
	size_t criticallyLowSize_;

private:
	// this is an ID that has recently been released
	// we cannot reuse it for a little bit of time
	struct LockedID
	{
		LockedID( EntityID id );
		// this is when the ID is safe to be unlocked
		uint64 unlockTime_;
		// this is the id to unlock
		EntityID id_;
	};
	// this contains ID's that are currently locked but could
	// be reused in the future
	std::queue<LockedID> lockedIDs_;

protected:
	// have we already asked for id's? (don't be greedy)
	bool pendingRequest_;

private:
	// save runaway high/low sizes
	bool inEmergency_;

	void getMoreIDs();
	bool getMoreIDsBlocking();
	void getMoreIDs( Mercury::ReplyMessageHandler * pHandler );

	void putBackIDs();

	const Mercury::InterfaceElement * pGetMoreMethod_;
	const Mercury::InterfaceElement * pPutBackMethod_;

	void handleMessage( const Mercury::Address& source,
			Mercury::UnpackedMessageHeader& header, BinaryIStream& data,
			void * arg );
	void handleException( const Mercury::NubException& exception, void * arg );
};

BW_END_NAMESPACE

#endif // ID_CLIENT_HPP

