#include "pch.hpp"
#include "gl_view.hpp"
#include "i_model_editor_app.hpp"

#include "moo/render_context.hpp"

#include "tools/common/cooperative_moo.hpp"

BW_BEGIN_NAMESPACE

//==============================================================================
GLView::GLView()
	: editorApp_( NULL )
{
}


//==============================================================================
void GLView::setEditorApp( IModelEditorApp * editorApp )
{
	editorApp_ = editorApp;
}


//==============================================================================
bool GLView::paint( bool resize )
{
	if (editorApp_->initDone() == false)
	{
		return false;
	}
	if (CooperativeMoo::beginOnPaint( editorApp_ ) == false)
	{		
		return false;
	}
	if (resize)
	{
		resizeWindow();
	}

	editorApp_->mfApp()->updateFrame( false );

	CooperativeMoo::endOnPaint( editorApp_ );
	return true;
}

//==============================================================================
void GLView::resizeWindow()
{
	Moo::rc().changeMode(Moo::rc().modeIndex(), Moo::rc().windowed(), true);
}

BW_END_NAMESPACE

