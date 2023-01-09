#ifndef SERVER_INFO_HPP
#define SERVER_INFO_HPP

#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_vector.hpp"
#include "cstdmf/init_singleton.hpp"
#include "cstdmf/stdmf.hpp"


BW_BEGIN_NAMESPACE

/**
 * 	This class provides details about the server
 * 	hardware being run on
 */
class ServerInfo
{
public:
	/// Constructor
	ServerInfo();

	/// Returns the hostname of this machine
	const BW::string & serverName() const { return serverName_; }


	/// Returns a textual description of this machine's CPU
	const BW::string & cpuInfo() const { return cpuInfo_; }

	typedef BW::vector< float > CpuSpeeds;
	/// Returns a vector of CPU speeds for the CPU cores in this machine
	const CpuSpeeds & cpuSpeeds() const { return cpuSpeeds_; }


	/// Returns a textual description of this machine's RAM
	const BW::string & memInfo() const { return memInfo_; }
	/// Returns the number of bytes of total RAM
	uint64 memTotal() const { return memTotal_; }
	/// Returns the number of bytes of process-used RAM
	uint64 memUsed() const { return memUsed_; }
	/// Request an update of the RAM statistics
	void updateMem();

private:

#ifndef _WIN32 // WIN32PORT
//#else // _WIN32
	void fetchLinuxCpuInfo();
	void fetchLinuxMemInfo();
#endif // _WIN32

	BW::string serverName_;

	BW::string cpuInfo_;
	BW::vector<float> cpuSpeeds_;

	BW::string memInfo_;
	uint64 memTotal_;
	uint64 memUsed_;
};

BW_END_NAMESPACE

#endif // SERVER_INFO_HPP

