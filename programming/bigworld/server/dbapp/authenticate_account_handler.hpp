#ifndef AUTHENTICATE_ACCOUNT_HANDLER_HPP
#define AUTHENTICATE_ACCOUNT_HANDLER_HPP

#include "db_storage/billing_system.hpp"

#include "network/misc.hpp"


BW_BEGIN_NAMESPACE

class BillingSystem;

namespace Mercury
{
class NetworkInterface;
class UnpackedMessageHeader;
}


/**
 *	This class is used to handle a message to authenticateAccount.
 */
class AuthenticateAccountHandler : public IGetEntityKeyForAccountHandler
{
public:
	virtual ~AuthenticateAccountHandler() {}

	static bool handle( const Mercury::Address & srcAddr,
			const Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data,
			BillingSystem & billingSystem,
			Mercury::NetworkInterface & interface );

private:
	AuthenticateAccountHandler( const Mercury::Address & replyAddr,
			Mercury::ReplyID replyID, Mercury::NetworkInterface & interface );

	// IGetEntityKeyForAccountHandler overrides
	virtual void onGetEntityKeyForAccountSuccess(
			const EntityKey & ekey,
			const BW::string & dataForClient,
			const BW::string & dataForBaseEntity );

	virtual void onGetEntityKeyForAccountLoadFromUsername(
			EntityTypeID typeID,
			const BW::string & username,
			bool shouldCreateNewOnLoadFailure,
			bool shouldRememberNewOnLoadFailure,
			const BW::string & dataForClient,
			const BW::string & dataForBaseEntity );

	virtual void onGetEntityKeyForAccountCreateNew(
			EntityTypeID typeID,
			bool shouldRemember,
			const BW::string & dataForClient,
			const BW::string & dataForBaseEntity );

	virtual void onGetEntityKeyForAccountFailure(
			LogOnStatus status,
			const BW::string & errorMsg );

	void sendFailure( LogOnStatus status, const BW::string & errorMsg );

	Mercury::Address replyAddr_;
	Mercury::ReplyID replyID_;
	Mercury::NetworkInterface & interface_;
};

BW_END_NAMESPACE

#endif // AUTHENTICATE_ACCOUNT_HANDLER_HPP
