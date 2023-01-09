// offline_processor_main.cpp : Defines the entry point for the application.
//

#include "pch.hpp"
#include "cstdmf\stdmf.hpp"
#include "cstdmf\string_utils.hpp"

#include "command.hpp"
#include "command_manager.hpp"

#include "command_upgrade_bsp.hpp"
#include "command_process_chunks.hpp"

#include "chunk/user_data_object_link_data_type.hpp"

BW_BEGIN_NAMESPACE
DECLARE_WATCHER_DATA( NULL )
DECLARE_COPY_STACK_INFO( false )
DEFINE_CREATE_EDITOR_PROPERTY_STUB
BW_END_NAMESPACE

int main()
{
	BW_SYSTEMSTAGE_MAIN();
	BW_NAMESPACE OfflineProcessor::CommandManager manager;
	bool bSuccess = manager.run();
	return bSuccess ? 0 : 1;
}


