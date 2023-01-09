#include "bw_message_forwarder.hpp"

#include "cstdmf/config.hpp"
#if defined( MF_SERVER ) && (ENABLE_WATCHERS != 0)


#include "network/event_dispatcher.hpp"
#include "network/network_interface.hpp"
#include "network/udp_channel.hpp"

#include "server/bwconfig.hpp"

BW_BEGIN_NAMESPACE

BWMessageForwarder::BWMessageForwarder(
		const char * componentName, const char * configPath,
		bool isForwarding,
		Mercury::EventDispatcher & dispatcher,
		Mercury::NetworkInterface & networkInterface ) :
	watcherNub_(),
	pForwarder_( NULL )
{
	BW::string path( configPath );

	BW::string monitoringInterfaceName =
		BWConfig::get( (path + "/monitoringInterface").c_str(),
				BWConfig::get( "monitoringInterface", "" ) );

	networkInterface.setLossRatio(
				BWConfig::get( (path + "/internalLossRatio").c_str(),
				BWConfig::get( "internalLossRatio", 0.f ) ) );
	networkInterface.setLatency(
			BWConfig::get( (path + "/internalLatencyMin").c_str(),
				BWConfig::get( "internalLatencyMin", 0.f ) ),
			BWConfig::get( (path + "/internalLatencyMax").c_str(),
				BWConfig::get( "internalLatencyMax", 0.f ) ) );

	networkInterface.setIrregularChannelsResendPeriod(
			BWConfig::get( (path + "/irregularResendPeriod").c_str(),
				BWConfig::get( "irregularResendPeriod",
					1.5f / BWConfig::get( "gameUpdateHertz", 10.f ) ) ) );

	networkInterface.shouldUseChecksums(
		BWConfig::get( (path + "/shouldUseChecksums").c_str(),
			BWConfig::get( "shouldUseChecksums", true ) ) );

	float maxSocketProcessingTime =
		BWConfig::get< float >( (path + "/maxInternalSocketProcessingTime").c_str(),
			BWConfig::get< float >( "maxInternalSocketProcessingTime", true ) );
	int gameUpdateHertz = BWConfig::get< int >( "gameUpdateHertz", true );

	if (maxSocketProcessingTime < 0.f)
	{
		maxSocketProcessingTime = 1.f/gameUpdateHertz;
	}

	networkInterface.maxSocketProcessingTime( maxSocketProcessingTime );

	Mercury::UDPChannel::setInternalMaxOverflowPackets(
		BWConfig::get( "maxChannelOverflow/internal",
		Mercury::UDPChannel::getInternalMaxOverflowPackets() ));

	Mercury::UDPChannel::setIndexedMaxOverflowPackets(
		BWConfig::get( "maxChannelOverflow/indexed",
		Mercury::UDPChannel::getIndexedMaxOverflowPackets() ));

	Mercury::UDPChannel::setExternalMaxOverflowPackets(
		BWConfig::get( "maxChannelOverflow/external",
		Mercury::UDPChannel::getExternalMaxOverflowPackets() ));

	Mercury::UDPChannel::assertOnMaxOverflowPackets(
		BWConfig::get( "maxChannelOverflow/isAssert",
		Mercury::UDPChannel::assertOnMaxOverflowPackets() ));

	if (monitoringInterfaceName == "")
	{
		monitoringInterfaceName =
				inet_ntoa( (struct in_addr &)networkInterface.address().ip );
	}

	this->watcherNub().init( monitoringInterfaceName.c_str(), 0 );

	unsigned spamFilterThreshold =
		BWConfig::get( (path + "/logSpamThreshold").c_str(),
			BWConfig::get( "logSpamThreshold", 20 ) );

	bool spamFilterEnabled = 
		BWConfig::get( (path + "/shouldFilterLogSpam").c_str(),
			BWConfig::get( "shouldFilterLogSpam", true ) );

	bool spamFilterSummariesEnabled = 
		BWConfig::get( (path + "/logSpamFilterSummaries").c_str(),
			BWConfig::get( "logSpamFilterSummaries", true ) );

	pForwarder_.reset(
		new LoggerMessageForwarder( componentName,
			this->watcherNub().udpSocket(), dispatcher,
			BWConfig::get( "loggerID", "" ),
			isForwarding, spamFilterEnabled, 
			spamFilterSummariesEnabled, spamFilterThreshold ) );

	DataSectionPtr pSuppressionPatterns =
		BWConfig::getSection( (path + "/logSpamPatterns").c_str(),
			BWConfig::getSection( "logSpamPatterns" ) );

	if (pSuppressionPatterns)
	{
		for (DataSectionIterator iter = pSuppressionPatterns->begin();
			 iter != pSuppressionPatterns->end(); ++iter)
		{
			pForwarder_->addSuppressionPattern( (*iter)->asString() );
		}
	}
}

BW_END_NAMESPACE

#endif // MF_SERVER && ENABLE_WATCHERS

// bw_message_forwarder.cpp
