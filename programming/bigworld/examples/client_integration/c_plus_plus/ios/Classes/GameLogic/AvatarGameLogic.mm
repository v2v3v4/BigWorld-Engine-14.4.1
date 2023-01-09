#include "AvatarGameLogic.hpp"

#import "CCDirector.h"
#import "ApplicationDelegate.h"


/*
 *	Override from EntityExtensions.
 */
void AvatarGameLogic::onBecomePlayer()
{
	this->EntityExtension::onBecomePlayer();
	
	ApplicationDelegate* delegate = [ApplicationDelegate sharedDelegate];
	
	[delegate onEndLogin];
	[delegate hideMenuScreen];
}


// -----------------------------------------------------------------------------
// Section: ClientMethod implementations 
// -----------------------------------------------------------------------------

/**
 *	This method implements the ClientMethod shonk.
 */
void AvatarGameLogic::shonk( 
			const BW::int32 & opponentID,
			const BW::uint8 & oppAction,
			const BW::uint8 & ownAction )
{
	// DEBUG_MSG( "AvatarGameLogic::set_shonk\n" );
}



/**
 *	This method implements the ClientMethod handshake.
 */
void AvatarGameLogic::handshake( 
			const BW::int32 & partnerID )
{
	// DEBUG_MSG( "AvatarGameLogic::set_handshake\n" );
}



/**
 *	This method implements the ClientMethod pullUp.
 */
void AvatarGameLogic::pullUp( 
			const BW::int32 & partnerID )
{
	// DEBUG_MSG( "AvatarGameLogic::set_pullUp\n" );
}



/**
 *	This method implements the ClientMethod pushUp.
 */
void AvatarGameLogic::pushUp( 
			const BW::int32 & partnerID )
{
	// DEBUG_MSG( "AvatarGameLogic::set_pushUp\n" );
}



/**
 *	This method implements the ClientMethod didGesture.
 */
void AvatarGameLogic::didGesture( 
			const BW::uint8 & actionID )
{
	// DEBUG_MSG( "AvatarGameLogic::set_didGesture\n" );
}



/**
 *	This method implements the ClientMethod fireWeapon.
 */
void AvatarGameLogic::fireWeapon( 
			const BW::int32 & shotid,
			const BW::int8 & lock )
{
	// DEBUG_MSG( "AvatarGameLogic::set_fireWeapon\n" );
}



/**
 *	This method implements the ClientMethod castSpell.
 */
void AvatarGameLogic::castSpell( 
			const BW::int32 & targetID,
			const BW::Vector3 & hitLocn,
			const BW::uint8 & materialKind )
{
	// DEBUG_MSG( "AvatarGameLogic::set_castSpell\n" );
}



/**
 *	This method implements the ClientMethod fragged.
 */
void AvatarGameLogic::fragged( 
			const BW::int32 & shooterID )
{
	// DEBUG_MSG( "AvatarGameLogic::set_fragged\n" );
}



/**
 *	This method implements the ClientMethod eat.
 */
void AvatarGameLogic::eat( 
			const BW::int32 & item )
{
	// DEBUG_MSG( "AvatarGameLogic::set_eat\n" );
}



/**
 *	This method implements the ClientMethod assail.
 */
void AvatarGameLogic::assail(  )
{
	// DEBUG_MSG( "AvatarGameLogic::set_assail\n" );
}



/**
 *	This method implements the ClientMethod salida.
 */
void AvatarGameLogic::salida( 
			const BW::int8 & result )
{
	// DEBUG_MSG( "AvatarGameLogic::set_salida\n" );
}



/**
 *	This method implements the ClientMethod chat.
 */
void AvatarGameLogic::chat( 
			const BW::string & msg )
{
	// DEBUG_MSG( "AvatarGameLogic::set_chat\n" );
}



/**
 *	This method implements the ClientMethod onAddNote.
 */
void AvatarGameLogic::onAddNote( 
			const BW::uint32 & id )
{
	// DEBUG_MSG( "AvatarGameLogic::set_onAddNote\n" );
}



/**
 *	This method implements the ClientMethod onGetNotes.
 */
void AvatarGameLogic::onGetNotes( 
			const BW::SequenceValueType< Note > & noteList )
{
	// DEBUG_MSG( "AvatarGameLogic::set_onGetNotes\n" );
}



/**
 *	This method implements the ClientMethod clientOnCreateCellFailure.
 */
void AvatarGameLogic::clientOnCreateCellFailure(  )
{
	// DEBUG_MSG( "AvatarGameLogic::set_clientOnCreateCellFailure\n" );
}



/**
 *	This method implements the ClientMethod newFriendsList.
 */
void AvatarGameLogic::newFriendsList( 
			const BW::SequenceValueType< BW::string > & friendsList )
{
	// DEBUG_MSG( "AvatarGameLogic::set_newFriendsList\n" );
}



/**
 *	This method implements the ClientMethod onAddedFriend.
 */
void AvatarGameLogic::onAddedFriend( 
			const BW::string & friendName,
			const BW::uint8 & online )
{
	// DEBUG_MSG( "AvatarGameLogic::set_onAddedFriend\n" );
}



/**
 *	This method implements the ClientMethod setFriendStatus.
 */
void AvatarGameLogic::setFriendStatus( 
			const BW::uint8 & index,
			const BW::uint8 & isOnline )
{
	// DEBUG_MSG( "AvatarGameLogic::set_setFriendStatus\n" );
}



/**
 *	This method implements the ClientMethod onReceiveMessageFromAdmirer.
 */
void AvatarGameLogic::onReceiveMessageFromAdmirer( 
			const BW::string & admirerName,
			const BW::string & message )
{
	// DEBUG_MSG( "AvatarGameLogic::set_onReceiveMessageFromAdmirer\n" );
}



/**
 *	This method implements the ClientMethod onRcvFriendInfo.
 */
void AvatarGameLogic::onRcvFriendInfo( 
			const BW::string & friendName,
			const BW::string & info )
{
	// DEBUG_MSG( "AvatarGameLogic::set_onRcvFriendInfo\n" );
}



/**
 *	This method implements the ClientMethod showMessage.
 */
void AvatarGameLogic::showMessage( 
			const BW::uint8 & type,
			const BW::string & source,
			const BW::string & message )
{
	// DEBUG_MSG( "AvatarGameLogic::set_showMessage\n" );
}



/**
 *	This method implements the ClientMethod onTeleportComplete.
 */
void AvatarGameLogic::onTeleportComplete(  )
{
	// DEBUG_MSG( "AvatarGameLogic::set_onTeleportComplete\n" );
}



/**
 *	This method implements the ClientMethod tradeDeny.
 */
void AvatarGameLogic::tradeDeny(  )
{
	// DEBUG_MSG( "AvatarGameLogic::set_tradeDeny\n" );
}



/**
 *	This method implements the ClientMethod tradeOfferItemNotify.
 */
void AvatarGameLogic::tradeOfferItemNotify( 
			const BW::int32 & itemType )
{
	// DEBUG_MSG( "AvatarGameLogic::set_tradeOfferItemNotify\n" );
}



/**
 *	This method implements the ClientMethod tradeOfferItemDeny.
 */
void AvatarGameLogic::tradeOfferItemDeny( 
			const BW::int32 & tradeItemLock )
{
	// DEBUG_MSG( "AvatarGameLogic::set_tradeOfferItemDeny\n" );
}



/**
 *	This method implements the ClientMethod tradeCommitNotify.
 */
void AvatarGameLogic::tradeCommitNotify( 
			const BW::uint8 & success,
			const BW::int32 & outItemsLock,
			const BW::SequenceValueType< BW::int32 > & outItemsSerial,
			const BW::int16 & outGoldPieces,
			const BW::SequenceValueType< BW::int32 > & inItemsTypes,
			const BW::SequenceValueType< BW::int32 > & inItemsSerials,
			const BW::int16 & inGoldPieces )
{
	// DEBUG_MSG( "AvatarGameLogic::set_tradeCommitNotify\n" );
}



/**
 *	This method implements the ClientMethod tradeAcceptNotify.
 */
void AvatarGameLogic::tradeAcceptNotify( 
			const BW::uint8 & accepted )
{
	// DEBUG_MSG( "AvatarGameLogic::set_tradeAcceptNotify\n" );
}



/**
 *	This method implements the ClientMethod itemsLockNotify.
 */
void AvatarGameLogic::itemsLockNotify( 
			const BW::int32 & lockHandle,
			const BW::SequenceValueType< BW::int32 > & itemsSerials,
			const BW::int16 & goldPieces )
{
	// DEBUG_MSG( "AvatarGameLogic::set_itemsLockNotify\n" );
}



/**
 *	This method implements the ClientMethod itemsUnlockNotify.
 */
void AvatarGameLogic::itemsUnlockNotify( 
			const BW::uint8 & success,
			const BW::int32 & lockHandle )
{
	// DEBUG_MSG( "AvatarGameLogic::set_itemsUnlockNotify\n" );
}



/**
 *	This method implements the ClientMethod commerceStartDeny.
 */
void AvatarGameLogic::commerceStartDeny(  )
{
	// DEBUG_MSG( "AvatarGameLogic::set_commerceStartDeny\n" );
}



/**
 *	This method implements the ClientMethod commerceItemsNotify.
 */
void AvatarGameLogic::commerceItemsNotify( 
			const BW::SequenceValueType< InventoryEntry > & items )
{
	// DEBUG_MSG( "AvatarGameLogic::set_commerceItemsNotify\n" );
}



/**
 *	This method implements the ClientMethod pickUpNotify.
 */
void AvatarGameLogic::pickUpNotify( 
			const BW::int32 & droppedItemID )
{
	// DEBUG_MSG( "AvatarGameLogic::set_pickUpNotify\n" );
}



/**
 *	This method implements the ClientMethod pickUpResponse.
 */
void AvatarGameLogic::pickUpResponse( 
			const BW::uint8 & success,
			const BW::int32 & droppedItemID,
			const BW::int32 & itemSerial )
{
	// DEBUG_MSG( "AvatarGameLogic::set_pickUpResponse\n" );
}



/**
 *	This method implements the ClientMethod dropDeny.
 */
void AvatarGameLogic::dropDeny(  )
{
	// DEBUG_MSG( "AvatarGameLogic::set_dropDeny\n" );
}



/**
 *	This method implements the ClientMethod loadGenMeth1.
 */
void AvatarGameLogic::loadGenMeth1( 
			const BW::string & strArg )
{
	// DEBUG_MSG( "AvatarGameLogic::set_loadGenMeth1\n" );
}



/**
 *	This method implements the ClientMethod loadGenMeth2.
 */
void AvatarGameLogic::loadGenMeth2( 
			const BW::uint32 & intArg,
			const BW::string & strArg )
{
	// DEBUG_MSG( "AvatarGameLogic::set_loadGenMeth2\n" );
}



/**
 *	This method implements the ClientMethod loadGenMeth3.
 */
void AvatarGameLogic::loadGenMeth3( 
			const float & floatArg1,
			const float & floatArg2,
			const float & floatArg3 )
{
	// DEBUG_MSG( "AvatarGameLogic::set_loadGenMeth3\n" );
}



/**
 *	This method implements the ClientMethod loadGenMeth4.
 */
void AvatarGameLogic::loadGenMeth4( 
			const BW::SequenceValueType< BW::uint8 > & arrayArg )
{
	// DEBUG_MSG( "AvatarGameLogic::set_loadGenMeth4\n" );
}



/**
 *	This method implements the ClientMethod entitySummoned.
 */
void AvatarGameLogic::entitySummoned( 
			const BW::int32 & entityID,
			const BW::string & typeName )
{
	// DEBUG_MSG( "AvatarGameLogic::set_entitySummoned\n" );
}



/**
 *	This method implements the ClientMethod teleportTo.
 */
void AvatarGameLogic::teleportTo( 
			const BW::string & spaceName,
			const BW::string & pointName )
{
	// DEBUG_MSG( "AvatarGameLogic::set_teleportTo\n" );
}



/**
 *	This method implements the ClientMethod addInfoMsg.
 */
void AvatarGameLogic::addInfoMsg( 
			const BW::string & msg )
{
	// DEBUG_MSG( "AvatarGameLogic::set_addInfoMsg\n" );
}



/**
 *	This method implements the ClientMethod onXmppRegisterWithTransport.
 */
void AvatarGameLogic::onXmppRegisterWithTransport( 
			const BW::string & transport )
{
	// DEBUG_MSG( "AvatarGameLogic::set_onXmppRegisterWithTransport\n" );
}



/**
 *	This method implements the ClientMethod onXmppDeregisterWithTransport.
 */
void AvatarGameLogic::onXmppDeregisterWithTransport( 
			const BW::string & transport )
{
	// DEBUG_MSG( "AvatarGameLogic::set_onXmppDeregisterWithTransport\n" );
}



/**
 *	This method implements the ClientMethod onXmppMessage.
 */
void AvatarGameLogic::onXmppMessage( 
			const BW::string & friendID,
			const BW::string & transport,
			const BW::string & message )
{
	// DEBUG_MSG( "AvatarGameLogic::set_onXmppMessage\n" );
}



/**
 *	This method implements the ClientMethod onXmppPresence.
 */
void AvatarGameLogic::onXmppPresence( 
			const BW::string & friendID,
			const BW::string & transport,
			const BW::string & presence )
{
	// DEBUG_MSG( "AvatarGameLogic::set_onXmppPresence\n" );
}



/**
 *	This method implements the ClientMethod onXmppRoster.
 */
void AvatarGameLogic::onXmppRoster( 
			const BW::SequenceValueType< XMPPFriend > & roster )
{
	// DEBUG_MSG( "AvatarGameLogic::set_onXmppRoster\n" );
}



/**
 *	This method implements the ClientMethod onXmppRosterItemAdd.
 */
void AvatarGameLogic::onXmppRosterItemAdd( 
			const BW::string & friendID,
			const BW::string & transport )
{
	// DEBUG_MSG( "AvatarGameLogic::set_onXmppRosterItemAdd\n" );
}



/**
 *	This method implements the ClientMethod onXmppRosterItemDelete.
 */
void AvatarGameLogic::onXmppRosterItemDelete( 
			const BW::string & friendID,
			const BW::string & transport )
{
	// DEBUG_MSG( "AvatarGameLogic::set_onXmppRosterItemDelete\n" );
}



/**
 *	This method implements the ClientMethod onXmppError.
 */
void AvatarGameLogic::onXmppError( 
			const BW::string & message )
{
	// DEBUG_MSG( "AvatarGameLogic::set_onXmppError\n" );
}

// -----------------------------------------------------------------------------
// Section: Property setter callbacks
// -----------------------------------------------------------------------------

/**
 *	This method implements the setter callback for the property modeTarget.
 */
void AvatarGameLogic::set_modeTarget( const BW::int32 & oldValue )
{
	// DEBUG_MSG( "AvatarGameLogic::set_modeTarget\n" );
}

/**
 *	This method implements the setter callback for the property playerName.
 */
void AvatarGameLogic::set_playerName( const BW::string & oldValue )
{
	// DEBUG_MSG( "AvatarGameLogic::set_playerName\n" );
}

/**
 *	This method implements the setter callback for the property avatarModel.
 */
void AvatarGameLogic::set_avatarModel( const PackedAvatarModel & oldValue )
{
	// DEBUG_MSG( "AvatarGameLogic::set_avatarModel\n" );
}

/**
 *	This method implements the property setter callback method for the
 *	property avatarModel. 
 *	It is called when a single sub-property element has changed. The
 *	location of the element is described in the given change path.
 */
void AvatarGameLogic::setNested_avatarModel( const BW::NestedPropertyChange::Path & path, 
		const PackedAvatarModel & oldValue )
{
	this->set_avatarModel( oldValue );
}

/**
 *	This method implements the property setter callback method for the
 *	property avatarModel. 
 *	It is called when a single sub-property slice has changed. The
 *	location of the ARRAY element is described in the given change path,
 *	and the slice within that element is described in the two given slice
 *	indices. 
 */
void AvatarGameLogic::setSlice_avatarModel( const BW::NestedPropertyChange::Path & path,
		int startIndex, int endIndex, 
		const PackedAvatarModel & oldValue )
{
	this->set_avatarModel( oldValue );
}


/**
 *	This method implements the setter callback for the property modelNumber.
 */
void AvatarGameLogic::set_modelNumber( const BW::uint16 & oldValue )
{
	// DEBUG_MSG( "AvatarGameLogic::set_modelNumber\n" );
}

/**
 *	This method implements the setter callback for the property customColour.
 */
void AvatarGameLogic::set_customColour( const BW::int8 & oldValue )
{
	// DEBUG_MSG( "AvatarGameLogic::set_customColour\n" );
}

/**
 *	This method implements the setter callback for the property rightHand.
 */
void AvatarGameLogic::set_rightHand( const BW::int32 & oldValue )
{
	// DEBUG_MSG( "AvatarGameLogic::set_rightHand\n" );
}

/**
 *	This method implements the setter callback for the property shoulder.
 */
void AvatarGameLogic::set_shoulder( const BW::int32 & oldValue )
{
	// DEBUG_MSG( "AvatarGameLogic::set_shoulder\n" );
}

/**
 *	This method implements the setter callback for the property rightHip.
 */
void AvatarGameLogic::set_rightHip( const BW::int32 & oldValue )
{
	// DEBUG_MSG( "AvatarGameLogic::set_rightHip\n" );
}

/**
 *	This method implements the setter callback for the property leftHip.
 */
void AvatarGameLogic::set_leftHip( const BW::int32 & oldValue )
{
	// DEBUG_MSG( "AvatarGameLogic::set_leftHip\n" );
}

/**
 *	This method implements the setter callback for the property healthPercent.
 */
void AvatarGameLogic::set_healthPercent( const BW::int8 & oldValue )
{
	// DEBUG_MSG( "AvatarGameLogic::set_healthPercent\n" );
}

/**
 *	This method implements the setter callback for the property frags.
 */
void AvatarGameLogic::set_frags( const BW::int32 & oldValue )
{
	// DEBUG_MSG( "AvatarGameLogic::set_frags\n" );
}

/**
 *	This method implements the setter callback for the property mode.
 */
void AvatarGameLogic::set_mode( const BW::int8 & oldValue )
{
	// DEBUG_MSG( "AvatarGameLogic::set_mode\n" );
}

/**
 *	This method implements the setter callback for the property stance.
 */
void AvatarGameLogic::set_stance( const BW::int8 & oldValue )
{
	// DEBUG_MSG( "AvatarGameLogic::set_stance\n" );
}

/**
 *	This method implements the setter callback for the property cellBounds.
 */
void AvatarGameLogic::set_cellBounds( const BW::SequenceValueType< float > & oldValue )
{
	// DEBUG_MSG( "AvatarGameLogic::set_cellBounds\n" );
}

/**
 *	This method implements the property setter callback method for the
 *	property cellBounds. 
 *	It is called when a single sub-property element has changed. The
 *	location of the element is described in the given change path.
 */
void AvatarGameLogic::setNested_cellBounds( const BW::NestedPropertyChange::Path & path, 
		const BW::SequenceValueType< float > & oldValue )
{
	this->set_cellBounds( oldValue );
}

/**
 *	This method implements the property setter callback method for the
 *	property cellBounds. 
 *	It is called when a single sub-property slice has changed. The
 *	location of the ARRAY element is described in the given change path,
 *	and the slice within that element is described in the two given slice
 *	indices. 
 */
void AvatarGameLogic::setSlice_cellBounds( const BW::NestedPropertyChange::Path & path,
		int startIndex, int endIndex, 
		const BW::SequenceValueType< float > & oldValue )
{
	this->set_cellBounds( oldValue );
}


/**
 *	This method implements the setter callback for the property cellAppID.
 */
void AvatarGameLogic::set_cellAppID( const BW::int32 & oldValue )
{
	// DEBUG_MSG( "AvatarGameLogic::set_cellAppID\n" );
}

/**
 *	This method implements the setter callback for the property entityInfo.
 */
void AvatarGameLogic::set_entityInfo( const BW::uint32 & oldValue )
{
	// DEBUG_MSG( "AvatarGameLogic::set_entityInfo\n" );
}

// AvatarGameLogic.mm
