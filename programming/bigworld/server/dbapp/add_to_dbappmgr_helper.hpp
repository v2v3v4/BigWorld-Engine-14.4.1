#include "cstdmf/stdmf.hpp"

#include "server/add_to_manager_helper.hpp"


BW_BEGIN_NAMESPACE

/**
 *  This class helps to add this app to the DBAppMgr.
 */
class AddToDBAppMgrHelper : public AddToManagerHelper
{
public:
	AddToDBAppMgrHelper( DBApp & dbApp ) :
		AddToManagerHelper( dbApp.mainDispatcher() ),
		dbApp_( dbApp )
	{
		// Auto-send on construction.
		this->send();
	}


	/*
	 *	Override from AddToManagerHelper. 	
	 **/
	void handleFatalTimeout() /* override */
	{
		ERROR_MSG( "AddToDBAppMgrHelper::handleFatalTimeout: "
				"Unable to add DBApp to DBAppMgr. Terminating.\n" );

		dbApp_.mainDispatcher().breakProcessing();
	}


	/*
	 *	Override from AddToManagerHelper. 	
	 **/
	void doSend() /* override */
	{
		dbApp_.dbAppMgr().addDBApp( this );
	}


	/*
	 *	Override from AddToManagerHelper. 	
	 **/
	bool finishInit( BinaryIStream & data ) /* override */
	{
		return dbApp_.onDBAppMgrRegistrationCompleted( data );
	}

private:
	DBApp & dbApp_;
};

BW_END_NAMESPACE

// add_to_dbappmgr_helper.cpp
