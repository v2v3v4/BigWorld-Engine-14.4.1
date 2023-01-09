// command_manager.cpp : Defines the CommandManager class 
//	(main one for offline_processor, for the actual main entry point see offline_processor_main.cpp)
//

#include "pch.hpp"
#include "cstdmf\cstdmf_init.hpp"
#include "cstdmf\stdmf.hpp"
#include "cstdmf\string_utils.hpp"
#include "cstdmf\debug.hpp"

DECLARE_DEBUG_COMPONENT2( "CommandManager", 0 )

#include "command.hpp"
#include "command_manager.hpp"

#include "command_upgrade_bsp.hpp"
#include "command_process_chunks.hpp"

#include <string.h>

BW_BEGIN_NAMESPACE

namespace OfflineProcessor
{
/**
 *	Constructor. Register/unregister all supported commands.
 */
CommandManager::CommandManager():
applicationCommandLine_( bw_wtoutf8( GetCommandLine() ).c_str() ),
verbose_( false )
{
	this->init();
	
}

/************************************************************************/
/* alternative constructor for unit testing the offlineprocessor.		*/
/*	Instead of using the command line args, this constructor takes		*/
/*	optional arguments from each test									*/
/************************************************************************/
CommandManager::CommandManager(const char *testCommandLine):
	applicationCommandLine_( testCommandLine),
	verbose_( false )
{
	this->init();
}

CommandManager::~CommandManager()
{
	for (ProcessorCommands::iterator it = commands_.begin();
		it != commands_.end(); ++it)
	{
		bw_safe_delete( *it );
	}

	this->fini();
}

void CommandManager::init()
{
	commands_.push_back( new CommandProcessChunks( applicationCommandLine_ ) );
	commands_.push_back( new CommandUpgradeBSP( applicationCommandLine_ ) );

	DebugFilter::instance().addMessageCallback(
		&debugCallback_ );
}

void	CommandManager::fini()
{
	DebugFilter::instance().deleteMessageCallback( &(this->debugCallback_) );
}

bool CommandManagerDebugMessageCallback::handleMessage( 
		DebugMessagePriority messagePriority, const char * pCategory,
		DebugMessageSource /*messageSource*/, const LogMetaData & /*metaData*/,
		const char * pFormat, va_list argPtr )
{
	BW_GUARD;

	// only filter out trace messages or lower priority
	//  so error & all the others still appear if needed.
	if (messagePriority <= MESSAGE_PRIORITY_TRACE)
	{
		if (verbose_)
		{
			// send CommandManager & ConvertTextureTask & ConvertTextureTool
			//  categories to the console
			if (pCategory &&
				((strcmp( pCategory, "CommandManager") == 0) ||
					(strcmp( pCategory, "ConvertTextureTask") == 0)))
			{
				const char * pPriorityName = messagePrefix( messagePriority );

				fprintf( DebugFilter::consoleOutputFile(), "%s: ", pPriorityName );
				fprintf( DebugFilter::consoleOutputFile(), "[%s] ", pCategory );
				vfprintf( DebugFilter::consoleOutputFile(), pFormat, argPtr );
			}
			// tell the message system to continue processing this message,
			//  since we are verbose.
			return false; 
		}
		else 
		{
			// swallow the message, as we don't want this message spamming up
			//	the debug output, since we are not in verbose mode.
			return true;
		}
	}
	
	// tell the message system to continue processing this message
	return false;
}

/**
 *	Select command based on command line arguments
 *  Usage: offline_processor [command] [options]
 */
Command* CommandManager::selectCommand() const
{
	for (ProcessorCommands::const_iterator it = commands_.begin();
		it != commands_.end(); ++it)
	{
		// format is: offline_processor [command] [options]
		if (applicationCommandLine_.hasFullParam( (*it)->strCommand(), 1 ) )
		{
			return *it;
		}
	}
	return NULL;
}

/**
 *	Display information for all supported commands
 */
void CommandManager::showAllSupportedCommands() const
{
	std::cout << "BigWorld offline processor" << std::endl << std::endl;
	std::cout << "Usage: offline_processor [command] [options]" << std::endl << std::endl;
	std::cout << "Commands:" << std::endl << std::endl;

	for (ProcessorCommands::const_iterator it = commands_.begin();
		it != commands_.end(); ++it)
	{
		(*it)->showShortHelp();
		std::cout << std::endl;
	}

	std::cout << "help\t\t\tShow this help" << std::endl << std::endl;

	std::cout << "Use offline_processor help [command]";
	std::cout <<" for detailed help" << std::endl;
}

/**
 *	Display detailed help for given command if it's supported
 */
void CommandManager::displayHelp( const char* helpCmd ) const
{
	for (ProcessorCommands::const_iterator it = commands_.begin();
		it != commands_.end(); ++it)
	{
		if (strcmp((*it)->strCommand(), helpCmd) == 0)
		{
			(*it)->showDetailedHelp();
			return;
		}
	}
	this->showAllSupportedCommands( );
}

/**
 *	Run command based on command line arguments or display help if there are no arguments 
 */
bool CommandManager::run()
{
	const char* helpCmd = applicationCommandLine_.getFullParam( "help");
	if (helpCmd)
	{
		this->displayHelp( helpCmd );
		return true;
	}

	const char* unattendedCmd = applicationCommandLine_.getFullParam( "unattended");
	if (unattendedCmd)
	{
		CStdMf::checkUnattended();
	}
	
	// tell the commandmanager & converttexturetask messages to go to the console
	this->debugCallback_.verbose_ = applicationCommandLine_.hasParam("verbose");

	// make sure all messages get sent through
	DebugMessagePriority previousThreshold = DebugFilter::instance().filterThreshold();
	if (this->debugCallback_.verbose_)
	{
		DebugFilter::instance().filterThreshold( MESSAGE_PRIORITY_TRACE );
	}
	

	bool bRes = true;

	Command* pCommand = this->selectCommand();
	if (pCommand)
	{
		bRes = pCommand->process();
	}
	else
	{
		bRes = false;

		// see if we have a command
		const char* cmd = applicationCommandLine_.getParamByIndex( 1 );
		if (cmd != NULL && strlen(cmd) > 0)
		{
			// unknown command
			ERROR_MSG("[CommandManager::run()] ERROR: Cannot recognise command "
				"%s\n", cmd );
		}
		this->showAllSupportedCommands();
	}

	// restore debugfilter threshold
	DebugFilter::instance().filterThreshold(previousThreshold);

	return bRes;
}

} // namespace OfflineProcessor

BW_END_NAMESPACE


