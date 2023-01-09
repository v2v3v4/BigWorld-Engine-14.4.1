#ifdef _MSC_VER 
#pragma once
#endif

#ifndef SUPER_MODEL_ANIMATION_HPP
#define SUPER_MODEL_ANIMATION_HPP


#include "fashion.hpp"

#include "moo/reload.hpp"


BW_BEGIN_NAMESPACE

class Animation;
class SuperModel;

/**
 *	This class represents an animation from a SuperModel perspective.
 *
 *	@note This is a variable-length class, so don't try to make these
 *		at home, kids.
 */
class SuperModelAnimation : 
	public TransformFashion,
	/*
	*	Note: SuperModelAnimation is always listening if it's superModel_ has 
	*	been reloaded, if you are pulling info out from it
	*	then please update it again in onReloaderReloaded which is 
	*	called when the superModel_ is reloaded so that related data
	*	will need be update again.and you might want to do something
	*	in onReloaderPriorReloaded which happen right before the superModel_ reloaded.
	*/
	public ReloadListener
{
public:
	float	time;
	float	lastTime;
	float	blendRatio;

	/*const*/ ModelAnimation * pSource( SuperModel & superModel ) const;

	void tick( SuperModel & superModel, float dtime );
	virtual void onReloaderReloaded( Reloader* pReloader );

	/**
	 *	SuperModelAnimation does not require models to re-calculate their
	 *	transform state after this fashion is applied.
	 *	@return false
	 */
	bool needsRedress() const { return false; }
private:
	SuperModel* pSuperModel_;
	BW::string actionName_;
	int		index[1];

	SuperModelAnimation( SuperModel & superModel, const BW::string & name );
	friend class SuperModel;

	virtual void dress( SuperModel & superModel, Matrix& world );
	void pullInfoFromSuperModel( SuperModel & superModel );

};

typedef SmartPointer<SuperModelAnimation> SuperModelAnimationPtr;

BW_END_NAMESPACE


#endif // SUPER_MODEL_ANIMATION_HPP

