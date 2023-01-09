#include "pch.hpp"

// Module Interface
#include "speedtree_renderer.hpp"
#include "speedtree_config.hpp"

#if !SPEEDTREE_SUPPORT // ------------------------------------------------------

// BW Tech Headers
#include "cstdmf/debug.hpp"
#include "math/boundbox.hpp"
#include "physics2/bsp.hpp"
#include "resmgr/datasection.hpp"
#include <stdexcept>

DECLARE_DEBUG_COMPONENT2( "SpeedTree", 0 )


BW_BEGIN_NAMESPACE

namespace speedtree {

/**
 * TODO: to be documented.
 */
class TSpeedTreeType : public ReferenceCount {};
class BillboardOptimiser : public ReferenceCount {};

SpeedTreeRenderer::SpeedTreeRenderer() {}
SpeedTreeRenderer::~SpeedTreeRenderer() {}
void SpeedTreeRenderer::init() {}
void SpeedTreeRenderer::fini() {}
void SpeedTreeRenderer::tick( float ) {}
void SpeedTreeRenderer::update() {}
void SpeedTreeRenderer::beginFrame( EnviroMinder *, ShadowCaster * ) {}
void SpeedTreeRenderer::endFrame() {}
void SpeedTreeRenderer::draw( const Matrix &, uint32, float, float ) {}
void SpeedTreeRenderer::resetTransform( const Matrix& world, bool updateBB ) {}
const char * SpeedTreeRenderer::filename() const { return "Disabled"; }
uint SpeedTreeRenderer::seed() const { return 1; }
float SpeedTreeRenderer::lodMode(float) { return 0; }
float SpeedTreeRenderer::maxLod( float newValue ) { return 0; }
bool SpeedTreeRenderer::enviroMinderLighting(bool) { return false; }
bool SpeedTreeRenderer::drawTrees(bool) { return false; }

#ifdef EDITOR_ENABLED
void SpeedTreeRenderer::enableLightLines( bool drawLightLines ) {};
#endif // EDITOR_ENABLED

void SpeedTreeRenderer::load( const char * filename, uint seed, const Matrix&  world )
{
	throw std::runtime_error( "SpeedTree support is disabled" );
}

const BoundingBox & SpeedTreeRenderer::boundingBox() const
{
	return BoundingBox::s_insideOut_;
}

const BSPTree* SpeedTreeRenderer::bsp() const
{
	return NULL;
};

} // namespace speedtree

BW_END_NAMESPACE

#else  // !SPEEDTREE_SUPPORT ---------------------------------------------------

BW_BEGIN_NAMESPACE

namespace
{
    int     noLinkWarning   = 0;    // stop the compiler complaining
}

BW_END_NAMESPACE

#endif // !SPEEDTREE_SUPPORT ---------------------------------------------------
