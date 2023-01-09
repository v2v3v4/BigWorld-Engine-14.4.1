#include "pch.hpp"

#include "super_model_animation.hpp"

#include "model.hpp"
#include "super_model.hpp"

DECLARE_DEBUG_COMPONENT2( "Model", 0 )


BW_BEGIN_NAMESPACE

 /**
 * if our pSuperModel_ has been reloaded, 
 * update the related data.
 */
void SuperModelAnimation::onReloaderReloaded( Reloader* pReloader )
{
	BW_GUARD;
	if (pReloader && pSuperModel_->hasModel( (Model*)pReloader), true )
	{
		this->pullInfoFromSuperModel( *pSuperModel_ );
		//reset
		time = 0.f;
		lastTime = 0.f;
	}
}
 /**
 *	This method pull the info from our pSuperModel_
 */
void SuperModelAnimation::pullInfoFromSuperModel( SuperModel & superModel )
{
	BW_GUARD;
	SuperModel::ReadGuard guard( &superModel );

	for (int i = 0; i < superModel.nModels(); i++)
	{
		index[i] = superModel.topModel(i)->getAnimation( actionName_ );
	}
}

/**
 *	SuperModelAnimation constructor
 */
SuperModelAnimation::SuperModelAnimation( SuperModel & superModel,
		const BW::string & name ) :
	time( 0.f ),
	lastTime( 0.f ),
	blendRatio( 0.f ),
	pSuperModel_( &superModel ),
	actionName_( name )
{
	BW_GUARD;

	// Mutually exclusive of pSuperModel_ reloading, start
	pSuperModel_->beginRead();

	this->pullInfoFromSuperModel( superModel );
	pSuperModel_->registerModelListener( this );

	// Mutually exclusive of pSuperModel_ reloading, end
	pSuperModel_->endRead();
}


/**
 *	This method ticks the animations represented by this class in
 *	the input supermodel. It must be the same as it was created with.
 *	Only the top model's animations are ticked.
 *	This is a helper method, since ticks are discouraged in SuperModel code.
 */
void SuperModelAnimation::tick( SuperModel & superModel, float dtime )
{
	BW_GUARD;
	for (int i = 0; i < superModel.nModels(); i++)
	{
		Model * cM = &*superModel.topModel(i);
		if (cM != NULL) cM->tickAnimation( index[i], dtime, lastTime, time );
	}

	lastTime = time;
}


/**
 *	This method applies the animation represented by this class to
 *	the input supermodel. It must be the same as it was created with.
 */
void SuperModelAnimation::dress( SuperModel & superModel, Matrix& world )
{
	BW_GUARD;
	for (int i = 0; i < superModel.nModels(); i++)
	{
		Model * cM = superModel.curModel(i);
		if (cM != NULL && index[i] != -1)
		{
			cM->playAnimation( index[i], time, blendRatio, 0 );
		}
	}
}


/**
 *	This method returns the given animation, for the top lod of the first
 *	model it exists in.
 *
 *	@return the animation, or NULL if none exists
 */
/*const*/ ModelAnimation * SuperModelAnimation::pSource( SuperModel & superModel ) const
{
	BW_GUARD;
	for (int i = 0; i < superModel.nModels(); i++)
	{
		if (index[i] != -1)
		{
			return superModel.topModel(i)->lookupLocalAnimation( index[i] );
		}
	}

	return NULL;	// no models have this animation
}

BW_END_NAMESPACE

// super_model_animation.cpp
