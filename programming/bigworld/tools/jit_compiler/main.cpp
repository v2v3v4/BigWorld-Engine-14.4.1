#include "wtl.hpp"
#include "main_window.hpp"
#include "jit_compiler.hpp"
#include "task_store.hpp"
#include "message_loop.hpp"

#include "asset_pipeline/compiler/asset_compiler_options.hpp"

#include "resmgr/bwresource.hpp"
#include "resmgr/dataresource.hpp"
#include "resmgr/multi_file_system.hpp"

#include "cstdmf/command_line.hpp"

BW_BEGIN_NAMESPACE
DECLARE_WATCHER_DATA( NULL )
DECLARE_COPY_STACK_INFO( false )
BW_END_NAMESPACE

namespace
{
	bool init()
	{
		WTL::AtlInitCommonControls(ICC_COOL_CLASSES | ICC_BAR_CLASSES);

		// Initialise the file systems
		if (!BW::BWResource::init( BW::BWResource::appDirectory(), false ))
		{
			return false;
		}
		BW::BWResource::instance().enableModificationMonitor( true ); 

		return true;
	}

	void fini()
	{
		// Cleanup the file systems
		BW::BWResource::fini();

		::CoUninitialize();
	}
}

#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")


int WINAPI WinMain( HINSTANCE instance, HINSTANCE prevInstance, LPSTR commandLine, int showCmd )
{
	BW_SYSTEMSTAGE_MAIN();
#ifdef ENABLE_MEMTRACKER
	MemTracker::instance().setCrashOnLeak( true );
#endif

	int exitCode = 1;
	if (init())
	{
		BW::AssetCompilerOptions options;
		options.parseCommandLine( BW::bw_wtoutf8( GetCommandLineW() ) );

		BW::TaskStore store;
		BW::MainMessageLoop messageLoop;
		BW::JITCompiler jitCompiler(store);
		BW::MainWindow mainWindow(store, messageLoop, jitCompiler, jitCompiler);
		options.apply(jitCompiler);
		jitCompiler.initPlugins();
		jitCompiler.initCompiler();

		if (mainWindow.init())
		{
			// Spawn another thread to do the disk scanning for the JIT Compiler
			auto scanningThreadFunc = [](void * arg)
			{
				BW::JITCompiler * jitCompiler = static_cast< BW::JITCompiler *>( arg );
				jitCompiler->scanningThreadMain();
			};
			BW::SimpleThread jitScanningThread(scanningThreadFunc, &jitCompiler, "JITCompiler Scanning Thread");

			auto managingThreadFunc = [](void * arg)
			{
				BW::JITCompiler * jitCompiler = static_cast< BW::JITCompiler *>( arg );
				jitCompiler->managingThreadMain();
			};
			BW::SimpleThread jitManagingThread(managingThreadFunc, &jitCompiler, "JITCompiler Process Thread");

			exitCode = messageLoop.run();

			jitCompiler.stop();
		}

		mainWindow.fini();

		jitCompiler.finiCompiler();
		jitCompiler.finiPlugins();
	}

	fini();

	return exitCode;
}
