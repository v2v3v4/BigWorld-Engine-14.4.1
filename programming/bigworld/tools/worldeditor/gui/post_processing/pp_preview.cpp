#include "pch.hpp"
#include "phase_node_view.hpp"
#include "pp_preview.hpp"
#include "post_processing/manager.hpp"
#include "post_processing/effect.hpp"
#include "post_processing/phase.hpp"
#include "moo/render_context.hpp"
#include "moo/geometrics.hpp"
#include "romp/py_render_target.hpp"

DECLARE_DEBUG_COMPONENT2( "PostProcessing", 0 )

BW_BEGIN_NAMESPACE

// Python statics
PY_TYPEOBJECT( PPPreview )

PY_BEGIN_METHODS( PPPreview )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PPPreview )
PY_END_ATTRIBUTES()


const int PREVIEW_TEXTURE_SIZEX = 1024;
const int PREVIEW_TEXTURE_SIZEY = 512;
const float PREVIEW_TEXTURE_SIZEXF = (float)PREVIEW_TEXTURE_SIZEX;
const float PREVIEW_TEXTURE_SIZEYF = (float)PREVIEW_TEXTURE_SIZEY;


/*~ class PostProcessing.Preview
 *	@components{ worldeditor }
 *
 *	This class records each step of the entire PostProcessing chain into
 *	a render target, allowing you to see every single step.
 *
 *	This kind of debugging is required as the individual phases in the
 *	PostProcessing chain may (and should) be re-using render targets.
 */
/*~ function PostProcessing.Preview
 *	@components{ worldeditor }
 *
 *	Factory function to create and return a PostProcessing Preview object.
 *
 *	@return A new PostProcessing Preview object.
 */
PY_FACTORY_NAMED( PPPreview, "Preview", _PostProcessing )

PPPreview::PPPreview( PyTypeObject *pType ):
	PostProcessing::Debug( pType )	
{
	BW_GUARD;
	Moo::RenderTargetPtr pRT = new Moo::RenderTarget( "Post-processing preview" );
	pRT->create( PREVIEW_TEXTURE_SIZEX,PREVIEW_TEXTURE_SIZEY );
	previewRT_ = SmartPointer<PyRenderTarget>( new PyRenderTarget( pRT ), PyObjectPtr::STEAL_REFERENCE );
	this->pySet_renderTarget(  previewRT_.get() );
	pSystemTex_ = Moo::rc().createTexture(PREVIEW_TEXTURE_SIZEX,PREVIEW_TEXTURE_SIZEY,1,D3DUSAGE_DYNAMIC,D3DFMT_A8R8G8B8,D3DPOOL_SYSTEMMEM);
};


PPPreview::~PPPreview()
{
	BW_GUARD;

	pSystemTex_ = NULL;
}


/**
 *	This method returns whether or not the phase node is visible in the
 *	graph's client rectangle.  If not, then there is no need to draw
 *	the preview data into the HDC
 */
bool PPPreview::isVisible( PhaseNodeView* pnv ) const
{
	BW_GUARD;

	PostProcessing::Phase* phase = pnv->phase();
	return ( layoutMap_.find(phase) != layoutMap_.end() );
}


/**
 *	This method examines the chain and the current state of the PP editing window,
 *	and records an appropriate render target layout.
 */
void PPPreview::edLayout( RECT& clientRect, const CPoint& offset, BW::vector<PhaseNodeView*>& pnvVector )
{
	BW_GUARD;
	//Save previous results and associated layout map, before we clobber it
	copyResults();

	layoutMap_.clear();

	CRect cRect(clientRect);
	BW::vector<PhaseNodeView*> visibleNodes;

	for (size_t i=0; i<pnvVector.size(); i++)
	{
		PhaseNodeView* pnv = pnvVector[i];
		CRect r = pnv->rect();
		r.OffsetRect(offset);
		CRect intersection;
		intersection.IntersectRect(&cRect,&r);
		if ( !intersection.IsRectEmpty() )
		{
			visibleNodes.push_back(pnv);
		}
	}

	if ( !visibleNodes.size() )
	{
		return;
	}

	//the age old problem of allocating n even size rectangles into
	//a rectangular area
	float nNodes = (float)visibleNodes.size();
	int ny = (int)(floorf(sqrtf(nNodes)));
	int nx = (int)(ceilf(nNodes/ny));
	int w = PREVIEW_TEXTURE_SIZEX/nx;
	int h = PREVIEW_TEXTURE_SIZEY/ny;

	//limit the size of the previews to exactly match the size of
	//the phase nodes, if they are smaller than the available space.
	int rtx = 6;	//magic numbers - see PhaseNodeView::draw
	int rty = 2;
	int rtw = visibleNodes[0]->rect().Width() - rtx * 2;
	int rth = visibleNodes[0]->rect().Height() - rty * 2;
	w = min( w, rtw );
	h = min( h, rth );

	MF_ASSERT( visibleNodes.size() <= INT_MAX );
	for ( int i = 0; i < ( int ) visibleNodes.size(); i++)
	{
		int l = (i % nx) * w;
		int t = (i / nx) * h;
		layoutMap_.insert( std::make_pair(visibleNodes[i]->phase(),CRect(l,t,l+w,t+h)) );
	}
}


/**
 * This method retrieves the render target rectangle for the given phase.
 */
bool PPPreview::phaseRect( const PostProcessing::Phase* phase, CRect& out )
{
	BW_GUARD;

	if ( layoutMap_.empty() )
		return false;

	LayoutMap::iterator it = layoutMap_.find(phase);
	if ( it != layoutMap_.end() )
	{
		out = it->second;
		return true;
	}
	return false;
}


/**
 * This method retrieves the render target rectangle for the given phase,
 * for the time when the results were copied out.  This is an important
 * distinction, because the layout may change well before the render
 * target is updated.
 */
bool PPPreview::phaseRectForResults( const PostProcessing::Phase* phase, CRect& out )
{
	BW_GUARD;

	if ( resultsLayoutMap_.empty() )
		return false;

	LayoutMap::iterator it = resultsLayoutMap_.find(phase);
	if ( it != resultsLayoutMap_.end() )
	{
		out = it->second;
		return true;
	}
	return false;
}


void PPPreview::copyResults()
{
	BW_GUARD_PROFILER( PPPreview_copyResults );

	//Copy last frames data away
	ComObjectWrap<DX::Surface> pSrcSurface;
	ComObjectWrap<DX::Surface> pDstSurface;

	previewRT_->pRenderTarget()->pSurface( pSrcSurface );
	pSystemTex_->GetSurfaceLevel(0, &pDstSurface);

	HRESULT hr = Moo::rc().device()->GetRenderTargetData(
		pSrcSurface.pComObject(),pDstSurface.pComObject()
		);

	if ( FAILED(hr) )
	{
		ERROR_MSG( "Could not copy render target data %s\n", DX::errorAsString(hr).c_str() );
	}

	//Store the layout results to go with the render target copy.
	resultsLayoutMap_ = layoutMap_;
}


/**
 *	This method should be called to let the debug object know an entire
 *	chain is about to be drawn.  It resets some internal variables.
 *	@param	nEffects	number of effects in the chain.
 */
void PPPreview::beginChain( uint32 nEffects )
{
	BW_GUARD;

	PostProcessing::Debug::beginChain( nEffects );
}


/**
 *	This method should be called to let the debug object know an effect
 *	is about to draw.  It resets some internal variables.
 *	@param	nPhases	number of phases in the effect.
 */
void PPPreview::beginEffect( uint32 nPhases )
{
	BW_GUARD;

	PostProcessing::Debug::beginEffect( nPhases );
}


/**
 *	This method returns the texture coordinates for the given effect/phase
 *	pair, packed into a Vector4 as ( bl.x, bl.y, tr.x, tr.y ), or
 *	( l, t, r, b )
 *
 *	@param	effect	index of the effect within the PostProcessing chain
 *	@param	phase	index of the phase within the effect
 *	@param	nEffects number of effects
 *	@param	nPhases	number of phases within the desired effect
 *
 *	@return	Vector4	packed uv coordinates of the phase
 */
Vector4 PPPreview::phaseUV( uint32 effect, uint32 phase, uint32 nEffects, uint32 nPhases )
{
	BW_GUARD;
	const PostProcessing::Phase* p = PostProcessing::Manager::instance().effects()[effect]->phases()[phase].get();
	CRect rect;
	if (phaseRect( p, rect ))
	{
		return Vector4((float)rect.left / PREVIEW_TEXTURE_SIZEXF,
			1.0f - (float)rect.bottom / PREVIEW_TEXTURE_SIZEYF,
			(float)rect.right / PREVIEW_TEXTURE_SIZEXF,
			1.0f - (float)rect.top / PREVIEW_TEXTURE_SIZEYF);
	}
	else
	{
		return Vector4::zero();
	}
}


/**
 *	This method advances to the next phase in the output chain.
 *
 *	It returns the viewport rectangle that a phase containing
 *	its own method for previewing should draw into.
 */
RECT PPPreview::nextPhase()
{
	BW_GUARD;

	return PostProcessing::Debug::nextPhase();
}


PyObject* PPPreview::pyNew(PyObject* args)
{
	BW_GUARD;

	return new PPPreview;
}
BW_END_NAMESPACE

