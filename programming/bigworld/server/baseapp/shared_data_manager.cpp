#include "script/first_include.hpp"

#include "shared_data_manager.hpp"

#include "baseapp.hpp"
#include "baseappmgr_gateway.hpp"

#include "baseappmgr/baseappmgr_interface.hpp"
#include "cellappmgr/cellappmgr_interface.hpp"

#include "network/bundle.cpp"

#include "server/shared_data.hpp"
#include "server/shared_data_type.hpp"

#include "pyscript/script.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: Helper functions
// -----------------------------------------------------------------------------

namespace Util
{

/**
 *	This function sets a shared data value.
 */
void setSharedData( const BW::string & key, const BW::string & value,
		SharedDataType dataType )
{
	// TODO:BAR
	BaseAppMgrGateway & baseAppMgr = BaseApp::instance().baseAppMgr();
	Mercury::Bundle & bundle = baseAppMgr.bundle();

	bundle.startMessage( BaseAppMgrInterface::setSharedData );
	bundle << dataType << key << value;
	baseAppMgr.send();
}


/**
 *	This method is called when a shared data value is set.
 */
void onSetSharedData( PyObject * pKey, PyObject * pValue,
		SharedDataType dataType )
{
	const char * methodName = (dataType == SHARED_DATA_TYPE_BASE_APP) ?
							"onBaseAppData" : "onGlobalData";
	BaseApp::instance().scriptEvents().triggerEvent( methodName,
			PyTuple_Pack( 2, pKey, pValue ) );
}


/**
 *	This function deletes a shared data value.
 */
void delSharedData( const BW::string & key, SharedDataType dataType )
{
	// TODO:BAR
	BaseAppMgrGateway & baseAppMgr = BaseApp::instance().baseAppMgr();
	Mercury::Bundle & bundle = baseAppMgr.bundle();
	bundle.startMessage( BaseAppMgrInterface::delSharedData );
	bundle << dataType << key;
	baseAppMgr.send();
}


/**
 *	This method is called when a shared data value is set.
 */
void onDelSharedData( PyObject * pKey, SharedDataType dataType )
{
	const char * methodName = (dataType == SHARED_DATA_TYPE_BASE_APP) ?
							"onDelBaseAppData" : "onDelGlobalData";
	BaseApp::instance().scriptEvents().triggerEvent( methodName,
		PyTuple_Pack( 1, pKey ) );
}

} // namespace Util


// -----------------------------------------------------------------------------
// Section: Python methods
// -----------------------------------------------------------------------------

namespace
{

/*~ function BigWorld.setCellAppData
 *	@components{ base }
 *	@param key The key of the value to be set.
 *	@param value The new value.
 *
 *	This function sets a data value that is accessible from all CellApps. This
 *	can be accessed on CellApps via BigWorld.cellAppData.
 *
 *	The CellAppMgr is the authoritative copy of this data. If two or more
 *	components try to set the same value, the last one to reach the CellAppMgr
 *	will be the one that is kept.
 *
 *	Note: Care should be taken using this functionality. It does not scale well
 *	and should be used sparingly.
 */
PyObject * py_setCellAppData( PyObject * args )
{
	PyObject * pKey = NULL;
	PyObject * pValue = NULL;

	if (!PyArg_ParseTuple( args, "OO:setCellAppData", &pKey, &pValue ))
	{
		return NULL;
	}

	BW::string key = BaseApp::instance().pickle( 
		ScriptObject( pKey, ScriptObject::FROM_BORROWED_REFERENCE ) );
	BW::string value = BaseApp::instance().pickle( 
		ScriptObject( pValue, ScriptObject::FROM_BORROWED_REFERENCE ) );

	if (key.empty())
	{
		PyErr_SetString( PyExc_TypeError, "key could not be pickled" ); 
		return NULL;
	}

	if (value.empty())
	{
		PyErr_SetString( PyExc_TypeError, "value could not be pickled" );
		return NULL;
	}

	Util::setSharedData( key, value, SHARED_DATA_TYPE_CELL_APP );

	Py_RETURN_NONE;
}
PY_MODULE_FUNCTION( setCellAppData, BigWorld )


/*~ function BigWorld.delCellAppData
 *	@components{ base }
 *	@param key The key of the value to be deleted.
 *
 *	This function deletes a data value that is accessible from all CellApps.
 *	This can be accessed on CellApps via BigWorld.cellAppData.
 *
 *	The CellAppMgr is the authoritative copy of this data. If two or more
 *	components try to set the same value, the last one to reach the CellAppMgr
 *	will be the one that is kept.
 *
 *	Note: Care should be taken using this functionality. It does not scale well
 *	and should be used sparingly.
 */
PyObject * py_delCellAppData( PyObject * args )
{
	PyObject * pKey = NULL;

	if (!PyArg_ParseTuple( args, "O:delCellAppData", &pKey ))
	{
		return NULL;
	}

	BW::string key = BaseApp::instance().pickle( 
		ScriptObject( pKey, ScriptObject::FROM_BORROWED_REFERENCE ) );

	if (key.empty())
	{
		PyErr_SetString( PyExc_TypeError, "key could not be pickled" );
		return NULL;
	}

	Util::delSharedData( key, SHARED_DATA_TYPE_CELL_APP );

	Py_RETURN_NONE;
}
PY_MODULE_FUNCTION( delCellAppData, BigWorld )

} // anonymous namespace


// -----------------------------------------------------------------------------
// Section: class SharedDataManager
// -----------------------------------------------------------------------------

/**
 *	This static method is used to create SharedDataManager instances.
 */
shared_ptr< SharedDataManager > SharedDataManager::create( Pickler * pPickler )
{
	shared_ptr< SharedDataManager > pMgr( new SharedDataManager() );

	return pMgr->init( pPickler ) ? pMgr : shared_ptr< SharedDataManager >();
}


/**
 *	Constructor.
 */
SharedDataManager::SharedDataManager() :
	pBaseAppData_( NULL ),
	pGlobalData_( NULL )
{
}


/**
 *	Destructor
 */
SharedDataManager::~SharedDataManager()
{
	Py_XDECREF( pBaseAppData_ );
	pBaseAppData_ = NULL;

	Py_XDECREF( pGlobalData_ );
	pGlobalData_ = NULL;
}


bool SharedDataManager::init( Pickler * pPickler )
{
	PyObject * pModule = PyImport_AddModule( "BigWorld" );

	MF_ASSERT( pBaseAppData_ == NULL );
	pBaseAppData_ = new SharedData( SHARED_DATA_TYPE_BASE_APP,
		   Util::setSharedData, Util::delSharedData,
		   Util::onSetSharedData, Util::onDelSharedData,
		   pPickler );

	if (PyObject_SetAttrString( pModule, "baseAppData", pBaseAppData_ ) == -1)
	{
		PyErr_Print();
		return false;
	}

	MF_ASSERT( pGlobalData_ == NULL );
	pGlobalData_ = new SharedData( SHARED_DATA_TYPE_GLOBAL_FROM_BASE_APP,
		   Util::setSharedData, Util::delSharedData,
		   Util::onSetSharedData, Util::onDelSharedData,
		   pPickler );

	if (PyObject_SetAttrString( pModule, "globalData", pGlobalData_ ) == -1)
	{
		PyErr_Print();
		return false;
	}

	return true;
}


/**
 *	This method is called by the BaseAppMgr to inform us that a shared value has
 *	changed. This value may be shared between BaseApps or BaseApps and CellApps.
 */
void SharedDataManager::setSharedData( BinaryIStream & data )
{
	// This code is nearly identical to the code in CellApp::setSharedData.
	// We could look at sharing the code somehow.
	SharedDataType dataType;
	BW::string key;
	BW::string value;

	data >> dataType >> key >> value;

	Mercury::Address originalSrcAddress = Mercury::Address::NONE;

	if (data.remainingLength() == sizeof( Mercury::Address ))
	{
		data >> originalSrcAddress;
	}

	IF_NOT_MF_ASSERT_DEV( (data.remainingLength() == 0) && !data.error() )
	{
		ERROR_MSG( "BaseApp::setSharedData: Invalid data!!\n" );
		return;
	}

	// If the original change was made by this BaseApp, treat this
	// message as an ack.
	bool isAck =
		(originalSrcAddress == BaseApp::instance().interface().address());

	switch (dataType)
	{
	case SHARED_DATA_TYPE_BASE_APP:
		pBaseAppData_->setValue( key, value, isAck );
		break;

	case SHARED_DATA_TYPE_GLOBAL:
		pGlobalData_->setValue( key, value, isAck );
		break;

	default:
		ERROR_MSG( "BaseApp::setValue: Invalid dataType %d\n", dataType );
		break;
	}
}


/**
 *	This method is called by the BaseAppMgr to inform us that a shared value was
 *	deleted. This value may be shared between BaseApps or BaseApps and CellApps.
 */
void SharedDataManager::delSharedData( BinaryIStream & data )
{
	// This code is nearly identical to the code in CellApp::delSharedData.
	// We could look at sharing the code somehow.
	SharedDataType dataType;
	BW::string key;

	data >> dataType >> key;

	Mercury::Address originalSrcAddress = Mercury::Address::NONE;

	if (data.remainingLength() == sizeof( Mercury::Address ))
	{
		data >> originalSrcAddress;
	}

	IF_NOT_MF_ASSERT_DEV( (data.remainingLength() == 0) && !data.error() )
	{
		ERROR_MSG( "BaseApp::delSharedData: Invalid data!!\n" );
		return;
	}

	// If original change was made by this BaseApp, treat this
	// message as an ack
	bool isAck =
		(originalSrcAddress == BaseApp::instance().interface().address());

	switch (dataType)
	{
	case SHARED_DATA_TYPE_BASE_APP:
		pBaseAppData_->delValue( key, isAck );
		break;

	case SHARED_DATA_TYPE_GLOBAL:
		pGlobalData_->delValue( key, isAck );
		break;

	default:
		ERROR_MSG( "BaseApp::delValue: Invalid dataType %d\n", dataType );
		break;
	}
}


void SharedDataManager::addToStream( BinaryOStream & stream )
{
	pBaseAppData_->addToStream( stream );
	pGlobalData_->addToStream( stream );
}

BW_END_NAMESPACE

// -----------------------------------------------------------------------------
// Section: Python documentation
// -----------------------------------------------------------------------------

/*~ attribute BigWorld baseAppData
 *	@components{ base }
 *	This property contains a dictionary-like object that is automatically
 *	duplicated between all BaseApps. Whenever a value in the dictionary is
 *	modified, this change is propagated to all BaseApps. The BaseAppMgr keeps
 *	the authoritative copy of this information and resolves race conditions.
 *
 *	For example:
 *	@{
 *	BigWorld.baseAppData[ "hello" ] = "there"
 *	@}
 *
 *	Another BaseApp can access this as follows:
 *	@{
 *	print BigWorld.baseAppData[ "hello" ]
 *	@}
 *
 *	The key and value can be any type that can be pickled and unpickled on all
 *	destination components.
 *
 *	When a value is changed or deleted, a callback function is called on all
 *	components.
 *	See: BWPersonality.onBaseAppData and BWPersonality.onDelBaseAppData.
 *
 *	NOTE: Only top-level values are propagated, if you have a value that is
 *	mutable (for example, a list) and it changed internally (for example,
 *	changing just one member), this information is NOT propagated.
 *
 *	Do NOT do the following:
 *	@{
 *	BigWorld.baseAppData[ "list" ] = [1, 2, 3]
 *	BigWorld.baseAppData[ "list" ][1] = 7
 *	@}
 *	Locally, this would look like [1, 7, 3] and remotely [1, 2, 3].
 */
/*~ attribute BigWorld globalData
 *	@components{ base }
 *	This property contains a dictionary-like object that is automatically
 *	duplicated between all BaseApps and CellApps. Whenever a value in the
 *	dictionary is modified, this change is propagated to all BaseApps and
 *	CellApps. The CellAppMgr keeps the authoritative copy of this information
 *	and resolves race conditions.
 *
 *	For example:
 *	@{
 *	BigWorld.globalData[ "hello" ] = "there"
 *	@}
 *
 *	Another CellApp or BaseApp can access this as follows:
 *	@{
 *	print BigWorld.globalData[ "hello" ]
 *	@}
 *
 *	The key and value can be any type that can be pickled and unpickled on all
 *	destination components.
 *
 *	When a value is changed or deleted, a callback function is called on all
 *	components.
 *	See: BWPersonality.onGlobalData and BWPersonality.onDelGlobalData.
 *
 *	NOTE: Only top-level values are propagated, if you have a value that is
 *	mutable (for example, a list) and it changed internally (for example,
 *	changing just one member), this information is NOT propagated.
 *
 *	Do NOT do the following:
 *	@{
 *	BigWorld.globalData[ "list" ] = [1, 2, 3]
 *	BigWorld.globalData[ "list" ][1] = 7
 *	@}
 *	Locally, this would look like [1, 7, 3] and remotely [1, 2, 3].
 */

// shared_data_manager.cpp
