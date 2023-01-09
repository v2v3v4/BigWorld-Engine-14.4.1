#include "pch.hpp"

#include "super_model_action.hpp"

#include "model.hpp"
#include "model_action.hpp"
#include "super_model.hpp"


DECLARE_DEBUG_COMPONENT2( "Model", 0 )


BW_BEGIN_NAMESPACE

 /**
 * if our pSuperModel_ has been reloaded, 
 * update the related data.
 */
void SuperModelAction::onReloaderReloaded( Reloader* pReloader )
{
	BW_GUARD;
	if (pReloader && pSuperModel_->hasModel( (Model*)pReloader, true ))
	{
		this->pullInfoFromSuperModel();
		//reset
		passed_ = 0.f;
		played_ = 0.f;
		finish_ = -1.f;
		lastPlayed_ = 0.f;
	}
}
 /**
 *	This method pull the info from our pSuperModel_
 */
void SuperModelAction::pullInfoFromSuperModel()
{
	BW_GUARD;
	SuperModel::ReadGuard guard( pSuperModel_ );	
	Model * pM = pSuperModel_->topModel(0).getObject();

	const ModelAction * pTop = pSource_ = pM->getAction( actionName_ );
	index[0] = pTop != NULL ? pM->getAnimation( pTop->animation_ ) : -1;
	int lui = 0;

	for (int i = 1; i < pSuperModel_->nModels(); i++)
	{
		pM = pSuperModel_->topModel(i).getObject();

		const ModelAction * pOwn = pM->getAction( actionName_ );
		index[i] = pOwn != NULL ? pM->getAnimation( pOwn->animation_ ) :
			pTop != NULL ? pM->getAnimation( pTop->animation_ ) : -1;

		if (pSource_ == NULL)
		{
			pSource_ = pOwn;
			lui = i;
		}
	}

	// get the animation pointer here too
	pM = pSuperModel_->topModel(lui).getObject();
	pFirstAnim_ = (index[lui] != -1) ?
		pM->lookupLocalAnimation( index[lui] ) :
	pM->lookupLocalDefaultAnimation();
}
/**
 *	SuperModelAction constructor
 */
SuperModelAction::SuperModelAction( SuperModel & superModel,
		const BW::string & name ) :
	passed_( 0.f ),
	played_( 0.f ),
	finish_( -1.f ),
	lastPlayed_( 0.f ),
	pSource_( NULL ),
	pFirstAnim_( NULL ),
	pSuperModel_( &superModel ),
	actionName_( name )
{
	BW_GUARD;
	// Mutually exclusive of pVisual_ reloading, start
	pSuperModel_->beginRead();

	this->pullInfoFromSuperModel();

	pSuperModel_->registerModelListener( this );

	// Mutually exclusive of pVisual_ reloading, end
	pSuperModel_->endRead();
}


/**
 *	This method ticks the animations referenced by this action in
 *	the input supermodel. It must be the same as it was created with.
 *	Only the top model's animations are ticked.
 *	This is a helper method, since ticks are discouraged in SuperModel code.
 */
void SuperModelAction::tick( SuperModel & superModel, float dtime )
{
	BW_GUARD;
	// tick corresponding animations
	for (int i = 0; i < superModel.nModels(); i++)
	{
		Model * cM = &*superModel.topModel(i);
		if (cM != NULL && index[i] != -1)
			cM->tickAnimation( index[i], dtime, lastPlayed_, played_ );
	}

	// remember when we last ticked them
	lastPlayed_ = played_;
}


/**
 *	This method applies the animations referenced by this action to
 *	the input supermodel. It must be the same as it was created with.
 */
void SuperModelAction::dress( SuperModel & superModel, Matrix& world )
{
	BW_GUARD;
	// cache blend ratio
	const float br = this->blendRatio();

	// configure flags
	int flags = pSource_->flagSum_;

	// and play corresponding animations
 	for (int i = 0; i < superModel.nModels(); i++)
	{
		Model * cM = superModel.curModel(i);
		if (cM != NULL && index[i] != -1)
		{
			cM->playAnimation( index[i], played_, br, flags );
		}
	}
}


/**
 *	Calculate the blend ratio for this action
 */
float SuperModelAction::blendRatio() const
{
	BW_GUARD;
	float fsum = min( (passed_) / pSource_->blendInTime_,
		finish_ > 0.f ? (finish_ - passed_) / pSource_->blendOutTime_ : 1.f );
	return fsum >= 1.f ? 1.f : fsum > 0.f ? fsum : 0.f;
}

BW_END_NAMESPACE

// super_model_action.cpp
