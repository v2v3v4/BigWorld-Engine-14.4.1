#include "pch.hpp"

// Module Interface
#include "speedtree_thumb_prov.hpp"
#include "speedtree_renderer.hpp"

#if SPEEDTREE_SUPPORT // -------------------------------------------------------

#include "resmgr/bwresource.hpp"
#include "math/boundbox.hpp"
#include "moo/render_context.hpp"
#include "moo/effect_manager.hpp"
#include "moo/material.hpp"
#include "cstdmf/string_utils.hpp"
#include "moo/effect_visual_context.hpp"



DECLARE_DEBUG_COMPONENT2("SpeedTree", 0)


BW_BEGIN_NAMESPACE

int SpeedTreeThumbProv_token;

namespace speedtree {

// Thumbnail Provider
IMPLEMENT_THUMBNAIL_PROVIDER(SpeedTreeThumbProv)

/**
 *	Returns true if this is a SpeedTree (".spt") file.
 */
bool SpeedTreeThumbProv::isValid(const ThumbnailManager& manager, const BW::wstring& file)
{
	BW_GUARD;
	if (file.empty())
		return false;

	BW::string nfile;
	bw_wtoutf8(  file, nfile );
	BW::StringRef ext = BWResource::getExtension(nfile);
	return ext == "spt" || ext == "SPT";
}

/**
 *	Loads a speedtree for later rendering.
 */
bool SpeedTreeThumbProv::prepare(const ThumbnailManager& manager, const BW::wstring& file)
{
	BW_GUARD;
	bool result = false;
	try
	{
		BW::string nfile;
		bw_wtoutf8(  file, nfile );

		// load the tree
		tree_ = new SpeedTreeRenderer();
		BW::string resFile = BWResource::dissolveFilename(nfile);
		tree_->load(resFile .c_str(), 1, Matrix::identity);

		result = true;
	}
	catch (const std::runtime_error &err)
	{
		ERROR_MSG("Error loading tree: %s\n", err.what());
		if ( tree_ )
		{
			delete tree_;
			tree_ = 0;
		}
	}
	return result;
}

/**
 *	Creates the thumbnail image. Renders speedtree previously loaded in the
 *	prepare method into render target provided.
 */
bool SpeedTreeThumbProv::render( const ThumbnailManager& manager, const BW::wstring& file, Moo::RenderTarget* rt )
{
	BW_GUARD;
	if ( !tree_ )
		return false;

	bool result = false;
	try
	{
		/* Flush any events queued by prepare so they are available to 
		 * render the thumbnails
		 */
		Moo::rc().device()->Clear( 0, NULL,
			D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 
			RGB(192, 192, 192), 1, 0 );
		Moo::SunLight savedSun = Moo::rc().effectVisualContext().sunLight();

		// set view matrix
		Moo::rc().view(Matrix::identity);
		this->zoomToExtents(tree_->boundingBox(), 0.8f);

		// set world and 
		// projection matrices
		Matrix projection;
		projection.perspectiveProjection( DEG_TO_RAD(60.0f), 1.0f, 1.0f, 5000.0f);
		Moo::rc().device()->SetTransform( D3DTS_PROJECTION, &projection );
		Moo::rc().world(Matrix::identity);

		// force max lod level	
		float lodMode = SpeedTreeRenderer::lodMode(1.0f);

		// render the tree
		SpeedTreeRenderer::beginFrame( NULL, Moo::RENDERING_PASS_REFLECTION, Moo::rc().invView());
		tree_->draw(Matrix::identity);
		SpeedTreeRenderer::endFrame();

		//-- reset global rendering state.
		Moo::rc().effectVisualContext().sunLight(savedSun);
		Moo::rc().effectVisualContext().updateSharedConstants(Moo::CONSTANTS_PER_FRAME);
		
		// restore previous lod level
		SpeedTreeRenderer::lodMode(lodMode);	
		result = true;
	}
	catch (const std::runtime_error &err)
	{
		ERROR_MSG("Error loading tree: %s\n", err.what());
	}
	delete tree_;
	tree_ = 0;
	return result;
}

} // namespace speedtree

BW_END_NAMESPACE

#endif // SPEEDTREE_SUPPORT
