#include "cstdmf/bw_map.hpp"
#include "network/netmask.hpp"
#include "server/server_app_option.hpp"


BW_BEGIN_NAMESPACE

class NATConfig
{
public:
	static ServerAppOption< BW::string > localNetMask;
	static ServerAppOption< BW::string > externalAddress;

	static bool isExternalIP( uint32 ip );
	static bool isInternalIP( uint32 ip );
	static uint32 externalIPFor( uint32 ip );

	static bool postInit();

private:
	// Never made directly
	NATConfig();

	static NetMask s_netMask_;
	static uint32 s_externalIP_;

	typedef BW::map< uint32, uint32 > ExternalIPs;
	static ExternalIPs s_externalIPs_;
};

BW_END_NAMESPACE

// nat_config.hpp
