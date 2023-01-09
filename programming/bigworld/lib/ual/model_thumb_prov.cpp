/**
 *  Model Thumbnail Provider
 */

#include "pch.hpp"
#include "cstdmf/bw_string.hpp"

#include "moo/render_context.hpp"
#include "moo/effect_manager.hpp"
#include "moo/light_container.hpp"
#include "moo/visual_manager.hpp"
#include "moo/visual.hpp"
#include "moo/draw_context.hpp"
#include "moo/renderer.hpp"
#include "moo/effect_visual_context.hpp"

#include "math/boundbox.hpp"
#include "math/matrix.hpp"

#include "model_thumb_prov.hpp"

#include "common/string_utils.hpp"

#include "cstdmf/string_utils.hpp"

DECLARE_DEBUG_COMPONENT( 0 )

BW_BEGIN_NAMESPACE

// Token so this file gets linked in by the linker
int ModelThumbProv_token;

// Implement the Model Thumbnail Provider Factory
IMPLEMENT_THUMBNAIL_PROVIDER( ModelThumbProv )


/**
 *  Constuctor.
 */
ModelThumbProv::ModelThumbProv():
    lights_(NULL),
    visual_(NULL)
{
    BW_GUARD;
}


/**
 *  Destuctor.
 */
ModelThumbProv::~ModelThumbProv()
{
    BW_GUARD;

    lights_ = NULL;
}


/**
 *  This method tells the manager if the asset specified in 'file' can be
 *  handled by the ModelThumbProv, i.e. if it's a .visual or a .model file.
 *
 *  @param manager  ThumbnailManager instance that is dealing with the asset.
 *  @param file     Asset requesting the thumbnail
 *  @return         True if the asset is a model or visual file.
 */
bool ModelThumbProv::isValid( const ThumbnailManager& manager, const BW::wstring& file )
{
    BW_GUARD;

    if ( file.empty() )
        return false;

    BW::wstring ext = file.substr( file.find_last_of( L'.' ) + 1 );
    return wcsicmp( ext.c_str(), L"model" ) == 0
        || wcsicmp( ext.c_str(), L"visual" ) == 0;
}


/**
 *  This method is called to find out if the image for the asset needs to be
 *  created in the background thread.  For models and visuals, it only needs
 *  to be created if the thumbnail image file for it doesn't exist.
 *
 *  @param manager  ThumbnailManager instance that is dealing with the asset.
 *  @param file     Asset requesting the thumbnail.
 *  @param thumb    Returns the path to the thumbnail img file for this asset.
 *  @param size     Returns the desired size of this image, 128 for models.
 *  @return         True if there's no thumbnail image created for the model.
 */
bool ModelThumbProv::needsCreate( const ThumbnailManager& manager, const BW::wstring& file, BW::wstring& thumb, int& size )
{
    BW_GUARD;

    size = 128; // Models want 128 x 128 thumbnails
    
    BW::string nfile;
    bw_wtoutf8( file, nfile );

    BW::StringRef basename = BWResource::removeExtension( nfile );
    if ( BWResource::getExtension( basename ) == "static" )
    {
        // it's a visual with two extensions, so remove the remaining extension
        basename = BWResource::removeExtension( basename );
    }

    BW::string thumbName = basename + ".thumbnail.jpg";

    bw_utf8tow( thumbName, thumb );

    if ( PathFileExists( thumb.c_str() ) )
    {
        return false;
    }
    else
    {
        thumbName = basename + ".thumbnail.bmp";

        BW::wstring wthumb;
        bw_utf8tow( thumbName, wthumb );
        if ( PathFileExists( wthumb.c_str() ) )
        {
            // change the thumbnail path to the legacy one
            thumb = wthumb;
            return false;
        }
    }
    return true;
}



/**
 *  This method is called from the bg thread to prepare the thumbnail, so here
 *  we just load the model and get it ready for rendering.
 *
 *  @param manager  ThumbnailManager instance that is dealing with the asset.
 *  @param file     Model file asset requesting the thumbnail.
 *  @return         True if the model was loaded correctly, false otherwise.
 */
bool ModelThumbProv::prepare( const ThumbnailManager& manager, const BW::wstring& file )
{
    BW_GUARD;

    BW::wstring visualName = file;
    BW::string nfile;
    bw_wtoutf8( file, nfile );
    BW::string modelName = BWResource::dissolveFilename( nfile );
    bool errored = (errorModels_.find( modelName ) != errorModels_.end());

    if ( BWResource::getExtension( nfile ) != "visual" )
    {
        DataSectionPtr model = BWResource::openSection( modelName, false );

        if (model == NULL) 
        {
            if (errored) return false;
            ERROR_MSG( "ModelThumbProv::create: Could not open model file"
                " \"%s\"\n", modelName.c_str() );
            errorModels_.insert( modelName );
            return false;
        }

        static const char * visuals [] = { "nodefullVisual", "nodelessVisual", "billboardVisual" };
        for ( int idx = 0 ; idx < ARRAY_SIZE( visuals ) ; ++idx )
        {
            visualName = model->readWideString( visuals[ idx ] );
            if ( visualName != L"" )
                break;
        }
        if ( visualName == L"" )
        {
            if (errored) return false;
            ERROR_MSG( "ModelThumbProv::create: Could not determine type of model"
                " in file \"%s\"\n", modelName.c_str() );
            errorModels_.insert( modelName );
            return false;
        }
        visualName += L".visual";
    }

    BW::string nvisualName;
    bw_wtoutf8( visualName, nvisualName );
    visual_ = Moo::VisualManager::instance()->get( nvisualName );
    if (visual_ == NULL)
    {
        if (errored) 
            return false;
        ERROR_MSG( "ModelThumbProv::create: Couldn't load visual \"%s\"\n", nvisualName.c_str() );
        errorModels_.insert( modelName );
        return false;
    }

    return true;
}


/**
 *  This method is called from the main thread to render the prepared model
 *  into the passed-in render target of the thumbnail manager.
 *
 *  @param manager  ThumbnailManager instance that is dealing with the asset.
 *  @param file     Model file asset requesting the thumbnail.
 *  @param rt       Render target to render the prepared model to.
 *  @return         True if the thumbnail was rendered, false otherwise.
 */
bool ModelThumbProv::render( const ThumbnailManager& manager,
                            const BW::wstring& file,
                            Moo::RenderTarget* rt )
{
    // WARNING: This functionality is duplicated inside 
    // MeModule::renderThumbnail(). There are already discrepancies.

    BW_GUARD;

    if ( !visual_ )
        return false;

    Matrix rotation;
    
    //-- prepare lighting.
    Moo::SunLight savedSun = Moo::rc().effectVisualContext().sunLight();
    Moo::SunLight sun = savedSun;
    
    Moo::rc().effectVisualContext().sunLight(sun);
    Moo::rc().effectVisualContext().updateSharedConstants(Moo::CONSTANTS_PER_FRAME);
    
    //Make sure we set this before we try to draw
    Moo::rc().setRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );

    Moo::rc().device()->Clear( 0, NULL,
        D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, RGB( 192, 192, 192 ), 1, 0 );

    // Set the projection matrix
    Moo::Camera cam = Moo::rc().camera();
    cam.aspectRatio( 1.f );
    Moo::rc().camera( cam );
    Moo::rc().updateProjectionMatrix();

    // camera should look along +Z axis
    //// Set a standard view
    //Matrix view (Matrix::identity);
    //Moo::rc().world( view );
    //rotation.setRotateX( - MATH_PI / 8.f );
    //view.preMultiply( rotation );
    //rotation.setRotateY( + MATH_PI / 8.f );
    //view.preMultiply( rotation );
    //Moo::rc().view( view );

    // Zoom to the models bounding box
    zoomToExtents( visual_->boundingBox() );

    Moo::rc().effectVisualContext().updateSharedConstants(Moo::CONSTANTS_PER_VIEW);

    Moo::DrawContext thumbnailDrawContext( 
        Renderer::instance().pipelineType() == IRendererPipeline::TYPE_DEFERRED_SHADING ?
        Moo::RENDERING_PASS_REFLECTION: Moo::RENDERING_PASS_COLOR ) ;

    thumbnailDrawContext.begin( Moo::DrawContext::ALL_CHANNELS_MASK );
    // Draw the model
    visual_->draw( thumbnailDrawContext );
    thumbnailDrawContext.end( Moo::DrawContext::ALL_CHANNELS_MASK );
    thumbnailDrawContext.flush( Moo::DrawContext::ALL_CHANNELS_MASK );

    //Make sure we restore this after we are done
    Moo::rc().setRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );

    //-- reset global rendering state.
    Moo::rc().effectVisualContext().sunLight(savedSun);
    Moo::rc().effectVisualContext().updateSharedConstants(Moo::CONSTANTS_PER_FRAME);

    visual_ = NULL;

    return true;
}

BW_END_NAMESPACE

