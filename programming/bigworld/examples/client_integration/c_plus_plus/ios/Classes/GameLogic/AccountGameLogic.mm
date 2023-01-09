#include "AccountGameLogic.hpp"

#import "ApplicationDelegate.h"


// -----------------------------------------------------------------------------
// Section: ClientMethod implementations 
// -----------------------------------------------------------------------------


/**
 *	Constructor.
 *
 */
AccountGameLogic::AccountGameLogic( const Account * pEntity ) :
		AccountExtension( pEntity ),
		charNameSuffix_( 0 )
{}


/**
 *	This method implements the ClientMethod logMessage.
 */
void AccountGameLogic::logMessage( 
			const BW::string & msg )
{
	// DEBUG_MSG( "AccountGameLogic::set_logMessage\n" );
}



/**
 *	This method implements the ClientMethod failLogon.
 */
void AccountGameLogic::failLogon( 
			const BW::string & message )
{
	// DEBUG_MSG( "AccountGameLogic::set_failLogon\n" );
}



/**
 *	This method implements the ClientMethod switchRealm.
 */
void AccountGameLogic::switchRealm( 
			const BW::string & selectedRealm,
			const BW::SequenceValueType< CharacterInfo > & characterList )
{
	// DEBUG_MSG( "AccountGameLogic::set_switchRealm\n" );
	
	// If we have a character on this account, enter the world with it,
	// otherwise automatically create a character using the account name.
	// This is different than the normal Client where we create the character
	// separately.
	
	const Account * pEntity = this->pEntity();
	
	if (!characterList.empty())
	{
		pEntity->base().characterBeginPlay( characterList[0].name );
	}
	else
	{
		pEntity->base().createNewAvatar( pEntity->accountName() );
	}
}



/**
 *	This method implements the ClientMethod createCharacterCallback.
 */
void AccountGameLogic::createCharacterCallback( 
			const BW::uint8 & succeeded,
			const BW::string & msg,
			const BW::SequenceValueType< CharacterInfo > & characterList )
{
	// DEBUG_MSG( "AccountGameLogic::set_createCharacterCallback\n" );
	// always attempt to login the first character, no matter who it is.
	// NOTE: there really should be either 1 or none at this point.
	
	const Account * pEntity = this->pEntity();
	
	if (!characterList.empty())
	{
		charNameSuffix_ = 0;
		pEntity->base().characterBeginPlay( characterList[0].name );
	}
	// NOTE: failed to create the char? attempt to create another with a
	// modified name.
	else if (!succeeded && charNameSuffix_ < 10)
	{
		BW::stringstream stream;
		stream << pEntity->accountName() << charNameSuffix_ << std::endl;
		pEntity->base().createNewAvatar( stream.str() );
		
		charNameSuffix_++;
	}
	else
	{
		UIAlertView * alert = [UIAlertView alloc];
		[alert initWithTitle: @"Create Character"
					 message: [NSString stringWithUTF8String:msg.c_str()]
					delegate: nil
		   cancelButtonTitle: nil
		   otherButtonTitles: @"OK", nil];
		[alert autorelease];
		[alert show];
		
		charNameSuffix_ = 0;
		
		// stop the logon if we failed to create a character.
		[[ApplicationDelegate sharedDelegate] onEndLogin];
	}
}


/*
 *	Override from EntityExtension.
 */
void AccountGameLogic::onBecomePlayer()
{
	this->EntityExtension::onBecomePlayer();
	
	// use the first realm we find
	if (!this->realms().empty())
	{
		this->pEntity()->base().selectRealm( this->realms()[0].first );
	}
	else
	{
		UIAlertView * alert = [UIAlertView alloc];
		[alert initWithTitle: @"Create Character"
					 message: @"No Realms found on server."
					delegate: nil
		   cancelButtonTitle: nil
		   otherButtonTitles: @"OK", nil];
		[alert autorelease];
		[alert show];
		
		// stop the logon if we failed to find a valid realm.
		[[ApplicationDelegate sharedDelegate] onEndLogin];
	}
	
}


/**
 *	This method returns the available realms.
 */
const AccountGameLogic::RealmInfos & AccountGameLogic::realms() const
{
	static RealmInfos s_realms;
	static bool isInitialised = false;
	
	if (!isInitialised)
	{
#if 0
		// NOTE: these are the other valid realms.
		// for the IOS demo we do not want to have to select a realm so
		// these have been removed.
		s_realms.push_back( RealmInfo( "fantasy", "Fantasy Realm" ) );
		s_realms.push_back( RealmInfo( "urban", "Urban Realm" ) );
#endif
		s_realms.push_back( RealmInfo( "minspec", "MinSpec Realm" ) );
		
		isInitialised = true;
	}
	
	return s_realms;
}


// -----------------------------------------------------------------------------
// Section: Property setter callbacks
// -----------------------------------------------------------------------------

// AccountGameLogic.mm
