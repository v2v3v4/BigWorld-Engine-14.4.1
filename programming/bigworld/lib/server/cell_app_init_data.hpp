#ifndef CELL_APP_INIT_DATA_HPP
#define CELL_APP_INIT_DATA_HPP

BW_BEGIN_NAMESPACE

/**
 * Data to use when initialising a CellApp.
 */
struct CellAppInitData
{
	int32 id;				//!< ID of the new CellApp
	GameTime time;			//!< Current game time
	Mercury::Address baseAppAddr;	//!< Address of the BaseApp to talk to
	Mercury::Address dbAppAlphaAddr; //!< Address of the DBApp to talk to
	bool isReady;			//!< Flag indicating whether the server is ready
	float timeoutPeriod;	//!< The timeout period for CellApps

	CellAppInitData() :
		id(),
		time(),
		baseAppAddr(),
		isReady(),
		timeoutPeriod()
	{}
};

BW_END_NAMESPACE

#endif // CELL_APP_INIT_DATA_HPP
