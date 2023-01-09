#ifndef PROCESSOR_COMMAND_UPGRADE_BSP_HPP
#define PROCESSOR_COMMAND_UPGRADE_BSP_HPP

#include "command.hpp"

BW_BEGIN_NAMESPACE

namespace OfflineProcessor
{

/**
 * BSP upgrade command. Loads and saves bsp data upgrading it to the latest format
 * Supports the following files:
 * *.primitives
 * *.bsp2
 *
 * This command supports batch processing. User can pass a folder name and use -recusive option
 */
class CommandUpgradeBSP : public BatchCommand
{
public:
	CommandUpgradeBSP( const CommandLine& commandLine );
	virtual ~CommandUpgradeBSP();

	virtual const char*		strCommand() const { return "upgrade-bsp"; };

	virtual void	showDetailedHelp() const;
	virtual void	showShortHelp() const;

	virtual FileProcessResult	processFile( const BW::string& fileName, const BW::string& outFileName = "" );

private:
	bool	doFormatUpgradeBSPInPrimitives( const BW::string& primName );
	bool	doFormatUpgradeStandaloneBSPFile( const BW::string& bspName );
};

} // namespace OfflineProcessor

BW_END_NAMESPACE

#endif // PROCESSOR_COMMAND_UPGRADE_BSP_HPP