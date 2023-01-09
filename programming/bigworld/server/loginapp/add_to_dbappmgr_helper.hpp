#ifndef ADD_TO_DBAPPMGR_HELPER_HPP
#define ADD_TO_DBAPPMGR_HELPER_HPP

#include "cstdmf/stdmf.hpp"

#include "db/dbappmgr_interface.hpp"

#include "server/add_to_manager_helper.hpp"

#include "network/basictypes.hpp"


BW_BEGIN_NAMESPACE

/**
 *  This class helps to add this app to the DBAppMgr.
 */
class AddToDBAppMgrHelper : public AddToManagerHelper
{
public:

	/**
	 *	Constructor.
	 *
	 *	@param loginApp 	The LoginApp instance.
	 */
	AddToDBAppMgrHelper( LoginApp & loginApp ) :
		AddToManagerHelper( loginApp.mainDispatcher() ),
		app_( loginApp )
	{
		// Auto-send on construction.
		this->send();
	}


	/* Override from AddToManagerHelper. */
	void handleFatalTimeout() /* override */
	{
		ERROR_MSG( "AddToDBAppMgrHelper::handleFatalTimeout: Unable to add "
				"LoginApp to DBAppMgr (%s). Terminating.\n",
			app_.dbAppMgr().addr().c_str() );
		app_.mainDispatcher().breakProcessing();
	}


	/* Override from AddToDBAppHelper. */
	void doSend() /* override */
	{
		Mercury::Bundle	& bundle = app_.dbAppMgr().bundle();
		bundle.startRequest( DBAppMgrInterface::addLoginApp, this );
		app_.dbAppMgr().send();
	}


	/* Override from AddToDBAppHelper. */
	bool finishInit( BinaryIStream & data ) /* override */
	{
		LoginAppID appID;
		Mercury::Address dbAppAlphaAddr;
		data >> appID >> dbAppAlphaAddr;
		return app_.finishInit( appID, dbAppAlphaAddr );
	}

private:
	LoginApp & app_;
};

BW_END_NAMESPACE


#endif // ADD_TO_DBAPPMGR_HELPER_HPP
