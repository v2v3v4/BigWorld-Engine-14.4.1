#ifndef AVATAR_GAME_LOGIC_HPP
#define AVATAR_GAME_LOGIC_HPP

#include "EntityExtensions/AvatarExtension.hpp"


/**
 *	This class implements the app-specific logic for the Avatar entity.
 */
class AvatarGameLogic : public AvatarExtension
{
public:
	AvatarGameLogic( const Avatar * pEntity ) : AvatarExtension( pEntity ) {}

	// Client methods

	virtual void shonk(
			const BW::int32 & opponentID,
			const BW::uint8 & oppAction,
			const BW::uint8 & ownAction );

	virtual void handshake(
			const BW::int32 & partnerID );

	virtual void pullUp(
			const BW::int32 & partnerID );

	virtual void pushUp(
			const BW::int32 & partnerID );

	virtual void didGesture(
			const BW::uint8 & actionID );

	virtual void fireWeapon(
			const BW::int32 & shotid,
			const BW::int8 & lock );

	virtual void castSpell(
			const BW::int32 & targetID,
			const BW::Vector3 & hitLocn,
			const BW::uint8 & materialKind );

	virtual void fragged(
			const BW::int32 & shooterID );

	virtual void eat(
			const BW::int32 & item );

	virtual void assail( );

	virtual void salida(
			const BW::int8 & result );

	virtual void chat(
			const BW::string & msg );

	virtual void onAddNote(
			const BW::uint32 & id );

	virtual void onGetNotes(
			const BW::SequenceValueType< Note > & noteList );

	virtual void clientOnCreateCellFailure( );

	virtual void newFriendsList(
			const BW::SequenceValueType< BW::string > & friendsList );

	virtual void onAddedFriend(
			const BW::string & friendName,
			const BW::uint8 & online );

	virtual void setFriendStatus(
			const BW::uint8 & index,
			const BW::uint8 & isOnline );

	virtual void onReceiveMessageFromAdmirer(
			const BW::string & admirerName,
			const BW::string & message );

	virtual void onRcvFriendInfo(
			const BW::string & friendName,
			const BW::string & info );

	virtual void showMessage(
			const BW::uint8 & type,
			const BW::string & source,
			const BW::string & message );

	virtual void onTeleportComplete( );

	virtual void tradeDeny( );

	virtual void tradeOfferItemNotify(
			const BW::int32 & itemType );

	virtual void tradeOfferItemDeny(
			const BW::int32 & tradeItemLock );

	virtual void tradeCommitNotify(
			const BW::uint8 & success,
			const BW::int32 & outItemsLock,
			const BW::SequenceValueType< BW::int32 > & outItemsSerial,
			const BW::int16 & outGoldPieces,
			const BW::SequenceValueType< BW::int32 > & inItemsTypes,
			const BW::SequenceValueType< BW::int32 > & inItemsSerials,
			const BW::int16 & inGoldPieces );

	virtual void tradeAcceptNotify(
			const BW::uint8 & accepted );

	virtual void itemsLockNotify(
			const BW::int32 & lockHandle,
			const BW::SequenceValueType< BW::int32 > & itemsSerials,
			const BW::int16 & goldPieces );

	virtual void itemsUnlockNotify(
			const BW::uint8 & success,
			const BW::int32 & lockHandle );

	virtual void commerceStartDeny( );

	virtual void commerceItemsNotify(
			const BW::SequenceValueType< InventoryEntry > & items );

	virtual void pickUpNotify(
			const BW::int32 & droppedItemID );

	virtual void pickUpResponse(
			const BW::uint8 & success,
			const BW::int32 & droppedItemID,
			const BW::int32 & itemSerial );

	virtual void dropDeny( );

	virtual void loadGenMeth1(
			const BW::string & strArg );

	virtual void loadGenMeth2(
			const BW::uint32 & intArg,
			const BW::string & strArg );

	virtual void loadGenMeth3(
			const float & floatArg1,
			const float & floatArg2,
			const float & floatArg3 );

	virtual void loadGenMeth4(
			const BW::SequenceValueType< BW::uint8 > & arrayArg );

	virtual void entitySummoned(
			const BW::int32 & entityID,
			const BW::string & typeName );

	virtual void teleportTo(
			const BW::string & spaceName,
			const BW::string & pointName );

	virtual void addInfoMsg(
			const BW::string & msg );

	virtual void onXmppRegisterWithTransport(
			const BW::string & transport );

	virtual void onXmppDeregisterWithTransport(
			const BW::string & transport );

	virtual void onXmppMessage(
			const BW::string & friendID,
			const BW::string & transport,
			const BW::string & message );

	virtual void onXmppPresence(
			const BW::string & friendID,
			const BW::string & transport,
			const BW::string & presence );

	virtual void onXmppRoster(
			const BW::SequenceValueType< XMPPFriend > & roster );

	virtual void onXmppRosterItemAdd(
			const BW::string & friendID,
			const BW::string & transport );

	virtual void onXmppRosterItemDelete(
			const BW::string & friendID,
			const BW::string & transport );

	virtual void onXmppError(
			const BW::string & message );



	// Client property callbacks (optional)


	virtual void set_modeTarget( const BW::int32 & oldValue );

	virtual void set_playerName( const BW::string & oldValue );

	virtual void set_avatarModel( const PackedAvatarModel & oldValue );

	virtual void setNested_avatarModel( const BW::NestedPropertyChange::Path & path,
			const PackedAvatarModel & oldValue );

	virtual void setSlice_avatarModel( const BW::NestedPropertyChange::Path & path,
			int startIndex, int endIndex,
			const PackedAvatarModel & oldValue );

	virtual void set_modelNumber( const BW::uint16 & oldValue );

	virtual void set_customColour( const BW::int8 & oldValue );

	virtual void set_rightHand( const BW::int32 & oldValue );

	virtual void set_shoulder( const BW::int32 & oldValue );

	virtual void set_rightHip( const BW::int32 & oldValue );

	virtual void set_leftHip( const BW::int32 & oldValue );

	virtual void set_healthPercent( const BW::int8 & oldValue );

	virtual void set_frags( const BW::int32 & oldValue );

	virtual void set_mode( const BW::int8 & oldValue );

	virtual void set_stance( const BW::int8 & oldValue );

	virtual void set_cellBounds( const BW::SequenceValueType< float > & oldValue );

	virtual void setNested_cellBounds( const BW::NestedPropertyChange::Path & path,
			const BW::SequenceValueType< float > & oldValue );

	virtual void setSlice_cellBounds( const BW::NestedPropertyChange::Path & path,
			int startIndex, int endIndex,
			const BW::SequenceValueType< float > & oldValue );

	virtual void set_cellAppID( const BW::int32 & oldValue );

	virtual void set_entityInfo( const BW::uint32 & oldValue );
};


#endif // AVATAR_GAME_LOGIC_HPP
