#include "pch.hpp"
#include "resource.h"
#include "command_process_chunks.hpp"
#include "offline_chunk_processor_manager.hpp"

#include "appmgr/options.hpp"

#include "cstdmf/memory_load.hpp"
#include "cstdmf/progress.hpp"
#include "cstdmf/string_builder.hpp"
#include "cstdmf/cpuinfo.hpp"

#include "chunk/chunk_item_amortise_delete.hpp"
#include "chunk/chunk_manager.hpp"
#include "chunk/geometry_mapping.hpp"

#include "common/space_mgr.hpp"
#include "common/editor_chunk_cache_base.hpp"
#include "common/editor_chunk_terrain_cache.hpp"
#include "common/editor_chunk_terrain_lod_cache.hpp"
#include "common/editor_chunk_terrain_base.hpp"

#include "cstdmf/bgtask_manager.hpp"
#include "cstdmf/command_line.hpp"

#include "entitydef/constants.hpp"

#include "input/input.hpp"

#include "moo/init.hpp"
#include "moo/texture_manager.hpp"

#include "physics2/material_kinds.hpp"

#include "pyscript/py_import_paths.hpp"
#include "pyscript/script.hpp"

#include "resmgr/auto_config.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/string_provider.hpp"
#include "resmgr/multi_file_system.hpp"

#include "romp/water.hpp"
#include "romp/lens_effect_manager.hpp"
#include "romp/texture_feeds.hpp"

#include "speedtree/speedtree_renderer.hpp"

#include "terrain/manager.hpp"
#include "terrain/terrain2/resource.hpp"

#include "cstdmf/debug.hpp"

#include <iomanip>

BW_BEGIN_NAMESPACE

extern int ChunkModel_token;
extern int ChunkTerrain_token;
extern int ChunkTree_token;
extern int EditorChunkNavmeshCacheBase_token;
extern int PyLogging_token;
extern int ResMgr_token;

typedef std::auto_ptr< TextureFeeds > TextureFeedsPtr;
static TextureFeedsPtr s_pTextureFeeds;

typedef std::auto_ptr< Terrain::Manager > TerrainManagerPtr;
static TerrainManagerPtr s_pTerrainManager;

typedef std::auto_ptr< LensEffectManager > LensEffectManagerPtr;
LensEffectManagerPtr s_pLensEffectManager;

namespace OfflineProcessor
{

CommandProcessChunks::CommandProcessChunks( const CommandLine& commandLine ):
Command( commandLine )
{
}

CommandProcessChunks::~CommandProcessChunks()
{
}

void CommandProcessChunks::showDetailedHelp() const
{
	std::cout << "Usage: offline_processor "  << strCommand() << " [options]" << std::endl;
	std::cout << "Runs offline chunk processing generating navigation data:" << std::endl;
	std::cout << "Options:" << std::endl;
	std::cout << "\t-space <space path>\t\tProcess the given space" << std::endl;
	std::cout << "\t-cluster-index <cluster index>\tSet the computer index in cluster" << std::endl;
	std::cout << "\t-cluster-size <cluster size>\tSet the number of computers in cluster" << std::endl;
	std::cout << "\t-overwrite\t\t\tOverwrite all existing generated data" << std::endl;
	std::cout << "\t-nogui\t\t\t\tDisplay no GUI" << std::endl;
	std::cout << "\t-unattended\t\t\tRun offline_processor in autotest mode" << std::endl;
}

void CommandProcessChunks::showShortHelp() const
{
	std::cout << strCommand();
	std::cout << "\t\tRun offline chunk processing and generate navigation data.";
}

namespace
{

void convertSecondsToFullTime( int tick, BW::StringBuilder& strBuilder)
{
	int seconds = tick / 1000;

	if (seconds > 3600)
	{
		strBuilder.appendf( "%d hours, ", seconds / 3600);
		seconds /= 60;
	}
	if (seconds > 60)
	{
		strBuilder.appendf( "%d minutes, ", seconds / 60);
		seconds /= 60;
	}
	strBuilder.appendf( "%d seconds", seconds);
}


void disableConsoleCursor()
{
	HANDLE output = GetStdHandle( STD_OUTPUT_HANDLE );

	if (output != INVALID_HANDLE_VALUE)
	{
		CONSOLE_CURSOR_INFO cci = { sizeof( cci ), FALSE };

		SetConsoleCursorInfo( output, &cci );
	}
}


void enableConsoleCursor()
{
	HANDLE output = GetStdHandle( STD_OUTPUT_HANDLE );

	if (output != INVALID_HANDLE_VALUE)
	{
		CONSOLE_CURSOR_INFO cci = { sizeof( cci ), TRUE };

		SetConsoleCursorInfo( output, &cci );
	}
}


void ensureVisible( int row )
{
    SMALL_RECT rect; 
	HANDLE output = GetStdHandle( STD_OUTPUT_HANDLE );

	if (output != INVALID_HANDLE_VALUE)
	{
		CONSOLE_SCREEN_BUFFER_INFO scbi = { sizeof( CONSOLE_SCREEN_BUFFER_INFO ) };

		GetConsoleScreenBufferInfo( output, &scbi );

		if (scbi.dwCursorPosition.Y + row > scbi.srWindow.Bottom)
		{
			rect.Top = scbi.dwCursorPosition.Y + row - scbi.srWindow.Bottom;
			rect.Bottom = scbi.dwCursorPosition.Y + row - scbi.srWindow.Bottom;
			rect.Left = 0;
			rect.Right = 0;

			SetConsoleWindowInfo( output, FALSE, &rect );
		}
	}
}


void clearConsoleOutput()
{
	HANDLE output = GetStdHandle( STD_OUTPUT_HANDLE );

	if (output != INVALID_HANDLE_VALUE)
	{
		CONSOLE_SCREEN_BUFFER_INFO scbi = { sizeof( CONSOLE_SCREEN_BUFFER_INFO ) };
		BW::vector<CHAR_INFO> charInfos;
		CHAR_INFO charInfo;
		size_t row;

		GetConsoleScreenBufferInfo( output, &scbi );

		row = scbi.dwMaximumWindowSize.Y - scbi.dwCursorPosition.Y;

		charInfo.Attributes = scbi.wAttributes;
		charInfo.Char.UnicodeChar = ' ';
		charInfos.resize( scbi.dwMaximumWindowSize.X * row, charInfo );

		COORD size = { (SHORT)scbi.dwMaximumWindowSize.X, (SHORT)row };
		COORD coord = { 0, 0 };
		SMALL_RECT rect;

		rect.Left = scbi.dwCursorPosition.X;
		rect.Top = scbi.dwCursorPosition.Y;
		rect.Right = rect.Left + size.X - 1;
		rect.Bottom = rect.Top + size.Y - 1;
		WriteConsoleOutput( output, &charInfos[0], size, coord, &rect );
	}
}

}


class ProcessProgress : public Progress
{
	bool cancelled_;
	DWORD startTick_;
	float totalTasks_;
	float tasksLeft_;
	BW::string task_;

	void name( const BW::string& task )
	{
		task_ = task;
	}

	void length( float totalTasks )
	{
		startTick_ = GetTickCount();
		totalTasks_ = totalTasks;
		tasksLeft_ = totalTasks;
	}

	void update()
	{
		HANDLE output = GetStdHandle( STD_OUTPUT_HANDLE );

		if (output != INVALID_HANDLE_VALUE)
		{
			CONSOLE_SCREEN_BUFFER_INFO scbi = { sizeof( CONSOLE_SCREEN_BUFFER_INFO ) };
			char status[4096];
			this->generateStatus( status, ARRAY_SIZE( status) );
			BW::vector<CHAR_INFO> charInfo;
			size_t row = std::count( status, status + ARRAY_SIZE( status), '\n' );

			GetConsoleScreenBufferInfo( output, &scbi );
			charInfo.resize( scbi.dwMaximumWindowSize.X * row );

			COORD size = { (SHORT)scbi.dwMaximumWindowSize.X, (SHORT)row };
			COORD coord = { 0, 0 };
			SMALL_RECT rect;
			int statusOffset = 0;
			int charInfoOffset = 0;

			for (int y = 0; y < size.Y; ++y)
			{
				int x = 0;

				while (x < size.X)
				{
					charInfo[ charInfoOffset + x ].Attributes = scbi.wAttributes;

					if (status[statusOffset] != '\n')
					{
						charInfo[ charInfoOffset + x ].Char.UnicodeChar = status[statusOffset];

						++statusOffset;
					}
					else
					{
						charInfo[ charInfoOffset + x ].Char.UnicodeChar = ' ';
					}

					++x;
				}

				++statusOffset;
				charInfoOffset += size.X;
			}

			rect.Left = scbi.dwCursorPosition.X;
			rect.Top = scbi.dwCursorPosition.Y;
			rect.Right = rect.Left + size.X - 1;
			rect.Bottom = rect.Top + size.Y - 1;
			WriteConsoleOutput( output, &charInfo[0], size, coord, &rect );
		}
	}

	bool set( float tasksFinished )
	{
		tasksLeft_ = totalTasks_ - tasksFinished;

		update();

		return true;
	}

	bool step( float step )
	{
		tasksLeft_ = std::max<float>( 0.f, tasksLeft_ - step );

		update();

		return true;
	}

public:
	ProcessProgress()
	{
		cancelled_ = false;
		startTick_ = 0;
		totalTasks_ = tasksLeft_ = 0.f;
	}

	bool started() const
	{
		return startTick_ != 0;
	}

	bool ended() const
	{
		return tasksLeft_ == 0;
	}

	const BW::string& task() const
	{
		return task_;
	}

	DWORD tickElapsed() const
	{
		return GetTickCount() - startTick_;
	}

	DWORD averageTickPerTask() const
	{
		if (totalTasks() - tasksLeft())
		{
			return DWORD( tickElapsed() / ( totalTasks() - tasksLeft() ) );
		}

		return tickElapsed();
	}

	float totalTasks() const
	{
		return totalTasks_;
	}

	float tasksLeft() const
	{
		return tasksLeft_;
	}

	void generateStatus(char* outputBuf, size_t sizeBuf) const
	{
		BW::StringBuilder strBuilder( outputBuf, sizeBuf );

		if (started() && !ended())
		{
			int totalTask = int( totalTasks() );
			int taskProcessed = totalTask - int( tasksLeft() );
			DWORD ticks = tickElapsed();

			strBuilder.appendf( "%s\n", this->task().c_str() );
			strBuilder.append( "========================================================\n" );
			strBuilder.appendf( "%d of %d tasks processed\n", taskProcessed, totalTask );
			strBuilder.appendf( "%d chunks loaded",
				ChunkManager::instance().cameraSpace()->boundChunkNum() );
			if (ChunkManager::instance().busy())
			{
				strBuilder.append( " and is loading\n" );
			}
			else
			{
				strBuilder.append( "\n" );
			}
			convertSecondsToFullTime( tickElapsed(), strBuilder );
			strBuilder.append( " elapsed\n" );
			strBuilder.appendf( "average %2.4f seconds per task\n", averageTickPerTask() / 1000.0f );
			strBuilder.append( "approximately " );
			convertSecondsToFullTime( int( averageTickPerTask() * tasksLeft() ), strBuilder );
			strBuilder.append( " left\n");
			strBuilder.appendf( "Memory load: %d%%\n", (int)Memory::memoryLoad() );
		}
		else
		{
			strBuilder.append( "Initialising ...\n" );
		}
	}

	void cancel()
	{
		cancelled_ = true;
	}

	bool isCancelled()
	{
		return cancelled_;
	}
};


static int s_chunkTokenSet = ChunkModel_token | ChunkTerrain_token | 
	ChunkTree_token | EditorChunkNavmeshCacheBase_token | PyLogging_token | ResMgr_token;


HWND				g_hWnd;
ProcessProgress		g_processProgress;

static AutoConfigString s_engineConfigXML("system/engineConfigXML");

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass( HINSTANCE hInstance );
BOOL				InitInstance( HINSTANCE );
void				ExitInstance();
LRESULT CALLBACK	WndProc( HWND, UINT, WPARAM, LPARAM );


static BOOL WINAPI consoleCtrlHandler( DWORD dwCtrlType )
{
	g_processProgress.cancel();
	return TRUE;
}

bool	CommandProcessChunks::process()
{
	int exitCode = 1;
	unsigned int clusterSize = 1;
	unsigned int clusterIndex = 0;

	if (commandLine_.hasParam( "cluster-index" ) &&
		commandLine_.hasParam( "cluster-size" ))
	{
		clusterSize = std::max<unsigned int>(
			atoi( commandLine_.getParam( "cluster-index" ) ), 1 );
		clusterIndex = std::min<unsigned int>(
			atoi( commandLine_.getParam( "cluster-size" ) ), clusterSize - 1 );
	}

	MyRegisterClass( (HINSTANCE)GetModuleHandle( NULL ) );

	// Perform application initialization:
	if (!InitInstance( (HINSTANCE)GetModuleHandle( NULL ) ))
	{
		return FALSE;
	}

	SetConsoleCtrlHandler( consoleCtrlHandler, TRUE );
	

	#if ENABLE_MULTITHREADED_COLLISION 
	ChunkSpace::usingMultiThreadedCollision( true );
	#endif //ENABLE_MULTITHREADED_COLLISION

	CpuInfo cpuInfo;
	OfflineChunkProcessorManager processorManager( cpuInfo.numberOfSystemCores(),
		clusterSize, clusterIndex );
	BW::string space = commandLine_.getParam( "space" );
	bool nogui = commandLine_.hasParam( "nogui" );

	if (space.empty() && !nogui)
	{
		BW::wstring wpath = bw_utf8tow( BWResolver::resolveFilename( "spaces/highlands" ) );
		wchar_t fullPath[MAX_PATH];
		wchar_t* filePart = NULL;

		std::replace( wpath.begin(), wpath.end(), '/', '\\' );

		if (GetFullPathName( wpath.c_str(), ARRAY_SIZE( fullPath ), fullPath, &filePart ))
		{
			wpath = fullPath;
		}

		space = SpaceNameManager::browseForSpaces( g_hWnd, wpath );

		if (!space.empty())
		{
			space = BWResolver::dissolveFilename( space );
		}
	}

	DataSectionPtr spaceSettings = 
		BWResource::openSection( space + "/space.settings" );

	if (!space.empty() && spaceSettings != NULL)
	{
		disableConsoleCursor();
		ensureVisible( 7 );

		processorManager.init( space );

		ChunkSaver chunkSaver;
		CdataSaver thumbnailSaver;
		bool quitting = false;
		BW::string navmeshGenerator = 
			spaceSettings->readString( "navmeshGenerator" );

		if (navmeshGenerator != "recast")
		{
			BW::wstring errorMsg = L"The navmeshGenerator specified in space.settings is not supported.\n";
			errorMsg += L"offline_processor supports \"recast\" only.";
			if(!nogui)
			{
				MessageBox( g_hWnd, errorMsg.c_str(), L"Warning",
					MB_OK | MB_ICONWARNING );
			}
			else
			{
				std::cerr << "The navmeshGenerator specified in space.settings is not supported.offline_processor supports \"recast\" only." << std::endl;
			}
			ExitInstance();
			return 1;
		}

		if (commandLine_.hasParam( "overwrite" ))
		{
			// NOTE: Until offline_processor becomes a fully fledged processor
			// we will only invalidate navigation data (the last parameter
			// controls this).
			quitting = !processorManager.invalidateAllChunks(
				processorManager.geometryMapping(), &g_processProgress,
				true );
		} 

		if (!quitting)
		{
			exitCode = !processorManager.saveChunks( processorManager.geometryMapping()->gatherChunks(),
				chunkSaver, thumbnailSaver, &g_processProgress, true );
		}

		processorManager.stopAll();

		clearConsoleOutput();
		enableConsoleCursor();
	}
	else
	{
		if (space.empty())
		{
			std::cout << "You have to specify a space using /space" << std::endl;
		}
		else
		{
			std::cout << "Cannot open space " << space << std::endl;
		}
	}

	ExitInstance();

	return exitCode == 0;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex = { sizeof(WNDCLASSEX) };

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon( hInstance, MAKEINTRESOURCE( IDI_OFFLINE_PROCESSOR ) );
	wcex.hCursor		= LoadCursor( NULL, IDC_ARROW );
	wcex.hbrBackground	= (HBRUSH)( COLOR_WINDOW + 1 );
	wcex.lpszClassName	= _T( "BWOPCLASS" );

	return RegisterClassEx(&wcex);
}


BOOL InitInstance( HINSTANCE hInstance )
{
	g_hWnd = CreateWindow( _T( "BWOPCLASS" ), _T( "BigWorld Offline Processor" ), WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

	if (!g_hWnd)
	{
		return FALSE;
	}

	BWResource* bwResource = new BWResource();
	MF_ASSERT( BWResource::pInstance() );

	// initialise the MF file services (read in the command line arguments too)
	int args = 0;

	if (!BWResource::init( args, NULL ))
		return FALSE;

	StringProvider::instance().load( BWResource::openSection("helpers/languages/files_en.xml" ) );
	StringProvider::instance().setLanguage();
	WindowTextNotifier::instance();

	Options::init( 0, 0, L"offlineprocessor.options"  );

	if(!Moo::init())
		return 0;

	new AmortiseChunkItemDelete();
	BgTaskManager::init();
	BgTaskManager::instance().startThreads( "InitInstance", 1 );

	FileIOTaskManager::init();
	FileIOTaskManager::instance().startThreads( "FileIOTaskManager", 1 );

	InputDevices * pInputDevices = new InputDevices();

	if (!InputDevices::instance().init( hInstance, g_hWnd ))
	{
		ERROR_MSG( "Init inputDevices FAILED\n" );

		return false;
	}

	if (!AutoConfig::configureAllFrom( "resources.xml" ))
	{
		CRITICAL_MSG( "Failed to load resources.xml. " );
		return false;
	}


	//Load the "engine_config.xml" file...
	DataSectionPtr configRoot = BWResource::instance().openSection( s_engineConfigXML.value() );

	if (!configRoot)
	{
		ERROR_MSG( "App::init: Couldn't open \"%s\"\n", s_engineConfigXML.value().c_str() );
		return false;
	}

	XMLSection::shouldReadXMLAttributes( configRoot->readBool( 
		"shouldReadXMLAttributes", XMLSection::shouldReadXMLAttributes() ));
	XMLSection::shouldWriteXMLAttributes( configRoot->readBool( 
		"shouldWriteXMLAttributes", XMLSection::shouldWriteXMLAttributes() ));

    Water::backgroundLoad( false );

	// Initialise python
	PyImportPaths paths;
	paths.addResPath( BWResolver::resolveFilename( 
		EntityDef::Constants::entitiesEditorPath() ) );

	if (!Script::init( paths ))
	{
		return false;
	}

	if (!MaterialKinds::init())
	{
		return false;
	}

	if (!Moo::TextureManager::instance())
		Moo::TextureManager::init();

	Moo::rc().createDevice( g_hWnd );

	if (Moo::rc().mixedVertexProcessing())
		Moo::rc().device()->SetSoftwareVertexProcessing( TRUE );
	ShowCursor( TRUE );

	// Needed to properly initialise elements such as Water
	Waters::instance().init();

#if SPEEDTREE_SUPPORT
	speedtree::SpeedTreeRenderer::enviroMinderLighting( false );
#endif

	Moo::Camera cam( 0.25f, 200.f, DEG_TO_RAD(60.f), 2.f );
	cam.aspectRatio(
		float(Moo::rc().screenWidth()) / float(Moo::rc().screenHeight()) );
	Moo::rc().camera( cam );

	s_pTerrainManager = TerrainManagerPtr( new Terrain::Manager() );
	s_pTextureFeeds = TextureFeedsPtr( new TextureFeeds() );
	s_pLensEffectManager = LensEffectManagerPtr( new LensEffectManager() );
	ChunkManager::instance().init();
	EnviroMinder::init();

	Terrain::ResourceBase::defaultStreamType( Terrain::RST_Syncronous );

	ChunkManager::instance().disableScan();

	return TRUE;
}


void ExitInstance()
{
	BgTaskManager::instance().stopAll();
	ChunkManager::instance().tick( 0.f );
	ChunkManager::instance().fini();

	MaterialKinds::fini();
	Script::fini();
	s_pTextureFeeds.reset();
	s_pTerrainManager.reset();
	s_pLensEffectManager.reset();
}


LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	switch (message)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}

} // namespace OfflineProcessor
BW_END_NAMESPACE

