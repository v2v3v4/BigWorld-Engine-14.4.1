#include "CppUnitLite2/src/CppUnitLite2.h"

#include "db/dbapps_gateway.hpp"

#include <cmath>

BW_BEGIN_NAMESPACE

namespace // (anonymous)
{
	Mercury::Address addr_1( 0x01010101, 0x100 );
	Mercury::Address addr_3( 0x03030303, 0x300 );
	Mercury::Address addr_4( 0x04040404, 0x400 );
	Mercury::Address addr_7( 0x07070707, 0x700 );
} // end namespace (anonymous)


TEST( DBAppsGateway )
{
	DBAppsGateway gateway;

	gateway.addDBApp( DBAppGateway( 1, addr_1 ) );
	gateway.addDBApp( DBAppGateway( 3, addr_3 ) );
	gateway.addDBApp( DBAppGateway( 4, addr_4 ) );
	gateway.addDBApp( DBAppGateway( 7, addr_7 ) );

	typedef BW::map< DBAppID, uint > Counts;
	Counts counts;

	static const size_t NUM_ITERATIONS = 2000000;
	for (DatabaseID entityID = 1;
			static_cast< size_t >( entityID ) < NUM_ITERATIONS;
			++entityID)
	{
		const DBAppGateway & dbApp = gateway.getDBApp( entityID );
		CHECK( dbApp != DBAppGateway::NONE );
		++counts[ dbApp.id() ];
	}

	const double average = double( NUM_ITERATIONS ) / gateway.size();
	double sumAbsoluteDeviation = 0.0;

	for (Counts::const_iterator iter = counts.begin();
			iter != counts.end();
			++iter)
	{
		HACK_MSG( "%d: %u\n", iter->first, iter->second );
		sumAbsoluteDeviation += std::fabs( average - iter->second );
	}

	const double madRatio = sumAbsoluteDeviation / gateway.size() / average;
	HACK_MSG( "madRatio = %.03lf\n", madRatio );
	CHECK( madRatio < 0.05 );

}


BW_END_NAMESPACE 


// test_dbapps_gateway.cpp
