#include "pch.hpp"

#include "network/packet_loss_parameters.hpp"
#include "network/network_interface.hpp"

BW_BEGIN_NAMESPACE


namespace Mercury
{


/**
 *	This method sets the artificial loss parameters.
 *
 *	@param lossRatioValue	The loss ratio to set.
 *	@param minLatencyValue	The minimum latency.
 *	@param maxLatencyValue	The maximum latency.
 *
 */
void PacketLossParameters::set(
		float lossRatioValue, float minLatencyValue, float maxLatencyValue )
{
	lossRatio( lossRatioValue );
	minLatency( minLatencyValue );
	maxLatency( maxLatencyValue );
}


/**
 * 	This method apply the current settings to the given network interface
 *
 * 	@param networkInterface		The network interface
 */
void PacketLossParameters::apply( 
	Mercury::NetworkInterface & networkInterface ) const
{
	networkInterface.setLossRatio( lossRatio_ );
	networkInterface.setLatency( minLatency_, maxLatency_ );
}


/**
 * 	This method sets the ratio of packets to drop. If non-zero, packets will be 
 * 	randomly dropped, and not sent.
 *
 * 	@param lossRatio	The ratio of packets to drop. Setting to 0.0 disables
 *						artificial packet dropping. 1.0 means all packets are
 *						dropped.
 */
void PacketLossParameters::lossRatio( float value )
{
	lossRatio_ = value;
	dropPerMillion_ = int( value * 1000000 );

	if ( value != 0.f )
	{
		WARNING_MSG( "PacketLossParameters::lossRatio:" 
				"Loss ratio set to %.2f\n",
				lossRatio_);
	}
}


/**
 * 	This method sets the number of packets to drop per million. If non-zero, 
 *	packets will be randomly dropped, and not sent.
 *
 * 	@param dropPerMillion	The number of packets to drop per million. Setting 
 *							to 0 disables artificial packet dropping. 1000000 
 *							means all packets are dropped.
 */
void PacketLossParameters::dropPerMillion( int value )
{
	lossRatio( float( value ) / 1000000.f );
}


/**
 *	This method sets the minimum latency associated. If non-zero, packets will 
 *	be randomly delayed before they are sent.
 *
 *	@param latencyMin	The minimum latency in seconds
 */
void PacketLossParameters::minLatency( float value )
{
	minLatency_ = value;
	minLatencyMillion_ = int( value * 1000 );

	if ( value != 0.f )
	{
		WARNING_MSG( "PacketLossParameters::latencyMin: "
				"Latency enabled. In range from %.2f to %.2f milliseconds.\n",
				minLatency_, maxLatency_ );
	}
}


/**
 *	This method sets the minimum latency associated. If non-zero, packets will 
 *	be randomly delayed before they are sent.
 *
 *	@param latencyMin	The minimum latency in milliseconds
 */
void PacketLossParameters::minLatencyMillion( int value )
{
	minLatency( float( value ) / 1000.f );
}


/**
 *	This method sets the maximum latency associated. If non-zero, packets will 
 *	be randomly delayed before they are sent.
 *
 *	@param latencyMax	The maximum latency in seconds
 */
void PacketLossParameters::maxLatency( float value )
{
	maxLatency_ = value;
	maxLatencyMillion_ = int( value * 1000 );

	if ( value != 0.f )
	{
		WARNING_MSG( "PacketLossParameters::maxLatency: "
				"Latency enabled. In range from %.2f to %.2f milliseconds.\n",
				minLatency_, maxLatency_ );
	}
}


/**
 *	This method sets the maximum latency associated with this
 *	interface. If non-zero, packets will be randomly delayed before they are
 *	sent.
 *
 *	@param latencyMax	The maximum latency in milliseconds
 */
void PacketLossParameters::maxLatencyMillion( int value )
{
	maxLatency( float( value ) / 1000.f );
}


#if ENABLE_WATCHERS

/**
 *	This method returns a watcher for the artificial loss settings.
 */
WatcherPtr PacketLossParameters::pWatcher()
{
	WatcherPtr pWatcher = new DirectoryWatcher();
	PacketLossParameters * pNull = NULL;

	pWatcher->addChild( "dropPerMillion",
		makeNonRefWatcher( *pNull, 
			&PacketLossParameters::dropPerMillion,
			&PacketLossParameters::dropPerMillion ) );

	pWatcher->addChild( "minLatency",
		makeNonRefWatcher( *pNull, 
			&PacketLossParameters::minLatencyMillion,
			&PacketLossParameters::minLatencyMillion ) );

	pWatcher->addChild( "maxLatency",
		makeNonRefWatcher( *pNull, 
			&PacketLossParameters::maxLatencyMillion,
			&PacketLossParameters::maxLatencyMillion ) );

	return pWatcher;
}

#endif  //ENABLE_WATCHERS

} // end namespace Mercury


BW_END_NAMESPACE


// packet_sender.cpp
