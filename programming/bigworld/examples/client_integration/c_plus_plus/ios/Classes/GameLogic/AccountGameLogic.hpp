#ifndef AccountGameLogic_HPP
#define AccountGameLogic_HPP

#include "EntityExtensions/AccountExtension.hpp"

#include <string>
#include <utility>
#include "cstdmf/bw_vector.hpp"


/**
 *	This class implements the app-specific logic for the Account entity.
 */
class AccountGameLogic : public AccountExtension
{
public:
	AccountGameLogic( const Account * pEntity );
	
	// Override from AccountExtension / EntityExtension
	virtual void onBecomePlayer();
	
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

private:
	typedef std::pair< BW::string, BW::string > RealmInfo;
	typedef BW::SequenceValueType< RealmInfo > RealmInfos;
	
	const RealmInfos & realms() const;

	uint charNameSuffix_;
};

#endif // AccountGameLogic_HPP
