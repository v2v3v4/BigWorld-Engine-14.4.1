#include "shutdown_handler.hpp"

#include "baseappmgr/baseappmgr_interface.hpp"

#include "cellappmgr.hpp"
#include "cellappmgr_config.hpp"
#include "cellapp.hpp"

#include "db/dbappmgr_interface.hpp"

#include "network/bundle.hpp"

#include "server/bwconfig.hpp"


DECLARE_DEBUG_COMPONENT( 0 )


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: PerformBaseAppsHandler
// -----------------------------------------------------------------------------

/**
 *	This object handles the stage once we have decided to shut down.
 */
class PerformBaseAppsHandler : public ShutDownHandler
{
public:
	PerformBaseAppsHandler( CellAppMgr & mgr );

	void ackBaseApps( ShutDownStage stage );
	void ackCellApp( ShutDownStage stage, CellApp & app );

private:
	CellAppMgr & 	mgr_;
};


/**
 *	Constructor.
 */
PerformBaseAppsHandler::PerformBaseAppsHandler( CellAppMgr & mgr ) : mgr_( mgr )
{
	CellAppMgr & app = CellAppMgr::instance();

	app.writeGameTimeToDB();
	app.writeSpacesToDB();

	// Tell the BaseAppMgr to perform.
	{
		BaseAppMgrInterface::controlledShutDownArgs args;
		args.stage = SHUTDOWN_PERFORM;
		args.shutDownTime = 0;


		Mercury::Bundle & bundle = app.baseAppMgr().bundle();
		bundle << args;

		app.baseAppMgr().send();
	}
}


/**
 *	This method is called when the BaseApps have all completed the perform
 *	stage.
 */
void PerformBaseAppsHandler::ackBaseApps( ShutDownStage stage )
{
	DBAppMgrInterface::controlledShutDownArgs args;
	args.stage = SHUTDOWN_PERFORM;

	mgr_.dbAppMgr().bundle() << args;

	mgr_.dbAppMgr().send();
	mgr_.setShutDownHandler( NULL );
	mgr_.shutDown( /* shutDownOthers */ true );
	delete this;
}


/**
 *	This method handles an ack from a CellApp. This should not be called because
 *	the CellApps are not involved in this stage.
 */
void PerformBaseAppsHandler::ackCellApp( ShutDownStage stage, CellApp & app )
{
	ERROR_MSG( "PerformBaseAppsHandler::ackCellApp: Got stage %s from %s\n",
		ServerApp::shutDownStageToString( stage ), app.addr().c_str() );
}


// -----------------------------------------------------------------------------
// Section: InformHandler
// -----------------------------------------------------------------------------

/**
 *	This class handles the stage of shutting down where all components are told
 *	when to shut down.
 */
class InformHandler : public ShutDownHandler
{
public:
	InformHandler( CellAppMgr & mgr, GameTime shutDownTime );
	virtual ~InformHandler() {}

	bool isPaused() const;
	void checkStatus();

	void ackBaseApps( ShutDownStage stage );
	void ackCellApp( ShutDownStage stage, CellApp & app );

private:
	CellAppMgr & mgr_;
	GameTime shutDownTime_;
	bool baseAppsAcked_;
	BW::set< CellApp * > ackedCellApps_;
};

InformHandler::InformHandler( CellAppMgr & mgr, GameTime shutDownTime ) :
	mgr_( mgr ),
	shutDownTime_( shutDownTime ),
	baseAppsAcked_( false ),
	ackedCellApps_()
{
	// Inform the BaseAppMgr.
	{
		Mercury::Bundle & bundle = mgr_.baseAppMgr().bundle();
		BaseAppMgrInterface::controlledShutDownArgs args;
		args.stage = SHUTDOWN_INFORM;
		args.shutDownTime = shutDownTime_;
		bundle << args;

		mgr_.baseAppMgr().send();
	}

	// Inform all of the CellApps.
	mgr_.cellApps().controlledShutDown( SHUTDOWN_INFORM, shutDownTime_ );
}

bool InformHandler::isPaused() const
{
   	return mgr_.time() == shutDownTime_;
}

void InformHandler::checkStatus()
{
	INFO_MSG( "InformHandler::checkStatus: "
			"baseAppsAcked_ = %d. cells %" PRIzu "/%d shutDownTime %u time %u\n",
			baseAppsAcked_,
			ackedCellApps_.size(), mgr_.numCellApps(),
			shutDownTime_, mgr_.time() );

	if (this->isPaused() &&
		baseAppsAcked_ &&
		ackedCellApps_.size() >= size_t(mgr_.numCellApps()))
	{
		// TODO: Could check that the correct ones are ack'ed.
		// mgr_.setShutDownHandler( new PerformCellAppHandler( mgr_ ) );
		mgr_.setShutDownHandler( new PerformBaseAppsHandler( mgr_ ) );
		delete this;
	}
}

void InformHandler::ackBaseApps( ShutDownStage stage )
{
	if (stage != SHUTDOWN_INFORM)
	{
		ERROR_MSG( "InformHandler::ackBaseApps: Incorrect stage %s\n", 
			ServerApp::shutDownStageToString( stage ) );
	}

	baseAppsAcked_ = true;
	this->checkStatus();
}

void InformHandler::ackCellApp( ShutDownStage stage, CellApp & app )
{
	if (stage != SHUTDOWN_INFORM)
	{
		ERROR_MSG( "InformHandler::ackCellApp: Incorrect stage %s\n",
				ServerApp::shutDownStageToString( stage ) );
	}

	ackedCellApps_.insert( &app );

	this->checkStatus();
}


// -----------------------------------------------------------------------------
// Section: static methods
// -----------------------------------------------------------------------------

/**
 *	This method starts the process of shutting down the server.
 */
void ShutDownHandler::start( CellAppMgr & mgr )
{
	// No delay if we haven't started yet since game time won't go forward.
	int shuttingDownDelay = 0;

	if (mgr.hasStarted())
	{
		shuttingDownDelay = CellAppMgrConfig::shuttingDownDelayInTicks();
	}

	mgr.setShutDownHandler(
		new InformHandler( mgr, mgr.time() + shuttingDownDelay ) );
}

BW_END_NAMESPACE

// shutdown_handler.cpp
