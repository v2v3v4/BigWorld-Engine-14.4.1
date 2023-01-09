#pragma once

#include "resource.h"
#include "cstdmf/debug_message_callbacks.hpp"

BW_BEGIN_NAMESPACE

namespace OfflineProcessor
{

struct CommandManagerDebugMessageCallback : public DebugMessageCallback
{
	virtual bool handleMessage(
		DebugMessagePriority messagePriority, const char * pCategory,
		DebugMessageSource messageSource, const LogMetaData & metaData,
		const char * pFormat, va_list argPtr );
	bool verbose_;
};
	
/**
 *	Command Manager encapsulates all supported commands and
 *  provides a simple interface to run them
 */
class CommandManager
{
public:
	CommandManager();
	// alternative constructor for unit tests
	CommandManager( const char *testCommandLine ); 
	~CommandManager();

	bool	run();

private:
	void	showAllSupportedCommands() const;
	void	displayHelp( const char* helpCmd ) const;
	void	init();
	void	fini();

	Command*	selectCommand() const;

	typedef BW::vector<Command*> ProcessorCommands;

	ProcessorCommands	commands_;
	CommandLine			applicationCommandLine_;
	CommandManagerDebugMessageCallback debugCallback_;
	bool	verbose_;
};

} // namespace OfflineProcessor
BW_END_NAMESPACE

