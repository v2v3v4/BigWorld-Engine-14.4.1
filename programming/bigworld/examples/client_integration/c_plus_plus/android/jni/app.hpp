#ifndef APP_HPP
#define APP_HPP

#include <memory>

#include "game_logic_factory.hpp"
#include "EntityFactory.hpp"
#include "DefsDigest.hpp"

#include "cstdmf/singleton.hpp"

#include "connection/condemned_interfaces.hpp"
#include "connection/entity_def_constants.hpp"
#include "connection/login_challenge_factory.hpp"
#include "connection/server_connection_handler.hpp"
#include "connection_model/bw_connection_helper.hpp"
#include "connection_model/bw_server_connection.hpp"
#include "connection_model/bw_entity_factory.hpp"
#include "connection_model/simple_space_data_storage.hpp"


namespace BW
{

class LoopTask;

} // namespace BW


/**
 *	This class represents the entire application.
 */
class App :
		public BW::Singleton< App >,
		public BW::ServerConnectionHandler
{
public:
	App();
	~App();

	bool init();
	bool update();
	void fini();

//	void addLoopTask( LoopTask * loopTask );
//	void removeLoopTask( LoopTask * loopTask );

	// ServerConnectionHandler overrides
	virtual void onLoggedOn();
	virtual void onLogOnFailure( const BW::LogOnStatus & status,
		const BW::string & error );
	virtual void onLoggedOff();

	bool isOnline() { return pConnection_->isOnline(); }
	bool isLoggingIn() { return pConnection_->isLoggingIn(); }

	float getTime() const;

private:
	// Private member data
	bool running_;
	bool stopRequested_;
	float lastTime_;

	std::auto_ptr< BW::LoopTask > pConnectionTicker_;

	BW::SimpleSpaceDataStorage spaceDataStorage_;
	EntityFactory entityFactory_;
	GameLogicFactory gameLogicFactory_;
	BW::BWConnectionHelper connectionHelper_;
	std::auto_ptr< BW::BWServerConnection > pConnection_;

 	BW::string serverAddressString_;
 	BW::string username_;
 	BW::string password_;
 	BW::string resourceDir_;
};


#endif // APP_HPP

