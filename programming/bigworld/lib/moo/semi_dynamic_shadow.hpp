#pragma once

#include "cstdmf/guard.hpp"
#include "moo/render_target.hpp"
#include "chunk/chunk.hpp"

BW_BEGIN_NAMESPACE

namespace Moo
{
    class DrawContext;
//--------------------------------------------------------------------------------------------------

/**
 *  Class that provides the functions of a semi-dynamic shadows.
 *  It knows all the necessary DX-resources.
 */
class SemiDynamicShadow
{
public:
    SemiDynamicShadow(
        // Length of one side of the square throughout the covered area
        // expressed in
        // number of chunks (coveredAreaSize = 16, if the limit
        // visibility of shadows 800m).
        uint coveredAreaSize,

        // Scale whole shadow map.
        uint shadowMapSize,

        // Debug mode. Enables the display of special HUD, to
        // which shows the status of all used RT and textures.
        bool debugMode
    );        

    ~SemiDynamicShadow();

    /// Update internal data structures or behavioral conditions in
    /// accordance with chunks, Sun and current position of the camera.
    void tick( float dTime );

    /// Update the shadow map, for those chunks that need it.
    void cast( Moo::DrawContext& shadowDrawContext );

    /// Fill the buffer-shadow receiver.
    void receive();

    /// Force update of shadows cast by the specified area.
    void updateArea( const AABB& area );

    /// Display debugging information.
    void drawDebugInfo();

#ifdef EDITOR_ENABLED
    typedef void ( *GenerationFinishedCallback )();
    void setGenerationFinishedCallback(
        GenerationFinishedCallback callback );
#endif

private:
    struct Impl;
    std::auto_ptr<Impl> m_impl;

    // non copyable
    SemiDynamicShadow(const SemiDynamicShadow& other);
    SemiDynamicShadow& operator = (const SemiDynamicShadow& other);
};

//--------------------------------------------------------------------------------------------------
}

BW_END_NAMESPACE
