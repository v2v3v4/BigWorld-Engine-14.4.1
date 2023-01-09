#ifndef PACKET_LOSS_PARAMETERS_HPP
#define PACKET_LOSS_PARAMETERS_HPP

#include "misc.hpp"

#if ENABLE_WATCHERS
#include "cstdmf/watcher.hpp"
#endif

BW_BEGIN_NAMESPACE


namespace Mercury
{

class NetworkInterface;

/**
 *	This class is used to store the artificial loss parameters in a value type.
 */
class PacketLossParameters
{
public:
	/**
	*	Constructor.
	*
	*	@param lossRatioValue	The ratio of packets to drop.
	*	@param minLatencyValue	The minimum latency in seconds.
	*	@param maxLatencyValue	The maximum latency in seconds.
	*
	*/
	PacketLossParameters(
			float lossRatioValue = 0.f,
			float minLatencyValue = 0.f,
			float maxLatencyValue = 0.f )
	{
		lossRatio( lossRatioValue );
		minLatency( minLatencyValue );
		maxLatency( maxLatencyValue );
	}

	
	void set( float lossRatio, float minLatency, float maxLatency );
	void apply( NetworkInterface & networkInterface ) const;

	/**
	 *	This method returns whether artificial loss or latency is enabled.
	 */
	bool hasArtificialLossOrLatency() const
	{
		return dropPerMillion_ || minLatencyMillion_ || maxLatencyMillion_;
	}
	

	/** This method returns the ratio of packets to drop. */
	float lossRatio() const 		{ return lossRatio_; }
	
	void lossRatio( float value );
	
	/** This method returns the packets to drop per million. */
	int dropPerMillion() const		{ return dropPerMillion_; }
	
	void dropPerMillion( int value);

	/** This method returns the minimum latency in seconds. */
	float minLatency() const 		{ return minLatency_; }
	
	void minLatency( float value );
	
	/** This method returns the minimum latency in milliseconds. */
	int minLatencyMillion() const	{ return minLatencyMillion_; }	
	
	void minLatencyMillion( int value );

	/** This method returns the maximum latency in seconds. */
	float maxLatency() const		{ return maxLatency_; }	
	
	void maxLatency( float value );
	
	/** This method returns the maximum latency in milliseconds. */
	int maxLatencyMillion() const 	{ return maxLatencyMillion_; }
	
	void maxLatencyMillion( int value );

#if ENABLE_WATCHERS
	static WatcherPtr pWatcher();
#endif  //ENABLE_WATCHERS

private:
	float lossRatio_;
	float minLatency_;
	float maxLatency_;

	/**
	 * Adding the following three integers is an optimisation so that a floating
	 * point multiplication wouldn't have to be done each time a packet is sent.
	 */

	// Of every million packets sent, this many packets will be dropped for
	// debugging.
	int dropPerMillion_;

	// In milliseconds
	int minLatencyMillion_;
	int maxLatencyMillion_;

};

} // end namespace Mercury

BW_END_NAMESPACE

#endif // PACKET_LOSS_PARAMETERS_HPP
