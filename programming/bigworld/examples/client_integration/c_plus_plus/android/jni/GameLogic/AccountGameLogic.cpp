#include "AccountGameLogic.hpp"

#include <ctime>


// -----------------------------------------------------------------------------
// Section: EntityExtension callbacks
// -----------------------------------------------------------------------------

void AccountGameLogic::onBecomePlayer()
{
	this->AccountExtension::onBecomePlayer();

	DEBUG_MSG( "Account::onBecomePlayer: id = %d, player: %s\n",
			this->entity().entityID(),
			(this->entity().isPlayer() ? "true" : "false") );

	this->entity().base().selectRealm( "fantasy" );
}


// -----------------------------------------------------------------------------
// Section: ClientMethod implementations
// -----------------------------------------------------------------------------

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
	DEBUG_MSG( "AccountGameLogic::set_switchRealm\n" );

	static bool isInitialised = false;
	if (!isInitialised)
	{
		srand( int( time( NULL ) ) );

		isInitialised = true;
	}

	if (characterList.size() == 0)
	{
		char buf[6];
		bw_snprintf( buf, sizeof( buf ), "-%04x", int( rand() % 0x10000 ) );
		BW::string newAvatarName( this->entity().accountName() + buf );
		// DEBUG_MSG( "Account::switchRealm: creating new avatar %s\n",
		//	newAvatarName.c_str() );
		this->entity().base().createNewAvatar( newAvatarName );
	}
	else
	{
		// DEBUG_MSG( "Account::switchRealm: using %s\n",
		//	characterList[0].name.c_str() );
		this->entity().base().characterBeginPlay( characterList[0].name );
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
	DEBUG_MSG( "AccountGameLogic::set_createCharacterCallback\n" );

	// DEBUG_MSG( "Account::createCharacterCallback: using %s\n",
	//		characterList[0].name.c_str() );
	if (!succeeded)
	{
		ERROR_MSG( "Account::createCharacterCallback: "
				"Failed to create character: %s\n",
			msg.c_str() );
		return;
	}
	else if (characterList.empty())
	{
		ERROR_MSG( "Account::createCharacterCallback: "
				"Character list is empty, despite succeeding\n" );
		return;
	}

	this->entity().base().characterBeginPlay( characterList[0].name );
}

// -----------------------------------------------------------------------------
// Section: Property setter callbacks
// -----------------------------------------------------------------------------

// AccountGameLogic.cpp
