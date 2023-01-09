#ifndef ACCOUNT_GAME_LOGIC_HPP
#define ACCOUNT_GAME_LOGIC_HPP

#include "EntityExtensions/AccountExtension.hpp"


/**
 *	This class implements the app-specific logic for the Account entity.
 */
class AccountGameLogic : public AccountExtension
{
public:
	AccountGameLogic( const Account * pEntity ) : AccountExtension( pEntity ) {}

	void onBecomePlayer();

	// Client methods

	virtual void logMessage(
			const BW::string & msg );

	virtual void failLogon(
			const BW::string & message );

	virtual void switchRealm(
			const BW::string & selectedRealm,
			const BW::SequenceValueType< CharacterInfo > & characterList );

	virtual void createCharacterCallback(
			const BW::uint8 & succeeded,
			const BW::string & msg,
			const BW::SequenceValueType< CharacterInfo > & characterList );



	// Client property callbacks (optional)

	const Account & entity() const
	{
		return *static_cast< const Account * >( this->pEntity() );
	}
};


#endif // ACCOUNT_GAME_LOGIC_HPP
