#ifndef PROCESSOR_COMMAND_HPP
#define PROCESSOR_COMMAND_HPP

#include "cstdmf/command_line.hpp"
#include "resmgr/multi_file_system.hpp"

BW_BEGIN_NAMESPACE

namespace OfflineProcessor
{
/**
 * Interface for all offline processor commands
 * Each derived command has to implement:
 * - name
 * - short and detailed help outputs
 * - process function
 */
class Command
{
public:
	Command( const CommandLine& commandLine );
	virtual ~Command() {}

	virtual bool			initCommand() { return true; }
	virtual void			finiCommand() {} 

	virtual const char*		strCommand() const = 0;
	virtual void			showDetailedHelp() const = 0;
	virtual void			showShortHelp() const = 0;
	virtual bool			process() = 0;

protected:
	const CommandLine&	commandLine_;
};

/**
 * Specialised verion of offline processor command with batch processing support
 * Batch processing means a possiblity to pass directory path instead of file path
 * and possibly use -recusive option
 *
 * Commands which derive from this class have to implement:
 * - name
 * - short and detailed help outputs
 * - processFile function
 */
class BatchCommand : public Command
{
public:
	BatchCommand( const CommandLine& commandLine,
				  bool allowMissingFiles = false );
	virtual ~BatchCommand() {}

	enum FileProcessResult
	{
		RESULT_SUCCESS,
		RESULT_FAILURE,
		RESULT_UNSUPPORTED_FILE
	};

	virtual bool				process();
	virtual FileProcessResult	processFile( const BW::string& fileName, const BW::string& outFileName = "" ) = 0;

	int			numErrors() const { return numErrors_; }

protected:
	enum ResourceSystemInitResult
	{
		RES_INIT_SUCCESS_FILE,
		RES_INIT_SUCCESS_DIRECTORY,
		RES_INIT_FAILED
	};
	ResourceSystemInitResult initResourceSystem( const BW::string& fileName );

	int		processFiles( const BW::string& fileName, bool processRecursively , const BW::string& outFileName );

	bool			allowMisssingFiles_;
	int				numErrors_;

};

}
BW_END_NAMESPACE

#endif // PROCESSOR_COMMAND__HPP