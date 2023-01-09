#ifndef SMART_SERVER_CONNECTION_HPP
#define SMART_SERVER_CONNECTION_HPP

#include "rsa_stream_encoder.hpp"

#include "server_connection.hpp"
#include "server_connection_handler.hpp"

BW_BEGIN_NAMESPACE

namespace Mercury
{
	class PacketLossParameters;
}

class ServerMessageHandler;


/**
 *	This class extends ServerConnection with extra functionality and
 *	simplifying the API.
 *
 *	ServerConnection has been left generally unchanged for backwards
 *	compatibility.
 */
class SmartServerConnection : public ServerConnection
{
public:
	SmartServerConnection( LoginChallengeFactories & challengeFactories,
		CondemnedInterfaces & condemnedInterfaces,
		const EntityDefConstants & entityDefConstants );

	virtual ~SmartServerConnection();

	void setConnectionHandler( ServerConnectionHandler * pHandler );

	bool setLoginAppPublicKeyString( const BW::string & publicKeyString );

	bool setLoginAppPublicKeyPath( const BW::string & publicKeyPath );

	bool logOnTo( const BW::string & serverAddressString,
		const BW::string & username,
		const BW::string & password,
		ConnectionTransport transport = CONNECTION_TRANSPORT_UDP );

	void logOff();

	void update( float dt, ServerMessageHandler * pMessageHandler );
	void updateServer();

	bool isLoggingIn() const;

	/**
	 *	This returns the client-side elapsed time.
	 */
	double clientTime() const
	{ 
		return clientTime_;
	}

	double serverTime() const;

	bool isSwitchingBaseApps() const;

protected:
	virtual void onSwitchBaseApp( LoginHandler & loginHandler );

private:
	SmartServerConnection( const SmartServerConnection & );
	SmartServerConnection & operator=( const SmartServerConnection & );

	void onLoggedOn();
	void onLogOnFailure( const LogOnStatus & status, const BW::string & error );
	void onLoggedOff();

	ServerConnectionHandler * 			pConnectionHandler_;

	LoginHandlerPtr 					pInProgressLogin_;
	bool								loggedOn_;

	double								clientTime_;

	RSAStreamEncoder 					streamEncoder_;
};

BW_END_NAMESPACE

#endif // SMART_SERVER_CONNECTION_HPP

