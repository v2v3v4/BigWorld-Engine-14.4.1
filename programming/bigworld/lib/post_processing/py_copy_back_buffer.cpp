#include "pch.hpp"
#include "py_copy_back_buffer.hpp"
#include "manager.hpp"
#include "debug.hpp"
#include "moo/render_context.hpp"


#ifdef EDITOR_ENABLED
#include "gizmo/general_editor.hpp"
#include "gizmo/general_properties.hpp"
#include "resmgr/string_provider.hpp"
#endif //EDITOR_ENABLED

DECLARE_DEBUG_COMPONENT2( "PostProcessing", 0 )


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "py_copy_back_buffer.ipp"
#endif

namespace PostProcessing
{

uint32 PyCopyBackBuffer::s_drawCookie_ = 0;

// Python statics
PY_TYPEOBJECT( PyCopyBackBuffer )

PY_BEGIN_METHODS( PyCopyBackBuffer )	
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PyCopyBackBuffer )
	/*~ attribute PyCopyBackBuffer.renderTarget
	 *	@components{ client, tools }
	 *
	 *	The render target in which a copy of the back buffer will be placed.
	 *	Internally the DirectX function StretchRect is used, so please refer
	 *	to the DirectX documentation for information on restrictions regarding
	 *	pairs of back buffer formats and render target formats.
	 *
	 *	@type PyRenderTarget.
	 */
	PY_ATTRIBUTE( renderTarget )
	/*~ attribute PyCopyBackBuffer.name
	 *	@components{ client, tools }
	 *
	 *  Arbitrary name identifying this phase.
	 *
	 *	@type String.
	 */
	PY_ATTRIBUTE( name )
PY_END_ATTRIBUTES()

/*~ class PostProcessing.PyCopyBackBuffer
 *	@components{ client, tools }
 *	This class derives from PostProcessing::Phase, and
 *	provides a way to get a copy of the back buffer in
 *	a render target.
 *
 *	It uses a draw cookie to know when the back buffer
 *	has been updated.  If backBufferCopy.draw is called
 *	but the render target already has the latest copy
 *	of the render target then it optimises out the draw call.
 *	Note this optimisation only works if you reuse the same
 *	PyCopyBackBuffer phase instance.
 */
/*~ function PostProcessing.CopyBackBuffer
 *	@components{ client, tools }
 *	Factory function to create and return a PostProcessing PyCopyBackBuffer object.
 *	@return A new PostProcessing PyCopyBackBuffer object.
 */
PY_FACTORY_NAMED( PyCopyBackBuffer, "CopyBackBuffer", _PostProcessing )

IMPLEMENT_PHASE( PyCopyBackBuffer, PyCopyBackBuffer )

PyCopyBackBuffer::PyCopyBackBuffer( PyTypeObject *pType ):
	Phase( pType ),
	name_( "copy back buffer" ),
	drawCookie_( s_drawCookie_ - 1 )
{
}


PyCopyBackBuffer::~PyCopyBackBuffer()
{
}


void PyCopyBackBuffer::tick( float dTime )
{
	//This is a convenient spot to increment the static draw cookie.
	//Ideally, the application would do this, whenever it has updated
	//the back buffer (by drawing a frame of the world).  However
	//doing this here in tick should be just as good.
	incrementDrawCookie();
}


/**
 *	This method draws the PyCopyBackBuffer effect, by taking a copy
 *	of the currently set render target and stretchRecting it into
 *	our destination render target.
 *	@param	debug	optional debug object to record the output of this phase.
 *	@param	rect	optional debug rectangle in which to draw.
 */
void PyCopyBackBuffer::draw( Debug* debug, RECT* rect )
{
	ComObjectWrap<DX::Surface> pBB = Moo::rc().getRenderTarget(0);

	if (this->isDirty())
	{
		if (pBB)
		{
			if (pRenderTarget_.hasObject())
			{
				ComObjectWrap<DX::Surface> pDest;
				if (SUCCEEDED(pRenderTarget_->pRenderTarget()->pSurface( pDest )))
				{
					HRESULT hr = Moo::rc().device()->StretchRect(
						pBB.pComObject(), NULL, pDest.pComObject(), NULL, D3DTEXF_LINEAR );
				}
			}
		}
		else
		{
			ERROR_MSG( "PyCopyBackBuffer - could not get the back buffer surface\n" );
		}
	}

	if ( debug )
	{
		if (this->isDirty())
		{
			if (pRenderTarget_.hasObject())
				debug->copyPhaseOutput( pRenderTarget_->pRenderTarget() );
			else
				debug->copyPhaseOutput( NULL );
		}
		else
		{
			debug->drawDisabledPhase();
		}
	}

	drawCookie_ = s_drawCookie_;
}


bool PyCopyBackBuffer::load( DataSectionPtr pSect )
{
	name_ = pSect->readString( "name", name_ );
	BW::string rtName = pSect->readString( "renderTarget" );
	if (!rtName.empty())
	{
		renderTargetFromString( rtName );
	}
	return true;
}


bool PyCopyBackBuffer::save( DataSectionPtr pDS )
{
	DataSectionPtr pSect = pDS->newSection( "PyCopyBackBuffer" );
	pSect->writeString( "name", name_ );
	if (pRenderTarget_.hasObject())
	{
		pSect->writeString( "renderTarget", pRenderTarget_->pRenderTarget()->resourceID() );
	}
	return true;
}


bool PyCopyBackBuffer::isDirty() const
{
	return s_drawCookie_ != drawCookie_;
}


void PyCopyBackBuffer::incrementDrawCookie()
{
	s_drawCookie_++;
}


uint32 PyCopyBackBuffer::drawCookie()
{
	return s_drawCookie_;
}


#ifdef EDITOR_ENABLED


void PyCopyBackBuffer::edChangeCallback( PhaseChangeCallback pCallback )
{
	pCallback_ = pCallback;
}


void PyCopyBackBuffer::edEdit( GeneralEditor * editor )
{
	STATIC_LOCALISE_NAME( s_renderTarget, "WORLDEDITOR/GUI/PAGE_POST_PROCESSING/PY_COPY_BACK_BUFFER/RENDER_TARGET" );
	STATIC_LOCALISE_NAME( s_renderTargetDesc, "WORLDEDITOR/GUI/PAGE_POST_PROCESSING/PY_COPY_BACK_BUFFER/RENDER_TARGET_DESC" );

	TextProperty * outputRT = new TextProperty( s_renderTarget,
			new GetterSetterProxy< PyCopyBackBuffer, StringProxy >(
			this, "outputRenderTarget",
			&PyCopyBackBuffer::getOutputRenderTarget, 
			&PyCopyBackBuffer::setOutputRenderTarget ) );

	outputRT->fileFilter( Name( "Render Targets (*.rt)|*.rt|Render Targets (*.rt)|*.rt||" ) );

	outputRT->UIDesc( s_renderTargetDesc );

	editor->addProperty( outputRT );
}


BW::string PyCopyBackBuffer::getOutputRenderTarget() const
{
	BW::string ret = "backBuffer";

	if (pRenderTarget_ && pRenderTarget_->pRenderTarget())
	{
		ret = pRenderTarget_->pRenderTarget()->resourceID();
	}

	return ret;
}


bool PyCopyBackBuffer::setOutputRenderTarget( const BW::string & rt )
{
	BW::string rtName = rt;

	// We need this postfix so the properties list understands this is a texture.
	BW::string renderTargetPostfix = ".rt";
	BW::string::size_type postfixPos = rtName.rfind( renderTargetPostfix );

	if (postfixPos != BW::string::npos)
	{
		// It's a render target, remove the postfix so we get the name right.
		rtName = rtName.substr( 0, postfixPos );
	}

	if (pCallback_)
	{
		(*pCallback_)( false );
	}

	return renderTargetFromString( rtName );
}


#endif // EDITOR_ENABLED

PyObject* PyCopyBackBuffer::pyNew(PyObject* args)
{
	return new PyCopyBackBuffer;
}


bool PyCopyBackBuffer::renderTargetFromString( const BW::string & resourceID )
{
	bool ok = false;

	if (!resourceID.empty())
	{
		Moo::BaseTexturePtr pTex = Moo::TextureManager::instance()->get( resourceID );
		if (pTex.hasObject())
		{
			Moo::RenderTarget* pRT = dynamic_cast<Moo::RenderTarget*>( pTex.getObject() );
			if ( pRT )
			{
				pRenderTarget_ = PyRenderTargetPtr( new PyRenderTarget(pRT), true );
				ok = true;
			}
			else
			{
				pRenderTarget_ = NULL;
				ok = false;
			}
		}
	}
	else
	{
		pRenderTarget_ = NULL;
		ok = true;
	}

	return ok;
}

} //namespace PostProcessing

BW_END_NAMESPACE

// py_copy_back_buffer.cpp
