#ifndef ENVIRO_MINDER_HPP
#define ENVIRO_MINDER_HPP

#include "cstdmf/smartpointer.hpp"

#include "pyscript/script.hpp"
#include "pyscript/script_math.hpp"

#include "moo/camera_planes_setter.hpp"
#include "moo/viewport_setter.hpp"
#include "moo/visual.hpp"
#include "hdr_settings.hpp"
#include "ssao_settings.hpp"


BW_BEGIN_NAMESPACE

class DataSection;
typedef SmartPointer<DataSection> DataSectionPtr;

class TimeOfDay;
class Weather;
class SkyGradientDome;
class SunAndMoon;
class Seas;
class SkyLightMap;
class Flora;
class Decal;
class EnvironmentCubeMap;
class SkyDomeOccluder;
class SkyDomeShadows;
class ZBufferOccluder;
class ChunkObstacleOccluder;
class FootPrintRenderer;

struct WeatherSettings;
class PyMetaParticleSystem;

typedef uint32 ChunkSpaceID;

class PyAttachment;
typedef ScriptObjectPtr<PyAttachment> PyAttachmentPtr;

namespace Moo
{
    class DrawContext;
}
/**
 *  Something that wants to be attached to the main player model.
 */
struct PlayerAttachment
{
    PyMetaParticleSystem* pSystem;
    BW::string      onNode;
};

/**
 *  A vector of PlayerAttachment objects
 */
class PlayerAttachments : public BW::vector<PlayerAttachment>
{
public:
    void add( PyMetaParticleSystem* pSys, const BW::string & node );
};


/**
 *  This class minds a whole lot of pointers to classes used to render
 *  and control the physical environment in the game. It deletes any
 *  pointers it holds in its destructor.
 */
class EnviroMinder
{
public:
    EnviroMinder(ChunkSpaceID spaceID);
    ~EnviroMinder();

    static void init();
    static void fini();

    enum
    {
        // Sorted in drawing order
        DRAW_SUN_AND_MOON   = 0x0001,
        DRAW_SKY_GRADIENT   = 0x0002,
        DRAW_SKY_BOXES      = 0x0004,
        DRAW_SUN_FLARE      = 0x0008,

        DRAW_ALL            = 0xffff
    };

    bool load( DataSectionPtr pDS, bool loadFromExternal = true );
#ifdef EDITOR_ENABLED
    bool save( DataSectionPtr pDS, bool saveToExternal = true ) const;
#endif
    const DataSectionPtr pData()            { return data_; }

    void tick( float dTime, bool outside,
        const WeatherSettings * pWeatherOverride = NULL );

    void allowUpdate(bool val) { allowUpdate_ = val; }

    void activate();
    void deactivate();

    //-- get access to the HDR settings.
    const HDRSettings&  hdrSettings() const { return hdrSettings_; }
    void                hdrSettings(const HDRSettings& newSettings);
    //-- get access to the SSAO settings.
    const SSAOSettings& ssaoSettings() const { return ssaoSettings_; }
    void                ssaoSettings(const SSAOSettings& newSettings);
    
    float farPlane() const;
    void setFarPlane(float farPlane);
    
    float farPlaneBaseLine() const;
    void setFarPlaneBaseLine(float farPlaneBaseLine);

    void updateAnimations();
    void drawHind( float dTime, uint32 drawWhat = DRAW_ALL, bool showWeather = true );
    void drawHindDelayed( float dTime, uint32 drawWhat = DRAW_ALL );
    void drawFore(  float dTime,
                    bool showWeather = true,
                    bool showFlora = true,
                    bool deprecated_showFloraShadowing = false,
                    bool drawOverLays = true,
                    bool drawObjects = true );
    void tickSkyDomes( float dTime );
    void drawSkyDomes( bool isOcclusionPass = false );
    void drawSkySunCloudsMoon( float dTime, uint32 drawWhat );
    TimeOfDay *         timeOfDay()         { return timeOfDay_; }
    Weather *           weather()           { return weather_; }
    const Weather*      weather() const     { return weather_; }
    SkyGradientDome *   skyGradientDome()   { return skyGradientDome_; }
    SunAndMoon *        sunAndMoon()        { return sunAndMoon_; }
    Seas *              seas()              { return seas_; }
    SkyLightMap *       skyLightMap()       { return skyLightMap_; }
    Flora *             flora()             { return flora_; }
    Decal *             decal()             { return decal_; }
    EnvironmentCubeMap *environmentCubeMap(){ return environmentCubeMap_; }
#ifndef EDITOR_ENABLED
    FootPrintRenderer * footPrintRenderer() { return footPrintRenderer_; }
#endif // EDITOR_ENABLED

    BW::vector<Moo::VisualPtr> &skyDomes() { return skyDomes_; }
    size_t skyDomesPartition() const        { return skyDomesPartition_; }
    void skyDomesPartition( size_t p )      { skyDomesPartition_ = p; }

    void addPySkyDome( PyAttachmentPtr att, Vector4ProviderPtr provider );
    void delPySkyDome( PyAttachmentPtr att, Vector4ProviderPtr provider );
    void delStaticSkyBoxes();

    PlayerAttachments & playerAttachments() { return playerAttachments_; }
    bool playerDead() const                 { return playerDead_; }
    void playerDead( bool isDead )          { playerDead_ = isDead; }

#ifdef EDITOR_ENABLED
    BW::string         timeOfDayFile() const;
    void                timeOfDayFile(BW::string const &filename);

    BW::string         skyGradientDomeFile() const;
    void                skyGradientDomeFile(BW::string const &filename);
#endif

    BW::vector<Vector4ProviderPtr> skyDomeControllers_;
    Vector4ProviderPtr  sunlightControl_;
    Vector4ProviderPtr  ambientControl_;
    Vector4ProviderPtr  fogControl_;

private:
    EnviroMinder( const EnviroMinder& );
    EnviroMinder& operator=( const EnviroMinder& );

    void decideLightingAndFog();

    void loadHDR(const DataSectionPtr& data, bool loadFromExternal);
    void loadSSAO(const DataSectionPtr& data, bool loadFromExternal);
    void loadTimeOfDay(DataSectionPtr data, bool loadFromExternal);
    void loadSkyGradientDome(DataSectionPtr data, bool loadFromExternal,
            float &farPlane);

    void beginClipPlaneBiasDraw( float bias );
    void endClipPlaneBiasDraw();

    void updateSkyDomes();

    TimeOfDay         *           timeOfDay_;
    Weather           *           weather_;
    SkyGradientDome   *           skyGradientDome_;
    SunAndMoon        *           sunAndMoon_;
    Seas              *           seas_;
    SkyDomeShadows    *           skyDomeShadows_;
    SkyDomeOccluder   *           skyDomeOccluder_;
    ZBufferOccluder   *           zBufferOccluder_;
    ChunkObstacleOccluder *       chunkObstacleOccluder_;
    HDRSettings                   hdrSettings_;
    SSAOSettings                  ssaoSettings_;
    SkyLightMap       *           skyLightMap_;
    Flora             *           flora_;
    Decal             *           decal_;
    EnvironmentCubeMap *           environmentCubeMap_;
#ifndef EDITOR_ENABLED
    FootPrintRenderer *         footPrintRenderer_;
#endif // EDITOR_ENABLED
    
    BW::vector<Moo::VisualPtr>  skyDomes_;
    size_t                      skyDomesPartition_;
    BW::vector<PyAttachmentPtr> pySkyDomes_;
    PlayerAttachments           playerAttachments_;
    bool                        playerDead_;
    DataSectionPtr              data_;
    static EnviroMinder*        s_activatedEM_;

    float                       farPlaneBaseLine_;
    float                       farPlane_;
    
    bool                        allowUpdate_;

    // Saved in begin/end ClipPlaneBiasDraw
    float                       savedNearPlane_;
    float                       savedFarPlane_;

#ifdef EDITOR_ENABLED
    BW::string                 todFile_;
    BW::string                 sgdFile_;
#endif
    Moo::DrawContext*           delayedDrawContext_;
};


/**
 *  Manages all graphics settings related to the EnviroMinder.
 */
class EnviroMinderSettings
{
public:
    void init(DataSectionPtr resXML);
    
    void activate(EnviroMinder * activeMinder);     
    void refresh();
    
    bool isInitialised() const;

    static EnviroMinderSettings & instance();

private:
    typedef BW::vector<float> FloatVector;
    typedef Moo::GraphicsSetting::GraphicsSettingPtr GraphicsSettingPtr;

#ifndef EDITOR_ENABLED
    void setFarPlaneOption(int optionIndex);

    GraphicsSettingPtr farPlaneSettings_;
    FloatVector        farPlaneOptions_;
#endif // EDITOR_ENABLED

    EnviroMinder     * activeMinder_;

private:
    EnviroMinderSettings() :
#ifndef EDITOR_ENABLED
        farPlaneSettings_(NULL),
        farPlaneOptions_(),
#endif // EDITOR_ENABLED
        activeMinder_(NULL)
    {}
};


/**
 * This class commits fog on destruction.
 */
class ScopedUpdateFogParams
{
public:
    ~ScopedUpdateFogParams();
};

/**
 *  This class encapsulates the graphics setup required for rendering sky boxes
 *  and similar elements. It's used in both EnviroMinder and in ModelEditor
 *  directly when loading skyboxes as models, so changes here need to be tested
 *  when editing skyboxes in ModelEditor.
 */
class SkyBoxScopedSetup
{
public:
    SkyBoxScopedSetup();
private:
    // Members are reverse ordered for scoped destructor behaviour:
    // - view port
    // - camera planes
    // - fog commit
    ScopedUpdateFogParams scf_;
    Moo::CameraPlanesSetter cps_;
    Moo::ViewportSetter vps_;
};


#ifdef CODE_INLINE
#include "enviro_minder.ipp"
#endif

BW_END_NAMESPACE

#endif // ENVIRO_MINDER_HPP
