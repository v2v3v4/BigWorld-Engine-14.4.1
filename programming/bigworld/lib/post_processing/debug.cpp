#include "pch.hpp"
#include "debug.hpp"
#include "moo/render_context.hpp"
#include "moo/geometrics.hpp"
#include "romp/py_render_target.hpp"

#ifndef CODE_INLINE
#include "debug.ipp"
#endif

DECLARE_DEBUG_COMPONENT2( "PostProcessing", 0 )


BW_BEGIN_NAMESPACE

namespace PostProcessing
{
// Python statics
PY_TYPEOBJECT( Debug )

PY_BEGIN_METHODS( Debug )
	/*~ function Debug.phaseUV
	 *	@components{ client, tools }
	 *
	 *	This method returns the texture coordinates for the given effect/phase
	 *	pair, packed into a Vector4 as ( bl.x, bl.y, tr.x, tr.y ), or
	 *	 l, b, r, t )
	 *
	 *	@param	effect	index of the effect within the PostProcessing chain
	 *	@param	phase	index of the phase within the effect
	 *	@param	nEffects number of effects
	 *	@param	nPhases	number of phases within the desired effect
	 *
	 *	@return	Vector4	packed uv coordinates of the phase
	 */
	PY_METHOD( phaseUV )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( Debug )
	/*~ attribute Debug.renderTarget
	 *	@components{ client, tools }
	 *	
	 *	The Debug object requires a render target to do its work.  With this
	 *	attribute you can provide the Debug object with a render target of
	 *	whatever size and format you like.
	 *
	 *	@type PyRenderTarget.
	 */
	PY_ATTRIBUTE( renderTarget )
PY_END_ATTRIBUTES()


/*~ class PostProcessing.Debug
 *	@components{ client, tools }
 *	This class records each step of the entire PostProcessing chain into
 *	a render target, allowing you to see every single step.
 *
 *	This kind of debugging is required as the individual phases in the
 *	PostProcessing chain may (and should) be re-using render targets.
 */
/*~ function PostProcessing.Debug
 *	@components{ client, tools }
 *	Factory function to create and return a PostProcessing Debug object.
 *
 *	@return A new PostProcessing Debug object.
 */
PY_FACTORY_NAMED( Debug, "Debug", _PostProcessing )

Debug::Debug( PyTypeObject *pType ):
		PyObjectPlus( pType ),
		enabled_(true),
		nEffects_(0),
		nPhases_(0),
		effect_(0),
		phase_(0)
{
};


/**
 *	This method should be called to let the debug object know an entire
 *	chain is about to be drawn.  It resets some internal variables.
 *	@param	nEffects	number of effects in the chain.
 */
void Debug::beginChain( uint32 nEffects )
{
	nEffects_ = (int32)nEffects;
	effect_ = -1;
}


/**
 *	This method should be called to let the debug object know an effect
 *	is about to draw.  It resets some internal variables.
 *	@param	nPhases	number of phases in the effect.
 */
void Debug::beginEffect( uint32 nPhases )
{
	nPhases_ = (int32)nPhases;
	phase_ = 0;
	effect_++;
}


/**
 *	This method draws a grey box where a phase would be recorded.  It indicates
 *	the phase has been temporarily disabled. 
 */
void Debug::drawDisabledPhase()
{
	if ( pRT_.hasObject() && pRT_->pRenderTarget().hasObject() )
	{
		RECT r = this->nextPhase();

		if (pRT_->pRenderTarget()->push())
		{
			Geometrics::drawRect( Vector2((float)r.left, (float)r.top), Vector2((float)r.right, (float)r.bottom), 0x80808080 );
			pRT_->pRenderTarget()->pop();
		}		
	}
}


/**
 *	This method advances to the next phase in the output chain.
 *
 *	It returns the viewport rectangle that a phase containing
 *	its own method for previewing should draw into.
 *
 *	Internally it increments the phase index, meaning for a phase you
 *	should call nextPhase once and once onlye.
 */
RECT Debug::nextPhase()
{
	float rtWidth = (float)pRT_->pRenderTarget()->width();
	float rtHeight = (float)pRT_->pRenderTarget()->height();
	Vector4 uvs = this->phaseUV( effect_, phase_, nEffects_, nPhases_ );
	RECT r;
	r.left = (uint32)(uvs[0] * rtWidth);
	r.top = (uint32)(rtHeight - uvs[3] * rtHeight);
	r.right = (uint32)(uvs[2] * rtWidth);
	r.bottom = (uint32)(rtHeight - uvs[1] * rtHeight);
	phase_++;
	return r;
}


/**
 *	This method takes a snapshot of the supplied render target and copies
 *	it in our internally held render target, in some spot calculated by
 *	the effect/phase id we are currently in.
 *
 *	@param	pRT		Render target containing the results of the latest phase.
 */
void Debug::copyPhaseOutput( Moo::RenderTargetPtr pRT )
{
	if ( !pRT_ )
	{
		return;
	}

	if (effect_ == 0 && phase_ == 0 && pRT_->pRenderTarget()->push())
	{
		Moo::rc().device()->Clear( 0, NULL, D3DCLEAR_TARGET, 0x00000000, 1, 0 );
		pRT_->pRenderTarget()->pop();
	}

	RECT r = this->nextPhase();

	if (r.right-r.left <= 0 || r.bottom-r.top <= 0)
	{
		return;
	}

	ComObjectWrap<DX::Surface> src;
	ComObjectWrap<DX::Surface> dst;
	pRT_->pRenderTarget()->pSurface(dst);
	if (pRT.hasObject())
	{
		pRT->pSurface(src);
	}
	else
	{
		src = Moo::rc().getRenderTarget(0);
	}

	Moo::rc().device()->StretchRect( src.pComObject(), NULL, dst.pComObject(), &r, D3DTEXF_LINEAR );
}


/**
 *	This method returns the texture coordinates for the given effect/phase
 *	pair, packed into a Vector4 as ( bl.x, bl.y, tr.x, tr.y ), or
 *	( l, b, r, t )
 *
 *	@param	effect	index of the effect within the PostProcessing chain
 *	@param	phase	index of the phase within the effect
 *	@param	nEffects number of effects
 *	@param	nPhases	number of phases within the desired effect
 *
 *	@return	Vector4	packed uv coordinates of the phase
 */
Vector4 Debug::phaseUV( uint32 effect, uint32 phase, uint32 nEffects, uint32 nPhases )
{
	//u,v how many effects to draw in the x,y directions
	float effectU = ceilf(sqrtf((float)nEffects));
	float effectV = ceilf((float)nEffects/effectU);
	//effect grid size
	float effectW = 1.f / effectU;
	float effectH = 1.f / effectV;
	//effect location
	float effectX = fmodf((float)effect,effectU) * effectW;
	float effectY = floorf(effect/effectU) * effectH;

	//u,v how many phases to draw in the x,y directions within
	//our effect region
	float phaseU = ceilf(sqrtf((float)nPhases));
	float phaseV = ceilf((float)nPhases/phaseU);
	//phase grid size
	float phaseW = effectW / phaseU;
	float phaseH = effectH / phaseV;
	//effect location
	float phaseX = fmodf((float)phase, phaseU) * phaseW + effectX;
	float phaseY = floorf(phase/phaseU) * phaseH + effectY;

	return Vector4(phaseX, phaseY, phaseX + phaseW, phaseY + phaseH);
}


PyRenderTargetPtr Debug::pRT() const
{
	return pRT_;
}


PyObject* Debug::pyNew(PyObject* args)
{
	return new Debug;
}
}	//namespace PostProcessing

BW_END_NAMESPACE

// debug.cpp
