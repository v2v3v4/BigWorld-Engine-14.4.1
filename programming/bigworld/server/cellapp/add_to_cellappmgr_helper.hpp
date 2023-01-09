#include "cstdmf/stdmf.hpp"
#include "server/add_to_manager_helper.hpp"
#include "server/cell_app_init_data.hpp"


BW_BEGIN_NAMESPACE

/**
 *  This class helps to add this app to the CellAppMgr.
 */
class AddToCellAppMgrHelper : public AddToManagerHelper
{
public:
	AddToCellAppMgrHelper( CellApp & cellApp, uint16 viewerPort ) :
		AddToManagerHelper( cellApp.mainDispatcher() ),
		app_( cellApp ),
		viewerPort_( viewerPort )
	{
		// Auto-send on construction.
		this->send();
	}


	void handleFatalTimeout()
	{
		ERROR_MSG( "AddToCellAppMgrHelper::handleFatalTimeout: Unable to add "
			"CellApp to CellAppMgr. Terminating.\n" );
		app_.mainDispatcher().breakProcessing();
	}


	void doSend()
	{
		app_.cellAppMgr().add( app_.interface().address(), viewerPort_, this );
	}


	bool finishInit( BinaryIStream & data )
	{
		CellAppInitData initData;
		data >> initData;
		return app_.finishInit( initData );
	}

private:
	CellApp & app_;
	uint16 viewerPort_;
};

BW_END_NAMESPACE

// add_to_cellappmgr_helper.cpp
