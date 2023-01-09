#include "pch.hpp"

#include "resmgr/bwresource.hpp"
#include "resmgr/multi_file_system.hpp"
#include "cstdmf/concurrency.hpp"
#include "cstdmf/bw_util.hpp"


#ifndef MF_SERVER
#include "moo/init.hpp"
#include "moo/render_context.hpp"
#include "moo/texture_manager.hpp"
#include "moo/texture_compressor.hpp"
#include "moo/mrt_support.hpp"
#endif // MF_SERVER

#include "moo/convert_texture_tool.hpp"
#include "cstdmf/debug.hpp"
#include "moo/texture_detail_level.hpp"
#include "moo/texture_helpers.hpp"
#include "moo/renderer.hpp"

DECLARE_DEBUG_COMPONENT2( "ConvertTextureTool", 0 )

BW_BEGIN_NAMESPACE

namespace Moo
{
#ifndef MF_SERVER
    LRESULT CALLBACK WndProcConvertTexture( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
    {
        BW_GUARD;
 
        return DefWindowProc( hWnd, msg, wParam, lParam );
    }
#endif // MF_SERVER


    // initialise the renderer, 
    //  so the texture manager is active.
    bool ConvertTextureTool::initMoo()
    {
        BW_GUARD;

#ifndef MF_SERVER
        if (Moo::isInitialised() && Moo::rc().isValid())
            return true;

        // Initialise moo
        Moo::init( true, true );

        // make sure we havent already initialised a window,
        //  like if we are running multiple tests one after the other
        if (!hWnd_)
        {
            // Create window to be able to init moo
            WNDCLASS wc = { 0, WndProcConvertTexture, 0, 0, NULL, NULL, NULL, NULL, NULL, L"ConvertTextureTool" };
            if ( !RegisterClass( &wc ) )
            {
                printf( "Error: Couldn't register Moo's window class\n" );
                return false;
            }

            // create the actual window
            hWnd_ = CreateWindow(
                L"ConvertTextureTool", L"ConvertTextureTool",
                WS_OVERLAPPEDWINDOW,
                CW_USEDEFAULT, CW_USEDEFAULT,
                100, 100,
                NULL, NULL, NULL, 0 );

            MF_ASSERT( pRenderer_ == NULL );
            pRenderer_ = new Renderer();
            pRenderer_->init( true, true );

            if ( !hWnd_ )
            {
                printf( "Error: Couldn't create Moo's window\n" );
                return false;
            }
            else
            {
                OSVERSIONINFO ose = { sizeof( ose ) };
                GetVersionEx( &ose );

                bool isVista = ose.dwMajorVersion > 5;
                bool forceRef = !isVista;

                if ( !Moo::rc().createDevice( hWnd_,0,0,true,false,Vector2(0,0),true,forceRef ) )   
                {
                    printf( "Error: Couldn't create Moo's render device\n" );
                    return false;
                }
            }
        }


#endif // MF_SERVER
        return true;
    }

    void ConvertTextureTool::MooFini()
    {
        BW_GUARD;

        if (hWnd_)
        {
            Moo::rc().releaseDevice();
            DestroyWindow(hWnd_);
            hWnd_ = NULL;
            WNDCLASS wc = { 0, WndProcConvertTexture, 0, 0, NULL, NULL, NULL, NULL, NULL, L"ConvertTextureTool" };
            if ( !UnregisterClass(L"ConvertTextureTool",NULL))
            {
                printf( "Error: Couldn't UnRegister Moo's window class\n" );
            }
        }
        Moo::fini();

        delete pRenderer_;
        pRenderer_ = NULL;
    }

    bool ConvertTextureTool::initResources()
    {
        BW_GUARD;

        bool bSuccessfulInit = true;

        // init fileio system
#ifndef MF_SERVER
        bSuccessfulInit = FileIOTaskManager::init();
        if (bSuccessfulInit == false)
        {
            ERROR_MSG("Could not initialise FileIOTaskManager a new ConvertTextureTool, texture conversion failed");
        }

        if (bSuccessfulInit && FileIOTaskManager::instance().numRunningThreads() == 0)
        {
            FileIOTaskManager::instance().startThreads("File IO Thread: ConvertTextureTool", 1);
        }
#endif // MF_SERVER

        if(bSuccessfulInit == true)
        {
            bSuccessfulInit = initMoo();
        }

        return(bSuccessfulInit);
    }

    void ConvertTextureTool::resourcesFini()
    {
        MooFini();

#ifndef MF_SERVER
        // Finialise moo
        if (FileIOTaskManager::pInstance() != NULL)
        {
            FileIOTaskManager::fini();
        }
#endif // MF_SERVER
    }

    // convert a texture here.
    bool ConvertTextureTool::convert(const BW::string& srcName,
        const BW::string& destName, 
        Moo::TextureDetailLevelPtr& texLod,
        const bool bCompressionOption )
    {
        TRACE_MSG( "Converting texture '%s'.\n",
            srcName.c_str() );

        Moo::TextureCompressor tc( srcName );
        BinaryPtr bin = tc.compressToMemory( texLod, bCompressionOption );
        if (bin)
        {
            bool bSuccess = BWResource::instance().fileSystem()->writeFile(
                destName, bin, true );
            if (!bSuccess)
            {
                char lastError[ 1024 ] = { 0 };
                BWUtil::getLastErrorMsg( lastError, ARRAY_SIZE( lastError ) );
                WARNING_MSG("Failed to convert %s to %s : %s\n",
                    srcName.c_str(), destName.c_str(), lastError );
                return false;
            }
            return true;
        }
        else
        {
            return false;
        }
    }
}
BW_END_NAMESPACE

