#ifndef PROCESSOR_COMMAND_PROCESS_CHUNKS_HPP
#define PROCESSOR_COMMAND_PROCESS_CHUNKS_HPP

#include "command.hpp"

BW_BEGIN_NAMESPACE

namespace OfflineProcessor
{

/**
 * chunk processing command.
 * Runs offline chunk processing generating navigation data
 * Supports interactive mode ( default ) and command line only mode
 */
class CommandProcessChunks : public Command
{
public:
	CommandProcessChunks( const CommandLine& commandLine );
	virtual ~CommandProcessChunks();

	virtual const char*		strCommand() const { return "process-chunks"; };

	virtual void	showDetailedHelp() const;
	virtual void	showShortHelp() const;

	virtual bool	process();
};

} // namespace OfflineProcessor
BW_END_NAMESPACE

#endif // PROCESSOR_COMMAND_PROCESS_CHUNKS_HPP
