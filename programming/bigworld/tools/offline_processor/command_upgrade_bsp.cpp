#include "pch.hpp"
#include "command_upgrade_bsp.hpp"
#include "resmgr/bwresource.hpp"
#include "physics2/bsp.hpp"

BW_BEGIN_NAMESPACE

namespace OfflineProcessor
{

CommandUpgradeBSP::CommandUpgradeBSP( const CommandLine& commandLine ):
BatchCommand( commandLine )
{
}

CommandUpgradeBSP::~CommandUpgradeBSP()
{
}

/**
 *	Show detailed help for upgrade-bsp command
 */
void CommandUpgradeBSP::showDetailedHelp() const
{
	std::cout << "Usage: offline_processor " << strCommand();
	std::cout << " [file] [options]" << std::endl;
	std::cout << "Loads and saves bsp data upgrading it to the latest format:" << std::endl;
	std::cout << "[file] name can be a folder name" << std::endl;

	std::cout << "Options:" << std::endl;
	std::cout << "\t-recursive\t\tProcess all files recursively starting from the given folder." << std::endl;
}

/**
 *	Show short help for upgrade-bsp command
 */
void CommandUpgradeBSP::showShortHelp() const
{
	std::cout << strCommand();
	std::cout << "\t\tUpgrade bsp in .primitives and .bsp2 to the latest format.";
}

/**
 *	Upgrade bsp in .primitives file
 */
bool	CommandUpgradeBSP::doFormatUpgradeBSPInPrimitives( const BW::string& primName )
{
	DataSectionPtr primDS = BWResource::openSection( primName );
	if (!primDS)
	{
		std::cerr << "ERROR: Unable to open primitives " << primName << std::endl;
		return false;
	}

	// convert on load
	BinaryPtr bp = primDS->readBinary( "bsp2" );
	if (!bp)
	{
		// primitive might not contain bsp section, this just means there is no bsp in there
		// return true as this is not an error
		return true;
	}
	BSPTree* pBSP = BSPTreeTool::loadBSP( bp );
	if (!pBSP)
	{
		std::cerr << "ERROR: Can't load bsp for  " << primName << std::endl;
		return false;
	}

	bp = BSPTreeTool::saveBSPInMemory( pBSP );
	bw_safe_delete( pBSP );
	MF_ASSERT( bp );
	bool bRes = primDS->writeBinary( "bsp2", bp );
	if (!bRes)
	{
		std::cerr << "ERROR: Failed ot write regenerated bsp for " << primName << std::endl;
		return false;
	}
	bRes = primDS->save( primName );
	if (!bRes)
	{
		std::cerr << "ERROR: Failed ot save regenerated bsp for " << primName << std::endl;
		return false;
	}
	
	std::cout << "regenerating bsp in "<< primName << std::endl;

	return true;
}

/**
 *	Upgrade bsp in .bsp2 file
 */
bool	CommandUpgradeBSP::doFormatUpgradeStandaloneBSPFile( const BW::string& bspName )
{
	DataSectionPtr bspSect = BWResource::openSection( bspName );
	if ( !bspSect.exists() )
	{
		std::cout << "ERROR: Could not load BSP section from "<< bspName << std::endl;
		return false;
	}

	BinaryPtr bp = bspSect->asBinary();
	if ( !bp.exists() )
	{
		std::cout << "ERROR: Could not load BSP data from file "<< bspName << std::endl;
		return false;
	}

	BSPTree* pBSP = BSPTreeTool::loadBSP( bp );
	BSPTreeTool::saveBSPInFile( pBSP, bspName.c_str() );
	bw_safe_delete( pBSP );
	std::cout << "regenerating bsp in "<< bspName << std::endl;
	return true;
}

/**
 *	Upgrade bsp in all supported file formats
 */
BatchCommand::FileProcessResult
	CommandUpgradeBSP::processFile( const BW::string& fileName, const BW::string& outFileName )
{
	const BW::string disFileName = BWResolver::dissolveFilename( fileName );
	BW::StringRef ext = BWResource::getExtension( disFileName );
	if (ext == "primitives")
	{
		return doFormatUpgradeBSPInPrimitives( disFileName ) ? RESULT_SUCCESS : RESULT_FAILURE;
	}
	else if (ext == "bsp2" )
	{
		return doFormatUpgradeStandaloneBSPFile( disFileName ) ? RESULT_SUCCESS : RESULT_FAILURE;
	}
	return RESULT_UNSUPPORTED_FILE;
}

} // namespace OfflineProcessor


BW_END_NAMESPACE

