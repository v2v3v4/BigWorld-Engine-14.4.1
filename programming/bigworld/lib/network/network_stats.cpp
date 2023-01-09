#include "pch.hpp"

#include "cstdmf/bw_unordered_map.hpp"
#include "cstdmf/timestamp.hpp"

#include "network_stats.hpp"
#include "endpoint.hpp"

BW_BEGIN_NAMESPACE

namespace
{

class Updater
{
public:
	Updater();
	virtual ~Updater();

	bool getQueueSizes( const Mercury::Address & addr, int & tx, int & rx );

private: 
	bool update();

	class StatsBlock
	{
	public:
		StatsBlock();

		int txQueueSize_;
		int rxQueueSize_;
	};

	typedef BW::unordered_map< Mercury::Address, StatsBlock > StatsMap;
	size_t lastActiveRecordCount_;
	StatsMap statsMap_;

	uint64 lastUpdateTime_;
	// The default frequency of stats updating: 400ms
	static const double DEFAULT_UPDATE_INTERVAL;
};

const double Updater::DEFAULT_UPDATE_INTERVAL = 0.4;

/**
 *	Constructor.
 */
Updater::Updater():
	lastActiveRecordCount_( 0 ),
	statsMap_(),
	lastUpdateTime_( 0 )
{
}

/**
 *	Destructor.
 */
Updater::~Updater()
{
}

/**
 *	This method returns the current size of the transmit and receive queues
 *	for this specified address. This method is only implemented on Unix.
 *
 *	@param addr	The specified address, including ip and port.
 *	@param tx	The current size of the transmit queue is returned here.
 *	@param rx 	The current size of the receive queue is returned here.
 *
 *	@return true if successful, false otherwise.
 */
bool Updater::getQueueSizes( const Mercury::Address & addr, int & tx, int & rx )
{
	if (stampsToSeconds( timestamp() - lastUpdateTime_ ) >=
			DEFAULT_UPDATE_INTERVAL)
	{
		this->update();

		lastUpdateTime_ = timestamp();
	}

	StatsMap::iterator it = statsMap_.find( addr );

	if (it == statsMap_.end())
	{
		return false;
	}

	tx = it->second.txQueueSize_;
	rx = it->second.rxQueueSize_;

	return true;
}

/**
 *	This method update the current size of udp transmit and receive queues
 *	for this process. This method is only implemented on Unix.
 *
 *	@return true if successful, false otherwise.
 */
bool Updater::update()
{
	PROFILER_SCOPED( NetworkStats_update );

#ifdef __unix__
	FILE * f = fopen( "/proc/net/udp", "r" );

	if (!f)
	{
		ERROR_MSG( "Updater::update: "
				"could not open /proc/net/udp: %s\n",
			strerror( errno ) );
		return false;
	}

	// Here statsMap_ is a buffer for saving the tx/rx of sockets for
	// later usage to prevent frequent I/O. As the address will not be
	// changed frequently, the entries of this map could be reused at
	// most of the time to prevent frequent allocation/deallocation.
	// And the addresses will also be gradually gone which leads to 
	// statsMap_ accumulating continuously. So in order to stop this
	// sort of accumulation at some point, when the size of statsMap_
	// is greater than the double of number of connections list in
	// /proc/net/udp, clear it as a forced garbage collection.
	if (statsMap_.size() > 2 * lastActiveRecordCount_)
	{
		statsMap_.clear();
	}

	lastActiveRecordCount_ = 0;

	char aline[ 256 ];
	// Skip the header
	fgets( aline, sizeof( aline ), f );

	// Use the first line to get the offsets
	if (fgets( aline, sizeof( aline ), f ) == NULL)
	{
		fclose(f);
		return false;
	}

	char * start = strstr( aline, ": " ) + 2;
	int addrOffset = start - aline;
	start = strstr( start, " " ) + 1;
	start = strstr( start, " " ) + 1;
	start = strstr( start, " " ) + 1;
	int txrxOffset = start - aline;

	if ((addrOffset < 0) || (txrxOffset < 0))
	{
		ERROR_MSG( "Updater::update: /proc/net/udp invalid format: %s\n",
			aline );
		fclose( f );
		return false;
	}

	Mercury::Address addr;
	StatsBlock statsBlock;
	do
	{
		// Get local_address
		char * endptr = NULL;
		addr.ip = strtoul( &aline[ addrOffset ], &endptr, 16 );
		endptr++;
		addr.port = strtoul( endptr, NULL, 16 );
		addr.port = htons( addr.port );

		// Get tx/rx
		endptr = NULL;
		statsBlock.txQueueSize_ = strtol( &aline[ txrxOffset ], &endptr, 16 );
		endptr++;
		statsBlock.rxQueueSize_ = strtol( endptr, NULL, 16 );

		// Save to the map for later usage.
		statsMap_[ addr ] = statsBlock;

		lastActiveRecordCount_++;
	} while (fgets( aline, sizeof( aline ), f ) != NULL);

	fclose(f);
#endif // __unix__

	return true;
}

static Updater s_updater;

/**
 *	Constructor for StatsBlock.
 */
Updater::StatsBlock::StatsBlock():
	txQueueSize_( 0 ),
	rxQueueSize_( 0 )
{
}

} // anonymous namespace


namespace NetworkStats
{
/**
 *	This method returns the current size of the transmit and receive queues
 *	for this endpoint.
 *
 *	@param ep	The Endpoint.
 *	@param tx	The current size of the transmit queue is returned here.
 *	@param rx 	The current size of the receive queue is returned here.
 *
 *	@return true if successful, false otherwise.
 */
bool getQueueSizes( const Endpoint & ep, int & tx, int & rx )
{
	u_int16_t	port = 0;
	u_int32_t	ip = 0;

	ep.getlocaladdress( &port, &ip );
	Mercury::Address addr( ip, port );

	return s_updater.getQueueSizes( addr, tx, rx );
}

} // namespace NetworkStats

BW_END_NAMESPACE

