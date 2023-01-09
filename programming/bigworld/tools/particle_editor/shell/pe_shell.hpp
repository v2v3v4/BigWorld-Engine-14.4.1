#ifndef PE_SHELL_HPP
#define PE_SHELL_HPP

#include "fwd.hpp"
#include <iostream>
#include "cstdmf/cstdmf_windows.hpp"
#include "common/tools_camera.hpp"

#include "moo/renderer.hpp"

#include "cstdmf/debug_message_callbacks.hpp"


BW_BEGIN_NAMESPACE

class AssetClient;
class ChunkSpace;
typedef SmartPointer<ChunkSpace> ChunkSpacePtr;
class FontManager;
class TextureFeeds;
class LensEffectManager;

namespace Terrain
{
    class Manager;
}

namespace PostProcessing
{
    class Manager;
}

/*
 *  Interface to the BigWorld message handler
 */
struct PeShellDebugMessageCallback : public DebugMessageCallback
{
    PeShellDebugMessageCallback()
    {
    }

    ~PeShellDebugMessageCallback()
    {
    }

    /*virtual*/ bool handleMessage(
        DebugMessagePriority messagePriority, const char * pCategory,
        DebugMessageSource messageSource, const LogMetaData & metaData,
        char const * pFormat, va_list argPtr );
};


/**
 *  This class adds solution specific routines to what already exists in
 *  appmgr/app.cpp
 */
class PeShell
{
public:
    PeShell();

    ~PeShell();

    static PeShell & instance();

    static bool hasInstance();

    static bool 
    initApp
    ( 
        HINSTANCE       hInstance, 
        HWND            hWndApp, 
        HWND            hWndGraphics 
    ); 
    
    bool init
    (
        HINSTANCE       hInstance, 
        HWND            hWndApp, 
        HWND            hWndGraphics 
    );

    void fini();

    HINSTANCE & hInstance();

    HWND &hWndApp();

    HWND &hWndGraphics();

    RompHarness &romp();

    ToolsCamera &camera();

    Floor &floor();

    POINT currentCursorPosition() const;

private:
    friend std::ostream& operator<<(std::ostream&, const PeShell&);

    friend struct PeShellDebugMessageCallback;

    static bool messageHandler( DebugMessagePriority messagePriority,
            char const * pFormat, va_list argPtr );

    bool initGraphics();

    void finiGraphics();

    bool initScripts();

    void finiScripts();

    bool initConsoles();

    bool initErrorHandling();

    bool initRomp();

    bool initCamera();

    bool initSound();

    PeShellDebugMessageCallback debugMessageCallback_;

private:
    PeShell(PeShell const&);                // not permitted
    PeShell& operator=(PeShell const &);    // not permitted

private:    
    static PeShell      *s_instance_;       // the instance of this class (there should be only one!)
    bool                inited_;
    HINSTANCE           hInstance_;         // the current instance of the application
    HWND                hWndApp_;           // application window
    HWND                hWndGraphics_;      // 3D window
    RompHarness         *romp_;
    ToolsCameraPtr      camera_;
    Floor*              floor_;
    ChunkSpacePtr       space_;

    std::auto_ptr<Renderer> m_renderer;

    std::auto_ptr< AssetClient > pAssetClient_;
    typedef std::auto_ptr< FontManager > FontManagerPtr;
    FontManagerPtr pFontManager_;

    typedef std::auto_ptr< TextureFeeds > TextureFeedsPtr;
    TextureFeedsPtr pTextureFeeds_;

    typedef std::auto_ptr< Terrain::Manager > TerrainManagerPtr;
    TerrainManagerPtr pTerrainManager_;

    typedef std::auto_ptr< PostProcessing::Manager > PostProcessingManagerPtr;
    PostProcessingManagerPtr pPostProcessingManager_;

    typedef std::auto_ptr< LensEffectManager > LensEffectManagerPtr;
    LensEffectManagerPtr pLensEffectManager_;
};

BW_END_NAMESPACE

#endif // PE_SHELL_HPP
