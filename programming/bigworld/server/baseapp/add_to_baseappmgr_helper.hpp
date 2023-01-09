#ifndef ADD_TO_BASEAPPMGR_HELPER
#define ADD_TO_BASEAPPMGR_HELPER

#include "baseapp.hpp"
#include "baseappmgr_gateway.hpp"
#include "baseappmgr/baseappmgr_interface.hpp"

#include "server/add_to_manager_helper.hpp"


BW_BEGIN_NAMESPACE

/**
 *  This class helps to add this app to the BaseAppMgr.
 */
class AddToBaseAppMgrHelper : public AddToManagerHelper
{
public:
	AddToBaseAppMgrHelper( BaseApp & baseApp ) :
		AddToManagerHelper( baseApp.mainDispatcher() ),
		app_( baseApp )
	{
		// Auto-send on construction.
 		this->send();
	}


	void handleFatalTimeout()
	{
		ERROR_MSG( "AddToBaseAppMgrHelper::handleFatalTimeout: Unable to add "
				"%s to BaseAppMgr. Terminating.\n", app_.getAppName());
		app_.shutDown();
	}


	void doSend()
	{
		app_.baseAppMgr().add( app_.intInterface().address(), 
			app_.extInterface().address(), app_.isServiceApp(), this );
	}


	bool finishInit( BinaryIStream & data )
	{
		// Call send now so that any pending ACKs are sent now. This gives
		// finishInit a little longer before resends occur. Should not rely
		// on this as BaseAppMgr already thinks this BaseApp is running
		// normally.
		// TODO:BAR
		app_.baseAppMgr().send();

		BaseAppInitData initData;
		data >> initData;

		return app_.finishInit( initData, data );
	}

private:
	BaseApp & app_;
};

BW_END_NAMESPACE

#endif // ADD_TO_BASEAPPMGR_HELPER
